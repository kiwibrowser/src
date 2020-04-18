/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
 *
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Test KHR_wide_color
 *//*--------------------------------------------------------------------*/

#include "teglWideColorTests.hpp"

#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"

#include "egluNativeWindow.hpp"
#include "egluStrUtil.hpp"
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

#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"

#include <string>
#include <vector>
#include <sstream>

using std::string;
using std::vector;
using glw::GLubyte;
using glw::GLfloat;
using tcu::IVec2;

using namespace eglw;

namespace deqp
{
namespace egl
{
namespace
{

typedef tcu::Vec4 Color;

class GLES2Renderer;

class ReferenceRenderer;

class WideColorTests : public TestCaseGroup
{
public:
						WideColorTests		(EglTestContext& eglTestCtx);
	void				init				(void);

private:
						WideColorTests		(const WideColorTests&);
	WideColorTests&		operator=			(const WideColorTests&);
};

class WideColorTest : public TestCase
{
public:
	enum DrawType
	{
		DRAWTYPE_GLES2_CLEAR,
		DRAWTYPE_GLES2_RENDER
	};

						WideColorTest				(EglTestContext& eglTestCtx, const char* name, const char* description);
						~WideColorTest				(void);

	void				init						(void);
	void				deinit						(void);
	void				checkPixelFloatSupport		(void);
	void				checkColorSpaceSupport		(void);
	void				checkDisplayP3Support		(void);
	void				check1010102Support			(void);
	void				checkFP16Support			(void);
	void				checkSCRGBSupport			(void);
	void				checkSCRGBLinearSupport		(void);

protected:
	void				initEGLSurface				(EGLConfig config);
	void				initEGLContext				(EGLConfig config);

	EGLDisplay			m_eglDisplay;
	glw::Functions		m_gl;
};

struct ColoredRect
{
public:
			ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_);
	IVec2	bottomLeft;
	IVec2	topRight;
	Color	color;
};

ColoredRect::ColoredRect (const IVec2& bottomLeft_, const IVec2& topRight_, const Color& color_)
	: bottomLeft	(bottomLeft_)
	, topRight		(topRight_)
	, color			(color_)
{
}

void clearColorScreen (const glw::Functions& gl, const Color& clearColor)
{
	gl.clearColor(clearColor.x(), clearColor.y(), clearColor.z(), clearColor.w());
	gl.clear(GL_COLOR_BUFFER_BIT);
}

float windowToDeviceCoordinates (int x, int length)
{
	return (2.0f * float(x) / float(length)) - 1.0f;
}

class GLES2Renderer
{
public:
							GLES2Renderer		(const glw::Functions& gl, int width, int height);
							~GLES2Renderer		(void);
	void					render				(const ColoredRect& coloredRect) const;

private:
							GLES2Renderer		(const GLES2Renderer&);
	GLES2Renderer&			operator=			(const GLES2Renderer&);

	const glw::Functions&	m_gl;
	glu::ShaderProgram		m_glProgram;
	glw::GLuint				m_coordLoc;
	glw::GLuint				m_colorLoc;
	glw::GLuint				m_bufWidth;
	glw::GLuint				m_bufHeight;
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

GLES2Renderer::GLES2Renderer (const glw::Functions& gl, int width, int height)
	: m_gl				(gl)
	, m_glProgram		(gl, getSources())
	, m_coordLoc		((glw::GLuint)-1)
	, m_colorLoc		((glw::GLuint)-1)
	, m_bufWidth		(width)
	, m_bufHeight		(height)
{
	m_colorLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_color");
	m_coordLoc = m_gl.getAttribLocation(m_glProgram.getProgram(), "a_pos");
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to get attribute locations");
}

GLES2Renderer::~GLES2Renderer (void)
{
}

void GLES2Renderer::render (const struct ColoredRect &coloredRect) const
{
	const float x1 = windowToDeviceCoordinates(coloredRect.bottomLeft.x(), m_bufWidth);
	const float y1 = windowToDeviceCoordinates(coloredRect.bottomLeft.y(), m_bufHeight);
	const float x2 = windowToDeviceCoordinates(coloredRect.topRight.x(), m_bufWidth);
	const float y2 = windowToDeviceCoordinates(coloredRect.topRight.y(), m_bufHeight);

	const glw::GLfloat coords[] =
	{
		x1, y1, 0.0f, 1.0f,
		x1, y2, 0.0f, 1.0f,
		x2, y2, 0.0f, 1.0f,

		x2, y2, 0.0f, 1.0f,
		x2, y1, 0.0f, 1.0f,
		x1, y1, 0.0f, 1.0f
	};

	const glw::GLfloat colors[] =
	{
		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),
		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),
		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),

		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),
		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),
		coloredRect.color.x(), coloredRect.color.y(), coloredRect.color.z(), coloredRect.color.w(),
	};

	m_gl.useProgram(m_glProgram.getProgram());
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");

	m_gl.enableVertexAttribArray(m_coordLoc);
	m_gl.enableVertexAttribArray(m_colorLoc);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to enable attributes");

	m_gl.vertexAttribPointer(m_coordLoc, 4, GL_FLOAT, GL_FALSE, 0, coords);
	m_gl.vertexAttribPointer(m_colorLoc, 4, GL_FLOAT, GL_TRUE, 0, colors);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to set attribute pointers");

	m_gl.drawArrays(GL_TRIANGLES, 0, DE_LENGTH_OF_ARRAY(coords)/4);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays(), failed");

	m_gl.disableVertexAttribArray(m_coordLoc);
	m_gl.disableVertexAttribArray(m_colorLoc);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "Failed to disable attributes");

	m_gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram() failed");
}

class ReferenceRenderer
{
public:
						ReferenceRenderer		(void);
private:
						ReferenceRenderer		(const ReferenceRenderer&);
	ReferenceRenderer&	operator=				(const ReferenceRenderer&);
};

