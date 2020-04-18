/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  es31cTextureStorageMultisampleTests.cpp
 * \brief Implements test group consisting of all the tests verifying
 *        multisample texture functionality. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"
#include "es31cTextureStorageMultisampleDependenciesTests.hpp"
#include "es31cTextureStorageMultisampleFunctionalTests.hpp"
#include "es31cTextureStorageMultisampleGLCoverageTests.hpp"
#include "es31cTextureStorageMultisampleGetActiveUniformTests.hpp"
#include "es31cTextureStorageMultisampleGetMultisamplefvTests.hpp"
#include "es31cTextureStorageMultisampleGetTexLevelParameterifvTests.hpp"
#include "es31cTextureStorageMultisampleSampleMaskiTests.hpp"
#include "es31cTextureStorageMultisampleTexStorage2DMultisampleTests.hpp"
#include "es31cTextureStorageMultisampleTexStorage3DMultisampleTests.hpp"

namespace glcts
{
/** Constructor.
 *
 *  @param context Rendering context.
 **/
TextureStorageMultisampleTests::TextureStorageMultisampleTests(Context& context)
	: TestCaseGroup(context, "texture_storage_multisample", "Multisample texture storage conformance test group")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void TextureStorageMultisampleTests::init(void)
{
	/* Creating Tests Groups */
	tcu::TestCaseGroup* apiGLGetActiveUniformTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLGetActiveUniform", "glGetActiveUniform()");
	tcu::TestCaseGroup* apiGLTexStorage2DMultisampleTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLTexStorage2DMultisample", "glTexStorage2DMultisample()");
	tcu::TestCaseGroup* apiGLTexStorage3DMultisampleTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLTexStorage3DMultisample", "glTexStorage3DMultisampleOES()");
	tcu::TestCaseGroup* apiGLGetMultisamplefvTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLGetMultisamplefv", "glGetMultisamplefv()");
	tcu::TestCaseGroup* apiGLGetTexLevelParameterifvTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLGetTexLevelParameterifv", "glGetTexLevelParameterifv()");
	tcu::TestCaseGroup* apiGLSampleMaskiTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIGLSampleMaski", "glSampleMaski()");
	tcu::TestCaseGroup* apiDependenciesTestGroup =
		new tcu::TestCaseGroup(m_testCtx, "APIDependencies", "API Dependncies");
	tcu::TestCaseGroup* apiGLCoverageTestGroup = new tcu::TestCaseGroup(m_testCtx, "GLCoverage", "GL Coverage");
	tcu::TestCaseGroup* functionalTestGroup = new tcu::TestCaseGroup(m_testCtx, "FunctionalTests", "Functional Tests");

	/* Adding tests groups as children of this test suite */
	addChild(apiGLGetActiveUniformTestGroup);
	addChild(apiGLTexStorage2DMultisampleTestGroup);
	addChild(apiGLTexStorage3DMultisampleTestGroup);
	addChild(apiGLGetMultisamplefvTestGroup);
	addChild(apiGLGetTexLevelParameterifvTestGroup);
	addChild(apiGLSampleMaskiTestGroup);
	addChild(apiDependenciesTestGroup);
	addChild(apiGLCoverageTestGroup);
	addChild(functionalTestGroup);

	/* Assign tests to parent group: APIGLGetActiveUniform */
	apiGLGetActiveUniformTestGroup->addChild(new glcts::MultisampleTextureGetActiveUniformSamplersTest(m_context));

	/* Assign tests to parent group: APIGLTexStorage2DMultisample */
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DGeneralSamplesNumberTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DReconfigurationRejectedTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(
		new glcts::MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(new glcts::MultisampleTextureTexStorage2DValidCallsTest(m_context));
	apiGLTexStorage2DMultisampleTestGroup->addChild(new glcts::MultisampleTextureTexStorage2DZeroSampleTest(m_context));

	/* Assign tests to parent group: APIGLTexStorage3DMultisample */
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::InvalidTextureSizesAreRejectedValidAreAcceptedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(new glcts::MultisampleTextureTexStorage3DZeroSampleTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::NonColorDepthStencilRenderableInternalformatsAreRejectedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(new glcts::Texture2DMultisampleTargetIsRejectedTest(m_context));
	apiGLTexStorage3DMultisampleTestGroup->addChild(
		new glcts::ValidInternalformatAndSamplesValuesAreAcceptedTest(m_context));

	/* Assign tests to parent group: APIGLGetMultisamplefv */
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest(m_context));

	/* Assign tests to parent group: APIGLGetTexLevelParameterifv */
	apiGLGetTexLevelParameterifvTestGroup->addChild(
		new glcts::MultisampleTextureGetTexLevelParametervFunctionalTest(m_context));
	apiGLGetTexLevelParameterifvTestGroup->addChild(
		new glcts::MultisampleTextureGetTexLevelParametervInvalidTextureTargetRejectedTest(m_context));
	apiGLGetTexLevelParameterifvTestGroup->addChild(
		new glcts::MultisampleTextureGetTexLevelParametervInvalidValueArgumentRejectedTest(m_context));
	apiGLGetTexLevelParameterifvTestGroup->addChild(
		new glcts::MultisampleTextureGetTexLevelParametervNegativeLodIsRejectedTest(m_context));
	apiGLGetTexLevelParameterifvTestGroup->addChild(
		new glcts::MultisampleTextureGetTexLevelParametervWorksForMaximumLodTest(m_context));

	/* Assign tests to parent group: APIGLSampleMaski */
	apiGLGetMultisamplefvTestGroup->addChild(new glcts::MultisampleTextureSampleMaskiGettersTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest(m_context));
	apiGLGetMultisamplefvTestGroup->addChild(
		new glcts::MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest(m_context));

	/* Assign tests to parent group: APIDependencies */
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesFBOIncompleteness1Test(m_context));
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesFBOIncompleteness2Test(m_context));
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesFBOIncompleteness3Test(m_context));
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesFBOIncompleteness4Test(m_context));
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesFBOIncompleteness5Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest(m_context));
	apiDependenciesTestGroup->addChild(
		new glcts::MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest(
			m_context));
	apiDependenciesTestGroup->addChild(new glcts::MultisampleTextureDependenciesTexParameterTest(m_context));

	/* Assign tests to parent group: GLCoverage */
	apiGLCoverageTestGroup->addChild(new glcts::GLCoverageExtensionSpecificEnumsAreRecognizedTest(m_context));
	apiGLCoverageTestGroup->addChild(
		new glcts::GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets(m_context));
	apiGLCoverageTestGroup->addChild(new glcts::GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest(m_context));
	apiGLCoverageTestGroup->addChild(
		new glcts::GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest(
			m_context));

	/* Assign tests to parent group: FunctionalTests */
	functionalTestGroup->addChild(new glcts::MultisampleTextureFunctionalTestsBlittingTest(m_context));
	functionalTestGroup->addChild(
		new glcts::MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest(m_context));
	functionalTestGroup->addChild(
		new glcts::MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest(m_context));
	functionalTestGroup->addChild(
		new glcts::MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest(m_context));
	functionalTestGroup->addChild(
		new glcts::MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest(m_context));
	// TODO: temporarily disabled per request. Needs to be fixed.
	//functionalTestGroup->addChild(new glcts::MultisampleTextureFunctionalTestsSampleMaskingTexturesTest                            (m_context) );
	functionalTestGroup->addChild(
		new glcts::MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest(m_context));
	functionalTestGroup->addChild(new glcts::MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest(m_context));
}
} /* glcts namespace */
