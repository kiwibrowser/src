# WSI tests

include_directories(..)

set(DEQP_VK_WSI_SRCS
	vktWsiTests.cpp
	vktWsiTests.hpp
	vktWsiSurfaceTests.cpp
	vktWsiSurfaceTests.hpp
	vktWsiSwapchainTests.cpp
	vktWsiSwapchainTests.hpp
	vktWsiIncrementalPresentTests.cpp
	vktWsiIncrementalPresentTests.hpp
	vktWsiDisplayTimingTests.cpp
	vktWsiDisplayTimingTests.hpp
	vktWsiSharedPresentableImageTests.cpp
	vktWsiSharedPresentableImageTests.hpp
	vktWsiDisplayTests.hpp
	vktWsiDisplayTests.cpp
	vktWsiColorSpaceTests.cpp
	vktWsiColorSpaceTests.hpp
	)

set(DEQP_VK_WSI_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-wsi STATIC ${DEQP_VK_WSI_SRCS})
target_link_libraries(deqp-vk-wsi ${DEQP_VK_WSI_LIBS})
