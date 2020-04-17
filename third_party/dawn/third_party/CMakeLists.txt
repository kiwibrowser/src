# Copyright 2017 The Dawn Authors
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

# GLFW, only build the library
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
DawnExternalTarget("third_party" glfw)

# GoogleTest
set(GTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest)
set(GMOCK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock)
add_library(gtest STATIC ${GTEST_DIR}/src/gtest-all.cc ${GMOCK_DIR}/src/gmock-all.cc)
target_include_directories(gtest SYSTEM PUBLIC ${GTEST_DIR}/include ${GMOCK_DIR}/include)
target_include_directories(gtest SYSTEM PRIVATE ${GTEST_DIR} ${GMOCK_DIR})
find_package(Threads)
target_link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})
DawnExternalTarget("third_party" gtest)

# Glad
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include/glad/glad.h
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include/KHR/khrplatform.h
)
set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
set(GLAD_INCLUDE_DIR ${GLAD_INCLUDE_DIR} PARENT_SCOPE)
target_include_directories(glad SYSTEM PUBLIC ${GLAD_INCLUDE_DIR})
DawnExternalTarget("third_party" glad)

# SPIRV-Tools
set(SPIRV_TOOLS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spirv-tools/include PARENT_SCOPE)

# ShaderC
# Prevent SPIRV-Tools from using Werror as it has a warning on MSVC
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
# Don't add unnecessary shaderc targets
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_INSTALL ON)
# Remove unused glslang and spirv-tools parts
# set(ENABLE_HLSL OFF CACHE BOOL "")
set(ENABLE_OPT OFF CACHE BOOL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "")
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "")
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "")
# Help shaderc find the non-standard paths for its dependencies
set(SHADERC_GOOGLE_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/googletest CACHE STRING "Location of googletest source")
set(SHADERC_GLSLANG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glslang" CACHE STRING "Location of glslang source")
set(SHADERC_SPIRV_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spirv-tools CACHE STRING "Location of spirv-tools source")
# Help shaderc find the python executable when run inside VS.
find_package(PythonInterp REQUIRED)
set(PYTHON_EXE ${PYTHON_EXECUTABLE})
# Need to include this for spirv-tools to find it
add_subdirectory(spirv-headers)
add_subdirectory(shaderc)
# Namespace the shaderc targets in a folder to avoid cluttering the
# Visual Studio solution explorer
set_target_properties(
    add-copyright
    build-version
    check-copyright
    glslc
    glslc_exe
    install-headers
    shaderc
    shaderc_shared
    shaderc_util
    shaderc_combined_genfile
    shaderc-online-compile testdata
    SPIRV-Headers-example
    SPIRV-Headers-example-1.1
    PROPERTIES FOLDER "third_party/shaderc"
)
# Remove a bunch of targets we don't need that are pulled by shaderc and glslang
set_target_properties(
    SPIRV-Headers-example-1.1
    SPIRV-Headers-example
    glslc_exe
    SPIRV-Tools-link
    SPVRemapper
    shaderc
    shaderc-online-compile
    shaderc_combined_genfile
    PROPERTIES EXCLUDE_FROM_ALL true
)

# SPIRV-Cross
set(SPIRV_CROSS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/GLSL.std.450.h
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_common.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_cfg.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_cfg.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_cross.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_cross.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv.hpp
)

set(NEED_SPIRV_CROSS_GLSL OFF)
if (DAWN_ENABLE_D3D12)
    list(APPEND SPIRV_CROSS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_hlsl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_hlsl.hpp
    )
    set(NEED_SPIRV_CROSS_GLSL ON)
endif()

if (DAWN_ENABLE_METAL)
    list(APPEND SPIRV_CROSS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_msl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_msl.hpp
    )
    set(NEED_SPIRV_CROSS_GLSL ON)
endif()

if (DAWN_ENABLE_OPENGL OR NEED_SPIRV_CROSS_GLSL)
    list(APPEND SPIRV_CROSS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_glsl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/spirv_glsl.hpp
    )
endif()

add_library(spirv_cross STATIC ${SPIRV_CROSS_SOURCES})
target_compile_definitions(spirv_cross PUBLIC SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
set(SPIRV_CROSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
DawnExternalTarget("third_party" spirv_cross)

# STB, used for stb_image
set(STB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/stb PARENT_SCOPE)

# glm matrix math library
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm PARENT_SCOPE)

# Tiny glTF loader library
set(TINYGLTFLOADER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

# Vulkan headers
set(VULKAN_HEADERS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
