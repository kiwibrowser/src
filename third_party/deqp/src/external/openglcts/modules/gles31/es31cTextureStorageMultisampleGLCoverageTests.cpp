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
 * \file  es31cTextureStorageMultisampleGLCoverageTests.cpp
 * \brief Implements coverage tests for multisample textures. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleGLCoverageTests.hpp"
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
GLCoverageExtensionSpecificEnumsAreRecognizedTest::GLCoverageExtensionSpecificEnumsAreRecognizedTest(Context& context)
	: TestCase(context, "extension_specific_enums_are_recognized",
			   "GL_MAX_SAMPLE_MASK_WORDS, GL_MAX_COLOR_TEXTURE_SAMPLES, GL_MAX_DEPTH_TEXTURE_SAMPLES"
			   ", GL_MAX_INTEGER_SAMPLES, GL_TEXTURE_BINDING_2D_MULTISAMPLE and"
			   " GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES enums are recognized.")
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, to_id_2d_multisample(0)
	, to_id_2d_multisample_array(0)
{
	/* Left blank on purpose */
}

/* Deinitializes ES objects that may have been created while the test executed. */
void GLCoverageExtensionSpecificEnumsAreRecognizedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id_2d_multisample != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample);

		to_id_2d_multisample = 0;
	}

	if (to_id_2d_multisample_array != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_array);

		to_id_2d_multisample_array = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GLCoverageExtensionSpecificEnumsAreRecognizedTest::iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Iterate through all pnames that need to be verified */
	const glw::GLenum pnames[] = { GL_MAX_SAMPLE_MASK_WORDS, GL_MAX_COLOR_TEXTURE_SAMPLES, GL_MAX_DEPTH_TEXTURE_SAMPLES,
								   GL_MAX_INTEGER_SAMPLES };
	const unsigned int n_pnames = sizeof(pnames) / sizeof(pnames[0]);

	for (unsigned int n_pname = 0; n_pname < n_pnames; ++n_pname)
	{
		const float epsilon = (float)1e-5;
		glw::GLenum pname   = pnames[n_pname];

		/* Test glGetIntegerv() */
		glw::GLint int_value = -1;

		gl.getIntegerv(pname, &int_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() reported an error for a valid pname");

		if (int_value < 1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "An invalid integer value " << int_value
							   << " was reported for pname [" << pname << "]." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid integer value reported for pname.");
		}

		/* Test glGetBooleanv() */
		glw::GLboolean bool_value = GL_TRUE;

		gl.getBooleanv(pname, &bool_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() reported an error for a valid pname");

		if ((int_value != 0 && bool_value == GL_FALSE) || (int_value == 0 && bool_value != GL_FALSE))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "An invalid boolean value [" << bool_value << "]"
							   << " was reported for pname [" << pname << "]"
							   << " (integer value reported for the same property:" << int_value << ")"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid boolean value reported for pname.");
		}

		/* Test glGetFloatv() */
		glw::GLfloat float_value = -1.0f;

		gl.getFloatv(pname, &float_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() reported an error for a valid pname");

		if (de::abs(float_value - float(int_value)) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "An invalid floating-point value [" << float_value << "]"
							   << " was reported for pname        [" << pname << "]"
							   << " (integer value reported for the same property:" << int_value << ")"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid floating-point value reported for pname.");
		}
	} /* for (all pnames) */

	/* Verify default multisample texture bindings are valid */
	glw::GLboolean bool_value  = GL_TRUE;
	const float	epsilon	 = (float)1e-5;
	glw::GLfloat   float_value = 1.0f;
	glw::GLint	 int_value   = 1;

	gl.getBooleanv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &bool_value);
	gl.getFloatv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &float_value);
	gl.getIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"GL_TEXTURE_BINDING_2D_MULTISAMPLE pname was not recognized by one of the glGet*() functions");

	if (bool_value != GL_FALSE || de::abs(float_value) > epsilon || int_value != 0)
	{
		TCU_FAIL("Default GL_TEXTURE_BINDING_2D_MULTISAMPLE value is invalid");
	}

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		bool_value  = GL_TRUE;
		float_value = 1.0f;
		int_value   = 1;

		gl.getBooleanv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &bool_value);
		gl.getFloatv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &float_value);
		gl.getIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &int_value);

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES pname was not recognized by one of the glGet*() functions");

		if (bool_value != GL_FALSE || de::abs(float_value) > epsilon || int_value != 0)
		{
			TCU_FAIL("Default GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES value is invalid");
		}
	}

	/* Generate two texture objects we will later bind to multisample texture targets to
	 * verify the values reported for corresponding pnames have also changed.
	 */
	gl.genTextures(1, &to_id_2d_multisample);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.genTextures(1, &to_id_2d_multisample_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed");

	/* Now bind the IDs to relevant texture targets */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_multisample_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed");

	/* Verify new bindings are reported */
	bool_value  = GL_FALSE;
	float_value = 0.0f;
	int_value   = 0;

	gl.getBooleanv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &bool_value);
	gl.getFloatv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &float_value);
	gl.getIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &int_value);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"GL_TEXTURE_BINDING_2D_MULTISAMPLE pname was not recognized by one of the glGet*() functions");

	glw::GLfloat expected_float_value = float(to_id_2d_multisample);

	if (bool_value != GL_TRUE || de::abs(float_value - expected_float_value) > epsilon ||
		(glw::GLuint)int_value != to_id_2d_multisample)
	{
		TCU_FAIL("GL_TEXTURE_BINDING_2D_MULTISAMPLE value is invalid");
	}

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		bool_value  = GL_FALSE;
		float_value = 0.0f;
		int_value   = 0;

		gl.getBooleanv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &bool_value);
		gl.getFloatv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &float_value);
		gl.getIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES, &int_value);

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES pname was not recognized by one of the glGet*() functions");

		expected_float_value = float(to_id_2d_multisample_array);

		if (bool_value != GL_TRUE || de::abs(float_value - expected_float_value) > epsilon ||
			(glw::GLuint)int_value != to_id_2d_multisample_array)
		{
			TCU_FAIL("GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES value is invalid");
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets::
	GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets(Context& context)
	: TestCase(context, "get_tex_parameter_reports_correct_default_values_for_multisample_texture_targets",
			   "glGetTexParameter*() report correct default values for multisample texture targets.")
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, to_id_2d(0)
	, to_id_2d_array(0)
{
	/* Left blank on purpose */
}

/* Deinitializes test-specific ES objects */
void GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id_2d != 0)
	{
		gl.deleteTextures(1, &to_id_2d);

		to_id_2d = 0;
	}

	if (to_id_2d_array != 0)
	{
		gl.deleteTextures(1, &to_id_2d_array);

		to_id_2d_array = 0;
	}

	/* Call base test class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets::
	iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	const float			  epsilon = (float)1e-5;
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();

	/* Generate texture objects */
	gl.genTextures(1, &to_id_2d);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.genTextures(1, &to_id_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGenTextures() call failed");

	/* Bind the texture objects to multisample texture targets */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glBindTexture() call failed");

	/* Initialize texture storage */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 4,					 /* width */
							   4,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGBA8, 4,							   /* width */
								   4,									   /* height */
								   4,									   /* depth */
								   GL_TRUE);							   /* fixedsamplelocations */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture storage initialization failed");

	/* List of supported texture targets when the extension is not present. */
	glw::GLenum texture_targets_without_extension[] = { GL_TEXTURE_2D_MULTISAMPLE };

	/* List of supported texture targets when the extension is present. */
	glw::GLenum texture_targets_with_extension[] = { GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES };

	/* Iterate through all the texture targets supported for the specific case. */
	glw::GLenum* texture_targets = NULL;

	unsigned int n_texture_targets = 0;

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		n_texture_targets = sizeof(texture_targets_with_extension) / sizeof(texture_targets_with_extension[0]);
		texture_targets   = texture_targets_with_extension;
	}
	else
	{
		n_texture_targets = sizeof(texture_targets_without_extension) / sizeof(texture_targets_without_extension[0]);
		texture_targets   = texture_targets_without_extension;
	}

	for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
	{
		glw::GLenum target = texture_targets[n_texture_target];

		/* Iterate through all texture parameters */
		const glw::GLenum texture_parameters[] = {
			GL_TEXTURE_BASE_LEVEL, GL_TEXTURE_IMMUTABLE_FORMAT, GL_TEXTURE_MAX_LEVEL, GL_TEXTURE_SWIZZLE_R,
			GL_TEXTURE_SWIZZLE_G,  GL_TEXTURE_SWIZZLE_B,		GL_TEXTURE_SWIZZLE_A,

			/* The following are sampler states, hence are left out */

			// GL_TEXTURE_COMPARE_FUNC,
			// GL_TEXTURE_COMPARE_MODE,
			// GL_TEXTURE_MAG_FILTER,
			// GL_TEXTURE_MAX_LOD,
			// GL_TEXTURE_MIN_FILTER,
			// GL_TEXTURE_MIN_LOD,
			// GL_TEXTURE_WRAP_S,
			// GL_TEXTURE_WRAP_T,
			// GL_TEXTURE_WRAP_R
		};
		const unsigned int n_texture_parameters = sizeof(texture_parameters) / sizeof(texture_parameters[0]);

		for (unsigned int n_texture_parameter = 0; n_texture_parameter < n_texture_parameters; ++n_texture_parameter)
		{
			glw::GLenum texture_parameter = texture_parameters[n_texture_parameter];

			/* Query implementation for the parameter values for texture target considered */
			glw::GLint   expected_int_value = 0;
			glw::GLfloat float_value		= 0.0f;
			glw::GLint   int_value			= 0;

			gl.getTexParameterfv(target, texture_parameter, &float_value);
			gl.getTexParameteriv(target, texture_parameter, &int_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Either of the glGetTexParameter*() calls generated an error");

			/* Verify the value is valid */
			switch (texture_parameter)
			{
			case GL_TEXTURE_BASE_LEVEL:
				expected_int_value = 0;
				break;
			case GL_TEXTURE_COMPARE_FUNC:
				expected_int_value = GL_LEQUAL;
				break;
			case GL_TEXTURE_COMPARE_MODE:
				expected_int_value = GL_NONE;
				break;
			case GL_TEXTURE_IMMUTABLE_FORMAT:
				expected_int_value = GL_TRUE;
				break;
			case GL_TEXTURE_MAG_FILTER:
				expected_int_value = GL_LINEAR;
				break;
			case GL_TEXTURE_MAX_LEVEL:
				expected_int_value = 1000;
				break;
			case GL_TEXTURE_MAX_LOD:
				expected_int_value = 1000;
				break;
			case GL_TEXTURE_MIN_FILTER:
				expected_int_value = GL_NEAREST_MIPMAP_LINEAR;
				break;
			case GL_TEXTURE_MIN_LOD:
				expected_int_value = -1000;
				break;
			case GL_TEXTURE_SWIZZLE_R:
				expected_int_value = GL_RED;
				break;
			case GL_TEXTURE_SWIZZLE_G:
				expected_int_value = GL_GREEN;
				break;
			case GL_TEXTURE_SWIZZLE_B:
				expected_int_value = GL_BLUE;
				break;
			case GL_TEXTURE_SWIZZLE_A:
				expected_int_value = GL_ALPHA;
				break;
			case GL_TEXTURE_WRAP_S:
				expected_int_value = GL_REPEAT;
				break;
			case GL_TEXTURE_WRAP_T:
				expected_int_value = GL_REPEAT;
				break;
			case GL_TEXTURE_WRAP_R:
				expected_int_value = GL_REPEAT;
				break;

			default:
			{
				TCU_FAIL("Unrecognized texture parameter name");
			}
			} /* switch (texture_parameter) */

			/* Verify integer value the implementation returned */
			if (expected_int_value != int_value)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Implementation returned an invalid integer value of "
								   << int_value << " instead of the expected value " << expected_int_value
								   << " for property " << texture_parameter << " of texture target " << target
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid integer value returned by glGetTexParameteriv()");
			}

			/* Verify floating-point value the implementation returned */
			if (de::abs(float(expected_int_value) - float_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Implementation returned an invalid floating-point value of " << float_value
								   << " instead of the expected value " << expected_int_value << " for property "
								   << texture_parameter << " of texture target " << target << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid floating-point value returned by glGetTexParameteriv()");
			}
		} /* for (all texture parameters) */
	}	 /* for (all texture targets) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest::GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest(
	Context& context)
	: TestCase(context, "gl_sample_mask_mode_status_is_reported_correctly",
			   "glGet*() calls report correct disabled/enabled status of GL_SAMPLE_MASK mode.")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest::iterate()
{
	const float			  epsilon = (float)1e-5;
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();

	/* Check GL_SAMPLE_MASK is disabled by default */
	glw::GLint enabled = gl.isEnabled(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Unexpected error generated by glIsEnabled(GL_SAMPLE_MASK) call.");

	if (enabled != GL_FALSE)
	{
		TCU_FAIL("GL_SAMPLE_MASK mode is considered enabled by default which is incorrect.");
	}

	/* Verify glGet*() calls also report the mode to be disabled */
	glw::GLboolean bool_value  = GL_TRUE;
	glw::GLfloat   float_value = 1.0f;
	glw::GLint	 int_value   = 1;

	gl.getBooleanv(GL_SAMPLE_MASK, &bool_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() reported an error when queried for GL_SAMPLE_MASK property");

	if (bool_value != GL_FALSE)
	{
		TCU_FAIL("Invalid boolean value reported for GL_SAMPLE_MASK property");
	}

	gl.getFloatv(GL_SAMPLE_MASK, &float_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() reported an error when queried for GL_SAMPLE_MASK property");

	if (de::abs(float_value - float(GL_FALSE)) > epsilon)
	{
		TCU_FAIL("Invalid float value reported for GL_SAMPLE_MASK property");
	}

	gl.getIntegerv(GL_SAMPLE_MASK, &int_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() reported an error when queried for GL_SAMPLE_MASK property");

	if (int_value != GL_FALSE)
	{
		TCU_FAIL("Invalid integer value reported for GL_SAMPLE_MASK property");
	}

	/* Enable GL_SAMPLE_MASK mode */
	gl.enable(GL_SAMPLE_MASK);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK) call generated an unexpected error");

	/* Verify values reported by glIsEnabled(), as well as GL getters is still correct */
	enabled = gl.isEnabled(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsEnabled(GL_SAMPLE_MASK) call generated an unexpected error");

	if (enabled != GL_TRUE)
	{
		TCU_FAIL(
			"glIsEnabled(GL_SAMPLE_MASK) did not report GL_TRUE after a successful glEnable(GL_SAMPLE_MASK) call.");
	}

	gl.getBooleanv(GL_SAMPLE_MASK, &bool_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() reported an error when queried for GL_SAMPLE_MASK property");

	if (bool_value != GL_TRUE)
	{
		TCU_FAIL("Invalid boolean value reported for GL_SAMPLE_MASK property");
	}

	gl.getFloatv(GL_SAMPLE_MASK, &float_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() reported an error when queried for GL_SAMPLE_MASK property");

	if (de::abs(float_value - float(GL_TRUE)) > epsilon)
	{
		TCU_FAIL("Invalid float value reported for GL_SAMPLE_MASK property");
	}

	gl.getIntegerv(GL_SAMPLE_MASK, &int_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() reported an error when queried for GL_SAMPLE_MASK property");

	if (int_value != GL_TRUE)
	{
		TCU_FAIL("Invalid integer value reported for GL_SAMPLE_MASK property");
	}

	/* Disable the mode and see if the getters react accordingly */
	gl.disable(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_SAMPLE_MASK) call generated an unexpected error");

	enabled = gl.isEnabled(GL_SAMPLE_MASK);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIsEnabled(GL_SAMPLE_MASK) call generated an unexpected error");

	if (enabled != GL_FALSE)
	{
		TCU_FAIL(
			"glIsEnabled(GL_SAMPLE_MASK) did not report GL_FALSE after a successful glDisable(GL_SAMPLE_MASK) call.");
	}

	gl.getBooleanv(GL_SAMPLE_MASK, &bool_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() reported an error when queried for GL_SAMPLE_MASK property");

	if (bool_value != GL_FALSE)
	{
		TCU_FAIL("Invalid boolean value reported for GL_SAMPLE_MASK property");
	}

	gl.getFloatv(GL_SAMPLE_MASK, &float_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() reported an error when queried for GL_SAMPLE_MASK property");

	if (de::abs(float_value - float(GL_FALSE)) > epsilon)
	{
		TCU_FAIL("Invalid float value reported for GL_SAMPLE_MASK property");
	}

	gl.getIntegerv(GL_SAMPLE_MASK, &int_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() reported an error when queried for GL_SAMPLE_MASK property");

	if (int_value != GL_FALSE)
	{
		TCU_FAIL("Invalid integer value reported for GL_SAMPLE_MASK property");
	}

	/* All good! */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest::
	GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest(Context& context)
	: TestCase(context, "gl_tex_parameter_handlers_accept_zero_base_level",
			   "glTexParameter*() calls should not generate an error if zero base "
			   "level is requested for 2D multisample or 2D multisample array texture targets.")
	, are_2d_array_multisample_tos_supported(false)
	, to_id_2d(0)
	, to_id_2d_array(0)
{
	/* Left blank on purpose */
}

/* Deinitializes texture objects created specifically for this test case. */
void GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id_2d != 0)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		gl.deleteTextures(1, &to_id_2d);
	}

	if (to_id_2d_array != 0)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
		gl.deleteTextures(1, &to_id_2d_array);
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/* Initializes test instance: creates test-specific texture objects. */
void GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture objects */
	gl.genTextures(1, &to_id_2d);

	if (are_2d_array_multisample_tos_supported)
	{
		gl.genTextures(1, &to_id_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture object(s)");

	/* Bind the texture objects to extension-specific texture targets */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d);

	if (are_2d_array_multisample_tos_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture object(s) to corresponding texture target(s).");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest::
	iterate()
{
	are_2d_array_multisample_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	initInternals();

	/* Initialize texture storage for 2D multisample texture object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2 /* samples */, GL_RGBA8, 4, /* width */
							   4,														/* height */
							   GL_TRUE /* fixedsamplelocations */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up an immutable 2D multisample texture object");

	/* Initialize texture storage for 2D multisample array texture object */
	if (are_2d_array_multisample_tos_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2 /* samples */, GL_RGBA8, 4 /* width */,
								   4 /* height */, 4 /* depth */, GL_TRUE /* fixedsamplelocations */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up an immutable 2D multisample array texture object");
	}

	/* Force a zero base level setting for 2D multisample texture using all available glTexParameter*() entry-points */
	glw::GLfloat zero_float = 0.0f;
	glw::GLint   zero_int   = 0;

	gl.texParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0.0f);
	gl.texParameterfv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, &zero_float);
	gl.texParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0);
	gl.texParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, &zero_int);

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glTexParameter*() call reported an error.");

	/* Do the same for 2D multisample array texture */
	if (are_2d_array_multisample_tos_supported)
	{
		gl.texParameterf(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_BASE_LEVEL, 0.0f);
		gl.texParameterfv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_BASE_LEVEL, &zero_float);
		gl.texParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_BASE_LEVEL, 0);
		gl.texParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_TEXTURE_BASE_LEVEL, &zero_int);

		GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glTexParameter*() call reported an error.");
	}

	/* All good! */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}
} /* glcts namespace */
