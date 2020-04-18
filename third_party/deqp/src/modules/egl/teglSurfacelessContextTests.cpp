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
 * \brief EGL_KHR_surfaceless_context extension tests
 *//*--------------------------------------------------------------------*/

#include "teglSurfacelessContextTests.hpp"
#include "teglSimpleConfigCase.hpp"

#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"

#include "tcuTestLog.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "deSTLUtil.hpp"

#include <string>
#include <vector>
#include <algorithm>

using std::vector;
using std::string;
using tcu::TestLog;

using namespace eglw;

namespace deqp
{
namespace egl
{
namespace
{

class SurfacelessContextCase : public SimpleConfigCase
{
public:
						SurfacelessContextCase			(EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters);
						~SurfacelessContextCase			(void);

	void				executeForConfig				(EGLDisplay display, EGLConfig config);
};

SurfacelessContextCase::SurfacelessContextCase (EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters)
	: SimpleConfigCase(eglTestCtx, name, description, filters)
{
}

SurfacelessContextCase::~SurfacelessContextCase (void)
{
}

void SurfacelessContextCase::executeForConfig (EGLDisplay display, EGLConfig config)
{
	const Library&	egl		= m_eglTestCtx.getLibrary();
	TestLog&		log		= m_testCtx.getLog();
	const EGLint	id		= eglu::getConfigAttribInt(egl, display, config, EGL_CONFIG_ID);
	const EGLint	apiBits	= eglu::getConfigAttribInt(egl, display, config, EGL_RENDERABLE_TYPE);

	static const EGLint es1Attrs[] = { EGL_CONTEXT_CLIENT_VERSION,		1, EGL_NONE };
	static const EGLint es2Attrs[] = { EGL_CONTEXT_CLIENT_VERSION,		2, EGL_NONE };
	static const EGLint es3Attrs[] = { EGL_CONTEXT_MAJOR_VERSION_KHR,	3, EGL_NONE };

	static const struct
	{
		const char*		name;
		EGLenum			api;
		EGLint			apiBit;
		const EGLint*	ctxAttrs;
	} apis[] =
	{
		{ "OpenGL",			EGL_OPENGL_API,		EGL_OPENGL_BIT,			DE_NULL		},
		{ "OpenGL ES 1",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES_BIT,		es1Attrs	},
		{ "OpenGL ES 2",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES2_BIT,		es2Attrs	},
		{ "OpenGL ES 3",	EGL_OPENGL_ES_API,	EGL_OPENGL_ES3_BIT_KHR,	es3Attrs	},
		{ "OpenVG",			EGL_OPENVG_API,		EGL_OPENVG_BIT,			DE_NULL		}
	};

	if (!eglu::hasExtension(egl, display, "EGL_KHR_surfaceless_context"))
		TCU_THROW(NotSupportedError, "EGL_KHR_surfaceless_context not supported");

	for (int apiNdx = 0; apiNdx < (int)DE_LENGTH_OF_ARRAY(apis); apiNdx++)
	{
		if ((apiBits & apis[apiNdx].apiBit) == 0)
			continue; // Not supported API

		log << TestLog::Message << "Creating " << apis[apiNdx].name << " context with config ID " << id << TestLog::EndMessage;

		EGLU_CHECK_CALL(egl, bindAPI(apis[apiNdx].api));

		eglu::UniqueContext context(egl, display, egl.createContext(display, config, EGL_NO_CONTEXT, apis[apiNdx].ctxAttrs));
		EGLU_CHECK_MSG(egl, "eglCreateContext()");

		if (!egl.makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, *context))
		{
			const EGLenum err = egl.getError();

			if (err == EGL_BAD_MATCH)
			{
				log << TestLog::Message << "  eglMakeCurrent() failed with EGL_BAD_MATCH. Context doesn't support surfaceless mode." << TestLog::EndMessage;
				continue;
			}
			else
			{
				log << TestLog::Message << "  Fail, context: " << tcu::toHex(*context) << ", error: " << eglu::getErrorName(err) << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to make context current");
				continue;
			}
		}

		EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

		log << TestLog::Message << "  Pass" << TestLog::EndMessage;
	}
}



} // anonymous

SurfacelessContextTests::SurfacelessContextTests (EglTestContext& eglTestCtx)
	: TestCaseGroup (eglTestCtx, "surfaceless_context", "EGL_KHR_surfaceless_context extension tests")
{
}

void SurfacelessContextTests::init (void)
{
	vector<NamedFilterList>	filterLists;
	getDefaultFilterLists(filterLists, eglu::FilterList());

	for (vector<NamedFilterList>::const_iterator i = filterLists.begin(); i != filterLists.end(); i++)
		addChild(new SurfacelessContextCase(m_eglTestCtx, i->getName(), i->getDescription(), *i));
}

} // egl
} // deqp
