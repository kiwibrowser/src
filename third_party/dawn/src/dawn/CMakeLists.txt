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

###############################################################################
# Dawn headers
###############################################################################

DawnJSONGenerator(
    TARGET "dawn_headers"
    PRINT_NAME "Dawn headers"
    RESULT_VARIABLE "DAWN_HEADERS_GEN_SOURCES"
)

# Headers only INTERFACE library with generated headers don't work in CMake
# because the GENERATED property is local to a directory. Instead we make a
# STATIC library with a Dummy cpp file.
#
# INTERFACE libraries can only have INTERFACE sources so the sources get added
# to the dependant's list of sources. If these dependents are in another
# directory, they don't see the GENERATED property and fail to configure
# because the file doesn't exist on disk.
add_library(dawn_headers STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_headers PRIVATE
    "${DAWN_INCLUDE_DIR}/dawn/dawn_wsi.h"
    ${DAWN_HEADERS_GEN_SOURCES}
)
target_link_libraries(dawn_headers INTERFACE dawn_public_config)

###############################################################################
# Dawn C++ headers
###############################################################################

DawnJSONGenerator(
    TARGET "dawncpp_headers"
    PRINT_NAME "Dawn C++ headers"
    RESULT_VARIABLE "DAWNCPP_HEADERS_GEN_SOURCES"
)

# This headers only library needs to be a STATIC library, see comment for
# dawn_headers above.
add_library(dawncpp_headers STATIC ${DAWN_DUMMY_FILE})
target_sources(dawncpp_headers PRIVATE
    "${DAWN_INCLUDE_DIR}/dawn/EnumClassBitmasks.h"
    ${DAWNCPP_HEADERS_GEN_SOURCES}
)
target_link_libraries(dawncpp_headers INTERFACE dawn_headers)

###############################################################################
# Dawn C++ wrapper
###############################################################################

DawnJSONGenerator(
    TARGET "dawncpp"
    PRINT_NAME "Dawn C++ wrapper"
    RESULT_VARIABLE "DAWNCPP_GEN_SOURCES"
)

add_library(dawncpp STATIC ${DAWN_DUMMY_FILE})
target_sources(dawncpp PRIVATE ${DAWNCPP_GEN_SOURCES})
target_link_libraries(dawncpp PUBLIC dawncpp_headers)

###############################################################################
# libdawn_proc
###############################################################################

DawnJSONGenerator(
    TARGET "dawn_proc"
    PRINT_NAME "Dawn C++ wrapper"
    RESULT_VARIABLE "DAWNPROC_GEN_SOURCES"
)

add_library(dawn_proc STATIC ${DAWN_DUMMY_FILE})
target_sources(dawn_proc PRIVATE ${DAWNPROC_GEN_SOURCES})
target_link_libraries(dawn_proc PUBLIC dawn_headers)
