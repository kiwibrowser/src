/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Test KHR_swap_buffer_with_damage
 *//*--------------------------------------------------------------------*/

#include "teglSwapBuffersWithDamageTests.hpp"

#include "tcuImageCompare.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"

#include "egluNativeWindow.hpp"
#include "egluUtil.hpp"
#include "egluConfigFilter.hpp"

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

#include <string>
#include <vector>
#include <sstream>

using std::string;
using std::vector;
using glw::GLubyte;
using tcu::IVec2;

using namespace eglw;

namespace deqp
{
namespace egl
{
namespace
{

typedef	tcu::Vector<GLubyte, 3> Color;

enum DrawType
{
    DRAWTYPE_GLES2_CLEAR,
    DRAWTYPE_GLES2_RENDER
};

enum ResizeType
{
	RESIZETYPE_NONE = 0,
	RESIZETYPE_BEFORE_SWAP,
	RESIZETYPE_AFTER_SWAP,

	RESIZETYPE_LAST
};

struct ColoredRect
{
public:
				ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_);
	IVec2		bottomLeft;
	IVec2		topRight;
	Color		color;
};

ColoredRect::ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_)
	: bottomLeft	(bottomLeft_)
	, topRight		(topRight_)
	, color			(color_)
{
}

struct DrawCommand
{
				DrawCommand (DrawType drawType_, const ColoredRect& rect_);
    DrawType	drawType;
	ColoredRect	rect;
};

DrawCommand::DrawCommand (DrawType drawType_, const ColoredRect& rect_)
	: drawType	(drawType_)
	, rect		(rect_)
{
}

struct Frame
{
						Frame (int width_, int height_);
	int					width;
	int					height;
	vector<DrawCommand> draws;
};

Frame::Frame (int width_, int height_)
	: width (width_)
	, height(height_)
{
}

typedef vector<Frame> FrameSequence;

//helper function declaration
EGLConfig		getEGLConfig					(const Library& egl, EGLDisplay eglDisplay, bool preserveBuffer);
void			clearColorScreen				(const glw::Functions& gl, const tcu::Vec4& clearColor);
float			windowToDeviceCoordinates		(int x, int length);

class GLES2Renderer
{
public:
							GLES2Renderer		(const glw::Functions& gl);
							~GLES2Renderer		(void);
	void					render				(int width, int height, const Frame& frame) const;

private:
							GLES2Renderer		(const GLES2Renderer&);
	GLES2Renderer&			operator=			(const GLES2Renderer&);

