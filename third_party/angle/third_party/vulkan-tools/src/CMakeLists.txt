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

# CMake project initialization ---------------------------------------------------------------------------------------------------
# This section contains pre-project() initialization, and ends with the project() command.

cmake_minimum_required(VERSION 3.4)

# Apple: Must be set before enable_language() or project() as it may influence configuration of the toolchain and flags.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project(Vulkan-Tools)

# find_package(), include() and global project settings --------------------------------------------------------------------------

find_package(PythonInterp 3 REQUIRED)

# User-interface declarations ----------------------------------------------------------------------------------------------------
# This section contains variables that affect development GUIs (e.g. CMake GUI and IDEs), such as option(), folders, and variables
# with the CACHE property.

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_CUBE "Build cube" ON)
option(BUILD_VULKANINFO "Build vulkaninfo" ON)
option(BUILD_ICD "Build icd" ON)
# Installing the Mock ICD to system directories is probably not desired since this ICD is not a very complete implementation.
# Require the user to ask that it be installed if they really want it.
option(INSTALL_ICD "Install icd" OFF)

# Enable IDE GUI folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# "Helper" targets that don't have interesting source code should set their FOLDER property to this
set(TOOLS_HELPER_FOLDER "Helper Targets")

option(USE_CCACHE "Use ccache" OFF)
if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    endif()
endif()

# ~~~
# Find Vulkan Headers and Loader
# Search order:
#  User-supplied CMAKE_PREFIX_PATH containing paths to the header and/or loader install dirs
#  CMake options VULKAN_HEADERS_INSTALL_DIR and/or VULKAN_LOADER_INSTALL_DIR
#  Env vars VULKAN_HEADERS_INSTALL_DIR and/or VULKAN_LOADER_INSTALL_DIR
#  If on MacOS
#   CMake option MOTLENVK_REPO_ROOT
#   Env vars MOLTENVK_REPO_ROOT
#  Fallback to FindVulkan operation using SDK install or system installed components.
# ~~~
set(VULKAN_HEADERS_INSTALL_DIR "HEADERS-NOTFOUND" CACHE PATH "Absolute path to a Vulkan-Headers install directory")
set(VULKAN_LOADER_INSTALL_DIR "LOADER-NOTFOUND" CACHE PATH "Absolute path to a Vulkan-Loader install directory")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};${VULKAN_HEADERS_INSTALL_DIR};${VULKAN_LOADER_INSTALL_DIR};
    $ENV{VULKAN_HEADERS_INSTALL_DIR};$ENV{VULKAN_LOADER_INSTALL_DIR})

if(APPLE)
    set(MOLTENVK_REPO_ROOT "MOLTENVK-NOTFOUND" CACHE PATH "Absolute path to a MoltenVK repo directory")
    if(NOT MOLTENVK_REPO_ROOT AND NOT DEFINED ENV{MOLTENVK_REPO_ROOT})
        message(FATAL_ERROR "Must define location of MoltenVK repo -- see BUILD.md")
    endif()

    if(NOT MOLTENVK_REPO_ROOT)
        set(MOLTENVK_REPO_ROOT $ENV{MOLTENVK_REPO_ROOT})
    endif()
    message(STATUS "Using MoltenVK repo location at ${MOLTENVK_REPO_ROOT}")
endif()
message(STATUS "Using find_package to locate Vulkan")
find_package(Vulkan)
find_package(VulkanHeaders)
get_filename_component(Vulkan_LIBRARY_DIR ${Vulkan_LIBRARY} DIRECTORY)
message(STATUS "Vulkan FOUND = ${Vulkan_FOUND}")
message(STATUS "Vulkan Lib Dir = ${Vulkan_LIBRARY_DIR}")
message(STATUS "Vulkan Lib = ${Vulkan_LIBRARY}")
message(STATUS "Vulkan Headers Include = ${VulkanHeaders_INCLUDE_DIR}")
message(STATUS "Vulkan Headers Registry = ${VulkanRegistry_DIR}")

include(GNUInstallDirs)

if(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Windows: if install locations not set by user, set install prefix to "<build_dir>\install".
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "default install path" FORCE)
endif()

# uninstall target
if(NOT TARGET uninstall)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                   IMMEDIATE
                   @ONLY)

    add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    set_target_properties(uninstall PROPERTIES FOLDER ${TOOLS_HELPER_FOLDER})
endif()

if(APPLE)
    # CMake versions 3 or later need CMAKE_MACOSX_RPATH defined. This avoids the CMP0042 policy message.
    set(CMAKE_MACOSX_RPATH 1)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(COMMON_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
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

if(APPLE)
    include(mac_common.cmake)
endif()

if(BUILD_CUBE)
    add_subdirectory(cube)
endif()

if(BUILD_VULKANINFO)
    add_subdirectory(vulkaninfo)
endif()

if(BUILD_ICD)
    add_subdirectory(icd)
endif()
