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
 * \brief Test EXT_buffer_age
 *//*--------------------------------------------------------------------*/

#include "teglBufferAgeTests.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
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

class GLES2Renderer;

class ReferenceRenderer;

class BufferAgeTest : public TestCase
{
public:
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

								BufferAgeTest	(EglTestContext&			eglTestCtx,
												 bool						preserveColorBuffer,
												 const vector<DrawType>&	oddFrameDrawType,
												 const vector<DrawType>&	evenFrameDrawType,
												 ResizeType					resizeType,
												 const char*				name,
												 const char*				description);

								~BufferAgeTest	(void);

	void						init			(void);
	void						deinit			(void);
	IterateResult				iterate			(void);

private:
	void						initEGLSurface (EGLConfig config);
	void						initEGLContext (EGLConfig config);

	const int					m_seed;
	const bool					m_preserveColorBuffer;
	const vector<DrawType>		m_oddFrameDrawType;
	const vector<DrawType>		m_evenFrameDrawType;
	const ResizeType			m_resizeType;

	EGLDisplay					m_eglDisplay;
	eglu::NativeWindow*			m_window;
	EGLSurface					m_eglSurface;
	EGLConfig					m_eglConfig;
	EGLContext					m_eglContext;
	glw::Functions				m_gl;

	GLES2Renderer*				m_gles2Renderer;
	ReferenceRenderer*			m_refRenderer;

};

struct ColoredRect
{
public:
							ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_);
	IVec2					bottomLeft;
	IVec2					topRight;
	Color					color;
};

ColoredRect::ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_)
	: bottomLeft(bottomLeft_)
	, topRight	(topRight_)
	, color		(color_)
{
}

struct DrawCommand
{
							DrawCommand (const BufferAgeTest::DrawType drawType_, const ColoredRect& rect_);
	BufferAgeTest::DrawType drawType;
	ColoredRect				rect;
};

DrawCommand::DrawCommand (const BufferAgeTest::DrawType drawType_, const ColoredRect& rect_)
	: drawType(drawType_)
	, rect    (rect_)
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
	: width(width_)
	, height(height_)
{
}


// (x1,y1) lie in the lower-left quadrant while (x2,y2) lie in the upper-right.
// the coords are multiplied by 4 to amplify the minimial difference between coords to 4 (if not zero)
// to avoid the situation where two edges are too close to each other which makes the rounding error
// intoleratable by compareToReference()
void generateRandomFrame (Frame* dst, const vector<BufferAgeTest::DrawType>& drawTypes, de::Random& rnd)
{
	for (size_t ndx = 0; ndx < drawTypes.size(); ndx++)
	{
		const int			x1			= rnd.getInt(0, (dst->width-1)/8) * 4;
		const int			y1			= rnd.getInt(0, (dst->height-1)/8) * 4;
		const int			x2			= rnd.getInt((dst->width-1)/8, (dst->width-1)/4) * 4;
		const int			y2			= rnd.getInt((dst->height-1)/8, (dst->height-1)/4) * 4;
		const GLubyte		r			= rnd.getUint8();
		const GLubyte		g			= rnd.getUint8();
		const GLubyte		b			= rnd.getUint8();
		const ColoredRect	coloredRect	(IVec2(x1, y1), IVec2(x2, y2), Color(r, g, b));
		const DrawCommand	drawCommand (drawTypes[ndx], coloredRect);
		(*dst).draws.push_back(drawCommand);
	}
}

typedef vector<Frame> FrameSequence;

//helper function declaration
EGLConfig		getEGLConfig					(const Library& egl, EGLDisplay eglDisplay, bool preserveColorBuffer);
void			clearColorScreen				(const glw::Functions& gl, const tcu::Vec4& clearColor);
void			clearColorReference				(tcu::Surface* ref, const tcu::Vec4& clearColor);
void			readPixels						(const glw::Functions& gl, tcu::Surface* screen);
float			windowToDeviceCoordinates		(int x, int length);
bool			compareToReference				(tcu::TestLog& log, const tcu::Surface& reference, const tcu::Surface& buffer, int frameNdx, int bufferNum);
vector<int>		getFramesOnBuffer				(const vector<int>& bufferAges, int frameNdx);

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

