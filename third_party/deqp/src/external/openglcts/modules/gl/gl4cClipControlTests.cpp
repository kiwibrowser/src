/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 * \file  gl4GPUShaderFP64Tests.cpp
 * \brief Implements conformance tests for "GPU Shader FP64" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cClipControlTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

#include <cmath>

namespace gl4cts
{

class ClipControlApi
{
public:
	ClipControlApi(deqp::Context& context, ClipControlTests::API api) : m_context(context)
	{
		if (!Supported(m_context, api))
		{
			throw tcu::NotSupportedError("Required clip_control extension is not supported");
		}

		switch (api)
		{
		case ClipControlTests::API_GL_ARB_clip_control:
		case ClipControlTests::API_GL_45core: //fall through
			clipControl = context.getRenderContext().getFunctions().clipControl;
			break;
		}
	}

	static bool Supported(deqp::Context& context, ClipControlTests::API api)
	{
		return (api == ClipControlTests::API_GL_ARB_clip_control &&
				context.getContextInfo().isExtensionSupported("GL_ARB_clip_control")) ||
			   api == ClipControlTests::API_GL_45core;
	}

	glw::glClipControlFunc clipControl;

private:
	deqp::Context& m_context;
};

class ClipControlBaseTest : public deqp::TestCase
{
protected:
	ClipControlBaseTest(deqp::Context& context, ClipControlTests::API api, const char* name, const char* description)
		: TestCase(context, name, description), m_api(api)
	{
	}

	void init()
	{
		ClipControlApi(m_context, m_api);
	}

	bool verifyState(glw::GLenum origin, glw::GLenum depth)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		bool ret = true;

		glw::GLint retI;
		gl.getIntegerv(GL_CLIP_ORIGIN, &retI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_CLIP_ORIGIN");

		ret &= (static_cast<glw::GLenum>(retI) == origin);

		gl.getIntegerv(GL_CLIP_DEPTH_MODE, &retI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_CLIP_DEPTH_MODE");

		ret &= (static_cast<glw::GLenum>(retI) == depth);

		return ret;
	}

protected:
	const ClipControlTests::API m_api;
};

class ClipControlRenderBaseTest : public ClipControlBaseTest
{
protected:
	ClipControlRenderBaseTest(deqp::Context& context, ClipControlTests::API api, const char* name,
							  const char* description)
		: ClipControlBaseTest(context, api, name, description), m_fbo(0), m_rboC(0), m_rboD(0)
	{
	}

	const char* fsh()
	{
		return "#version 400"
			   "\n"
			   "out vec4 FragColor;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
			   "\n"
			   "}";
	}

	bool fuzzyDepthCompare(tcu::TestLog& log, const char* imageSetName, const char* imageSetDesc,
						   const tcu::TextureLevel& reference, const tcu::TextureLevel& result, float threshold,
						   const tcu::TextureLevel* importanceMask = NULL)
	{
		(void)imageSetName;
		(void)imageSetDesc;
		bool  depthOk	= true;
		float difference = 0.0f;

		for (int y = 0; y < result.getHeight() && depthOk; y++)
		{
			for (int x = 0; x < result.getWidth() && depthOk; x++)
			{
				float ref  = reference.getAccess().getPixDepth(x, y);
				float res  = result.getAccess().getPixDepth(x, y);
				difference = std::abs(ref - res);
				if (importanceMask)
				{
					difference *= importanceMask->getAccess().getPixDepth(x, y);
				}
				depthOk &= (difference < threshold);
			}
		}

		if (!depthOk)
			log << tcu::TestLog::Message << "Image comparison failed: difference = " << difference
				<< ", threshold = " << threshold << tcu::TestLog::EndMessage;
		tcu::Vec4 pixelBias(0.0f, 0.0f, 0.0f, 0.0f);
		tcu::Vec4 pixelScale(1.0f, 1.0f, 1.0f, 1.0f);
		log << tcu::TestLog::ImageSet("Result", "Depth image comparison result")
			<< tcu::TestLog::Image("Result", "Result", result.getAccess(), pixelScale, pixelBias)
			<< tcu::TestLog::Image("Reference", "Reference", reference.getAccess(), pixelScale, pixelBias);
		if (importanceMask)
		{
			log << tcu::TestLog::Image("Importance mask", "mask", importanceMask->getAccess(), pixelScale, pixelBias);
		}
		log << tcu::TestLog::EndImageSet;

		return depthOk;
	}

	virtual void init(void)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		glw::GLuint				 viewportW	= renderTarget.getWidth();
		glw::GLuint				 viewportH	= renderTarget.getHeight();
		const glw::Functions&	gl			  = m_context.getRenderContext().getFunctions();

		gl.genFramebuffers(1, &m_fbo);
		gl.genRenderbuffers(1, &m_rboC);
		gl.genRenderbuffers(1, &m_rboD);

		gl.bindRenderbuffer(GL_RENDERBUFFER, m_rboC);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, viewportW, viewportH);
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_rboD);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, viewportW, viewportH);

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rboC);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboD);
	}

	virtual void deinit(void)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteFramebuffers(1, &m_fbo);
		gl.deleteRenderbuffers(1, &m_rboC);
		gl.deleteRenderbuffers(1, &m_rboD);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

