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
 * \brief Test EGL_SWAP_BEHAVIOR_PRESERVED_BIT.
 *//*--------------------------------------------------------------------*/

#include "teglPreservingSwapTests.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"

#include "egluNativeWindow.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "deRandom.hpp"

#include "deString.h"

#include <vector>
#include <string>

using std::vector;
using std::string;

using namespace eglw;

namespace deqp
{
namespace egl
{

namespace
{
class GLES2Program;
class ReferenceProgram;

class PreservingSwapTest : public TestCase
{
public:
	enum DrawType
	{
		DRAWTYPE_NONE = 0,
		DRAWTYPE_GLES2_CLEAR,
		DRAWTYPE_GLES2_RENDER
	};

					PreservingSwapTest	(EglTestContext& eglTestCtx, bool preserveColorbuffer, bool readPixelsBeforeSwap, DrawType preSwapDrawType, DrawType postSwapDrawType, const char* name, const char* description);
					~PreservingSwapTest	(void);

	void			init				(void);
	void			deinit				(void);
	IterateResult	iterate				(void);

private:
	const int					m_seed;
	const bool					m_preserveColorbuffer;
	const bool					m_readPixelsBeforeSwap;
	const DrawType				m_preSwapDrawType;
	const DrawType				m_postSwapDrawType;

	EGLDisplay					m_eglDisplay;
	eglu::NativeWindow*			m_window;
	EGLSurface					m_eglSurface;
	EGLConfig					m_eglConfig;
	EGLContext					m_eglContext;
	glw::Functions				m_gl;

	GLES2Program*				m_gles2Program;
	ReferenceProgram*			m_refProgram;

	void initEGLSurface	(EGLConfig config);
	void initEGLContext (EGLConfig config);
};

class GLES2Program
{
public:
					GLES2Program	(const glw::Functions& gl);
					~GLES2Program	(void);

	void			render			(int width, int height, float x1, float y1, float x2, float y2, PreservingSwapTest::DrawType drawType);

private:
	const glw::Functions&	m_gl;
	glu::ShaderProgram		m_glProgram;
	glw::GLuint				m_coordLoc;
	glw::GLuint				m_colorLoc;

