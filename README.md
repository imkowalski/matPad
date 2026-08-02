# MATLAB Lite

MATLAB Lite is a small GTK 4 + libadwaita editor for writing and running MATLAB scripts from a desktop window. It includes syntax-highlighted editing, quick actions for opening and saving scripts, inline script execution, and preview images for generated plots.

## Requirements

- CMake
- A C++20 compiler
- GTK 4
- libadwaita-1
- gtksourceview-5
- MATLAB installed at `/usr/local/bin/matlab`

## Build

```bash
make build
```

Or with CMake directly:

```bash
cmake -B build
cmake --build build
```

## Run

```bash
make run
```

The `run` target clears the build directory, rebuilds the app, and launches it.

## Install

Run the installer script to build the app, install the binary into `~/.local/bin`, copy the app icon into the local icon theme, and create a desktop entry for GNOME and KDE:

```bash
bash installer/install.sh
```

The install script updates existing files in place, so rerunning it upgrades the local install.

Installed desktop launchers use the icon from `icon.png`, which is installed as `matlab-lite` in the local icon theme.

## Features

- Open, edit, and save MATLAB scripts
- Run scripts from the toolbar or `Ctrl+Enter`
- View generated figure previews in a separate window
- Save expanded preview images from the preview window