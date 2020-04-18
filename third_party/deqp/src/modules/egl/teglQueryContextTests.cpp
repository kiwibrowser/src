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
 * \brief Config query tests.
 *//*--------------------------------------------------------------------*/

#include "teglQueryContextTests.hpp"
#include "teglRenderCase.hpp"
#include "teglRenderCase.hpp"
#include "egluCallLogWrapper.hpp"
#include "egluStrUtil.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include "tcuTestContext.hpp"

#include "egluUtil.hpp"
#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "deUniquePtr.hpp"
#include "deSTLUtil.hpp"

#include <vector>

namespace deqp
{
namespace egl
{

using std::vector;
using eglu::ConfigInfo;
using tcu::TestLog;
using namespace eglw;

static EGLint getClientTypeFromAPIBit (EGLint apiBit)
{
	switch (apiBit)
	{
		case EGL_OPENGL_BIT:		return EGL_OPENGL_API;
		case EGL_OPENGL_ES_BIT:		return EGL_OPENGL_ES_API;
		case EGL_OPENGL_ES2_BIT:	return EGL_OPENGL_ES_API;
		case EGL_OPENGL_ES3_BIT:	return EGL_OPENGL_ES_API;
		case EGL_OPENVG_BIT:		return EGL_OPENVG_API;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

static EGLint getMinClientMajorVersion (EGLint apiBit)
{
	switch (apiBit)
	{
		case EGL_OPENGL_BIT:		return 1;
		case EGL_OPENGL_ES_BIT:		return 1;
		case EGL_OPENGL_ES2_BIT:	return 2;
		case EGL_OPENGL_ES3_BIT:	return 3;
		case EGL_OPENVG_BIT:		return 1;
		default:
			DE_ASSERT(false);
			return 0;
	}
}

class GetCurrentContextCase : public SingleContextRenderCase, private eglu::CallLogWrapper
{
public:
	GetCurrentContextCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters, EGLint surfaceTypeMask)
		: SingleContextRenderCase	(eglTestCtx, name, description, getBuildClientAPIMask(), surfaceTypeMask, filters)
		, eglu::CallLogWrapper		(eglTestCtx.getLibrary(), m_testCtx.getLog())
	{
	}

	void executeForContext (EGLDisplay display, EGLContext context, EGLSurface surface, const Config& config)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		TestLog&		log	= m_testCtx.getLog();

		DE_UNREF(display);
		DE_UNREF(surface);
		DE_UNREF(config);

		enableLogging(true);

		const EGLContext	gotContext	= eglGetCurrentContext();
		EGLU_CHECK_MSG(egl, "eglGetCurrentContext");

		if (gotContext == context)
		{
			log << TestLog::Message << "  Pass" << TestLog::EndMessage;
		}
		else if (gotContext == EGL_NO_CONTEXT)
		{
			log << TestLog::Message << "  Fail, got EGL_NO_CONTEXT" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected EGL_NO_CONTEXT");
		}
		else if (gotContext != context)
		{
			log << TestLog::Message << "  Fail, call returned the wrong context. Expected: " << tcu::toHex(context) << ", got: " << tcu::toHex(gotContext) << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid context");
		}

		enableLogging(false);
	}
};

class GetCurrentSurfaceCase : public SingleContextRenderCase, private eglu::CallLogWrapper
{
public:
	GetCurrentSurfaceCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters, EGLint surfaceTypeMask)
		: SingleContextRenderCase	(eglTestCtx, name, description, getBuildClientAPIMask(), surfaceTypeMask, filters)
		, eglu::CallLogWrapper		(eglTestCtx.getLibrary(), m_testCtx.getLog())
	{
	}

	void executeForContext (EGLDisplay display, EGLContext context, EGLSurface surface, const Config& config)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		TestLog&		log	= m_testCtx.getLog();

		DE_UNREF(display);
		DE_UNREF(context);
		DE_UNREF(config);

		enableLogging(true);

		const EGLContext	gotReadSurface	= eglGetCurrentSurface(EGL_READ);
		EGLU_CHECK_MSG(egl, "eglGetCurrentSurface(EGL_READ)");

		const EGLContext	gotDrawSurface	= eglGetCurrentSurface(EGL_DRAW);
		EGLU_CHECK_MSG(egl, "eglGetCurrentSurface(EGL_DRAW)");

		if (gotReadSurface == surface && gotDrawSurface == surface)
		{
			log << TestLog::Message << "  Pass" << TestLog::EndMessage;
		}
		else
		{
			log << TestLog::Message << "  Fail, read surface: " << tcu::toHex(gotReadSurface)
									<< ", draw surface: " << tcu::toHex(gotDrawSurface)
									<< ", expected: " << tcu::toHex(surface) << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid surface");
		}

		enableLogging(false);
	}
};