GLES2Renderer::GLES2Renderer (const glw::Functions& gl)
	: m_gl		  (gl)
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
		if (frame.draws[drawNdx].drawType == BufferAgeTest::DRAWTYPE_GLES2_RENDER)
		{
			float x1 = windowToDeviceCoordinates(coloredRect.bottomLeft.x(), width);
			float y1 = windowToDeviceCoordinates(coloredRect.bottomLeft.y(), height);
			float x2 = windowToDeviceCoordinates(coloredRect.topRight.x(), width);
			float y2 = windowToDeviceCoordinates(coloredRect.topRight.y(), height);

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

			m_gl.vertexAttribPointer(m_coordLoc, 4, GL_FLOAT, GL_FALSE, 0, coords);
			m_gl.vertexAttribPointer(m_colorLoc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, colors);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to set attribute pointers");

			m_gl.drawArrays(GL_TRIANGLES, 0, DE_LENGTH_OF_ARRAY(coords)/4);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays(), failed");

			m_gl.disableVertexAttribArray(m_coordLoc);
			m_gl.disableVertexAttribArray(m_colorLoc);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to disable attributes");

			m_gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");
		}
		else if (frame.draws[drawNdx].drawType == BufferAgeTest::DRAWTYPE_GLES2_CLEAR)
		{
			m_gl.enable(GL_SCISSOR_TEST);
			m_gl.scissor(coloredRect.bottomLeft.x(), coloredRect.bottomLeft.y(),
						 coloredRect.topRight.x()-coloredRect.bottomLeft.x(), coloredRect.topRight.y()-coloredRect.bottomLeft.y());
			m_gl.clearColor(coloredRect.color.x()/255.0f, coloredRect.color.y()/255.0f, coloredRect.color.z()/255.0f, 1.0f);
			m_gl.clear(GL_COLOR_BUFFER_BIT);
			m_gl.disable(GL_SCISSOR_TEST);
		}
		else
			DE_ASSERT(false);
	}
}

class ReferenceRenderer
{
public:
						ReferenceRenderer	(void);
	void				render				(tcu::Surface* target, const Frame& frame) const;
private:
						ReferenceRenderer	(const ReferenceRenderer&);
	ReferenceRenderer&	operator=			(const ReferenceRenderer&);
};

ReferenceRenderer::ReferenceRenderer(void)
{
}

void ReferenceRenderer::render (tcu::Surface* target, const Frame& frame) const
{
	for (size_t drawNdx = 0; drawNdx < frame.draws.size(); drawNdx++)
	{
		const ColoredRect& coloredRect = frame.draws[drawNdx].rect;
		if (frame.draws[drawNdx].drawType == BufferAgeTest::DRAWTYPE_GLES2_RENDER || frame.draws[drawNdx].drawType == BufferAgeTest::DRAWTYPE_GLES2_CLEAR)
		{
			// tcu does not support degenerate subregions. Since they correspond to no-op rendering, just skip them.
			if (coloredRect.bottomLeft.x() == coloredRect.topRight.x() || coloredRect.bottomLeft.y() == coloredRect.topRight.y())
				continue;

			const tcu::UVec4 color(coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), 255);
			tcu::clear(tcu::getSubregion(target->getAccess(), coloredRect.bottomLeft.x(), coloredRect.bottomLeft.y(),
										 coloredRect.topRight.x()-coloredRect.bottomLeft.x(), coloredRect.topRight.y()-coloredRect.bottomLeft.y()), color);
		}
		else
			DE_ASSERT(false);
	}
}

BufferAgeTest::BufferAgeTest (EglTestContext&			eglTestCtx,
							  bool						preserveColorBuffer,
							  const vector<DrawType>&	oddFrameDrawType,
							  const vector<DrawType>&	evenFrameDrawType,
							  ResizeType				resizeType,
							  const char*				name,
							  const char*				description)
	: TestCase				(eglTestCtx, name, description)
	, m_seed				(deStringHash(name))
	, m_preserveColorBuffer (preserveColorBuffer)
	, m_oddFrameDrawType	(oddFrameDrawType)
	, m_evenFrameDrawType	(evenFrameDrawType)
	, m_resizeType			(resizeType)
	, m_eglDisplay			(EGL_NO_DISPLAY)
	, m_window				(DE_NULL)
	, m_eglSurface			(EGL_NO_SURFACE)
	, m_eglContext			(EGL_NO_CONTEXT)
	, m_gles2Renderer		(DE_NULL)
	, m_refRenderer			(DE_NULL)
{
}

BufferAgeTest::~BufferAgeTest (void)
{
	deinit();
}

