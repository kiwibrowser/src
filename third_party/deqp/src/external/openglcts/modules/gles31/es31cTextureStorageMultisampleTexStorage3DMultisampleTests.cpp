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
 * \file  es31cTextureStorageMultisampleTexStorage3DMultisampleTests.cpp
 * \brief Implements conformance tests for glTexStorage3DMultisampleOES()
 *        entry-points (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTexStorage3DMultisampleTests.hpp"
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
/* Array holding color renderable internalformats used by the following tests:
 * - "valid internalformat and samples values are accepted" test,
 * - "requests to set up multisample color textures with unsupported number of samples are rejected" test.
 */
const glw::GLint color_renderable_internalformats[] = { GL_R8, GL_RGB565, GL_RGB10_A2UI, GL_SRGB8_ALPHA8, GL_R8I };

/* Array holding depth renderable internalformats used by the following tests:
 * - valid internalformat and samples values are accepted" test,
 * - requests to set up multisample depth textures with unsupported number of samples are rejected" test.
 */
const glw::GLint depth_renderable_internalformats[] = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32F,
														GL_DEPTH24_STENCIL8 };

/* Array holding depth-stencil renderable internalformats used by the following tests:
 * - valid internalformat and samples values are accepted" test,
 * - requests to set up multisample stencil textures with unsupported number of samples are rejected" test.
 */
const glw::GLint depth_stencil_renderable_internalformats[] = { GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8 };

/* Array holding boolean values indicating possible fixed sample locations values. */
const glw::GLboolean fixed_sample_locations_values[] = { GL_TRUE, GL_FALSE };

/* Array holding supported internalformat values used by the following tests:
 * - requests to set up multisample textures with valid and invalid number of samples" test.
 */
const glw::GLint supported_internalformats[] = { GL_R8,
												 GL_RGB565,
												 GL_RGB10_A2UI,
												 GL_SRGB8_ALPHA8,
												 GL_R8I,
												 GL_DEPTH_COMPONENT16,
												 GL_DEPTH_COMPONENT32F,
												 GL_DEPTH24_STENCIL8,
												 GL_DEPTH24_STENCIL8 };

/* Array holding internalformats which are neither color-, stencil- nor depth-renderable,
 * used by the following tests:
 * - non color-, depth-, stencil-, renderable internalformats are rejected test.
 */
