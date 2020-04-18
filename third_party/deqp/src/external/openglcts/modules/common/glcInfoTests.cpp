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
 * \brief Platform Information Tests.
 */ /*-------------------------------------------------------------------*/

#include "glcInfoTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

namespace deqp
{

using std::string;
using std::vector;
using tcu::TestLog;

class QueryStringCase : public TestCase
{
public:
	QueryStringCase(Context& context, const char* name, const char* description, deUint32 query)
		: TestCase(context, name, description), m_query(query)
	{
	}

	IterateResult iterate(void)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		const char* result = (const char*)gl.getString(m_query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetString() failed");

		m_testCtx.getLog() << tcu::TestLog::Message << result << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

private:
	deUint32 m_query;
};

class QueryExtensionsCase : public TestCase
{
public:
	QueryExtensionsCase(Context& context) : TestCase(context, "extensions", "Supported Extensions")
	{
	}

	IterateResult iterate(void)
	{
		const vector<string> extensions = m_context.getContextInfo().getExtensions();

		for (vector<string>::const_iterator i = extensions.begin(); i != extensions.end(); i++)
			m_testCtx.getLog() << tcu::TestLog::Message << *i << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

class RenderTargetInfoCase : public TestCase
{
public:
	RenderTargetInfoCase(Context& context) : TestCase(context, "render_target", "Render Target Information")
	{
	}

	IterateResult iterate(void)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();

		m_testCtx.getLog() << TestLog::Integer("Width", "Width", "px", QP_KEY_TAG_NONE, renderTarget.getWidth())
						   << TestLog::Integer("Height", "Height", "px", QP_KEY_TAG_NONE, renderTarget.getHeight())
						   << TestLog::Integer("RedBits", "Red bits", "", QP_KEY_TAG_NONE, pixelFormat.redBits)
						   << TestLog::Integer("GreenBits", "Green bits", "", QP_KEY_TAG_NONE, pixelFormat.greenBits)
						   << TestLog::Integer("BlueBits", "Blue bits", "", QP_KEY_TAG_NONE, pixelFormat.blueBits)
						   << TestLog::Integer("AlphaBits", "Alpha bits", "", QP_KEY_TAG_NONE, pixelFormat.alphaBits)
						   << TestLog::Integer("DepthBits", "Depth bits", "", QP_KEY_TAG_NONE,
											   renderTarget.getDepthBits())
						   << TestLog::Integer("StencilBits", "Stencil bits", "", QP_KEY_TAG_NONE,
											   renderTarget.getStencilBits())
						   << TestLog::Integer("SampleCount", "Sample count", "", QP_KEY_TAG_NONE,
											   renderTarget.getNumSamples());

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
};

InfoTests::InfoTests(Context& context) : TestCaseGroup(context, "info", "Platform information queries")
{
}

InfoTests::~InfoTests(void)
{
}

void InfoTests::init(void)
{
	addChild(new QueryStringCase(m_context, "vendor", "Vendor String", GL_VENDOR));
	addChild(new QueryStringCase(m_context, "renderer", "Renderer String", GL_RENDERER));
	addChild(new QueryStringCase(m_context, "version", "Version String", GL_VERSION));
	addChild(new QueryStringCase(m_context, "shading_language_version", "Shading Language Version String",
								 GL_SHADING_LANGUAGE_VERSION));
	addChild(new QueryExtensionsCase(m_context));
	addChild(new RenderTargetInfoCase(m_context));
}

} // deqp