private:
	GLuint m_fbo, m_rboC, m_rboD;
};

/*
 Verify the following state values are implemented and return a valid
 initial value by calling GetIntegerv:

 Get Value                                 Initial Value
 -------------------------------------------------------
 CLIP_ORIGIN                                  LOWER_LEFT
 CLIP_DEPTH_MODE                     NEGATIVE_ONE_TO_ONE

 Verify no GL error is generated.
 */
class ClipControlInitialState : public ClipControlBaseTest
{
public:
	ClipControlInitialState(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlBaseTest(context, api, name, "Verify initial state")
	{
	}

	IterateResult iterate()
	{
		if (!verifyState(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE))
		{
			TCU_FAIL("Wrong intitial state: GL_CLIP_ORIGIN should be GL_LOWER_LEFT,"
					 " GL_CLIP_ORIGIN should be NEGATIVE_ONE_TO_ONE");
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, qpGetTestResultName(QP_TEST_RESULT_PASS));
		return STOP;
	}
};

/*
 Modify the state to each of the following combinations and after each
 state change verify the state values:

 ClipControl(UPPER_LEFT, ZERO_TO_ONE)
 ClipControl(UPPER_LEFT, NEGATIVE_ONE_TO_ONE)
 ClipControl(LOWER_LEFT, ZERO_TO_ONE)
 ClipControl(LOWER_LEFT, NEGATIVE_ONE_TO_ONE)

 Verify no GL error is generated.

 */
class ClipControlModifyGetState : public ClipControlBaseTest
{
public:
	ClipControlModifyGetState(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlBaseTest(context, api, name, "Verify initial state")
	{
	}

	void deinit()
	{
		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}
	}

	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		GLenum cases[4][2] = {
			{ GL_UPPER_LEFT, GL_ZERO_TO_ONE },
			{ GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE },
			{ GL_LOWER_LEFT, GL_ZERO_TO_ONE },
			{ GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE },
		};

		for (size_t i = 0; i < DE_LENGTH_OF_ARRAY(cases); i++)
		{
			cc.clipControl(cases[i][0], cases[i][1]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");
			if (!verifyState(cases[i][0], cases[i][1]))
			{
				TCU_FAIL("Wrong ClipControl state after ClipControl() call");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, qpGetTestResultName(QP_TEST_RESULT_PASS));
		return STOP;
	}
};

/*
 Check that ClipControl generate an GL_INVALID_ENUM error if origin is
 not GL_LOWER_LEFT or GL_UPPER_LEFT.

 Check that ClipControl generate an GL_INVALID_ENUM error if depth is
 not GL_NEGATIVE_ONE_TO_ONE or GL_ZERO_TO_ONE.

 Test is based on OpenGL 4.5 Core Profile Specification May 28th Section
 13.5 Primitive Clipping:
 "An INVALID_ENUM error is generated if origin is not LOWER_LEFT or
 UPPER_LEFT.
 An INVALID_ENUM error is generated if depth is not NEGATIVE_ONE_-
 TO_ONE or ZERO_TO_ONE."
 */
class ClipControlErrors : public ClipControlBaseTest
{
public:
	ClipControlErrors(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlBaseTest(context, api, name, "Verify that proper errors are generated when using ClipControl.")
	{
	}

	void deinit()
	{
		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}
	}

