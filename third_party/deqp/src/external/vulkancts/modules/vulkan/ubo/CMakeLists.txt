include_directories(
	..
)

set(DEQP_VK_UBO_SRCS
	vktUniformBlockCase.cpp
	vktUniformBlockCase.hpp
	vktRandomUniformBlockCase.cpp
	vktRandomUniformBlockCase.hpp
	vktUniformBlockTests.cpp
	vktUniformBlockTests.hpp
)

set(DEQP_VK_UBO_LIBS
	tcutil
	vkutil
)

add_library(deqp-vk-ubo STATIC ${DEQP_VK_UBO_SRCS})
target_link_libraries(deqp-vk-ubo ${DEQP_VK_UBO_LIBS})
