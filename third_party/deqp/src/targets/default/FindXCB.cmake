#-------------------------------------------------------------------------
# drawElements CMake utilities
# ----------------------------
#
# Copyright (c) 2016 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#-------------------------------------------------------------------------

# FindXCB
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(XCB xcb)
FIND_PATH(XCB_INCLUDE_DIR	NAMES	xcb/xcb.h	HINTS	${PKG_XCB_INCLUDE_DIRS})
FIND_LIBRARY(XCB_LIBRARIES	NAMES	xcb		HINTS	${PKG_XCB_LIBRARY_DIRS})
FIND_PACKAGE_HANDLE_STANDARD_ARGS(XCB	DEFAULT_MSG	XCB_LIBRARIES	XCB_INCLUDE_DIR)
