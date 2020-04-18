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
 * \brief OpenGL ES 3.1 Test Package.
 */ /*-------------------------------------------------------------------*/

#include "es31cTestPackage.hpp"

#include "es31cArrayOfArraysTests.hpp"
#include "es31cComputeShaderTests.hpp"
#include "es31cDrawIndirectTests.hpp"
#include "es31cExplicitUniformLocationTest.hpp"
#include "es31cFramebufferNoAttachmentsTests.hpp"
#include "es31cLayoutBindingTests.hpp"
#include "es31cProgramInterfaceQueryTests.hpp"
#include "es31cSampleShadingTests.hpp"

#include "es31cSeparateShaderObjsTests.hpp"
#include "es31cShaderAtomicCountersTests.hpp"
#include "es31cShaderBitfieldOperationTests.hpp"
#include "es31cShaderImageLoadStoreTests.hpp"
#include "es31cShaderImageSizeTests.hpp"
#include "es31cShaderStorageBufferObjectTests.hpp"
#include "es31cTextureGatherTests.hpp"
#include "es31cTextureStorageMultisampleTests.hpp"
#include "es31cVertexAttribBindingTests.hpp"
#include "glcAggressiveShaderOptimizationsTests.hpp"
#include "glcBlendEquationAdvancedTests.hpp"
#include "glcInfoTests.hpp"
#include "glcInternalformatTests.hpp"
#include "glcPolygonOffsetClampTests.hpp"
#include "glcSampleVariablesTests.hpp"
#include "glcShaderConstExprTests.hpp"
#include "glcShaderGroupVoteTests.hpp"
#include "glcShaderIntegerMixTests.hpp"
#include "glcShaderMacroTests.hpp"
#include "glcShaderMultisampleInterpolationTests.hpp"
#include "glcShaderNegativeTests.hpp"

#include "gluStateReset.hpp"

#include "../glesext/draw_buffers_indexed/esextcDrawBuffersIndexedTests.hpp"
#include "../glesext/geometry_shader/esextcGeometryShaderTests.hpp"
#include "../glesext/gpu_shader5/esextcGPUShader5Tests.hpp"
#include "../glesext/tessellation_shader/esextcTessellationShaderTests.hpp"
#include "../glesext/texture_border_clamp/esextcTextureBorderClampTests.hpp"
#include "../glesext/texture_buffer/esextcTextureBufferTests.hpp"
#include "../glesext/texture_cube_map_array/esextcTextureCubeMapArrayTests.hpp"
#include "glcViewportArrayTests.hpp"

namespace es31cts
{

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(ES31TestPackage& package);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);

private:
	ES31TestPackage& m_testPackage;
};

TestCaseWrapper::TestCaseWrapper(ES31TestPackage& package) : m_testPackage(package)
{
}

TestCaseWrapper::~TestCaseWrapper(void)
{
}

void TestCaseWrapper::init(tcu::TestCase* testCase, const std::string&)
{
	glu::resetState(m_testPackage.getContext().getRenderContext(), m_testPackage.getContext().getContextInfo());

	testCase->init();
}

void TestCaseWrapper::deinit(tcu::TestCase* testCase)
{
	testCase->deinit();

	glu::resetState(m_testPackage.getContext().getRenderContext(), m_testPackage.getContext().getContextInfo());
}

tcu::TestNode::IterateResult TestCaseWrapper::iterate(tcu::TestCase* testCase)
{
	tcu::TestContext&   testCtx   = m_testPackage.getContext().getTestContext();
	glu::RenderContext& renderCtx = m_testPackage.getContext().getRenderContext();

	// Clear to black
	{
		const glw::Functions& gl = renderCtx.getFunctions();
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.f);
		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	const tcu::TestCase::IterateResult result = testCase->iterate();

	// Call implementation specific post-iterate routine (usually handles native events and swaps buffers)
	try
	{
		m_testPackage.getContext().getRenderContext().postIterate();
		return result;
	}
	catch (const tcu::ResourceError& e)
	{
		testCtx.getLog() << e;
		testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Resource error in context post-iteration routine");
		testCtx.setTerminateAfter(true);
		return tcu::TestNode::STOP;
	}
	catch (const std::exception& e)
	{
		testCtx.getLog() << e;
		testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Error in context post-iteration routine");
		return tcu::TestNode::STOP;
	}
}

