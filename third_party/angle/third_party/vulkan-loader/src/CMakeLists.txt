# ~~~
# Copyright (c) 2014-2018 Valve Corporation
# Copyright (c) 2014-2018 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ~~~

cmake_minimum_required(VERSION 3.4)

# Apple: Must be set before enable_language() or project() as it may influence configuration of the toolchain and flags.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project(Vulkan-Loader)

add_definitions(-DAPI_NAME="Vulkan")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(PythonInterp 3 REQUIRED)

if (TARGET Vulkan::Headers)
    message(STATUS "Using Vulkan headers from Vulkan::Headers target")
    get_target_property(VulkanRegistry_DIR Vulkan::Registry INTERFACE_INCLUDE_DIRECTORIES)
else()
    find_package(VulkanHeaders)
    if(NOT ${VulkanHeaders_FOUND})
        message(FATAL_ERROR "Could not find Vulkan headers path. This can be fixed by setting VULKAN_HEADERS_INSTALL_DIR to an "
                            "installation of the Vulkan-Headers repository.")
    endif()
    if(NOT ${VulkanRegistry_FOUND})
        message(FATAL_ERROR "Could not find Vulkan registry path. This can be fixed by setting VULKAN_HEADERS_INSTALL_DIR to an "
                            "installation of the Vulkan-Headers repository.")
    endif()

    # set up the Vulkan::Headers target for consistency
    add_library(vulkan-headers INTERFACE)
    target_include_directories(vulkan-headers INTERFACE "${VulkanHeaders_INCLUDE_DIRS}")
    add_library(Vulkan::Headers ALIAS vulkan-headers)
endif()

option(USE_CCACHE "Use ccache" OFF)
if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    endif()
endif()

include(GNUInstallDirs)

if(UNIX AND NOT APPLE) # i.e.: Linux
    include(FindPkgConfig)
endif()

if(APPLE)
    # CMake versions 3 or later need CMAKE_MACOSX_RPATH defined. This avoids the CMP0042 policy message.
    set(CMAKE_MACOSX_RPATH 1)
endif()

if(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Windows: if install locations not set by user, set install prefix to "<build_dir>\install".
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
endif()

# Enable IDE GUI folders.  "Helper targets" that don't have interesting source code should set their FOLDER property to this
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(LOADER_HELPER_FOLDER "Helper Targets")

if(UNIX)
    set(
        FALLBACK_CONFIG_DIRS "/etc/xdg"
        CACHE
            STRING
            "Search path to use when XDG_CONFIG_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant."
        )
    set(
        FALLBACK_DATA_DIRS "/usr/local/share:/usr/share"
        CACHE
            STRING
            "Search path to use when XDG_DATA_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant."
        )
endif()

if(UNIX AND NOT APPLE) # i.e.: Linux
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)

    if(BUILD_WSI_XCB_SUPPORT)
        find_package(XCB REQUIRED)
    endif()

    if(BUILD_WSI_XLIB_SUPPORT)
        find_package(X11 REQUIRED)
    endif()

    if(BUILD_WSI_WAYLAND_SUPPORT)
        find_package(Wayland REQUIRED)
        include_directories(${WAYLAND_CLIENT_INCLUDE_DIR})
    endif()
endif()

if(WIN32)
    option(ENABLE_WIN10_ONECORE "Link the loader with OneCore umbrella libraries" OFF)
    option(ENABLE_STATIC_LOADER "Build the loader as a static library" OFF)
endif()

option(BUILD_LOADER "Build loader" ON)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest)
    option(BUILD_TESTS "Build Tests" ON)
else()
    option(BUILD_TESTS "Build Tests" OFF)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-stringop-truncation -Wno-stringop-overflow")
    set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -fno-strict-aliasing -fno-builtin-memcmp")

    # For GCC version 7.1 or greater, we need to disable the implicit fallthrough warning since there's no consistent way to satisfy
    # all compilers until they all accept the C++17 standard
    if(CMAKE_COMPILER_IS_GNUCC AND NOT (CMAKE_CXX_COMPILER_VERSION LESS 7.1))
        set(COMMON_COMPILE_FLAGS "${COMMON_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
    endif()

    if(APPLE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_COMPILE_FLAGS}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 ${COMMON_COMPILE_FLAGS}")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_COMPILE_FLAGS} -std=c++11 -fno-rtti")
    if(UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    endif()
endif()

if(MSVC)
    # /WX: Treat warnings as errors
    # /GR-: Disable RTTI
    # /w34456: Warn about nested declarations
    # /w34701, /w34703: Warn about potentially uninitialized variables
    # /w34057: Warn about different indirection types.
    # /w34245: Warn about signed/unsigned mismatch.
    set(MSVC_LOADER_COMPILE_OPTIONS /WX /GR- /w34456 /w34701 /w34703 /w34057 /w34245)
endif()

# Define a macro to generate source code from vk.xml
set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
macro(GenerateFromVkXml output dependency)
    add_custom_command(OUTPUT ${output}
                       COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/loader_genvk.py -registry ${VulkanRegistry_DIR}/vk.xml -scripts
                               ${VulkanRegistry_DIR} ${output}
                       DEPENDS ${SCRIPTS_DIR}/${dependency}
                               ${SCRIPTS_DIR}/loader_genvk.py
                               ${VulkanRegistry_DIR}/vk.xml
                               ${VulkanRegistry_DIR}/generator.py
                               ${VulkanRegistry_DIR}/reg.py)
endmacro()

# Use the macro to generate the source files.
GenerateFromVkXml(vk_layer_dispatch_table.h loader_extension_generator.py)
GenerateFromVkXml(vk_dispatch_table_helper.h dispatch_table_helper_generator.py)
GenerateFromVkXml(vk_safe_struct.h helper_file_generator.py)
GenerateFromVkXml(vk_safe_struct.cpp helper_file_generator.py)
GenerateFromVkXml(vk_enum_string_helper.h helper_file_generator.py)
GenerateFromVkXml(vk_object_types.h helper_file_generator.py)
GenerateFromVkXml(vk_extension_helper.h helper_file_generator.py)
GenerateFromVkXml(vk_typemap_helper.h helper_file_generator.py)

# Use a custom target to cause the source files to be generated.
add_custom_target(generate_helper_files
                  DEPENDS vk_enum_string_helper.h
                          vk_safe_struct.h
                          vk_safe_struct.cpp
                          vk_object_types.h
                          vk_layer_dispatch_table.h
                          vk_dispatch_table_helper.h
                          vk_extension_helper.h
                          vk_typemap_helper.h)

set_target_properties(generate_helper_files PROPERTIES FOLDER ${LOADER_HELPER_FOLDER})

if(UNIX)
    add_definitions(-DFALLBACK_CONFIG_DIRS="${FALLBACK_CONFIG_DIRS}")
    add_definitions(-DFALLBACK_DATA_DIRS="${FALLBACK_DATA_DIRS}")
    add_definitions(-DSYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}")

    # Make sure /etc is searched by the loader
    if(NOT (CMAKE_INSTALL_FULL_SYSCONFDIR STREQUAL "/etc"))
        add_definitions(-DEXTRASYSCONFDIR="/etc")
    endif()
endif()

# uninstall target
if(NOT TARGET uninstall)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                   IMMEDIATE
                   @ONLY)
    add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    set_target_properties(uninstall PROPERTIES FOLDER ${LOADER_HELPER_FOLDER})
endif()

if(BUILD_LOADER)
    add_subdirectory(loader)
endif()

add_subdirectory(external)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
