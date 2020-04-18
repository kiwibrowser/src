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

# Vulkaninfo Application Bundle

# We already have a "vulkaninfo" target, so create a new target with a different name and use the OUTPUT_NAME property to rename the
# target to the desired name. The standalone binary is called "vulkaninfo" and the bundle is called "vulkaninfo.app". Note that the
# executable is a script that launches Terminal to see the output.
add_executable(vulkaninfo-bundle
               MACOSX_BUNDLE
               vulkaninfo.c
               ${CMAKE_BINARY_DIR}/staging-json/MoltenVK_icd.json
               ${CMAKE_CURRENT_SOURCE_DIR}/macOS/vulkaninfo.sh
               ${CMAKE_CURRENT_SOURCE_DIR}/macOS/Resources/VulkanIcon.icns
               ${CMAKE_CURRENT_SOURCE_DIR}/macOS/vulkaninfo/metal_view.m
               ${CMAKE_CURRENT_SOURCE_DIR}/macOS/vulkaninfo/metal_view.h)
set_target_properties(vulkaninfo-bundle
                      PROPERTIES OUTPUT_NAME
                                 vulkaninfo
                                 MACOSX_BUNDLE_INFO_PLIST
                                 ${CMAKE_CURRENT_SOURCE_DIR}/macOS/Info.plist)
# We do this so vulkaninfo is linked to an individual library and NOT a framework.
target_link_libraries(vulkaninfo-bundle ${Vulkan_LIBRARY} "-framework AppKit -framework QuartzCore")
target_include_directories(vulkaninfo-bundle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/macOS/vulkaninfo ${VulkanHeaders_INCLUDE_DIR})
add_dependencies(vulkaninfo-bundle MoltenVK_icd-staging-json)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/macOS/vulkaninfo.sh PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/macOS/Resources/VulkanIcon.icns
                            PROPERTIES
                            MACOSX_PACKAGE_LOCATION
                            "Resources")
set_source_files_properties(${CMAKE_BINARY_DIR}/staging-json/MoltenVK_icd.json
                            PROPERTIES
                            MACOSX_PACKAGE_LOCATION
                            "Resources/vulkan/icd.d")

# Xcode projects need some extra help with what would be install steps.
if(${CMAKE_GENERATOR} MATCHES "^Xcode.*")
    add_custom_command(TARGET vulkaninfo-bundle POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy "${MOLTENVK_DIR}/MoltenVK/macOS/dynamic/libMoltenVK.dylib"
                               ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/vulkaninfo.app/Contents/Frameworks/libMoltenVK.dylib
                       DEPENDS vulkan)
else()
    add_custom_command(TARGET vulkaninfo-bundle POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy "${MOLTENVK_DIR}/MoltenVK/macOS/dynamic/libMoltenVK.dylib"
                               ${CMAKE_CURRENT_BINARY_DIR}/vulkaninfo.app/Contents/Frameworks/libMoltenVK.dylib
                       DEPENDS vulkan)
endif()

# Keep RPATH so fixup_bundle can use it to find libraries
set_target_properties(vulkaninfo-bundle PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
install(TARGETS vulkaninfo-bundle BUNDLE DESTINATION "vulkaninfo")
# Fix up the library search path in the executable to find (loader) libraries in the bundle. When fixup_bundle() is passed a bundle
# in the first argument, it looks at the Info.plist file to determine the BundleExecutable. In this case, the executable is a
# script, which can't be fixed up. Instead pass it the explicit name of the executable.
install(CODE "
    include(BundleUtilities)
    fixup_bundle(\${CMAKE_INSTALL_PREFIX}/vulkaninfo/vulkaninfo.app/Contents/MacOS/vulkaninfo \"\" \"${Vulkan_LIBRARY_DIR}\")
    ")
