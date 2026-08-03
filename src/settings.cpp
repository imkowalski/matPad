#include "settings.h"

#include <adwaita.h>
#include <glib/gstdio.h>

#include <string>

static std::string settings_dir() {
    return std::string(g_get_user_config_dir()) + "/matpad";
}

static std::string settings_file() {
    return settings_dir() + "/settings.ini";
}

void settings_load(MatpadApp *self) {
    GKeyFile *kf = g_key_file_new();
    if (g_key_file_load_from_file(kf, settings_file().c_str(), G_KEY_FILE_NONE, nullptr)) {
        self->use_python = g_key_file_get_boolean(kf, "backend", "use-python", nullptr);

        gchar *matlab = g_key_file_get_string(kf, "backend", "matlab-path", nullptr);
        if (matlab && *matlab) {
            g_free(self->matlab_path);
            self->matlab_path = matlab;
        } else if (matlab) {
            g_free(matlab);
        }

        gchar *python = g_key_file_get_string(kf, "backend", "python-path", nullptr);
        if (python && *python) {
            g_free(self->python_path);
            self->python_path = python;
        } else if (python) {
            g_free(python);
        }

        gchar *prepend = g_key_file_get_string(kf, "backend", "python-prepend", nullptr);
        if (prepend) {
            g_free(self->python_prepend);
            self->python_prepend = prepend;
        }
    }
    g_key_file_free(kf);

    if (!self->matlab_path) self->matlab_path = g_strdup(SETTINGS_DEFAULT_MATLAB);
    if (!self->python_path) self->python_path = g_strdup(SETTINGS_DEFAULT_PYTHON);
}

void settings_save(MatpadApp *self) {
    GKeyFile *kf = g_key_file_new();
    g_key_file_set_boolean(kf, "backend", "use-python", self->use_python);
    g_key_file_set_string(kf, "backend", "matlab-path",
                          self->matlab_path ? self->matlab_path : SETTINGS_DEFAULT_MATLAB);
    g_key_file_set_string(kf, "backend", "python-path",
                          self->python_path ? self->python_path : SETTINGS_DEFAULT_PYTHON);
    g_key_file_set_string(kf, "backend", "python-prepend",
                          self->python_prepend ? self->python_prepend : "");

    g_mkdir_with_parents(settings_dir().c_str(), 0700);
    gchar *data = g_key_file_to_data(kf, nullptr, nullptr);
    if (data) {
        g_file_set_contents(settings_file().c_str(), data, -1, nullptr);
        g_free(data);
    }
    g_key_file_free(kf);
}

void settings_apply_editor_language(MatpadApp *self) {
    if (!self->buffer) return;

    GtkSourceLanguageManager *lang_mgr = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = nullptr;

    if (self->use_python) {
        lang = gtk_source_language_manager_get_language(lang_mgr, "python3");
        if (!lang) lang = gtk_source_language_manager_get_language(lang_mgr, "python");
    } else {
        lang = gtk_source_language_manager_get_language(lang_mgr, "matlab");
    }

    if (lang) gtk_source_buffer_set_language(self->buffer, lang);
}

const char *settings_interpreter(MatpadApp *self) {
    return self->use_python
        ? (self->python_path ? self->python_path : SETTINGS_DEFAULT_PYTHON)
        : (self->matlab_path ? self->matlab_path : SETTINGS_DEFAULT_MATLAB);
}

static void on_pick_interpreter_finished(GObject *source, GAsyncResult *result, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GtkEntry *entry = GTK_ENTRY(user_data);

    GError *error = nullptr;
    GFile *file = gtk_file_dialog_open_finish(dialog, result, &error);
    if (file) {
        char *path = g_file_get_path(file);
        gtk_editable_set_text(GTK_EDITABLE(entry), path);
        g_free(path);
        g_object_unref(file);
    }
    if (error) g_error_free(error);
}

static void on_pick_interpreter(GtkButton *button, gpointer user_data) {    (void)button;
    GtkEntry *entry = GTK_ENTRY(user_data);

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Executable");
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Executables");
    gtk_file_filter_add_mime_type(filter, "application/x-executable");
    gtk_file_filter_add_mime_type(filter, "application/x-sharedlib");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_dialog_set_default_filter(dialog, filter);
    gtk_file_dialog_open(dialog, nullptr, nullptr, on_pick_interpreter_finished, entry);
    g_object_unref(dialog);
}

typedef struct {
    MatpadApp *self;
    GtkEntry *matlab_entry;
    GtkEntry *python_entry;
    AdwSwitchRow *python_row;
    GtkTextBuffer *prefix_buf;
} SettingsDialogData;

static void on_settings_dialog_closed(AdwDialog *dialog, gpointer user_data) {
    (void)dialog;
    delete static_cast<SettingsDialogData *>(user_data);
}

