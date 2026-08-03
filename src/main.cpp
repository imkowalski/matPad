#include "window.h"

int main(int argc, char **argv) {
    MatpadApp app_data = {0};

    adw_init();
    app_data.app = adw_application_new("com.michal.Matpad", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app_data.app, "activate", G_CALLBACK(on_activate), &app_data);

    int status = g_application_run(G_APPLICATION(app_data.app), argc, argv);

    if (app_data.current_file) g_free(app_data.current_file);
    if (app_data.matlab_path) g_free(app_data.matlab_path);
    if (app_data.python_path) g_free(app_data.python_path);
    if (app_data.python_prepend) g_free(app_data.python_prepend);
    g_object_unref(app_data.app);

    return status;
}