WideColorTest::WideColorTest (EglTestContext& eglTestCtx, const char* name, const char* description)
	: TestCase				 (eglTestCtx, name, description)
	, m_eglDisplay			 (EGL_NO_DISPLAY)
{
}

WideColorTest::~WideColorTest (void)
{
	deinit();
}

void WideColorTest::init (void)
{
	m_eglDisplay		= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

void WideColorTest::checkPixelFloatSupport (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_EXT_pixel_format_float"))
		TCU_THROW(NotSupportedError, "EGL_EXT_pixel_format_float is not supported");
}

void WideColorTest::checkColorSpaceSupport (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_KHR_gl_colorspace"))
		TCU_THROW(NotSupportedError, "EGL_KHR_gl_colorspace is not supported");
}

void WideColorTest::checkDisplayP3Support (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_EXT_gl_colorspace_display_p3"))
		TCU_THROW(NotSupportedError, "EGL_EXT_gl_colorspace_display_p3 is not supported");
}

void WideColorTest::checkSCRGBSupport (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_EXT_gl_colorspace_scrgb"))
		TCU_THROW(NotSupportedError, "EGL_EXT_gl_colorspace_scrgb is not supported");
}

void WideColorTest::checkSCRGBLinearSupport (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (!eglu::hasExtension(egl, m_eglDisplay, "EGL_EXT_gl_colorspace_scrgb_linear"))
		TCU_THROW(NotSupportedError, "EGL_EXT_gl_colorspace_scrgb_linear is not supported");
}

void WideColorTest::check1010102Support (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log	= m_testCtx.getLog();

	const EGLint attribList[] =
	{
		EGL_SURFACE_TYPE,				EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					10,
		EGL_GREEN_SIZE,					10,
		EGL_BLUE_SIZE,					10,
		EGL_ALPHA_SIZE,					2,
		EGL_NONE,						EGL_NONE
	};
	EGLint numConfigs = 0;
	EGLConfig config;

	// Query from EGL implementation
	EGLU_CHECK_CALL(egl, chooseConfig(m_eglDisplay, &attribList[0], DE_NULL, 0, &numConfigs));

	if (numConfigs <= 0)
	{
		log << tcu::TestLog::Message << "No configs returned." << tcu::TestLog::EndMessage;
		TCU_THROW(NotSupportedError, "10:10:10:2 pixel format is not supported");
	}

	log << tcu::TestLog::Message << numConfigs << " configs returned" << tcu::TestLog::EndMessage;

	EGLU_CHECK_CALL(egl, chooseConfig(m_eglDisplay, &attribList[0], &config, 1, &numConfigs));
	if (numConfigs > 1)
	{
		log << tcu::TestLog::Message << "Fail, more configs returned than requested." << tcu::TestLog::EndMessage;
		TCU_FAIL("Too many configs returned");
	}

	EGLint components[4];

	EGLU_CHECK_CALL(egl, getConfigAttrib(m_eglDisplay, config, EGL_RED_SIZE, &components[0]));
	EGLU_CHECK_CALL(egl, getConfigAttrib(m_eglDisplay, config, EGL_GREEN_SIZE, &components[1]));
	EGLU_CHECK_CALL(egl, getConfigAttrib(m_eglDisplay, config, EGL_BLUE_SIZE, &components[2]));
	EGLU_CHECK_CALL(egl, getConfigAttrib(m_eglDisplay, config, EGL_ALPHA_SIZE, &components[3]));

	TCU_CHECK_MSG(components[0] == 10, "Missing 10bit deep red channel");
	TCU_CHECK_MSG(components[1] == 10, "Missing 10bit deep green channel");
	TCU_CHECK_MSG(components[2] == 10, "Missing 10bit deep blue channel");
	TCU_CHECK_MSG(components[3] == 2, "Missing 2bit deep alpha channel");
}

void WideColorTest::checkFP16Support (void)
{
	const Library&	egl			= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log			= m_testCtx.getLog();
	EGLint			numConfigs	= 0;
	EGLConfig		config;

	const EGLint attribList[] =
	{
		EGL_SURFACE_TYPE,			  EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,		  EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,				  16,
		EGL_GREEN_SIZE,				  16,
		EGL_BLUE_SIZE,				  16,
		EGL_ALPHA_SIZE,				  16,
		EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
		EGL_NONE,					  EGL_NONE
	};

	// Query from EGL implementation
	EGLU_CHECK_CALL(egl, chooseConfig(m_eglDisplay, &attribList[0], DE_NULL, 0, &numConfigs));

	if (numConfigs <= 0)
	{
		log << tcu::TestLog::Message << "No configs returned." << tcu::TestLog::EndMessage;
		TCU_THROW(NotSupportedError, "16:16:16:16 pixel format is not supported");
	}

	log << tcu::TestLog::Message << numConfigs << " configs returned" << tcu::TestLog::EndMessage;

	EGLBoolean success = egl.chooseConfig(m_eglDisplay, &attribList[0], &config, 1, &numConfigs);
	if (success != EGL_TRUE)
	{
		log << tcu::TestLog::Message << "Fail, eglChooseConfig returned an error." << tcu::TestLog::EndMessage;
		TCU_FAIL("eglChooseConfig failed");
	}
	if (numConfigs > 1)
	{
		log << tcu::TestLog::Message << "Fail, more configs returned than requested." << tcu::TestLog::EndMessage;
		TCU_FAIL("Too many configs returned");
	}

	EGLint components[4];

	success = egl.getConfigAttrib(m_eglDisplay, config, EGL_RED_SIZE, &components[0]);
	TCU_CHECK_MSG(success == EGL_TRUE, "eglGetConfigAttrib failed");
	EGLU_CHECK(egl);
	success = egl.getConfigAttrib(m_eglDisplay, config, EGL_GREEN_SIZE, &components[1]);
	TCU_CHECK_MSG(success == EGL_TRUE, "eglGetConfigAttrib failed");
	EGLU_CHECK(egl);
	success = egl.getConfigAttrib(m_eglDisplay, config, EGL_BLUE_SIZE, &components[2]);
	TCU_CHECK_MSG(success == EGL_TRUE, "eglGetConfigAttrib failed");
	EGLU_CHECK(egl);
	success = egl.getConfigAttrib(m_eglDisplay, config, EGL_ALPHA_SIZE, &components[3]);
	TCU_CHECK_MSG(success == EGL_TRUE, "eglGetConfigAttrib failed");
	EGLU_CHECK(egl);

	TCU_CHECK_MSG(components[0] == 16, "Missing 16bit deep red channel");
	TCU_CHECK_MSG(components[1] == 16, "Missing 16bit deep green channel");
	TCU_CHECK_MSG(components[2] == 16, "Missing 16bit deep blue channel");
	TCU_CHECK_MSG(components[3] == 16, "Missing 16bit deep alpha channel");
}

void WideColorTest::deinit (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();

	if (m_eglDisplay != EGL_NO_DISPLAY)
	{
		egl.terminate(m_eglDisplay);
		m_eglDisplay = EGL_NO_DISPLAY;
	}
}

class WideColorFP16Test : public WideColorTest
{
public:
						WideColorFP16Test		(EglTestContext& eglTestCtx, const char* name, const char* description);

	void				init					(void);
	void				executeTest				(void);
	IterateResult		iterate					(void);
};

WideColorFP16Test::WideColorFP16Test (EglTestContext&	eglTestCtx,
									  const char*		name,
									  const char*		description)
	: WideColorTest(eglTestCtx, name, description)
{
}


void WideColorFP16Test::executeTest (void)
{
	checkPixelFloatSupport();
	checkFP16Support();
}

TestCase::IterateResult WideColorFP16Test::iterate (void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	executeTest();
	return STOP;
}

void WideColorFP16Test::init (void)
{
	WideColorTest::init();
}

class WideColor1010102Test : public WideColorTest
{
public:
						WideColor1010102Test	(EglTestContext&	eglTestCtx,
												 const char*		name,
												 const char*		description);

	void				executeTest				(void);
	IterateResult		iterate					(void);
};

WideColor1010102Test::WideColor1010102Test (EglTestContext& eglTestCtx, const char* name, const char* description)
	: WideColorTest(eglTestCtx, name, description)
{
}

void WideColor1010102Test::executeTest (void)
{
	check1010102Support();
}

TestCase::IterateResult WideColor1010102Test::iterate (void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	executeTest();
	return STOP;
}

struct Iteration
{
	float	start;
	float	increment;
	int		iterationCount;
	Iteration(float s, float i, int c)
		: start(s), increment(i), iterationCount(c) {}
};

class WideColorSurfaceTest : public WideColorTest
{
public:
						WideColorSurfaceTest	(EglTestContext&				eglTestCtx,
												 const char*					name,
												 const char*					description,
												 const EGLint*					attribList,
												 EGLint							colorSpace,
												 const std::vector<Iteration>&	iterations);

	void				init					(void);
	void				executeTest				(void);
	IterateResult		iterate					(void);

protected:
	void				readPixels				(const glw::Functions& gl, float* dataPtr);
	void				readPixels				(const glw::Functions& gl, deUint32* dataPtr);
	void				readPixels				(const glw::Functions& gl, deUint8* dataPtr);
	deUint32			expectedUint10			(float reference);
	deUint32			expectedUint2			(float reference);
	deUint8				expectedUint8			(float reference);
	deUint8				expectedAlpha8			(float reference);
	bool				checkWithThreshold8		(deUint8 value, deUint8 reference, deUint8 threshold = 1);
	bool				checkWithThreshold10	(deUint32 value, deUint32 reference, deUint32 threshold = 1);
	bool				checkWithThresholdFloat (float value, float reference, float threshold);
	void				doClearTest				(EGLSurface surface);
	void				testPixels				(float reference, float increment);
	void				writeEglConfig			(EGLConfig config);

private:
	std::vector<EGLint>					m_attribList;
	EGLConfig							m_eglConfig;
	EGLint								m_surfaceType;
	EGLint								m_componentType;
	EGLint								m_redSize;
	EGLint								m_colorSpace;
	const std::vector<struct Iteration> m_iterations;
	std::stringstream					m_debugLog;
};

WideColorSurfaceTest::WideColorSurfaceTest (EglTestContext& eglTestCtx, const char* name, const char* description, const EGLint* attribList, EGLint colorSpace, const std::vector<struct Iteration>& iterations)
	: WideColorTest		(eglTestCtx, name, description)
	, m_colorSpace		(colorSpace)
	, m_iterations		(iterations)
{
	deUint32 idx = 0;
	while (attribList[idx] != EGL_NONE)
	{
		if (attribList[idx] == EGL_COLOR_COMPONENT_TYPE_EXT)
		{
			m_componentType = attribList[idx + 1];
		}
		else if (attribList[idx] == EGL_SURFACE_TYPE)
		{
			m_surfaceType = attribList[idx+1];
		}
		else if (attribList[idx] == EGL_RED_SIZE)
		{
			m_redSize = attribList[idx + 1];
		}
		m_attribList.push_back(attribList[idx++]);
		m_attribList.push_back(attribList[idx++]);
	}
	m_attribList.push_back(EGL_NONE);
}

void WideColorSurfaceTest::init (void)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log	= m_testCtx.getLog();

	WideColorTest::init();

	// Only check for pixel format required for this specific run
	// If not available, check will abort test with "NotSupported"
	switch (m_redSize)
	{
		case 10:
			check1010102Support();
			break;
		case 16:
			checkPixelFloatSupport();
			checkFP16Support();
			break;
	}

	if (m_colorSpace != EGL_NONE && !eglu::hasExtension(egl, m_eglDisplay, "EGL_KHR_gl_colorspace"))
		TCU_THROW(NotSupportedError, "EGL_KHR_gl_colorspace is not supported");

	switch (m_colorSpace) {
		case EGL_GL_COLORSPACE_SRGB_KHR:
			checkColorSpaceSupport();
			break;
		case EGL_GL_COLORSPACE_DISPLAY_P3_EXT:
			checkDisplayP3Support();
			break;
		case EGL_GL_COLORSPACE_SCRGB_EXT:
			checkSCRGBSupport();
			break;
		case EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT:
			checkSCRGBLinearSupport();
			break;
		default:
			break;
	}

	EGLint numConfigs = 0;

	// Query from EGL implementation
	EGLU_CHECK_CALL(egl, chooseConfig(m_eglDisplay, &m_attribList[0], DE_NULL, 0, &numConfigs));

	if (numConfigs <= 0)
	{
		log << tcu::TestLog::Message << "No configs returned." << tcu::TestLog::EndMessage;
		TCU_FAIL("No configs returned");
	}

	log << tcu::TestLog::Message << numConfigs << " configs returned" << tcu::TestLog::EndMessage;

	EGLBoolean success = egl.chooseConfig(m_eglDisplay, &m_attribList[0], &m_eglConfig, 1, &numConfigs);
	if (success != EGL_TRUE)
	{
		log << tcu::TestLog::Message << "Fail, eglChooseConfig returned an error." << tcu::TestLog::EndMessage;
		TCU_FAIL("eglChooseConfig failed");
	}
	if (numConfigs > 1)
	{
		log << tcu::TestLog::Message << "Fail, more configs returned than requested." << tcu::TestLog::EndMessage;
		TCU_FAIL("Too many configs returned");
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	writeEglConfig(m_eglConfig);

}

void WideColorSurfaceTest::readPixels (const glw::Functions& gl, float* dataPtr)
{
	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, dataPtr);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels with floats");
}

