#include "app_utils.h"

#include <fstream>
#include <string>

void update_title(MatpadApp *self) {
    std::string title = "MatPad";
    if (self->current_file) {
        std::string path(self->current_file);
        size_t idx = path.find_last_of("/\\");
        std::string filename = (idx == std::string::npos) ? path : path.substr(idx + 1);
        title += " - " + filename;
    }
    gtk_window_set_title(GTK_WINDOW(self->win), title.c_str());
}

void write_to_file(MatpadApp *self, const char *path) {
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->buffer), &start, &end);
    char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(self->buffer), &start, &end, TRUE);

    std::ofstream out(path);
    if (out.is_open()) {
        out << text;
        out.close();
    }
    g_free(text);
}