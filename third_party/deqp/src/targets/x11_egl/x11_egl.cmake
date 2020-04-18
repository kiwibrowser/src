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

message("*** Using X11 EGL target")
set(DEQP_TARGET_NAME	"X11 EGL")
set(DEQP_SUPPORT_GLX	OFF)

# Use X11 target
set(DEQP_USE_X11	ON)

find_package(X11)
if (NOT X11_FOUND)
	message(FATAL_ERROR "X11 development package not found")
endif ()

# Support GLES1, we use pkg-config because some distributions do not ship
# GLES1 libraries and headers, this way user can override search path by
# using PKG_CONFIG_PATH environment variable
FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GLES1 glesv1_cm)
if (GLES1_LIBRARIES)
	set(DEQP_SUPPORT_GLES1	ON)
	set(DEQP_GLES1_LIBRARIES ${GLES1_LIBRARIES})
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${GLES1_LIBRARY_DIRS}")
	if ("${PKG_GLES1_INCLUDE_DIRS}" STREQUAL "")
		# PKG_GLES1_INCLUDE_DIRS empty, see if matching include
		# path (GLES/gl.h) exists beside library directory
		set(GLES1_INCLUDE "${GLES1_LIBDIR}/../include")
		if (EXISTS ${GLES1_INCLUDE}/GLES/gl.h)
			include_directories(${GLES1_INCLUDE})
		else()
			message(FATAL_ERROR "Could not find include path for GLES1 headers")
		endif (EXISTS ${GLES1_INCLUDE}/GLES/gl.h)
	endif ("${PKG_GLES1_INCLUDE_DIRS}" STREQUAL "")
endif (GLES1_LIBRARIES)

set(DEQP_PLATFORM_LIBRARIES ${X11_LIBRARIES})
include_directories(${X11_INCLUDE_DIR})
