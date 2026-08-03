#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SOURCE_BINARY="$BUILD_DIR/matpad"

BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

BIN_TARGET="$BIN_DIR/matpad"
DESKTOP_TARGET="$APP_DIR/matpad.desktop"
ICON_TARGET="$ICON_DIR/matpad.png"
DESKTOP_TEMPLATE="$SCRIPT_DIR/matpad.desktop.in"

mkdir -p "$BIN_DIR" "$APP_DIR" "$ICON_DIR"

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

install -Dm755 "$SOURCE_BINARY" "$BIN_TARGET"
install -Dm644 "$PROJECT_ROOT/icon.png" "$ICON_TARGET"

sed \
    -e "s|__BIN__|$BIN_TARGET|g" \
    -e "s|__ICON__|matpad|g" \
    "$DESKTOP_TEMPLATE" > "$DESKTOP_TARGET"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DIR" >/dev/null 2>&1 || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

printf 'Installed %s\n' "$BIN_TARGET"
printf 'Installed desktop entry %s\n' "$DESKTOP_TARGET"
printf 'Installed icon %s\n' "$ICON_TARGET"