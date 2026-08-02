#include "runner.h"

#include "app_utils.h"
#include "image_preview.h"

#include <adwaita.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <filesystem>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define MATLAB_PATH "/usr/local/bin/matlab"

typedef struct {
    MatlabLiteApp *self;
    std::string output_text;
    std::vector<std::string> figure_paths;
} ThreadResult;

static void on_figure_clicked(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data) {
    (void)gesture;
    (void)n_press;
    (void)x;
    (void)y;

    GtkWidget *picture = GTK_WIDGET(user_data);
    const char *fig_path = static_cast<const char *>(g_object_get_data(G_OBJECT(picture), "figure-path"));
    if (fig_path) {
        show_figure_preview(fig_path);
    }
}

static void remove_old_figure_cache(const std::string &temp_dir, const std::string &base_name) {
    namespace fs = std::filesystem;

    const std::string prefix = base_name + "_Plot_";

    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(temp_dir, ec)) {
        if (ec) {
            break;
        }

        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }

        const std::string filename = entry.path().filename().string();
        if (filename.rfind(prefix, 0) == 0 && entry.path().extension() == ".png") {
            fs::remove(entry.path(), ec);
            ec.clear();
        }
    }
}

static gboolean update_ui_after_run(gpointer user_data) {
    ThreadResult *res = static_cast<ThreadResult *>(user_data);
    MatlabLiteApp *self = res->self;

    GtkWidget *child = gtk_widget_get_first_child(self->output_box);
    while (child != nullptr) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        if (child != self->output_label) {
            gtk_box_remove(GTK_BOX(self->output_box), child);
        }
        child = next;
    }

    gtk_label_set_text(GTK_LABEL(self->output_label), res->output_text.c_str());

    for (const auto &fig_path : res->figure_paths) {
        GtkWidget *picture = gtk_picture_new_for_filename(fig_path.c_str());
        gtk_picture_set_can_shrink(GTK_PICTURE(picture), TRUE);
        gtk_widget_set_size_request(picture, 450, 300);
        gtk_widget_add_css_class(picture, "clickable-image");
        gtk_widget_set_hexpand(picture, FALSE);
        gtk_widget_set_halign(picture, GTK_ALIGN_START);

        g_object_set_data_full(G_OBJECT(picture), "figure-path", g_strdup(fig_path.c_str()), g_free);

        GtkGesture *click = gtk_gesture_click_new();
        g_signal_connect(click, "pressed", G_CALLBACK(on_figure_clicked), picture);
        gtk_widget_add_controller(picture, GTK_EVENT_CONTROLLER(click));

        gtk_box_append(GTK_BOX(self->output_box), picture);
    }

    gtk_widget_set_sensitive(self->run_button, TRUE);
    gtk_widget_set_sensitive(self->stop_button, FALSE);
    self->matlab_pid = 0;

    delete res;
    return G_SOURCE_REMOVE;
}

static void execute_matlab_thread(MatlabLiteApp *self, std::string code) {
    std::string base_name;

    if (self->current_file) {
        std::string path(self->current_file);
        size_t idx = path.find_last_of("/\\");
        std::string filename = (idx == std::string::npos) ? path : path.substr(idx + 1);
        size_t last_dot = filename.find_last_of('.');
        base_name = (last_dot == std::string::npos) ? filename : filename.substr(0, last_dot);
    } else {
        base_name = "Untitled_Plot";
    }

    std::string temp_dir = g_get_tmp_dir();
    std::string fig_prefix = temp_dir + "/" + base_name + "_Plot";

    remove_old_figure_cache(temp_dir, base_name);

    char script_path[] = "/tmp/matlab_script_XXXXXX.m";
    int fd = mkstemps(script_path, 2);
    if (fd == -1) return;
    close(fd);

    std::string script_path_str(script_path);
    size_t last_slash = script_path_str.find_last_of('/');
    std::string script_dir = script_path_str.substr(0, last_slash);
    std::string script_name = script_path_str.substr(last_slash + 1);
    script_name = script_name.substr(0, script_name.length() - 2);

    std::string matlab_wrapper =
        "warning('off', 'all');\n"
        "set(0, 'DefaultFigureVisible', 'off');\n"
        "try\n" + code + "\n"
        "    figs = findobj('Type', 'figure');\n"
        "    for idx = 1:length(figs)\n"
        "        fig_file = sprintf('" + fig_prefix + "_%d.png', idx);\n"
        "        saveas(figs(idx), fig_file);\n"
        "    end\n"
        "catch ME\n"
        "    disp(ME.message);\n"
        "end\n";

    std::ofstream script_file(script_path_str);
    script_file << matlab_wrapper;
    script_file.close();

    std::string output_text;
    std::vector<std::string> figure_paths;

    try {
        setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
        setenv("MESA_DEBUG", "0", 1);
        setenv("QT_X11_NO_MITSHM", "1", 1);

        std::string cmd = std::string(MATLAB_PATH) + " -batch \"warning('off','all'); addpath('" +
                          script_dir + "'); " + script_name + ";\" 2>&1";

        FILE *pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::string raw_output;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                raw_output += buffer;
            }
            pclose(pipe);

            std::string ansi_clean;
            for (size_t i = 0; i < raw_output.size(); ++i) {
                if (raw_output[i] == '\x1b' && i + 1 < raw_output.size() && raw_output[i + 1] == '[') {
                    i += 2;
                    while (i < raw_output.size() && !isalpha(static_cast<unsigned char>(raw_output[i]))) {
                        i++;
                    }
                } else {
                    ansi_clean += raw_output[i];
                }
            }

            std::string printable_clean;
            for (char ch : ansi_clean) {
                if (ch >= 32 || ch == '\n' || ch == '\t') {
                    printable_clean += ch;
                }
            }

            raw_output = printable_clean;
            std::regex warning_pattern(
                "(libGL|Mesa|Fontconfig|Gtk-WARNING|DirectRendering|dri3|OpenGL|pci|Driver|XServer|Graphics acceleration|software opengl|Warning:|performance might be diminished|System Requirements|In alternatePrintPath|In print|In saveas|In tmp)",
                std::regex_constants::icase
            );

            std::stringstream ss(raw_output);
            std::string line;
            std::string clean_output;

            while (std::getline(ss, line)) {
                if (!std::regex_search(line, warning_pattern)) {
                    clean_output += line + "\n";
                }
            }

            output_text = clean_output;

            int idx = 1;
            while (true) {
                std::string path = fig_prefix + "_" + std::to_string(idx) + ".png";
                if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
                    figure_paths.push_back(path);
                    idx++;
                } else {
                    break;
                }
            }
        }
    } catch (const std::exception &e) {
        output_text = e.what();
    }

    unlink(script_path_str.c_str());

    ThreadResult *res = new ThreadResult{self, output_text, figure_paths};
    g_idle_add(update_ui_after_run, res);
}

void on_run_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->buffer), &start, &end);
    char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(self->buffer), &start, &end, TRUE);
    std::string code(text);
    g_free(text);

    if (code.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
        return;
    }

    gtk_widget_set_sensitive(self->run_button, FALSE);
    gtk_widget_set_sensitive(self->stop_button, TRUE);
    gtk_label_set_text(GTK_LABEL(self->output_label), "Running script...");

    std::thread(execute_matlab_thread, self, code).detach();
}

void on_stop_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    std::system("pkill -f /usr/local/bin/matlab");

    gtk_label_set_text(GTK_LABEL(self->output_label), "Execution stopped by user.");
    gtk_widget_set_sensitive(self->stop_button, FALSE);
    gtk_widget_set_sensitive(self->run_button, TRUE);
}