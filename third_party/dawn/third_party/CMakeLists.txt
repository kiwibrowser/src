# Copyright 2020 The Dawn Authors
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

if (NOT TARGET SPIRV-Headers)
    set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
    set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)

    message(STATUS "Dawn: using SPIRV-Headers at ${DAWN_SPIRV_HEADERS_DIR}")
    add_subdirectory(${DAWN_SPIRV_HEADERS_DIR})
endif()

if (NOT TARGET SPIRV-Tools)
    set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
    set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)

    message(STATUS "Dawn: using SPIRV-Tools at ${DAWN_SPIRV_TOOLS_DIR}")
    add_subdirectory(${DAWN_SPIRV_TOOLS_DIR})
endif()

if (NOT TARGET glslang)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
    set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)

    message(STATUS "Dawn: using GLSLang at ${DAWN_GLSLANG_DIR}")
    add_subdirectory(${DAWN_GLSLANG_DIR})
endif()

if (TARGET shaderc)
    if (NOT TARGET shaderc_spvc)
        message(FATAL_ERROR "Dawn: If shaderc is configured before Dawn, it must include SPVC")
    endif()
else()
    set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    set(SHADERC_ENABLE_SPVC ON CACHE BOOL "" FORCE)

    # Change the default value of SHADERC_ENABLE_SHARED_CRT to ON as that's what matches the
    # CMake defaults better.
    if(MSVC)
        option(SHADERC_ENABLE_SHARED_CRT "Use the shared CRT instead of the static CRT" ON CACHE BOOL "" FORCE)
    endif()

    # Let SPVC's CMakeLists.txt deal with configuring SPIRV-Cross
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(SHADERC_SPIRV_CROSS_DIR "${DAWN_SPIRV_CROSS_DIR}" CACHE BOOL "" FORCE)

    message(STATUS "Dawn: using shaderc[_spvc] at ${DAWN_SHADERC_DIR}")
    message(STATUS "Dawn:  - with SPIRV-Cross at ${DAWN_SPIRV_CROSS_DIR}")
    add_subdirectory(${DAWN_SHADERC_DIR})
endif()

if (DAWN_BUILD_EXAMPLES)
    if (NOT TARGET glfw)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

        message(STATUS "Dawn: using GLFW at ${DAWN_GLFW_DIR}")
        add_subdirectory(${DAWN_GLFW_DIR})
    endif()

    if (NOT TARGET glm)
        message(STATUS "Dawn: using GLM at ${DAWN_GLM_DIR}")
        add_subdirectory(${DAWN_GLM_DIR})
    endif()
endif()

# Header-only library for khrplatform.h
add_library(dawn_khronos_platform INTERFACE)
target_sources(dawn_khronos_platform INTERFACE "${DAWN_THIRD_PARTY_DIR}/khronos/KHR/khrplatform.h")
target_include_directories(dawn_khronos_platform INTERFACE "${DAWN_THIRD_PARTY_DIR}/khronos")

# Header-only library for Vulkan headers
add_library(dawn_vulkan_headers INTERFACE)
target_sources(dawn_vulkan_headers INTERFACE
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vk_icd.h"
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vk_layer.h"
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vk_platform.h"
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vk_sdk_platform.h"
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vulkan.h"
    "${DAWN_THIRD_PARTY_DIR}/khronos/vulkan/vulkan_core.h"
)
target_include_directories(dawn_vulkan_headers INTERFACE "${DAWN_THIRD_PARTY_DIR}/khronos")
