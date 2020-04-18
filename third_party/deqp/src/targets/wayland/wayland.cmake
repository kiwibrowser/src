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

message("*** Using Wayland target")
set(DEQP_TARGET_NAME "Wayland")

# Use Wayland target
set(DEQP_USE_WAYLAND	ON)

# Add FindWayland module
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/targets/default")

find_package(Wayland)
if (NOT WAYLAND_FOUND)
	message(FATAL_ERROR "Wayland development package not found")
endif ()

set(DEQP_PLATFORM_LIBRARIES ${WAYLAND_LIBRARIES})
include_directories(${WAYLAND_INCLUDE_DIR})