	GLES2Program&			operator=		(const GLES2Program&);
							GLES2Program	(const GLES2Program&);
};

static glu::ProgramSources getSources (void)
{
	const char* const vertexShaderSource =
		"attribute mediump vec4 a_pos;\n"
		"attribute mediump vec4 a_color;\n"
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tv_color = a_color;\n"
		"\tgl_Position = a_pos;\n"
		"}";

	const char* const fragmentShaderSource =
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = v_color;\n"
		"}";

	return glu::makeVtxFragSources(vertexShaderSource, fragmentShaderSource);
}

GLES2Program::GLES2Program (const glw::Functions& gl)
	: m_gl			(gl)
	, m_glProgram	(gl, getSources())
	, m_coordLoc	((glw::GLuint)-1)
	, m_colorLoc	((glw::GLuint)-1)
{
	m_colorLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_color");
	m_coordLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_pos");
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to get attribute locations");
}

GLES2Program::~GLES2Program (void)
{
}

void GLES2Program::render (int width, int height, float x1, float y1, float x2, float y2, PreservingSwapTest::DrawType drawType)
{
	if (drawType == PreservingSwapTest::DRAWTYPE_GLES2_RENDER)
	{
		const glw::GLfloat coords[] =
		{
			x1, y1, 0.0f, 1.0f,
			x1, y2, 0.0f, 1.0f,
			x2, y2, 0.0f, 1.0f,

			x2, y2, 0.0f, 1.0f,
			x2, y1, 0.0f, 1.0f,
			x1, y1, 0.0f, 1.0f
		};

		const glw::GLubyte colors[] =
		{
			127,	127,	127,	255,
			127,	127,	127,	255,
			127,	127,	127,	255,

			127,	127,	127,	255,
			127,	127,	127,	255,
			127,	127,	127,	255
		};

		m_gl.useProgram(m_glProgram.getProgram());
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");

		m_gl.enableVertexAttribArray(m_coordLoc);
		m_gl.enableVertexAttribArray(m_colorLoc);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to enable attributes");

		m_gl.vertexAttribPointer(m_coordLoc, 4, GL_FLOAT, GL_FALSE, 0, coords);
		m_gl.vertexAttribPointer(m_colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to set attribute pointers");

		m_gl.drawArrays(GL_TRIANGLES, 0, 6);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() failed");

		m_gl.disableVertexAttribArray(m_coordLoc);
		m_gl.disableVertexAttribArray(m_colorLoc);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to disable attributes");

		m_gl.useProgram(0);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");
	}
	else if (drawType == PreservingSwapTest::DRAWTYPE_GLES2_CLEAR)
	{
		const int ox	= width/2;
		const int oy	= height/2;

		const int px	= width;
		const int py	= height;

		const int x1i	= (int)(((float)px/2.0f) * x1 + (float)ox);
		const int y1i	= (int)(((float)py/2.0f) * y1 + (float)oy);

		const int x2i	= (int)(((float)px/2.0f) * x2 + (float)ox);
		const int y2i	= (int)(((float)py/2.0f) * y2 + (float)oy);

		m_gl.enable(GL_SCISSOR_TEST);
		m_gl.scissor(x1i, y1i, x2i-x1i, y2i-y1i);
		m_gl.clearColor(0.5f, 0.5f, 0.5f, 1.0f);
		m_gl.clear(GL_COLOR_BUFFER_BIT);
		m_gl.disable(GL_SCISSOR_TEST);
	}
	else
		DE_ASSERT(false);
}

class ReferenceProgram
{
public:
			ReferenceProgram	(void);
			~ReferenceProgram	(void);

	void	render				(tcu::Surface* target, float x1, float y1, float x2, float y2, PreservingSwapTest::DrawType drawType);

private:
						ReferenceProgram	(const ReferenceProgram&);
	ReferenceProgram&	operator=			(const ReferenceProgram&);
};

ReferenceProgram::ReferenceProgram (void)
{
}

ReferenceProgram::~ReferenceProgram (void)
{
}

void ReferenceProgram::render (tcu::Surface* target, float x1, float y1, float x2, float y2, PreservingSwapTest::DrawType drawType)
{
	if (drawType == PreservingSwapTest::DRAWTYPE_GLES2_RENDER || drawType == PreservingSwapTest::DRAWTYPE_GLES2_CLEAR)
	{
		const int ox	= target->getWidth()/2;
		const int oy	= target->getHeight()/2;

		const int px	= target->getWidth();
		const int py	= target->getHeight();

		const int x1i	= (int)((px/2.0) * x1 + ox);
		const int y1i	= (int)((py/2.0) * y1 + oy);

		const int x2i	= (int)((px/2.0) * x2 + ox);
		const int y2i	= (int)((py/2.0) * y2 + oy);

		const tcu::RGBA	color(127, 127, 127, 255);

		for (int y = y1i; y <= y2i; y++)
		{
			for (int x = x1i; x <= x2i; x++)
				target->setPixel(x, y, color);
		}
	}
	else
		DE_ASSERT(false);
}

PreservingSwapTest::PreservingSwapTest (EglTestContext& eglTestCtx, bool preserveColorbuffer, bool readPixelsBeforeSwap, DrawType preSwapDrawType, DrawType postSwapDrawType, const char* name, const char* description)
	: TestCase					(eglTestCtx, name, description)
	, m_seed					(deStringHash(name))
	, m_preserveColorbuffer		(preserveColorbuffer)
	, m_readPixelsBeforeSwap	(readPixelsBeforeSwap)
	, m_preSwapDrawType			(preSwapDrawType)
	, m_postSwapDrawType		(postSwapDrawType)
	, m_eglDisplay				(EGL_NO_DISPLAY)
	, m_window					(DE_NULL)
	, m_eglSurface				(EGL_NO_SURFACE)
	, m_eglContext				(EGL_NO_CONTEXT)
	, m_gles2Program			(DE_NULL)
	, m_refProgram				(DE_NULL)
{
}

PreservingSwapTest::~PreservingSwapTest (void)
{
	deinit();
}

EGLConfig getEGLConfig (const Library& egl, EGLDisplay eglDisplay, bool preserveColorbuffer)
{
	const EGLint attribList[] =
	{
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT | (preserveColorbuffer ? EGL_SWAP_BEHAVIOR_PRESERVED_BIT : 0),
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	return eglu::chooseSingleConfig(egl, eglDisplay, &attribList[0]);
}

void clearColorScreen (const glw::Functions& gl, float red, float green, float blue, float alpha)
{
	gl.clearColor(red, green, blue, alpha);
	gl.clear(GL_COLOR_BUFFER_BIT);
}

void clearColorReference (tcu::Surface* ref, float red, float green, float blue, float alpha)
{
	tcu::clear(ref->getAccess(), tcu::Vec4(red, green, blue, alpha));
}

void readPixels (const glw::Functions& gl, tcu::Surface* screen)
{
	gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(),  GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());
}

void PreservingSwapTest::initEGLSurface (EGLConfig config)
{
	const eglu::NativeWindowFactory&	factory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

	m_window		= factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_eglDisplay, config, DE_NULL, eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
	m_eglSurface	= eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_eglDisplay, config, DE_NULL);
}

void PreservingSwapTest::initEGLContext (EGLConfig config)
{
	const Library&	egl				= m_eglTestCtx.getLibrary();
	const EGLint	attribList[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	egl.bindAPI(EGL_OPENGL_ES_API);
	m_eglContext = egl.createContext(m_eglDisplay, config, EGL_NO_CONTEXT, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext");

	DE_ASSERT(m_eglSurface != EGL_NO_SURFACE);
	egl.makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");
}

void PreservingSwapTest::init (void)
{
	m_eglDisplay	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_eglConfig		= getEGLConfig(m_eglTestCtx.getLibrary(), m_eglDisplay, m_preserveColorbuffer);

	if (m_eglConfig == DE_NULL)
		TCU_THROW(NotSupportedError, "No supported config found");

	initEGLSurface(m_eglConfig);
	initEGLContext(m_eglConfig);

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));

	m_gles2Program	= new GLES2Program(m_gl);
	m_refProgram	= new ReferenceProgram();
}

void PreservingSwapTest::deinit (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	delete m_refProgram;
	m_refProgram = DE_NULL;

	delete m_gles2Program;
	m_gles2Program = DE_NULL;

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

	delete m_window;
	m_window = DE_NULL;
}

bool compareToReference (tcu::TestLog& log, const char* name, const char* description, const tcu::Surface& reference, const tcu::Surface& screen, int x, int y, int width, int height)
{
	return tcu::fuzzyCompare(log, name, description,
							 getSubregion(reference.getAccess(), x, y, width, height),
							 getSubregion(screen.getAccess(), x, y, width, height),
							 0.05f, tcu::COMPARE_LOG_RESULT);
}

bool comparePreAndPostSwapFramebuffers (tcu::TestLog& log, const tcu::Surface& preSwap, const tcu::Surface& postSwap)
{
	return tcu::pixelThresholdCompare(log, "Pre- / Post framebuffer compare", "Compare pre- and post-swap framebuffers", preSwap, postSwap, tcu::RGBA(0, 0, 0, 0), tcu::COMPARE_LOG_RESULT);
}

TestCase::IterateResult PreservingSwapTest::iterate (void)
{
	const Library&	egl				= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log				= m_testCtx.getLog();
	de::Random		rnd(m_seed);

	const int		width			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_WIDTH);
	const int		height			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_HEIGHT);

