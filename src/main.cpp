#include "window.h"

int main(int argc, char **argv) {
    MatlabLiteApp app_data = {0};

    adw_init();
    app_data.app = adw_application_new("michal.Matlablite", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app_data.app, "activate", G_CALLBACK(on_activate), &app_data);

    int status = g_application_run(G_APPLICATION(app_data.app), argc, argv);

    if (app_data.current_file) g_free(app_data.current_file);
    g_object_unref(app_data.app);

    return status;
}