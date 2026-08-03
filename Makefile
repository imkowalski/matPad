.PHONY: all build clean run deb rpm appimage

APP_NAME := matpad
APP_ID := com.michal.Matpad
VERSION ?= 1.1.2
PROJECT_ROOT := $(CURDIR)
BUILD_DIR := build
DIST_DIR := dist
RELEASE_DIR := release
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
	@./build/matpad


deb: build
	@rm -rf "$(DEB_ROOT)" "$(DIST_DIR)"
	@mkdir -p "$(DEB_ROOT)/DEBIAN" \
		"$(DEB_ROOT)/usr/bin" \
		"$(DEB_ROOT)/usr/share/applications" \
		"$(DEB_ROOT)/usr/share/icons/hicolor" \
		"$(DIST_DIR)"
	@install -Dm755 "$(BUILD_DIR)/$(APP_NAME)" "$(DEB_ROOT)/usr/bin/$(APP_NAME)"
	@cp -r "data/icons/hicolor/." "$(DEB_ROOT)/usr/share/icons/hicolor/"
	@sed \
		-e "s|@VERSION@|$(VERSION)|g" \
		-e "s|@ARCH@|$(DEB_ARCH)|g" \
		-e "s|@DEPENDS@|$(DEB_DEPENDS)|g" \
		packaging/deb/control.in > "$(DEB_ROOT)/DEBIAN/control"
	@sed \
		-e "s|@BIN@|/usr/bin/$(APP_NAME)|g" \
		-e "s|@ICON@|$(APP_NAME)|g" \
		packaging/desktop/matpad.desktop.in > "$(DEB_ROOT)/usr/share/applications/$(APP_ID).desktop"
	@dpkg-deb --build --root-owner-group "$(DEB_ROOT)" "$(DIST_DIR)/$(APP_NAME)_$(VERSION)_$(DEB_ARCH).deb"

rpm: build
	@command -v rpmbuild >/dev/null 2>&1 || { echo "rpmbuild is required to build RPMs (Debian/Ubuntu: sudo apt install rpm; Fedora/RHEL: sudo dnf install rpm-build)"; exit 1; }
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
		packaging/rpm/matpad.spec.in > "$(RPM_TOP)/SPECS/$(APP_NAME).spec"
	@sed \
		-e "s|@BIN@|matpad|g" \
		-e "s|@ICON@|matpad|g" \
		packaging/desktop/matpad.desktop.in > "$(RPM_TOP)/SOURCES/$(APP_ID).desktop"
	@rpmbuild --define "_topdir $(abspath $(RPM_TOP))" -bb "$(RPM_TOP)/SPECS/$(APP_NAME).spec"
	@find "$(RPM_TOP)/RPMS" -name '*.rpm' -exec cp {} "$(DIST_DIR)"/ \;

release-deb: deb 
	@mkdir -p "$(RELEASE_DIR)"
	@cp "$(DIST_DIR)/$(APP_NAME)_$(VERSION)_$(DEB_ARCH).deb" "$(RELEASE_DIR)/"

release-rpm: rpm
	@mkdir -p "$(RELEASE_DIR)"
	@cp "$(DIST_DIR)/$(APP_NAME)-$(VERSION)-1.$(RPM_ARCH).rpm" "$(RELEASE_DIR)/"

release: build release-deb release-rpm 
	@mkdir -p "$(RELEASE_DIR)"
	@cp "$(BUILD_DIR)/$(APP_NAME)" "$(RELEASE_DIR)/"