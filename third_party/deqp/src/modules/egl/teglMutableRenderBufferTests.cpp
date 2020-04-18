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
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Test KHR_mutable_render_buffer
 *//*--------------------------------------------------------------------*/

#include "teglMutableRenderBufferTests.hpp"

#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "gluRenderContext.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

using namespace eglw;

using std::vector;

namespace deqp
{
namespace egl
{
namespace
{

class MutableRenderBufferTest : public TestCase
{
public:
						MutableRenderBufferTest		(EglTestContext&	eglTestCtx,
													 const char*		name,
													 const char*		description,
													 bool				enableConfigBit);
						~MutableRenderBufferTest	(void);
	void				init						(void);
	void				deinit						(void);
	IterateResult		iterate						(void);

protected:
	deUint32			drawAndSwap					(const Library&		egl,
													 deUint32			color,
													 bool				flush);
	bool				m_enableConfigBit;
	EGLDisplay			m_eglDisplay;
	EGLSurface			m_eglSurface;
	EGLConfig			m_eglConfig;
	eglu::NativeWindow*	m_window;
	EGLContext			m_eglContext;
	glw::Functions		m_gl;
};

MutableRenderBufferTest::MutableRenderBufferTest (EglTestContext& eglTestCtx,
												  const char* name, const char* description,
												  bool enableConfigBit)
	: TestCase			(eglTestCtx, name, description)
	, m_enableConfigBit	(enableConfigBit)
	, m_eglDisplay		(EGL_NO_DISPLAY)
	, m_eglSurface		(EGL_NO_SURFACE)
	, m_eglConfig		(DE_NULL)
	, m_window			(DE_NULL)
	, m_eglContext		(EGL_NO_CONTEXT)
{
}

MutableRenderBufferTest::~MutableRenderBufferTest (void)
{
	deinit();
}

void MutableRenderBufferTest::init (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	// create display
	m_eglDisplay		= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_KHR_mutable_render_buffer"))
	{
		TCU_THROW(NotSupportedError, "EGL_KHR_mutable_render_buffer is not supported");
	}

	// get mutable render buffer config
	const EGLint	attribs[]	=
	{
        EGL_RED_SIZE,			8,
        EGL_GREEN_SIZE,			8,
        EGL_BLUE_SIZE,			8,
		EGL_ALPHA_SIZE,			8,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT | EGL_MUTABLE_RENDER_BUFFER_BIT_KHR,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	const EGLint	attribsNoBit[]	=
	{
        EGL_RED_SIZE,			8,
        EGL_GREEN_SIZE,			8,
        EGL_BLUE_SIZE,			8,
		EGL_ALPHA_SIZE,			8,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	if (m_enableConfigBit)
	{
		m_eglConfig = eglu::chooseSingleConfig(egl, m_eglDisplay, attribs);
	}
	else
	{
		const vector<EGLConfig> configs = eglu::chooseConfigs(egl, m_eglDisplay, attribsNoBit);

		for (vector<EGLConfig>::const_iterator config = configs.begin(); config != configs.end(); ++config)
		{
			EGLint surfaceType = -1;
			EGLU_CHECK_CALL(egl, getConfigAttrib(m_eglDisplay, *config, EGL_SURFACE_TYPE, &surfaceType));

			if (!(surfaceType & EGL_MUTABLE_RENDER_BUFFER_BIT_KHR))
			{
				m_eglConfig = *config;
				break;
			}
		}

		if (m_eglConfig == DE_NULL)
			TCU_THROW(NotSupportedError, "No config without support for mutable_render_buffer found");
	}

	// create surface
	const eglu::NativeWindowFactory& factory = eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
	m_window = factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_eglDisplay, m_eglConfig, DE_NULL,
									eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
	m_eglSurface = eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_eglDisplay, m_eglConfig, DE_NULL);

	// create context and make current
	const EGLint	contextAttribList[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	egl.bindAPI(EGL_OPENGL_ES_API);
	m_eglContext = egl.createContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext");
	TCU_CHECK(m_eglSurface != EGL_NO_SURFACE);
	egl.makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

void MutableRenderBufferTest::deinit (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (m_eglContext != EGL_NO_CONTEXT)
	{
		egl.makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		egl.destroyContext(m_eglDisplay, m_eglContext);
		m_eglContext = EGL_NO_CONTEXT;
	}

	if (m_eglSurface != EGL_NO_SURFACE)
	{
		egl.destroySurface(m_eglDisplay, m_eglSurface);
		m_eglSurface = EGL_NO_SURFACE;
	}

	if (m_eglDisplay != EGL_NO_DISPLAY)
	{
		egl.terminate(m_eglDisplay);
		m_eglDisplay = EGL_NO_DISPLAY;
	}

	if (m_window != DE_NULL)
	{
		delete m_window;
		m_window = DE_NULL;
	}
}

deUint32 MutableRenderBufferTest::drawAndSwap (const Library& egl, deUint32 color, bool flush)
{
	DE_ASSERT(color < 256);
	m_gl.clearColor((float)color/255.f, (float)color/255.f, (float)color/255.f, (float)color/255.f);
	m_gl.clear(GL_COLOR_BUFFER_BIT);
	if (flush)
	{
		m_gl.flush();
	}
	else
	{
		EGLU_CHECK_CALL(egl, swapBuffers(m_eglDisplay, m_eglSurface));
	}
	return (color | color << 8 | color << 16 | color << 24);
}

TestCase::IterateResult MutableRenderBufferTest::iterate (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	int frameNumber = 1;

	// run a few back-buffered frames even if we can't verify their contents
	for (; frameNumber < 5; frameNumber++)
	{
		drawAndSwap(egl, frameNumber, false);
	}

	// switch to single-buffer rendering
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER));

	// Use eglSwapBuffers for the first frame
	drawAndSwap(egl, frameNumber, false);
	frameNumber++;

	// test a few single-buffered frames
	for (; frameNumber < 10; frameNumber++)
	{
		deUint32 backBufferPixel = 0xFFFFFFFF;
		deUint32 frontBufferPixel = drawAndSwap(egl, frameNumber, true);
		m_gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &backBufferPixel);

		// when single buffered, front-buffer == back-buffer
		if (backBufferPixel != frontBufferPixel)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface isn't single-buffered");
			return STOP;
		}
	}