	IterateResult iterate()
	{
		/* API query */
		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		/* Finding improper value. */
		GLenum improper_value = GL_NONE;

		while ((GL_UPPER_LEFT == improper_value) || (GL_LOWER_LEFT == improper_value) ||
			   (GL_ZERO_TO_ONE == improper_value) || (GL_NEGATIVE_ONE_TO_ONE == improper_value))
		{
			++improper_value;
		}

		/* Test setup. */
		GLenum cases[5][2] = { { GL_UPPER_LEFT, improper_value },
							   { GL_LOWER_LEFT, improper_value },
							   { improper_value, GL_ZERO_TO_ONE },
							   { improper_value, GL_NEGATIVE_ONE_TO_ONE },
							   { improper_value, improper_value } };

		/* Test iterations. */
		for (size_t i = 0; i < DE_LENGTH_OF_ARRAY(cases); i++)
		{
			cc.clipControl(cases[i][0], cases[i][1]);

			if (GL_INVALID_ENUM != gl.getError())
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, qpGetTestResultName(QP_TEST_RESULT_FAIL));

				log << tcu::TestLog::Message
					<< "ClipControl have not generated GL_INVALID_ENUM error when called with invalid value ("
					<< cases[i][0] << ", " << cases[i][1] << ")." << tcu::TestLog::EndMessage;
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, qpGetTestResultName(QP_TEST_RESULT_PASS));
		return STOP;
	}
};

/*
 Clip Control Origin Test

 * Basic <origin> behavior can be tested by rendering to a viewport with
 clip coordinates where -1.0 <= x_c <= 0.0 and -1.0 <= y_c <= 0.0.
 When <origin> is LOWER_LEFT the "bottom left" portion of the window
 is rendered and when UPPER_LEFT is used the "top left" portion of the
 window is rendered. The default framebuffer should be bound. Here is the
 basic outline of the test:

 - Clear the default framebuffer to red (1,0,0).
 - Set ClipControl(UPPER_LEFT, NEGATIVE_ONE_TO_ONE)
 - Render a triangle fan covering (-1.0, -1.0) to (0.0, 0.0) and
 write a pixel value of green (0,1,0).
 - Read back the default framebuffer with ReadPixels
 - Verify the green pixels at the top and red at the bottom.

 Repeat the above test with LOWER_LEFT and verify green at the bottom
 and red at the top.
 */
class ClipControlOriginTest : public ClipControlRenderBaseTest
{
public:
	ClipControlOriginTest(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlRenderBaseTest(context, api, name, "Clip Control Origin Test"), m_vao(0), m_vbo(0)
	{
	}

	void deinit()
	{
		ClipControlRenderBaseTest::deinit();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}

		gl.clearColor(0.0, 0.0, 0.0, 0.0);
		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);
		}
		if (m_vbo)
		{
			gl.deleteBuffers(1, &m_vbo);
		}
	}

	IterateResult iterate()
	{

		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		//Render a triangle fan covering(-1.0, -1.0) to(1.0, 0.0) and
		//write a pixel value of green(0, 1, 0).

		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));

		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}

		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);

		const float vertex_data0[] = { -1.0, -1.0, 0.0, -1.0, -1.0, 0.0, 0.0, 0.0 };

		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertex_data0), vertex_data0, GL_STATIC_DRAW);

		gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(0);

		gl.useProgram(program->getProgram());

		glw::GLenum origins[] = { GL_UPPER_LEFT, GL_LOWER_LEFT };

		qpTestResult result = QP_TEST_RESULT_PASS;

		for (size_t orig = 0; orig < DE_LENGTH_OF_ARRAY(origins); orig++)
		{
			//Clear the default framebuffer to red(1, 0, 0).
			gl.clearColor(1.0, 0.0, 0.0, 1.0);
			gl.clear(GL_COLOR_BUFFER_BIT);

			//Set ClipControl(UPPER_LEFT, NEGATIVE_ONE_TO_ONE)
			cc.clipControl(origins[orig], GL_NEGATIVE_ONE_TO_ONE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");

			//test method modification: use GL_TRIANGLE_STRIP, not FAN.
			gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

			//Read back the default framebuffer with ReadPixels
			//Verify the green pixels at the top and red at the bottom.
			qpTestResult loopResult = ValidateFramebuffer(m_context, origins[orig]);
			if (loopResult != QP_TEST_RESULT_PASS)
			{
				result = loopResult;
			}
		}

		m_testCtx.setTestResult(result, qpGetTestResultName(result));

		return STOP;
	}

	const char* vsh()
	{
		return "#version 400"
			   "\n"
			   "in vec2 Position;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    gl_Position = vec4(Position, 0.0, 1.0);"
			   "\n"
			   "}";
	}

	qpTestResult ValidateFramebuffer(deqp::Context& context, glw::GLenum origin)
	{
		const tcu::RenderTarget& renderTarget = context.getRenderContext().getRenderTarget();
		glw::GLuint				 viewportW	= renderTarget.getWidth();
		glw::GLuint				 viewportH	= renderTarget.getHeight();
		tcu::Surface			 renderedFrame(viewportW, viewportH);
		tcu::Surface			 referenceFrame(viewportW, viewportH);

		tcu::TestLog& log = context.getTestContext().getLog();

		for (int y = 0; y < renderedFrame.getHeight(); y++)
		{
			float yCoord = (float)(y) / (float)renderedFrame.getHeight();

			for (int x = 0; x < renderedFrame.getWidth(); x++)
			{

				float xCoord = (float)(x) / (float)renderedFrame.getWidth();

				bool greenQuadrant;

				if (origin == GL_UPPER_LEFT)
				{
					greenQuadrant = (yCoord > 0.5 && xCoord <= 0.5);
				}
				else
				{
					greenQuadrant = (yCoord <= 0.5 && xCoord <= 0.5);
				}

				if (greenQuadrant)
				{
					referenceFrame.setPixel(x, y, tcu::RGBA::green());
				}
				else
				{
					referenceFrame.setPixel(x, y, tcu::RGBA::red());
				}
			}
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		if (tcu::fuzzyCompare(log, "Result", "Image comparison result", referenceFrame, renderedFrame, 0.05f,
							  tcu::COMPARE_LOG_RESULT))
		{
			return QP_TEST_RESULT_PASS;
		}
		else
		{
			return QP_TEST_RESULT_FAIL;
		}
	}

	glw::GLuint m_vao, m_vbo;
};

/*   Depth Mode Test

 * Basic <depth> behavior can be tested by writing specific z_c (z
 clip coordinates) and observing its clipping and transformation.
 Create and bind a framebuffer object with a floating-point depth
 buffer attachment. Make sure depth clamping is disabled. The best
 steps for verifying the correct depth mode:

 - Clear the depth buffer to 0.5.
 - Set ClipControl(LOWER_LEFT, ZERO_TO_ONE)
 - Enable(DEPTH_TEST) with DepthFunc(ALWAYS)
 - Render a triangle fan coverage (-1.0,-1.0,-1.0) to (1.0,1.0,1.0).
 - Read back the floating-point depth buffer with ReadPixels
 - Verify that the pixels with a Z clip coordinate less than 0.0 are
 clipped and those coordinates from 0.0 to 1.0 update the depth
 buffer with values 0.0 to 1.0.
 */
