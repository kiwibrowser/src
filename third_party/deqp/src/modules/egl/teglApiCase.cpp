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
 * \brief API test case.
 *//*--------------------------------------------------------------------*/

#include "teglApiCase.hpp"
#include "egluUtil.hpp"
#include "egluStrUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "deSTLUtil.hpp"

namespace deqp
{
namespace egl
{

using tcu::TestLog;
using std::vector;
using namespace eglw;

ApiCase::ApiCase (EglTestContext& eglTestCtx, const char* name, const char* description)
	: TestCase		(eglTestCtx, name, description)
	, CallLogWrapper(eglTestCtx.getLibrary(), eglTestCtx.getTestContext().getLog())
	, m_display		(EGL_NO_DISPLAY)
{
}

ApiCase::~ApiCase (void)
{
}

void ApiCase::init (void)
{
	m_display				= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_supportedClientAPIs	= eglu::getClientAPIs(m_eglTestCtx.getLibrary(), m_display);
}

void ApiCase::deinit (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	egl.terminate(m_display);

	m_display = EGL_NO_DISPLAY;
	m_supportedClientAPIs.clear();
}

ApiCase::IterateResult ApiCase::iterate (void)
{
	// Initialize result to pass.
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Enable call logging.
	enableLogging(true);

	// Run test.
	test();

	return STOP;
}

bool ApiCase::isAPISupported (eglw::EGLenum api) const
{
	return de::contains(m_supportedClientAPIs.begin(), m_supportedClientAPIs.end(), api);
}

void ApiCase::expectError (EGLenum expected)
{
	EGLenum err = m_eglTestCtx.getLibrary().getError();
	if (err != expected)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: " << eglu::getErrorStr(expected) << ", Got: " << eglu::getErrorStr(err) << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid error");
	}
}

void ApiCase::expectEitherError (EGLenum expectedA, EGLenum expectedB)
{
	EGLenum err = m_eglTestCtx.getLibrary().getError();
	if (err != expectedA && err != expectedB)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: " << eglu::getErrorStr(expectedA) << " or " << eglu::getErrorStr(expectedB) << ", Got: " << eglu::getErrorStr(err) << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid error");
	}
}

void ApiCase::expectBoolean (EGLBoolean expected, EGLBoolean got)
{
	if (expected != got)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: " << eglu::getBooleanStr(expected) <<  ", Got: " << eglu::getBooleanStr(got) << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
	}
}

void ApiCase::expectNoContext (EGLContext got)
{
	if (got != EGL_NO_CONTEXT)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: EGL_NO_CONTEXT" << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
		eglDestroyContext(getDisplay(), got);
	}
}

void ApiCase::expectNoSurface (EGLSurface got)
{
	if (got != EGL_NO_SURFACE)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: EGL_NO_SURFACE" << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
		eglDestroySurface(getDisplay(), got);
	}
}

void ApiCase::expectNoDisplay (EGLDisplay got)
{
	if (got != EGL_NO_DISPLAY)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: EGL_NO_DISPLAY" << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
	}
}

void ApiCase::expectNull (const void* got)
{
	if (got != DE_NULL)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: NULL" << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
	}
}

bool ApiCase::getConfig (EGLConfig* config, const eglu::FilterList& filters)
{
	try
	{
		*config = eglu::chooseSingleConfig(m_eglTestCtx.getLibrary(), m_display, filters);
		return true;
	}
	catch (const tcu::NotSupportedError&)
	{
		return false;
	}
}

} // egl
} // deqp
