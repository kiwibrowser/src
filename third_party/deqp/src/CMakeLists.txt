# dEQP cmake file

# Module FindGit requires cmake >= 2.8.2
cmake_minimum_required(VERSION 2.8.2)

find_package(Git)

# dEQP Target.
set(DEQP_TARGET "default" CACHE STRING "dEQP Target (default, android...)")

if (DEFINED DEQP_TARGET_TOOLCHAIN)
	# \note Toolchain must be included before project() command
	include(targets/${DEQP_TARGET}/${DEQP_TARGET_TOOLCHAIN}.cmake)
endif ()

project(dEQP-Core-${DEQP_TARGET})

include(framework/delibs/cmake/Defs.cmake NO_POLICY_SCOPE)
include(framework/delibs/cmake/CFlags.cmake)

add_definitions(-DDE_ASSERT_FAILURE_CALLBACK)

# dEQP-specific configuration. Target file should override these.
set(DEQP_TARGET_NAME		"UNKNOWN")		# Target name

set(DEQP_GLES2_LIBRARIES	)				# GLESv2 libraries. If empty, run-time linking is used
set(DEQP_GLES3_LIBRARIES	)				# GLESv3 libraries. If empty, run-time linking is used
set(DEQP_EGL_LIBRARIES		)				# EGL libraries

# Legacy APIs that don't use run-time loading
set(DEQP_SUPPORT_GLES1		OFF)			# Is GLESv1 supported
set(DEQP_GLES1_LIBRARIES	)				# GLESv1 libraries
set(DEQP_SUPPORT_VG			OFF)			# Is OpenVG supported
set(DEQP_VG_LIBRARIES		)				# VG libraries

set(DEQP_SUPPORT_WGL		OFF)
set(DEQP_SUPPORT_GLX		OFF)			# Is GLX supported \todo [2016-10-12 pyry] X11 specific - move to framework/platform/

set(DEQP_PLATFORM_LIBRARIES	)				# Other platform libraries

set(DEQP_PLATFORM_COPY_LIBRARIES	)		# Libraries / binaries that need to be copied to binary directory

# Delibs include directories
include_directories(
	framework/delibs/debase
	framework/delibs/decpp
	framework/delibs/depool
	framework/delibs/dethread
	framework/delibs/deutil
	framework/delibs/destream
	)

# Include target-specific definitions
include(targets/${DEQP_TARGET}/${DEQP_TARGET}.cmake)

# zlib
find_path(ZLIB_INCLUDE_PATH	zlib.h)
find_library(ZLIB_LIBRARY	z)

if (NOT ZLIB_INCLUDE_PATH OR NOT ZLIB_LIBRARY)
	message(STATUS "System version of zlib not found, using external/zlib")
	add_subdirectory(external/zlib)
	# \note ZLIB_LIBRARY and ZLIB_INCLUDE_PATH are promoted from external/zlib/CMakeLists.txt
endif ()

include_directories(${ZLIB_INCLUDE_PATH})

# libpng
#
# The FindPNG module defines PNG_INCLUDE_DIRS and PNG_LIBRARIES. But dEQP's
# CMake files expect the non-standard PNG_INCLUDE_PATH and PNG_LIBRARY. Set the
# non-standard variables here to retain compatibility with dEQP's existing
# CMake files.
include(FindPNG)
set(PNG_INCLUDE_PATH ${PNG_INCLUDE_DIRS})
set(PNG_LIBRARY ${PNG_LIBRARIES})

if (NOT PNG_INCLUDE_PATH OR NOT PNG_LIBRARY)
	message(STATUS "System version of libpng not found, using external/libpng")
	add_subdirectory(external/libpng)
	# \note PNG_LIBRARY and PNG_INCLUDE_PATH are promoted from external/libpng/CMakeLists.txt
endif ()

# glslang
add_subdirectory(external/glslang)

# spirv-tools
add_subdirectory(external/spirv-tools)

include_directories(${PNG_INCLUDE_PATH})

message(STATUS "DEQP_TARGET_NAME        = ${DEQP_TARGET_NAME}")
message(STATUS "DEQP_SUPPORT_GLES1      = ${DEQP_SUPPORT_GLES1}")
message(STATUS "DEQP_GLES1_LIBRARIES    = ${DEQP_GLES1_LIBRARIES}")
message(STATUS "DEQP_GLES2_LIBRARIES    = ${DEQP_GLES2_LIBRARIES}")
message(STATUS "DEQP_GLES3_LIBRARIES    = ${DEQP_GLES3_LIBRARIES}")
message(STATUS "DEQP_GLES31_LIBRARIES   = ${DEQP_GLES31_LIBRARIES}")
message(STATUS "DEQP_GLES32_LIBRARIES   = ${DEQP_GLES32_LIBRARIES}")
message(STATUS "DEQP_SUPPORT_VG         = ${DEQP_SUPPORT_VG}")
message(STATUS "DEQP_VG_LIBRARIES       = ${DEQP_VG_LIBRARIES}")
message(STATUS "DEQP_EGL_LIBRARIES      = ${DEQP_EGL_LIBRARIES}")
message(STATUS "DEQP_PLATFORM_LIBRARIES = ${DEQP_PLATFORM_LIBRARIES}")

