/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief OpenGL ES 3 Test Package.
 */ /*-------------------------------------------------------------------*/

#include "es32cTestPackage.hpp"
#include "es32cCopyImageTests.hpp"
#include "esextcTestPackage.hpp"
#include "glcAggressiveShaderOptimizationsTests.hpp"
#include "glcFragDepthTests.hpp"
#include "glcInfoTests.hpp"
#include "glcInternalformatTests.hpp"
#include "glcSeparableProgramsTransformFeedbackTests.hpp"
#include "glcShaderConstExprTests.hpp"
#include "glcShaderIndexingTests.hpp"
#include "glcShaderIntegerMixTests.hpp"
#include "glcShaderLibrary.hpp"
#include "glcShaderLoopTests.hpp"
#include "glcShaderMacroTests.hpp"
#include "glcShaderNegativeTests.hpp"
#include "glcShaderStructTests.hpp"
#include "glcShaderSwitchTests.hpp"
#include "glcUniformBlockTests.hpp"
#include "gluStateReset.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include "../glesext/draw_buffers_indexed/esextcDrawBuffersIndexedTests.hpp"
#include "../glesext/geometry_shader/esextcGeometryShaderTests.hpp"
#include "../glesext/gpu_shader5/esextcGPUShader5Tests.hpp"
#include "../glesext/tessellation_shader/esextcTessellationShaderTests.hpp"
#include "../glesext/texture_border_clamp/esextcTextureBorderClampTests.hpp"
#include "../glesext/texture_buffer/esextcTextureBufferTests.hpp"
#include "../glesext/texture_cube_map_array/esextcTextureCubeMapArrayTests.hpp"

namespace es32cts
{

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(ES32TestPackage& package);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);

private:
	ES32TestPackage& m_testPackage;
};

TestCaseWrapper::TestCaseWrapper(ES32TestPackage& package) : m_testPackage(package)
{
}

TestCaseWrapper::~TestCaseWrapper(void)
{
}

void TestCaseWrapper::init(tcu::TestCase* testCase, const std::string&)
{
	testCase->init();
}

void TestCaseWrapper::deinit(tcu::TestCase* testCase)
{
	testCase->deinit();

	glu::resetState(m_testPackage.getContext().getRenderContext(), m_testPackage.getContext().getContextInfo());
}

tcu::TestNode::IterateResult TestCaseWrapper::iterate(tcu::TestCase* testCase)
{
	tcu::TestContext&			 testCtx   = m_testPackage.getContext().getTestContext();
	glu::RenderContext&			 renderCtx = m_testPackage.getContext().getRenderContext();
	tcu::TestCase::IterateResult result;

	// Clear to surrender-blue
	{
		const glw::Functions& gl = renderCtx.getFunctions();
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.f);
		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	result = testCase->iterate();

	// Call implementation specific post-iterate routine (usually handles native events and swaps buffers)
	try
	{
		renderCtx.postIterate();
		return result;
	}
	catch (const tcu::ResourceError&)
	{
		testCtx.getLog().endCase(QP_TEST_RESULT_RESOURCE_ERROR, "Resource error in context post-iteration routine");
		testCtx.setTerminateAfter(true);
		return tcu::TestNode::STOP;
	}
	catch (const std::exception&)
	{
		testCtx.getLog().endCase(QP_TEST_RESULT_FAIL, "Error in context post-iteration routine");
		return tcu::TestNode::STOP;
	}
}

ES32TestPackage::ES32TestPackage(tcu::TestContext& testCtx, const char* packageName)
	: deqp::TestPackage(testCtx, packageName, "OpenGL ES 3.2 Conformance Tests",
						glu::ContextType(glu::ApiType::es(3, 2)), "gl_cts/data/gles32/")
{
}

ES32TestPackage::~ES32TestPackage(void)
{
	deqp::TestPackage::deinit();
}

void ES32TestPackage::init(void)
{
	// Call init() in parent - this creates context.
	deqp::TestPackage::init();

	try
	{
		// Add main test groups
		addChild(new deqp::InfoTests(getContext()));

		tcu::TestCaseGroup* shadersGroup = new tcu::TestCaseGroup(getTestContext(), "shaders", "");
		shadersGroup->addChild(new deqp::ShaderIntegerMixTests(getContext(), glu::GLSL_VERSION_320_ES));
		shadersGroup->addChild(new deqp::ShaderNegativeTests(getContext(), glu::GLSL_VERSION_320_ES));
		shadersGroup->addChild(new glcts::AggressiveShaderOptimizationsTests(getContext()));
		addChild(shadersGroup);

		tcu::TestCaseGroup*  coreGroup = new tcu::TestCaseGroup(getTestContext(), "core", "");
		glcts::ExtParameters extParams(glu::GLSL_VERSION_320_ES, glcts::EXTENSIONTYPE_NONE);
		coreGroup->addChild(new glcts::GeometryShaderTests(getContext(), extParams));
		coreGroup->addChild(new glcts::GPUShader5Tests(getContext(), extParams));
		coreGroup->addChild(new glcts::TessellationShaderTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureCubeMapArrayTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureBorderClampTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureBufferTests(getContext(), extParams));
		coreGroup->addChild(new glcts::DrawBuffersIndexedTests(getContext(), extParams));
		coreGroup->addChild(new glcts::ShaderConstExprTests(getContext()));
		coreGroup->addChild(new glcts::ShaderMacroTests(getContext()));
		coreGroup->addChild(new glcts::SeparableProgramsTransformFeedbackTests(getContext()));
		coreGroup->addChild(new glcts::CopyImageTests(getContext()));
		coreGroup->addChild(new glcts::InternalformatTests(getContext()));
		addChild(coreGroup);
	}
	catch (...)
	{
		// Destroy context.
		deqp::TestPackage::deinit();
		throw;
	}
}

tcu::TestCaseExecutor* ES32TestPackage::createExecutor(void) const
{
	return new TestCaseWrapper(const_cast<ES32TestPackage&>(*this));
}

} // es32cts
