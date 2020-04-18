/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief EGL multi context tests
 *//*--------------------------------------------------------------------*/

#include "teglMultiContextTests.hpp"

#include "egluUtil.hpp"
#include "egluUnique.hpp"
#include "egluStrUtil.hpp"
#include "egluConfigFilter.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "tcuResultCollector.hpp"
#include "tcuTestLog.hpp"

#include "deRandom.hpp"

#include <vector>

namespace deqp
{
namespace egl
{
namespace
{

using tcu::TestLog;

class MultiContextTest : public TestCase
{
public:
	enum Use
	{
		USE_NONE = 0,
		USE_MAKECURRENT,
		USE_CLEAR,

		USE_LAST
	};

	enum Sharing
	{
		SHARING_NONE = 0,
		SHARING_SHARED,
		SHARING_LAST
	};
					MultiContextTest	(EglTestContext& eglTestCtx, Sharing sharing, Use use, const char* name, const char* description);

	IterateResult	iterate				(void);

private:
	const Sharing	m_sharing;
	const Use		m_use;
};

MultiContextTest::MultiContextTest (EglTestContext& eglTestCtx, Sharing sharing, Use use, const char* name, const char* description)
	: TestCase	(eglTestCtx, name, description)
	, m_sharing	(sharing)
	, m_use		(use)
{
}

bool isES2Renderable (const eglu::CandidateConfig& c)
{
	return (c.get(EGL_RENDERABLE_TYPE) & EGL_OPENGL_ES2_BIT) == EGL_OPENGL_ES2_BIT;
}

eglw::EGLConfig getConfig (const eglw::Library& egl, eglw::EGLDisplay display)
{
	eglu::FilterList filters;
	filters << isES2Renderable;
	return eglu::chooseSingleConfig(egl, display, filters);
}

tcu::TestCase::IterateResult MultiContextTest::iterate (void)
{
	const deUint32					seed			= m_sharing == SHARING_SHARED ? 2498541716u : 8825414;
	const size_t					maxContextCount	= 128;
	const size_t					minContextCount	= 32;
	const eglw::EGLint				attribList[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	const eglw::EGLint				pbufferAttribList[]	=
	{
		EGL_WIDTH,	64,
		EGL_HEIGHT,	64,
		EGL_NONE
	};

	TestLog&						log				= m_testCtx.getLog();
	tcu::ResultCollector			resultCollector	(log);
	de::Random						rng				(seed);

	const eglw::Library&			egl				= m_eglTestCtx.getLibrary();
	const eglu::UniqueDisplay		display			(egl, eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay()));
	const eglw::EGLConfig			config			= getConfig(egl, *display);

	const eglu::UniqueSurface		surface			(egl, *display, m_use != USE_NONE ? egl.createPbufferSurface(*display, config, pbufferAttribList) : EGL_NO_SURFACE);
	EGLU_CHECK_MSG(egl, "Failed to create pbuffer.");

	std::vector<eglw::EGLContext>	contexts;
	glw::Functions					gl;

	contexts.reserve(maxContextCount);

	log << TestLog::Message << "Trying to create " << maxContextCount << (m_sharing == SHARING_SHARED ? " shared " : " ") << "contexts." << TestLog::EndMessage;
	log << TestLog::Message << "Requiring that at least " << minContextCount << " contexts can be created." << TestLog::EndMessage;

	if (m_use == USE_CLEAR)
		m_eglTestCtx.initGLFunctions(&gl, glu::ApiType::es(2,0));

	EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));

