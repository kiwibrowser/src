# dEQP-VK.texture

include_directories(
	..
	../shaderexecutor
	)

set(DEQP_VK_TEXTURE_SRCS
	vktSampleVerifier.cpp
	vktSampleVerifier.hpp
	vktSampleVerifierUtil.cpp
	vktSampleVerifierUtil.hpp
	vktTextureTests.cpp
	vktTextureTests.hpp
	vktTextureTestUtil.cpp
	vktTextureTestUtil.hpp
	vktTextureFilteringExplicitLodTests.cpp
	vktTextureFilteringExplicitLodTests.hpp
	vktTextureFilteringTests.cpp
	vktTextureFilteringTests.hpp
	vktTextureMipmapTests.cpp
	vktTextureMipmapTests.hpp
	vktTextureShadowTests.cpp
	vktTextureShadowTests.hpp
	vktTextureFilteringAnisotropyTests.cpp
	vktTextureFilteringAnisotropyTests.hpp
	vktTextureCompressedFormatTests.cpp
	vktTextureCompressedFormatTests.hpp
	)

set(DEQP_VK_TEXTURE_LIBS
	deqp-vk-shaderexecutor
	tcutil
	vkutil
	)

add_library(deqp-vk-texture STATIC ${DEQP_VK_TEXTURE_SRCS})
target_link_libraries(deqp-vk-texture ${DEQP_VK_TEXTURE_LIBS})