	const glw::Functions&	m_gl;
	glu::ShaderProgram		m_glProgram;
	glw::GLuint				m_coordLoc;
	glw::GLuint				m_colorLoc;
};

// generate sources for vertex and fragment buffer
glu::ProgramSources getSources (void)
{
	const char* const vertexShaderSource =
		"attribute mediump vec2 a_pos;\n"
		"attribute mediump vec4 a_color;\n"
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tv_color = a_color;\n"
		"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
		"}";

	const char* const fragmentShaderSource =
		"varying mediump vec4 v_color;\n"
		"void main(void)\n"
		"{\n"
		"\tgl_FragColor = v_color;\n"
		"}";

	return glu::makeVtxFragSources(vertexShaderSource, fragmentShaderSource);
}

GLES2Renderer::GLES2Renderer (const glw::Functions& gl)
	: m_gl        (gl)
	, m_glProgram (gl, getSources())
	, m_coordLoc  ((glw::GLuint)-1)
	, m_colorLoc  ((glw::GLuint)-1)
{
	m_colorLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_color");
	m_coordLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_pos");
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to get attribute locations");
}

GLES2Renderer::~GLES2Renderer (void)
{
}

void GLES2Renderer::render (int width, int height, const Frame& frame) const
{
	for (size_t drawNdx = 0; drawNdx < frame.draws.size(); drawNdx++)
	{
		const ColoredRect& coloredRect = frame.draws[drawNdx].rect;

		if (frame.draws[drawNdx].drawType == DRAWTYPE_GLES2_RENDER)
		{
			const float x1 = windowToDeviceCoordinates(coloredRect.bottomLeft.x(), width);
			const float y1 = windowToDeviceCoordinates(coloredRect.bottomLeft.y(), height);
			const float x2 = windowToDeviceCoordinates(coloredRect.topRight.x(), width);
			const float y2 = windowToDeviceCoordinates(coloredRect.topRight.y(), height);

			const glw::GLfloat coords[] =
			{
				x1, y1,
				x1, y2,
				x2, y2,

				x2, y2,
				x2, y1,
				x1, y1,
			};

			const glw::GLubyte colors[] =
			{
				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,
				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,
				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,

				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,
				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,
				coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255,
			};

			m_gl.useProgram(m_glProgram.getProgram());
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");

			m_gl.enableVertexAttribArray(m_coordLoc);
			m_gl.enableVertexAttribArray(m_colorLoc);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to enable attributes");

			m_gl.vertexAttribPointer(m_coordLoc, 2, GL_FLOAT, GL_FALSE, 0, coords);
			m_gl.vertexAttribPointer(m_colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to set attribute pointers");

			m_gl.drawArrays(GL_TRIANGLES, 0, DE_LENGTH_OF_ARRAY(coords)/2);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays(), failed");

			m_gl.disableVertexAttribArray(m_coordLoc);
			m_gl.disableVertexAttribArray(m_colorLoc);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to disable attributes");

			m_gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");
		}
		else if (frame.draws[drawNdx].drawType == DRAWTYPE_GLES2_CLEAR)
		{
			m_gl.enable(GL_SCISSOR_TEST);
			m_gl.scissor(coloredRect.bottomLeft.x(), coloredRect.bottomLeft.y(),
						 coloredRect.topRight.x()-coloredRect.bottomLeft.x(), coloredRect.topRight.y()-coloredRect.bottomLeft.y());
			m_gl.clearColor(coloredRect.color.x()/255.0f, coloredRect.color.y()/255.0f, coloredRect.color.z()/255.0f, 1.0f);
			m_gl.clear(GL_COLOR_BUFFER_BIT);
			m_gl.disable(GL_SCISSOR_TEST);
		}
		else
			DE_FATAL("Invalid drawtype");
	}
}

class SwapBuffersWithDamageTest : public TestCase
{
public:
								SwapBuffersWithDamageTest		(EglTestContext&			eglTestCtx,
																 const vector<DrawType>&	frameDrawType,
																 int						iterationTimes,
																 ResizeType					resizeType,
																 const char*				name,
																 const char*				description);

								~SwapBuffersWithDamageTest		(void);

	virtual void				init							(void);
	void						deinit							(void);
	virtual IterateResult		iterate							(void);

protected:
	virtual EGLConfig			getConfig						(const Library& egl, EGLDisplay eglDisplay);
	virtual void				checkExtension					(const Library& egl, EGLDisplay eglDisplay);
	void						initEGLSurface					(EGLConfig config);
	void						initEGLContext					(EGLConfig config);

	eglu::NativeWindow*			m_window;
	EGLConfig					m_eglConfig;
	EGLContext					m_eglContext;
	const int					m_seed;
	const int					m_iterationTimes;
	const vector<DrawType>		m_frameDrawType;
	const ResizeType			m_resizeType;
	EGLDisplay					m_eglDisplay;
	EGLSurface					m_eglSurface;
	glw::Functions				m_gl;
	GLES2Renderer*				m_gles2Renderer;
};

SwapBuffersWithDamageTest::SwapBuffersWithDamageTest (EglTestContext& eglTestCtx, const vector<DrawType>& frameDrawType, int iterationTimes, ResizeType resizeType, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, m_window			(DE_NULL)
	, m_eglContext		(EGL_NO_CONTEXT)
	, m_seed			(deStringHash(name))
	, m_iterationTimes	(iterationTimes)
	, m_frameDrawType	(frameDrawType)
	, m_resizeType		(resizeType)
	, m_eglDisplay		(EGL_NO_DISPLAY)
	, m_eglSurface		(EGL_NO_SURFACE)
	, m_gles2Renderer	 (DE_NULL)
{
}

SwapBuffersWithDamageTest::~SwapBuffersWithDamageTest (void)
{
	deinit();
}

EGLConfig SwapBuffersWithDamageTest::getConfig (const Library& egl, EGLDisplay eglDisplay)
{
	return getEGLConfig(egl, eglDisplay, false);
}

void SwapBuffersWithDamageTest::checkExtension (const Library& egl, EGLDisplay eglDisplay)
{
	if (!eglu::hasExtension(egl, eglDisplay, "EGL_KHR_swap_buffers_with_damage"))
		TCU_THROW(NotSupportedError, "EGL_KHR_swap_buffers_with_damage is not supported");
}

void SwapBuffersWithDamageTest::init (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	m_eglDisplay = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_eglConfig  = getConfig(egl, m_eglDisplay);

	checkExtension(egl, m_eglDisplay);

	initEGLSurface(m_eglConfig);
	initEGLContext(m_eglConfig);

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
	m_gles2Renderer = new GLES2Renderer(m_gl);
}

void SwapBuffersWithDamageTest::deinit (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	delete m_gles2Renderer;
	m_gles2Renderer = DE_NULL;

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

void SwapBuffersWithDamageTest::initEGLSurface (EGLConfig config)
{
	const eglu::NativeWindowFactory& factory = eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
	m_window = factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_eglDisplay, config, DE_NULL,
									eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
	m_eglSurface = eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_eglDisplay, config, DE_NULL);
}

void SwapBuffersWithDamageTest::initEGLContext (EGLConfig config)
{
	const Library&	egl			 = m_eglTestCtx.getLibrary();
	const EGLint	attribList[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	egl.bindAPI(EGL_OPENGL_ES_API);
	m_eglContext = egl.createContext(m_eglDisplay, config, EGL_NO_CONTEXT, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext");
	TCU_CHECK(m_eglSurface != EGL_NO_SURFACE);
	egl.makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");
}

FrameSequence	generateFrameSequence	(const vector<DrawType>& frameDrawType, de::Random& rnd, int numFrames, int width, int height);
vector<EGLint>	getDamageRegion			(const Frame& frame);

TestCase::IterateResult SwapBuffersWithDamageTest::iterate (void)
{
	de::Random			rnd				(m_seed);
	const Library&		egl				= m_eglTestCtx.getLibrary();
	const int			width			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_WIDTH);
	const int			height			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_HEIGHT);
	const float			clearRed		= rnd.getFloat();
	const float			clearGreen		= rnd.getFloat();
	const float			clearBlue		= rnd.getFloat();
	const tcu::Vec4		clearColor		(clearRed, clearGreen, clearBlue, 1.0f);
	const int			numFrames		= 24; // (width, height) = (480, 480) --> numFrame = 24, divisible
	const FrameSequence frameSequence	= generateFrameSequence(m_frameDrawType, rnd, numFrames, width, height);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));

	for (int iterationNdx = 0; iterationNdx < m_iterationTimes; iterationNdx++)
	{
		for (int currentFrameNdx = 0; currentFrameNdx < numFrames; currentFrameNdx++)
		{
			vector<EGLint>	damageRegion = getDamageRegion(frameSequence[currentFrameNdx]);

			clearColorScreen(m_gl, clearColor);
			for (int ndx = 0; ndx <= currentFrameNdx; ndx++)
				m_gles2Renderer->render(width, height, frameSequence[ndx]);

			if (m_resizeType == RESIZETYPE_BEFORE_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}

			EGLU_CHECK_CALL(egl, swapBuffersWithDamageKHR(m_eglDisplay, m_eglSurface, &damageRegion[0], (EGLint)damageRegion.size()/4));

			if (m_resizeType == RESIZETYPE_AFTER_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}
		}
	}
	return STOP;
}

class SwapBuffersWithDamageAndPreserveBufferTest : public SwapBuffersWithDamageTest
{
public:
					SwapBuffersWithDamageAndPreserveBufferTest	(EglTestContext&			eglTestCtx,
																 const vector<DrawType>&	frameDrawType,
																 int						iterationTimes,
																 ResizeType					resizeType,
																 const char*				name,
																 const char*				description);

