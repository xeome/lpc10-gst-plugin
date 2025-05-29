cmake_minimum_required(VERSION 3.15)

project(GstLpc10Plugin VERSION 0.1.0 LANGUAGES C)

# --- Project Version Information ---
# These versions are used by configure_file for version.h and CPack.
# Update them here when making a new release.
set(PROJECT_VERSION_RELEASE 0)
set(PROJECT_VERSION_MAJOR 0) # Corresponds to GST_VERSION_MAJOR for plugin_init
set(PROJECT_VERSION_MINOR 1) # Corresponds to GST_VERSION_MINOR for plugin_init
set(PROJECT_VERSION_PATCH 0) # Corresponds to your internal patch/build number

set(PROJECT_VERSION "${PROJECT_VERSION_RELEASE}.${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
message(STATUS "Configuring ${PROJECT_NAME} version ${PROJECT_VERSION}")

# --- Compiler and Build Options ---
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Find Dependencies ---
find_package(PkgConfig REQUIRED)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3 -O0 -Wall -Wextra -Wpedantic -Wno-unused-parameter -DDEBUG -UNDEBUG")


pkg_check_modules(GSTREAMER REQUIRED
    gstreamer-1.0>=1.6
    gstreamer-base-1.0>=1.6
    gstreamer-audio-1.0>=1.6
)

# --- Git Information (Optional) ---
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE PROJECT_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} show -s --format=%ct
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE PROJECT_GIT_COMMIT_TIMESTAMP_UNIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(PROJECT_GIT_COMMIT_TIMESTAMP_UNIX)
        execute_process(
            COMMAND date -u -d "@${PROJECT_GIT_COMMIT_TIMESTAMP_UNIX}" "+%Y-%m-%d %H:%M:%S"
            OUTPUT_VARIABLE PROJECT_GIT_COMMIT_TIMESTAMP
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(NOT PROJECT_GIT_COMMIT_TIMESTAMP)
            set(PROJECT_GIT_COMMIT_TIMESTAMP "${PROJECT_GIT_COMMIT_TIMESTAMP_UNIX}")
        endif()
    endif()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} symbolic-ref --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE PROJECT_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} config user.name
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE PROJECT_BUILT_BY
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(PROJECT_GIT_REVISION "N/A")
    set(PROJECT_GIT_COMMIT_TIMESTAMP "N/A")
    set(PROJECT_GIT_BRANCH "N/A")
    set(PROJECT_BUILT_BY "N/A")
endif()

string(TIMESTAMP PROJECT_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d" UTC)
string(TIMESTAMP PROJECT_BUILD_TIME "%H:%M:%S" UTC)

# For defines like VERSION_RELEASE_I which expect integers
set(PROJECT_VERSION_RELEASE_I ${PROJECT_VERSION_RELEASE})
set(PROJECT_VERSION_MAJOR_I ${PROJECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR_I ${PROJECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH_I ${PROJECT_VERSION_PATCH})

# --- Configure version.h ---
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)

# --- LPC10 Library Target ---
# Add the lpc10 subdirectory which should define the lpc10_lib target
add_subdirectory(lpc10)

# --- Plugin Target ---
set(PLUGIN_NAME lpc10) # Used for GST_PLUGIN_DEFINE
set(PLUGIN_TARGET_NAME gst${PLUGIN_NAME}) # Shared library name, e.g., libgstlpc10.so

set(PLUGIN_SOURCES
    src/gstlpc10.c
    src/gstlpc10enc.c
    src/gstlpc10enc.h
    src/gstlpc10dec.c
    src/gstlpc10dec.h
    src/gstlpc10_util.c
    src/gstlpc10_util.h
)

add_library(${PLUGIN_TARGET_NAME} SHARED ${PLUGIN_SOURCES})

target_include_directories(${PLUGIN_TARGET_NAME}
    PUBLIC
        ${GSTREAMER_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/src" # For plugin's own header
        "${CMAKE_CURRENT_SOURCE_DIR}/lpc10" # For lpc10.h
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}" # For generated version.h
)

target_link_libraries(${PLUGIN_TARGET_NAME}
    PUBLIC
        ${GSTREAMER_LIBRARIES}
        # Explicitly include all symbols from the static library when linking the shared library
        # This is sometimes necessary for static libraries containing f2c-translated code
        LINKER:--whole-archive
        lpc10 # Link against the lpc10 static library
        LINKER:--no-whole-archive
)

# --- Installation ---
include(GNUInstallDirs)

install(TARGETS ${PLUGIN_TARGET_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0
)
# -fPIC
set_target_properties(${PLUGIN_TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# --- Packaging (CPack) ---
# The version for GST_PLUGIN_DEFINE comes from version.h which uses PROJECT_VERSION_MAJOR etc.
# For CPack, we use the combined PROJECT_VERSION.
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}) # Will be GstLpc10Plugin
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GStreamer LPC10 audio codec plugin")
set(CPACK_PACKAGE_VENDOR "Your Name/Organization") # TODO: Update vendor
set(CPACK_PACKAGE_MAINTAINER "your.email@example.com") # TODO: Update maintainer
set(CPACK_PACKAGE_CONTACT ${CPACK_PACKAGE_MAINTAINER})

# Debian specific CPack settings
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_MAINTAINER})
set(CPACK_DEBIAN_PACKAGE_SECTION "sound") # Changed from video to sound
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# CPack should determine architecture automatically, or set if needed:
# set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64") # or "arm64", etc.
# For "all", ensure no architecture-specific compiled code is included directly in ways CPack can't handle.
# GStreamer plugins are usually architecture-specific.

# If control.in is still needed for more complex fields:
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/control.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/control.tmp"
#     @ONLY
# )
# set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/control.tmp")
# For now, assuming basic fields are sufficient. Update control.in if used.

# Add GStreamer runtime dependencies to the package
# This is a basic example; exact dependencies might vary or need more specific versions.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "gstreamer1.0-plugins-base, libgstreamer-plugins-base1.0-0, libgstreamer1.0-0 (>= 1.6), libglib2.0-0 (>= 2.40)")

include(CPack)

# --- Testing (Optional, if you have tests in a 'test' subdirectory) ---
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
#     enable_testing()
#     add_subdirectory(test)
# endif()

message(STATUS "Configuration complete. You can now build with 'make' or your chosen generator.")
message(STATUS "Run 'make package' to build a Debian package after building the project.")
