cmake_minimum_required(VERSION 3.16)
project(Matpad VERSION 1.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

set(MATPAD_VERSION "${PROJECT_VERSION}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
    @ONLY
)

# Dependencies matching GI requirements
pkg_check_modules(ADW REQUIRED libadwaita-1)
pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(SOURCEVIEW REQUIRED gtksourceview-5)

include_directories(
    ${ADW_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${SOURCEVIEW_INCLUDE_DIRS}
)

link_directories(
    ${ADW_LIBRARY_DIRS}
    ${GTK_LIBRARY_DIRS}
    ${SOURCEVIEW_LIBRARY_DIRS}
)

add_compile_options(
    ${ADW_CFLAGS_OTHER}
    ${GTK_CFLAGS_OTHER}
    ${SOURCEVIEW_CFLAGS_OTHER}
)

add_executable(matpad
    src/main.cpp
    src/window.cpp
    src/app_utils.cpp
    src/runner.cpp
    src/settings.cpp
    src/image_preview.cpp
)

target_include_directories(matpad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${ADW_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${SOURCEVIEW_INCLUDE_DIRS}
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/flatpak/com.michal.Matpad.metainfo.xml
    DESTINATION share/metainfo
)

set(BIN "matpad")
set(ICON "matpad")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/packaging/desktop/matpad.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/com.michal.Matpad.desktop
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/com.michal.Matpad.desktop
    DESTINATION share/applications
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/data/icons/hicolor
    DESTINATION share/icons
)

target_link_libraries(matpad
    ${ADW_LIBRARIES}
    ${GTK_LIBRARIES}
    ${SOURCEVIEW_LIBRARIES}
    pthread
)