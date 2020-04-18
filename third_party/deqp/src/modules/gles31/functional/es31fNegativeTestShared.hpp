#ifndef _ES31FNEGATIVETESTSHARED_HPP
#define _ES31FNEGATIVETESTSHARED_HPP
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

#include "tcuDefs.hpp"
#include "glwDefs.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluShaderUtil.hpp"
#include "tes31TestCase.hpp"

namespace tcu
{

class ResultCollector;

} // tcu

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

class ErrorCase : public TestCase
{
public:
								ErrorCase		(Context& ctx, const char* name, const char* desc);
	virtual						~ErrorCase		(void) {}

	virtual void				expectError		(glw::GLenum error0, glw::GLenum error1) = 0;
};

class NegativeTestContext : public glu::CallLogWrapper
{
public:
								NegativeTestContext		(ErrorCase& host, glu::RenderContext& renderCtx, const glu::ContextInfo& ctxInfo, tcu::TestLog& log, tcu::ResultCollector& results, bool enableLog);
								~NegativeTestContext	();

	const tcu::ResultCollector&	getResults				(void) const;

	void						fail					(const std::string& msg);
	int							getInteger				(glw::GLenum pname) const;
	const glu::RenderContext&	getRenderContext		(void) const { return m_renderCtx; }
	const glu::ContextInfo&		getContextInfo			(void) const { return m_ctxInfo; }
	void						beginSection			(const std::string& desc);
	void						endSection				(void);

	void						expectError				(glw::GLenum error);
	void						expectError				(glw::GLenum error0, glw::GLenum error1);
	bool						isShaderSupported		(glu::ShaderType shaderType);
	bool						isExtensionSupported	(std::string extension);

protected:
	ErrorCase&					m_host;

private:
	glu::RenderContext&			m_renderCtx;
	const glu::ContextInfo&		m_ctxInfo;
	tcu::ResultCollector&		m_results;
	int							m_openSections;
};

typedef void (*TestFunc)(NegativeTestContext& ctx);

struct FunctionContainer
{
	TestFunc	function;
	const char* name;
	const char* desc;
};

} // NegativeTestShared
} // Functional
} // gles31
} // deqp

#endif // _ES31FNEGATIVETESTSHARED_HPP
