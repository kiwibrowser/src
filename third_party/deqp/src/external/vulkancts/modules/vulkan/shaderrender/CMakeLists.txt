include_directories(
	..
	)

set(DEQP_VK_SHADERRENDER_SRCS
	vktShaderRender.cpp
	vktShaderRender.hpp
	vktShaderRenderBuiltinVarTests.cpp
	vktShaderRenderBuiltinVarTests.hpp
	vktShaderRenderDerivateTests.cpp
	vktShaderRenderDerivateTests.hpp
	vktShaderRenderDiscardTests.cpp
	vktShaderRenderDiscardTests.hpp
	vktShaderRenderIndexingTests.cpp
	vktShaderRenderIndexingTests.hpp
	vktShaderRenderLoopTests.cpp
	vktShaderRenderLoopTests.hpp
	vktShaderRenderMatrixTests.cpp
	vktShaderRenderMatrixTests.hpp
	vktShaderRenderOperatorTests.cpp
	vktShaderRenderOperatorTests.hpp
	vktShaderRenderReturnTests.cpp
	vktShaderRenderReturnTests.hpp
	vktShaderRenderStructTests.cpp
	vktShaderRenderStructTests.hpp
	vktShaderRenderSwitchTests.cpp
	vktShaderRenderSwitchTests.hpp
	vktShaderRenderTextureFunctionTests.cpp
	vktShaderRenderTextureFunctionTests.hpp
	vktShaderRenderTextureGatherTests.cpp
	vktShaderRenderTextureGatherTests.hpp
	)

set(DEQP_VK_SHADERRENDER_LIBS
	tcutil
	vkutil
	)

add_library(deqp-vk-shaderrender STATIC ${DEQP_VK_SHADERRENDER_SRCS})
target_link_libraries(deqp-vk-shaderrender ${DEQP_VK_SHADERRENDER_LIBS})
