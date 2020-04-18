/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Shared structures for ES 3.1 negative API tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeTestShared.hpp"

#include "tcuResultCollector.hpp"

#include "gluContextInfo.hpp"
#include "gluRenderContext.hpp"
#include "glwFunctions.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

using glw::GLenum;
using tcu::TestLog;
using std::string;

ErrorCase::ErrorCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

NegativeTestContext::NegativeTestContext (ErrorCase&				host,
										  glu::RenderContext&		renderCtx,
										  const glu::ContextInfo&	ctxInfo,
										  tcu::TestLog&				log,
										  tcu::ResultCollector&		results,
										  bool						enableLogging_)
	: glu::CallLogWrapper	(renderCtx.getFunctions(), log)
	, m_host				(host)
	, m_renderCtx			(renderCtx)
	, m_ctxInfo				(ctxInfo)
	, m_results				(results)
	, m_openSections		(0)
{
	enableLogging(enableLogging_);
}

NegativeTestContext::~NegativeTestContext ()
{
	while (m_openSections--)
		getLog() << TestLog::EndSection;
}

void NegativeTestContext::fail (const string& msg)
{
	m_results.addResult(QP_TEST_RESULT_FAIL, msg);
}

int NegativeTestContext::getInteger (GLenum pname) const
{
	int retval = 0;
	m_renderCtx.getFunctions().getIntegerv(pname, &retval);
	return retval;
}

void NegativeTestContext::beginSection (const string& desc)
{
	if (isLoggingEnabled())
	{
		getLog() << TestLog::Section("callstream", desc);
		m_openSections++;
	}
}

void NegativeTestContext::endSection (void)
{
	if (isLoggingEnabled())
	{
		DE_ASSERT (m_openSections > 0);
		getLog() << TestLog::EndSection;
		m_openSections--;
	}
}

void NegativeTestContext::expectError (GLenum error)
{
	m_host.expectError(error, error);
}

void NegativeTestContext::expectError (GLenum error0, GLenum error1)
{
	m_host.expectError(error0, error1);
}

bool NegativeTestContext::isShaderSupported (glu::ShaderType shaderType)
{
	if (contextSupports(getRenderContext().getType(), glu::ApiType::es(3, 2)))
		return true;

	switch (shaderType)
	{
		case glu::SHADERTYPE_GEOMETRY:
			return getContextInfo().isExtensionSupported("GL_EXT_geometry_shader");
		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			return getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader");
		default:
			return true;
	}
}

bool NegativeTestContext::isExtensionSupported (std::string extension)
{
	return getContextInfo().isExtensionSupported(extension.c_str());
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