class ClipControlDepthModeZeroToOneTest : public ClipControlRenderBaseTest
{
public:
	ClipControlDepthModeZeroToOneTest(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlRenderBaseTest(context, api, name, "Depth Mode Test, ZERO_TO_ONE"), m_vao(0), m_vbo(0)
	{
	}

	void deinit()
	{
		ClipControlRenderBaseTest::deinit();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}

		gl.clearDepth(0.0);
		gl.clearColor(0.0, 0.0, 0.0, 0.0);

		gl.disable(GL_DEPTH_TEST);
		gl.depthFunc(GL_LESS);

		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);
		}
		if (m_vbo)
		{
			gl.deleteBuffers(1, &m_vbo);
		}
	}

	IterateResult iterate()
	{

		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		gl.clearColor(1.0, 0.0, 0.0, 1.0);
		gl.clear(GL_COLOR_BUFFER_BIT);

		//Clear the depth buffer to 0.5.
		gl.clearDepth(0.5);
		gl.clear(GL_DEPTH_BUFFER_BIT);

		//Set ClipControl(LOWER_LEFT, ZERO_TO_ONE)
		cc.clipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");

		//Enable(DEPTH_TEST) with DepthFunc(ALWAYS)
		gl.enable(GL_DEPTH_TEST);
		gl.depthFunc(GL_ALWAYS);

		//Render a triangle fan coverage (-1.0,-1.0,-1.0) to (1.0,1.0,1.0).
		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));

		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}

		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);

		const float vertex_data0[] = {
			-1.0, -1.0, -1.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, 1.0, 1.0, 1.0,
		};

		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertex_data0), vertex_data0, GL_STATIC_DRAW);

		gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(0);

		gl.useProgram(program->getProgram());

		//test method modification: use GL_TRIANGLE_STRIP, not FAN.
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//Read back the floating-point depth buffer with ReadPixels
		//Verify that the pixels with a Z clip coordinate less than 0.0 are
		//  clipped and those coordinates from 0.0 to 1.0 update the depth
		//  buffer with values 0.0 to 1.0.
		qpTestResult result = ValidateFramebuffer(m_context);
		m_testCtx.setTestResult(result, qpGetTestResultName(result));

		return STOP;
	}

	const char* vsh()
	{
		return "#version 400"
			   "\n"
			   "in vec3 Position;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    gl_Position = vec4(Position, 1.0);"
			   "\n"
			   "}";
	}

	qpTestResult ValidateFramebuffer(deqp::Context& context)
	{
		const tcu::RenderTarget& renderTarget = context.getRenderContext().getRenderTarget();
		glw::GLuint				 viewportW	= renderTarget.getWidth();
		glw::GLuint				 viewportH	= renderTarget.getHeight();
		tcu::Surface			 renderedColorFrame(viewportW, viewportH);
		tcu::Surface			 referenceColorFrame(viewportW, viewportH);
		tcu::TextureFormat		 depthFormat(tcu::TextureFormat::D, tcu::TextureFormat::FLOAT);
		tcu::TextureLevel		 renderedDepthFrame(depthFormat, viewportW, viewportH);
		tcu::TextureLevel		 referenceDepthFrame(depthFormat, viewportW, viewportH);
		tcu::TextureLevel		 importanceMaskFrame(depthFormat, viewportW, viewportH);

		tcu::TestLog& log = context.getTestContext().getLog();

		const float rasterizationError =
			2.0f / (float)renderedColorFrame.getHeight() + 2.0f / (float)renderedColorFrame.getWidth();

		for (int y = 0; y < renderedColorFrame.getHeight(); y++)
		{
			float yCoord = ((float)(y) + 0.5f) / (float)renderedColorFrame.getHeight();

			for (int x = 0; x < renderedColorFrame.getWidth(); x++)
			{
				float xCoord = ((float)(x) + 0.5f) / (float)renderedColorFrame.getWidth();

				if (yCoord >= 1.0 - xCoord - rasterizationError && yCoord <= 1.0 - xCoord + rasterizationError)
				{
					importanceMaskFrame.getAccess().setPixDepth(0.0f, x, y);
				}
				else
				{
					importanceMaskFrame.getAccess().setPixDepth(1.0f, x, y);
				}

				if (yCoord < 1.0 - xCoord)
				{
					referenceColorFrame.setPixel(x, y, tcu::RGBA::red());
					referenceDepthFrame.getAccess().setPixDepth(0.5f, x, y);
				}
				else
				{
					referenceColorFrame.setPixel(x, y, tcu::RGBA::green());

					referenceDepthFrame.getAccess().setPixDepth(-1.0f + xCoord + yCoord, x, y);
				}
			}
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedColorFrame.getAccess());
		if (!tcu::fuzzyCompare(log, "Result", "Color image comparison result", referenceColorFrame, renderedColorFrame,
							   0.05f, tcu::COMPARE_LOG_RESULT))
		{

			return QP_TEST_RESULT_FAIL;
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedDepthFrame.getAccess());
		if (!fuzzyDepthCompare(log, "Result", "Depth image comparison result", referenceDepthFrame, renderedDepthFrame,
							   0.05f, &importanceMaskFrame))
		{
			return QP_TEST_RESULT_FAIL;
		}
		return QP_TEST_RESULT_PASS;
	}

	glw::GLuint m_vao, m_vbo;
};