/* GL_SRGB8_ALPHA8 is renderable according to spec - replaced with GL_SRGB8 */
/* GL_RGBA32F is renderable if EXT_color_buffer_float extension is supported - replaced with GL_RGB32F */
/* GL_SRGB8 is renderable if extension NV_sRGB_formats is supported. */
/* GL_R8_SNORM is renderable if extension EXT_render_snorm is supported - replace with GL_RGB8_SNORM*/
const glw::GLint unsupported_internalformats[] = { GL_RGB8_SNORM, GL_RGB32F, GL_RGB32I };

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
InvalidTextureSizesAreRejectedValidAreAcceptedTest::InvalidTextureSizesAreRejectedValidAreAcceptedTest(Context& context)
	: TestCase(context, "invalid_texture_sizes_are_rejected_valid_are_accepted_test",
			   "Verifies gltexStorage3DMultisample() rejects invalid multisample "
			   "texture sizes by generating GL_INVALID_VALUE error; border cases are correctly accepted.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, max_texture_size(0)
	, max_array_texture_layers(0)
	, to_id_2d_array_1(0)
	, to_id_2d_array_2(0)
	, to_id_2d_array_3(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void InvalidTextureSizesAreRejectedValidAreAcceptedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	if (to_id_2d_array_1 != 0)
	{
		/* Delete texture object. */
		gl.deleteTextures(1, &to_id_2d_array_1);

		to_id_2d_array_1 = 0;
	}

	if (to_id_2d_array_2 != 0)
	{
		/* Delete texture object. */
		gl.deleteTextures(1, &to_id_2d_array_2);

		to_id_2d_array_2 = 0;
	}

	if (to_id_2d_array_3 != 0)
	{
		/* Delete texture object. */
		gl.deleteTextures(1, &to_id_2d_array_3);

		to_id_2d_array_3 = 0;
	}

	max_texture_size		 = 0;
	max_array_texture_layers = 0;

	/* Make sure no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture objects deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void InvalidTextureSizesAreRejectedValidAreAcceptedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate first texture object. */
	gl.genTextures(1, &to_id_2d_array_1);

	/* Generate second texture object. */
	gl.genTextures(1, &to_id_2d_array_2);

	/* Generate third texture object. */
	gl.genTextures(1, &to_id_2d_array_3);

	/* Retrieve maximum 3D texture image dimensions. */
	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_texture_layers);

	/* Make sure no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture objects creation failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult InvalidTextureSizesAreRejectedValidAreAcceptedTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture objects were generated properly. */
	TCU_CHECK_MSG(to_id_2d_array_1 != 0, "First texture object has not been generated.");
	TCU_CHECK_MSG(to_id_2d_array_2 != 0, "Second texture object has not been generated.");
	TCU_CHECK_MSG(to_id_2d_array_3 != 0, "Third texture object has not been generated.");

	/* Make sure valid maximum 3d image dimensions were returned. */
	TCU_CHECK_MSG(max_texture_size >= 2048, "Invalid GL_MAX_TEXTURE_SIZE was returned.");
	TCU_CHECK_MSG(max_array_texture_layers >= 256, "Invalid GL_MAX_ARRAY_TEXTURE_LAYERS was returned.");

	/* Bind texture object to_id_2d_array_1 to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_array_1);

	/* Make sure no error was generated. */
	glw::GLenum error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "Unexpected error was generated when binding texture object to "
									"GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target.");

	/* Call gltexStorage3DMultisample() with invalid depth argument value (depth value cannot be negative). */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8, 4, 4,
							   0, /* Invalid depth argument value. */
							   GL_TRUE);

	/* Expect GL_INVALID_VALUE error code. */
	error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_VALUE,
				  "gltexStorage3DMultisample() did not generate GL_INVALID_VALUE error.");

	/* Call gltexStorage3DMultisample() with invalid depth argument value
	 * (depth value cannot be greater than GL_MAX_TEXTURE_SIZE). */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8, 4, 4,
							   max_array_texture_layers + 1, /* Invalid depth argument value. */
							   GL_TRUE);

	/* Expect GL_INVALID_VALUE error code. */
	error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_VALUE,
				  "gltexStorage3DMultisample() did not generate GL_INVALID_VALUE error.");

	/* Set up a valid immutable 2D array multisample texture object using gltexStorage3DMultisample() call. */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8, 4, 4, 1, GL_TRUE);

	/* Make sure no error was generated. */
	error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "gltexStorage3DMultisample() reported unexpected error code.");

	/* Bind texture object to_id_2d_array_2 to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_array_2);

	/* Make sure no error was generated. */
	error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "Unexpected error was generated when binding texture object to "
									"GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target.");

	/* Set up a valid immutable 2D array multisample texture object using gltexStorage3DMultisample() call. */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8, 4, 4, max_array_texture_layers,
							   GL_TRUE);

	/* Make sure no error was generated. */
	error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "gltexStorage3DMultisample() reported unexpected error code.");

	/* Bind texture object to_id_2d_array_3 to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_array_3);

	/* Make sure no error was generated. */
	error_code = gl.getError();

	GLU_EXPECT_NO_ERROR(error_code, "Unexpected error was generated when binding texture object to "
									"GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target.");

	/* Call gltexStorage3DMultisample() with invalid width argument value
	 * (width value cannot be greater than GL_MAX_3D_TEXTURE_SIZE). */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8,
							   max_texture_size + 1, /* Invalid width argument value. */
							   max_texture_size, 2, GL_TRUE);

	/* Expect GL_INVALID_VALUE error code. */
	error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_VALUE,
				  "gltexStorage3DMultisample() did not generate GL_INVALID_VALUE error.");

	/* Call gltexStorage3DMultisample() with invalid height argument value
	 * (height value cannot be greater than GL_MAX_3D_TEXTURE_SIZE). */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, GL_RGBA8, max_texture_size,
							   max_texture_size + 1, /* Invalid height argument value. */
							   2, GL_TRUE);

	/* Expect GL_INVALID_VALUE error code. */
	error_code = gl.getError();

	TCU_CHECK_MSG(error_code == GL_INVALID_VALUE,
				  "gltexStorage3DMultisample() did not generate GL_INVALID_VALUE error.");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage3DZeroSampleTest::MultisampleTextureTexStorage3DZeroSampleTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_3d_zero_sample",
			   "Verifies TexStorage3DMultisample() rejects zero sample requests "
			   "by generating a GL_INVALID_VALUE error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage3DZeroSampleTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES */
	if (gl_oes_texture_multisample_2d_array_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");

	/* Delete texture object */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

	to_id = 0;

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Initializes ES objects required for test execution */
void MultisampleTextureTexStorage3DZeroSampleTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id */
	gl.genTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed to generate texture");

	/* Verify texture object has been generated properly */
	if (to_id == 0)
	{
		TCU_FAIL("Texture object has not been generated properly");
	}

	/* Bind texture to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage3DZeroSampleTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Issue call function for target GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES_, but provide zero for samples argument */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0, GL_RGBA8, 1, 1, 1, true);

	/* Check if the expected error code was reported */
	glw::GLenum error_code = gl.getError();

	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
