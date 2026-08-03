#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SOURCE_BINARY="$BUILD_DIR/matpad"

APP_ID="com.michal.Matpad"
BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor"

BIN_TARGET="$BIN_DIR/matpad"
DESKTOP_TARGET="$APP_DIR/$APP_ID.desktop"
ICON_SRC_DIR="$PROJECT_ROOT/data/icons/hicolor"
DESKTOP_TEMPLATE="$SCRIPT_DIR/matpad.desktop.in"

mkdir -p "$BIN_DIR" "$APP_DIR"

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"

install -Dm755 "$SOURCE_BINARY" "$BIN_TARGET"

# Remove legacy desktop entry name
rm -f "$APP_DIR/matpad.desktop"

# Install the themed icon at all sizes
cp -r "$ICON_SRC_DIR/." "$ICON_DIR/"

sed \
    -e "s|@BIN@|$BIN_TARGET|g" \
    -e "s|@ICON@|matpad|g" \
    "$DESKTOP_TEMPLATE" > "$DESKTOP_TARGET"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DIR" >/dev/null 2>&1 || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
fi

printf 'Installed %s\n' "$BIN_TARGET"
printf 'Installed desktop entry %s\n' "$DESKTOP_TARGET"
printf 'Installed icon %s\n' "$ICON_DIR"