	IterateResult	iterate										(void);

protected:
	EGLConfig		getConfig									(const Library& egl, EGLDisplay eglDisplay);
};

SwapBuffersWithDamageAndPreserveBufferTest::SwapBuffersWithDamageAndPreserveBufferTest (EglTestContext&			eglTestCtx,
																						const vector<DrawType>&	frameDrawType,
																						int						iterationTimes,
																						ResizeType				resizeType,
																						const char*				name,
																						const char*				description)
	: SwapBuffersWithDamageTest (eglTestCtx, frameDrawType, iterationTimes, resizeType, name, description)
{
}

EGLConfig SwapBuffersWithDamageAndPreserveBufferTest::getConfig (const Library& egl, EGLDisplay eglDisplay)
{
	return getEGLConfig(egl, eglDisplay, true);
}

TestCase::IterateResult SwapBuffersWithDamageAndPreserveBufferTest::iterate (void)
{

	de::Random			rnd				(m_seed);
	const Library&		egl				= m_eglTestCtx.getLibrary();
	const int			width			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_WIDTH);
	const int			height			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_HEIGHT);
	const float			clearRed		= rnd.getFloat();
	const float			clearGreen		= rnd.getFloat();
	const float			clearBlue		= rnd.getFloat();
	const tcu::Vec4		clearColor		(clearRed, clearGreen, clearBlue, 1.0f);
	const int			numFrames		= 24; // (width, height) = (480, 480) --> numFrame = 24, divisible
	const FrameSequence frameSequence	= generateFrameSequence(m_frameDrawType, rnd, numFrames, width, height);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));

	for (int iterationNdx = 0; iterationNdx < m_iterationTimes; iterationNdx++)
	{
		clearColorScreen(m_gl, clearColor);
		EGLU_CHECK_CALL(egl, swapBuffersWithDamageKHR(m_eglDisplay, m_eglSurface, DE_NULL, 0));

		for (int frameNdx = 0; frameNdx < numFrames; frameNdx++)
		{
			const Frame&	currentFrame = frameSequence[frameNdx];
			vector<EGLint>	damageRegion = getDamageRegion(currentFrame);

			m_gles2Renderer->render(width, height, currentFrame);

			if (m_resizeType == RESIZETYPE_BEFORE_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}

			EGLU_CHECK_CALL(egl, swapBuffersWithDamageKHR(m_eglDisplay, m_eglSurface, &damageRegion[0], (EGLint)damageRegion.size()/4));

			if (m_resizeType == RESIZETYPE_AFTER_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}
		}
	}

	return STOP;
}

