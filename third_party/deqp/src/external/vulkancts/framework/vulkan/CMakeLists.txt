# vk - Vulkan utilites

set(VKUTIL_SRCS
	vkApiVersion.cpp
	vkApiVersion.hpp
	vkBuilderUtil.cpp
	vkBuilderUtil.hpp
	vkDefs.cpp
	vkDefs.hpp
	vkRef.cpp
	vkRef.hpp
	vkRefUtil.cpp
	vkRefUtil.hpp
	vkPlatform.cpp
	vkPlatform.hpp
	vkPrograms.cpp
	vkPrograms.hpp
	vkStrUtil.cpp
	vkStrUtil.hpp
	vkQueryUtil.cpp
	vkQueryUtil.hpp
	vkMemUtil.cpp
	vkMemUtil.hpp
	vkDeviceUtil.cpp
	vkDeviceUtil.hpp
	vkShaderToSpirV.cpp
	vkShaderToSpirV.hpp
	vkSpirVAsm.hpp
	vkSpirVAsm.cpp
	vkSpirVProgram.hpp
	vkSpirVProgram.cpp
	vkBinaryRegistry.cpp
	vkBinaryRegistry.hpp
	vkNullDriver.cpp
	vkNullDriver.hpp
	vkImageUtil.cpp
	vkImageUtil.hpp
	vkTypeUtil.cpp
	vkTypeUtil.hpp
	vkAllocationCallbackUtil.cpp
	vkAllocationCallbackUtil.hpp
	vkWsiPlatform.cpp
	vkWsiPlatform.hpp
	vkWsiUtil.cpp
	vkWsiUtil.hpp
	vkDebugReportUtil.cpp
	vkDebugReportUtil.hpp
	vkBufferWithMemory.cpp
	vkBufferWithMemory.hpp
	vkImageWithMemory.cpp
	vkImageWithMemory.hpp
	vkBufferWithMemory.cpp
	vkBufferWithMemory.hpp
	vkImageWithMemory.cpp
	vkImageWithMemory.hpp
	vkShaderProgram.cpp
	vkShaderProgram.hpp
	)

set(VKUTIL_LIBS
	glutil
	tcutil
	)

if (DEQP_HAVE_GLSLANG)
	include_directories(${GLSLANG_INCLUDE_PATH})
	add_definitions(-DDEQP_HAVE_GLSLANG=1)

	# \note Code interfacing with glslang needs to include third-party headers
	#       that cause all sorts of warnings to appear.
	if (DE_COMPILER_IS_GCC OR DE_COMPILER_IS_CLANG)
		set_source_files_properties(
			FILES vkShaderToSpirV.cpp
			PROPERTIES COMPILE_FLAGS "${DE_3RD_PARTY_CXX_FLAGS} -std=c++11")
	endif ()

	set(VKUTIL_LIBS ${VKUTIL_LIBS} ${GLSLANG_LIBRARY})
endif ()

if(DEQP_HAVE_SPIRV_TOOLS)
	include_directories(${spirv-tools_SOURCE_DIR}/include)
	include_directories(${spirv-tools_SOURCE_DIR}/external/include)

	add_definitions(-DDEQP_HAVE_SPIRV_TOOLS=1)
	set(VKUTIL_LIBS ${VKUTIL_LIBS} SPIRV-Tools)
endif()

add_library(vkutil STATIC ${VKUTIL_SRCS})
target_link_libraries(vkutil ${VKUTIL_LIBS})
