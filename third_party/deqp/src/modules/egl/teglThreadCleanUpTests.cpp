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
 * \brief EGL thread clean up tests
 *//*--------------------------------------------------------------------*/

#include "teglThreadCleanUpTests.hpp"

#include "egluUtil.hpp"
#include "egluUnique.hpp"
#include "egluConfigFilter.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "tcuMaybe.hpp"
#include "tcuTestLog.hpp"

#include "deThread.hpp"

namespace deqp
{
namespace egl
{
namespace
{

using namespace eglw;
using tcu::TestLog;

bool isES2Renderable (const eglu::CandidateConfig& c)
{
	return (c.get(EGL_RENDERABLE_TYPE) & EGL_OPENGL_ES2_BIT) == EGL_OPENGL_ES2_BIT;
}

bool isPBuffer (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & EGL_PBUFFER_BIT) == EGL_PBUFFER_BIT;
}

class Thread : public de::Thread
{
public:
	Thread (const Library& egl, EGLDisplay display, EGLSurface surface, EGLContext context, EGLConfig config, tcu::Maybe<eglu::Error>& error)
		: m_egl		(egl)
		, m_display	(display)
		, m_surface	(surface)
		, m_context	(context)
		, m_config	(config)
		, m_error	(error)
	{
	}

	void testContext (EGLContext context)
	{
		if (m_surface != EGL_NO_SURFACE)
		{
			EGLU_CHECK_MSG(m_egl, "eglCreateContext");
			m_egl.makeCurrent(m_display, m_surface, m_surface, context);
			EGLU_CHECK_MSG(m_egl, "eglMakeCurrent");
		}
		else
		{
			const EGLint attribs[] =
			{
				EGL_WIDTH, 32,
				EGL_HEIGHT, 32,
				EGL_NONE
			};
			const eglu::UniqueSurface surface (m_egl, m_display, m_egl.createPbufferSurface(m_display, m_config, attribs));

			EGLU_CHECK_MSG(m_egl, "eglCreateContext");
			m_egl.makeCurrent(m_display, *surface, *surface, context);
			EGLU_CHECK_MSG(m_egl, "eglMakeCurrent");
		}
	}

	void run (void)
	{
		try
		{
			const EGLint	attribList[] =
			{
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};

			m_egl.bindAPI(EGL_OPENGL_ES_API);

			if (m_context == EGL_NO_CONTEXT)
			{
				const eglu::UniqueContext context (m_egl, m_display, m_egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attribList));

				testContext(*context);
			}
			else
			{
				testContext(m_context);
			}

		}
		catch (const eglu::Error& error)
		{
			m_error = error;
		}

		m_egl.releaseThread();
	}

private:
	const Library&				m_egl;
	const EGLDisplay			m_display;
	const EGLSurface			m_surface;
	const EGLContext			m_context;
	const EGLConfig				m_config;
	tcu::Maybe<eglu::Error>&	m_error;
};

class ThreadCleanUpTest : public TestCase
{
public:
	enum ContextType
	{
		CONTEXTTYPE_SINGLE = 0,
		CONTEXTTYPE_MULTI
	};

	enum SurfaceType
	{
		SURFACETYPE_SINGLE = 0,
		SURFACETYPE_MULTI
	};

	static std::string testCaseName (ContextType contextType, SurfaceType surfaceType)
	{
		std::string name;

		if (contextType == CONTEXTTYPE_SINGLE)
			name += "single_context_";
		else
			name += "multi_context_";

		if (surfaceType ==SURFACETYPE_SINGLE)
			name += "single_surface";
		else
			name += "multi_surface";

		return name;
	}