/*
 Do the same as above, but use the default NEGATIVE_ONE_TO_ONE depth mode:

 - Clear the depth buffer to 0.5.
 - Set ClipControl(LOWER_LEFT, NEGATIVE_ONE_TO_ONE)
 - Enable(DEPTH_TEST) with DepthFunc(ALWAYS)
 - Render a triangle fan coverage (-1.0,-1.0,-1.0) to (1.0,1.0,1.0).
 - Read back the floating-point depth buffer with ReadPixels
 - Verify that no pixels are clipped and the depth buffer contains
 values from 0.0 to 1.0.
 */
class ClipControlDepthModeOneToOneTest : public ClipControlRenderBaseTest
{
public:
	ClipControlDepthModeOneToOneTest(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlRenderBaseTest(context, api, name, "Depth Mode Test, ZERO_TO_ONE"), m_vao(0), m_vbo(0)
	{
	}

	void deinit()
	{
		ClipControlRenderBaseTest::deinit();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}

		gl.clearDepth(0.0);
		gl.clearColor(0.0, 0.0, 0.0, 0.0);

		gl.disable(GL_DEPTH_TEST);
		gl.depthFunc(GL_LESS);

		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);
		}
		if (m_vbo)
		{
			gl.deleteBuffers(1, &m_vbo);
		}
	}

	IterateResult iterate()
	{
		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		gl.clearColor(1.0, 0.0, 0.0, 1.0);
		gl.clear(GL_COLOR_BUFFER_BIT);

		//Clear the depth buffer to 0.5.
		gl.clearDepth(0.5);
		gl.clear(GL_DEPTH_BUFFER_BIT);

		//Set ClipControl(LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE)
		cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");

		//Enable(DEPTH_TEST) with DepthFunc(ALWAYS)
		gl.enable(GL_DEPTH_TEST);
		gl.depthFunc(GL_ALWAYS);

		//Render a triangle fan coverage (-1.0,-1.0,-1.0) to (1.0,1.0,1.0).
		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));

		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}

		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);

		const float vertex_data0[] = {
			-1.0, -1.0, -1.0, 1.0, -1.0, 0.0, -1.0, 1.0, 0.0, 1.0, 1.0, 1.0,
		};

		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertex_data0), vertex_data0, GL_STATIC_DRAW);

		gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(0);

		gl.useProgram(program->getProgram());

		//test method modification: use GL_TRIANGLE_STRIP, not FAN.
		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//Read back the floating-point depth buffer with ReadPixels
		//Verify that the pixels with a Z clip coordinate less than 0.0 are
		//  clipped and those coordinates from 0.0 to 1.0 update the depth
		//  buffer with values 0.0 to 1.0.
		qpTestResult result = ValidateFramebuffer(m_context);
		m_testCtx.setTestResult(result, qpGetTestResultName(result));

		return STOP;
	}

	const char* vsh()
	{
		return "#version 400"
			   "\n"
			   "in vec3 Position;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    gl_Position = vec4(Position, 1.0);"
			   "\n"
			   "}";
	}

	qpTestResult ValidateFramebuffer(deqp::Context& context)
	{
		const tcu::RenderTarget& renderTarget = context.getRenderContext().getRenderTarget();
		glw::GLuint				 viewportW	= renderTarget.getWidth();
		glw::GLuint				 viewportH	= renderTarget.getHeight();
		tcu::Surface			 renderedColorFrame(viewportW, viewportH);
		tcu::Surface			 referenceColorFrame(viewportW, viewportH);
		tcu::TextureFormat		 depthFormat(tcu::TextureFormat::D, tcu::TextureFormat::FLOAT);
		tcu::TextureLevel		 renderedDepthFrame(depthFormat, viewportW, viewportH);
		tcu::TextureLevel		 referenceDepthFrame(depthFormat, viewportW, viewportH);

		tcu::TestLog& log = context.getTestContext().getLog();

		for (int y = 0; y < renderedColorFrame.getHeight(); y++)
		{
			float yCoord = (float)(y) / (float)renderedColorFrame.getHeight();
			for (int x = 0; x < renderedColorFrame.getWidth(); x++)
			{
				float xCoord = (float)(x) / (float)renderedColorFrame.getWidth();

				referenceColorFrame.setPixel(x, y, tcu::RGBA::green());
				referenceDepthFrame.getAccess().setPixDepth((xCoord + yCoord) * 0.5f, x, y);
			}
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedColorFrame.getAccess());
		if (!tcu::fuzzyCompare(log, "Result", "Color image comparison result", referenceColorFrame, renderedColorFrame,
							   0.05f, tcu::COMPARE_LOG_RESULT))
		{

			return QP_TEST_RESULT_FAIL;
		}
		glu::readPixels(context.getRenderContext(), 0, 0, renderedDepthFrame.getAccess());
		if (!fuzzyDepthCompare(log, "Result", "Depth image comparison result", referenceDepthFrame, renderedDepthFrame,
							   0.05f))
		{

			return QP_TEST_RESULT_FAIL;
		}

		return QP_TEST_RESULT_PASS;
	}

	glw::GLuint m_vao, m_vbo;
};

