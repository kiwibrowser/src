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
 * \brief Base class for rendering tests.
 *//*--------------------------------------------------------------------*/

#include "teglRenderCase.hpp"

#include "teglSimpleConfigCase.hpp"

#include "egluNativeDisplay.hpp"
#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

namespace deqp
{
namespace egl
{

using std::string;
using std::vector;
using std::set;
using tcu::TestLog;
using namespace eglw;

static void postSurface (const Library& egl, EGLDisplay display, EGLSurface surface, EGLint typeBit)
{
	if (typeBit == EGL_WINDOW_BIT)
		EGLU_CHECK_CALL(egl, swapBuffers(display, surface));
	else if (typeBit == EGL_PIXMAP_BIT)
		EGLU_CHECK_CALL(egl, waitClient());
	else if (typeBit == EGL_PBUFFER_BIT)
		EGLU_CHECK_CALL(egl, waitClient());
	else
		DE_ASSERT(false);
}

// RenderCase

RenderCase::RenderCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint surfaceTypeMask, const eglu::FilterList& filters)
	: SimpleConfigCase	(eglTestCtx, name, description, filters)
	, m_surfaceTypeMask	(surfaceTypeMask)
{
}

RenderCase::~RenderCase (void)
{
}

EGLint getBuildClientAPIMask (void)
{
	EGLint apiMask = 0;

	// Always supported regardless of flags - dynamically loaded
	apiMask |= EGL_OPENGL_ES2_BIT;
	apiMask |= EGL_OPENGL_ES3_BIT;
	apiMask |= EGL_OPENGL_BIT;

#if defined(DEQP_SUPPORT_GLES1)
	apiMask |= EGL_OPENGL_ES_BIT;
#endif

#if defined(DEQP_SUPPORT_VG)
	apiMask |= EGL_OPENVG_BIT;
#endif

	return apiMask;
}

static void checkBuildClientAPISupport (EGLint requiredAPIs)
{
	const EGLint	builtClientAPIs		= getBuildClientAPIMask();

	if ((requiredAPIs & builtClientAPIs) != requiredAPIs)
		TCU_THROW(InternalError, "Test case requires client API not supported in current build");
}

void RenderCase::executeForConfig (EGLDisplay display, EGLConfig config)
{
	const Library&						egl				= m_eglTestCtx.getLibrary();
	tcu::TestLog&						log				= m_testCtx.getLog();
	const int							width			= 128;
	const int							height			= 128;
	const EGLint						configId		= eglu::getConfigID(egl, display, config);

	const eglu::NativeDisplayFactory&	displayFactory	= m_eglTestCtx.getNativeDisplayFactory();
	eglu::NativeDisplay&				nativeDisplay	= m_eglTestCtx.getNativeDisplay();

	bool								isOk			= true;
	string								failReason		= "";

	if (m_surfaceTypeMask & EGL_WINDOW_BIT)
	{
		tcu::ScopedLogSection(log,
							  string("Config") + de::toString(configId) + "-Window",
							  string("Config ID ") + de::toString(configId) + ", window surface");

		const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(displayFactory, m_testCtx.getCommandLine());

		try
		{
			const eglu::WindowParams			params		(width, height, eglu::parseWindowVisibility(m_testCtx.getCommandLine()));
			de::UniquePtr<eglu::NativeWindow>	window		(windowFactory.createWindow(&nativeDisplay, display, config, DE_NULL, params));
			EGLSurface							eglSurface	= createWindowSurface(nativeDisplay, *window, display, config, DE_NULL);
			eglu::UniqueSurface					surface		(egl, display, eglSurface);

			executeForSurface(display, *surface, Config(config, EGL_WINDOW_BIT, 0));
		}
		catch (const tcu::TestError& e)
		{
			log << e;
			isOk = false;
			failReason = e.what();
		}
	}

	if (m_surfaceTypeMask & EGL_PIXMAP_BIT)
	{
		tcu::ScopedLogSection(log,
							  string("Config") + de::toString(configId) + "-Pixmap",
							  string("Config ID ") + de::toString(configId) + ", pixmap surface");

		const eglu::NativePixmapFactory&	pixmapFactory	= eglu::selectNativePixmapFactory(displayFactory, m_testCtx.getCommandLine());

		try
		{
			de::UniquePtr<eglu::NativePixmap>	pixmap		(pixmapFactory.createPixmap(&nativeDisplay, display, config, DE_NULL, width, height));
			EGLSurface							eglSurface	= createPixmapSurface(nativeDisplay, *pixmap, display, config, DE_NULL);
			eglu::UniqueSurface					surface		(egl, display, eglSurface);

			executeForSurface(display, *surface, Config(config, EGL_PIXMAP_BIT, 0));
		}
		catch (const tcu::TestError& e)
		{
			log << e;
			isOk = false;
			failReason = e.what();
		}
	}

	if (m_surfaceTypeMask & EGL_PBUFFER_BIT)
	{
		tcu::ScopedLogSection(log,
							  string("Config") + de::toString(configId) + "-Pbuffer",
							  string("Config ID ") + de::toString(configId) + ", pbuffer surface");
		try
		{
			const EGLint surfaceAttribs[] =
			{
				EGL_WIDTH,	width,
				EGL_HEIGHT,	height,
				EGL_NONE
			};

			eglu::UniqueSurface surface(egl, display, egl.createPbufferSurface(display, config, surfaceAttribs));
			EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

			executeForSurface(display, *surface, Config(config, EGL_PBUFFER_BIT, 0));
		}
		catch (const tcu::TestError& e)
		{
			log << e;
			isOk = false;
			failReason = e.what();
		}
	}

	if (!isOk && m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, failReason.c_str());
}

// SingleContextRenderCase

SingleContextRenderCase::SingleContextRenderCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint apiMask, EGLint surfaceTypeMask, const eglu::FilterList& filters)
	: RenderCase	(eglTestCtx, name, description, surfaceTypeMask, filters)
	, m_apiMask		(apiMask)
{
}

