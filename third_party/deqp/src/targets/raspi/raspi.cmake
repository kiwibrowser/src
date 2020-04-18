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

# Raspberry Pi target
message("*** Using Raspberry Pi")
set(DEQP_TARGET_NAME	"Raspberry Pi")
set(DEQP_SUPPORT_VG		ON)

find_path(SYSTEM_INCLUDE bcm_host.h PATHS /usr/include /opt/vc/include)
include_directories(
	${SYSTEM_INCLUDE}
	${SYSTEM_INCLUDE}/interface/vcos/pthreads
	)

# GLESv2 lib
find_library(GLES2_LIBRARY GLESv2 PATHS /usr/lib /opt/vc/lib)
set(DEQP_GLES2_LIBRARIES ${GLES2_LIBRARY})

# OpenVG lib
find_library(OPENVG_LIBRARY OpenVG PATHS /usr/lib /opt/vc/lib)
set(DEQP_VG_LIBRARIES ${OPENVG_LIBRARY})

# EGL lib
find_library(EGL_LIBRARY EGL PATHS /usr/lib /opt/vc/lib)
set(DEQP_EGL_LIBRARIES ${EGL_LIBRARY})

# Platform libs
find_library(BCM_HOST_LIBRARY NAMES bcm_host PATHS /usr/lib /opt/vc/lib)
set(DEQP_PLATFORM_LIBRARIES ${DEQP_PLATFORM_LIBRARIES} ${BCM_HOST_LIBRARY} ${GLES2_LIBRARY} ${EGL_LIBRARY})

get_filename_component(SYSLIB_PATH ${BCM_HOST_LIBRARY} PATH)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${SYSLIB_PATH}")

# Platform sources
set(TCUTIL_PLATFORM_SRCS
	raspi/tcuRaspiPlatform.cpp
	raspi/tcuRaspiPlatform.hpp
	tcuMain.cpp
	)