static void on_matlab_entry_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    MatpadApp *self = static_cast<MatpadApp *>(user_data);
    GtkEntry *entry = GTK_ENTRY(editable);
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    g_free(self->matlab_path);
    self->matlab_path = g_strdup((*text) ? text : SETTINGS_DEFAULT_MATLAB);
    settings_save(self);
}

static void on_python_entry_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    MatpadApp *self = static_cast<MatpadApp *>(user_data);
    GtkEntry *entry = GTK_ENTRY(editable);
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    g_free(self->python_path);
    self->python_path = g_strdup((*text) ? text : SETTINGS_DEFAULT_PYTHON);
    settings_save(self);
}

static void on_python_toggle(GObject *object, GParamSpec *pspec, gpointer user_data) {
    (void)object;
    (void)pspec;
    SettingsDialogData *data = static_cast<SettingsDialogData *>(user_data);

    data->self->use_python = adw_switch_row_get_active(data->python_row);
    gtk_widget_set_sensitive(GTK_WIDGET(data->python_entry), data->self->use_python);
    gtk_widget_set_sensitive(GTK_WIDGET(data->matlab_entry), !data->self->use_python);

    settings_apply_editor_language(data->self);
    settings_save(data->self);
}

static void on_prefix_changed(GtkTextBuffer *buffer, gpointer user_data) {
    MatpadApp *self = static_cast<MatpadApp *>(user_data);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

    g_free(self->python_prepend);
    self->python_prepend = g_strdup(text);
    g_free(text);
    settings_save(self);
}

static void on_reset_defaults(GtkButton *button, gpointer user_data) {
    (void)button;
    SettingsDialogData *data = static_cast<SettingsDialogData *>(user_data);

    data->self->use_python = FALSE;
    adw_switch_row_set_active(data->python_row, FALSE);

    g_free(data->self->matlab_path);
    data->self->matlab_path = g_strdup(SETTINGS_DEFAULT_MATLAB);
    gtk_editable_set_text(GTK_EDITABLE(data->matlab_entry), SETTINGS_DEFAULT_MATLAB);

    g_free(data->self->python_path);
    data->self->python_path = g_strdup(SETTINGS_DEFAULT_PYTHON);
    gtk_editable_set_text(GTK_EDITABLE(data->python_entry), SETTINGS_DEFAULT_PYTHON);

    g_free(data->self->python_prepend);
    data->self->python_prepend = nullptr;
    gtk_text_buffer_set_text(data->prefix_buf, "", -1);

    settings_apply_editor_language(data->self);
    settings_save(data->self);
}