void BufferAgeTest::init (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	m_eglDisplay = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_eglConfig	 = getEGLConfig(m_eglTestCtx.getLibrary(), m_eglDisplay, m_preserveColorBuffer);

	if (m_eglConfig == DE_NULL)
		TCU_THROW(NotSupportedError, "No supported config found");

	//create surface and context and make them current
	initEGLSurface(m_eglConfig);
	initEGLContext(m_eglConfig);

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));

	if (eglu::hasExtension(egl, m_eglDisplay, "EGL_EXT_buffer_age") == false)
		TCU_THROW(NotSupportedError, "EGL_EXT_buffer_age is not supported");

	m_gles2Renderer = new GLES2Renderer(m_gl);
	m_refRenderer   = new ReferenceRenderer();
}

void BufferAgeTest::deinit (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	delete m_refRenderer;
	m_refRenderer = DE_NULL;

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

void BufferAgeTest::initEGLSurface (EGLConfig config)
{
	const eglu::NativeWindowFactory& factory = eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
	m_window = factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_eglDisplay, config, DE_NULL,
									eglu::WindowParams(480, 480, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
	m_eglSurface = eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_eglDisplay, config, DE_NULL);
}

void BufferAgeTest::initEGLContext (EGLConfig config)
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
	DE_ASSERT(m_eglSurface != EGL_NO_SURFACE);
	egl.makeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");
}

// return indices of frames that have been written to the given buffer
vector<int> getFramesOnBuffer (const vector<int>& bufferAges, int frameNdx)
{
	DE_ASSERT(frameNdx < (int)bufferAges.size());
	vector<int> frameOnBuffer;
	int			age = bufferAges[frameNdx];
	while (age != 0)
	{
		frameNdx = frameNdx - age;
		DE_ASSERT(frameNdx >= 0);
		frameOnBuffer.push_back(frameNdx);
		age = bufferAges[frameNdx];
	}

	reverse(frameOnBuffer.begin(), frameOnBuffer.end());
	return frameOnBuffer;
}

