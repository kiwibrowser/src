# cmake file for libpng

if (NOT DE_DEFS)
	message(FATAL_ERROR "Include Defs.cmake")
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/png.h")
	set(DEFAULT_PNG_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
else ()
	# Assume build inside Android source tree
	set(DEFAULT_PNG_SRC_PATH "../libpng")
endif ()

set(PNG_SRC_PATH ${DEFAULT_PNG_SRC_PATH} CACHE STRING "Path to libpng source tree")

if (IS_ABSOLUTE ${PNG_SRC_PATH})
	set(PNG_ABS_PATH ${PNG_SRC_PATH})
else ()
	set(PNG_ABS_PATH "${CMAKE_SOURCE_DIR}/${PNG_SRC_PATH}")
endif ()

set(PNG_SRCS
	${PNG_ABS_PATH}/png.h
	${PNG_ABS_PATH}/pngconf.h
	${PNG_ABS_PATH}/png.c
	${PNG_ABS_PATH}/pngset.c
	${PNG_ABS_PATH}/pngget.c
	${PNG_ABS_PATH}/pngrutil.c
	${PNG_ABS_PATH}/pngtrans.c
	${PNG_ABS_PATH}/pngwutil.c
	${PNG_ABS_PATH}/pngread.c
	${PNG_ABS_PATH}/pngrio.c
	${PNG_ABS_PATH}/pngwio.c
	${PNG_ABS_PATH}/pngwrite.c
	${PNG_ABS_PATH}/pngrtran.c
	${PNG_ABS_PATH}/pngwtran.c
	${PNG_ABS_PATH}/pngmem.c
	${PNG_ABS_PATH}/pngerror.c
	${PNG_ABS_PATH}/pngpread.c
	)

if (DE_CPU_IS_ARM_64)
	# armv8-a has always neon
	set(PNG_SRCS
		${PNG_SRCS}
		${PNG_ABS_PATH}/arm/arm_init.c
		${PNG_ABS_PATH}/arm/filter_neon_intrinsics.c
		${PNG_ABS_PATH}/arm/filter_neon.S
		)
endif ()

if (DE_DEBUG EQUAL 1)
	add_definitions(-DPNG_DEBUG)
endif ()

if (DE_OS_IS_UNIX OR DE_OS_IS_QNX)
	# for snprintf()
	add_definitions(-D_XOPEN_SOURCE=600)
endif ()

set(CMAKE_C_FLAGS ${DE_3RD_PARTY_C_FLAGS})

add_library(png STATIC ${PNG_SRCS})
target_link_libraries(png ${ZLIB_LIBRARY})

set(PNG_INCLUDE_PATH	${PNG_ABS_PATH}	PARENT_SCOPE)
set(PNG_LIBRARY			png				PARENT_SCOPE)