SingleContextRenderCase::~SingleContextRenderCase (void)
{
}

void SingleContextRenderCase::executeForSurface (EGLDisplay display, EGLSurface surface, const Config& config)
{
	const Library&		egl				= m_eglTestCtx.getLibrary();
	const EGLint		apis[]			= { EGL_OPENGL_ES2_BIT, EGL_OPENGL_ES3_BIT_KHR, EGL_OPENGL_ES_BIT, EGL_OPENVG_BIT };
	tcu::TestLog&		log				= m_testCtx.getLog();
	const EGLint		configApiMask	= eglu::getConfigAttribInt(egl, display, config.config, EGL_RENDERABLE_TYPE);

	checkBuildClientAPISupport(m_apiMask);

	for (int apiNdx = 0; apiNdx < DE_LENGTH_OF_ARRAY(apis); apiNdx++)
	{
		EGLint apiBit = apis[apiNdx];

		// Skip API if build or current config doesn't support it.
		if ((apiBit & m_apiMask) == 0 || (apiBit & configApiMask) == 0)
			continue;

		EGLint			api		= EGL_NONE;
		const char*		apiName	= DE_NULL;
		vector<EGLint>	contextAttribs;

		// Select api enum and build context attributes.
		switch (apiBit)
		{
			case EGL_OPENGL_ES2_BIT:
				api		= EGL_OPENGL_ES_API;
				apiName	= "OpenGL ES 2.x";
				contextAttribs.push_back(EGL_CONTEXT_CLIENT_VERSION);
				contextAttribs.push_back(2);
				break;

			case EGL_OPENGL_ES3_BIT_KHR:
				api		= EGL_OPENGL_ES_API;
				apiName	= "OpenGL ES 3.x";
				contextAttribs.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
				contextAttribs.push_back(3);
				break;

			case EGL_OPENGL_ES_BIT:
				api		= EGL_OPENGL_ES_API;
				apiName	= "OpenGL ES 1.x";
				contextAttribs.push_back(EGL_CONTEXT_CLIENT_VERSION);
				contextAttribs.push_back(1);
				break;

			case EGL_OPENVG_BIT:
				api		= EGL_OPENVG_API;
				apiName	= "OpenVG";
				break;

			default:
				DE_ASSERT(DE_FALSE);
		}

		contextAttribs.push_back(EGL_NONE);

		log << TestLog::Message << apiName << TestLog::EndMessage;

		EGLU_CHECK_CALL(egl, bindAPI(api));

		eglu::UniqueContext	context	(egl, display, egl.createContext(display, config.config, EGL_NO_CONTEXT, &contextAttribs[0]));

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, *context));
		executeForContext(display, *context, surface, Config(config.config, config.surfaceTypeBit, apiBit));

		// Call SwapBuffers() / WaitClient() to finish rendering
		postSurface(egl, display, surface, config.surfaceTypeBit);
	}

	EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

