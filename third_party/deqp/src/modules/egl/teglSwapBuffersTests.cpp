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
 * \brief Test eglSwapBuffers() interaction with native window.
 *//*--------------------------------------------------------------------*/

#include "teglSwapBuffersTests.hpp"

#include "teglSimpleConfigCase.hpp"

#include "egluNativeWindow.hpp"
#include "egluUtil.hpp"
#include "egluUnique.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"

#include "deUniquePtr.hpp"
#include "deThread.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace deqp
{
namespace egl
{

using tcu::TestLog;
using std::string;
using std::vector;
using namespace eglw;

namespace
{

EGLContext createGLES2Context (const Library& egl, EGLDisplay display, EGLConfig config)
{
	EGLContext		context = EGL_NO_CONTEXT;
	const EGLint	attribList[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};


	EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));

	context = egl.createContext(display, config, EGL_NO_CONTEXT, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext() failed");
	TCU_CHECK(context);

	return context;
}

class SwapBuffersTest : public SimpleConfigCase
{
public:
						SwapBuffersTest		(EglTestContext& eglTestCtx, const NamedFilterList& filters);
						~SwapBuffersTest	(void);

private:
	void				executeForConfig	(EGLDisplay display, EGLConfig config);

	// Not allowed
						SwapBuffersTest		(const SwapBuffersTest&);
	SwapBuffersTest&	operator=			(const SwapBuffersTest&);
};


SwapBuffersTest::SwapBuffersTest (EglTestContext& eglTestCtx, const NamedFilterList& filters)
	: SimpleConfigCase(eglTestCtx, filters.getName(), filters.getDescription(), filters)
{
}

SwapBuffersTest::~SwapBuffersTest (void)
{
}

string getConfigIdString (const Library& egl, EGLDisplay display, EGLConfig config)
{
	std::ostringstream	stream;
	EGLint				id;

	EGLU_CHECK_CALL(egl, getConfigAttrib(display, config , EGL_CONFIG_ID, &id));

	stream << id;

	return stream.str();
}

deUint32 createGLES2Program (const glw::Functions& gl, TestLog& log)
{
	const char* const vertexShaderSource =
	"attribute highp vec2 a_pos;\n"
	"void main (void)\n"
	"{\n"
	"\tgl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}";

	const char* const fragmentShaderSource =
	"void main (void)\n"
	"{\n"
	"\tgl_FragColor = vec4(0.9, 0.1, 0.4, 1.0);\n"
	"}";

	deUint32	program			= 0;
	deUint32	vertexShader	= 0;
	deUint32	fragmentShader	= 0;

	deInt32		vertexCompileStatus;
	string		vertexInfoLog;
	deInt32		fragmentCompileStatus;
	string		fragmentInfoLog;
	deInt32		linkStatus;
	string		programInfoLog;

	try
	{
		program			= gl.createProgram();
		vertexShader	= gl.createShader(GL_VERTEX_SHADER);
		fragmentShader	= gl.createShader(GL_FRAGMENT_SHADER);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create shaders and program");

		gl.shaderSource(vertexShader, 1, &vertexShaderSource, DE_NULL);
		gl.compileShader(vertexShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup vertex shader");

		gl.shaderSource(fragmentShader, 1, &fragmentShaderSource, DE_NULL);
		gl.compileShader(fragmentShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup fragment shader");

		{
			deInt32		infoLogLength = 0;

			gl.getShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexCompileStatus);
			gl.getShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLogLength);

			vertexInfoLog.resize(infoLogLength, '\0');

			gl.getShaderInfoLog(vertexShader, (glw::GLsizei)vertexInfoLog.length(), &infoLogLength, &(vertexInfoLog[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get vertex shader compile info");

			vertexInfoLog.resize(infoLogLength);
		}

		{
			deInt32		infoLogLength = 0;

			gl.getShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentCompileStatus);
			gl.getShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLogLength);

			fragmentInfoLog.resize(infoLogLength, '\0');

			gl.getShaderInfoLog(fragmentShader, (glw::GLsizei)fragmentInfoLog.length(), &infoLogLength, &(fragmentInfoLog[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get fragment shader compile info");

			fragmentInfoLog.resize(infoLogLength);
		}

		gl.attachShader(program, vertexShader);
		gl.attachShader(program, fragmentShader);
		gl.linkProgram(program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup program");

		{
			deInt32		infoLogLength = 0;

			gl.getProgramiv(program, GL_LINK_STATUS, &linkStatus);
			gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

			programInfoLog.resize(infoLogLength, '\0');

			gl.getProgramInfoLog(program, (glw::GLsizei)programInfoLog.length(), &infoLogLength, &(programInfoLog[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get program link info");

			programInfoLog.resize(infoLogLength);
		}

		if (linkStatus == 0 || vertexCompileStatus == 0 || fragmentCompileStatus == 0)
		{

			log.startShaderProgram(linkStatus != 0, programInfoLog.c_str());

			log << TestLog::Shader(QP_SHADER_TYPE_VERTEX, vertexShaderSource, vertexCompileStatus != 0, vertexInfoLog);
			log << TestLog::Shader(QP_SHADER_TYPE_FRAGMENT, fragmentShaderSource, fragmentCompileStatus != 0, fragmentInfoLog);

			log.endShaderProgram();
		}

		gl.deleteShader(vertexShader);
		gl.deleteShader(fragmentShader);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete shaders");

		TCU_CHECK(linkStatus != 0 && vertexCompileStatus != 0 && fragmentCompileStatus != 0);
	}
	catch (...)
	{
		if (program)
			gl.deleteProgram(program);

		if (vertexShader)
			gl.deleteShader(vertexShader);

		if (fragmentShader)
			gl.deleteShader(fragmentShader);

		throw;
	}

	return program;
}

bool checkColor (tcu::TestLog& log, const tcu::TextureLevel& screen, const tcu::Vec4& color)
{
	const tcu::Vec4 threshold(0.01f, 0.01f, 0.01f, 1.00f);

	for (int y = 0; y < screen.getHeight(); y++)
	{
		for (int x = 0; x < screen.getWidth(); x++)
		{
			const tcu::Vec4	pixel(screen.getAccess().getPixel(x, y));
			const tcu::Vec4	diff(abs(pixel - color));

			if (!boolAll(lessThanEqual(diff, threshold)))
			{
				log << TestLog::Message << "Unexpected color values read from screen expected: " << color << TestLog::EndMessage;
				log << TestLog::Image("Screen", "Screen", screen.getAccess());
				return false;
			}
		}
	}

	return true;
}

void SwapBuffersTest::executeForConfig (EGLDisplay display, EGLConfig config)
{
	const Library&						egl			= m_eglTestCtx.getLibrary();
	const string						configIdStr	(getConfigIdString(egl, display, config));
	tcu::ScopedLogSection				logSection	(m_testCtx.getLog(), ("Config ID " + configIdStr).c_str(), ("Config ID " + configIdStr).c_str());
	const int							waitFrames	= 5;
	const eglu::NativeWindowFactory&	factory		= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

	if ((factory.getCapabilities() & eglu::NativeWindow::CAPABILITY_READ_SCREEN_PIXELS) == 0)
		TCU_THROW(NotSupportedError, "eglu::NativeWindow doesn't support readScreenPixels()");

	{
		TestLog& log = m_testCtx.getLog();

		log << TestLog::Message << "EGL_RED_SIZE: "		<< eglu::getConfigAttribInt(egl, display, config, EGL_RED_SIZE)		<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_GREEN_SIZE: "	<< eglu::getConfigAttribInt(egl, display, config, EGL_GREEN_SIZE)	<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_BLUE_SIZE: "	<< eglu::getConfigAttribInt(egl, display, config, EGL_BLUE_SIZE)	<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_ALPHA_SIZE: "	<< eglu::getConfigAttribInt(egl, display, config, EGL_ALPHA_SIZE)	<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_DEPTH_SIZE: "	<< eglu::getConfigAttribInt(egl, display, config, EGL_DEPTH_SIZE)	<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_STENCIL_SIZE: "	<< eglu::getConfigAttribInt(egl, display, config, EGL_STENCIL_SIZE)	<< TestLog::EndMessage;
		log << TestLog::Message << "EGL_SAMPLES: "		<< eglu::getConfigAttribInt(egl, display, config, EGL_SAMPLES)		<< TestLog::EndMessage;

		log << TestLog::Message << "Waiting " << waitFrames * 16 << "ms after eglSwapBuffers() and glFinish() for frame to become visible" << TestLog::EndMessage;
	}

	de::UniquePtr<eglu::NativeWindow>	window	(factory.createWindow(&m_eglTestCtx.getNativeDisplay(), display, config, DE_NULL, eglu::WindowParams(128, 128, eglu::WindowParams::VISIBILITY_VISIBLE)));

	eglu::UniqueSurface					surface	(egl, display, eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, display, config, DE_NULL));
	eglu::UniqueContext					context	(egl, display, createGLES2Context(egl, display, config));
	glw::Functions						gl;
	deUint32							program = 0;

	tcu::TextureLevel					whiteFrame;
	tcu::TextureLevel					blackFrame;
	tcu::TextureLevel					frameBegin;
	tcu::TextureLevel					frameEnd;

	m_eglTestCtx.initGLFunctions(&gl, glu::ApiType::es(2,0));
	EGLU_CHECK_CALL(egl, makeCurrent(display, *surface, *surface, *context));

	try
	{
		const float positions1[] = {
			 0.00f,  0.00f,
			 0.75f,  0.00f,
			 0.75f,  0.75f,

			 0.75f,  0.75f,
			 0.00f,  0.75f,
			 0.00f,  0.00f
		};

		const float positions2[] = {
			-0.75f, -0.75f,
			 0.00f, -0.75f,
			 0.00f,  0.00f,

			 0.00f,  0.00f,
			-0.75f,  0.00f,
			-0.75f, -0.75f
		};

		deUint32 posLocation;

		program	= createGLES2Program(gl, m_testCtx.getLog());

		gl.useProgram(program);
		posLocation	= gl.getAttribLocation(program, "a_pos");
		gl.enableVertexAttribArray(posLocation);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup shader program for rendering");

		// Clear screen to white and check that sceen is white
		gl.clearColor(1.0f, 1.0f, 1.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear surface");

		EGLU_CHECK_CALL(egl, swapBuffers(display, *surface));
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish() failed");
		deSleep(waitFrames * 16);
		window->processEvents();
		window->readScreenPixels(&whiteFrame);

		if (!checkColor(m_testCtx.getLog(), whiteFrame, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f)))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Couldn't reliably read pixels from screen");
			return;
		}

		// Clear screen to black and check that sceen is black
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear surface");

		EGLU_CHECK_CALL(egl, swapBuffers(display, *surface));
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish() failed");
		deSleep(waitFrames * 16);
		window->processEvents();
		window->readScreenPixels(&blackFrame);

		if (!checkColor(m_testCtx.getLog(), blackFrame, tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Couldn't reliably read pixels from screen");
			return;
		}

		gl.clearColor(0.7f, 1.0f, 0.3f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear surface");

		gl.vertexAttribPointer(posLocation, 2, GL_FLOAT, GL_FALSE, 0, positions1);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to render");

		EGLU_CHECK_CALL(egl, swapBuffers(display, *surface));
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish() failed");
		deSleep(waitFrames * 16);
		window->processEvents();
		window->readScreenPixels(&frameBegin);

		gl.clearColor(0.7f, 0.7f, 1.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear surface");

		gl.vertexAttribPointer(posLocation, 2, GL_FLOAT, GL_FALSE, 0, positions2);
		gl.drawArrays(GL_TRIANGLES, 0, 6);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to render");

		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish() failed");
		deSleep(waitFrames * 16);
		window->readScreenPixels(&frameEnd);

		EGLU_CHECK_CALL(egl, swapBuffers(display, *surface));
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFinish() failed");
		deSleep(waitFrames * 16);
		window->processEvents();

		gl.disableVertexAttribArray(posLocation);
		gl.useProgram(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to release program state");

		gl.deleteProgram(program);
		program = 0;
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram()");

		if (!tcu::intThresholdCompare(m_testCtx.getLog(), "Compare end of frame against beginning of frame" , "Compare end of frame against beginning of frame", frameBegin.getAccess(), frameEnd.getAccess(), tcu::UVec4(0, 0, 0, 0), tcu::COMPARE_LOG_RESULT))
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Screen pixels changed during frame");
	}
	catch (...)
	{
		if (program != 0)
			gl.deleteProgram(program);

		EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		throw;
	}

	EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
}

} // anonymous

SwapBuffersTests::SwapBuffersTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "swap_buffers", "Swap buffers tests")
{
}

static bool isWindow (const eglu::CandidateConfig& c) { return (c.surfaceType() & EGL_WINDOW_BIT) != 0; }

void SwapBuffersTests::init (void)
{
	eglu::FilterList baseFilters;
	baseFilters << isWindow;

	vector<NamedFilterList> filterLists;
	getDefaultFilterLists(filterLists, baseFilters);

	for (vector<NamedFilterList>::iterator i = filterLists.begin(); i != filterLists.end(); i++)
		addChild(new SwapBuffersTest(m_eglTestCtx, *i));
}

} // egl
} // deqp
