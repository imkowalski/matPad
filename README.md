# MATLAB Lite

MATLAB Lite is a GTK 4 + libadwaita editor for writing and running MATLAB scripts from a desktop window. It includes syntax-highlighted editing, quick actions for opening and saving scripts, inline script execution, and preview images for generated plots.

GitHub repository: https://github.com/imkowalski/Matlab-lite

## Features

- Open, edit, and save MATLAB scripts
- Run scripts from the toolbar or `Ctrl+Enter`
- View generated figure previews in a separate window
- Save expanded preview images from the preview window
- Open an App Info dialog from the File menu

## License

MATLAB Lite is licensed under the GNU Affero General Public License v3.0 or later.

## Requirements

- CMake
- A C++20 compiler
- GTK 4
- libadwaita-1
- gtksourceview-5
- MATLAB installed at `/usr/local/bin/matlab`

## Configuration

This build is not fully device agnostic yet. It expects MATLAB at `/usr/local/bin/matlab`, so if your system uses a different path, update the runner configuration in the source before building.
The app version is generated from Git during CMake configure time and is also used by the packaging targets.

## Build

```bash
make build
```

Or with CMake directly:

```bash
cmake -B build
cmake --build build
```

For versioned package builds, see [docs/compiling.md](docs/compiling.md).

## Run

```bash
make run
```

The `run` target clears the build directory, rebuilds the app, and launches it.

## Local Install

Run the installer script to build the app, install the binary into `~/.local/bin`, copy the app icon into the local icon theme, and create a desktop entry for GNOME and KDE:

```bash
bash installer/install.sh
```

The install script updates existing files in place, so rerunning it upgrades the local install.

The local installer is per-user and keeps files under `~/.local`.

## Packaging

The root `Makefile` includes packaging targets:

```bash
make deb
make rpm
make appimage
```

- `make deb` creates a Debian package in `dist/` using `dpkg-deb`.
- `make rpm` creates an RPM when `rpmbuild` is installed.
- `make appimage` stages an AppDir and then builds an AppImage when `linuxdeploy` and `appimagetool` are installed.

The `.deb`, `.rpm`, and `.AppImage` outputs use the app icon from `icon.png`.

To build packages for a specific version, pass `VERSION` on the command line:

```bash
make VERSION=1.2.3 deb
make VERSION=1.2.3 rpm
make VERSION=1.2.3 appimage
```

## Packaging Notes

- The `.deb` target is system-wide and installs into `/usr`.
- The RPM target is system-wide and installs into `/usr`.
- The AppImage target is a scaffold unless the extra AppImage tooling is installed.

