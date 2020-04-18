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
# iOS Target

set(DEQP_TARGET_NAME	"iOS")

# Libraries
find_library(GLES2_LIBRARY			NAMES	OpenGLES		PATHS /System/Library/Frameworks)
find_library(FOUNDATION_LIBRARY		NAMES	Foundation		PATHS /System/Library/Frameworks)
find_library(UIKIT_LIBRARY			NAMES	UIKit			PATHS /System/Library/Frameworks)
find_library(COREGRAPHICS_LIBRARY	NAMES	CoreGraphics	PATHS /System/Library/Frameworks)
find_library(QUARTZCORE_LIBRARY		NAMES	QuartzCore		PATHS /System/Library/Frameworks)

set(DEQP_GLES2_LIBRARIES		${GLES2_LIBRARY})
set(DEQP_GLES3_LIBRARIES		${GLES2_LIBRARY})
set(DEQP_PLATFORM_LIBRARIES		${FOUNDATION_LIBRARY} ${UIKIT_LIBRARY} ${COREGRAPHICS_LIBRARY} ${QUARTZCORE_LIBRARY})

# Execserver is compiled in
include_directories(execserver)