NonColorDepthStencilRenderableInternalformatsAreRejectedTest::
	NonColorDepthStencilRenderableInternalformatsAreRejectedTest(Context& context)
	: TestCase(context, "non_color_depth_stencil_renderable_internalformats_are_rejected_test",
			   "Verifies gltexStorage3DMultisample() rejects internalformats which are not"
			   " color-, depth-, nor stencil- renderable by generating GL_INVALID_ENUM error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void NonColorDepthStencilRenderableInternalformatsAreRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}
	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void NonColorDepthStencilRenderableInternalformatsAreRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult NonColorDepthStencilRenderableInternalformatsAreRejectedTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	const int number_of_internalformats_to_check =
		sizeof(unsupported_internalformats) / sizeof(unsupported_internalformats[0]);

	/* Go through all requested internalformats. */
	for (int internalformat_index = 0; internalformat_index < number_of_internalformats_to_check;
		 internalformat_index++)
	{
		gl.texStorage3DMultisample(
			GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 1,
			unsupported_internalformats[internalformat_index], /* One of unsupported internalformats. */
			1, 1, 1, true);

		/* Expect GL_INVALID_ENUM error code. */
		TCU_CHECK_MSG(gl.getError() == GL_INVALID_ENUM,
					  "gltexStorage3DMultisample() did not generate GL_INVALID_ENUM error.");
	} /* for each unsupported internalformat */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context)
	: TestCase(context,
			   "requests_to_set_up_multisample_color_textures_with_unsupported_number_of_samples_are_rejected_test",
			   "Verifies gltexStorage3DMultisample() rejects unsupported samples value by generating "
			   "GL_INVALID_VALUE or GL_INVALID_OPEARATION error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	glw::GLint internalformat_specific_max_samples = 0;
	glw::GLint max_color_texture_samples		   = 0;
	glw::GLint max_samples						   = 0;
	int		   number_of_color_renderable_internalformats_to_check =
		sizeof(color_renderable_internalformats) / sizeof(color_renderable_internalformats[0]);
	int number_of_fixed_sample_locations_values_to_check =
		sizeof(fixed_sample_locations_values) / sizeof(fixed_sample_locations_values[0]);

	/* Retrieve maximum color texture samples value. */
	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_texture_samples);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying GL_MAX_COLOR_TEXTURE_SAMPLES value failed.");

	/* Retrieve maximum samples value for an implementation. */
	gl.getIntegerv(GL_MAX_SAMPLES, &max_samples);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying GL_MAX_SAMPLES value failed.");

	/* Go through all supported color renderable internal formats. */
	for (int color_renderable_internalformat_index = 0;
		 color_renderable_internalformat_index < number_of_color_renderable_internalformats_to_check;
		 color_renderable_internalformat_index++)
	{
		/* Retrieve maximum amount of samples available for the combination of texture target and internalformat considered */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES,
							   color_renderable_internalformats[color_renderable_internalformat_index], GL_SAMPLES, 1,
							   &internalformat_specific_max_samples);

		/* Expect no error was generated. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Querying texture target-spcecific maximum sample value failed.");

		/* Go through all possible sample locations values. */
		for (int fixed_sample_locations_values_index = 0;
			 fixed_sample_locations_values_index < number_of_fixed_sample_locations_values_to_check;
			 fixed_sample_locations_values_index++)
		{
			glw::GLsizei samples = de::max(internalformat_specific_max_samples, max_color_texture_samples) + 1;

			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples,
									   color_renderable_internalformats[color_renderable_internalformat_index], 1, 1, 1,
									   fixed_sample_locations_values[fixed_sample_locations_values_index]);

			/* Expect GL_INVALID_OPERATION to be returned. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "gltexStorage3DMultisample() did not generate GL_INVALID_OPERATION error.");

			samples = internalformat_specific_max_samples + 1;
			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformat_specific_max_samples + 1,
									   color_renderable_internalformats[color_renderable_internalformat_index], 1, 1, 1,
									   fixed_sample_locations_values[fixed_sample_locations_values_index]);

			/* Expect GL_INVALID_OPERATION to be returned. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "gltexStorage3DMultisample() did not generate GL_INVALID_OPERATION error.");

		} /* for each fixed sample locations value */
	}	 /* for each color renderable internalformat */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context)
	: TestCase(context,
			   "requests_to_set_up_multisample_depth_textures_with_unsupported_number_of_samples_are_rejected_test",
			   "Verifies gltexStorage3DMultisample() rejects unsupported samples "
			   "value by generating GL_INVALID_VALUE or GL_INVALID_OPEARATION error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	glw::GLint internalformat_specific_max_samples = 0;
	glw::GLint max_depth_texture_samples		   = 0;
	int		   number_of_depth_renderable_internalformats_to_check =
		sizeof(depth_renderable_internalformats) / sizeof(depth_renderable_internalformats[0]);
	int number_of_fixed_sample_locations_values_to_check =
		sizeof(fixed_sample_locations_values) / sizeof(fixed_sample_locations_values[0]);

	/* Retrieve maximum depth texture samples value. */
	gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_texture_samples);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying maximum sample value failed.");

	/* Go through all supported depth renderable internal formats. */
	for (int depth_renderable_internalformat_index = 0;
		 depth_renderable_internalformat_index < number_of_depth_renderable_internalformats_to_check;
		 depth_renderable_internalformat_index++)
	{
		/* Retrieve maximum amount of samples available for the texture target considered */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES,
							   depth_renderable_internalformats[depth_renderable_internalformat_index], GL_SAMPLES, 1,
							   &internalformat_specific_max_samples);

		/* Expect no error was generated. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Querying texture target-spcecific maximum sample value failed.");

		/* Go through all possible sample locations values. */
		for (int fixed_sample_locations_values_index = 0;
			 fixed_sample_locations_values_index < number_of_fixed_sample_locations_values_to_check;
			 fixed_sample_locations_values_index++)
		{
			glw::GLsizei samples = de::max(internalformat_specific_max_samples, max_depth_texture_samples) + 1;

			gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples,
									   depth_renderable_internalformats[depth_renderable_internalformat_index], 1, 1, 1,
									   fixed_sample_locations_values[fixed_sample_locations_values_index]);

			/* Expect GL_INVALID_OPERATION error code. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "gltexStorage3DMultisample() did not generate GL_INVALID_OPERATION error.");

		} /* for each fixed sample locations value */
	}	 /* for each depth renderable internalformat */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context)
	: TestCase(context,
			   "requests_to_set_up_multisample_stencil_textures_with_unsupported_number_of_samples_are_rejected_test",
			   "Verifies gltexStorage3DMultisample() rejects unsupported samples value"
			   " by generating GL_INVALID_VALUE or GL_INVALID_OPERATION error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 * @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest::
	iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	glw::GLint internalformat_specific_max_samples = 0;
	glw::GLint max_depth_texture_samples		   = 0;
	int		   number_of_depth_stencil_renderable_internalformats_to_check =
		sizeof(depth_stencil_renderable_internalformats) / sizeof(depth_stencil_renderable_internalformats[0]);
	int number_of_fixed_sample_locations_values_to_check =
		sizeof(fixed_sample_locations_values) / sizeof(fixed_sample_locations_values[0]);

	/* Retrieve maximum depth texture samples value. */
	gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_texture_samples);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying maximum sample value failed.");

	/* Go through all supported depth-stencil renderable internal formats. */
	for (int depth_stencil_renderable_internalformat_index = 0;
		 depth_stencil_renderable_internalformat_index < number_of_depth_stencil_renderable_internalformats_to_check;
		 depth_stencil_renderable_internalformat_index++)
	{
		/* Retrieve maximum amount of samples available for the texture target considered */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES,
							   depth_stencil_renderable_internalformats[depth_stencil_renderable_internalformat_index],
							   GL_SAMPLES, 1, &internalformat_specific_max_samples);

		/* Expect no error was generated. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Querying texture target-spcecific maximum sample value failed.");

		/* Go through all possible sample locations values. */
		for (int fixed_sample_locations_values_index = 0;
			 fixed_sample_locations_values_index < number_of_fixed_sample_locations_values_to_check;
			 fixed_sample_locations_values_index++)
		{
			glw::GLsizei samples = de::max(internalformat_specific_max_samples, max_depth_texture_samples) + 1;

			gl.texStorage3DMultisample(
				GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples,
				depth_stencil_renderable_internalformats[depth_stencil_renderable_internalformat_index], 1, 1, 1,
				fixed_sample_locations_values[fixed_sample_locations_values_index]);

			/* Expect GL_INVALID_OPERATION to be returned. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "gltexStorage3DMultisample() did not generate GL_INVALID_OPERATION error.");

		} /* for each fixed sample locations value */
	}	 /* for each depth-stencil renderable internalformat */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::
	RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest(Context& context)
	: TestCase(context, "requests_to_set_up_multisample_textures_with_valid_and_invalid_number_of_samples_test",
			   "Verifies gltexStorage3DMultisample() rejects invalid samples value "
			   "by generating GL_INVALID_OPEARATION error and works properly when samples value is valid.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/* Generates texture object and binds it to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
void RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::createAssets()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object. */
	gl.genTextures(1, &to_id);

	/* Bind texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Deinitializes ES objects created during test execution */
void RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::deinit()
{
	if (to_id != 0)
	{
		/* Destroy created assets. */
		releaseAssets();
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/* Unbinds and deletes texture object. */
void RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::releaseAssets()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl								  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_samples_value				  = 0;
	glw::GLint			  internalformat_specific_max_samples = 0;
	int number_of_internalformats_to_check = sizeof(supported_internalformats) / sizeof(supported_internalformats[0]);

	/* Retrieve maximum samples value for an implementation. */
	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Querying GL_MAX_SAMPLES value failed.");

	/* Go through all supported internal formats. */
	for (int internalformat_index = 0; internalformat_index < number_of_internalformats_to_check;
		 internalformat_index++)
	{
		/* Generate and bind texture object. */
		RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::createAssets();

		/* Check if texture object was generated properly. */
		TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

		/* Retrieve maximum amount of samples available for the texture target considered */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, supported_internalformats[internalformat_index],
							   GL_SAMPLES, 1, &internalformat_specific_max_samples);

		/* Expect no error was generated. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "Querying texture target-spcecific maximum sample value failed.");

		/* Call gltexStorage3DMultisample() with valid samples value. */
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformat_specific_max_samples,
								   supported_internalformats[internalformat_index], 1, 1, 1, GL_FALSE);

		/* Expect no error was generated. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "gltexStorage3DMultisample() returned unexpected error code.");

		/* Delete texture object. */
		RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::releaseAssets();

		/* Generate and bind texture object. */
		RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::createAssets();

		/* Check if texture object was generated properly. */
		TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

		/* Call gltexStorage3DMultisample() with invalid samples value. */
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformat_specific_max_samples + 1,
								   supported_internalformats[internalformat_index], 1, 1, 1, GL_FALSE);

		/* Expect GL_INVALID_OPERATION error code. */
		TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
					  "gltexStorage3DMultisample() did not generate GL_INVALID_OPERATION error.");

		/* Delete texture object. */
		RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest::releaseAssets();
	} /* for each supported internalformat */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