void WideColorSurfaceTest::readPixels (const glw::Functions& gl, deUint32 *dataPtr)
{
	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, dataPtr);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels with RGBA_1010102 (32bits)");
}

void WideColorSurfaceTest::readPixels (const glw::Functions& gl, deUint8 *dataPtr)
{
	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, dataPtr);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels with RGBA_8888 (8 bit components)");
}

void WideColorSurfaceTest::writeEglConfig (EGLConfig config)
{
	const Library&	egl	= m_eglTestCtx.getLibrary();
	tcu::TestLog&	log		= m_testCtx.getLog();
	qpEglConfigInfo info;
	EGLint			val		= 0;

	info.bufferSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_BUFFER_SIZE);

	info.redSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_RED_SIZE);

	info.greenSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_GREEN_SIZE);

	info.blueSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_BLUE_SIZE);

	info.luminanceSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_LUMINANCE_SIZE);

	info.alphaSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_ALPHA_SIZE);

	info.alphaMaskSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_ALPHA_MASK_SIZE);

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_BIND_TO_TEXTURE_RGB);
	info.bindToTextureRGB = val == EGL_TRUE ? true : false;

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_BIND_TO_TEXTURE_RGBA);
	info.bindToTextureRGBA = val == EGL_TRUE ? true : false;

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_COLOR_BUFFER_TYPE);
	std::string colorBufferType = de::toString(eglu::getColorBufferTypeStr(val));
	info.colorBufferType = colorBufferType.c_str();

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_CONFIG_CAVEAT);
	std::string caveat = de::toString(eglu::getConfigCaveatStr(val));
	info.configCaveat = caveat.c_str();

	info.configID = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_CONFIG_ID);

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_CONFORMANT);
	std::string conformant = de::toString(eglu::getAPIBitsStr(val));
	info.conformant = conformant.c_str();

	info.depthSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_DEPTH_SIZE);

	info.level = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_LEVEL);

	info.maxPBufferWidth = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_MAX_PBUFFER_WIDTH);

	info.maxPBufferHeight = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_MAX_PBUFFER_HEIGHT);

	info.maxPBufferPixels = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_MAX_PBUFFER_PIXELS);

	info.maxSwapInterval = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_MAX_SWAP_INTERVAL);

	info.minSwapInterval = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_MIN_SWAP_INTERVAL);

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_NATIVE_RENDERABLE);
	info.nativeRenderable = val == EGL_TRUE ? true : false;

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_RENDERABLE_TYPE);
	std::string renderableTypes = de::toString(eglu::getAPIBitsStr(val));
	info.renderableType = renderableTypes.c_str();

	info.sampleBuffers = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_SAMPLE_BUFFERS);

	info.samples = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_SAMPLES);

	info.stencilSize = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_STENCIL_SIZE);

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_SURFACE_TYPE);
	std::string surfaceTypes = de::toString(eglu::getSurfaceBitsStr(val));
	info.surfaceTypes = surfaceTypes.c_str();

	val = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_TRANSPARENT_TYPE);
	std::string transparentType = de::toString(eglu::getTransparentTypeStr(val));
	info.transparentType = transparentType.c_str();

	info.transparentRedValue = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_TRANSPARENT_RED_VALUE);

	info.transparentGreenValue = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_TRANSPARENT_GREEN_VALUE);

	info.transparentBlueValue = eglu::getConfigAttribInt(egl, m_eglDisplay, config, EGL_TRANSPARENT_BLUE_VALUE);

	log.writeEglConfig(&info);
}