/*
 Clip Control Origin With Face Culling Test

 * Face culling should be tested with both <origin> settings.
 The reason for that is, when doing Y-inversion, implementation
 should not flip the calculated area sign for the triangle.
 In other words, culling of CCW and CW triangles should
 be orthogonal to used <origin> mode. Both triangle windings
 and both <origin> modes should be tested. Here is the basic
 outline of the test:

 - Clear the framebuffer to red (1,0,0).
 - Enable GL_CULL_FACE, leave default front face & cull face (CCW, BACK)
 - Set ClipControl(UPPER_LEFT, NEGATIVE_ONE_TO_ONE)
 - Render a counter-clockwise triangles covering
 (-1.0, -1.0) to (0.0, 1.0) and write a pixel value of green (0,1,0).
 - Render a clockwise triangles covering
 (0.0, -1.0) to (1.0, 1.0) and write a pixel value of green (0,1,0).
 - Read back the framebuffer with ReadPixels
 - Verify the green pixels at the left and red at the right.

 Repeat above test for ClipControl(LOWER_LEFT, NEGATIVE_ONE_TO_ONE)
 */
class ClipControlFaceCulling : public ClipControlRenderBaseTest
{
public:
	ClipControlFaceCulling(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlRenderBaseTest(context, api, name, "Face culling test, both origins"), m_vao(0), m_vbo(0)
	{
	}

	void deinit()
	{
		ClipControlRenderBaseTest::deinit();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}

		gl.disable(GL_CULL_FACE);

		gl.clearDepth(0.0);
		gl.clearColor(0.0, 0.0, 0.0, 0.0);

		gl.disable(GL_DEPTH_TEST);
		gl.depthFunc(GL_LESS);

		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);
		}
		if (m_vbo)
		{
			gl.deleteBuffers(1, &m_vbo);
		}
	}

	IterateResult iterate()
	{

		tcu::TestLog&		  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		ClipControlApi		  cc(m_context, m_api);

		//Enable GL_CULL_FACE, leave default front face & cull face(CCW, BACK)
		gl.enable(GL_CULL_FACE);

		//Render a counter-clockwise triangles covering
		//(-1.0, -1.0) to(0.0, 1.0) and write a pixel value of green(0, 1, 0).
		//Render a clockwise triangles covering
		//(0.0, -1.0) to(1.0, 1.0) and write a pixel value of green(0, 1, 0).
		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));

		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}

		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);

		const float vertex_data0[] = {
			//CCW
			-1.0, -1.0, 0.0, -1.0, -1.0, 1.0, 0.0, -1.0, 0.0, 1.0, -1.0, 1.0,
			//CW
			0.0, -1.0, 0.0, 1.0, 1.0, -1.0, 1.0, -1.0, 0.0, 1.0, 1.0, 1.0,
		};

		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertex_data0), vertex_data0, GL_STATIC_DRAW);

		gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(0);

		gl.useProgram(program->getProgram());

		glw::GLenum origins[] = { GL_UPPER_LEFT, GL_LOWER_LEFT };

		qpTestResult result = QP_TEST_RESULT_PASS;

		for (size_t orig = 0; orig < DE_LENGTH_OF_ARRAY(origins); orig++)
		{
			//Clear the framebuffer to red (1,0,0).
			gl.clearColor(1.0, 0.0, 0.0, 1.0);
			gl.clear(GL_COLOR_BUFFER_BIT);

			gl.drawArrays(GL_TRIANGLES, 0, 12);

			//Set ClipControl(<origin>, NEGATIVE_ONE_TO_ONE)
			cc.clipControl(origins[orig], GL_NEGATIVE_ONE_TO_ONE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");

			//Read back the framebuffer with ReadPixels
			//Verify the green pixels at the left and red at the right.
			qpTestResult loopResult = ValidateFramebuffer(m_context);
			if (loopResult != QP_TEST_RESULT_PASS)
			{
				result = loopResult;
			}
		}
		m_testCtx.setTestResult(result, qpGetTestResultName(result));

		return STOP;
	}

	const char* vsh()
	{
		return "#version 400"
			   "\n"
			   "in vec3 Position;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    gl_Position = vec4(Position, 1.0);"
			   "\n"
			   "}";
	}

	qpTestResult ValidateFramebuffer(deqp::Context& context)
	{
		const tcu::RenderTarget& renderTarget = context.getRenderContext().getRenderTarget();
		glw::GLuint				 viewportW	= renderTarget.getWidth();
		glw::GLuint				 viewportH	= renderTarget.getHeight();
		tcu::Surface			 renderedColorFrame(viewportW, viewportH);
		tcu::Surface			 referenceColorFrame(viewportW, viewportH);
		tcu::TestLog&			 log = context.getTestContext().getLog();

		for (int y = 0; y < renderedColorFrame.getHeight(); y++)
		{
			for (int x = 0; x < renderedColorFrame.getWidth(); x++)
			{
				float xCoord = (float)(x) / (float)renderedColorFrame.getWidth();

				if (xCoord < 0.5)
				{
					referenceColorFrame.setPixel(x, y, tcu::RGBA::green());
				}
				else
				{
					referenceColorFrame.setPixel(x, y, tcu::RGBA::red());
				}
			}
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedColorFrame.getAccess());
		if (!tcu::fuzzyCompare(log, "Result", "Color image comparison result", referenceColorFrame, renderedColorFrame,
							   0.05f, tcu::COMPARE_LOG_RESULT))
		{

			return QP_TEST_RESULT_FAIL;
		}
		return QP_TEST_RESULT_PASS;
	}

	glw::GLuint m_vao, m_vbo;
};

