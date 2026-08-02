#include <adwaita.h>

#include "image_preview.h"

#include <string>

typedef struct {
    std::string fig_path;
    bool save_started;
} PreviewData;

static void on_save_image_finish(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    PreviewData *data = static_cast<PreviewData *>(user_data);

    GError *error = nullptr;
    GFile *target = gtk_file_dialog_save_finish(dialog, result, &error);
    if (target) {
        GFile *source_file = g_file_new_for_path(data->fig_path.c_str());
        g_file_copy(source_file, target, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, &error);
        g_object_unref(source_file);
        g_object_unref(target);
    }

    if (error) {
        g_error_free(error);
    }
}

static void on_preview_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    PreviewData *data = static_cast<PreviewData *>(user_data);
    delete data;
}

static void on_save_button_clicked(GtkButton *button, gpointer user_data) {
    (void)button;

    PreviewData *data = static_cast<PreviewData *>(user_data);
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Save Figure As");

    GFile *initial_file = g_file_new_for_path(data->fig_path.c_str());
    gtk_file_dialog_set_initial_file(dialog, initial_file);
    g_object_unref(initial_file);

    GtkRoot *root = gtk_widget_get_root(GTK_WIDGET(button));
    GtkWindow *parent = GTK_IS_WINDOW(root) ? GTK_WINDOW(root) : nullptr;

    data->save_started = true;
    gtk_file_dialog_save(dialog, parent, nullptr, on_save_image_finish, data);
    g_object_unref(dialog);
}

void show_figure_preview(const std::string &fig_path) {
    GtkWidget *dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Expanded Figure View");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 700);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

    GError *error = nullptr;
    GFile *file = g_file_new_for_path(fig_path.c_str());
    GdkTexture *texture = gdk_texture_new_from_file(file, &error);
    g_object_unref(file);

    GtkWidget *picture = gtk_picture_new();
    if (texture) {
        gtk_picture_set_paintable(GTK_PICTURE(picture), GDK_PAINTABLE(texture));
        g_object_unref(texture);
    }

    gtk_picture_set_can_shrink(GTK_PICTURE(picture), TRUE);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), picture);

    GtkWidget *header = adw_header_bar_new();
    GtkWidget *save_button = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(save_button, "suggested-action");
    PreviewData *data = new PreviewData{fig_path, false};
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_button_clicked), data);
    adw_header_bar_pack_end(ADW_HEADER_BAR(header), save_button);

    g_signal_connect(dialog, "destroy", G_CALLBACK(on_preview_destroy), data);
    gtk_window_set_titlebar(GTK_WINDOW(dialog), header);
    gtk_window_set_child(GTK_WINDOW(dialog), scroll);
    gtk_window_present(GTK_WINDOW(dialog));

    if (error) {
        g_error_free(error);
    }
}