deUint32 WideColorSurfaceTest::expectedUint10 (float reference)
{
	deUint32 expected;

	if (reference < 0.0)
	{
		expected = 0;
	}
	else if (reference > 1.0)
	{
		expected = 1023;
	}
	else
	{
		expected = static_cast<deUint32>(deRound(reference * 1023.0));
	}

	return expected;
}

deUint32 WideColorSurfaceTest::expectedUint2 (float reference)
{
	deUint32 expected;

	if (reference < 0.0)
	{
		expected = 0;
	}
	else if (reference > 1.0)
	{
		expected = 3;
	}
	else
	{
		expected = static_cast<deUint32>(deRound(reference * 3.0));
	}

	return expected;
}

deUint8 WideColorSurfaceTest::expectedUint8 (float reference)
{
	deUint8 expected;
	if (reference < 0.0)
	{
		expected = 0;
	}
	else if (reference >= 1.0)
	{
		expected = 255;
	}
	else
	{
		// Apply sRGB transfer function when colorspace is sRGB and pixel component
		// size is 8 bits (which is why we are here in expectedUint8).
		if (m_colorSpace == EGL_GL_COLORSPACE_SRGB_KHR)
		{
			float srgbReference;

			if (reference <= 0.0031308)
			{
				srgbReference = 12.92f * reference;
			}
			else
			{
				float powRef = deFloatPow(reference, (1.0f/2.4f));
				srgbReference = (1.055f * powRef) - 0.055f;
			}
			expected = static_cast<deUint8>(deRound(srgbReference * 255.0));
		}
		else
		{
			expected = static_cast<deUint8>(deRound(reference * 255.0));
		}
	}
	return expected;
}

