#include <adwaita.h>
#include <gtksourceview/gtksource.h>

#include "window.h"

#include "app_utils.h"
#include "runner.h"

#include <fstream>
#include <sstream>
#include <string>

static const char *APP_VERSION = "1.0.0";

static const char *CSS = R"css(
.editor-box {
    padding: 10px;
}

.output-box {
    padding: 10px;
    margin: 10px;
}

.sourceview, .sourceview text, .sourceview gutter {
    background-color: transparent;
    color: #ffffff;
}

.sourceview gutter {
    color: #888888;
    padding-right: 8px;
}

.sourceview {
    border-radius: 8px;
}

.output-text {
    font-family: monospace;
    font-size: 13px;
    color: #ffffff;
}

.clickable-image {
    border-radius: 6px;
    margin-top: 8px;
    margin-bottom: 8px;
}

.clickable-image:hover {
    opacity: 0.85;
    cursor: pointer;
}
)css";

static void on_open_dialog_finish(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    GError *error = nullptr;
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    if (file) {
        char *path = g_file_get_path(file);
        std::ifstream in(path);
        if (in.is_open()) {
            std::stringstream buffer;
            buffer << in.rdbuf();
            gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->buffer), buffer.str().c_str(), -1);

            if (self->current_file) g_free(self->current_file);
            self->current_file = path;
            update_title(self);
            gtk_widget_grab_focus(GTK_WIDGET(self->editor));
        } else {
            g_free(path);
        }
        g_object_unref(file);
    }
    if (error) g_error_free(error);
}

static void on_save_dialog_finish(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(dialog, result, &error);
    if (file) {
        char *path = g_file_get_path(file);
        write_to_file(self, path);
        if (self->current_file) g_free(self->current_file);
        self->current_file = path;
        update_title(self);
        g_object_unref(file);
    }
    if (error) g_error_free(error);
}

static void action_new_file(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->buffer), "\n", -1);
    if (self->current_file) {
        g_free(self->current_file);
        self->current_file = nullptr;
    }
    update_title(self);
    gtk_widget_grab_focus(GTK_WIDGET(self->editor));
}

static void action_open_file(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open MATLAB Script");
    gtk_file_dialog_open(dialog, GTK_WINDOW(self->win), nullptr, on_open_dialog_finish, self);
    g_object_unref(dialog);
}

static void action_save_file_as(GSimpleAction *action, GVariant *param, gpointer user_data);

static void action_save_file(GSimpleAction *action, GVariant *param, gpointer user_data) {
    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);
    if (self->current_file) {
        write_to_file(self, self->current_file);
    } else {
        action_save_file_as(action, param, user_data);
    }
}

static void action_save_file_as(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Save MATLAB Script");
    gtk_file_dialog_save(dialog, GTK_WINDOW(self->win), nullptr, on_save_dialog_finish, self);
    g_object_unref(dialog);
}

static void action_app_info(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    AdwAboutDialog *about = ADW_ABOUT_DIALOG(adw_about_dialog_new());
    adw_about_dialog_set_application_name(about, "MATLAB Lite");
    adw_about_dialog_set_application_icon(about, "matlab-lite");
    adw_about_dialog_set_version(about, APP_VERSION);
    adw_about_dialog_set_developer_name(about, "MATLAB Lite");
    adw_about_dialog_set_comments(about, "Desktop MATLAB editor with script execution and plot previews.");

    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(self->win));
    gtk_window_present(GTK_WINDOW(about));
}

static void action_run_script(GSimpleAction *action, GVariant *param, gpointer user_data) {
    (void)action;
    (void)param;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);
    if (gtk_widget_is_sensitive(self->run_button)) {
        on_run_clicked(self->run_button, self);
    }
}

static void add_app_action(MatlabLiteApp *self, const char *name, GCallback callback, const char *accel) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_signal_connect(action, "activate", callback, self);
    g_action_map_add_action(G_ACTION_MAP(self->app), G_ACTION(action));

    if (accel) {
        const char *accels[] = {accel, nullptr};
        gtk_application_set_accels_for_action(GTK_APPLICATION(self->app), (std::string("app.") + name).c_str(), accels);
    }
}