/*
 Viewport Bounds Test

 * Viewport bounds should be tested, to ensure that rendering with flipped
 origin affects only viewport area.

 This can be done by clearing the window to blue, making viewport
 a non-symmetric-in-any-way subset of the window, than rendering
 full-viewport multiple color quad. The (-1.0, -1.0)..(0.0, 0.0) quadrant
 of a quad is red, the rest is green.
 Whatever the origin is, the area outside of the viewport should stay blue.
 If origin is LOWER_LEFT the "lower left" portion of the viewport is red,
 if origin is UPPER_LEFT the "top left" portion of the viewport is red
 (and in both cases the rest of viewport is green).

 Here is the basic outline of the test:

 - Clear the default framebuffer to blue (0,0,1).
 - Set viewport to A = (x, y, w, h) = (1/8, 1/4, 1/2, 1/4)  in terms of proportional window size
 - Set ClipControl(UPPER_LEFT, NEGATIVE_ONE_TO_ONE)
 - Render a triangle strip covering (-1.0, -1.0) to (1.0, 1.0).
 Write a pixel value of red (0,1,0) to (-1.0, -1.0)..(0.0, 0.0), other parts are green
 - Reset viewport to defaults
 - Read back the default framebuffer with ReadPixels
 - Verify:
 - regions outside A viewport are green
 - Inside A viewport upper upper left portion is red, rest is green.

 Repeat the above test with LOWER_LEFT origin and lower left portion of A is red,
 rest is green.
 */
class ClipControlViewportBounds : public ClipControlRenderBaseTest
{
public:
	ClipControlViewportBounds(deqp::Context& context, ClipControlTests::API api, const char* name)
		: ClipControlRenderBaseTest(context, api, name, "Clip Control Origin Test"), m_vao(0), m_vbo(0)
	{
	}