Texture2DMultisampleTargetIsRejectedTest::Texture2DMultisampleTargetIsRejectedTest(Context& context)
	: TestCase(context, "texture_2D_multisample_target_is_rejected_test",
			   "Verifies gltexStorage3DMultisample() rejects GL_TEXTURE_2D_MULTISAMPLE "
			   "texture target by generating GL_INVALID_ENUM error.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void Texture2DMultisampleTargetIsRejectedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void Texture2DMultisampleTargetIsRejectedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult Texture2DMultisampleTargetIsRejectedTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	/* Call gltexStorage3DMultisample() with invalid GL_TEXTURE_2D_MULTISAMPLE texture target argument. */
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE, /* invalid texture target */ 1, GL_RGBA8, 1, 1, 1, GL_FALSE);

	/* Expect GL_INVALID_ENUM error code. */
	TCU_CHECK_MSG(gl.getError() == GL_INVALID_ENUM,
				  "gltexStorage3DMultisample() did not generate GL_INVALID_ENUM error.");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
ValidInternalformatAndSamplesValuesAreAcceptedTest::ValidInternalformatAndSamplesValuesAreAcceptedTest(Context& context)
	: TestCase(context, "valid_internalformats_are_accepted_test",
			   "Verifies gltexStorage3DMultisample() accepts multisample color/depth/stencil "
			   "textures with disabled/enabled fixed sample locations and valid internalformats.")
	, gl_oes_texture_multisample_2d_array_supported(GL_FALSE)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void ValidInternalformatAndSamplesValuesAreAcceptedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (gl_oes_texture_multisample_2d_array_supported)
	{
		/* Bind default texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 0);
	}

	/* Delete texture object. */
	gl.deleteTextures(1, &to_id);

	to_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object deletion failed.");

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void ValidInternalformatAndSamplesValuesAreAcceptedTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id. */
	gl.genTextures(1, &to_id);

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture object initialization failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult ValidInternalformatAndSamplesValuesAreAcceptedTest::iterate()
{
	gl_oes_texture_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	if (!gl_oes_texture_multisample_2d_array_supported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Check if texture object was generated properly. */
	TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

	glw::GLint max_color_texture_samples = 0;
	glw::GLint max_depth_texture_samples = 0;
	const int  n_color_internalformats =
		sizeof(color_renderable_internalformats) / sizeof(color_renderable_internalformats[0]);
	const int n_depth_internalformats =
		sizeof(depth_renderable_internalformats) / sizeof(depth_renderable_internalformats[0]);
	const int n_fixed_sample_locations =
		sizeof(fixed_sample_locations_values) / sizeof(fixed_sample_locations_values[0]);
	const int n_stencil_internalformats =
		sizeof(depth_stencil_renderable_internalformats) / sizeof(depth_stencil_renderable_internalformats[0]);

	/* Retrieve maximum color texture samples value. */
	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_texture_samples);
	/* Retrieve maximum depth texture samples value. */
	gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_texture_samples);

	/* Expect no error was generated. */
	GLU_EXPECT_NO_ERROR(
		gl.getError(),
		"Querying maximum GL_MAX_COLOR_TEXTURE_SAMPLES and GL_MAX_DEPTH_TEXTURE_SAMPLES property values failed.");

	for (unsigned int n_iteration = 0; n_iteration < 3 /* color/depth/stencil */; ++n_iteration)
	{
		const glw::GLint* internalformats						  = NULL;
		glw::GLint		  internalformat_specific_max_samples	 = 0;
		glw::GLint		  max_iteration_specific_gl_samples_value = 0;
		glw::GLint		  max_supported_samples_value			  = 0;
		int				  n_internalformats						  = 0;

		switch (n_iteration)
		{
		case 0:
		{
			internalformats							= color_renderable_internalformats;
			max_iteration_specific_gl_samples_value = max_color_texture_samples;
			n_internalformats						= n_color_internalformats;

			break;
		}

		case 1:
		{
			internalformats							= depth_renderable_internalformats;
			max_iteration_specific_gl_samples_value = max_depth_texture_samples;
			n_internalformats						= n_depth_internalformats;

			break;
		}

		case 2:
		{
			internalformats							= depth_stencil_renderable_internalformats;
			max_iteration_specific_gl_samples_value = max_depth_texture_samples;
			n_internalformats						= n_stencil_internalformats;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized iteration index");
		}
		} /* switch (n_iteration) */

		/* Go through all requested internalformats. */
		for (int internalformat_index = 0; internalformat_index < n_internalformats; internalformat_index++)
		{
			/* Retrieve maximum amount of samples available for the combination of
			 * texture target and internalformat considered. */
			gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformats[internalformat_index],
								   GL_SAMPLES, 1, &internalformat_specific_max_samples);

			/* Expect no error was generated. */
			GLU_EXPECT_NO_ERROR(gl.getError(), "Querying texture target-spcecific maximum samples value failed.");

			/* Choose maximum supported samples value. */
			max_supported_samples_value =
				de::min(internalformat_specific_max_samples, max_iteration_specific_gl_samples_value);

			/* Go through all supported samples values. */
			for (glw::GLint n_samples = 1; n_samples <= max_supported_samples_value; n_samples++)
			{
				/* Go through all supported 'fixed_sample_locations' argument values. */
				for (int fixed_sample_location_value_index = 0;
					 fixed_sample_location_value_index < n_fixed_sample_locations; fixed_sample_location_value_index++)
				{
					/* Call gltexStorage3DMultisample() with valid arguments. */
					gl.texStorage3DMultisample(
						GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, n_samples, /* Iteration-specific sample value. */
						internalformats
							[internalformat_index], /* One of color/depth/stencil-renderable internalformats. */
						1,							/* width */
						1,							/* height */
						1,							/* depth */
						fixed_sample_locations_values[fixed_sample_location_value_index]);

					/* Expect no error was generated. */
					GLU_EXPECT_NO_ERROR(gl.getError(), "gltexStorage3DMultisample() generated unexpected error.");

					/* Delete texture object. */
					gl.deleteTextures(1, &to_id);

					/* Generate texture object. */
					gl.genTextures(1, &to_id);

					/* Check if texture object was generated properly. */
					TCU_CHECK_MSG(to_id != 0, "Texture object has not been generated.");

					/* Re-bind texture object. */
					gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

					/* Expect no error was generated. */
					GLU_EXPECT_NO_ERROR(gl.getError(), "Rebinding texture object generated unexpected error.");
				} /* for each fixed sample locations value (enabled/disabled). */
			}	 /* for each supported sample value. */
		}		  /* for each color/depth/stencil-renderable internalformat */
	}			  /* for color/depth/stencil interation */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