TestCase::IterateResult BufferAgeTest::iterate (void)
{
	de::Random		rnd					(m_seed);
	const Library&	egl					= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log					= m_testCtx.getLog();
	const int		width				= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_WIDTH);
	const int		height				= eglu::querySurfaceInt(egl, m_eglDisplay, m_eglSurface, EGL_HEIGHT);
	const float		clearRed			= rnd.getFloat();
	const float		clearGreen			= rnd.getFloat();
	const float		clearBlue			= rnd.getFloat();
	const tcu::Vec4	clearColor			(clearRed, clearGreen, clearBlue, 1.0f);
	const int		numFrames			= 20;
	FrameSequence	frameSequence;
	vector<int>		bufferAges;

	if (m_preserveColorBuffer)
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));
	else
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));

	for (int frameNdx = 0; frameNdx < numFrames; frameNdx++)
	{
		tcu::Surface					currentBuffer			(width, height);
		tcu::Surface					refBuffer				(width, height);
		Frame							newFrame				(width, height);
		EGLint							currentBufferAge		= -1;

		if (frameNdx % 2 == 0)
			generateRandomFrame(&newFrame, m_evenFrameDrawType, rnd);
		else
			generateRandomFrame(&newFrame, m_oddFrameDrawType, rnd);

		frameSequence.push_back(newFrame);

		EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_EXT, &currentBufferAge));

		if (currentBufferAge > frameNdx || currentBufferAge < 0) // invalid buffer age
		{
			std::ostringstream stream;
			stream << "Fail, the age is invalid. Age: " << currentBufferAge << ", frameNdx: " << frameNdx;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, stream.str().c_str());
			return STOP;
		}

		if (frameNdx > 0 && m_preserveColorBuffer && currentBufferAge != 1)
		{
			std::ostringstream stream;
			stream << "Fail, EGL_BUFFER_PRESERVED is set to true, but buffer age is: " << currentBufferAge << " (should be 1)";
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, stream.str().c_str());
			return STOP;
		}

		bufferAges.push_back(currentBufferAge);
		DE_ASSERT((int)bufferAges.size() == frameNdx+1);

		// during first half, just keep rendering without reading pixel back to mimic ordinary use case
		if (frameNdx < numFrames/2)
		{
			if (currentBufferAge == 0)
				clearColorScreen(m_gl, clearColor);

			m_gles2Renderer->render(width, height, newFrame);

			if (m_resizeType == RESIZETYPE_BEFORE_SWAP)
			{
				if (frameNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}

			EGLU_CHECK_CALL(egl, swapBuffers(m_eglDisplay, m_eglSurface));

			if (m_resizeType == RESIZETYPE_AFTER_SWAP)
			{
				if (frameNdx % 2 == 0)
					m_window->setSurfaceSize(IVec2(width*2, height/2));
				else
					m_window->setSurfaceSize(IVec2(height/2, width*2));
			}

			continue;
		}

		// do verification in the second half
		if (currentBufferAge > 0) //buffer contain previous content, need to verify
		{
			const vector<int> framesOnBuffer = getFramesOnBuffer(bufferAges, frameNdx);
			readPixels(m_gl, &currentBuffer);
			clearColorReference(&refBuffer, clearColor);

			for (vector<int>::const_iterator it = framesOnBuffer.begin(); it != framesOnBuffer.end(); it++)
				m_refRenderer->render(&refBuffer, frameSequence[*it]);

			if (compareToReference(log, refBuffer, currentBuffer, frameNdx, frameNdx-currentBufferAge) == false)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail, buffer content is not well preserved when age > 0");
				return STOP;
			}
		}
		else // currentBufferAge == 0, content is undefined, clear the buffer, currentBufferAge < 0 is ruled out at the beginning
		{
			clearColorScreen(m_gl, clearColor);
			clearColorReference(&refBuffer, clearColor);
		}

		m_gles2Renderer->render(width, height, newFrame);
		m_refRenderer->render(&refBuffer, newFrame);

		readPixels(m_gl, &currentBuffer);

		if (compareToReference(log, refBuffer, currentBuffer, frameNdx, frameNdx) == false)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail, render result is wrong");
			return STOP;
		}

		if (m_resizeType == RESIZETYPE_BEFORE_SWAP)
		{
			if (frameNdx % 2 == 0)
				m_window->setSurfaceSize(IVec2(width*2, height/2));
			else
				m_window->setSurfaceSize(IVec2(height/2, width*2));
		}

		EGLU_CHECK_CALL(egl, swapBuffers(m_eglDisplay, m_eglSurface));

		if (m_resizeType == RESIZETYPE_AFTER_SWAP)
		{
			if (frameNdx % 2 == 0)
				m_window->setSurfaceSize(IVec2(width*2, height/2));
			else
				m_window->setSurfaceSize(IVec2(height/2, width*2));
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

string generateDrawTypeName (const vector<BufferAgeTest::DrawType>& drawTypes)
{
	std::ostringstream stream;
	if (drawTypes.size() == 0)
		return string("_none");

	for (size_t ndx = 0; ndx < drawTypes.size(); ndx++)
	{
		if (drawTypes[ndx] == BufferAgeTest::DRAWTYPE_GLES2_RENDER)
			stream << "_render";
		else if (drawTypes[ndx] == BufferAgeTest::DRAWTYPE_GLES2_CLEAR)
			stream << "_clear";
		else
			DE_ASSERT(false);
	}
	return stream.str();
}

string generateTestName (const vector<BufferAgeTest::DrawType>& oddFrameDrawType, const vector<BufferAgeTest::DrawType>& evenFrameDrawType)
{
	return "odd" + generateDrawTypeName(oddFrameDrawType) + "_even" + generateDrawTypeName(evenFrameDrawType);
}

string generateResizeGroupName (BufferAgeTest::ResizeType resizeType)
{
	switch (resizeType)
	{
		case BufferAgeTest::RESIZETYPE_NONE:
			return "no_resize";

		case BufferAgeTest::RESIZETYPE_AFTER_SWAP:
			return "resize_after_swap";

		case BufferAgeTest::RESIZETYPE_BEFORE_SWAP:
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

EGLConfig getEGLConfig (const Library& egl, EGLDisplay eglDisplay, bool preserveColorBuffer)
{
	eglu::FilterList filters;
	filters << isWindow << isES2Renderable;
	if (preserveColorBuffer)
		filters << hasPreserveSwap;
	return eglu::chooseSingleConfig(egl, eglDisplay, filters);
}

void clearColorScreen (const glw::Functions& gl, const tcu::Vec4& clearColor)
{
	gl.clearColor(clearColor.x(), clearColor.y(), clearColor.z(), clearColor.w());
	gl.clear(GL_COLOR_BUFFER_BIT);
}

void clearColorReference (tcu::Surface* ref, const tcu::Vec4& clearColor)
{
	tcu::clear(ref->getAccess(), clearColor);
}

void readPixels (const glw::Functions& gl, tcu::Surface* screen)
{
	gl.readPixels(0, 0, screen->getWidth(), screen->getHeight(),  GL_RGBA, GL_UNSIGNED_BYTE, screen->getAccess().getDataPtr());
}

float windowToDeviceCoordinates (int x, int length)
{
	return (2.0f * float(x) / float(length)) - 1.0f;
}

bool compareToReference (tcu::TestLog& log,	 const tcu::Surface& reference, const tcu::Surface& buffer, int frameNdx, int bufferNum)
{
	std::ostringstream stream;
	stream << "FrameNdx = " << frameNdx << ", compare current buffer (numbered: " << bufferNum << ") to reference";
	return tcu::intThresholdPositionDeviationCompare(log, "buffer age test", stream.str().c_str(), reference.getAccess(), buffer.getAccess(),
													 tcu::UVec4(8, 8, 8, 0), tcu::IVec3(2,2,0), true, tcu::COMPARE_LOG_RESULT);
}

} // anonymous

BufferAgeTests::BufferAgeTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "buffer_age", "Color buffer age tests")
{
}

void BufferAgeTests::init (void)
{
	const BufferAgeTest::DrawType clearRender[] =
	{
		BufferAgeTest::DRAWTYPE_GLES2_CLEAR,
		BufferAgeTest::DRAWTYPE_GLES2_RENDER
	};

	const BufferAgeTest::DrawType renderClear[] =
	{
		BufferAgeTest::DRAWTYPE_GLES2_RENDER,
		BufferAgeTest::DRAWTYPE_GLES2_CLEAR
	};

	const BufferAgeTest::ResizeType resizeTypes[] =
	{
		BufferAgeTest::RESIZETYPE_NONE,
		BufferAgeTest::RESIZETYPE_BEFORE_SWAP,
		BufferAgeTest::RESIZETYPE_AFTER_SWAP
	};

	vector< vector<BufferAgeTest::DrawType> > frameDrawTypes;
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> ());
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (1, BufferAgeTest::DRAWTYPE_GLES2_CLEAR));
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (1, BufferAgeTest::DRAWTYPE_GLES2_RENDER));
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (2, BufferAgeTest::DRAWTYPE_GLES2_CLEAR));
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (2, BufferAgeTest::DRAWTYPE_GLES2_RENDER));
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (DE_ARRAY_BEGIN(clearRender), DE_ARRAY_END(clearRender)));
	frameDrawTypes.push_back(vector<BufferAgeTest::DrawType> (DE_ARRAY_BEGIN(renderClear), DE_ARRAY_END(renderClear)));

	for (int preserveNdx = 0; preserveNdx < 2; preserveNdx++)
	{
		const bool				preserve		= (preserveNdx == 0);
		TestCaseGroup* const	preserveGroup	= new TestCaseGroup(m_eglTestCtx, (preserve ? "preserve" : "no_preserve"), "");

		for (size_t resizeTypeNdx = 0; resizeTypeNdx < DE_LENGTH_OF_ARRAY(resizeTypes); resizeTypeNdx++)
		{
			const BufferAgeTest::ResizeType	resizeType	= resizeTypes[resizeTypeNdx];
			TestCaseGroup* const			resizeGroup	= new TestCaseGroup(m_eglTestCtx, generateResizeGroupName(resizeType).c_str(), "");

			for (size_t evenNdx = 0; evenNdx < frameDrawTypes.size(); evenNdx++)
			{
				const vector<BufferAgeTest::DrawType>& evenFrameDrawType = frameDrawTypes[evenNdx];

				for (size_t oddNdx = evenNdx; oddNdx < frameDrawTypes.size(); oddNdx++)
				{
					const vector<BufferAgeTest::DrawType>&	oddFrameDrawType	= frameDrawTypes[oddNdx];
					const std::string						name				= generateTestName(oddFrameDrawType, evenFrameDrawType);
					resizeGroup->addChild(new BufferAgeTest(m_eglTestCtx, preserve, oddFrameDrawType, evenFrameDrawType, BufferAgeTest::RESIZETYPE_NONE, name.c_str(), ""));
				}
			}

			preserveGroup->addChild(resizeGroup);
		}
		addChild(preserveGroup);
	}
}

} // egl
} // deqp