	const float		clearRed		= rnd.getFloat();
	const float		clearGreen		= rnd.getFloat();
	const float		clearBlue		= rnd.getFloat();
	const float		clearAlpha		= 1.0f;

	const float		preSwapX1		= -0.9f * rnd.getFloat();
	const float		preSwapY1		= -0.9f * rnd.getFloat();
	const float		preSwapX2		= 0.9f * rnd.getFloat();
	const float		preSwapY2		= 0.9f * rnd.getFloat();

	const float		postSwapX1		= -0.9f * rnd.getFloat();
	const float		postSwapY1		= -0.9f * rnd.getFloat();
	const float		postSwapX2		= 0.9f * rnd.getFloat();
	const float		postSwapY2		= 0.9f * rnd.getFloat();

	tcu::Surface	postSwapFramebufferReference(width, height);
	tcu::Surface	preSwapFramebufferReference(width, height);

	tcu::Surface	postSwapFramebuffer(width, height);
	tcu::Surface	preSwapFramebuffer(width, height);

	if (m_preserveColorbuffer)
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext));

	clearColorScreen(m_gl, clearRed, clearGreen, clearBlue, clearAlpha);

	if (m_readPixelsBeforeSwap)
		clearColorReference(&preSwapFramebufferReference, clearRed, clearGreen, clearBlue, clearAlpha);

	clearColorReference(&postSwapFramebufferReference, clearRed, clearGreen, clearBlue, clearAlpha);

	if (m_preSwapDrawType != DRAWTYPE_NONE)
	{
		m_gles2Program->render(width, height, preSwapX1, preSwapY1, preSwapX2, preSwapY2, m_preSwapDrawType);
		m_refProgram->render(&postSwapFramebufferReference, preSwapX1, preSwapY1, preSwapX2, preSwapY2, m_preSwapDrawType);
	}

	if (m_readPixelsBeforeSwap)
	{
		if (m_preSwapDrawType != DRAWTYPE_NONE)
			m_refProgram->render(&preSwapFramebufferReference, preSwapX1, preSwapY1, preSwapX2, preSwapY2, m_preSwapDrawType);

		readPixels(m_gl, &preSwapFramebuffer);
	}

	EGLU_CHECK_CALL(egl, swapBuffers(m_eglDisplay, m_eglSurface));

	if (m_postSwapDrawType != DRAWTYPE_NONE)
	{
		m_refProgram->render(&postSwapFramebufferReference, postSwapX1, postSwapY1, postSwapX2, postSwapY2, m_postSwapDrawType);
		m_gles2Program->render(width, height, postSwapX1, postSwapY1, postSwapX2, postSwapY2, m_postSwapDrawType);
	}

	readPixels(m_gl, &postSwapFramebuffer);

	bool isOk = true;

	if (m_preserveColorbuffer)
	{
		if (m_readPixelsBeforeSwap)
			isOk = isOk && compareToReference(log, "Compare pre-swap framebuffer to reference", "Compare pre-swap framebuffer to reference", preSwapFramebufferReference, preSwapFramebuffer, 0, 0, width, height);

		isOk = isOk && compareToReference(log, "Compare post-swap framebuffer to reference", "Compare post-swap framebuffer to reference", postSwapFramebufferReference, postSwapFramebuffer, 0, 0, width, height);

		if (m_readPixelsBeforeSwap && m_postSwapDrawType == PreservingSwapTest::DRAWTYPE_NONE)
			isOk = isOk && comparePreAndPostSwapFramebuffers(log, preSwapFramebuffer, postSwapFramebuffer);
	}
	else
	{
		const int ox	= width/2;
		const int oy	= height/2;

		const int px	= width;
		const int py	= height;

		const int x1i	= (int)(((float)px/2.0f) * postSwapX1 + (float)ox);
		const int y1i	= (int)(((float)py/2.0f) * postSwapY1 + (float)oy);

		const int x2i	= (int)(((float)px/2.0f) * postSwapX2 + (float)ox);
		const int y2i	= (int)(((float)py/2.0f) * postSwapY2 + (float)oy);

		if (m_readPixelsBeforeSwap)
			isOk = isOk && compareToReference(log, "Compare pre-swap framebuffer to reference", "Compare pre-swap framebuffer to reference", preSwapFramebufferReference, preSwapFramebuffer, 0, 0, width, height);

		DE_ASSERT(m_postSwapDrawType != DRAWTYPE_NONE);
		isOk = isOk && compareToReference(log, "Compare valid are of post-swap framebuffer to reference", "Compare valid area of post-swap framebuffer to reference", postSwapFramebufferReference, postSwapFramebuffer, x1i, y1i, x2i - x1i, y2i - y1i);
	}

	if (isOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

string generateTestName (PreservingSwapTest::DrawType preSwapDrawType, PreservingSwapTest::DrawType postSwapDrawType)
{
	std::ostringstream stream;

	if (preSwapDrawType == PreservingSwapTest::DRAWTYPE_NONE && postSwapDrawType == PreservingSwapTest::DRAWTYPE_NONE)
		stream << "no_draw";
	else
	{
		switch (preSwapDrawType)
		{
			case PreservingSwapTest::DRAWTYPE_NONE:
				// Do nothing
				break;

			case PreservingSwapTest::DRAWTYPE_GLES2_RENDER:
				stream << "pre_render";
				break;

			case PreservingSwapTest::DRAWTYPE_GLES2_CLEAR:
				stream << "pre_clear";
				break;

			default:
				DE_ASSERT(false);
		}

		if (preSwapDrawType != PreservingSwapTest::DRAWTYPE_NONE && postSwapDrawType != PreservingSwapTest::DRAWTYPE_NONE)
			stream << "_";

		switch (postSwapDrawType)
		{
			case PreservingSwapTest::DRAWTYPE_NONE:
				// Do nothing
				break;

			case PreservingSwapTest::DRAWTYPE_GLES2_RENDER:
				stream << "post_render";
				break;

			case PreservingSwapTest::DRAWTYPE_GLES2_CLEAR:
				stream << "post_clear";
				break;

			default:
				DE_ASSERT(false);
		}
	}

	return stream.str();
}

} // anonymous