# Defines
add_definitions(-DDEQP_TARGET_NAME="${DEQP_TARGET_NAME}")

# Entry points loaded directly by linking to libGL*?
if (DEFINED DEQP_GLES2_LIBRARIES)
	add_definitions(-DDEQP_GLES2_DIRECT_LINK=1)
endif ()

if (DEFINED DEQP_GLES3_LIBRARIES)
	add_definitions(-DDEQP_GLES3_DIRECT_LINK=1)
endif ()

if (DEFINED DEQP_GLES31_LIBRARIES)
	add_definitions(-DDEQP_GLES31_DIRECT_LINK=1)
endif ()

if (DEFINED DEQP_GLES32_LIBRARIES)
	add_definitions(-DDEQP_GLES32_DIRECT_LINK=1)
endif ()

if (DEFINED DEQP_EGL_LIBRARIES)
	add_definitions(-DDEQP_EGL_DIRECT_LINK=1)
endif ()

# Legacy APIs that don't support run-time loading
if (DEQP_SUPPORT_GLES1)
	add_definitions(-DDEQP_SUPPORT_GLES1=1)

	if (NOT DEFINED DEQP_GLES1_LIBRARIES)
		message(FATAL_ERROR "Run-time loading of GLES1 is not supported (DEQP_GLES1_LIBRARIES is not set)")
	endif ()
endif ()

if (DEQP_SUPPORT_VG)
	add_definitions(-DDEQP_SUPPORT_VG=1)

	if (NOT DEFINED DEQP_VG_LIBRARIES)
		message(FATAL_ERROR "Run-time loading of VG is not supported (DEQP_VG_LIBRARIES is not set)")
	endif ()
endif ()

if (DE_COMPILER_IS_MSC)
	# Don't nag about std::copy for example
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SCL_SECURE_NO_WARNINGS")
endif ()

# delibs projects
add_subdirectory(framework/delibs/debase)
add_subdirectory(framework/delibs/depool)
add_subdirectory(framework/delibs/dethread)
add_subdirectory(framework/delibs/destream)
add_subdirectory(framework/delibs/deutil)
add_subdirectory(framework/delibs/decpp)

# ExecServer
add_subdirectory(execserver)

# Executor framework and tools
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/executor)
	add_subdirectory(executor)
endif ()

# Test framework include directories
include_directories(
	framework/common
	framework/qphelper
	framework/opengl
	framework/opengl/wrapper
	framework/referencerenderer
	framework/opengl/simplereference
	framework/randomshaders
	framework/egl
	framework/egl/wrapper
	external/vulkancts/framework/vulkan
	)

if (DE_OS_IS_ANDROID OR DE_OS_IS_IOS)
	# On Android deqp modules are compiled as libraries and linked into final .so
	set(DEQP_MODULE_LIBRARIES )
	set(DEQP_MODULE_ENTRY_POINTS )
endif ()

# Macro for adding targets for copying binaries (usually target libraries) to the target destination dir
macro (target_copy_files target dep_name files)
	if (NOT "${files}" STREQUAL "")
		set(COPY_TARGETS )
		foreach (SRCNAME ${files})
			get_filename_component(BASENAME ${SRCNAME} NAME)
			set(DSTNAME "${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}")
			add_custom_command(OUTPUT ${DSTNAME}
							   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRCNAME} ${DSTNAME})
			set(COPY_TARGETS ${COPY_TARGETS} ${DSTNAME})
		endforeach ()

		add_custom_target(${dep_name} ALL DEPENDS ${COPY_TARGETS})
		add_dependencies(${target} ${dep_name})
	endif ()
endmacro (target_copy_files)

set(MODULE_LIB_TARGET_POSTFIX	"-package")
set(MODULE_DATA_TARGET_POSTFIX	"-data")

# Macro for adding dEQP module
# This adds 3 targets:
#	${MODULE_NAME}-package:		Static library that contains all SRCS and links to LIBS
#	${MODULE_NAME}-data:		Custom target that is used for data file copies
#	${MODULE_NAME}:				Executable binary (if supported by the platform)
macro (add_deqp_module MODULE_NAME SRCS LIBS ENTRY)

	# Library target
	add_library("${MODULE_NAME}${MODULE_LIB_TARGET_POSTFIX}" STATIC ${SRCS})
	target_link_libraries("${MODULE_NAME}${MODULE_LIB_TARGET_POSTFIX}" ${LIBS})

	set(DEQP_MODULE_LIBRARIES		${DEQP_MODULE_LIBRARIES} "${MODULE_NAME}${MODULE_LIB_TARGET_POSTFIX}")
	set(DEQP_MODULE_ENTRY_POINTS	${DEQP_MODULE_ENTRY_POINTS} "${CMAKE_CURRENT_SOURCE_DIR}/${ENTRY}")

	# Forward to parent scope
	set(DEQP_MODULE_LIBRARIES		${DEQP_MODULE_LIBRARIES} PARENT_SCOPE)
	set(DEQP_MODULE_ENTRY_POINTS	${DEQP_MODULE_ENTRY_POINTS} PARENT_SCOPE)

	if (NOT DE_OS_IS_ANDROID AND NOT DE_OS_IS_IOS)
		# Executable target
		add_executable(${MODULE_NAME} ${PROJECT_SOURCE_DIR}/framework/platform/tcuMain.cpp ${ENTRY})
		target_link_libraries(${MODULE_NAME} tcutil-platform "${MODULE_NAME}${MODULE_LIB_TARGET_POSTFIX}")
		target_copy_files(${MODULE_NAME} platform-libs-${MODULE_NAME} "${DEQP_PLATFORM_COPY_LIBRARIES}")
	endif ()

	# Data file target
	add_custom_target("${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}")
	add_dependencies("${MODULE_NAME}${MODULE_LIB_TARGET_POSTFIX}" "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}")
