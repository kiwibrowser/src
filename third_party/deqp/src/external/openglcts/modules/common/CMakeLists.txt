# Common OpenGL CTS Test Code

if (DEQP_SUPPORT_WGL)
	include_directories(${PROJECT_SOURCE_DIR}/framework/platform/win32)
	add_definitions(-DGLCTS_SUPPORT_WGL=1)
endif ()

include_directories(
	${PROJECT_SOURCE_DIR}/modules/glshared
	)

set(GLCTS_COMMON_SRCS
	glcAggressiveShaderOptimizationsTests.cpp
	glcAggressiveShaderOptimizationsTests.hpp
	glcBlendEquationAdvancedTests.cpp
	glcBlendEquationAdvancedTests.hpp
	glcConfigList.hpp
	glcConfigList.cpp
	glcConfigListCase.cpp
	glcConfigListCase.hpp
	glcConfigListEGL.hpp
	glcConfigListEGL.cpp
	glcConfigListWGL.hpp
	glcConfigListWGL.cpp
	glcConfigPackage.cpp
	glcConfigPackage.hpp
	glcContext.cpp
	glcContext.hpp
	glcContextFlagsTests.cpp
	glcContextFlagsTests.hpp
	glcExposedExtensionsTests.cpp
	glcExposedExtensionsTests.hpp
	glcFragDepthTests.cpp
	glcFragDepthTests.hpp
	glcInfoTests.cpp
	glcInfoTests.hpp
	glcInternalformatTests.cpp
	glcInternalformatTests.hpp
	glcMisc.cpp
	glcMisc.hpp
	glcMultipleContextsTests.cpp
	glcMultipleContextsTests.hpp
	glcNoErrorTests.cpp
	glcNoErrorTests.hpp
	glcRobustnessTests.cpp
	glcRobustnessTests.hpp
	glcPackedPixelsTests.cpp
	glcPackedPixelsTests.hpp
	glcPolygonOffsetClampTests.cpp
	glcPolygonOffsetClampTests.hpp
	glcPackedDepthStencilTests.cpp
	glcPackedDepthStencilTests.hpp
	glcParallelShaderCompileTests.cpp
	glcParallelShaderCompileTests.hpp
	glcRobustBufferAccessBehaviorTests.cpp
	glcRobustBufferAccessBehaviorTests.hpp
	glcSeparableProgramsTransformFeedbackTests.cpp
	glcSeparableProgramsTransformFeedbackTests.hpp
	glcShaderConstExprTests.hpp
	glcShaderConstExprTests.cpp
	glcShaderGroupVoteTests.cpp
	glcShaderGroupVoteTests.hpp
	glcShaderIndexingTests.cpp
	glcShaderIndexingTests.hpp
	glcShaderIntegerMixTests.cpp
	glcShaderIntegerMixTests.hpp
	glcShaderLibrary.cpp
	glcShaderLibrary.hpp
	glcShaderLibraryCase.cpp
	glcShaderLibraryCase.hpp
	glcShaderLoopTests.cpp
	glcShaderLoopTests.hpp
	glcShaderNegativeTests.hpp
	glcShaderNegativeTests.cpp
	glcShaderMacroTests.cpp
	glcShaderMacroTests.hpp
	glcShaderMultisampleInterpolationTests.cpp
	glcShaderMultisampleInterpolationTests.hpp
	glcShaderRenderCase.cpp
	glcShaderRenderCase.hpp
	glcShaderStructTests.cpp
	glcShaderStructTests.hpp
	glcShaderSwitchTests.cpp
	glcShaderSwitchTests.hpp
	glcSampleVariablesTests.cpp
	glcSampleVariablesTests.hpp
	glcTestCase.cpp
	glcTestCase.hpp
	glcTestCaseWrapper.cpp
	glcTestCaseWrapper.hpp
	glcTestPackage.cpp
	glcTestPackage.hpp
	glcTextureFilterAnisotropicTests.cpp
	glcTextureFilterAnisotropicTests.hpp
	glcTextureRepeatModeTests.cpp
	glcTextureRepeatModeTests.hpp
	glcUniformBlockCase.cpp
	glcUniformBlockCase.hpp
	glcUniformBlockTests.cpp
	glcUniformBlockTests.hpp
	glcViewportArrayTests.cpp
	glcViewportArrayTests.hpp
	glcTestSubcase.cpp
	glcTestSubcase.hpp
	glcExtTokens.cpp
	glcExtTokens.hpp
	glcLimitTest.inl
	glcLimitTest.hpp
	glcWaiver.hpp
	)

set(GLCTS_COMMON_LIBS
	glutil
	tcutil
	eglutil
	)

add_library(glcts-common STATIC ${GLCTS_COMMON_SRCS})
target_link_libraries(glcts-common ${GLCTS_COMMON_LIBS})

set(GLCTS_COMMON_NOCONTEXTPACKAGE_SRCS
	glcNoDefaultContextPackage.cpp
	glcNoDefaultContextPackage.hpp
	)
add_library(glcts-common-nocontext-package STATIC ${GLCTS_COMMON_NOCONTEXTPACKAGE_SRCS})
