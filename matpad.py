#!/usr/bin/env python3

import gi
import subprocess
import tempfile
import os
import re
import threading

gi.require_version("Adw", "1")
gi.require_version("Gtk", "4.0")
gi.require_version("GtkSource", "5")

from gi.repository import Adw, Gtk, GtkSource, Gdk, GLib, Gio


MATLAB = "/usr/local/bin/matlab"


CSS = """
.editor-box {
    padding: 10px;
}

.output-box {
    padding: 10px;
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
"""


class MatPad(Adw.Application):

    def __init__(self):
        super().__init__(
            application_id="com.michal.Matpad",
            flags=Gio.ApplicationFlags.FLAGS_NONE
        )
        self.current_file = None
        self.matlab_process = None

    def do_activate(self):

        css = Gtk.CssProvider()
        css.load_from_data(CSS.encode())

        Gtk.StyleContext.add_provider_for_display(
            Gdk.Display.get_default(),
            css,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        self.win = Adw.ApplicationWindow(application=self)
        self.update_title()
        self.win.set_default_size(900, 650)

        header = Adw.HeaderBar()

        # File Operations Menu
        menu = Gio.Menu()
        menu.append("New", "app.new_file")
        menu.append("Open...", "app.open_file")
        menu.append("Save", "app.save_file")
        menu.append("Save As...", "app.save_file_as")

        menu_button = Gtk.MenuButton()
        menu_button.set_icon_name("document-open-symbolic")
        menu_button.set_menu_model(menu)
        header.pack_start(menu_button)

        # Action Buttons
        self.run_button = Gtk.Button(label="▶ Run")
        self.run_button.connect("clicked", self.on_run_clicked)
        header.pack_end(self.run_button)

        self.stop_button = Gtk.Button(label="⏹ Stop")
        self.stop_button.set_sensitive(False)
        self.stop_button.connect("clicked", self.on_stop_clicked)
        header.pack_end(self.stop_button)

        # Register App Actions & Shortcuts
        self.create_action("new_file", self.action_new_file, ["<Control>n"])
        self.create_action("open_file", self.action_open_file, ["<Control>o"])
        self.create_action("save_file", self.action_save_file, ["<Control>s"])
        self.create_action("save_file_as", self.action_save_file_as, ["<Control><Shift>s"])
        self.create_action("run_script", lambda a, p: self.on_run_clicked(None), ["<Control>Return", "<Control>KP_Enter"])
        self.create_action("stop_script", lambda a, p: self.on_stop_clicked(None), ["<Control>period"])

        self.buffer = GtkSource.Buffer()

        style_scheme_mgr = GtkSource.StyleSchemeManager.get_default()
        scheme = style_scheme_mgr.get_scheme("adwaita-dark") or style_scheme_mgr.get_scheme("classic-dark")
        if scheme:
            self.buffer.set_style_scheme(scheme)

        manager = GtkSource.LanguageManager.get_default()
        matlab = manager.get_language("matlab")
        if matlab:
            self.buffer.set_language(matlab)

        self.editor = GtkSource.View.new_with_buffer(self.buffer)
        self.editor.add_css_class("sourceview")
        self.editor.set_show_line_numbers(True)
        self.editor.set_monospace(True)
        self.editor.set_auto_indent(True)
        self.editor.set_hexpand(True)

        self.editor.set_top_margin(8)
        self.editor.set_bottom_margin(8)
        self.editor.set_left_margin(8)
        self.editor.set_right_margin(8)

        editor_box = Gtk.Box()
        editor_box.add_css_class("editor-box")
        editor_box.set_hexpand(True)
        editor_box.append(self.editor)

        # Output Display
        self.output = Gtk.Label()
        self.output.add_css_class("output-text")
        self.output.set_selectable(True)
        self.output.set_wrap(True)
        self.output.set_xalign(0)

        output_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        output_box.add_css_class("output-box")
        output_box.set_hexpand(True)
        output_box.append(self.output)

        content_box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=10
        )
        content_box.set_hexpand(True)
        content_box.append(editor_box)
        content_box.append(output_box)

        global_scroll = Gtk.ScrolledWindow()
        global_scroll.set_vexpand(True)
        global_scroll.set_hexpand(True)
        global_scroll.set_child(content_box)

        main = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=6
        )

        main.append(header)
        main.append(global_scroll)

        self.win.set_content(main)
        self.win.present()

    def update_title(self):
        title = "MatPad"
        if self.current_file:
            title += f" - {os.path.basename(self.current_file)}"
        self.win.set_title(title)

    def create_action(self, name, callback, shortcuts=None):
        action = Gio.SimpleAction.new(name, None)
        action.connect("activate", callback)
        self.add_action(action)
        if shortcuts:
            self.set_accels_for_action(f"app.{name}", shortcuts)

    # File Operations
    def action_new_file(self, action, param):
        self.buffer.set_text("")
        self.current_file = None
        self.update_title()

    def action_open_file(self, action, param):
        dialog = Gtk.FileDialog(title="Open MATLAB Script")
        dialog.open(self.win, None, self.on_open_dialog_finish)

    def on_open_dialog_finish(self, dialog, result):
        try:
            file = dialog.open_finish(result)
            if file:
                path = file.get_path()
                with open(path, "r") as f:
                    self.buffer.set_text(f.read())
                self.current_file = path
                self.update_title()
        except Exception:
            pass

    def action_save_file(self, action, param):
        if self.current_file:
            self.write_to_file(self.current_file)
        else:
            self.action_save_file_as(action, param)

    def action_save_file_as(self, action, param):
        dialog = Gtk.FileDialog(title="Save MATLAB Script")
        dialog.save(self.win, None, self.on_save_dialog_finish)

    def on_save_dialog_finish(self, dialog, result):
        try:
            file = dialog.save_finish(result)
            if file:
                path = file.get_path()
                self.write_to_file(path)
                self.current_file = path
                self.update_title()
        except Exception:
            pass

    def write_to_file(self, path):
        text = self.buffer.get_text(
            self.buffer.get_start_iter(),
            self.buffer.get_end_iter(),
            True
        )
        with open(path, "w") as f:
            f.write(text)

    # Execution Handling
    def on_run_clicked(self, button):
        if self.matlab_process is not None:
            return

        code = self.buffer.get_text(
            self.buffer.get_start_iter(),
            self.buffer.get_end_iter(),
            True
        )

        if not code.strip():
            return

        self.run_button.set_sensitive(False)
        self.stop_button.set_sensitive(True)
        self.output.set_text("Running script...")

        threading.Thread(
            target=self.execute_matlab_thread,
            args=(code,),
            daemon=True
        ).start()

    def on_stop_clicked(self, button):
        if self.matlab_process and self.matlab_process.poll() is None:
            self.matlab_process.terminate()
            self.output.set_text("Execution stopped by user.")
        self.stop_button.set_sensitive(False)
        self.run_button.set_sensitive(True)

    def execute_matlab_thread(self, code):
        # Determine pretty name for saved plot file
        if self.current_file:
            base_name = os.path.splitext(os.path.basename(self.current_file))[0]
            pretty_title = base_name.replace("_", " ").title()
        else:
            base_name = "Untitled_Plot"
            pretty_title = "Untitled Script"

        temp_dir = tempfile.gettempdir()
        fig_path = os.path.join(temp_dir, f"{base_name}_Plot.png")

        with tempfile.NamedTemporaryFile(suffix=".m", mode="w", delete=False) as script_f:
            script_path = script_f.name
            script_dir = os.path.dirname(script_path)
            script_name = os.path.splitext(os.path.basename(script_path))[0]

        # MATLAB script wrapper sets clean figure window title and names saved file cleanly
        matlab_wrapper = f"""
warning('off', 'all');
set(0, 'DefaultFigureVisible', 'off');
try
    {code}
    
    figs = findobj('Type', 'figure');
    if ~isempty(figs)
        set(figs(1), 'Name', '{pretty_title}', 'NumberTitle', 'off');
        saveas(figs(1), '{fig_path}');
    end
catch ME
    disp(ME.message);
end
"""

        with open(script_path, "w") as f:
            f.write(matlab_wrapper)

        output_text = ""
        try:
            env = os.environ.copy()
            env["LIBGL_ALWAYS_SOFTWARE"] = "1"
            env["MESA_DEBUG"] = "0"
            env["QT_X11_NO_MITSHM"] = "1"

            self.matlab_process = subprocess.Popen(
                [
                    MATLAB,
                    "-batch",
                    f"warning('off','all'); addpath('{script_dir}'); {script_name};"
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                env=env
            )

            stdout, stderr = self.matlab_process.communicate()
            raw_output = stdout + ("\n" + stderr if stderr else "")

            # Removes ANSI/terminal control characters and specific MATLAB warnings/stacktraces
            raw_output = re.sub(r'\x1b\[[0-9;]*[a-zA-Z]', '', raw_output)
            raw_output = re.sub(r'[\x00-\x08\x0b\x0c\x0e-\x1f]', '', raw_output)

            warning_patterns = re.compile(
                r"(libGL|Mesa|Fontconfig|Gtk-WARNING|DirectRendering|dri3|OpenGL|pci|Driver|XServer|Graphics acceleration|software opengl|Warning:|performance might be diminished|System Requirements|In alternatePrintPath|In print|In saveas|In tmp)",
                re.IGNORECASE
            )

            clean_lines = []
            for line in raw_output.splitlines():
                if warning_patterns.search(line):
                    continue
                clean_lines.append(line)

            output_text = "\n".join(clean_lines).strip()

            if os.path.exists(fig_path):
                subprocess.Popen(["xdg-open", fig_path])

        except Exception as e:
            output_text = str(e)

        finally:
            if os.path.exists(script_path):
                os.unlink(script_path)

        GLib.idle_add(self.update_ui_after_run, output_text)

    def update_ui_after_run(self, text):
        self.output.set_text(text)
        self.run_button.set_sensitive(True)
        self.stop_button.set_sensitive(False)
        self.matlab_process = None


app = MatPad()
app.run()
