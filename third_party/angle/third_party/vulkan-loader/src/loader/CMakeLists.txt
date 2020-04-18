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

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_BINARY_DIR} ${CMAKE_BINARY_DIR})

# Check for the existance of the secure_getenv or __secure_getenv commands
include(CheckFunctionExists)
check_function_exists(secure_getenv HAVE_SECURE_GETENV)
check_function_exists(__secure_getenv HAVE___SECURE_GETENV)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/loader_cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/loader_cmake_config.h)

if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DWIN32_LEAN_AND_MEAN)
    if(MSVC AND NOT MSVC_VERSION LESS 1900)
        # Enable control flow guard
        message(STATUS "Building loader with control flow guard")
        set(MSVC_LOADER_COMPILE_OPTIONS ${MSVC_LOADER_COMPILE_OPTIONS} /guard:cf)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /guard:cf")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /guard:cf")
    endif()
elseif(ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX AND NOT APPLE) # i.e.: Linux
    if(BUILD_WSI_XCB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    endif()

    if(BUILD_WSI_XLIB_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_XLIB_KHR -DVK_USE_PLATFORM_XLIB_XRANDR_EXT)
    endif()

    if(BUILD_WSI_WAYLAND_SUPPORT)
        add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR)
    endif()
else()
    message(FATAL_ERROR "Unsupported Platform!")
endif()

# DEBUG enables runtime loader ICD verification
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

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

    if(ENABLE_WIN10_ONECORE)
        # Note: When linking your app or driver to OneCore.lib, be sure to remove any links to non-umbrella libs (such as
        # kernel32.lib).
        set(CMAKE_CXX_STANDARD_LIBRARIES " ") # space is intentional
        set(CMAKE_C_STANDARD_LIBRARIES ${CMAKE_CXX_STANDARD_LIBRARIES})
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
    # ~~~
    # Build dev_ext_trampoline.c with -O2 to allow tail-call optimization.
    # Build other C files with normal options.
    # Setup two CMake targets (loader-norm and loader-opt) for the different compilation flags.
    # ~~~
    separate_arguments(LOCAL_C_FLAGS_DBG WINDOWS_COMMAND ${CMAKE_C_FLAGS_DEBUG})
    set(CMAKE_C_FLAGS_DEBUG " ")
    separate_arguments(LOCAL_C_FLAGS_REL WINDOWS_COMMAND ${CMAKE_C_FLAGS_RELEASE})
endif()

set(NORMAL_LOADER_SRCS
    extension_manual.c
    loader.c
    loader.h
    vk_loader_platform.h
    vk_loader_layer.h
    trampoline.c
    wsi.c
    wsi.h
    debug_utils.c
    debug_utils.h
    gpa_helper.h
    cJSON.c
    cJSON.h
    murmurhash.c
    murmurhash.h)

set(OPT_LOADER_SRCS dev_ext_trampoline.c phys_dev_ext.c)

# Check for assembler support
set(ASM_FAILURE_MSG "The build will fall back on building with C code\n")
set(ASM_FAILURE_MSG "${ASM_FAILURE_MSG}Note that this may be unsafe, as the C code requires tail-call optimizations to remove")
set(ASM_FAILURE_MSG "${ASM_FAILURE_MSG} the stack frame for certain calls. If the compiler does not do this, then unknown device")
set(ASM_FAILURE_MSG "${ASM_FAILURE_MSG} extensions will suffer from a corrupted stack.")
if(WIN32)
    enable_language(ASM_MASM)
    if(CMAKE_ASM_MASM_COMPILER_WORKS)
        if(NOT CMAKE_CL_64)
            set(CMAKE_ASM_MASM_FLAGS ${CMAKE_ASM_MASM_FLAGS} /safeseh)
        endif()

        add_executable(asm_offset asm_offset.c)
        add_dependencies(asm_offset generate_helper_files loader_gen_files)
        target_link_libraries(asm_offset Vulkan::Headers)
        add_custom_command(OUTPUT gen_defines.asm DEPENDS asm_offset COMMAND asm_offset MASM)
        add_custom_target(loader_asm_gen_files DEPENDS gen_defines.asm)
        set_target_properties(loader_asm_gen_files PROPERTIES FOLDER ${LOADER_HELPER_FOLDER})
        add_library(loader-unknown-chain OBJECT unknown_ext_chain_masm.asm)
        add_dependencies(loader-unknown-chain loader_asm_gen_files)
    else()
        message(WARNING "Could not find working MASM assebler\n${ASM_FAILURE_MSG}")
        add_custom_target(loader_asm_gen_files)
        add_library(loader-unknown-chain OBJECT unknown_ext_chain.c)
        add_dependencies(loader-unknown-chain generate_helper_files loader_gen_files)
        target_compile_options(loader-unknown-chain PUBLIC "$<$<CONFIG:DEBUG>:${LOCAL_C_FLAGS_REL}>")
        target_compile_options(loader-unknown-chain PUBLIC ${MSVC_LOADER_COMPILE_OPTIONS})
    endif()