class GetCurrentDisplayCase : public SingleContextRenderCase, private eglu::CallLogWrapper
{
public:
	GetCurrentDisplayCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters, EGLint surfaceTypeMask)
		: SingleContextRenderCase	(eglTestCtx, name, description, getBuildClientAPIMask(), surfaceTypeMask, filters)
		, eglu::CallLogWrapper		(eglTestCtx.getLibrary(), m_testCtx.getLog())
	{
	}

	void executeForContext (EGLDisplay display, EGLContext context, EGLSurface surface, const Config& config)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		TestLog&		log	= m_testCtx.getLog();

		DE_UNREF(surface && context);
		DE_UNREF(config);

		enableLogging(true);

		const EGLDisplay	gotDisplay	= eglGetCurrentDisplay();
		EGLU_CHECK_MSG(egl, "eglGetCurrentDisplay");

		if (gotDisplay == display)
		{
			log << TestLog::Message << "  Pass" << TestLog::EndMessage;
		}
		else if (gotDisplay == EGL_NO_DISPLAY)
		{
			log << TestLog::Message << "  Fail, got EGL_NO_DISPLAY" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected EGL_NO_DISPLAY");
		}
		else if (gotDisplay != display)
		{
			log << TestLog::Message << "  Fail, call returned the wrong display. Expected: " << tcu::toHex(display) << ", got: " << tcu::toHex(gotDisplay) << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid display");
		}

		enableLogging(false);
	}
};

class QueryContextCase : public SingleContextRenderCase, private eglu::CallLogWrapper
{
public:
	QueryContextCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters, EGLint surfaceTypeMask)
		: SingleContextRenderCase	(eglTestCtx, name, description, getBuildClientAPIMask(), surfaceTypeMask, filters)
		, eglu::CallLogWrapper		(eglTestCtx.getLibrary(), m_testCtx.getLog())
	{
	}

	EGLint getContextAttrib (EGLDisplay display, EGLContext context, EGLint attrib)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		EGLint			value;
		EGLU_CHECK_CALL(egl, queryContext(display, context, attrib, &value));

		return value;
	}

	void executeForContext (EGLDisplay display, EGLContext context, EGLSurface surface, const Config& config)
	{
		const Library&		egl		= m_eglTestCtx.getLibrary();
		TestLog&			log		= m_testCtx.getLog();
		const eglu::Version	version	= eglu::getVersion(egl, display);

		DE_UNREF(surface);
		enableLogging(true);

		// Config ID
		{
			const EGLint	configID		= getContextAttrib(display, context, EGL_CONFIG_ID);
			const EGLint	surfaceConfigID	= eglu::getConfigAttribInt(egl, display, config.config, EGL_CONFIG_ID);

			if (configID != surfaceConfigID)
			{
				log << TestLog::Message << "  Fail, config ID doesn't match the one used to create the context." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid config ID");
			}
		}

		// Client API type
		if (version >= eglu::Version(1, 2))
		{
			const EGLint	clientType		= getContextAttrib(display, context, EGL_CONTEXT_CLIENT_TYPE);

			if (clientType != getClientTypeFromAPIBit(config.apiBits))
			{
				log << TestLog::Message << "  Fail, client API type doesn't match." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid client API type");
			}
		}

		// Client API version
		if (version >= eglu::Version(1, 3))
		{
			const EGLint	clientVersion	= getContextAttrib(display, context, EGL_CONTEXT_CLIENT_VERSION);

			// \todo [2014-10-21 mika] Query actual supported api version from client api to make this check stricter.
			if (clientVersion < getMinClientMajorVersion(config.apiBits))
			{
				log << TestLog::Message << "  Fail, client API version doesn't match." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid client API version");
			}
		}

		// Render buffer
		if (version >= eglu::Version(1, 2))
		{
			const EGLint	renderBuffer	= getContextAttrib(display, context, EGL_RENDER_BUFFER);

			if (config.surfaceTypeBit == EGL_PIXMAP_BIT && renderBuffer != EGL_SINGLE_BUFFER)
			{
				log << TestLog::Message << "  Fail, render buffer should be EGL_SINGLE_BUFFER for a pixmap surface." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid render buffer");
			}
			else if (config.surfaceTypeBit == EGL_PBUFFER_BIT && renderBuffer != EGL_BACK_BUFFER)
			{
				log << TestLog::Message << "  Fail, render buffer should be EGL_BACK_BUFFER for a pbuffer surface." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid render buffer");
			}
			else if (config.surfaceTypeBit == EGL_WINDOW_BIT && renderBuffer != EGL_SINGLE_BUFFER && renderBuffer != EGL_BACK_BUFFER)
			{
				log << TestLog::Message << "  Fail, render buffer should be either EGL_SINGLE_BUFFER or EGL_BACK_BUFFER for a window surface." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid render buffer");
			}
		}

		enableLogging(false);

		log << TestLog::Message << "  Pass" << TestLog::EndMessage;
	}
};

