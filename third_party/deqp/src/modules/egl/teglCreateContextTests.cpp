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
 * \brief Simple Context construction test.
 *//*--------------------------------------------------------------------*/

#include "teglCreateContextTests.hpp"
#include "teglSimpleConfigCase.hpp"
#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "tcuTestLog.hpp"
#include "deSTLUtil.hpp"

namespace deqp
{
namespace egl
{

using std::vector;
using tcu::TestLog;
using namespace eglw;

static const EGLint s_es1Attrs[] = { EGL_CONTEXT_CLIENT_VERSION,	1, EGL_NONE };
static const EGLint s_es2Attrs[] = { EGL_CONTEXT_CLIENT_VERSION,	2, EGL_NONE };
static const EGLint s_es3Attrs[] = { EGL_CONTEXT_MAJOR_VERSION_KHR,	3, EGL_NONE };

static const struct
{
	const char*		name;
	EGLenum			api;
	EGLint			apiBit;
	const EGLint*	ctxAttrs;
} s_apis[] =
{
	{ "OpenGL",			EGL_OPENGL_API,		EGL_OPENGL_BIT,			DE_NULL		},
	{ "OpenGL ES 1",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES_BIT,		s_es1Attrs	},
	{ "OpenGL ES 2",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES2_BIT,		s_es2Attrs	},
	{ "OpenGL ES 3",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES3_BIT_KHR,	s_es3Attrs	},
	{ "OpenVG",			EGL_OPENVG_API,		EGL_OPENVG_BIT,			DE_NULL		}
};

class CreateContextCase : public SimpleConfigCase
{
public:
						CreateContextCase			(EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters);
						~CreateContextCase			(void);

	void				executeForConfig			(EGLDisplay display, EGLConfig config);
};

CreateContextCase::CreateContextCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters)
	: SimpleConfigCase(eglTestCtx, name, description, filters)
{
}

CreateContextCase::~CreateContextCase (void)
{
}

void CreateContextCase::executeForConfig (EGLDisplay display, EGLConfig config)
{
	const Library&	egl		= m_eglTestCtx.getLibrary();
	TestLog&		log		= m_testCtx.getLog();
	EGLint			id		= eglu::getConfigAttribInt(egl, display, config, EGL_CONFIG_ID);
	EGLint			apiBits	= eglu::getConfigAttribInt(egl, display, config, EGL_RENDERABLE_TYPE);

	for (int apiNdx = 0; apiNdx < (int)DE_LENGTH_OF_ARRAY(s_apis); apiNdx++)
	{
		if ((apiBits & s_apis[apiNdx].apiBit) == 0)
			continue; // Not supported API

		log << TestLog::Message << "Creating " << s_apis[apiNdx].name << " context with config ID " << id << TestLog::EndMessage;
		EGLU_CHECK_MSG(egl, "init");

		EGLU_CHECK_CALL(egl, bindAPI(s_apis[apiNdx].api));

		EGLContext	context = egl.createContext(display, config, EGL_NO_CONTEXT, s_apis[apiNdx].ctxAttrs);
		EGLenum		err		= egl.getError();

		if (context == EGL_NO_CONTEXT || err != EGL_SUCCESS)
		{
			log << TestLog::Message << "  Fail, context: " << tcu::toHex(context) << ", error: " << eglu::getErrorName(err) << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to create context");
		}
		else
		{
			// Destroy
			EGLU_CHECK_CALL(egl, destroyContext(display, context));
			log << TestLog::Message << "  Pass" << TestLog::EndMessage;
		}
	}
}

class CreateContextNoConfigCase : public TestCase
{
public:
	CreateContextNoConfigCase (EglTestContext& eglTestCtx)
		: TestCase(eglTestCtx, "no_config", "EGL_KHR_no_config_context")
	{
	}

	IterateResult iterate (void)
	{
		const eglw::Library&		egl		= m_eglTestCtx.getLibrary();
		const eglu::UniqueDisplay	display	(egl, eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay(), DE_NULL));
		tcu::TestLog&				log		= m_testCtx.getLog();

		if (!eglu::hasExtension(egl, *display, "EGL_KHR_no_config_context"))
			TCU_THROW(NotSupportedError, "EGL_KHR_no_config_context is not supported");

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "pass");

		for (int apiNdx = 0; apiNdx < (int)DE_LENGTH_OF_ARRAY(s_apis); apiNdx++)
		{
			const EGLenum	api		= s_apis[apiNdx].api;

			if (egl.bindAPI(api) == EGL_FALSE)
			{
				TCU_CHECK(egl.getError() == EGL_BAD_PARAMETER);
				log << TestLog::Message << "eglBindAPI(" << eglu::getAPIStr(api) << ") failed, skipping" << TestLog::EndMessage;
				continue;
			}

			log << TestLog::Message << "Creating " << s_apis[apiNdx].name << " context" << TestLog::EndMessage;

			const EGLContext	context = egl.createContext(*display, (EGLConfig)0, EGL_NO_CONTEXT, s_apis[apiNdx].ctxAttrs);
			const EGLenum		err		= egl.getError();

			if (context == EGL_NO_CONTEXT || err != EGL_SUCCESS)
			{
				log << TestLog::Message << "  Fail, context: " << tcu::toHex(context) << ", error: " << eglu::getErrorName(err) << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to create context");
			}
			else
			{
				// Destroy
				EGLU_CHECK_CALL(egl, destroyContext(*display, context));
				log << TestLog::Message << "  Pass" << TestLog::EndMessage;
			}
		}

		return STOP;
	}
};

CreateContextTests::CreateContextTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "create_context", "Basic eglCreateContext() tests")
{
}

CreateContextTests::~CreateContextTests (void)
{
}

void CreateContextTests::init (void)
{
	vector<NamedFilterList>	filterLists;
	getDefaultFilterLists(filterLists, eglu::FilterList());

	for (vector<NamedFilterList>::iterator i = filterLists.begin(); i != filterLists.end(); i++)
		addChild(new CreateContextCase(m_eglTestCtx, i->getName(), i->getDescription(), *i));

	addChild(new CreateContextNoConfigCase(m_eglTestCtx));
}

} // egl
} // deqp
