include_directories(..)

set(DEQP_VK_IMAGE_SRCS
	vktImageTests.cpp
	vktImageTests.hpp
	vktImageTestsUtil.cpp
	vktImageTestsUtil.hpp
	vktImageAtomicOperationTests.cpp
	vktImageAtomicOperationTests.hpp
	vktImageLoadStoreTests.cpp
	vktImageLoadStoreTests.hpp
	vktImageMutableTests.cpp
	vktImageMutableTests.hpp
	vktImageQualifiersTests.cpp
	vktImageQualifiersTests.hpp
	vktImageSizeTests.cpp
	vktImageSizeTests.hpp
	vktImageTexture.cpp
	vktImageTexture.hpp
	vktImageMultisampleLoadStoreTests.cpp
	vktImageMultisampleLoadStoreTests.hpp
	vktImageCompressionTranscodingSupport.cpp
	vktImageCompressionTranscodingSupport.hpp
	vktImageLoadStoreUtil.cpp
	vktImageLoadStoreUtil.hpp
	vktImageTranscodingSupportTests.cpp
	vktImageTranscodingSupportTests.hpp
	)

set(DEQP_VK_IMAGE_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-image STATIC ${DEQP_VK_IMAGE_SRCS})
target_link_libraries(deqp-vk-image ${DEQP_VK_IMAGE_LIBS})