// MultiContextRenderCase

MultiContextRenderCase::MultiContextRenderCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi)
	: RenderCase			(eglTestCtx, name, description, surfaceType, filters)
	, m_numContextsPerApi	(numContextsPerApi)
	, m_apiMask				(api)
{
}

MultiContextRenderCase::~MultiContextRenderCase (void)
{
}

void MultiContextRenderCase::executeForSurface (EGLDisplay display, EGLSurface surface, const Config& config)
{
	const Library&							egl				= m_eglTestCtx.getLibrary();
	const EGLint							configApiMask	= eglu::getConfigAttribInt(egl, display, config.config, EGL_RENDERABLE_TYPE);
	vector<std::pair<EGLint, EGLContext> >	contexts;
	contexts.reserve(3*m_numContextsPerApi); // 3 types of contexts at maximum.

	checkBuildClientAPISupport(m_apiMask);

	// ConfigFilter should make sure that config always supports all of the APIs.
	TCU_CHECK_INTERNAL((configApiMask & m_apiMask) == m_apiMask);

	try
	{
		// Create contexts that will participate in rendering.
		for (int ndx = 0; ndx < m_numContextsPerApi; ndx++)
		{
			if (m_apiMask & EGL_OPENGL_ES2_BIT)
			{
				static const EGLint attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
				EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
				contexts.push_back(std::make_pair(EGL_OPENGL_ES2_BIT, egl.createContext(display, config.config, EGL_NO_CONTEXT, &attribs[0])));
			}

			if (m_apiMask & EGL_OPENGL_ES3_BIT_KHR)
			{
				static const EGLint attribs[] = { EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_NONE };
				EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
				contexts.push_back(std::make_pair(EGL_OPENGL_ES3_BIT_KHR, egl.createContext(display, config.config, EGL_NO_CONTEXT, &attribs[0])));
			}

			if (m_apiMask & EGL_OPENGL_ES_BIT)
			{
				static const EGLint attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE };
				EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
				contexts.push_back(std::make_pair(EGL_OPENGL_ES_BIT, egl.createContext(display, config.config, EGL_NO_CONTEXT, &attribs[0])));
			}

			if (m_apiMask & EGL_OPENVG_BIT)
			{
				static const EGLint attribs[] = { EGL_NONE };
				EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENVG_API));
				contexts.push_back(std::make_pair(EGL_OPENVG_BIT, egl.createContext(display, config.config, EGL_NO_CONTEXT, &attribs[0])));
			}
		}

		EGLU_CHECK_MSG(egl, "eglCreateContext()");

		// Execute for contexts.
		executeForContexts(display, surface, Config(config.config, config.surfaceTypeBit, m_apiMask), contexts);

		EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	}
	catch (...)
	{
		// Make sure all contexts have been destroyed.
		for (vector<std::pair<EGLint, EGLContext> >::iterator i = contexts.begin(); i != contexts.end(); i++)
			egl.destroyContext(display, i->second);
		throw;
	}

	// Destroy contexts.
	for (vector<std::pair<EGLint, EGLContext> >::iterator i = contexts.begin(); i != contexts.end(); i++)
		egl.destroyContext(display, i->second);
}

