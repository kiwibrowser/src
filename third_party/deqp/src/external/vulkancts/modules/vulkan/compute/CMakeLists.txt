include_directories(..)

set(DEQP_VK_COMPUTE_SRCS
	vktComputeTests.cpp
	vktComputeTests.hpp
	vktComputeBasicComputeShaderTests.cpp
	vktComputeBasicComputeShaderTests.hpp
	vktComputeIndirectComputeDispatchTests.cpp
	vktComputeIndirectComputeDispatchTests.hpp
	vktComputeShaderBuiltinVarTests.cpp
	vktComputeShaderBuiltinVarTests.hpp
	vktComputeTestsUtil.cpp
	vktComputeTestsUtil.hpp
	)

set(DEQP_VK_COMPUTE_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-compute STATIC ${DEQP_VK_COMPUTE_SRCS})
target_link_libraries(deqp-vk-compute ${DEQP_VK_COMPUTE_LIBS})
