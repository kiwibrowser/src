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
 * \brief Extension function pointer query tests.
 *//*--------------------------------------------------------------------*/

#include "teglGetProcAddressTests.hpp"
#include "teglTestCase.hpp"
#include "egluCallLogWrapper.hpp"
#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "tcuTestLog.hpp"
#include "deSTLUtil.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace egl
{

namespace
{

using tcu::TestLog;
using namespace eglw;

// Function name strings generated from API headers

#include "teglGetProcAddressTests.inl"

struct FunctionNames
{
	int					numFunctions;
	const char* const*	functions;

	FunctionNames (int numFunctions_, const char* const* functions_)
		: numFunctions	(numFunctions_)
		, functions		(functions_)
	{
	}
};

FunctionNames getExtFunctionNames (const std::string& extName)
{
	for (int ndx = 0; ndx <= DE_LENGTH_OF_ARRAY(s_extensions); ndx++)
	{
		if (extName == s_extensions[ndx].name)
			return FunctionNames(s_extensions[ndx].numFunctions, s_extensions[ndx].functions);
	}

	DE_ASSERT(false);
	return FunctionNames(0, DE_NULL);
}

FunctionNames getCoreFunctionNames (EGLint apiBit)
{
	switch (apiBit)
	{
		case 0:							return FunctionNames(DE_LENGTH_OF_ARRAY(s_EGL14),	s_EGL14);
		case EGL_OPENGL_ES_BIT:			return FunctionNames(DE_LENGTH_OF_ARRAY(s_GLES10),	s_GLES10);
		case EGL_OPENGL_ES2_BIT:		return FunctionNames(DE_LENGTH_OF_ARRAY(s_GLES20),	s_GLES20);
		case EGL_OPENGL_ES3_BIT_KHR:	return FunctionNames(DE_LENGTH_OF_ARRAY(s_GLES30),	s_GLES30);
		default:
			DE_ASSERT(false);
	}

	return FunctionNames(0, DE_NULL);
}

} // anonymous

// Base class for eglGetProcAddress() test cases

class GetProcAddressCase : public TestCase, protected eglu::CallLogWrapper
{
public:
								GetProcAddressCase		(EglTestContext& eglTestCtx, const char* name, const char* description);
	virtual						~GetProcAddressCase		(void);

	void						init					(void);
	void						deinit					(void);
	IterateResult				iterate					(void);

	bool						isSupported				(const std::string& extName);

	virtual void				executeTest				(void) = 0;

protected:
	EGLDisplay					m_display;

private:
	std::vector<std::string>	m_supported;
};

GetProcAddressCase::GetProcAddressCase (EglTestContext& eglTestCtx, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, CallLogWrapper	(eglTestCtx.getLibrary(), eglTestCtx.getTestContext().getLog())
	, m_display			(EGL_NO_DISPLAY)
{
}

GetProcAddressCase::~GetProcAddressCase (void)
{
}

void GetProcAddressCase::init (void)
{
	try
	{
		m_supported = eglu::getClientExtensions(m_eglTestCtx.getLibrary());
	}
	catch (const eglu::Error& error)
	{
		// EGL_BAD_DISPLAY is generated if client extensions are not supported.
		if (error.getError() != EGL_BAD_DISPLAY)
			throw;
	}

	DE_ASSERT(m_display == EGL_NO_DISPLAY);

	m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	{
		const std::vector<std::string> displayExtensios = eglu::getDisplayExtensions(m_eglTestCtx.getLibrary(), m_display);
		m_supported.insert(m_supported.end(), displayExtensios.begin(), displayExtensios.end());
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void GetProcAddressCase::deinit (void)
{
	m_eglTestCtx.getLibrary().terminate(m_display);
	m_display = EGL_NO_DISPLAY;
}

tcu::TestNode::IterateResult GetProcAddressCase::iterate (void)
{
	enableLogging(true);

	executeTest();

	enableLogging(false);

	return STOP;
}

bool GetProcAddressCase::isSupported (const std::string& extName)
{
	return de::contains(m_supported.begin(), m_supported.end(), extName);
}

// Test by extension

class GetProcAddressExtensionCase : public GetProcAddressCase
{
public:
	GetProcAddressExtensionCase (EglTestContext& eglTestCtx, const char* name, const char* description, const std::string& extName)
		: GetProcAddressCase	(eglTestCtx, name, description)
		, m_extName				(extName)
	{
	}

	virtual ~GetProcAddressExtensionCase (void)
	{
	}

	void executeTest (void)
	{
		TestLog&				log			= m_testCtx.getLog();
		bool					supported	= isSupported(m_extName);
		const FunctionNames		funcNames	= getExtFunctionNames(m_extName);

		DE_ASSERT(funcNames.numFunctions > 0);

		log << TestLog::Message << m_extName << ": " << (supported ? "supported" : "not supported") << TestLog::EndMessage;
		log << TestLog::Message << TestLog::EndMessage;

		for (int funcNdx = 0; funcNdx < funcNames.numFunctions; funcNdx++)
		{
			const char*	funcName		= funcNames.functions[funcNdx];
			void		(*funcPtr)(void);

			funcPtr = eglGetProcAddress(funcName);
			eglu::checkError(eglGetError(), "eglGetProcAddress()", __FILE__, __LINE__);

			if (supported && funcPtr == 0)
			{
				log << TestLog::Message << "Fail, received null pointer for supported extension function: " << funcName << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected null pointer");
			}
		}
	}

private:
	std::string	m_extName;
};

// Test core functions

class GetProcAddressCoreFunctionsCase : public GetProcAddressCase
{
public:
	GetProcAddressCoreFunctionsCase (EglTestContext& eglTestCtx, const char* name, const char* description, const EGLint apiBit)
		: GetProcAddressCase	(eglTestCtx, name, description)
		, m_apiBit				(apiBit)
	{
	}

	virtual ~GetProcAddressCoreFunctionsCase (void)
	{
	}

	void executeTest (void)
	{
		TestLog&				log					= m_testCtx.getLog();
		const bool				funcPtrSupported	= isSupported("EGL_KHR_get_all_proc_addresses");
		const bool				apiSupported		= (eglu::getRenderableAPIsMask(m_eglTestCtx.getLibrary(), m_display) & m_apiBit) == m_apiBit;
		const FunctionNames		funcNames			= getCoreFunctionNames(m_apiBit);

		log << TestLog::Message << "EGL_KHR_get_all_proc_addresses: " << (funcPtrSupported ? "supported" : "not supported") << TestLog::EndMessage;
		log << TestLog::Message << TestLog::EndMessage;

		if (!apiSupported)
		{
			log << TestLog::Message << eglu::getConfigAttribValueStr(EGL_RENDERABLE_TYPE, m_apiBit) << " not supported by any available configuration." << TestLog::EndMessage;
			log << TestLog::Message << TestLog::EndMessage;
		}

		for (int funcNdx = 0; funcNdx < funcNames.numFunctions; funcNdx++)
		{
			const char*	funcName			= funcNames.functions[funcNdx];
			void		(*funcPtr)(void);

			funcPtr = eglGetProcAddress(funcName);
			eglu::checkError(eglGetError(), "eglGetProcAddress()", __FILE__, __LINE__);

			if (apiSupported && funcPtrSupported && (funcPtr == 0))
			{
				log << TestLog::Message << "Fail, received null pointer for supported function: " << funcName << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected null pointer");
			}
			else if (!apiSupported && (funcPtr != 0))
			{
				log << TestLog::Message << "Warning, received non-null value for unsupported function: " << funcName << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Non-null value for unsupported function");
			}
		}
	}

private:
	const EGLint	m_apiBit;
};

GetProcAddressTests::GetProcAddressTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "get_proc_address", "eglGetProcAddress() tests")
{
}

GetProcAddressTests::~GetProcAddressTests (void)
{
}

void GetProcAddressTests::init (void)
{
	// extensions
	{
		tcu::TestCaseGroup* extensionsGroup = new tcu::TestCaseGroup(m_testCtx, "extension", "Test EGL extensions");
		addChild(extensionsGroup);

		for (int extNdx = 0; extNdx < DE_LENGTH_OF_ARRAY(s_extensions); extNdx++)
		{
			const std::string&		extName		= s_extensions[extNdx].name;
			std::string				testName	(extName);

			for (size_t ndx = 0; ndx < extName.length(); ndx++)
				testName[ndx] = de::toLower(extName[ndx]);

			extensionsGroup->addChild(new GetProcAddressExtensionCase(m_eglTestCtx, testName.c_str(), ("Test " + extName).c_str(), extName));
		}
	}

	// core functions
	{
		tcu::TestCaseGroup* coreFuncGroup = new tcu::TestCaseGroup(m_testCtx, "core", "Test core functions");
		addChild(coreFuncGroup);

		coreFuncGroup->addChild(new GetProcAddressCoreFunctionsCase	(m_eglTestCtx,	"egl",		"Test EGL core functions",			0));
		coreFuncGroup->addChild(new GetProcAddressCoreFunctionsCase	(m_eglTestCtx,	"gles",		"Test OpenGL ES core functions",	EGL_OPENGL_ES_BIT));
		coreFuncGroup->addChild(new GetProcAddressCoreFunctionsCase	(m_eglTestCtx,	"gles2",	"Test OpenGL ES 2 core functions",	EGL_OPENGL_ES2_BIT));
		coreFuncGroup->addChild(new GetProcAddressCoreFunctionsCase	(m_eglTestCtx,	"gles3",	"Test OpenGL ES 3 core functions",	EGL_OPENGL_ES3_BIT_KHR));
	}
}

} // egl
} // deqp