void on_activate(GtkApplication *app, gpointer user_data) {
    (void)app;

    MatlabLiteApp *self = static_cast<MatlabLiteApp *>(user_data);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(css);

    self->win = adw_application_window_new(GTK_APPLICATION(self->app));
    gtk_window_set_icon_name(GTK_WINDOW(self->win), "matlab-lite");
    update_title(self);
    gtk_window_set_default_size(GTK_WINDOW(self->win), 900, 650);

    GtkWidget *header = adw_header_bar_new();

    GMenu *menu = g_menu_new();
    g_menu_append(menu, "New", "app.new_file");
    g_menu_append(menu, "Open...", "app.open_file");
    g_menu_append(menu, "Save", "app.save_file");
    g_menu_append(menu, "Save As...", "app.save_file_as");
    g_menu_append(menu, "App Info", "app.app_info");

    GtkWidget *menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "document-open-symbolic");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
    adw_header_bar_pack_start(ADW_HEADER_BAR(header), menu_button);

    self->run_button = gtk_button_new_with_label("▶ Run");
    g_signal_connect(self->run_button, "clicked", G_CALLBACK(on_run_clicked), self);
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), self->run_button);

    self->stop_button = gtk_button_new_with_label("⏹ Stop");
    gtk_widget_set_sensitive(self->stop_button, FALSE);
    g_signal_connect(self->stop_button, "clicked", G_CALLBACK(on_stop_clicked), self);
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), self->stop_button);

    add_app_action(self, "new_file", G_CALLBACK(action_new_file), "<Control>n");
    add_app_action(self, "open_file", G_CALLBACK(action_open_file), "<Control>o");
    add_app_action(self, "save_file", G_CALLBACK(action_save_file), "<Control>s");
    add_app_action(self, "save_file_as", G_CALLBACK(action_save_file_as), "<Control><Shift>s");
    add_app_action(self, "app_info", G_CALLBACK(action_app_info), nullptr);

    GSimpleAction *run_action = g_simple_action_new("run_script", nullptr);
    g_signal_connect(run_action, "activate", G_CALLBACK(action_run_script), self);
    g_action_map_add_action(G_ACTION_MAP(self->app), G_ACTION(run_action));

    const char *run_accels[] = {"<Control>Return", "<Control>KP_Enter", nullptr};
    gtk_application_set_accels_for_action(GTK_APPLICATION(self->app), "app.run_script", run_accels);

    self->buffer = gtk_source_buffer_new(nullptr);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->buffer), "\n", -1);

    GtkSourceStyleSchemeManager *scheme_mgr = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(scheme_mgr, "adwaita-dark");
    if (!scheme) scheme = gtk_source_style_scheme_manager_get_scheme(scheme_mgr, "classic-dark");
    if (scheme) gtk_source_buffer_set_style_scheme(self->buffer, scheme);

    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *matlab_lang = gtk_source_language_manager_get_language(lang_mgr, "matlab");
    if (matlab_lang) gtk_source_buffer_set_language(self->buffer, matlab_lang);

    self->editor = GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(self->buffer));
    gtk_widget_add_css_class(GTK_WIDGET(self->editor), "sourceview");
    gtk_source_view_set_show_line_numbers(self->editor, TRUE);
    gtk_source_view_set_auto_indent(self->editor, TRUE);
    gtk_widget_set_hexpand(GTK_WIDGET(self->editor), TRUE);

    GtkTextView *editor_view = GTK_TEXT_VIEW(self->editor);
    gtk_text_view_set_monospace(editor_view, TRUE);
    gtk_text_view_set_top_margin(editor_view, 8);
    gtk_text_view_set_bottom_margin(editor_view, 8);
    gtk_text_view_set_left_margin(editor_view, 8);
    gtk_text_view_set_right_margin(editor_view, 8);

    GtkWidget *editor_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(editor_box, "editor-box");
    gtk_widget_set_hexpand(editor_box, TRUE);
    gtk_box_append(GTK_BOX(editor_box), GTK_WIDGET(self->editor));

    self->output_label = gtk_label_new("");
    gtk_widget_add_css_class(self->output_label, "output-text");
    gtk_label_set_selectable(GTK_LABEL(self->output_label), TRUE);
    gtk_label_set_wrap(GTK_LABEL(self->output_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(self->output_label), 0);

    self->output_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_add_css_class(self->output_box, "output-box");
    gtk_widget_set_hexpand(self->output_box, TRUE);
    gtk_box_append(GTK_BOX(self->output_box), self->output_label);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_hexpand(content_box, TRUE);
    gtk_box_append(GTK_BOX(content_box), editor_box);
    gtk_box_append(GTK_BOX(content_box), self->output_box);

    GtkWidget *global_scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(global_scroll, TRUE);
    gtk_widget_set_vexpand(global_scroll, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(global_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(global_scroll), content_box);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(main_box), header);
    gtk_widget_set_vexpand(main_box, TRUE);
    gtk_box_append(GTK_BOX(main_box), global_scroll);

    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self->win), main_box);
    gtk_window_present(GTK_WINDOW(self->win));

    gtk_widget_grab_focus(GTK_WIDGET(self->editor));
}