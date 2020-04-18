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

#include "es3cTestPackage.hpp"
#include "glcAggressiveShaderOptimizationsTests.hpp"
#include "glcExposedExtensionsTests.hpp"
#include "glcFragDepthTests.hpp"
#include "glcInfoTests.hpp"
#include "glcInternalformatTests.hpp"
#include "glcPackedDepthStencilTests.hpp"
#include "glcPackedPixelsTests.hpp"
#include "glcParallelShaderCompileTests.hpp"
#include "glcShaderConstExprTests.hpp"
#include "glcShaderIndexingTests.hpp"
#include "glcShaderIntegerMixTests.hpp"
#include "glcShaderLibrary.hpp"
#include "glcShaderLoopTests.hpp"
#include "glcShaderMacroTests.hpp"
#include "glcShaderNegativeTests.hpp"
#include "glcShaderStructTests.hpp"
#include "glcShaderSwitchTests.hpp"
#include "glcTextureFilterAnisotropicTests.hpp"
#include "glcTextureRepeatModeTests.hpp"
#include "glcUniformBlockTests.hpp"
#include "gluStateReset.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace es3cts
{

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(ES30TestPackage& package);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);

private:
	ES30TestPackage& m_testPackage;
};

TestCaseWrapper::TestCaseWrapper(ES30TestPackage& package) : m_testPackage(package)
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
	tcu::TestContext&			 testCtx   = m_testPackage.getContext().getTestContext();
	glu::RenderContext&			 renderCtx = m_testPackage.getContext().getRenderContext();
	tcu::TestCase::IterateResult result;

	// Clear to black
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

class ShaderTests : public deqp::TestCaseGroup
{
public:
	ShaderTests(deqp::Context& context) : TestCaseGroup(context, "shaders", "Shading Language Tests")
	{
	}

	void init(void)
	{
		addChild(new deqp::ShaderLibraryGroup(m_context, "arrays", "Array Tests", "arrays.test"));
		addChild(new deqp::ShaderLibraryGroup(m_context, "declarations", "Declaration Tests", "declarations.test"));
		addChild(new deqp::FragDepthTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderIndexingTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderLoopTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderLibraryGroup(m_context, "preprocessor", "Preprocessor Tests", "preprocessor.test"));
		addChild(new deqp::ShaderLibraryGroup(m_context, "literal_parsing", "Literal Parsing Tests",
											  "literal_parsing.test"));
		addChild(new deqp::ShaderLibraryGroup(m_context, "name_hiding", "Name Hiding Tests", "name_hiding.test"));
		addChild(new deqp::ShaderStructTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderSwitchTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::UniformBlockTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderIntegerMixTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new deqp::ShaderNegativeTests(m_context, glu::GLSL_VERSION_300_ES));
		addChild(new glcts::AggressiveShaderOptimizationsTests(m_context));
	}
};

ES30TestPackage::ES30TestPackage(tcu::TestContext& testCtx, const char* packageName)
	: deqp::TestPackage(testCtx, packageName, "OpenGL ES 3 Conformance Tests", glu::ContextType(glu::ApiType::es(3, 0)),
						"gl_cts/data/gles3/")
{
}

ES30TestPackage::~ES30TestPackage(void)
{
}

void ES30TestPackage::init(void)
{
	// Call init() in parent - this creates context.
	deqp::TestPackage::init();

	try
	{
		addChild(new ShaderTests(getContext()));
		addChild(new glcts::TextureFilterAnisotropicTests(getContext()));
		addChild(new glcts::TextureRepeatModeTests(getContext()));
		addChild(new glcts::ExposedExtensionsTests(getContext()));
		tcu::TestCaseGroup* coreGroup = new tcu::TestCaseGroup(getTestContext(), "core", "core tests");
		coreGroup->addChild(new glcts::ShaderConstExprTests(getContext()));
		coreGroup->addChild(new glcts::ShaderMacroTests(getContext()));
		coreGroup->addChild(new glcts::InternalformatTests(getContext()));
		addChild(coreGroup);
		addChild(new glcts::ParallelShaderCompileTests(getContext()));
		addChild(new glcts::PackedPixelsTests(getContext()));
		addChild(new glcts::PackedDepthStencilTests(getContext()));
	}
	catch (...)
	{
		// Destroy context.
		deqp::TestPackage::deinit();
		throw;
	}
}

tcu::TestCaseExecutor* ES30TestPackage::createExecutor(void) const
{
	return new TestCaseWrapper(const_cast<ES30TestPackage&>(*this));
}

} // es3cts