	// switch back to back-buffer rendering
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, EGL_BACK_BUFFER));

	// run a few back-buffered frames even if we can't verify their contents
	for (; frameNumber < 14; frameNumber++)
	{
		drawAndSwap(egl, frameNumber, false);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

class MutableRenderBufferQueryTest : public MutableRenderBufferTest
{
public:
						MutableRenderBufferQueryTest	(EglTestContext&	eglTestCtx,
														 const char*		name,
														 const char*		description);
						~MutableRenderBufferQueryTest	(void);
	IterateResult		iterate							(void);
};

MutableRenderBufferQueryTest::MutableRenderBufferQueryTest (EglTestContext& eglTestCtx,
															const char* name, const char* description)
	: MutableRenderBufferTest	(eglTestCtx, name, description, true)
{
}

MutableRenderBufferQueryTest::~MutableRenderBufferQueryTest (void)
{
	deinit();
}

TestCase::IterateResult MutableRenderBufferQueryTest::iterate (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	// check that by default the query returns back buffered
	EGLint curRenderBuffer = -1;
	EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, &curRenderBuffer));
	if (curRenderBuffer != EGL_BACK_BUFFER)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface didn't default to back-buffered rendering");
		return STOP;
	}

	// switch to single-buffer rendering and check that the query output changed
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER));
	EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, &curRenderBuffer));
	if (curRenderBuffer != EGL_SINGLE_BUFFER)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface didn't switch to single-buffer rendering");
		return STOP;
	}

	// switch back to back-buffer rendering and check the query again
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, EGL_BACK_BUFFER));
	EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, &curRenderBuffer));
	if (curRenderBuffer != EGL_BACK_BUFFER)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface didn't switch back to back-buffer rendering");
		return STOP;
	}
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

class MutableRenderBufferQueryNegativeTest : public MutableRenderBufferTest
{
public:
						MutableRenderBufferQueryNegativeTest	(EglTestContext&	eglTestCtx,
																 const char*		name,
																 const char*		description);
						~MutableRenderBufferQueryNegativeTest	(void);
	IterateResult		iterate									(void);
};

MutableRenderBufferQueryNegativeTest::MutableRenderBufferQueryNegativeTest (EglTestContext& eglTestCtx,
															const char* name, const char* description)
	: MutableRenderBufferTest	(eglTestCtx, name, description, false)
{
}

MutableRenderBufferQueryNegativeTest::~MutableRenderBufferQueryNegativeTest (void)
{
	deinit();
}

TestCase::IterateResult MutableRenderBufferQueryNegativeTest::iterate (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	// check that by default the query returns back buffered
	EGLint curRenderBuffer = -1;
	EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, &curRenderBuffer));
	if (curRenderBuffer != EGL_BACK_BUFFER)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface didn't default to back-buffered rendering");
		return STOP;
	}

	// check that trying to switch to single-buffer rendering fails when the config bit is not set
	EGLBoolean ret = egl.surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, EGL_SINGLE_BUFFER);
	EGLint err = egl.getError();
	if (ret != EGL_FALSE)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
			"eglSurfaceAttrib didn't return false when trying to enable single-buffering on a context without the mutable render buffer bit set");
		return STOP;
	}
	if (err != EGL_BAD_MATCH)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
			"eglSurfaceAttrib didn't set the EGL_BAD_MATCH error when trying to enable single-buffering on a context without the mutable render buffer bit set");
		return STOP;
	}

    EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_RENDER_BUFFER, &curRenderBuffer));
    if (curRenderBuffer != EGL_BACK_BUFFER)
    {
        m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Surface didn't stay in back-buffered rendering after error");
        return STOP;
    }

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} // anonymous

MutableRenderBufferTests::MutableRenderBufferTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "mutable_render_buffer", "Mutable render buffer tests")
{
}

void MutableRenderBufferTests::init (void)
{
	addChild(new MutableRenderBufferQueryTest(m_eglTestCtx, "querySurface",
		"Tests if querySurface returns the correct value after surfaceAttrib is called"));
	addChild(new MutableRenderBufferQueryNegativeTest(m_eglTestCtx, "negativeConfigBit",
		"Tests trying to enable single-buffering on a context without the mutable render buffer bit set"));
	addChild(new MutableRenderBufferTest(m_eglTestCtx, "basic",
		"Tests enabling/disabling single-buffer rendering and checks the buffering behavior", true));
}

} // egl
} // deqp
