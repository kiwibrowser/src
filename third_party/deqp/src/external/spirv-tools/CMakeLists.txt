# cmake file for spirv-tools

if (NOT DE_DEFS)
	message(FATAL_ERROR "Include Defs.cmake")
endif ()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/spirv-tools/libspirv.h")
	set(DEFAULT_SPIRV_TOOLS_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src)
else ()
	set(DEFAULT_SPIRV_TOOLS_SRC_PATH "../spirv-tools")
endif ()

set(SPIRV_TOOLS_SRC_PATH ${DEFAULT_SPIRV_TOOLS_SRC_PATH} CACHE STRING "Path to spirv-tools source tree")
if (IS_ABSOLUTE ${SPIRV_TOOLS_SRC_PATH})
	set(SPIRV_TOOLS_ABS_PATH ${SPIRV_TOOLS_SRC_PATH})
else ()
	set(SPIRV_TOOLS_ABS_PATH "${CMAKE_SOURCE_DIR}/${SPIRV_TOOLS_SRC_PATH}")
endif ()

set(SPIRV_HEADERS_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../spirv-headers/src CACHE STRING "Path to spirv-headers source tree")
if (IS_ABSOLUTE ${SPIRV_HEADERS_SRC_PATH})
	set(SPIRV_HEADERS_ABS_PATH ${SPIRV_HEADERS_SRC_PATH})
else ()
	set(SPIRV_HEADERS_ABS_PATH "${CMAKE_SOURCE_DIR}/${SPIRV_HEADERS_SRC_PATH}")
endif ()

if (EXISTS ${SPIRV_TOOLS_ABS_PATH}/source/opcode.cpp AND
	EXISTS ${SPIRV_HEADERS_ABS_PATH}/include/spirv/1.0/spirv.h)
	message(STATUS "spirv-tools found; building with DEQP_HAVE_SPIRV_TOOLS")
	set(CMAKE_C_FLAGS ${DE_3RD_PARTY_C_FLAGS})
	set(CMAKE_CXX_FLAGS ${DE_3RD_PARTY_CXX_FLAGS})

	# SPIR-V tools requires SPIRV-Headers
	set(SPIRV-Headers_SOURCE_DIR ${SPIRV_HEADERS_ABS_PATH})

	set(DEQP_HAVE_SPIRV_TOOLS		ON					PARENT_SCOPE)
	set(SPIRV_SKIP_EXECUTABLES		ON CACHE BOOL "" FORCE)
	add_subdirectory(${SPIRV_TOOLS_ABS_PATH} spirv-tools)
else ()
	message(STATUS "spirv-tools not found; SPIR-V assembly not available")
	set(DEQP_HAVE_SPIRV_TOOLS		OFF					PARENT_SCOPE)
endif ()
