#GL CTS Makefile

set(KC_CTS_HOME  "${PROJECT_SOURCE_DIR}/external/kc-cts"        CACHE STRING "Home directory of Khronos Confidential CTS")
set(GTF_HOME     "${KC_CTS_HOME}/src/GTF_ES"            CACHE STRING "Home directory of GTF test framework")

if (DE_COMPILER_IS_GCC OR DE_COMPILER_IS_CLANG)
	# -Wconversion generates a LOT of warnings in this code base
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

	# Same for -Wunused-function
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
endif ()

if (EXISTS "${GTF_HOME}/CMakeLists.txt")
        set(DEQP_GTF_AVAILABLE  ON)                             # Is GTF test framework available
else ()
        set(DEQP_GTF_AVAILABLE  OFF)
endif ()

message(STATUS "DEQP_GTF_AVAILABLE      = ${DEQP_GTF_AVAILABLE}")

if (DEQP_GTF_AVAILABLE)
	set(GLCTS_GTF_SUPPORT_GLES32 OFF)
	set(GLCTS_GTF_SUPPORT_GLES31 OFF)
	set(GLCTS_GTF_SUPPORT_GLES3  OFF)
	set(GLCTS_GTF_SUPPORT_GLES2  OFF)
	set(GLCTS_GTF_SUPPORT_OPENGL OFF)

	add_definitions(-DDEQP_GTF_AVAILABLE=1)

	if (GLCTS_GTF_TARGET STREQUAL "gl")
		set(GLCTS_GTF_SUPPORT_OPENGL ON)
	elseif (GLCTS_GTF_TARGET STREQUAL "gles32")
		set(GLCTS_GTF_SUPPORT_GLES32 ON)
	elseif (GLCTS_GTF_TARGET STREQUAL "gles31")
		set(GLCTS_GTF_SUPPORT_GLES31 ON)
	elseif (GLCTS_GTF_TARGET STREQUAL "gles3")
		set(GLCTS_GTF_SUPPORT_GLES3 ON)
	elseif (GLCTS_GTF_TARGET STREQUAL "gles2")
		set(GLCTS_GTF_SUPPORT_GLES2 ON)
	else ()
		set(GLCTS_GTF_SUPPORT_GLES32 ON)
		message(WARNING "GLCTS_GTF_TARGET invalid or not defined, allowed options are [gl, gles32, gles31, gles3, gles2]")
		message(WARNING "GLCTS_GTF_TARGET is set to gles32")
	endif ()
endif()

message(STATUS "GLCTS_GTF_SUPPORT_OPENGL = ${GLCTS_GTF_SUPPORT_OPENGL}")
message(STATUS "GLCTS_GTF_SUPPORT_GLES32 = ${GLCTS_GTF_SUPPORT_GLES32}")
message(STATUS "GLCTS_GTF_SUPPORT_GLES31 = ${GLCTS_GTF_SUPPORT_GLES31}")
message(STATUS "GLCTS_GTF_SUPPORT_GLES3  = ${GLCTS_GTF_SUPPORT_GLES3}")
message(STATUS "GLCTS_GTF_SUPPORT_GLES2  = ${GLCTS_GTF_SUPPORT_GLES2}")

set(GLCTS_GTF_SUPPORT_EGL ON)

include_directories(
	include
	framework/egl/wrapper
	)

add_subdirectory(modules)

if (DEQP_GTF_AVAILABLE)
	add_subdirectory(${GTF_HOME} GTF)
endif ()

# Pass DEQP_MODULE_LIBRARIES and DEQP_MODULE_ENTRY_POINTS
set(DEQP_MODULE_LIBRARIES ${DEQP_MODULE_LIBRARIES} PARENT_SCOPE)
set(DEQP_MODULE_ENTRY_POINTS ${DEQP_MODULE_ENTRY_POINTS} PARENT_SCOPE)
