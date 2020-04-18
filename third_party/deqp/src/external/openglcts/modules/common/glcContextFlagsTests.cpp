/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file glcContextFlagsTests.cpp
 * \brief Tests veryfing glGetIntegerv(GL_CONTEXT_FLAGS).
 */ /*-------------------------------------------------------------------*/

#include "glcContextFlagsTests.hpp"
#include "gluRenderContext.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

class ContextFlagsCase : public tcu::TestCase
{
private:
	glu::RenderContext* m_caseContext;
	glu::ContextFlags   m_passedFlags;
	glw::GLint			m_expectedResult;
	glu::ApiType		m_ApiType;

	void createContext();

public:
	ContextFlagsCase(tcu::TestContext& testCtx, glu::ContextFlags passedFlags, glw::GLint expectedResult,
					 const char* name, const char* description, glu::ApiType apiType)
		: tcu::TestCase(testCtx, name, description)
		, m_caseContext(NULL)
		, m_passedFlags(passedFlags)
		, m_expectedResult(expectedResult)
		, m_ApiType(apiType)
	{
	}

	void releaseContext(void);

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);
};

void ContextFlagsCase::createContext()
{
	glu::RenderConfig renderCfg(glu::ContextType(m_ApiType, m_passedFlags));

	const tcu::CommandLine& commandLine = m_testCtx.getCommandLine();
	glu::parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() != tcu::SURFACETYPE_WINDOW)
		throw tcu::NotSupportedError("Test not supported in non-windowed context");

	m_caseContext = glu::createRenderContext(m_testCtx.getPlatform(), commandLine, renderCfg);
}

void ContextFlagsCase::releaseContext(void)
{
	if (m_caseContext)
	{
		delete m_caseContext;
		m_caseContext = NULL;
	}
}

void ContextFlagsCase::deinit(void)
{
	releaseContext();
}

tcu::TestNode::IterateResult ContextFlagsCase::iterate(void)
{
	createContext();

	glw::GLint			  flags = 0;
	const glw::Functions& gl = m_caseContext->getFunctions();
	gl.getIntegerv(GL_CONTEXT_FLAGS, &flags);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");

	if (flags != m_expectedResult)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! glGet returned wrong  value " << flags
						   << ", expected " << m_expectedResult << "]." << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	releaseContext();
	return STOP;
}

ContextFlagsTests::ContextFlagsTests(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCaseGroup(testCtx, "context_flags", "Verifies if context flags query results are as expected.")
	, m_ApiType(apiType)
{
}

void ContextFlagsTests::init()
{
	tcu::TestCaseGroup::init();

	try
	{
		addChild(new ContextFlagsCase(m_testCtx, glu::ContextFlags(0), 0, "no_flags_set_case",
									  "Verifies no flags case.", m_ApiType));
		addChild(new ContextFlagsCase(m_testCtx, glu::CONTEXT_DEBUG, GL_CONTEXT_FLAG_DEBUG_BIT, "debug_flag_set_case",
									  "Verifies debug flag case..", m_ApiType));
		addChild(new ContextFlagsCase(m_testCtx, glu::CONTEXT_ROBUST, GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT,
									  "robust_flag_set_case", "Verifies robust access flag case.", m_ApiType));

		addChild(new ContextFlagsCase(m_testCtx, glu::CONTEXT_DEBUG | glu::CONTEXT_ROBUST,
									  GL_CONTEXT_FLAG_DEBUG_BIT | GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT,
									  "all_flags_set_case", "Verifies both debug and robust access flags case.",
									  m_ApiType));
	}
	catch (...)
	{
		// Destroy context.
		tcu::TestCaseGroup::deinit();
		throw;
	}
}

} // glcts namespace
