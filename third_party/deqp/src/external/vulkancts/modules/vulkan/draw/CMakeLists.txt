include_directories(..)

set(DEQP_VK_DRAW_SRCS
	vktDrawTests.hpp
	vktDrawTests.cpp
	vktDrawIndexedTest.hpp
	vktDrawIndexedTest.cpp
	vktDrawIndirectTest.hpp
	vktDrawIndirectTest.cpp
	vktDrawInstancedTests.hpp
	vktDrawInstancedTests.cpp
	vktDrawInvertedDepthRangesTests.hpp
	vktDrawInvertedDepthRangesTests.cpp
	vktDrawShaderDrawParametersTests.hpp
	vktDrawShaderDrawParametersTests.cpp
	vktDrawSimpleTest.hpp
	vktDrawSimpleTest.cpp
	vktDrawNegativeViewportHeightTests.hpp
	vktDrawNegativeViewportHeightTests.cpp
	vktDrawBaseClass.hpp
	vktDrawBaseClass.cpp
	vktDrawCreateInfoUtil.hpp
	vktDrawCreateInfoUtil.cpp
	vktDrawImageObjectUtil.hpp
	vktDrawImageObjectUtil.cpp
	vktDrawBufferObjectUtil.hpp
	vktDrawBufferObjectUtil.cpp
	vktDrawTestCaseUtil.hpp
	vktBasicDrawTests.hpp
	vktBasicDrawTests.cpp
)

set(DEQP_VK_DRAW_LIBS
	tcutil
	vkutil
)

add_library(deqp-vk-draw STATIC ${DEQP_VK_DRAW_SRCS})
target_link_libraries(deqp-vk-draw ${DEQP_VK_DRAW_LIBS})
