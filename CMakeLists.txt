cmake_minimum_required(VERSION 3.16)
project(MatlabLite C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Git QUIET)

set(MATLAB_LITE_VERSION "1.0.1")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always --abbrev=7
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE MATLAB_LITE_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(MATLAB_LITE_VERSION STREQUAL "")
    set(MATLAB_LITE_VERSION "1.0.0")
endif()

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

add_executable(matlab-lite
    src/main.cpp
    src/window.cpp
    src/app_utils.cpp
    src/runner.cpp
    src/image_preview.cpp
)

target_include_directories(matlab-lite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${ADW_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${SOURCEVIEW_INCLUDE_DIRS}
)

target_link_libraries(matlab-lite
    ${ADW_LIBRARIES}
    ${GTK_LIBRARIES}
    ${SOURCEVIEW_LIBRARIES}
    pthread
)