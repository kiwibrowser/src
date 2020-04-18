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
 * \brief OpenGL ES Extensions Test Package.
 */ /*-------------------------------------------------------------------*/

#include "esextcTestPackage.hpp"

#include "draw_elements_base_vertex/esextcDrawElementsBaseVertexTests.hpp"
#include "geometry_shader/esextcGeometryShaderTests.hpp"
#include "gpu_shader5/esextcGPUShader5Tests.hpp"
#include "tessellation_shader/esextcTessellationShaderTests.hpp"
#include "texture_border_clamp/esextcTextureBorderClampTests.hpp"
#include "texture_buffer/esextcTextureBufferTests.hpp"
#include "texture_cube_map_array/esextcTextureCubeMapArrayTests.hpp"

#include "glcViewportArrayTests.hpp"
#include "gluStateReset.hpp"
#include "tcuTestLog.hpp"

namespace esextcts
{

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper(ESEXTTestPackage& package);
	~TestCaseWrapper(void);

	void init(tcu::TestCase* testCase, const std::string& path);
	void deinit(tcu::TestCase* testCase);
	tcu::TestNode::IterateResult iterate(tcu::TestCase* testCase);

private:
	ESEXTTestPackage& m_testPackage;
};

TestCaseWrapper::TestCaseWrapper(ESEXTTestPackage& package) : m_testPackage(package)
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

ESEXTTestPackage::ESEXTTestPackage(tcu::TestContext& testCtx, const char* packageName)
	: TestPackage(testCtx, packageName, "OpenGL ES Extensions Conformance Tests",
				  glu::ContextType(glu::ApiType::es(3, 1)), "gl_cts/data/")
{
}

ESEXTTestPackage::~ESEXTTestPackage(void)
{
}

void ESEXTTestPackage::init(void)
{
	// Call init() in parent - this creates context.
	TestPackage::init();

	try
	{
		const glu::ContextType& context_type = getContext().getRenderContext().getType();
		glcts::ExtParameters	extParams(glu::GLSL_VERSION_310_ES, glcts::EXTENSIONTYPE_EXT);
		if (glu::contextSupports(context_type, glu::ApiType::es(3, 2)))
		{
			extParams.glslVersion = glu::GLSL_VERSION_320_ES;
			extParams.extType	 = glcts::EXTENSIONTYPE_NONE;
		}

		addChild(new glcts::GeometryShaderTests(getContext(), extParams));
		addChild(new glcts::GPUShader5Tests(getContext(), extParams));
		addChild(new glcts::TessellationShaderTests(getContext(), extParams));
		addChild(new glcts::TextureCubeMapArrayTests(getContext(), extParams));
		addChild(new glcts::TextureBorderClampTests(getContext(), extParams));
		addChild(new glcts::TextureBufferTests(getContext(), extParams));
		addChild(new glcts::DrawElementsBaseVertexTests(getContext(), extParams));
		glcts::ExtParameters viewportParams(glu::GLSL_VERSION_310_ES, glcts::EXTENSIONTYPE_OES);
		addChild(new glcts::ViewportArrayTests(getContext(), viewportParams));
	}
	catch (...)
	{
		// Destroy context.
		TestPackage::deinit();
		throw;
	}
}

tcu::TestCaseExecutor* ESEXTTestPackage::createExecutor(void) const
{
	return new TestCaseWrapper(const_cast<ESEXTTestPackage&>(*this));
}

} // esextcts
