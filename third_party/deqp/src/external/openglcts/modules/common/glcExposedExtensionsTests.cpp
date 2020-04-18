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
 * \file  glcExtensionsExposeTests.cpp
 * \brief Test that check if specified substring is not in extension name.
 */ /*--------------------------------------------------------------------*/

#include "glcExposedExtensionsTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include <string>
#include <vector>

using namespace glu;

namespace glcts
{

class ExposedExtensionsTest : public deqp::TestCase
{
public:
	/* Public methods */
	ExposedExtensionsTest(deqp::Context& context, std::string notAllowedSubstring,
						  const std::vector<std::string>* allowedExceptions = NULL);

	virtual ~ExposedExtensionsTest(void);

	void						 deinit(void);
	void						 init(void);
	tcu::TestNode::IterateResult iterate(void);

private:
	/* Private members */
	std::string				 m_notAllowedSubstring;
	std::vector<std::string> m_allowedExceptions;
};

/** Constructor.
	 *
	 *  @param context             Rendering context
	 *  @param name                Test name
	 *  @param description         Test description
	 *  @param notAllowedSubstring Substring that should not be found in extension name.
	 *  @param allowedExceptions   List of exceptions that are allowed even despite
	 *                             containing notAllowedFraze.
	 */
ExposedExtensionsTest::ExposedExtensionsTest(deqp::Context& context, std::string notAllowedSubstring,
											 const std::vector<std::string>* allowedExceptions)
	: deqp::TestCase(context, "validate_extensions", "Test verifies if extensions with "
													 "specified phrase are not exposed.")
	, m_notAllowedSubstring(notAllowedSubstring)
{
	if (allowedExceptions)
	{
		m_allowedExceptions = *allowedExceptions;
	}
}

ExposedExtensionsTest::~ExposedExtensionsTest(void)
{
}

/** Tears down any GL objects set up to run the test. */
void ExposedExtensionsTest::deinit(void)
{
}

/** Stub init method */
void ExposedExtensionsTest::init(void)
{
}

/** Executes test iteration.
	 *
	 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
	 */
tcu::TestNode::IterateResult ExposedExtensionsTest::iterate(void)
{
	typedef std::vector<std::string> string_vector;
	const string_vector&			 extensions			   = m_context.getContextInfo().getExtensions();
	string_vector::const_iterator	currExtension		   = extensions.begin();
	bool							 allExceptionsAreValid = true;

	while (currExtension != extensions.end())
	{
		// If the current extension does not contain not allowed substring then continue
		if (currExtension->find(m_notAllowedSubstring) == std::string::npos)
		{
			++currExtension;
			continue;
		}

		// Check if current extension is one of allowed exceptions
		bool						  currExtensionIsNotAnException = true;
		string_vector::const_iterator exception						= m_allowedExceptions.begin();
		while (exception != m_allowedExceptions.end())
		{
			if ((*exception).compare(*currExtension) == 0)
			{
				currExtensionIsNotAnException = false;
				break;
			}
			++exception;
		}

		// Current exception is not on allowed exceptions list, test will fail
		// but other exceptions will be checked to log all not allowed extensions
		if (currExtensionIsNotAnException)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Implementations should not expose " << *currExtension
							   << tcu::TestLog::EndMessage;
			allExceptionsAreValid = false;
		}

		++currExtension;
	}

	if (allExceptionsAreValid)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

ExposedExtensionsTests::ExposedExtensionsTests(deqp::Context& context)
	: TestCaseGroup(context, "exposed_extensions", "Verifies exposed extensions")
{
}

void ExposedExtensionsTests::init(void)
{
	if (isContextTypeES(m_context.getRenderContext().getType()))
	{
		addChild(new ExposedExtensionsTest(m_context, "ARB"));
	}
	else
	{
		std::vector<std::string> allowedExtensions(1, "GL_OES_EGL_image");
		addChild(new glcts::ExposedExtensionsTest(getContext(), "OES", &allowedExtensions));
	}
}

} /* glcts namespace */
