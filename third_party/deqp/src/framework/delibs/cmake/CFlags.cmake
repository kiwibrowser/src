#-------------------------------------------------------------------------
# drawElements CMake utilities
# ----------------------------
#
# Copyright 2014 The Android Open Source Project
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

set(DE_COVERAGE_BUILD "OFF" CACHE STRING "Build with coverage instrumentation with GCC (ON/OFF)")

if (NOT DE_DEFS)
	message(FATAL_ERROR "Defs.cmake is not included")
endif ()

if (DE_COMPILER_IS_GCC OR DE_COMPILER_IS_CLANG)
	# Compiler flags for GCC/Clang

	set(TARGET_FLAGS "")

	if (DE_COVERAGE_BUILD)
		if (not DE_COMPILER_IS_GCC)
			message(FATAL_ERROR "Coverage build requires GCC")
		endif ()

		add_definitions("-DDE_COVERAGE_BUILD")
		set(TARGET_FLAGS	"-fprofile-arcs -ftest-coverage")
		set(LINK_FLAGS		"${LINK_FLAGS} -lgcov")
	endif ()

	# For 3rd party sw disable all warnings
	set(DE_3RD_PARTY_C_FLAGS	"${CMAKE_C_FLAGS} ${TARGET_FLAGS} -w")
	set(DE_3RD_PARTY_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${TARGET_FLAGS} -w")

	# \note Remove -Wno-sign-conversion for more warnings
	set(WARNING_FLAGS			"-Wall -Wextra -Wno-long-long -Wshadow -Wundef -Wconversion -Wno-sign-conversion")

	set(CMAKE_C_FLAGS			"${TARGET_FLAGS} ${WARNING_FLAGS} ${CMAKE_C_FLAGS} -std=c90 -pedantic ")
	set(CMAKE_CXX_FLAGS			"${TARGET_FLAGS} ${WARNING_FLAGS} ${CMAKE_CXX_FLAGS} -std=c++03 -Wno-delete-non-virtual-dtor")

	# Force compiler to generate code where integers have well defined overflow
	# Turn on -Wstrict-overflow=5 and check all warnings before removing
	set(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} -fwrapv")
	set(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS} -fwrapv")

	# Force compiler to not export any symbols.
	# Any static libraries build are linked into the standalone executable binaries.
	set(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
elseif (DE_COMPILER_IS_MSC)
	# Compiler flags for msc

	# \note Following unnecessary nagging warnings are disabled:
	# 4820: automatic padding added after data
	# 4255: no function prototype given (from system headers)
	# 4668: undefined identifier in preprocessor expression (from system headers)
	# 4738: storing 32-bit float result in memory
	# 4711: automatic inline expansion
	set(MSC_BASE_FLAGS "/DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS")
	set(MSC_WARNING_FLAGS "/W3 /wd4820 /wd4255 /wd4668 /wd4738 /wd4711")

	# For 3rd party sw disable all warnings
	set(DE_3RD_PARTY_C_FLAGS	"${CMAKE_C_FLAGS} ${MSC_BASE_FLAGS} /W0")
	set(DE_3RD_PARTY_CXX_FLAGS	"${CMAKE_CXX_FLAGS} ${MSC_BASE_FLAGS} /EHsc /W0")

	set(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${MSC_BASE_FLAGS} ${MSC_WARNING_FLAGS}")
	set(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS} ${MSC_BASE_FLAGS} /EHsc ${MSC_WARNING_FLAGS}")

else ()
	message(FATAL_ERROR "DE_COMPILER is not valid")
endif ()
