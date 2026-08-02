#pragma once

#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include <sys/types.h>

typedef struct {
    AdwApplication *app;
    GtkWidget *win;
    GtkSourceBuffer *buffer;
    GtkSourceView *editor;
    GtkWidget *output_label;
    GtkWidget *output_box;
    GtkWidget *run_button;
    GtkWidget *stop_button;
    char *current_file;
    pid_t matlab_pid;
} MatlabLiteApp;