deUint8 WideColorSurfaceTest::expectedAlpha8 (float reference)
{
	deUint8 expected;
	if (reference < 0.0)
	{
		expected = 0;
	}
	else if (reference >= 1.0)
	{
		expected = 255;
	}
	else
	{
		// The sRGB transfer function is not applied to alpha
		expected = static_cast<deUint8>(deRound(reference * 255.0));
	}
	return expected;
}

// Return true for value out of range (fail)
bool WideColorSurfaceTest::checkWithThreshold8(deUint8 value, deUint8 reference, deUint8 threshold)
{
	const deUint8 low = reference >= threshold ? static_cast<deUint8>(reference - threshold) : 0;
	const deUint8 high = reference <= (255 - threshold) ? static_cast<deUint8>(reference + threshold) : 255;
	return !((value >= low) && (value <= high));
}

bool WideColorSurfaceTest::checkWithThreshold10(deUint32 value, deUint32 reference, deUint32 threshold)
{
	const deUint32 low = reference >= threshold ? reference - threshold : 0;
	const deUint32 high = reference <= (1023 - threshold) ? reference + threshold : 1023;
	return !((value >= low) && (value <= high));
}

bool WideColorSurfaceTest::checkWithThresholdFloat(float value, float reference, float threshold)
{
	const float low = reference - threshold;
	const float high = reference + threshold;
	return !((value >= low) && (value <= high));
}

void WideColorSurfaceTest::testPixels (float reference, float increment)
{
	tcu::TestLog&	log				= m_testCtx.getLog();

	if (m_componentType == EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT)
	{
		float pixels[16];
		const float expected[4] =
		{
			reference,
			reference + increment,
			reference - increment,
			reference + 2 * increment
		};
		readPixels(m_gl, pixels);

		if (checkWithThresholdFloat(pixels[0], expected[0], increment) ||
				checkWithThresholdFloat(pixels[1], expected[1], increment) ||
				checkWithThresholdFloat(pixels[2], expected[2], increment) ||
				checkWithThresholdFloat(pixels[3], expected[3], increment))
		{
			if (m_debugLog.str().size() > 0)
			{
				log << tcu::TestLog::Message
					<< "Prior passing tests\n"
					<< m_debugLog.str()
					<< tcu::TestLog::EndMessage;
				m_debugLog.str("");
			}
			log << tcu::TestLog::Message
				<< "Image comparison failed: "
				<< "reference = " << reference
				<< ", expected = " << expected[0]
					<< ":" << expected[1]
					<< ":" << expected[2]
					<< ":" << expected[3]
				<< ", result = " << pixels[0]
					<< ":" << pixels[1]
					<< ":" << pixels[2]
					<< ":" << pixels[3]
				<< tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Color test failed");
		}
		else
		{
			// Pixel matches expected value
			m_debugLog << "Image comparison passed: "
				<< "reference = " << reference
				<< ", result = " << pixels[0]
					<< ":" << pixels[1]
					<< ":" << pixels[2]
					<< ":" << pixels[3]
				<< "\n";
		}
	}
	else if (m_redSize > 8)
	{
		deUint32 buffer[16];
		readPixels(m_gl, buffer);
		deUint32 pixels[4];
		deUint32 expected[4];

		pixels[0] = buffer[0] & 0x3ff;
		pixels[1] = (buffer[0] >> 10) & 0x3ff;
		pixels[2] = (buffer[0] >> 20) & 0x3ff;
		pixels[3] = (buffer[0] >> 30) & 0x3;

		expected[0] = expectedUint10(reference);
		expected[1] = expectedUint10(reference + increment);
		expected[2] = expectedUint10(reference - increment);
		expected[3] = expectedUint2(reference + 2 * increment);
		if (checkWithThreshold10(pixels[0], expected[0]) || checkWithThreshold10(pixels[1], expected[1])
				|| checkWithThreshold10(pixels[2], expected[2]) || checkWithThreshold10(pixels[3], expected[3]))
		{
			if (m_debugLog.str().size() > 0) {
				log << tcu::TestLog::Message
					<< "Prior passing tests\n"
					<< m_debugLog.str()
					<< tcu::TestLog::EndMessage;
				m_debugLog.str("");
			}
			log << tcu::TestLog::Message
				<< "Image comparison failed: "
				<< "reference = " << reference
				<< ", expected = " << static_cast<deUint32>(expected[0])
					<< ":" << static_cast<deUint32>(expected[1])
					<< ":" << static_cast<deUint32>(expected[2])
					<< ":" << static_cast<deUint32>(expected[3])
				<< ", result = " << static_cast<deUint32>(pixels[0])
					<< ":" << static_cast<deUint32>(pixels[1])
					<< ":" << static_cast<deUint32>(pixels[2])
					<< ":" << static_cast<deUint32>(pixels[3])
				<< tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Color test failed");
		}
		else
		{
			// Pixel matches expected value
			m_debugLog << "Image comparison passed: "
				<< "reference = " << reference
				<< ", result = " << static_cast<deUint32>(pixels[0])
					<< ":" << static_cast<deUint32>(pixels[1])
					<< ":" << static_cast<deUint32>(pixels[2])
					<< ":" << static_cast<deUint32>(pixels[3])
				<< "\n";
		}
	}
	else
	{
		deUint8 pixels[16];
		deUint8 expected[4];
		readPixels(m_gl, pixels);

		expected[0] = expectedUint8(reference);
		expected[1] = expectedUint8(reference + increment);
		expected[2] = expectedUint8(reference - increment);
		expected[3] = expectedAlpha8(reference + 2 * increment);
		if (checkWithThreshold8(pixels[0], expected[0]) || checkWithThreshold8(pixels[1], expected[1])
				|| checkWithThreshold8(pixels[2], expected[2]) || checkWithThreshold8(pixels[3], expected[3]))
		{
			if (m_debugLog.str().size() > 0) {
				log << tcu::TestLog::Message
					<< "(C)Prior passing tests\n"
					<< m_debugLog.str()
					<< tcu::TestLog::EndMessage;
				m_debugLog.str("");
			}
			log << tcu::TestLog::Message
				<< "Image comparison failed: "
				<< "reference = " << reference
				<< ", expected = " << static_cast<deUint32>(expected[0])
					<< ":" << static_cast<deUint32>(expected[1])
					<< ":" << static_cast<deUint32>(expected[2])
					<< ":" << static_cast<deUint32>(expected[3])
				<< ", result = " << static_cast<deUint32>(pixels[0])
					<< ":" << static_cast<deUint32>(pixels[1])
					<< ":" << static_cast<deUint32>(pixels[2])
					<< ":" << static_cast<deUint32>(pixels[3])
				<< tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Color test failed");
		}
		else
		{
			// Pixel matches expected value
			m_debugLog << "Image comparison passed: "
				<< "reference = " << reference
				<< ", result = " << static_cast<deUint32>(pixels[0])
					<< ":" << static_cast<deUint32>(pixels[1])
					<< ":" << static_cast<deUint32>(pixels[2])
					<< ":" << static_cast<deUint32>(pixels[3])
				<< "\n";
		}
	}
}

