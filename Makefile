.PHONY: all build clean run install-local deb rpm appimage

APP_NAME := matlab-lite
APP_ID := com.michal.MatlabLite
VERSION ?= $(shell git describe --tags --dirty --always --abbrev=7 2>/dev/null || echo 1.0.0)
PROJECT_ROOT := $(CURDIR)
BUILD_DIR := build
DIST_DIR := dist
PACKAGE_DIR := $(BUILD_DIR)/packages
DEB_ROOT := $(PACKAGE_DIR)/deb-root
RPM_TOP := $(PACKAGE_DIR)/rpmbuild
APPIMAGE_DIR := $(PACKAGE_DIR)/AppDir
DEB_ARCH := $(shell dpkg --print-architecture 2>/dev/null || uname -m)
RPM_ARCH := $(shell uname -m)
DEB_DEPENDS := libgtk-4-1, libadwaita-1-0, libgtksourceview-5-0
RPM_REQUIRES := gtk4, libadwaita, gtksourceview5

all: build
	@cmake --build build

build:
	@cmake -B build

clean:
	@rm -rf build

run: clean all 
	@./build/matlab-lite

install-local: build
	@bash installer/install.sh

deb: build
	@rm -rf "$(DEB_ROOT)" "$(DIST_DIR)"
	@mkdir -p "$(DEB_ROOT)/DEBIAN" \
		"$(DEB_ROOT)/usr/bin" \
		"$(DEB_ROOT)/usr/share/applications" \
		"$(DEB_ROOT)/usr/share/icons/hicolor/256x256/apps" \
		"$(DIST_DIR)"
	@install -Dm755 "$(BUILD_DIR)/$(APP_NAME)" "$(DEB_ROOT)/usr/bin/$(APP_NAME)"
	@install -Dm644 "icon.png" "$(DEB_ROOT)/usr/share/icons/hicolor/256x256/apps/$(APP_NAME).png"
	@sed \
		-e "s|@VERSION@|$(VERSION)|g" \
		-e "s|@ARCH@|$(DEB_ARCH)|g" \
		-e "s|@DEPENDS@|$(DEB_DEPENDS)|g" \
		packaging/deb/control.in > "$(DEB_ROOT)/DEBIAN/control"
	@sed \
		-e "s|__BIN__|/usr/bin/$(APP_NAME)|g" \
		-e "s|__ICON__|$(APP_NAME)|g" \
		installer/matlab-lite.desktop.in > "$(DEB_ROOT)/usr/share/applications/$(APP_NAME).desktop"
	@dpkg-deb --build --root-owner-group "$(DEB_ROOT)" "$(DIST_DIR)/$(APP_NAME)_$(VERSION)_$(DEB_ARCH).deb"

rpm: build
	@command -v rpmbuild >/dev/null 2>&1 || { echo "rpmbuild is required to build RPMs"; exit 1; }
	@rm -rf "$(RPM_TOP)" "$(DIST_DIR)"
	@mkdir -p "$(RPM_TOP)/BUILD" \
		"$(RPM_TOP)/RPMS" \
		"$(RPM_TOP)/SOURCES" \
		"$(RPM_TOP)/SPECS" \
		"$(RPM_TOP)/SRPMS" \
		"$(DIST_DIR)"
	@sed \
		-e "s|@VERSION@|$(VERSION)|g" \
		-e "s|@ARCH@|$(RPM_ARCH)|g" \
		-e "s|@PROJECT_ROOT@|$(PROJECT_ROOT)|g" \
		-e "s|@BUILD_DIR@|$(abspath $(BUILD_DIR))|g" \
		-e "s|@REQUIRES@|$(RPM_REQUIRES)|g" \
		packaging/rpm/matlab-lite.spec.in > "$(RPM_TOP)/SPECS/$(APP_NAME).spec"
	@sed \
		-e "s|__BIN__|matlab-lite|g" \
		-e "s|__ICON__|matlab-lite|g" \
		installer/matlab-lite.desktop.in > "$(RPM_TOP)/SOURCES/$(APP_NAME).desktop"
	@rpmbuild --define "_topdir $(abspath $(RPM_TOP))" -bb "$(RPM_TOP)/SPECS/$(APP_NAME).spec"
	@find "$(RPM_TOP)/RPMS" -name '*.rpm' -exec cp {} "$(DIST_DIR)"/ \;

appimage: build
	@rm -rf "$(APPIMAGE_DIR)" "$(DIST_DIR)"
	@mkdir -p "$(APPIMAGE_DIR)/usr/bin" \
		"$(APPIMAGE_DIR)/usr/share/applications" \
		"$(APPIMAGE_DIR)/usr/share/icons/hicolor/256x256/apps" \
		"$(DIST_DIR)"
	@install -Dm755 "$(BUILD_DIR)/$(APP_NAME)" "$(APPIMAGE_DIR)/usr/bin/$(APP_NAME)"
	@install -Dm644 "icon.png" "$(APPIMAGE_DIR)/usr/share/icons/hicolor/256x256/apps/$(APP_NAME).png"
	@sed \
		-e "s|__BIN__|matlab-lite|g" \
		-e "s|__ICON__|matlab-lite|g" \
		installer/matlab-lite.desktop.in > "$(APPIMAGE_DIR)/usr/share/applications/$(APP_NAME).desktop"
	@install -Dm755 packaging/appimage/AppRun "$(APPIMAGE_DIR)/AppRun"
	@command -v linuxdeploy >/dev/null 2>&1 || { echo "linuxdeploy is required to bundle GTK dependencies for an AppImage"; echo "AppDir scaffold prepared under $(APPIMAGE_DIR)"; exit 1; }
	@command -v appimagetool >/dev/null 2>&1 || { echo "appimagetool is required to turn the AppDir into an AppImage"; echo "AppDir scaffold prepared under $(APPIMAGE_DIR)"; exit 1; }
	@appimagetool "$(APPIMAGE_DIR)" "$(DIST_DIR)/$(APP_NAME)-$(VERSION)-$$(uname -m).AppImage"
	