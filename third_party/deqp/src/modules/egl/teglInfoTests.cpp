/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief EGL Implementation Information Tests
 *//*--------------------------------------------------------------------*/

#include "teglInfoTests.hpp"
#include "teglConfigList.hpp"
#include "tcuTestLog.hpp"
#include "deStringUtil.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include <vector>
#include <string>
#include <sstream>

namespace deqp
{
namespace egl
{

using std::vector;
using std::string;
using tcu::TestLog;
using namespace eglw;

static int toInt (std::string str)
{
	std::istringstream strStream(str);

	int out;
	strStream >> out;
	return out;
}

class InfoCase : public TestCase
{
public:
	InfoCase (EglTestContext& eglTestCtx, const char* name, const char* description)
		: TestCase	(eglTestCtx, name, description)
		, m_display	(EGL_NO_DISPLAY)
		, m_version	(0, 0)
	{
	}

	void init (void)
	{
		DE_ASSERT(m_display == EGL_NO_DISPLAY);
		m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay(), &m_version);
	}

	void deinit (void)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}

protected:
	EGLDisplay		m_display;
	eglu::Version	m_version;
};

class QueryStringCase : public InfoCase
{
public:
	QueryStringCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint query)
		: InfoCase	(eglTestCtx, name, description)
		, m_query	(query)
	{
	}

	void validateString (const std::string& result)
	{
		tcu::TestLog&				log		= m_testCtx.getLog();
		std::vector<std::string>	tokens	= de::splitString(result, ' ');

		if (m_query == EGL_VERSION)
		{
			const int	dispMajor	= m_version.getMajor();
			const int	dispMinor	= m_version.getMinor();

			const std::vector<std::string>	versionTokens	= de::splitString(tokens[0], '.');

			if (versionTokens.size() < 2)
			{
				log << TestLog::Message << "  Fail, first part of the string must be in the format <major_version.minor_version>" << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid version string");
			}
			else
			{
				const	int	stringMajor	= toInt(versionTokens[0]);
				const	int	stringMinor	= toInt(versionTokens[1]);

				if (stringMajor != dispMajor || stringMinor != dispMinor)
				{
					log << TestLog::Message << "  Fail, version numer (" << stringMajor << "." << stringMinor
						<< ") does not match the one reported by eglInitialize (" << dispMajor << "." << dispMinor << ")" << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Version number mismatch");
				}
			}
		}
	}

	IterateResult iterate (void)
	{
		const Library&	egl		= m_eglTestCtx.getLibrary();
		const char*		result	= egl.queryString(m_display, m_query);
		EGLU_CHECK_MSG(egl, "eglQueryString() failed");

		m_testCtx.getLog() << tcu::TestLog::Message << result << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		validateString(result);

		return STOP;
	}

private:
	EGLint m_query;
};

class QueryExtensionsCase : public InfoCase
{
public:
	QueryExtensionsCase (EglTestContext& eglTestCtx)
		: InfoCase	(eglTestCtx, "extensions", "Supported Extensions")
	{
	}

	IterateResult iterate (void)
	{
		const Library&	egl			= m_eglTestCtx.getLibrary();
		vector<string>	extensions	= eglu::getDisplayExtensions(egl, m_display);

		for (vector<string>::const_iterator i = extensions.begin(); i != extensions.end(); i++)
			m_testCtx.getLog() << tcu::TestLog::Message << *i << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

InfoTests::InfoTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "info", "Platform Information")
{
}

InfoTests::~InfoTests (void)
{
}

void InfoTests::init (void)
{
	addChild(new QueryStringCase(m_eglTestCtx, "version",		"EGL Version",				EGL_VERSION));
	addChild(new QueryStringCase(m_eglTestCtx, "vendor",		"EGL Vendor",				EGL_VENDOR));
	addChild(new QueryStringCase(m_eglTestCtx, "client_apis",	"Supported client APIs",	EGL_CLIENT_APIS));
	addChild(new QueryExtensionsCase(m_eglTestCtx));
	addChild(new ConfigList(m_eglTestCtx));
}

} // egl
} // deqp