void WideColorSurfaceTest::doClearTest (EGLSurface surface)
{
	tcu::TestLog&	log				= m_testCtx.getLog();
	const Library&	egl				= m_eglTestCtx.getLibrary();
	const EGLint	attribList[]	=
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLContext		eglContext		= egl.createContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext");

	egl.makeCurrent(m_eglDisplay, surface, surface, eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");

	{
		// put gles2Renderer inside it's own scope so that it's cleaned
		// up before we hit the destroyContext
		const GLES2Renderer gles2Renderer(m_gl, 128, 128);

		std::vector<Iteration>::const_iterator it;	// declare an Iterator to a vector of strings
		log << tcu::TestLog::Message << "m_iterations.count = " << m_iterations.size() << tcu::TestLog::EndMessage;
		for(it = m_iterations.begin() ; it < m_iterations.end(); it++)
		{
			float reference = it->start;
			log << tcu::TestLog::Message << "start = " << it->start
						<< tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message
						<< "increment = " << it->increment
						<< tcu::TestLog::EndMessage;
			log << tcu::TestLog::Message
						<< "count = " << it->iterationCount
						<< tcu::TestLog::EndMessage;
			m_debugLog.str("");
			for (int iterationCount = 0; iterationCount < it->iterationCount; iterationCount++)
			{
				const Color	clearColor(reference, reference + it->increment, reference - it->increment, reference + 2 * it->increment);

				clearColorScreen(m_gl, clearColor);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "Clear to test value");

				testPixels(reference, it->increment);

				// reset buffer contents so that we know render below did something
				const Color	clearColor2(1.0f - reference, 1.0f, 1.0f, 1.0f);
				clearColorScreen(m_gl, clearColor2);
				GLU_EXPECT_NO_ERROR(m_gl.getError(), "Clear to 1.0f - reference value");

				const ColoredRect	coloredRect	(IVec2(0.0f, 0.0f), IVec2(1.0f, 1.0f), clearColor);
				gles2Renderer.render(coloredRect);
				testPixels(reference, it->increment);

				reference += it->increment;
			}

			EGLU_CHECK_CALL(egl, swapBuffers(m_eglDisplay, surface));
		}
	}

	// disconnect surface & context so they can be destroyed when
	// this function exits.
	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

	egl.destroyContext(m_eglDisplay, eglContext);
}

