#pragma once

#include "app_context.h"

#define SETTINGS_DEFAULT_MATLAB "/usr/local/bin/matlab"
#define SETTINGS_DEFAULT_PYTHON "python3"

void settings_load(MatpadApp *self);
void settings_save(MatpadApp *self);
void settings_apply_editor_language(MatpadApp *self);
const char *settings_interpreter(MatpadApp *self);
void settings_dialog_show(MatpadApp *self);