// Utilities

template <int Red, int Green, int Blue, int Alpha>
static bool colorBits (const eglu::CandidateConfig& c)
{
	return c.redSize()		== Red		&&
		   c.greenSize()	== Green	&&
		   c.blueSize()		== Blue		&&
		   c.alphaSize()	== Alpha;
}

template <int Red, int Green, int Blue, int Alpha>
static bool notColorBits (const eglu::CandidateConfig& c)
{
	return c.redSize()		!= Red		||
		   c.greenSize()	!= Green	||
		   c.blueSize()		!= Blue		||
		   c.alphaSize()	!= Alpha;
}

template <deUint32 Type>
static bool surfaceType (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & Type) == Type;
}

static bool isConformant (const eglu::CandidateConfig& c)
{
	return c.get(EGL_CONFIG_CAVEAT) != EGL_NON_CONFORMANT_CONFIG;
}

static bool notFloat (const eglu::CandidateConfig& c)
{
	return c.colorComponentType() != EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT;
}

void getDefaultRenderFilterLists (vector<RenderFilterList>& filterLists, const eglu::FilterList& baseFilters)
{
	static const struct
	{
		const char*			name;
		eglu::ConfigFilter	filter;
	} s_colorRules[] =
	{
		{ "rgb565",		colorBits<5, 6, 5, 0>	},
		{ "rgb888",		colorBits<8, 8, 8, 0>	},
		{ "rgba4444",	colorBits<4, 4, 4, 4>	},
		{ "rgba5551",	colorBits<5, 5, 5, 1>	},
		{ "rgba8888",	colorBits<8, 8, 8, 8>	},
	};

	static const struct
	{
		const char*			name;
		EGLint				bits;
		eglu::ConfigFilter	filter;
	} s_surfaceRules[] =
	{
		{ "window",		EGL_WINDOW_BIT,		surfaceType<EGL_WINDOW_BIT>		},
		{ "pixmap",		EGL_PIXMAP_BIT,		surfaceType<EGL_PIXMAP_BIT>,	},
		{ "pbuffer",	EGL_PBUFFER_BIT,	surfaceType<EGL_PBUFFER_BIT>	}
	};

	for (int colorNdx = 0; colorNdx < DE_LENGTH_OF_ARRAY(s_colorRules); colorNdx++)
	{
		for (int surfaceNdx = 0; surfaceNdx < DE_LENGTH_OF_ARRAY(s_surfaceRules); surfaceNdx++)
		{
			const string		name	= string(s_colorRules[colorNdx].name) + "_" + s_surfaceRules[surfaceNdx].name;
			RenderFilterList	filters	(name.c_str(), "", s_surfaceRules[surfaceNdx].bits);

			filters << baseFilters
					<< s_colorRules[colorNdx].filter
					<< s_surfaceRules[surfaceNdx].filter
					<< isConformant;

			filterLists.push_back(filters);
		}
	}

	// Add other config ids to "other" set
	{
		RenderFilterList	filters	("other", "", EGL_WINDOW_BIT|EGL_PIXMAP_BIT|EGL_PBUFFER_BIT);

		filters << baseFilters
				<< notColorBits<5, 6, 5, 0>
				<< notColorBits<8, 8, 8, 0>
				<< notColorBits<4, 4, 4, 4>
				<< notColorBits<5, 5, 5, 1>
				<< notColorBits<8, 8, 8, 8>
				<< isConformant
				<< notFloat;

		filterLists.push_back(filters);
	}
}

} // egl
} // deqp
