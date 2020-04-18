
include_directories(
	..
	)

set(DEQP_VK_ROBUSTNESS_SRCS
	vktRobustnessTests.cpp
	vktRobustnessTests.hpp
	vktRobustnessBufferAccessTests.cpp
	vktRobustnessBufferAccessTests.hpp
	vktRobustnessUtil.cpp
	vktRobustnessUtil.hpp
	vktRobustnessVertexAccessTests.cpp
	vktRobustnessVertexAccessTests.hpp
	)

set(DEQP_VK_ROBUSTNESS_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-robustness STATIC ${DEQP_VK_ROBUSTNESS_SRCS})
target_link_libraries(deqp-vk-robustness ${DEQP_VK_ROBUSTNESS_LIBS})

