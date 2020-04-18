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

# Android
message("*** Using Android")
set(DEQP_TARGET_NAME	"Android")
set(DEQP_SUPPORT_GLES1	ON)

# GLESv1 lib
find_library(GLES1_LIBRARY GLESv1_CM PATHS /usr/lib)
set(DEQP_GLES1_LIBRARIES ${GLES1_LIBRARY})

# GLESv2 lib
find_library(GLES2_LIBRARY GLESv2 PATHS /usr/lib)
set(DEQP_GLES2_LIBRARIES ${GLES2_LIBRARY})

# EGL lib
find_library(EGL_LIBRARY EGL PATHS /usr/lib)
set(DEQP_EGL_LIBRARIES ${EGL_LIBRARY})

# Platform libs
find_library(LOG_LIBRARY NAMES log PATHS /usr/lib)
set(DEQP_PLATFORM_LIBRARIES ${DEQP_PLATFORM_LIBRARIES} ${LOG_LIBRARY})

if (DE_ANDROID_API GREATER 8)
	# libandroid for NativeActivity APIs
	find_library(ANDROID_LIBRARY NAMES android PATHS /usr/lib)
	set(DEQP_PLATFORM_LIBRARIES ${DEQP_PLATFORM_LIBRARIES} ${ANDROID_LIBRARY})
endif ()

# Android uses customized execserver
include_directories(execserver)
set(DEQP_PLATFORM_LIBRARIES xscore ${DEQP_PLATFORM_LIBRARIES})