PreservingSwapTests::PreservingSwapTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "preserve_swap", "Color buffer preserving swap tests")
{
}

void PreservingSwapTests::init (void)
{
	const PreservingSwapTest::DrawType preSwapDrawTypes[] =
	{
		PreservingSwapTest::DRAWTYPE_NONE,
		PreservingSwapTest::DRAWTYPE_GLES2_CLEAR,
		PreservingSwapTest::DRAWTYPE_GLES2_RENDER
	};

	const PreservingSwapTest::DrawType postSwapDrawTypes[] =
	{
		PreservingSwapTest::DRAWTYPE_NONE,
		PreservingSwapTest::DRAWTYPE_GLES2_CLEAR,
		PreservingSwapTest::DRAWTYPE_GLES2_RENDER
	};

	for (int preserveNdx = 0; preserveNdx < 2; preserveNdx++)
	{
		const bool				preserve		= (preserveNdx == 0);
		TestCaseGroup* const	preserveGroup	= new TestCaseGroup(m_eglTestCtx, (preserve ? "preserve" : "no_preserve"), "");

		for (int readPixelsNdx = 0; readPixelsNdx < 2; readPixelsNdx++)
		{
			const bool				readPixelsBeforeSwap		= (readPixelsNdx == 1);
			TestCaseGroup* const	readPixelsBeforeSwapGroup	= new TestCaseGroup(m_eglTestCtx, (readPixelsBeforeSwap ? "read_before_swap" : "no_read_before_swap"), "");

			for (int preSwapDrawTypeNdx = 0; preSwapDrawTypeNdx < DE_LENGTH_OF_ARRAY(preSwapDrawTypes); preSwapDrawTypeNdx++)
			{
				const PreservingSwapTest::DrawType preSwapDrawType = preSwapDrawTypes[preSwapDrawTypeNdx];

				for (int postSwapDrawTypeNdx = 0; postSwapDrawTypeNdx < DE_LENGTH_OF_ARRAY(postSwapDrawTypes); postSwapDrawTypeNdx++)
				{
					const PreservingSwapTest::DrawType postSwapDrawType = postSwapDrawTypes[postSwapDrawTypeNdx];

					// If not preserving and rendering after swap, then there is nothing to verify
					if (!preserve && postSwapDrawType == PreservingSwapTest::DRAWTYPE_NONE)
						continue;

					const std::string name = generateTestName(preSwapDrawType, postSwapDrawType);

					readPixelsBeforeSwapGroup->addChild(new PreservingSwapTest(m_eglTestCtx, preserve, readPixelsBeforeSwap, preSwapDrawType, postSwapDrawType, name.c_str(), ""));
				}
			}

			preserveGroup->addChild(readPixelsBeforeSwapGroup);
		}

		addChild(preserveGroup);
	}
}

} // egl
} // deqp
