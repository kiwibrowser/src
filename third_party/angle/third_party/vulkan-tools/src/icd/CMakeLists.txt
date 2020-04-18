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

set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/scripts")

# Define macro used for building vk.xml generated files
macro(run_vk_xml_generate dependency output)
    add_custom_command(OUTPUT ${output}
                       COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/kvt_genvk.py -registry ${VulkanRegistry_DIR}/vk.xml -scripts
                               ${VulkanRegistry_DIR} ${output}
                       DEPENDS ${VulkanRegistry_DIR}/vk.xml
                               ${VulkanRegistry_DIR}/generator.py
                               ${SCRIPTS_DIR}/${dependency}
                               ${SCRIPTS_DIR}/kvt_genvk.py
                               ${VulkanRegistry_DIR}/reg.py)
endmacro()

if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DVK_USE_PLATFORM_WIN32_KHX -DWIN32_LEAN_AND_MEAN)
elseif(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR -DVK_USE_PLATFORM_ANDROID_KHX)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX AND NOT APPLE) # i.e. Linux
    if(BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR -DVK_USE_PLATFORM_XCB_KHX)
    endif()

    if(BUILD_WSI_XLIB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR -DVK_USE_PLATFORM_XLIB_KHX -DVK_USE_PLATFORM_XLIB_XRANDR_EXT)
    endif()

    if(BUILD_WSI_WAYLAND_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR -DVK_USE_PLATFORM_WAYLAND_KHX)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

# Copy or link the JSON files to the binary directory for ease of use in the build tree.
set(ICD_JSON_FILES VkICD_mock_icd)
if(WIN32)
    # extra setup for out-of-tree builds
    if(NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        if(CMAKE_GENERATOR MATCHES "^Visual Studio.*")
            foreach(config_file ${ICD_JSON_FILES})
                file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json src_json)
                file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${config_file}.json dst_json)
                add_custom_target(${config_file}-json ALL COMMAND copy ${src_json} ${dst_json} VERBATIM)
                set_target_properties(${config_file}-json PROPERTIES FOLDER ${TOOLS_HELPER_FOLDER})
            endforeach(config_file)
        else()
            foreach(config_file ${ICD_JSON_FILES})
                file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json src_json)
                file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${config_file}.json dst_json)
                add_custom_target(${config_file}-json ALL COMMAND copy ${src_json} ${dst_json} VERBATIM)
            endforeach(config_file)
        endif()
    endif()
