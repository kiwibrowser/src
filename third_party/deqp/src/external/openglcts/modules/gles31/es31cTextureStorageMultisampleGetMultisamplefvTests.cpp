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
 */ /*!
 * \file  es31cTextureStorageMultisampleGetMultisamplefvTests.cpp
 * \brief Implements conformance tests testing whether glGetMultisamplefv()
 *        works correctly. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleGetMultisamplefvTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest::
	MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest(Context& context)
	: TestCase(context, "multisample_texture_get_multisamplefv_index_equal_gl_samples_rejected",
			   "Verifies GetMultisamplefv() rejects index equal to GL_SAMPLES value")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest::iterate()
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_samples_value = 0;

	/* Get GL_SAMPLES value */
	gl.getIntegerv(GL_SAMPLES, &gl_samples_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_SAMPLES value");

	/* Issue call with valid parameters, but invalid index equal to GL_SAMPLES value */
	glw::GLfloat val[2];
	gl.getMultisamplefv(GL_SAMPLE_POSITION, gl_samples_value, val);

	/* Check if the expected error code was reported */
	if (gl.getError() != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest::
	MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest(Context& context)
	: TestCase(context, "multisample_texture_get_multisamplefv_index_greater_gl_samples_rejected",
			   "Verifies GetMultisamplefv() rejects index greater than GL_SAMPLES value")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest::iterate()
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_samples_value = 0;

	/* Get GL_SAMPLES value */
	gl.getIntegerv(GL_SAMPLES, &gl_samples_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_SAMPLES value");

	/* Issue call with valid parameters, but invalid index greater than GL_SAMPLES value */
	glw::GLfloat val[2];
	gl.getMultisamplefv(GL_SAMPLE_POSITION, gl_samples_value + 1, val);

	/* Check if the expected error code was reported */
	if (gl.getError() != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest::MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest(
	Context& context)
	: TestCase(context, "multisample_texture_get_multisamplefv_invalid_pname_rejected",
			   "Verifies GetMultisamplefv() rejects invalid pname")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Issue call with valid parameters, but invalid pname GL_SAMPLES */
	glw::GLfloat val[2];
	gl.getMultisamplefv(GL_SAMPLES, 0, val);

	/* Check if the expected error code was reported */
	glw::GLenum error_code = gl.getError();

	if (error_code != GL_INVALID_ENUM)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest::
	MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest(Context& context)
	: TestCase(context, "multisample_texture_get_multisamplefv_null_val_arguments_accepted",
			   "Verifies NULL val arguments accepted for valid glGetMultisamplefv() calls.")
	, fbo_id(0)
	, to_2d_multisample_id(0)
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest::iterate()
{
	/* Issue call with valid parameters, but invalid pname GL_SAMPLES */
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	/* gl.getMultisamplefv(GL_SAMPLES, 0, NULL) is not legal, removed */

	/* Create multisampled FBO, as default framebuffer is not multisampled */

	gl.genTextures(1, &to_2d_multisample_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	if (to_2d_multisample_id == 0)
	{
		TCU_FAIL("Texture object has not been generated properly");
	}

	glw::GLint samples = 0;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, /* target */
						   GL_RGBA8, GL_SAMPLES, 1,   /* bufSize */
						   &samples);

	GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ() call failed");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

	/* Configure the texture object storage */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, 1, /* width */
							   1,												/* height */
							   GL_TRUE);										/* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2DMultisample() call failed");

	gl.genFramebuffers(1, &fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");

	if (fbo_id == 0)
	{
		TCU_FAIL("Framebuffer object has not been generated properly");
	}

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id,
							0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up framebuffer's attachments");

	glw::GLenum fbo_completeness_status = 0;

	fbo_completeness_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness_status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Source FBO completeness status is: " << fbo_completeness_status
						   << ", expected: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Source FBO is considered incomplete which is invalid");
	}

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	/* Get GL_SAMPLES value */
	glw::GLint gl_samples_value = 0;

	gl.getIntegerv(GL_SAMPLES, &gl_samples_value);

	error_code = gl.getError();
	GLU_EXPECT_NO_ERROR(error_code, "Failed to retrieve GL_SAMPLES value");

	glw::GLfloat val[2];
	gl.getMultisamplefv(GL_SAMPLE_POSITION, 0, val);

	error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "glGetMultisamplefv() call failed");

	/* Iterate through valid index values */
	for (glw::GLint index = 0; index < gl_samples_value; ++index)
	{
		/* Execute the test */
		gl.getMultisamplefv(GL_SAMPLE_POSITION, index, val);

		error_code = gl.getError();
		GLU_EXPECT_NO_ERROR(error_code, "A valid glGetMultisamplefv() call reported an error");
	} /* for (all valid index argument values) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind framebuffer object bound to GL_DRAW_FRAMEBUFFER target */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	/* Delete a 2D multisample texture object of id to_2d_multisample_id */
	if (to_2d_multisample_id != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_id);

		to_2d_multisample_id = 0;
	}

	/* Delete a framebuffer object of id fbo_id */
	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest::
	MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest(Context& context)
	: TestCase(context, "multisample_texture_get_multisamplefv_sample_position_values_validation",
			   "Verifies spec-wise correct values are reported for valid calls with GL_SAMPLE_POSITION pname")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest::iterate()
{
	/* Get GL_SAMPLES value */
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_samples_value = 0;

	gl.getIntegerv(GL_SAMPLES, &gl_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_SAMPLES value");

	/* Iterate through valid index values */
	for (glw::GLint index = 0; index < gl_samples_value; ++index)
	{
		/* Execute the test */
		glw::GLfloat val[2] = { -1.0f, -1.0f };

		gl.getMultisamplefv(GL_SAMPLE_POSITION, index, val);
		GLU_EXPECT_NO_ERROR(gl.getError(), "A valid glGetMultisamplefv() call reported an error");

		if (val[0] < 0.0f || val[0] > 1.0f || val[1] < 0.0f || val[1] > 1.0f)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "One or more coordinates used to describe sample position: "
							   << "(" << val[0] << ", " << val[1] << ") is outside the valid <0, 1> range."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid sample position reported");
		}
	} /* for (all valid index argument values) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