class SwapBuffersWithDamageAndBufferAgeTest : public SwapBuffersWithDamageTest
{
public:
					SwapBuffersWithDamageAndBufferAgeTest	(EglTestContext&			eglTestCtx,
															 const vector<DrawType>&	frameDrawType,
															 int						iterationTimes,
															 ResizeType					resizeType,
															 const char*				name,
															 const char*				description);

	IterateResult	iterate									(void);

protected:
	void			checkExtension							(const Library& egl, EGLDisplay eglDisplay);
};

SwapBuffersWithDamageAndBufferAgeTest::SwapBuffersWithDamageAndBufferAgeTest (EglTestContext&			eglTestCtx,
																			  const vector<DrawType>&	frameDrawType,
																			  int						iterationTimes,
																			  ResizeType				resizeType,
																			  const char*				name,
																			  const char*				description)
	: SwapBuffersWithDamageTest (eglTestCtx, frameDrawType, iterationTimes, resizeType, name, description)
{
}


void SwapBuffersWithDamageAndBufferAgeTest::checkExtension (const Library& egl, EGLDisplay eglDisplay)
{
	if (!eglu::hasExtension(egl, eglDisplay, "EGL_KHR_swap_buffers_with_damage"))
		TCU_THROW(NotSupportedError, "EGL_KHR_swap_buffers_with_damage is not supported");

	if (!eglu::hasExtension(egl, eglDisplay, "EGL_EXT_buffer_age"))
		TCU_THROW(NotSupportedError, "EGL_EXT_buffer_age not supported");
}

