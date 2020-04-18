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

# Platform defines.
set(CMAKE_SYSTEM_NAME Linux)

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

set(CMAKE_CROSSCOMPILING 1)

# NDK installation path
if (NOT DEFINED ANDROID_NDK_PATH)
	message(FATAL_ERROR "Please provide ANDROID_NDK_PATH")
endif ()

# Host os (for toolchain binaries)
if (NOT DEFINED ANDROID_NDK_HOST_OS)
	message(FATAL_ERROR "Please provide ANDROID_NDK_HOST_OS")
endif ()

# Compile target
set(ANDROID_ABI			"armeabi-v7a"			CACHE STRING "Android ABI")
set(ANDROID_NDK_TARGET	"android-${DE_ANDROID_API}")

# dE defines
set(DE_OS "DE_OS_ANDROID")

if (NOT DEFINED DE_COMPILER)
	set(DE_COMPILER	"DE_COMPILER_CLANG")
endif ()

if (NOT DEFINED DE_ANDROID_API)
	set(DE_ANDROID_API 9)
endif ()

set(COMMON_C_FLAGS		"-D__STDC_INT64__")
set(COMMON_CXX_FLAGS	"${COMMON_C_FLAGS} -frtti -fexceptions")
set(COMMON_LINKER_FLAGS	"")

# ABI-dependent bits
if (ANDROID_ABI STREQUAL "x86")
	set(DE_CPU					"DE_CPU_X86")
	set(CMAKE_SYSTEM_PROCESSOR	i686-android-linux)

	set(ANDROID_CC_PATH			"${ANDROID_NDK_PATH}/toolchains/x86-4.9/prebuilt/${ANDROID_NDK_HOST_OS}/")
	set(CROSS_COMPILE			"${ANDROID_CC_PATH}bin/i686-linux-android-")
	set(ANDROID_SYSROOT			"${ANDROID_NDK_PATH}/platforms/${ANDROID_NDK_TARGET}/arch-x86")

	set(CMAKE_FIND_ROOT_PATH
		"${ANDROID_CC_PATH}i686-linux-android"
		"${ANDROID_CC_PATH}lib/gcc/i686-linux-android/4.9"
		)

	set(TARGET_C_FLAGS			"-march=i686 -msse3 -mstackrealign -mfpmath=sse")
	set(TARGET_LINKER_FLAGS		"")
	set(LLVM_TRIPLE				"i686-none-linux-android")

elseif (ANDROID_ABI STREQUAL "armeabi" OR
		ANDROID_ABI STREQUAL "armeabi-v7a")
	set(DE_CPU					"DE_CPU_ARM")
	set(CMAKE_SYSTEM_PROCESSOR	arm-linux-androideabi)

	set(ANDROID_CC_PATH	"${ANDROID_NDK_PATH}/toolchains/arm-linux-androideabi-4.9/prebuilt/${ANDROID_NDK_HOST_OS}/")
	set(CROSS_COMPILE	"${ANDROID_CC_PATH}bin/arm-linux-androideabi-")
	set(ANDROID_SYSROOT	"${ANDROID_NDK_PATH}/platforms/${ANDROID_NDK_TARGET}/arch-arm")
	set(ARM_C_FLAGS		"-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ ")

	if (ANDROID_ABI STREQUAL "armeabi-v7a")
		set(TARGET_C_FLAGS		"${ARM_C_FLAGS} -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp")
		set(TARGET_LINKER_FLAGS	"-Wl,--fix-cortex-a8 -march=armv7-a")
		set(LLVM_TRIPLE			"armv7-none-linux-androideabi")

	else () # armeabi
		set(TARGET_C_FLAGS		"${ARM_C_FLAGS} -march=armv5te -mfloat-abi=softfp")
		set(TARGET_LINKER_FLAGS	"-Wl,--fix-cortex-a8 -march=armv5te")
		set(LLVM_TRIPLE			"armv5te-none-linux-androideabi")
	endif ()

	set(CMAKE_FIND_ROOT_PATH
		"${ANDROID_CC_PATH}arm-linux-androideabi"
		)

elseif (ANDROID_ABI STREQUAL "arm64-v8a")
	set(DE_CPU					"DE_CPU_ARM_64")
	set(CMAKE_SYSTEM_PROCESSOR	aarch64-linux-android)
	set(CMAKE_SIZEOF_VOID_P		8)

	set(ANDROID_CC_PATH	"${ANDROID_NDK_PATH}/toolchains/aarch64-linux-android-4.9/prebuilt/${ANDROID_NDK_HOST_OS}/")
	set(CROSS_COMPILE	"${ANDROID_CC_PATH}bin/aarch64-linux-android-")
	set(ANDROID_SYSROOT	"${ANDROID_NDK_PATH}/platforms/${ANDROID_NDK_TARGET}/arch-arm64")

	set(CMAKE_FIND_ROOT_PATH
		"${ANDROID_CC_PATH}arm-linux-androideabi"
		)

	set(TARGET_C_FLAGS		"-march=armv8-a")
	set(TARGET_LINKER_FLAGS	"-Wl,--fix-cortex-a53-835769 -Wl,--fix-cortex-a53-835769 -march=armv8-a")
	set(LLVM_TRIPLE			"aarch64-none-linux-android")

	if (DE_COMPILER STREQUAL "DE_COMPILER_GCC")
		set(TARGET_C_FLAGS "${TARGET_C_FLAGS} -mabi=lp64")
	endif ()