elseif(APPLE)
    # extra setup for out-of-tree builds
    if(NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        if(CMAKE_GENERATOR MATCHES "^Xcode.*")
            add_custom_target(mk_icd_config_dir ALL
                              COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
            foreach(config_file ${ICD_JSON_FILES})
                add_custom_target(${config_file}-json ALL
                                  DEPENDS mk_icd_config_dir
                                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/macos/${config_file}.json
                                          $<CONFIG> ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${config_file}.json
                                  VERBATIM)
            endforeach(config_file)
        else()
            foreach(config_file ${ICD_JSON_FILES})
                add_custom_target(${config_file}-json ALL
                                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/macos/${config_file}.json
                                          ${config_file}.json
                                  VERBATIM)
            endforeach(config_file)
        endif()
    endif()
else()
    # extra setup for out-of-tree builds
    if(NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
        foreach(config_file ${ICD_JSON_FILES})
            add_custom_target(${config_file}-json ALL
                              COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json
                                      ${config_file}.json
                              VERBATIM)
        endforeach(config_file)
    endif()
endif()

# Custom target for generated vulkan helper file dependencies
set(icd_generate_helper_files_DEPENDS)
add_custom_target(icd_generate_helper_files DEPENDS vk_typemap_helper.h)
set_target_properties(icd_generate_helper_files PROPERTIES FOLDER ${TOOLS_HELPER_FOLDER})
run_vk_xml_generate(vulkan_tools_helper_file_generator.py vk_typemap_helper.h)

# For ICD with a direct dependency on a project with the same name, use it.
if(NOT (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR))
    foreach(config_file ${ICD_JSON_FILES})
        add_dependencies(${config_file}-json ${config_file})
    endforeach(config_file)
endif()
add_custom_target(generate_icd_files DEPENDS mock_icd.h mock_icd.cpp)
set_target_properties(generate_icd_files PROPERTIES FOLDER ${TOOLS_HELPER_FOLDER})

if(WIN32)
    macro(add_vk_icd target)
        file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/VkICD_${target}.def DEF_FILE)
        add_custom_target(copy-${target}-def-file ALL
                          COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEF_FILE} VkICD_${target}.def
                          VERBATIM)
        set_target_properties(copy-${target}-def-file PROPERTIES FOLDER ${TOOLS_HELPER_FOLDER})
        add_library(VkICD_${target} SHARED ${ARGN} VkICD_${target}.def)
        add_dependencies(VkICD_${target} icd_generate_helper_files generate_icd_files)
        # target_link_Libraries(VkICD_${target} VkICD_utils)
        if(INSTALL_ICD)
            install(TARGETS VkICD_${target} DESTINATION ${CMAKE_INSTALL_LIBDIR})
        endif()
    endmacro()
elseif(APPLE)
    macro(add_vk_icd target)
        add_library(VkICD_${target} SHARED ${ARGN})
        # target_link_Libraries(VkICD_${target} VkICD_utils)
        add_dependencies(VkICD_${target} icd_generate_helper_files generate_icd_files)
        set_target_properties(VkICD_${target} PROPERTIES LINK_FLAGS "-Wl")
        if(INSTALL_ICD)
            install(TARGETS VkICD_${target} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        endif()
    endmacro()
else()
    macro(add_vk_icd target)
        add_library(VkICD_${target} SHARED ${ARGN})
        # target_link_Libraries(VkICD_${target} VkICD_utils)
        add_dependencies(VkICD_${target} icd_generate_helper_files generate_icd_files)
        set_target_properties(VkICD_${target} PROPERTIES LINK_FLAGS "-Wl,-export-dynamic,-Bsymbolic,--exclude-libs,ALL")
        if((UNIX AND NOT APPLE) AND INSTALL_ICD) # i.e. Linux
            install(TARGETS VkICD_${target} DESTINATION ${CMAKE_INSTALL_LIBDIR})
        endif()
    endmacro()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${VulkanHeaders_INCLUDE_DIR}
                    ${WAYLAND_CLIENT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${PROJECT_BINARY_DIR}
                    ${CMAKE_BINARY_DIR})

if(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS /bigobj")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_CRT_SECURE_NO_WARNINGS /bigobj")
    # Turn off transitional "changed behavior" warning message for Visual Studio versions prior to 2015. The changed behavior is
    # that constructor initializers are now fixed to clear the struct members.
    add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,19>>:/wd4351>")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpointer-arith -Wno-unused-function -Wno-sign-compare")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith -Wno-unused-function -Wno-sign-compare")
endif()

run_vk_xml_generate(mock_icd_generator.py mock_icd.h)
run_vk_xml_generate(mock_icd_generator.py mock_icd.cpp)

add_vk_icd(mock_icd mock_icd.cpp mock_icd.h)

# JSON file(s) install targets. For Linux, need to remove the "./" from the library path before installing to system directories.
if((UNIX AND NOT APPLE) AND INSTALL_ICD) # i.e. Linux
    foreach(config_file ${ICD_JSON_FILES})
        add_custom_target(${config_file}-staging-json ALL
                          COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/staging-json
                          COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json ${CMAKE_CURRENT_BINARY_DIR}/staging-json
                          COMMAND sed -i -e "/\"library_path\":/s$./libVkICD$libVkICD$"
                                  ${CMAKE_CURRENT_BINARY_DIR}/staging-json/${config_file}.json
                          VERBATIM
                          DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linux/${config_file}.json)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/staging-json/${config_file}.json
                DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/vulkan/icd.d)
    endforeach(config_file)
endif()

# Windows uses the JSON file as-is.
if(WIN32 AND INSTALL_ICD)
    foreach(config_file ${ICD_JSON_FILES})
        install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/windows/${config_file}.json DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endforeach(config_file)
endif()
