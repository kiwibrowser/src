# cmake file for zlib

if (NOT DE_DEFS)
	message(FATAL_ERROR "Include Defs.cmake")
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/zlib.h")
	set(DEFAULT_ZLIB_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
else ()
	# Assume build inside Android source tree
	set(DEFAULT_ZLIB_SRC_PATH "../zlib/src")
endif ()

set(ZLIB_SRC_PATH ${DEFAULT_ZLIB_SRC_PATH} CACHE STRING "Path to zlib source tree")

if (IS_ABSOLUTE ${ZLIB_SRC_PATH})
	set(ZLIB_ABS_PATH ${ZLIB_SRC_PATH})
else ()
	set(ZLIB_ABS_PATH "${CMAKE_SOURCE_DIR}/${ZLIB_SRC_PATH}")
endif ()

set(ZLIB_SRCS
	${ZLIB_ABS_PATH}/adler32.c
	${ZLIB_ABS_PATH}/compress.c
	${ZLIB_ABS_PATH}/crc32.c
	${ZLIB_ABS_PATH}/crc32.h
	${ZLIB_ABS_PATH}/deflate.c
	${ZLIB_ABS_PATH}/deflate.h
	${ZLIB_ABS_PATH}/gzclose.c
	${ZLIB_ABS_PATH}/gzguts.h
	${ZLIB_ABS_PATH}/gzlib.c
	${ZLIB_ABS_PATH}/gzread.c
	${ZLIB_ABS_PATH}/gzwrite.c
	${ZLIB_ABS_PATH}/infback.c
	${ZLIB_ABS_PATH}/inffast.c
	${ZLIB_ABS_PATH}/inffast.h
	${ZLIB_ABS_PATH}/inffixed.h
	${ZLIB_ABS_PATH}/inflate.c
	${ZLIB_ABS_PATH}/inflate.h
	${ZLIB_ABS_PATH}/inftrees.c
	${ZLIB_ABS_PATH}/inftrees.h
	${ZLIB_ABS_PATH}/trees.c
	${ZLIB_ABS_PATH}/trees.h
	${ZLIB_ABS_PATH}/uncompr.c
	${ZLIB_ABS_PATH}/zconf.h
	${ZLIB_ABS_PATH}/zlib.h
	${ZLIB_ABS_PATH}/zutil.c
	${ZLIB_ABS_PATH}/zutil.h
	)

if (DE_COMPILER_IS_MSC)
	# 4127: conditional expression is constant
	# 4131: 'x': uses old style declarator
	# 4242, 4244: conversion from 'x' to 'y', possible loss of data
	# 4996: deprecated POSIX name
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall /wd4127 /wd4131 /wd4242 /wd4244 /wd4996")

elseif (DE_COMPILER_IS_GCC OR DE_COMPILER_IS_CLANG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")

endif ()

if (DE_OS_IS_UNIX OR DE_OS_IS_QNX)
	add_definitions(-D_XOPEN_SOURCE=600)
endif ()

set(CMAKE_C_FLAGS ${DE_3RD_PARTY_C_FLAGS})

add_library(z STATIC ${ZLIB_SRCS})

set(ZLIB_INCLUDE_PATH	${ZLIB_ABS_PATH}	PARENT_SCOPE)
set(ZLIB_LIBRARY		z					PARENT_SCOPE)