void WideColorSurfaceTest::executeTest (void)
{
	tcu::TestLog&						log				= m_testCtx.getLog();
	const Library&						egl				= m_eglTestCtx.getLibrary();
	const eglu::NativeDisplayFactory&	displayFactory	= m_eglTestCtx.getNativeDisplayFactory();
	eglu::NativeDisplay&				nativeDisplay	= m_eglTestCtx.getNativeDisplay();
	egl.bindAPI(EGL_OPENGL_ES_API);

	if (m_surfaceType & EGL_PBUFFER_BIT)
	{
		log << tcu::TestLog::Message << "Test Pbuffer" << tcu::TestLog::EndMessage;

		std::vector<EGLint>			attribs;
		attribs.push_back(EGL_WIDTH);
		attribs.push_back(128);
		attribs.push_back(EGL_HEIGHT);
		attribs.push_back(128);
		if (m_colorSpace != EGL_NONE)
		{
			attribs.push_back(EGL_GL_COLORSPACE_KHR);
			attribs.push_back(m_colorSpace);
		}
		attribs.push_back(EGL_NONE);
		attribs.push_back(EGL_NONE);
		const EGLSurface surface = egl.createPbufferSurface(m_eglDisplay, m_eglConfig, attribs.data());
		if ((surface == EGL_NO_SURFACE) && (egl.getError() == EGL_BAD_MATCH))
		{
			TCU_THROW(NotSupportedError, "Colorspace is not supported with this format");
		}
		TCU_CHECK(surface != EGL_NO_SURFACE);
		EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

		doClearTest(surface);

		egl.destroySurface(m_eglDisplay, surface);
		EGLU_CHECK_MSG(egl, "eglDestroySurface()");
	}
	else if (m_surfaceType & EGL_WINDOW_BIT)
	{
		log << tcu::TestLog::Message << "Test Window" << tcu::TestLog::EndMessage;

		const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(displayFactory, m_testCtx.getCommandLine());

		de::UniquePtr<eglu::NativeWindow>	window			(windowFactory.createWindow(&nativeDisplay, m_eglDisplay, m_eglConfig, DE_NULL, eglu::WindowParams(128, 128, eglu::parseWindowVisibility(m_testCtx.getCommandLine()))));
		std::vector<EGLAttrib>		attribs;
		if (m_colorSpace != EGL_NONE)
		{
			attribs.push_back(EGL_GL_COLORSPACE_KHR);
			attribs.push_back(m_colorSpace);
		}
		attribs.push_back(EGL_NONE);
		attribs.push_back(EGL_NONE);

		EGLSurface	surface;
		try
		{
			surface = eglu::createWindowSurface(nativeDisplay, *window, m_eglDisplay, m_eglConfig, attribs.data());
		}
		catch (const eglu::Error& error)
		{
			if (error.getError() == EGL_BAD_MATCH)
				TCU_THROW(NotSupportedError, "createWindowSurface is not supported for this config");

			throw;
		}
		TCU_CHECK(surface != EGL_NO_SURFACE);
		EGLU_CHECK_MSG(egl, "eglCreateWindowSurface()");

		doClearTest(surface);

		egl.destroySurface(m_eglDisplay, surface);
		EGLU_CHECK_MSG(egl, "eglDestroySurface()");
	}
	else if (m_surfaceType & EGL_PIXMAP_BIT)
	{
		log << tcu::TestLog::Message << "Test Pixmap" << tcu::TestLog::EndMessage;

		const eglu::NativePixmapFactory&	pixmapFactory	= eglu::selectNativePixmapFactory(displayFactory, m_testCtx.getCommandLine());

		de::UniquePtr<eglu::NativePixmap>	pixmap			(pixmapFactory.createPixmap(&nativeDisplay, m_eglDisplay, m_eglConfig, DE_NULL, 128, 128));
		const EGLSurface					surface			= eglu::createPixmapSurface(nativeDisplay, *pixmap, m_eglDisplay, m_eglConfig, DE_NULL);
		TCU_CHECK(surface != EGL_NO_SURFACE);
		EGLU_CHECK_MSG(egl, "eglCreatePixmapSurface()");

		doClearTest(surface);

		egl.destroySurface(m_eglDisplay, surface);
		EGLU_CHECK_MSG(egl, "eglDestroySurface()");
	}
	else
		TCU_FAIL("No valid surface types supported in config");
}

TestCase::IterateResult WideColorSurfaceTest::iterate (void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	executeTest();
	return STOP;
}

} // anonymous

WideColorTests::WideColorTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "wide_color", "Wide Color tests")
{
}

