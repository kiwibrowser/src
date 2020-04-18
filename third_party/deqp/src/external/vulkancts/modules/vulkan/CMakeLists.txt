# dEQP-VK

add_subdirectory(api)
add_subdirectory(pipeline)
add_subdirectory(binding_model)
add_subdirectory(spirv_assembly)
add_subdirectory(shaderrender)
add_subdirectory(shaderexecutor)
add_subdirectory(memory)
add_subdirectory(ubo)
add_subdirectory(dynamic_state)
add_subdirectory(ssbo)
add_subdirectory(query_pool)
add_subdirectory(draw)
add_subdirectory(compute)
add_subdirectory(image)
add_subdirectory(wsi)
add_subdirectory(sparse_resources)
add_subdirectory(tessellation)
add_subdirectory(rasterization)
add_subdirectory(synchronization)
add_subdirectory(clipping)
add_subdirectory(fragment_ops)
add_subdirectory(geometry)
add_subdirectory(texture)
add_subdirectory(robustness)
add_subdirectory(renderpass)
add_subdirectory(ycbcr)

include_directories(
	api
	pipeline
	binding_model
	spirv_assembly
	shaderrender
	shaderexecutor
	memory
	ubo
	dynamic_state
	ssbo
	query_pool
	draw
	compute
	image
	wsi
	sparse_resources
	tessellation
	rasterization
	synchronization
	clipping
	fragment_ops
	texture
	geometry
	robustness
	renderpass
	ycbcr
	)

set(DEQP_VK_SRCS
	vktTestCase.cpp
	vktTestCase.hpp
	vktTestCaseUtil.cpp
	vktTestCaseUtil.hpp
	vktTestPackage.cpp
	vktTestPackage.hpp
	vktShaderLibrary.cpp
	vktShaderLibrary.hpp
	vktTestGroupUtil.cpp
	vktTestGroupUtil.hpp
	vktInfoTests.cpp
	vktInfoTests.hpp
	vktDrawUtil.cpp
	vktDrawUtil.hpp
	vktExternalMemoryUtil.cpp
	vktExternalMemoryUtil.hpp
	vktDrawUtil.cpp
	vktDrawUtil.hpp
	)

set(DEQP_VK_LIBS
	tcutil
	vkutil
	glutil
	deqp-vk-api
	deqp-vk-pipeline
	deqp-vk-binding-model
	deqp-vk-spirv-assembly
	deqp-vk-shaderrender
	deqp-vk-shaderexecutor
	deqp-vk-memory
	deqp-vk-ubo
	deqp-vk-dynamic-state
	deqp-vk-ssbo
	deqp-vk-query-pool
	deqp-vk-draw
	deqp-vk-compute
	deqp-vk-image
	deqp-vk-wsi
	deqp-vk-sparse-resources
	deqp-vk-tessellation
	deqp-vk-rasterization
	deqp-vk-synchronization
	deqp-vk-clipping
	deqp-vk-fragment-ops
	deqp-vk-texture
	deqp-vk-geometry
	deqp-vk-robustness
	deqp-vk-render-pass
	deqp-vk-ycbcr
	)

if (DE_COMPILER_IS_MSC AND (DE_PTR_SIZE EQUAL 4))
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
endif ()

add_deqp_module(deqp-vk "${DEQP_VK_SRCS}" "${DEQP_VK_LIBS}" vktTestPackageEntry.cpp)

add_data_dir(deqp-vk ../../data/vulkan	vulkan)

if (DE_OS_IS_WIN32 OR DE_OS_IS_UNIX OR DE_OS_IS_OSX)
	add_executable(vk-build-programs vktBuildPrograms.cpp)
	target_link_libraries(vk-build-programs deqp-vk${MODULE_LIB_TARGET_POSTFIX})
endif ()
