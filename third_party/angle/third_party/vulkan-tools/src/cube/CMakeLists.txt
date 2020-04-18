# ~~~
# Copyright (c) 2018 Valve Corporation
# Copyright (c) 2018 LunarG, Inc.
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

set(CUBE_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/.. ${VulkanHeaders_INCLUDE_DIR})

set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/scripts")

if (NOT GLSLANG_INSTALL_DIR AND DEFINED ENV{GLSLANG_INSTALL_DIR})
    set(GLSLANG_INSTALL_DIR $ENV{GLSLANG_INSTALL_DIR})
endif()

if(GLSLANG_INSTALL_DIR)
    message(STATUS "Using GLSLANG_INSTALL_DIR to look for glslangValidator")
    find_program(GLSLANG_VALIDATOR names glslangValidator HINTS "${GLSLANG_INSTALL_DIR}/bin")
else()
    set(GLSLANG_VALIDATOR_NAME "glslangValidator")
    message(STATUS "Using cmake find_program to look for glslangValidator")
    if(WIN32)
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-windows-x64-Release.zip)
        set(GLSLANG_VALIDATOR_NAME "glslangValidator.exe")
    elseif(APPLE)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-osx-Release.zip)
    elseif(UNIX AND NOT APPLE) # i.e. Linux
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/fetch_glslangvalidator.py glslang-master-linux-Release.zip)
    endif()
    find_program(GLSLANG_VALIDATOR NAMES ${GLSLANG_VALIDATOR_NAME} HINTS "${PROJECT_SOURCE_DIR}/glslang/bin")
endif()

if(UNIX AND NOT APPLE) # i.e. Linux
    include(FindPkgConfig)
    option(BUILD_WSI_XCB_SUPPORT "Build XCB WSI support" ON)
    option(BUILD_WSI_XLIB_SUPPORT "Build Xlib WSI support" ON)
    option(BUILD_WSI_WAYLAND_SUPPORT "Build Wayland WSI support" ON)
    set(CUBE_WSI_SELECTION "XCB" CACHE STRING "Select WSI target for vkcube (XCB, XLIB, WAYLAND, DISPLAY)")

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
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    if(NOT MSVC_VERSION LESS 1900)
        # Enable control flow guard
        message(STATUS "Building vkcube with control flow guard")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/guard:cf>")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
    endif()