void WideColorTests::init (void)
{
	addChild(new WideColorFP16Test(m_eglTestCtx, "fp16", "Verify that FP16 pixel format is present"));
	addChild(new WideColor1010102Test(m_eglTestCtx, "1010102", "Check if 1010102 pixel format is present"));

	// This is an increment FP16 can do between -1.0 to 1.0
	const float fp16Increment1 = deFloatPow(2.0, -11.0);
	// This is an increment FP16 can do between 1.0 to 2.0
	const float fp16Increment2 = deFloatPow(2.0, -10.0);

	const EGLint windowAttribListFP16[] =
	{
		EGL_SURFACE_TYPE,				EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					16,
		EGL_GREEN_SIZE,					16,
		EGL_BLUE_SIZE,					16,
		EGL_ALPHA_SIZE,					16,
		EGL_COLOR_COMPONENT_TYPE_EXT,	EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
		EGL_NONE,						EGL_NONE
	};

	std::vector<Iteration> fp16Iterations;
	// -0.333251953125f ~ -1/3 as seen in FP16
	fp16Iterations.push_back(Iteration(-0.333251953125f, fp16Increment1, 10));
	// test crossing 0
	fp16Iterations.push_back( Iteration(-fp16Increment1 * 5.0f, fp16Increment1, 10));
	// test crossing 1.0
	fp16Iterations.push_back( Iteration(1.0f - fp16Increment2 * 5.0f, fp16Increment2, 10));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_fp16_default_colorspace", "FP16 window surface has FP16 pixels in it", windowAttribListFP16, EGL_NONE, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_fp16_colorspace_srgb", "FP16 window surface, explicit sRGB colorspace", windowAttribListFP16, EGL_GL_COLORSPACE_SRGB_KHR, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_fp16_colorspace_p3", "FP16 window surface, explicit Display-P3 colorspace", windowAttribListFP16, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_fp16_colorspace_scrgb", "FP16 window surface, explicit scRGB colorspace", windowAttribListFP16, EGL_GL_COLORSPACE_SCRGB_EXT, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_fp16_colorspace_scrgb_linear", "FP16 window surface, explicit scRGB linear colorspace", windowAttribListFP16, EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT, fp16Iterations));

	const EGLint pbufferAttribListFP16[] =
	{
		EGL_SURFACE_TYPE,				EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					16,
		EGL_GREEN_SIZE,					16,
		EGL_BLUE_SIZE,					16,
		EGL_ALPHA_SIZE,					16,
		EGL_COLOR_COMPONENT_TYPE_EXT,	EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
		EGL_NONE,						EGL_NONE
	};
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_fp16_default_colorspace", "FP16 pbuffer surface has FP16 pixels in it", pbufferAttribListFP16, EGL_NONE, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_fp16_colorspace_srgb", "FP16 pbuffer surface, explicit sRGB colorspace", pbufferAttribListFP16, EGL_GL_COLORSPACE_SRGB_KHR, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_fp16_colorspace_p3", "FP16 pbuffer surface, explicit Display-P3 colorspace", pbufferAttribListFP16, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_fp16_colorspace_scrgb", "FP16 pbuffer surface, explicit scRGB colorspace", pbufferAttribListFP16, EGL_GL_COLORSPACE_SCRGB_EXT, fp16Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_fp16_colorspace_scrgb_linear", "FP16 pbuffer surface, explicit scRGB linear colorspace", pbufferAttribListFP16, EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT, fp16Iterations));

	const EGLint windowAttribList1010102[] =
	{
		EGL_SURFACE_TYPE,				EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					10,
		EGL_GREEN_SIZE,					10,
		EGL_BLUE_SIZE,					10,
		EGL_ALPHA_SIZE,					2,
		EGL_NONE,						EGL_NONE
	};

	std::vector<Iteration> int1010102Iterations;
	// -0.333251953125f ~ -1/3 as seen in fp16
	// Negative values will be 0 on read with fixed point pixel formats
	int1010102Iterations.push_back(Iteration(-0.333251953125f, fp16Increment1, 10));
	// test crossing 0
	int1010102Iterations.push_back(Iteration(-fp16Increment1 * 5.0f, fp16Increment1, 10));
	// test crossing 1.0
	// Values > 1.0 will be truncated to 1.0 with fixed point pixel formats
	int1010102Iterations.push_back(Iteration(1.0f - fp16Increment2 * 5.0f, fp16Increment2, 10));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_1010102_colorspace_default", "1010102 Window surface, default (sRGB) colorspace", windowAttribList1010102, EGL_NONE, int1010102Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_1010102_colorspace_srgb", "1010102 Window surface, explicit sRGB colorspace", windowAttribList1010102, EGL_GL_COLORSPACE_SRGB_KHR, int1010102Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_1010102_colorspace_p3", "1010102 Window surface, explicit Display-P3 colorspace", windowAttribList1010102, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, int1010102Iterations));

	const EGLint pbufferAttribList1010102[] =
	{
		EGL_SURFACE_TYPE,				EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					10,
		EGL_GREEN_SIZE,					10,
		EGL_BLUE_SIZE,					10,
		EGL_ALPHA_SIZE,					2,
		EGL_NONE,						EGL_NONE
	};
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_1010102_colorspace_default", "1010102 pbuffer surface, default (sRGB) colorspace", pbufferAttribList1010102, EGL_NONE, int1010102Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_1010102_colorspace_srgb", "1010102 pbuffer surface, explicit sRGB colorspace", pbufferAttribList1010102, EGL_GL_COLORSPACE_SRGB_KHR, int1010102Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_1010102_colorspace_p3", "1010102 pbuffer surface, explicit Display-P3 colorspace", pbufferAttribList1010102, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, int1010102Iterations));

	const EGLint windowAttribList8888[] =
	{
		EGL_SURFACE_TYPE,				EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					8,
		EGL_GREEN_SIZE,					8,
		EGL_BLUE_SIZE,					8,
		EGL_ALPHA_SIZE,					8,
		EGL_NONE,						EGL_NONE
	};

	std::vector<Iteration> int8888Iterations;
	// -0.333251953125f ~ -1/3 as seen in fp16
	// Negative values will be 0 on read with fixed point pixel formats
	int8888Iterations.push_back(Iteration(-0.333251953125f, fp16Increment1, 10));
	// test crossing 0
	int8888Iterations.push_back(Iteration(-fp16Increment1 * 5.0f, fp16Increment1, 10));
	// test crossing 1.0
	// Values > 1.0 will be truncated to 1.0 with fixed point pixel formats
	int8888Iterations.push_back(Iteration(1.0f - fp16Increment2 * 5.0f, fp16Increment2, 10));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_8888_colorspace_default", "8888 window surface, default (sRGB) colorspace", windowAttribList8888, EGL_NONE, int8888Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_8888_colorspace_srgb", "8888 window surface, explicit sRGB colorspace", windowAttribList8888, EGL_GL_COLORSPACE_SRGB_KHR, int8888Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "window_8888_colorspace_p3", "8888 window surface, explicit Display-P3 colorspace", windowAttribList8888, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, int8888Iterations));

	const EGLint pbufferAttribList8888[] =
	{
		EGL_SURFACE_TYPE,				EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,					8,
		EGL_GREEN_SIZE,					8,
		EGL_BLUE_SIZE,					8,
		EGL_ALPHA_SIZE,					8,
		EGL_NONE,						EGL_NONE
	};
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_8888_colorspace_default", "8888 pbuffer surface, default (sRGB) colorspace", pbufferAttribList8888, EGL_NONE, int8888Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_8888_colorspace_srgb", "8888 pbuffer surface, explicit sRGB colorspace", pbufferAttribList8888, EGL_GL_COLORSPACE_SRGB_KHR, int8888Iterations));
	addChild(new WideColorSurfaceTest(m_eglTestCtx, "pbuffer_8888_colorspace_p3", "8888 pbuffer surface, explicit Display-P3 colorspace", pbufferAttribList8888, EGL_GL_COLORSPACE_DISPLAY_P3_EXT, int8888Iterations));
}

TestCaseGroup* createWideColorTests (EglTestContext& eglTestCtx)
{
	return new WideColorTests(eglTestCtx);
}

} // egl
} // deqp
