# Render pass tests

include_directories(..)

set(DEQP_VK_RENDER_PASS_SRCS
	vktRenderPassTests.cpp
	vktRenderPassTests.hpp
	vktRenderPassMultisampleTests.cpp
	vktRenderPassMultisampleTests.hpp
	vktRenderPassMultisampleResolveTests.cpp
	vktRenderPassMultisampleResolveTests.hpp
	vktRenderPassSampleReadTests.cpp
	vktRenderPassSampleReadTests.hpp
	vktRenderPassSparseRenderTargetTests.cpp
	vktRenderPassSparseRenderTargetTests.hpp
	)

set(DEQP_VK_RENDER_PASS_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-render-pass STATIC ${DEQP_VK_RENDER_PASS_SRCS})
target_link_libraries(deqp-vk-render-pass ${DEQP_VK_RENDER_PASS_LIBS})