ES31TestPackage::ES31TestPackage(tcu::TestContext& testCtx, const char* packageName)
	: deqp::TestPackage(testCtx, packageName, "OpenGL ES 3.1 Conformance Tests",
						glu::ContextType(glu::ApiType::es(3, 1)), "gl_cts/data/gles31/")
{
}

ES31TestPackage::~ES31TestPackage(void)
{
}

class ShaderTests : public deqp::TestCaseGroup
{
public:
	ShaderTests(deqp::Context& context) : TestCaseGroup(context, "shaders", "Shading Language Tests")
	{
	}

	void init(void)
	{
		addChild(new deqp::ShaderNegativeTests(m_context, glu::GLSL_VERSION_310_ES));
		addChild(new glcts::AggressiveShaderOptimizationsTests(m_context));
	}
};

void ES31TestPackage::init(void)
{
	// Call init() in parent - this creates context.
	deqp::TestPackage::init();

	try
	{
		tcu::TestCaseGroup* coreGroup = new tcu::TestCaseGroup(getTestContext(), "core", "core tests");

		coreGroup->addChild(new glcts::TextureStorageMultisampleTests(getContext()));
		coreGroup->addChild(new glcts::ShaderAtomicCountersTests(getContext()));
		coreGroup->addChild(new glcts::TextureGatherTests(getContext()));
		coreGroup->addChild(new glcts::SampleShadingTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new deqp::SampleVariablesTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new glcts::SeparateShaderObjsTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new glcts::ShaderBitfieldOperationTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new deqp::ShaderMultisampleInterpolationTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new glcts::LayoutBindingTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new deqp::ShaderIntegerMixTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new glcts::ShaderConstExprTests(getContext()));
		coreGroup->addChild(new glcts::BlendEquationAdvancedTests(getContext(), glu::GLSL_VERSION_310_ES));
		coreGroup->addChild(new glcts::VertexAttribBindingTests(getContext()));
		coreGroup->addChild(new glcts::ShaderMacroTests(getContext()));
		coreGroup->addChild(new glcts::ShaderStorageBufferObjectTests(getContext()));
		coreGroup->addChild(new glcts::ComputeShaderTests(getContext()));
		coreGroup->addChild(new glcts::ShaderImageLoadStoreTests(getContext()));
		coreGroup->addChild(new glcts::ShaderImageSizeTests(getContext()));
		coreGroup->addChild(new glcts::DrawIndirectTestsES31(getContext()));
		coreGroup->addChild(new glcts::ExplicitUniformLocationES31Tests(getContext()));
		coreGroup->addChild(new glcts::ProgramInterfaceQueryTests(getContext()));
		coreGroup->addChild(new glcts::FramebufferNoAttachmentsTests(getContext()));
		coreGroup->addChild(new glcts::ArrayOfArraysTestGroup(getContext()));
		coreGroup->addChild(new glcts::PolygonOffsetClamp(getContext()));
		coreGroup->addChild(new glcts::ShaderGroupVote(getContext()));
		coreGroup->addChild(new glcts::InternalformatTests(getContext()));

		glcts::ExtParameters extParams(glu::GLSL_VERSION_310_ES, glcts::EXTENSIONTYPE_OES);
		coreGroup->addChild(new glcts::GeometryShaderTests(getContext(), extParams));
		coreGroup->addChild(new glcts::GPUShader5Tests(getContext(), extParams));
		coreGroup->addChild(new glcts::TessellationShaderTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureCubeMapArrayTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureBorderClampTests(getContext(), extParams));
		coreGroup->addChild(new glcts::TextureBufferTests(getContext(), extParams));
		coreGroup->addChild(new glcts::DrawBuffersIndexedTests(getContext(), extParams));
		coreGroup->addChild(new glcts::ViewportArrayTests(getContext(), extParams));

		addChild(coreGroup);

		addChild(new ShaderTests(getContext()));
	}
	catch (...)
	{
		// Destroy context.
		deqp::TestPackage::deinit();
		throw;
	}
}

tcu::TestCaseExecutor* ES31TestPackage::createExecutor(void) const
{
	return new TestCaseWrapper(const_cast<ES31TestPackage&>(*this));
}

} // es31cts
