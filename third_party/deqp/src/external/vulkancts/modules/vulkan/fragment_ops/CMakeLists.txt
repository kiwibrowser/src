include_directories(..)

set(DEQP_VK_FRAGMENT_OPS_SRCS
	vktFragmentOperationsEarlyFragmentTests.cpp
	vktFragmentOperationsEarlyFragmentTests.hpp
	vktFragmentOperationsMakeUtil.cpp
	vktFragmentOperationsMakeUtil.hpp
	vktFragmentOperationsScissorMultiViewportTests.cpp
	vktFragmentOperationsScissorMultiViewportTests.hpp
	vktFragmentOperationsScissorTests.cpp
	vktFragmentOperationsScissorTests.hpp
	vktFragmentOperationsTests.cpp
	vktFragmentOperationsTests.hpp
	)

set(DEQP_VK_FRAGMENT_OPS_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-fragment-ops STATIC ${DEQP_VK_FRAGMENT_OPS_SRCS})
target_link_libraries(deqp-vk-fragment-ops ${DEQP_VK_FRAGMENT_OPS_LIBS})
