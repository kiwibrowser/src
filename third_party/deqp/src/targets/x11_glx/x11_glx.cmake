#-------------------------------------------------------------------------
# drawElements CMake utilities
# ----------------------------
#
# Copyright 2016 The Android Open Source Project
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

message("*** Using X11 GLX target")
set(DEQP_TARGET_NAME	"X11 GLX")
set(DEQP_SUPPORT_GLX	ON)

# Use X11 target
set(DEQP_USE_X11		ON)

find_package(X11)
if (NOT X11_FOUND)
	message(FATAL_ERROR "X11 development package not found")
endif ()

set(DEQP_PLATFORM_LIBRARIES ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})
