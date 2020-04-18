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

# Set up common settings for building all demos on Apple platforms.

# Source for the MoltenVK ICD library and JSON file
set(MOLTENVK_DIR ${MOLTENVK_REPO_ROOT})

# MoltenVK JSON File

execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/staging-json)
execute_process(COMMAND sed -e "/\"library_path\":/s$:[[:space:]]*\"[[:space:]]*[\\.\\/]*$: \"..\\/..\\/..\\/Frameworks\\/$"
                        ${MOLTENVK_DIR}/MoltenVK/icd/MoltenVK_icd.json
                OUTPUT_FILE ${CMAKE_BINARY_DIR}/staging-json/MoltenVK_icd.json)

# ~~~
# Modify the ICD JSON file to adjust the library path.
# The ICD JSON file goes in the Resources/vulkan/icd.d directory, so adjust the
# library_path to the relative path to the Frameworks directory in the bundle.
# The regex does: substitute ':<whitespace>"<whitespace><all occurences of . and />' with:
# ': "../../../Frameworks/'
# ~~~
add_custom_target(MoltenVK_icd-staging-json ALL
                  COMMAND mkdir -p ${CMAKE_BINARY_DIR}/staging-json
                  COMMAND sed -e "/\"library_path\":/s$:[[:space:]]*\"[[:space:]]*[\\.\\/]*$: \"..\\/..\\/..\\/Frameworks\\/$"
                          ${MOLTENVK_DIR}/MoltenVK/icd/MoltenVK_icd.json > ${CMAKE_BINARY_DIR}/staging-json/MoltenVK_icd.json
                  VERBATIM
                  DEPENDS "${MOLTENVK_DIR}/MoltenVK/icd/MoltenVK_icd.json")
set_source_files_properties(${CMAKE_BINARY_DIR}/staging-json/MoltenVK_icd.json PROPERTIES GENERATED TRUE)

find_library(COCOA NAMES Cocoa)

# Locate Interface Builder Tool, needed to build things like Storyboards outside of Xcode.
if(NOT ${CMAKE_GENERATOR} MATCHES "^Xcode.*")
    # Make sure we can find the 'ibtool' program. If we can NOT find it we skip generation of this project.
    find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
    if(${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
        message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. "
                           "It should have been installed with the Apple developer tools. "
                           "The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin.")
    endif()
endif()