elseif(APPLE)
    # For MacOS, use the C code and force the compiler's tail-call optimization instead of using assembly code.
    set(OPT_LOADER_SRCS ${OPT_LOADER_SRCS} unknown_ext_chain.c)
    set_source_files_properties(${OPT_LOADER_SRCS} PROPERTIES COMPILE_FLAGS -O)
    add_custom_target(loader_asm_gen_files) # This causes no assembly files to be generated.
else(UNIX AND NOT APPLE) # i.e.: Linux
    enable_language(ASM-ATT)
    set(CMAKE_ASM-ATT_COMPILE_FLAGS "${CMAKE_ASM-ATT_COMPILE_FLAGS} $ENV{ASFLAGS}")
    set(CMAKE_ASM-ATT_COMPILE_FLAGS "${CMAKE_ASM-ATT_COMPILE_FLAGS} -I\"${CMAKE_CURRENT_BINARY_DIR}\"")

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/asm_test.asm
               ".intel_syntax noprefix\n.text\n.global sample\nsample:\nmov ecx, [eax + 16]\n")
    # try_compile uses the C/C++ linker flags even for ASM,
    # while they're not valid for ASM and making linking fail.
    set(TMP_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
    set(CMAKE_EXE_LINKER_FLAGS "")
    try_compile(ASSEMBLER_WORKS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/asm_test.asm)
    set(CMAKE_EXE_LINKER_FLAGS ${TMP_EXE_LINKER_FLAGS})
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/asm_test.asm)
    if(ASSEMBLER_WORKS)
        set(CMAKE_ASM-ATT_FLAGS "$ENV{ASFLAGS} -I\"${CMAKE_CURRENT_BINARY_DIR}\"")
        set(OPT_LOADER_SRCS ${OPT_LOADER_SRCS} unknown_ext_chain_gas.asm)
        add_executable(asm_offset asm_offset.c)
        add_dependencies(asm_offset generate_helper_files loader_gen_files)
        target_link_libraries(asm_offset Vulkan::Headers)
        add_custom_command(OUTPUT gen_defines.asm DEPENDS asm_offset COMMAND asm_offset GAS)
        add_custom_target(loader_asm_gen_files DEPENDS gen_defines.asm)
    else()
        message(WARNING "Could not find working x86 GAS assembler\n${ASM_FAILURE_MSG}")
        set(OPT_LOADER_SRCS ${OPT_LOADER_SRCS} unknown_ext_chain.c)
        add_custom_target(loader_asm_gen_files)
    endif()
endif()

GenerateFromVkXml(vk_loader_extensions.h loader_extension_generator.py)
GenerateFromVkXml(vk_loader_extensions.c loader_extension_generator.py)
add_custom_target(loader_gen_files DEPENDS vk_loader_extensions.h vk_loader_extensions.c)
set_target_properties(loader_gen_files PROPERTIES FOLDER ${LOADER_HELPER_FOLDER})

if(WIN32)
    add_library(loader-norm OBJECT ${NORMAL_LOADER_SRCS} dirent_on_windows.c)
    add_dependencies(loader-norm generate_helper_files loader_gen_files)
    target_compile_options(loader-norm PUBLIC "$<$<CONFIG:DEBUG>:${LOCAL_C_FLAGS_DBG}>")
    target_compile_options(loader-norm PUBLIC ${MSVC_LOADER_COMPILE_OPTIONS})
    target_include_directories(loader-norm PRIVATE "$<TARGET_PROPERTY:Vulkan::Headers,INTERFACE_INCLUDE_DIRECTORIES>")

    add_library(loader-opt OBJECT ${OPT_LOADER_SRCS})
    add_dependencies(loader-opt generate_helper_files loader_gen_files loader_asm_gen_files)
    target_compile_options(loader-opt PUBLIC "$<$<CONFIG:DEBUG>:${LOCAL_C_FLAGS_REL}>")
    target_compile_options(loader-opt PUBLIC ${MSVC_LOADER_COMPILE_OPTIONS})
    target_include_directories(loader-opt PRIVATE "$<TARGET_PROPERTY:Vulkan::Headers,INTERFACE_INCLUDE_DIRECTORIES>")

    if(NOT ENABLE_STATIC_LOADER)
        target_compile_definitions(loader-norm PUBLIC LOADER_DYNAMIC_LIB)
        target_compile_definitions(loader-opt PUBLIC LOADER_DYNAMIC_LIB)

        add_library(vulkan
                    SHARED
                    $<TARGET_OBJECTS:loader-opt>
                    $<TARGET_OBJECTS:loader-norm>
                    $<TARGET_OBJECTS:loader-unknown-chain>
                    ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-1.def
                    ${CMAKE_CURRENT_SOURCE_DIR}/loader.rc)
        set_target_properties(vulkan
                              PROPERTIES LINK_FLAGS_DEBUG
                                         "/ignore:4098"
                                         OUTPUT_NAME
                                         vulkan-1)
        target_link_libraries(vulkan Vulkan::Headers)
    else()
        add_library(vulkan
                    STATIC
                    $<TARGET_OBJECTS:loader-opt>
                    $<TARGET_OBJECTS:loader-norm>
                    $<TARGET_OBJECTS:loader-unknown-chain>)
        set_target_properties(vulkan PROPERTIES OUTPUT_NAME VKstatic.1)
    endif()

    if(ENABLE_WIN10_ONECORE)
        target_link_libraries(vulkan OneCoreUAP.lib LIBCMT.LIB LIBCMTD.LIB LIBVCRUNTIME.LIB LIBUCRT.LIB)
        set_target_properties(vulkan PROPERTIES LINK_FLAGS "/NODEFAULTLIB")
    else()
        target_link_libraries(vulkan Cfgmgr32)
    endif()

    add_dependencies(vulkan generate_helper_files loader_gen_files loader_asm_gen_files)