void settings_dialog_show(MatpadApp *self) {
    AdwPreferencesDialog *prefs = ADW_PREFERENCES_DIALOG(adw_preferences_dialog_new());
    adw_dialog_set_title(ADW_DIALOG(prefs), "Settings");

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());

    AdwPreferencesGroup *backend_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(backend_group, "Execution Backend");
    adw_preferences_group_set_description(
        backend_group,
        "Choose the interpreter used to run scripts. MATLAB mode also controls the editor language.");

    AdwSwitchRow *python_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(python_row), "Use Python instead of MATLAB");
    adw_switch_row_set_active(python_row, self->use_python);
    adw_preferences_group_add(backend_group, GTK_WIDGET(python_row));

    AdwActionRow *matlab_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(matlab_row), "MATLAB executable");
    
    GtkEntry *matlab_entry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(matlab_entry),
                          self->matlab_path ? self->matlab_path : SETTINGS_DEFAULT_MATLAB);
    gtk_editable_set_editable(GTK_EDITABLE(matlab_entry), FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(matlab_entry), 300, -1);

    GtkButton *matlab_browse = GTK_BUTTON(gtk_button_new_from_icon_name("folder-open-symbolic"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(matlab_browse), "Browse...");
    gtk_widget_set_valign(GTK_WIDGET(matlab_browse), GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(GTK_WIDGET(matlab_browse), 36, 36);

    GtkWidget *matlab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(matlab_box), GTK_WIDGET(matlab_entry));
    gtk_box_append(GTK_BOX(matlab_box), GTK_WIDGET(matlab_browse));
    gtk_widget_set_valign(matlab_box, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(ADW_ACTION_ROW(matlab_row), matlab_box);
    adw_preferences_group_add(backend_group, GTK_WIDGET(matlab_row));

    AdwActionRow *python_row_edit = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(python_row_edit), "Python interpreter");

    GtkEntry *python_entry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(python_entry),
                          self->python_path ? self->python_path : SETTINGS_DEFAULT_PYTHON);
    gtk_editable_set_editable(GTK_EDITABLE(python_entry), FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(python_entry), 300, -1);

    GtkButton *python_browse = GTK_BUTTON(gtk_button_new_from_icon_name("folder-open-symbolic"));
    gtk_widget_set_tooltip_text(GTK_WIDGET(python_browse), "Browse...");
    gtk_widget_set_valign(GTK_WIDGET(python_browse), GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(GTK_WIDGET(python_browse), 36, 36);

    GtkWidget *python_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(python_box), GTK_WIDGET(python_entry));
    gtk_box_append(GTK_BOX(python_box), GTK_WIDGET(python_browse));
    gtk_widget_set_valign(python_box, GTK_ALIGN_CENTER);
    adw_action_row_add_suffix(ADW_ACTION_ROW(python_row_edit), python_box);
    adw_preferences_group_add(backend_group, GTK_WIDGET(python_row_edit));

    adw_preferences_page_add(page, backend_group);

    AdwPreferencesGroup *prefix_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(prefix_group, "Python Prefix Script");
    adw_preferences_group_set_description(
        prefix_group,
        "Code prepended to the editor contents on every run while Python is selected, "
        "e.g. imports such as \"import numpy as np\".");

    GtkWidget *prefix_row = gtk_list_box_row_new();
    gtk_widget_set_hexpand(prefix_row, TRUE);
    GtkScrolledWindow *prefix_sw = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(prefix_sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_has_frame(prefix_sw, TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(prefix_sw), -1, 160);
    GtkTextView *prefix_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_monospace(prefix_view, TRUE);
    gtk_text_view_set_wrap_mode(prefix_view, GTK_WRAP_WORD_CHAR);
    gtk_widget_set_margin_top(GTK_WIDGET(prefix_view), 6);
    gtk_widget_set_margin_bottom(GTK_WIDGET(prefix_view), 6);
    gtk_widget_set_margin_start(GTK_WIDGET(prefix_view), 6);
    gtk_widget_set_margin_end(GTK_WIDGET(prefix_view), 6);
    gtk_scrolled_window_set_child(prefix_sw, GTK_WIDGET(prefix_view));
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(prefix_row), GTK_WIDGET(prefix_sw));
    adw_preferences_group_add(prefix_group, prefix_row);

    adw_preferences_page_add(page, prefix_group);

    AdwPreferencesGroup *actions_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(actions_group, "Actions");

    GtkWidget *reset_button = gtk_button_new_with_label("Reset to Defaults");
    gtk_widget_set_halign(reset_button, GTK_ALIGN_START);
    gtk_widget_set_valign(reset_button, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_end(reset_button, 6);
    gtk_widget_set_margin_top(reset_button, 0);
    gtk_widget_set_margin_bottom(reset_button, 0);
    GtkWidget *reset_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_append(GTK_BOX(reset_box), reset_button);
    gtk_widget_set_valign(reset_box, GTK_ALIGN_CENTER);
    AdwActionRow *reset_row = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(reset_row), "Reset settings");
    adw_action_row_add_suffix(ADW_ACTION_ROW(reset_row), reset_box);
    adw_preferences_group_add(actions_group, GTK_WIDGET(reset_row));

    adw_preferences_page_add(page, actions_group);

    adw_preferences_dialog_add(ADW_PREFERENCES_DIALOG(prefs), page);

    GtkTextBuffer *prefix_buf = gtk_text_view_get_buffer(prefix_view);
    if (self->python_prepend) gtk_text_buffer_set_text(prefix_buf, self->python_prepend, -1);

    SettingsDialogData *data = new SettingsDialogData{self, matlab_entry, python_entry, python_row, prefix_buf};

    g_signal_connect(matlab_entry, "changed", G_CALLBACK(on_matlab_entry_changed), self);
    g_signal_connect(python_entry, "changed", G_CALLBACK(on_python_entry_changed), self);
    g_signal_connect(matlab_browse, "clicked", G_CALLBACK(on_pick_interpreter), matlab_entry);
    g_signal_connect(python_browse, "clicked", G_CALLBACK(on_pick_interpreter), python_entry);
    g_signal_connect(python_row, "notify::active", G_CALLBACK(on_python_toggle), data);
    g_signal_connect(prefix_buf, "changed", G_CALLBACK(on_prefix_changed), self);
    g_signal_connect(reset_button, "clicked", G_CALLBACK(on_reset_defaults), data);
    g_signal_connect(prefs, "closed", G_CALLBACK(on_settings_dialog_closed), data);

    gtk_widget_set_sensitive(GTK_WIDGET(python_entry), self->use_python);
    gtk_widget_set_sensitive(GTK_WIDGET(matlab_entry), !self->use_python);

    adw_dialog_present(ADW_DIALOG(prefs), self->win);
}