	ThreadCleanUpTest (EglTestContext& eglTestCtx, ContextType contextType, SurfaceType surfaceType)
		: TestCase			(eglTestCtx, testCaseName(contextType, surfaceType).c_str(), "Simple thread context clean up test")
		, m_contextType		(contextType)
		, m_surfaceType		(surfaceType)
		, m_iterCount		(250)
		, m_iterNdx			(0)
		, m_display			(EGL_NO_DISPLAY)
		, m_config			(0)
		, m_surface			(EGL_NO_SURFACE)
		, m_context			(EGL_NO_CONTEXT)
	{
	}

	~ThreadCleanUpTest (void)
	{
		deinit();
	}

	void init (void)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();

		m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

		{
			eglu::FilterList filters;
			filters << isES2Renderable << isPBuffer;
			m_config = eglu::chooseSingleConfig(egl, m_display, filters);
		}

		if (m_contextType == CONTEXTTYPE_SINGLE)
		{
			const EGLint	attribList[] =
			{
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};

			egl.bindAPI(EGL_OPENGL_ES_API);

			m_context = egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attribList);
			EGLU_CHECK_MSG(egl, "Failed to create context");
		}

		if (m_surfaceType == SURFACETYPE_SINGLE)
		{
			const EGLint attribs[] =
			{
				EGL_WIDTH, 32,
				EGL_HEIGHT, 32,
				EGL_NONE
			};

			m_surface = egl.createPbufferSurface(m_display, m_config, attribs);
			EGLU_CHECK_MSG(egl, "Failed to create surface");
		}
	}

	void deinit (void)
	{
		const Library& egl = m_eglTestCtx.getLibrary();

		if (m_surface != EGL_NO_SURFACE)
		{
			egl.destroySurface(m_display, m_surface);
			m_surface = EGL_NO_SURFACE;
		}

		if (m_context != EGL_NO_CONTEXT)
		{
			egl.destroyContext(m_display, m_context);
			m_context = EGL_NO_CONTEXT;
		}

		if (m_display != EGL_NO_DISPLAY)
		{
			egl.terminate(m_display);
			m_display = EGL_NO_DISPLAY;
		}
	}

	IterateResult iterate (void)
	{
		if (m_iterNdx < m_iterCount)
		{
			tcu::Maybe<eglu::Error> error;

			Thread thread (m_eglTestCtx.getLibrary(), m_display, m_surface, m_context, m_config, error);

			thread.start();
			thread.join();

			if (error)
			{
				m_testCtx.getLog() << TestLog::Message << "Failed. Got error: " << error->getMessage() << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, error->getMessage());
				return STOP;
			}

			m_iterNdx++;
			return CONTINUE;
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
			return STOP;
		}
	}

private:
	const ContextType	m_contextType;
	const SurfaceType	m_surfaceType;
	const size_t		m_iterCount;
	size_t				m_iterNdx;
	EGLDisplay			m_display;
	EGLConfig			m_config;
	EGLSurface			m_surface;
	EGLContext			m_context;
};

} // anonymous

TestCaseGroup* createThreadCleanUpTest (EglTestContext& eglTestCtx)
{
	de::MovePtr<TestCaseGroup> group (new TestCaseGroup(eglTestCtx, "thread_cleanup", "Thread cleanup tests"));

	group->addChild(new ThreadCleanUpTest(eglTestCtx, ThreadCleanUpTest::CONTEXTTYPE_SINGLE,	ThreadCleanUpTest::SURFACETYPE_SINGLE));
	group->addChild(new ThreadCleanUpTest(eglTestCtx, ThreadCleanUpTest::CONTEXTTYPE_MULTI,		ThreadCleanUpTest::SURFACETYPE_SINGLE));

	group->addChild(new ThreadCleanUpTest(eglTestCtx, ThreadCleanUpTest::CONTEXTTYPE_SINGLE,	ThreadCleanUpTest::SURFACETYPE_MULTI));
	group->addChild(new ThreadCleanUpTest(eglTestCtx, ThreadCleanUpTest::CONTEXTTYPE_MULTI,		ThreadCleanUpTest::SURFACETYPE_MULTI));

	return group.release();
}

} // egl
} // deqp