class QueryAPICase : public TestCase, private eglu::CallLogWrapper
{
public:
	QueryAPICase (EglTestContext& eglTestCtx, const char* name, const char* description)
		: TestCase		(eglTestCtx, name, description)
		, CallLogWrapper(eglTestCtx.getLibrary(), eglTestCtx.getTestContext().getLog())
	{
	}

	void init (void)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	IterateResult iterate (void)
	{
		const Library&			egl				= m_eglTestCtx.getLibrary();
		EGLDisplay				display			= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
		tcu::TestLog&			log				= m_testCtx.getLog();
		const EGLenum			apis[]			= { EGL_OPENGL_API, EGL_OPENGL_ES_API, EGL_OPENVG_API };
		const vector<EGLenum>	supportedAPIs	= eglu::getClientAPIs(egl, display);

		enableLogging(true);

		{
			const EGLenum	api	= eglQueryAPI();

			if (api != EGL_OPENGL_ES_API && (de::contains(supportedAPIs.begin(), supportedAPIs.end(), EGL_OPENGL_ES_API)))
			{
				log << TestLog::Message << "  Fail, initial value should be EGL_OPENGL_ES_API if OpenGL ES is supported." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid default value");
			}
			else if (api != EGL_NONE && !(de::contains(supportedAPIs.begin(), supportedAPIs.end(), EGL_OPENGL_ES_API)))
			{
				log << TestLog::Message << "  Fail, initial value should be EGL_NONE if OpenGL ES is not supported." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid default value");
			}
		}

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(apis); ndx++)
		{
			const EGLenum	api	= apis[ndx];

			log << TestLog::Message << TestLog::EndMessage;

			if (de::contains(supportedAPIs.begin(), supportedAPIs.end(), api))
			{
				egl.bindAPI(api);

				if (api != egl.queryAPI())
				{
					log << TestLog::Message << "  Fail, return value does not match previously bound API." << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid return value");
				}
			}
			else
			{
				log << TestLog::Message << eglu::getAPIStr(api) << " not supported." << TestLog::EndMessage;
			}
		}

		enableLogging(false);
		eglTerminate(display);
		return STOP;
	}
};

QueryContextTests::QueryContextTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "query_context", "Rendering context query tests")
{
}

QueryContextTests::~QueryContextTests (void)
{
}

template<class QueryContextClass>
void createQueryContextGroups (EglTestContext& eglTestCtx, tcu::TestCaseGroup* group)
{
	std::vector<RenderFilterList> filterLists;

	getDefaultRenderFilterLists(filterLists, eglu::FilterList());

	for (std::vector<RenderFilterList>::const_iterator listIter = filterLists.begin(); listIter != filterLists.end(); listIter++)
		group->addChild(new QueryContextClass(eglTestCtx, listIter->getName(), "", *listIter, listIter->getSurfaceTypeMask()));
}

void QueryContextTests::init (void)
{
	{
		tcu::TestCaseGroup* simpleGroup = new tcu::TestCaseGroup(m_testCtx, "simple", "Simple API tests");
		addChild(simpleGroup);

		simpleGroup->addChild(new QueryAPICase(m_eglTestCtx, "query_api", "eglQueryAPI() test"));
	}

	// eglGetCurrentContext
	{
		tcu::TestCaseGroup* getCurrentContextGroup = new tcu::TestCaseGroup(m_testCtx, "get_current_context", "eglGetCurrentContext() tests");
		addChild(getCurrentContextGroup);

		createQueryContextGroups<GetCurrentContextCase>(m_eglTestCtx, getCurrentContextGroup);
	}

	// eglGetCurrentSurface
	{
		tcu::TestCaseGroup* getCurrentSurfaceGroup = new tcu::TestCaseGroup(m_testCtx, "get_current_surface", "eglGetCurrentSurface() tests");
		addChild(getCurrentSurfaceGroup);

		createQueryContextGroups<GetCurrentSurfaceCase>(m_eglTestCtx, getCurrentSurfaceGroup);
	}

	// eglGetCurrentDisplay
	{
		tcu::TestCaseGroup* getCurrentDisplayGroup = new tcu::TestCaseGroup(m_testCtx, "get_current_display", "eglGetCurrentDisplay() tests");
		addChild(getCurrentDisplayGroup);

		createQueryContextGroups<GetCurrentDisplayCase>(m_eglTestCtx, getCurrentDisplayGroup);
	}

	// eglQueryContext
	{
		tcu::TestCaseGroup* queryContextGroup = new tcu::TestCaseGroup(m_testCtx, "query_context", "eglQueryContext() tests");
		addChild(queryContextGroup);

		createQueryContextGroups<QueryContextCase>(m_eglTestCtx, queryContextGroup);
	}
}

} // egl
} // deqp