else()
    # Linux and MacOS
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")

    # Clang (and not gcc) warns about redefining a typedef with the same types, so disable that warning. Note that it will still
    # throw an error if a typedef is redefined with a different type.
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-typedef-redefinition")
    endif()

    add_library(vulkan SHARED ${NORMAL_LOADER_SRCS} ${OPT_LOADER_SRCS})
    add_dependencies(vulkan generate_helper_files loader_gen_files loader_asm_gen_files)
    target_compile_definitions(vulkan PUBLIC -DLOADER_DYNAMIC_LIB)
    set_target_properties(vulkan
                          PROPERTIES SOVERSION
                                     "1"
                                     VERSION
                                     "${VulkanHeaders_VERSION_MAJOR}.${VulkanHeaders_VERSION_MINOR}.${VulkanHeaders_VERSION_PATCH}")
    target_link_libraries(vulkan -ldl -lpthread -lm)
    target_link_libraries(vulkan Vulkan::Headers)

    if(APPLE)
        find_library(COREFOUNDATION_LIBRARY NAMES CoreFoundation)
        target_link_libraries(vulkan "-framework CoreFoundation")

        # Build vulkan.framework
        set(FRAMEWORK_HEADERS
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vk_icd.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vk_layer.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vk_platform.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vk_sdk_platform.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_android.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_core.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_ios.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_macos.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_vi.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_wayland.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_win32.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_xcb.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_xlib.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan_xlib_xrandr.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan.h
            ${VulkanHeaders_INCLUDE_DIRS}/vulkan/vulkan.hpp)
        add_library(vulkan-framework SHARED ${NORMAL_LOADER_SRCS} ${OPT_LOADER_SRCS} ${FRAMEWORK_HEADERS})
        add_dependencies(vulkan-framework generate_helper_files loader_gen_files loader_asm_gen_files)
        target_compile_definitions(vulkan-framework PUBLIC -DLOADER_DYNAMIC_LIB)
        target_link_libraries(vulkan-framework -ldl -lpthread -lm "-framework CoreFoundation")
        target_link_libraries(vulkan-framework Vulkan::Headers)

        # The FRAMEWORK_VERSION needs to be "A" here so that Xcode code-signing works when a user adds their framework to an Xcode
        # project and does "Sign on Copy". It would have been nicer to use "1" to denote Vulkan 1. Although Apple docs say that a
        # framework version does not have to be "A", this part of the Apple toolchain expects it.
        # https://forums.developer.apple.com/thread/65963

# cmake-format: off
        set_target_properties(vulkan-framework PROPERTIES
            OUTPUT_NAME vulkan
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            VERSION "${VulkanHeaders_VERSION_MAJOR}.${VulkanHeaders_VERSION_MINOR}.${VulkanHeaders_VERSION_PATCH}" # "current version"
            SOVERSION "1.0.0"                        # "compatibility version"
            MACOSX_FRAMEWORK_IDENTIFIER com.lunarg.vulkanFramework
            PUBLIC_HEADER "${FRAMEWORK_HEADERS}"
        )
        install(TARGETS vulkan-framework
            PUBLIC_HEADER DESTINATION vulkan
            FRAMEWORK DESTINATION loader
        )
# cmake-format: on
    endif()

    if(NOT APPLE)
        # Generate pkg-config file.
        include(FindPkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            set(VK_API_VERSION "${VulkanHeaders_VERSION_MAJOR}.${VulkanHeaders_VERSION_MINOR}.${VulkanHeaders_VERSION_PATCH}")
            foreach(LIB ${CMAKE_CXX_IMPLICIT_LINK_LIBRARIES} ${PLATFORM_LIBS})
                set(PRIVATE_LIBS "${PRIVATE_LIBS} -l${LIB}")
            endforeach()
            configure_file("vulkan.pc.in" "vulkan.pc" @ONLY)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vulkan.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
        endif()
    endif()
endif()

target_link_libraries(vulkan Vulkan::Headers)
add_library(Vulkan::Vulkan ALIAS vulkan)

install(TARGETS vulkan
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