elseif(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX AND NOT APPLE) # i.e. Linux
    if(NOT CUBE_WSI_SELECTION)
        set(CUBE_WSI_SELECTION "XCB")
    endif()

    if(CUBE_WSI_SELECTION STREQUAL "XCB")
        if(NOT BUILD_WSI_XCB_SUPPORT)
            message(FATAL_ERROR "Selected XCB for vkcube build but not building Xcb support")
        endif()
        set(CUBE_INCLUDE_DIRS ${XCB_INCLUDE_DIRS} ${CUBE_INCLUDE_DIRS})
        link_libraries(${XCB_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    elseif(CUBE_WSI_SELECTION STREQUAL "XLIB")
        if(NOT BUILD_WSI_XLIB_SUPPORT)
            message(FATAL_ERROR "Selected XLIB for vkcube build but not building Xlib support")
        endif()
        set(CUBE_INCLUDE_DIRS ${X11_INCLUDE_DIR} ${CUBE_INCLUDE_DIRS})
        link_libraries(${X11_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
    elseif(CUBE_WSI_SELECTION STREQUAL "WAYLAND")
        if(NOT BUILD_WSI_WAYLAND_SUPPORT)
            message(FATAL_ERROR "Selected Wayland for vkcube build but not building Wayland support")
        endif()
        set(CUBE_INCLUDE_DIRS ${WAYLAND_CLIENT_INCLUDE_DIR} ${CUBE_INCLUDE_DIRS})
        link_libraries(${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    elseif(CUBE_WSI_SELECTION STREQUAL "DISPLAY")
        add_definitions(-DVK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message(FATAL_ERROR "Unrecognized value for CUBE_WSI_SELECTION: ${CUBE_WSI_SELECTION}")
    endif()

    link_libraries(${API_LOWERCASE} m)
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(WIN32)
    # Use static MSVCRT libraries
    foreach(configuration
            in
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${configuration} MATCHES "/MD")
            string(REGEX
                   REPLACE "/MD"
                           "/MT"
                           ${configuration}
                           "${${configuration}}")
        endif()
    endforeach()

    file(COPY cube.vcxproj.user DESTINATION ${CMAKE_BINARY_DIR}/cube)
endif()

add_custom_command(COMMENT "Compiling cube vertex shader"
                   OUTPUT cube.vert.inc
                   COMMAND ${GLSLANG_VALIDATOR} -V -x -o ${CMAKE_CURRENT_BINARY_DIR}/cube.vert.inc
                           ${PROJECT_SOURCE_DIR}/cube/cube.vert
                   MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/cube/cube.vert
                   DEPENDS ${PROJECT_SOURCE_DIR}/cube/cube.vert ${GLSLANG_VALIDATOR})
add_custom_command(COMMENT "Compiling cube fragment shader"
                   OUTPUT cube.frag.inc
                   COMMAND ${GLSLANG_VALIDATOR} -V -x -o ${CMAKE_CURRENT_BINARY_DIR}/cube.frag.inc
                           ${PROJECT_SOURCE_DIR}/cube/cube.frag
                   MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/cube/cube.frag
                   DEPENDS ${PROJECT_SOURCE_DIR}/cube/cube.frag ${GLSLANG_VALIDATOR})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES")
endif()

include_directories(${CUBE_INCLUDE_DIRS})

# ----------------------------------------------------------------------------
# vkcube

if(APPLE)
    include(macOS/cube/cube.cmake)
elseif(NOT WIN32)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(vkcube
                       cube.c
                       ${PROJECT_SOURCE_DIR}/cube/cube.vert
                       ${PROJECT_SOURCE_DIR}/cube/cube.frag
                       cube.vert.inc
                       cube.frag.inc)
        target_link_libraries(vkcube Vulkan::Vulkan)
        CHECK_LIBRARY_EXISTS("rt" clock_gettime "" NEED_RT)
        if (NEED_RT)
            target_link_libraries(vkcube rt)
        endif()
    endif()
else()
    if(CMAKE_CL_64)
        set(LIB_DIR "Win64")
    else()
        set(LIB_DIR "Win32")
    endif()

    add_executable(vkcube
                   WIN32
                   cube.c
                   ${PROJECT_SOURCE_DIR}/cube/cube.vert
                   ${PROJECT_SOURCE_DIR}/cube/cube.frag
                   cube.vert.inc
                   cube.frag.inc)
    target_link_libraries(vkcube Vulkan::Vulkan)
endif()

if(APPLE)
    # Keep RPATH so fixup_bundle can use it to find libraries
    set_target_properties(vkcube PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(TARGETS vkcube BUNDLE DESTINATION "cube")
    # Fix up the library references to be self-contained within the bundle.
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\${CMAKE_INSTALL_PREFIX}/cube/vkcube.app \"\" \"${Vulkan_LIBRARY_DIR}\")
        ")
else()
    install(TARGETS vkcube RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ----------------------------------------------------------------------------
# vkcubepp

if(APPLE)
    include(macOS/cubepp/cubepp.cmake)
elseif(NOT WIN32)
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
        add_executable(vkcubepp
                       cube.cpp
                       ${PROJECT_SOURCE_DIR}/cube/cube.vert
                       ${PROJECT_SOURCE_DIR}/cube/cube.frag
                       cube.vert.inc
                       cube.frag.inc)
        target_link_libraries(vkcubepp Vulkan::Vulkan)
    endif()
else()
    if(CMAKE_CL_64)
        set(LIB_DIR "Win64")
    else()
        set(LIB_DIR "Win32")
    endif()

    add_executable(vkcubepp
                   WIN32
                   cube.cpp
                   ${PROJECT_SOURCE_DIR}/cube/cube.vert
                   ${PROJECT_SOURCE_DIR}/cube/cube.frag
                   cube.vert.inc
                   cube.frag.inc)
    target_link_libraries(vkcubepp Vulkan::Vulkan)
endif()

if(APPLE)
    # Keep RPATH so fixup_bundle can use it to find libraries
    set_target_properties(vkcubepp PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(TARGETS vkcubepp BUNDLE DESTINATION "cube")
    # Fix up the library references to be self-contained within the bundle.
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\${CMAKE_INSTALL_PREFIX}/cube/vkcubepp.app \"\" \"${Vulkan_LIBRARY_DIR}\")
        ")
else()
    install(TARGETS vkcubepp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