	try
	{
		for (size_t contextCount = 0; contextCount < maxContextCount; contextCount++)
		{
			const eglw::EGLContext	sharedContext	= (m_sharing == SHARING_SHARED && contextCount > 0 ? contexts[rng.getUint32() % (deUint32)contextCount] : EGL_NO_CONTEXT);
			const eglw::EGLContext	context			= egl.createContext(*display, config, sharedContext, attribList);
			const eglw::EGLint		error			= egl.getError();

			if (context == EGL_NO_CONTEXT || error != EGL_SUCCESS)
			{
				log << TestLog::Message << "Got error after creating " << contextCount << " contexts." << TestLog::EndMessage;

				if (error == EGL_BAD_ALLOC)
				{
					if (contextCount < minContextCount)
						resultCollector.fail("Couldn't create the minimum number of contexts required.");
					else
						log << TestLog::Message << "Got EGL_BAD_ALLOC." << TestLog::EndMessage;
				}
				else
					resultCollector.fail("eglCreateContext() produced error that is not EGL_BAD_ALLOC: " + eglu::getErrorStr(error).toString());

				if (context != EGL_NO_CONTEXT)
					resultCollector.fail("eglCreateContext() produced error, but context is not EGL_NO_CONTEXT");

				break;
			}
			else
			{
				contexts.push_back(context);

				if (m_use == USE_MAKECURRENT || m_use == USE_CLEAR)
				{
					const eglw::EGLBoolean	result				= egl.makeCurrent(*display, *surface, *surface, context);
					const eglw::EGLint		makeCurrentError	= egl.getError();

					if (!result || makeCurrentError != EGL_SUCCESS)
					{
						log << TestLog::Message << "Failed to make " << (contextCount + 1) << "th context current: " << eglu::getErrorStr(makeCurrentError) << TestLog::EndMessage;
						resultCollector.fail("Failed to make context current");

						break;
					}
					else if (m_use == USE_CLEAR)
					{
						gl.clearColor(0.25f, 0.75f, 0.50f, 1.00f);
						gl.clear(GL_COLOR_BUFFER_BIT);
						gl.finish();
						GLU_CHECK_GLW_MSG(gl, "Failed to clear color.");
					}
				}
			}
		}

		for (size_t contextNdx = 0; contextNdx < contexts.size(); contextNdx++)
		{
			EGLU_CHECK_CALL(egl, destroyContext(*display, contexts[contextNdx]));
			contexts[contextNdx] = EGL_NO_CONTEXT;
		}

		if (m_use == USE_MAKECURRENT || m_use == USE_CLEAR)
			EGLU_CHECK_CALL(egl, makeCurrent(*display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	}
	catch (...)
	{
		for (size_t contextNdx = 0; contextNdx < contexts.size(); contextNdx++)
		{
			if (contexts[contextNdx] != EGL_NO_CONTEXT)
				EGLU_CHECK_CALL(egl, destroyContext(*display, contexts[contextNdx]));
		}

		if (m_use == USE_MAKECURRENT || m_use == USE_CLEAR)
			EGLU_CHECK_CALL(egl, makeCurrent(*display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

		throw;
	}

	resultCollector.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

TestCaseGroup* createMultiContextTests (EglTestContext& eglTestCtx)
{
	de::MovePtr<TestCaseGroup> group (new TestCaseGroup(eglTestCtx, "multicontext", "EGL multi context tests."));

	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_NONE,	MultiContextTest::USE_NONE,			"non_shared",				"Create multiple non-shared contexts."));
	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_SHARED,	MultiContextTest::USE_NONE,			"shared",					"Create multiple shared contexts."));

	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_NONE,	MultiContextTest::USE_MAKECURRENT,	"non_shared_make_current",	"Create multiple non-shared contexts."));
	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_SHARED,	MultiContextTest::USE_MAKECURRENT,	"shared_make_current",		"Create multiple shared contexts."));

	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_NONE,	MultiContextTest::USE_CLEAR,		"non_shared_clear",			"Create multiple non-shared contexts."));
	group->addChild(new MultiContextTest(eglTestCtx, MultiContextTest::SHARING_SHARED,	MultiContextTest::USE_CLEAR,		"shared_clear",				"Create multiple shared contexts."));

	return group.release();
}

} // egl
} // deqp
