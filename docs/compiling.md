# Compiling MATLAB Lite

## Build From Source

```bash
make build
```

Or directly with CMake:

```bash
cmake -B build
cmake --build build
```

## Build a Specific Version

The packaging targets use the `VERSION` variable from the `Makefile`.

```bash
make VERSION=1.2.3 deb
make VERSION=1.2.3 rpm
make VERSION=1.2.3 appimage
```

That version string is written into the package filenames and metadata.

## Notes

- `make deb` requires `dpkg-deb`.
- `make rpm` requires `rpmbuild`.
- `make appimage` requires `linuxdeploy` and `appimagetool`.
