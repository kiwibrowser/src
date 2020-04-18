include_directories(
	..
	../draw
	)

set(DEQP_VK_DYNAMIC_STATE_SRCS
	vktDynamicStateTestCaseUtil.hpp
	vktDynamicStateBaseClass.hpp
	vktDynamicStateBaseClass.cpp
	vktDynamicStateCBTests.cpp
	vktDynamicStateCBTests.hpp
	vktDynamicStateDSTests.cpp
	vktDynamicStateDSTests.hpp
	vktDynamicStateGeneralTests.cpp
	vktDynamicStateGeneralTests.hpp
	vktDynamicStateRSTests.cpp
	vktDynamicStateRSTests.hpp
	vktDynamicStateTests.cpp
	vktDynamicStateTests.hpp
	vktDynamicStateVPTests.cpp
	vktDynamicStateVPTests.hpp
)

set(DEQP_VK_DYNAMIC_STATE_LIBS
	deqp-vk-draw
	tcutil
	vkutil
)

add_library(deqp-vk-dynamic-state STATIC ${DEQP_VK_DYNAMIC_STATE_SRCS})
target_link_libraries(deqp-vk-dynamic-state ${DEQP_VK_DYNAMIC_STATE_LIBS})