TestCase::IterateResult SwapBuffersWithDamageAndBufferAgeTest::iterate (void)
{

	de::Random			rnd				(m_seed);
	const Library&		egl				= m_eglTestCtx.getLibrary();
	const int			width			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_WIDTH);
	const int			height			= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_HEIGHT);
	const float			clearRed		= rnd.getFloat();
	const float			clearGreen		= rnd.getFloat();
	const float			clearBlue		= rnd.getFloat();
	const tcu::Vec4		clearColor		(clearRed, clearGreen, clearBlue, 1.0f);
	const int			numFrames		= 24; // (width, height) = (480, 480) --> numFrame = 24, divisible
	const FrameSequence frameSequence	= generateFrameSequence(m_frameDrawType, rnd, numFrames, width, height);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));

	for (int iterationNdx = 0; iterationNdx < m_iterationTimes; iterationNdx++)
	{
		clearColorScreen(m_gl, clearColor);
		EGLU_CHECK_CALL(egl, swapBuffersWithDamageKHR(m_eglDisplay, m_eglSurface, DE_NULL, 0));

		for (int frameNdx = 0; frameNdx < numFrames; frameNdx++)
		{
			vector<EGLint>	damageRegion;
			int				bufferAge		= -1;
			int				startFrameNdx	= -1;
			int				endFrameNdx		= frameNdx;

			EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_EXT, &bufferAge));

			if (bufferAge < 0) // invalid buffer age
			{
				std::ostringstream stream;
				stream << "Fail, the age is invalid. Age: " << bufferAge << ", frameNdx: " << frameNdx;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, stream.str().c_str());
				return STOP;
			}

			if (bufferAge == 0 || bufferAge > frameNdx)
			{
				clearColorScreen(m_gl, clearColor);
				startFrameNdx = 0;
			}
			else
				startFrameNdx = frameNdx-bufferAge+1;

			for (int ndx = startFrameNdx; ndx <= endFrameNdx; ndx++)
			{
				const vector<EGLint> partialDamageRegion = getDamageRegion(frameSequence[ndx]);

				damageRegion.insert(damageRegion.end(), partialDamageRegion.begin(), partialDamageRegion.end());
				m_gles2Renderer->render(width, height, frameSequence[ndx]);
			}

			if (m_resizeType == RESIZETYPE_BEFORE_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}

			EGLU_CHECK_CALL(egl, swapBuffersWithDamageKHR(m_eglDisplay, m_eglSurface, &damageRegion[0], (EGLint)damageRegion.size()/4));

			if (m_resizeType == RESIZETYPE_AFTER_SWAP)
			{
				if (iterationNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}
		}
	}
	return STOP;
}

// generate a frame sequence with certain frame for visual verification
FrameSequence generateFrameSequence (const vector<DrawType>& frameDrawType, de::Random& rnd, int numFrames, int width, int height)
{
	const int			frameDiff		= height / numFrames;
	const GLubyte		r				= rnd.getUint8();
	const GLubyte		g				= rnd.getUint8();
	const GLubyte		b				= rnd.getUint8();
	const Color			color			(r, g, b);
	FrameSequence		frameSequence;

	for (int frameNdx = 0; frameNdx < numFrames; frameNdx++)
	{
		Frame frame (width, height);

		for (int rectNdx = 0; rectNdx < (int)frameDrawType.size(); rectNdx++)
		{
			const int			rectHeight		= frameDiff / (int)frameDrawType.size();
			const ColoredRect	rect			(IVec2(0, frameNdx*frameDiff+rectNdx*rectHeight), IVec2(width, frameNdx*frameDiff+(rectNdx+1)*rectHeight), color);
			const DrawCommand	drawCommand		(frameDrawType[rectNdx], rect);

			frame.draws.push_back(drawCommand);
		}
		frameSequence.push_back(frame);
	}
	return frameSequence;
}

vector<EGLint> getDamageRegion (const Frame& frame)
{
	vector<EGLint> damageRegion;
	for (size_t drawNdx = 0; drawNdx < frame.draws.size(); drawNdx++)
	{
		const ColoredRect& rect = frame.draws[drawNdx].rect;
		damageRegion.push_back(rect.bottomLeft.x());
		damageRegion.push_back(rect.bottomLeft.y());
		damageRegion.push_back(rect.topRight.x() - rect.bottomLeft.x());
		damageRegion.push_back(rect.topRight.y() - rect.bottomLeft.y());
	}

	DE_ASSERT(damageRegion.size() % 4 == 0);
	return damageRegion;
}

string generateTestName (const vector<DrawType>& frameDrawType)
{
	std::ostringstream stream;

	for (size_t ndx = 0; ndx < frameDrawType.size(); ndx++)
	{
		if (frameDrawType[ndx] == DRAWTYPE_GLES2_RENDER)
			stream << "render";
		else if (frameDrawType[ndx] == DRAWTYPE_GLES2_CLEAR)
			stream << "clear";
		else
			DE_ASSERT(false);

		if (ndx < frameDrawType.size()-1)
			stream << "_";
	}

	return stream.str();
}

string generateResizeGroupName (ResizeType resizeType)
{
	switch (resizeType)
	{
		case RESIZETYPE_NONE:
			return "no_resize";

		case RESIZETYPE_AFTER_SWAP:
			return "resize_after_swap";

		case RESIZETYPE_BEFORE_SWAP:
			return "resize_before_swap";

		default:
			DE_FATAL("Unknown resize type");
			return "";
	}
}

bool isWindow (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & EGL_WINDOW_BIT) == EGL_WINDOW_BIT;
}