endmacro (add_deqp_module)

# Macro for adding data dirs to module
macro (add_data_dir MODULE_NAME SRC_DIR DST_DIR)
	if (DE_OS_IS_WIN32 OR DE_OS_IS_UNIX OR DE_OS_IS_OSX OR DE_OS_IS_QNX)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${DST_DIR})

	elseif (DE_OS_IS_ANDROID)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR} ${CMAKE_BINARY_DIR}/assets/${DST_DIR})

	elseif (DE_OS_IS_IOS)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_DIR} ${CMAKE_BINARY_DIR}/\${CONFIGURATION}\${EFFECTIVE_PLATFORM_NAME}/deqp.app/${DST_DIR})
	endif ()
endmacro (add_data_dir)

# Macro for adding individual data files to module
macro (add_data_file MODULE_NAME SRC_FILE DST_FILE)
	if (DE_OS_IS_WIN32 OR DE_OS_IS_UNIX OR DE_OS_IS_OSX OR DE_OS_IS_QNX)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${DST_FILE})

	elseif (DE_OS_IS_ANDROID)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE} ${CMAKE_BINARY_DIR}/assets/${DST_FILE})

	elseif (DE_OS_IS_IOS)
		add_custom_command(TARGET "${MODULE_NAME}${MODULE_DATA_TARGET_POSTFIX}" POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE} ${CMAKE_BINARY_DIR}/\${CONFIGURATION}\${EFFECTIVE_PLATFORM_NAME}/deqp.app/${DST_FILE})
	endif ()
endmacro (add_data_file)

add_subdirectory(framework)
add_subdirectory(external/vulkancts/framework/vulkan)

if (DE_COMPILER_IS_MSC)
	add_compile_options(/bigobj) # Required by glsBuiltinPrecisionTests.cpp
endif ()

add_subdirectory(modules)
add_subdirectory(external/vulkancts/modules/vulkan)
add_subdirectory(external/openglcts)

# Single-binary targets
if (DE_OS_IS_ANDROID)
	include_directories(executor)

	set(DEQP_SRCS
		framework/platform/android/tcuAndroidMain.cpp
		framework/platform/android/tcuAndroidJNI.cpp
		framework/platform/android/tcuAndroidPlatformCapabilityQueryJNI.cpp
		framework/platform/android/tcuTestLogParserJNI.cpp
		${DEQP_MODULE_ENTRY_POINTS}
		)

	set(DEQP_LIBS
		tcutil-platform
		xecore
		${DEQP_MODULE_LIBRARIES}
		)

	add_library(deqp SHARED ${DEQP_SRCS})
	target_link_libraries(deqp ${DEQP_LIBS})

	# Needed by OpenGL CTS that defines its own activity but depends on
	# common Android support code.
	target_include_directories(deqp PRIVATE framework/platform/android)

elseif (DE_OS_IS_IOS)
	# Code sign identity
	set(DEQP_IOS_CODE_SIGN_IDENTITY "drawElements" CACHE STRING "Code sign identity for iOS build")

	set(MACOSX_BUNDLE_PRODUCT_NAME "\${PRODUCT_NAME}")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.drawelements.\${PRODUCT_NAME:identifier}")

	include_directories(framework/platform/ios)
	set(TESTERCORE_SRC_FILES
		framework/platform/ios/tcuEAGLView.h
		framework/platform/ios/tcuEAGLView.m
		framework/platform/ios/tcuIOSAppDelegate.h
		framework/platform/ios/tcuIOSAppDelegate.m
		framework/platform/ios/tcuIOSViewController.h
		framework/platform/ios/tcuIOSViewController.m
		framework/platform/ios/tcuIOSMain.m
		)
	set_source_files_properties(${TESTERCORE_SRC_FILES} COMPILE_FLAGS "-std=c99")

	add_executable(deqp MACOSX_BUNDLE ${TESTERCORE_SRC_FILES} ${DEQP_MODULE_ENTRY_POINTS})
	target_link_libraries(deqp tcutil-platform xscore ${DEQP_MODULE_LIBRARIES})
	set_target_properties(deqp PROPERTIES XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")
	set_target_properties(deqp PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer: ${DEQP_IOS_CODE_SIGN_IDENTITY}")
endif ()