elseif (ANDROID_ABI STREQUAL "x86_64")
	set(DE_CPU					"DE_CPU_X86_64")
	set(CMAKE_SYSTEM_PROCESSOR	x86_64-linux-android)
	set(CMAKE_SIZEOF_VOID_P		8)

	set(CMAKE_LIBRARY_PATH "/usr/lib64")

	set(ANDROID_CC_PATH	"${ANDROID_NDK_PATH}/toolchains/x86_64-4.9/prebuilt/${ANDROID_NDK_HOST_OS}/")
	set(CROSS_COMPILE	"${ANDROID_CC_PATH}bin/x86_64-linux-android-")
	set(ANDROID_SYSROOT	"${ANDROID_NDK_PATH}/platforms/${ANDROID_NDK_TARGET}/arch-x86_64")

	set(CMAKE_FIND_ROOT_PATH
		"${ANDROID_CC_PATH}x86_64-linux-android"
		)

	set(LLVM_TRIPLE			"x86_64-none-linux-android")

else ()
	message(FATAL_ERROR "Unknown ABI \"${ANDROID_ABI}\"")
endif ()

# Use LLVM libc++ for full C++11 support
set(ANDROID_CXX_LIBRARY		"${ANDROID_NDK_PATH}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libc++_static.a")
set(CXX_INCLUDES			"-I${ANDROID_NDK_PATH}/sources/cxx-stl/llvm-libc++/libcxx/include")
set(CMAKE_FIND_ROOT_PATH	"" ${CMAKE_FIND_ROOT_PATH})

set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ANDROID_SYSROOT})

include(CMakeForceCompiler)

if (ANDROID_NDK_HOST_OS STREQUAL "windows" OR
	ANDROID_NDK_HOST_OS STREQUAL "windows-x86_64")
	set(BIN_EXT ".exe")
else ()
	set(BIN_EXT "")
endif ()

if (DE_COMPILER STREQUAL "DE_COMPILER_GCC")
	set(CMAKE_C_COMPILER	"${CROSS_COMPILE}gcc${BIN_EXT}"		CACHE FILEPATH "C Compiler")
	set(CMAKE_CXX_COMPILER	"${CROSS_COMPILE}g++${BIN_EXT}"		CACHE FILEPATH "C++ Compiler")

	set(TARGET_C_FLAGS		"-mandroid ${TARGET_C_FLAGS}")

elseif (DE_COMPILER STREQUAL "DE_COMPILER_CLANG")
	set(LLVM_PATH "${ANDROID_NDK_PATH}/toolchains/llvm/prebuilt/${ANDROID_NDK_HOST_OS}/")

	set(CMAKE_C_COMPILER	"${LLVM_PATH}bin/clang${BIN_EXT}"	CACHE FILEPATH "C Compiler")
	set(CMAKE_CXX_COMPILER	"${LLVM_PATH}bin/clang++${BIN_EXT}"	CACHE FILEPATH "C++ Compiler")
	set(CMAKE_AR			"${CROSS_COMPILE}ar${BIN_EXT}"		CACHE FILEPATH "Archiver")
	set(CMAKE_RANLIB		"${CROSS_COMPILE}ranlib${BIN_EXT}"	CACHE FILEPATH "Indexer")

	set(TARGET_C_FLAGS		"-target ${LLVM_TRIPLE} -gcc-toolchain ${ANDROID_CC_PATH} ${TARGET_C_FLAGS}")
	set(TARGET_LINKER_FLAGS	"-target ${LLVM_TRIPLE} -gcc-toolchain ${ANDROID_CC_PATH} ${TARGET_LINKER_FLAGS}")

endif ()

set(CMAKE_SHARED_LIBRARY_C_FLAGS	"")
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS	"")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# \note Without CACHE STRING FORCE cmake ignores these.
set(CMAKE_C_FLAGS				"--sysroot=${ANDROID_SYSROOT} ${CMAKE_C_FLAGS} ${COMMON_C_FLAGS} ${TARGET_C_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS				"--sysroot=${ANDROID_SYSROOT} ${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS} ${TARGET_C_FLAGS} ${CXX_INCLUDES} -I${ANDROID_NDK_PATH}/sources/android/support/include" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS	"-nodefaultlibs -Wl,-shared,-Bsymbolic -Wl,--no-undefined ${COMMON_LINKER_FLAGS} ${TARGET_LINKER_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS		"-nodefaultlibs ${COMMON_LINKER_FLAGS} ${TARGET_LINKER_FLAGS}" CACHE STRING "" FORCE)