bool isES2Renderable (const eglu::CandidateConfig& c)
{
	return (c.get(EGL_RENDERABLE_TYPE) & EGL_OPENGL_ES2_BIT) == EGL_OPENGL_ES2_BIT;
}

bool hasPreserveSwap (const eglu::CandidateConfig& c)
{
	return (c.surfaceType() & EGL_SWAP_BEHAVIOR_PRESERVED_BIT) == EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
}

EGLConfig getEGLConfig (const Library& egl, EGLDisplay eglDisplay, bool preserveBuffer)
{
	eglu::FilterList filters;

	filters << isWindow << isES2Renderable;
	if (preserveBuffer)
		filters << hasPreserveSwap;

	return eglu::chooseSingleConfig(egl, eglDisplay, filters);
}

void clearColorScreen (const glw::Functions& gl, const tcu::Vec4& clearColor)
{
	gl.clearColor(clearColor.x(), clearColor.y(), clearColor.z(), clearColor.w());
	gl.clear(GL_COLOR_BUFFER_BIT);
}

float windowToDeviceCoordinates (int x, int length)
{
	return (2.0f * float(x) / float(length)) - 1.0f;
}

} // anonymous

SwapBuffersWithDamageTests::SwapBuffersWithDamageTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "swap_buffers_with_damage", "Swap buffers with damages tests")
{
}

void SwapBuffersWithDamageTests::init (void)
{
	const DrawType clearRender[2] =
	{
		DRAWTYPE_GLES2_CLEAR,
		DRAWTYPE_GLES2_RENDER
	};

	const DrawType renderClear[2] =
	{
		DRAWTYPE_GLES2_RENDER,
		DRAWTYPE_GLES2_CLEAR
	};

	const ResizeType resizeTypes[] =
	{
		RESIZETYPE_NONE,
		RESIZETYPE_BEFORE_SWAP,
		RESIZETYPE_AFTER_SWAP
	};

	vector< vector<DrawType> > frameDrawTypes;
	frameDrawTypes.push_back(vector<DrawType> (1, DRAWTYPE_GLES2_CLEAR));
	frameDrawTypes.push_back(vector<DrawType> (1, DRAWTYPE_GLES2_RENDER));
	frameDrawTypes.push_back(vector<DrawType> (2, DRAWTYPE_GLES2_CLEAR));
	frameDrawTypes.push_back(vector<DrawType> (2, DRAWTYPE_GLES2_RENDER));
	frameDrawTypes.push_back(vector<DrawType> (DE_ARRAY_BEGIN(clearRender), DE_ARRAY_END(clearRender)));
	frameDrawTypes.push_back(vector<DrawType> (DE_ARRAY_BEGIN(renderClear), DE_ARRAY_END(renderClear)));

	for (size_t resizeTypeNdx = 0; resizeTypeNdx < DE_LENGTH_OF_ARRAY(resizeTypes); resizeTypeNdx++)
	{
		const ResizeType		resizeType	= resizeTypes[resizeTypeNdx];
		TestCaseGroup* const	resizeGroup	= new TestCaseGroup(m_eglTestCtx, generateResizeGroupName(resizeType).c_str(), "");

		for (size_t drawTypeNdx = 0; drawTypeNdx < frameDrawTypes.size(); drawTypeNdx++)
		{
			string name = generateTestName(frameDrawTypes[drawTypeNdx]);
			resizeGroup->addChild(new SwapBuffersWithDamageTest(m_eglTestCtx, frameDrawTypes[drawTypeNdx], 4, resizeType, name.c_str(), ""));
		}

		for (size_t drawTypeNdx = 0; drawTypeNdx < frameDrawTypes.size(); drawTypeNdx++)
		{
			string name = "preserve_buffer_" + generateTestName(frameDrawTypes[drawTypeNdx]);
			resizeGroup->addChild(new SwapBuffersWithDamageAndPreserveBufferTest(m_eglTestCtx, frameDrawTypes[drawTypeNdx], 4, resizeType, name.c_str(), ""));
		}

		for (size_t drawTypeNdx = 0; drawTypeNdx < frameDrawTypes.size(); drawTypeNdx++)
		{
			string name = "buffer_age_" + generateTestName(frameDrawTypes[drawTypeNdx]);
			resizeGroup->addChild(new SwapBuffersWithDamageAndBufferAgeTest(m_eglTestCtx, frameDrawTypes[drawTypeNdx], 4, resizeType, name.c_str(),  ""));
		}

		addChild(resizeGroup);
	}
}

} // egl
} // deqp