	void deinit()
	{
		ClipControlRenderBaseTest::deinit();

		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		glw::GLuint				 windowW	  = renderTarget.getWidth();
		glw::GLuint				 windowH	  = renderTarget.getHeight();
		const glw::Functions&	gl			  = m_context.getRenderContext().getFunctions();

		if (ClipControlApi::Supported(m_context, m_api))
		{
			ClipControlApi cc(m_context, m_api);
			cc.clipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
		}

		gl.clearColor(0.0, 0.0, 0.0, 0.0);
		gl.viewport(0, 0, windowW, windowH);

		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);
		}
		if (m_vbo)
		{
			gl.deleteBuffers(1, &m_vbo);
		}
	}

	IterateResult iterate()
	{
		tcu::TestLog&			 log		  = m_testCtx.getLog();
		const glw::Functions&	gl			  = m_context.getRenderContext().getFunctions();
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		glw::GLuint				 windowW	  = renderTarget.getWidth();
		glw::GLuint				 windowH	  = renderTarget.getHeight();
		ClipControlApi			 cc(m_context, m_api);

		//Clear the default framebuffer to blue (0,0,1).
		gl.clearColor(0.0, 0.0, 1.0, 1.0);
		gl.clear(GL_COLOR_BUFFER_BIT);

		de::SharedPtr<glu::ShaderProgram> program(
			new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(vsh(), fsh())));

		log << (*program);
		if (!program->isOk())
		{
			TCU_FAIL("Program compilation failed");
		}
		gl.genVertexArrays(1, &m_vao);
		gl.bindVertexArray(m_vao);

		gl.genBuffers(1, &m_vbo);

		const float vertex_data0[] = { -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0 };

		gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertex_data0), vertex_data0, GL_STATIC_DRAW);

		gl.vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(0);

		gl.useProgram(program->getProgram());

		glw::GLenum origins[] = { GL_UPPER_LEFT, GL_LOWER_LEFT };

		qpTestResult result = QP_TEST_RESULT_PASS;

		for (size_t orig = 0; orig < DE_LENGTH_OF_ARRAY(origins); orig++)
		{
			//Set viewport to A = (x, y, w, h) = (1/8, 1/4, 1/2, 1/4) in terms of proportional window size
			gl.viewport((glw::GLint)(0.125f * (float)windowW), (glw::GLint)(0.25f * (float)windowH),
						(glw::GLsizei)(0.5f * (float)windowW), (glw::GLsizei)(0.25f * (float)windowH));

			//Set ClipControl(<origin>, NEGATIVE_ONE_TO_ONE)
			cc.clipControl(origins[orig], GL_NEGATIVE_ONE_TO_ONE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClipControl()");

			//Render a triangle strip covering (-1.0, -1.0) to (1.0, 1.0).
			//Write a pixel value of red (0,1,0) to (-1.0, -1.0)..(0.0, 0.0), other parts are green
			gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

			gl.viewport(0, 0, windowW, windowH);

			//Read back the default framebuffer with ReadPixels
			//Verify the green pixels at the top and red at the bottom.
			qpTestResult loopResult = ValidateFramebuffer(m_context, origins[orig]);
			if (loopResult != QP_TEST_RESULT_PASS)
			{
				result = loopResult;
			}
		}
		m_testCtx.setTestResult(result, qpGetTestResultName(result));
		return STOP;
	}

	const char* vsh()
	{
		return "#version 400"
			   "\n"
			   "in  vec2 Position;"
			   "\n"
			   "out vec2 PositionOut;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    gl_Position = vec4(Position, 0.0, 1.0);"
			   "\n"
			   "    PositionOut = Position;"
			   "\n"
			   "}";
	}

	const char* fsh()
	{
		return "#version 400"
			   "\n"
			   "in  vec2 PositionOut;"
			   "\n"
			   "out vec4 FragColor;"
			   "\n"
			   "void main() {"
			   "\n"
			   "    if (PositionOut.x < 0.0 && PositionOut.y < 0.0)"
			   "\n"
			   "       FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
			   "\n"
			   "    else"
			   "\n"
			   "       FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
			   "\n"
			   "}";
	}

	qpTestResult ValidateFramebuffer(deqp::Context& context, glw::GLenum origin)
	{
		const tcu::RenderTarget& renderTarget = context.getRenderContext().getRenderTarget();
		glw::GLuint				 windowW	  = renderTarget.getWidth();
		glw::GLuint				 windowH	  = renderTarget.getHeight();
		tcu::Surface			 renderedFrame(windowW, windowH);
		tcu::Surface			 referenceFrame(windowW, windowH);

		tcu::TestLog& log = context.getTestContext().getLog();

		for (int y = 0; y < renderedFrame.getHeight(); y++)
		{
			float yCoord   = (float)(y) / (float)renderedFrame.getHeight();
			float yVPCoord = (yCoord - 0.25f) * 4.0f;

			for (int x = 0; x < renderedFrame.getWidth(); x++)
			{
				float xCoord   = (float)(x) / (float)renderedFrame.getWidth();
				float xVPCoord = (xCoord - 0.125f) * 2.0f;

				if (xVPCoord > 0.0f && xVPCoord < 1.0f && yVPCoord > 0.0f && yVPCoord < 1.0f)
				{

					bool greenQuadrant;

					//inside viewport
					if (origin == GL_UPPER_LEFT)
					{
						greenQuadrant = (yVPCoord > 0.5f && xVPCoord <= 0.5f);
					}
					else
					{
						greenQuadrant = (yVPCoord <= 0.5f && xVPCoord <= 0.5f);
					}

					if (greenQuadrant)
					{
						referenceFrame.setPixel(x, y, tcu::RGBA::green());
					}
					else
					{
						referenceFrame.setPixel(x, y, tcu::RGBA::red());
					}
				}
				else
				{
					//outside viewport
					referenceFrame.setPixel(x, y, tcu::RGBA::blue());
				}
			}
		}

		glu::readPixels(context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		if (tcu::fuzzyCompare(log, "Result", "Image comparison result", referenceFrame, renderedFrame, 0.05f,
							  tcu::COMPARE_LOG_RESULT))
		{
			return QP_TEST_RESULT_PASS;
		}
		else
		{
			return QP_TEST_RESULT_FAIL;
		}
	}

	glw::GLuint m_vao, m_vbo;
};

const char* apiToTestName(ClipControlTests::API api)
{
	switch (api)
	{
	case ClipControlTests::API_GL_45core:
		return "clip_control";
	case ClipControlTests::API_GL_ARB_clip_control:
		return "clip_control_ARB";
	}
	DE_ASSERT(0);
	return "";
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
ClipControlTests::ClipControlTests(deqp::Context& context, API api)
	: TestCaseGroup(context, apiToTestName(api), "Verifies \"clip_control\" functionality"), m_api(api)
{
	/* Left blank on purpose */
}

/** Destructor.
 *
 **/
ClipControlTests::~ClipControlTests()
{
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void ClipControlTests::init(void)
{
	addChild(new ClipControlInitialState(m_context, m_api, "initial"));
	addChild(new ClipControlModifyGetState(m_context, m_api, "modify_get"));
	addChild(new ClipControlErrors(m_context, m_api, "errors"));
	addChild(new ClipControlOriginTest(m_context, m_api, "origin"));
	addChild(new ClipControlDepthModeZeroToOneTest(m_context, m_api, "depth_mode_zero_to_one"));
	addChild(new ClipControlDepthModeOneToOneTest(m_context, m_api, "depth_mode_one_to_one"));
	addChild(new ClipControlFaceCulling(m_context, m_api, "face_culling"));
	addChild(new ClipControlViewportBounds(m_context, m_api, "viewport_bounds"));
}
} /* deqp namespace */
