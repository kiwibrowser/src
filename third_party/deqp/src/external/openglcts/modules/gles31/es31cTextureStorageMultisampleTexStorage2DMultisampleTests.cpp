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
 * \file  es31cTextureStorageMultisampleTexStorage2DMultisampleTests.cpp
 * \brief Implements conformance tests for glTexStorage2DMultisample()
 *        entry-points (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTexStorage2DMultisampleTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace glcts
{

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DGeneralSamplesNumberTest::MultisampleTextureTexStorage2DGeneralSamplesNumberTest(
	Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_general_samples_number",
			   "Verifies TexStorage2DMultisample() requests with exact number of samples"
			   " reported by glGetInternalformativ() succeed and larger values rejected")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DGeneralSamplesNumberTest::deinit()
{
	/* Delete texture in case the test case failed */
	deinitInternalIteration();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Deinitializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DGeneralSamplesNumberTest::deinitInternalIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		/* Delete texture object */
		gl.deleteTextures(1, &to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");
}

/** Initializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DGeneralSamplesNumberTest::initInternalIteration()
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

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DGeneralSamplesNumberTest::iterate()
{
	/* Test case variables */
	const glw::GLboolean  fixedsamplelocations  = GL_FALSE;
	const glw::Functions& gl					= m_context.getRenderContext().getFunctions();
	const glw::GLenum	 internalformat_list[] = { GL_R8,
												GL_RGB565,
												GL_RGB10_A2UI,
												GL_SRGB8_ALPHA8,
												GL_R8I,
												GL_DEPTH_COMPONENT16,
												GL_DEPTH_COMPONENT32F,
												GL_DEPTH24_STENCIL8,
												GL_DEPTH24_STENCIL8,
												GL_DEPTH32F_STENCIL8 };
	const int		   internalformat_list_count  = sizeof(internalformat_list) / sizeof(internalformat_list[0]);
	glw::GLint		   internalformat_max_samples = -1; /* Will be determined later */
	const glw::GLsizei height					  = 1;
	const glw::GLsizei width					  = 1;
	const glw::GLenum  target = GL_TEXTURE_2D_MULTISAMPLE; /* Test case uses GL_TEXTURE_2D_MULTISAMPLE target */
	glw::GLint		   gl_max_samples_value = -1;

	/* Get GL_MAX_SAMPLES value */
	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLES value");

	/* Iterate through all internal formats test case should check */
	for (int internalformat_index = 0; internalformat_index < internalformat_list_count; internalformat_index++)
	{
		/* Iteration-specific internalformat */
		glw::GLenum internalformat = internalformat_list[internalformat_index];

		/* Subiteration. Case samples = internalformat_max_samples */
		{
			/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
			initInternalIteration();

			/* Retrieve maximum amount of samples available for the target's texture internalformat */
			gl.getInternalformativ(target, internalformat, GL_SAMPLES, 1, &internalformat_max_samples);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed to retrieve GL_SAMPLES value");

			/* Issue call with valid parameters */
			gl.texStorage2DMultisample(target, internalformat_max_samples, internalformat, width, height,
									   fixedsamplelocations);

			GLU_EXPECT_NO_ERROR(
				gl.getError(),
				"glTexStorage2DMultisample() call, for which a valid number of samples was used, has failed.");

			/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
			deinitInternalIteration();
		}

		/* Subiteration. Case: samples > internalformat_max_samples */
		{
			/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
			initInternalIteration();

			/* Issue call with valid parameters, but invalid sample parameter */
			gl.texStorage2DMultisample(target, internalformat_max_samples + 1, internalformat, width, height,
									   fixedsamplelocations);

			/* Check if the expected error code was reported */
			/* From spec:
			 * An INVALID_OPERATION error is generated if samples is greater than the
			 * maximum number of samples supported for this target and internalformat.*/

			/* Expect GL_INVALID_OPERATION error code. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "glTexStorage2DMultisample() did not generate GL_INVALID_OPERATION error.");

			/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
			deinitInternalIteration();
		}
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest::
	MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_invalid_and_border_case_texture_sizes",
			   "Invalid multisample texture sizes are rejected; border cases are correctly accepted.")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes GL ES objects used by the test */
void MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();

	/* Release test texture object */
	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}
}

/** Initializes GL ES objects used by the test */
void MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate texture object id */
	gl.genTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Retrieve GL_MAX_TEXTURE_SIZE pname value */
	glw::GLint gl_max_texture_size_value = 0;

	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_TEXTURE_SIZE pname value");

	/* Try to set up a valid 2D multisample texture object of (max texture size, 1) resolution. */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2 /* samples */, GL_RGBA8, gl_max_texture_size_value,
							   1 /* height */, GL_TRUE /* fixedsamplelocations */);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Valid glTexStorage2DMultisample() call ((max texture size, 1) resolution) failed");

	/* Delete the texture object before we continue */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() failed");

	/* Create a new texture object and bind it to 2D multisample texture target. */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create 2D multisample texture object");

	/* Try to set up another valid 2D multisample texture object of (1, max texture size) resolution. */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2 /* samples */, GL_RGBA8, 1 /* width */,
							   gl_max_texture_size_value, GL_TRUE /* fixedsamplelocations */);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Valid glTexStorage2DMultisample() call ((1, max texture size) resolution) failed");

	/* Delete the texture object before we continue */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() failed");

	/* Create a new texture object and bind it to 2D multisample texture target. */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create 2D multisample texture object");

	/* Try to set up invalid 2D multisample texture objects. */
	glw::GLenum error_code = GL_NO_ERROR;

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2,			/* samples */
							   GL_RGBA8, gl_max_texture_size_value + 1, /* width */
							   1,										/* height */
							   GL_TRUE);								/* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid glTexStorage2DMultisample() call ((max texture size+1, 1) resolution) did not generate "
				 "GL_INVALID_VALUE error");
	}

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2,  /* samples */
							   GL_RGBA8, 1,					  /* width */
							   gl_max_texture_size_value + 1, /* height */
							   GL_TRUE);					  /* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid glTexStorage2DMultisample() call ((1, max texture size+1) resolution) did not generate "
				 "GL_INVALID_VALUE error");
	}

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2,			/* samples */
							   GL_RGBA8, gl_max_texture_size_value + 1, /* width */
							   gl_max_texture_size_value + 1,			/* height */
							   GL_TRUE);								/* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid glTexStorage2DMultisample() call ((max texture size+1, max texture size+1) resolution) did "
				 "not generate GL_INVALID_VALUE error");
	}

	/* Try to set up a null resolution 2D multisample TO. */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 0,					 /* width */
							   0,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL(
			"Invalid glTexStorage2DMultisample() call with a 0x0 resolution did not generate GL_INVALID_VALUE error");
	}

	/* Delete the texture object before we continue */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() failed");

	/* Create a new texture object and bind it to 2D multisample texture target. */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create 2D multisample texture object");

	/* Try to set up an invalid texture object with at least one dimension size defined as a negative value */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2 /* samples */, GL_RGBA8 /* sizedinternalformat */,
							   -1, /* width */
							   0,  /* height */
							   GL_TRUE /* fixedsamplelocations */);

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL(
			"Invalid glTexStorage2DMultisample() call ((-1, 0) resolution) did not generate GL_INVALID_VALUE error");
	}

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 0,					 /* width */
							   -1,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL(
			"Invalid glTexStorage2DMultisample() call ((0, -1) resolution) did not generate GL_INVALID_VALUE error");
	}

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, -1,				 /* width */
							   -1,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	error_code = gl.getError();
	if (error_code != GL_INVALID_VALUE)
	{
		TCU_FAIL(
			"Invalid glTexStorage2DMultisample() call ((-1, -1) resolution) did not generate GL_INVALID_VALUE error");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest::
	MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_non_color_depth_or_stencil_internal_formats_rejected",
			   "Verifies TexStorage2DMultisample() rejects internal formats that "
			   "are not color-renderable, depth-renderable and stencil-renderable")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest::deinit()
{
	/* Delete texture in case the test case failed */
	deinitInternalIteration();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Deinitializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest::deinitInternalIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		/* Delete texture object */
		gl.deleteTextures(1, &to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");
}

/** Initializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest::initInternalIteration()
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

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest::iterate()
{
	/* Test case variables */
	const glw::GLboolean  fixedsamplelocations = GL_TRUE;
	const glw::Functions& gl				   = m_context.getRenderContext().getFunctions();
	const glw::GLsizei	height			   = 1;
	/* GL_SRGB8_ALPHA8 is renderable according to spec - replaced with GL_SRGB8 */
	/* GL_RGBA32F is renderable if EXT_color_buffer_float extension is supported - replaced with GL_RGB32F */
	/* GL_SRGB8 is renderable if extension NV_sRGB_formats is supported. */
	/* GL_R8_SNORM is renderable if extension EXT_render_snorm is supported - replace with GL_RGB8_SNORM*/
	const glw::GLenum  internalformats_list[]	 = { GL_RGB8_SNORM, GL_RGB32F, GL_RGB32I };
	const int		   internalformats_list_count = sizeof(internalformats_list) / sizeof(internalformats_list[0]);
	const glw::GLsizei samples					  = 1;
	const glw::GLenum  target					  = GL_TEXTURE_2D_MULTISAMPLE;
	const glw::GLsizei width					  = 1;

	/* Iterate through all internal formats test case should check */
	for (int i = 0; i < internalformats_list_count; i++)
	{
		/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
		initInternalIteration();

		/* Issue call with valid parameters, but invalid internalformats */
		gl.texStorage2DMultisample(target, samples, internalformats_list[i], width, height, fixedsamplelocations);

		/* Check if the expected error code was reported */
		if (gl.getError() != GL_INVALID_ENUM)
		{
			TCU_FAIL("Invalid error code reported");
		}

		/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
		deinitInternalIteration();
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DReconfigurationRejectedTest::MultisampleTextureTexStorage2DReconfigurationRejectedTest(
	Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_reconfiguration_rejected",
			   "Verifies TexStorage2DMultisample() reconfiguration fails")
	, gl_oes_texture_storage_multisample_2d_array_supported(GL_FALSE)
	, to_id_2d(0)
	, to_id_2d_array(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DReconfigurationRejectedTest::deinit()
{
	/* Delete texture and bind default texture to GL_TEXTURE_2D_MULTISAMPLE */
	deinitTexture(to_id_2d, GL_TEXTURE_2D_MULTISAMPLE);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		/* Delete texture and bind default texture to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES */
		deinitTexture(to_id_2d_array, GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES);
	}
	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Deinitializes texture: delete texture and release texture object bound to specified target.
 *
 * @param to_id          Texture object to delete & unbind. Will be set to 0 afterward
 * @param texture_target Target from which the texture will be unbound
 */
void MultisampleTextureTexStorage2DReconfigurationRejectedTest::deinitTexture(glw::GLuint& to_id,
																			  glw::GLenum  texture_target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind texture object bound to texture_target target */
	gl.bindTexture(texture_target, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind a texture object");

	/* Delete texture object */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

	to_id = 0;
}

/** Initializes ES objects required for test execution */
void MultisampleTextureTexStorage2DReconfigurationRejectedTest::initInternals()
{
	/* Generate and bind texture to GL_TEXTURE_2D_MULTISAMPLE target */
	initTexture(to_id_2d, GL_TEXTURE_2D_MULTISAMPLE);

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		/* Generate and bind texture to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES target */
		initTexture(to_id_2d_array, GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES);
	}
}

/** Initializes texture: creates texture object and binds it to specified texture target.
 *
 * @param to_id          Will be set to new texture object's id
 * @param texture_target Texture target, to which the created texture should be bound to
 */
void MultisampleTextureTexStorage2DReconfigurationRejectedTest::initTexture(glw::GLuint& to_id,
																			glw::GLenum  texture_target)
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

	/* Bind generated texture object ID to texture_target target */
	gl.bindTexture(texture_target, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DReconfigurationRejectedTest::iterate()
{
	gl_oes_texture_storage_multisample_2d_array_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	initInternals();

	/* Test case variables */
	const glw::GLsizei   depth				  = 4;
	const glw::GLboolean fixedsamplelocations = GL_TRUE;
	const glw::GLsizei   height				  = 4;
	const glw::GLenum	internalformat		  = GL_RGBA8;
	const glw::GLsizei   samples			  = 2;
	const glw::GLsizei   width				  = 4;

	/* Set up immutable 2D multisample texture object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, width, height, fixedsamplelocations);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glTexStorage2DMultisample() failed to set up immutable 2D multisample texture object");

	/* Try to reset immutable 2D multisample texture object */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, width, height, fixedsamplelocations);

	if (gl.getError() != GL_INVALID_OPERATION)
	{
		TCU_FAIL("Invalid error code reported");
	}

	if (gl_oes_texture_storage_multisample_2d_array_supported)
	{
		/* Set up immutable 2D array multisample texture object */
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples, internalformat, width, height, depth,
								   fixedsamplelocations);

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"gltexStorage3DMultisample() failed to set up immutable 2D array multisample texture object");

		/* Try to reset immutable 2D array multisample texture object */
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples, internalformat, width, height, depth,
								   fixedsamplelocations);

		if (gl.getError() != GL_INVALID_OPERATION)
		{
			TCU_FAIL("Invalid error code reported");
		}
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest::
	MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_texture_2d_multsample_array",
			   "Verifies TexStorage2DMultisample() rejects GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES targets")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest::iterate()
{
	/* NOTE: This test can be executed, no matter whether GL_OES_texture_storage_multisample_2d_array
	 *       extension is supported on the running platform, or not.
	 */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Issue call with valid parameters and GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES target */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 1, GL_RGBA8, 1, 1, false);

	/* Check if the expected error code was reported */
	if (gl.getError() != GL_INVALID_ENUM)
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
MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest::
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_unsupported_samples_count_for_color_textures_rejected",
			   "Verifies TexStorage2DMultisample() rejects requests to set up "
			   "multisample color textures with unsupported number of samples")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest::deinit()
{
	/* Delete texture in case the test case failed */
	deinitInternalIteration();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Deinitializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest::deinitInternalIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		/* Delete texture object */
		gl.deleteTextures(1, &to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");
}

/** Initializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest::initInternalIteration()
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

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest::iterate()
{
	/* Test case variables */
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	const glw::GLsizei	height					  = 1;
	const glw::GLboolean  fixedsamplelocations_list[] = { GL_FALSE, GL_TRUE };
	const int			  fixedsamplelocations_list_count =
		sizeof(fixedsamplelocations_list) / sizeof(fixedsamplelocations_list[0]);
	glw::GLint		   gl_max_color_texture_samples_value  = -1; /* Will be determined later */
	glw::GLint		   gl_max_internalformat_samples_value = -1; /* Will be determined later */
	glw::GLint		   gl_max_samples_value				   = -1; /* Will be determined later */
	const glw::GLenum  internalformat_list[]	 = { GL_R8, GL_RGB565, GL_RGB10_A2UI, GL_SRGB8_ALPHA8, GL_R8I };
	const int		   internalformat_list_count = sizeof(internalformat_list) / sizeof(internalformat_list[0]);
	const glw::GLenum  target = GL_TEXTURE_2D_MULTISAMPLE; /* Test case uses GL_TEXTURE_2D_MULTISAMPLE target */
	const glw::GLsizei width  = 1;

	/* Iterate through all internal formats test case should check */
	for (int internalformat_index = 0; internalformat_index < internalformat_list_count; internalformat_index++)
	{
		/* Iteration-specific internalformat */
		glw::GLenum internalformat = internalformat_list[internalformat_index];

		/* Iterate through all fixedsamplelocations test case should check */
		for (int fixedsamplelocations_index = 0; fixedsamplelocations_index < fixedsamplelocations_list_count;
			 fixedsamplelocations_index++)
		{
			/* Iteration-specific fixedsamplelocations */
			glw::GLboolean fixedsamplelocations = fixedsamplelocations_list[fixedsamplelocations_index];

			/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
			initInternalIteration();

			/* Get GL_MAX_SAMPLES value */
			gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLES value");

			/* Get GL_MAX_COLOR_TEXTURE_SAMPLES value */
			gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &gl_max_color_texture_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_COLOR_TEXTURE_SAMPLES value");

			/* Retrieve maximum amount of samples available for the texture target considered */
			gl.getInternalformativ(target, internalformat, GL_SAMPLES, 1, &gl_max_internalformat_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed to retrieve GL_SAMPLES");

			/* Issue call with valid parameters, but samples argument might be invalid */
			gl.texStorage2DMultisample(target, gl_max_internalformat_samples_value + 1, internalformat, width, height,
									   fixedsamplelocations);

			/* Expect GL_INVALID_OPERATION error code. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "glTexStorage2DMultisample() did not generate GL_INVALID_OPERATION error.");

			/* Issue call with valid parameters, but to another target GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES and with invalid samples.
			 *
			 * NOTE: This can be executed on both the implementations that support GL_OES_texture_storage_multisample_2d_array extension
			 *       and on those that don't.
			 */
			gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, gl_max_internalformat_samples_value + 1,
									   internalformat, width, height, fixedsamplelocations);

			/* Expect GL_INVALID_ENUM error code from invalid target. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_ENUM,
						  "glTexStorage2DMultisample() did not generate GL_INVALID_ENUM error.");

			/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
			deinitInternalIteration();
		}
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest::
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_unsupported_samples_count_for_depth_textures_rejected",
			   "Verifies TexStorage2DMultisample() rejects requests to set up multisample "
			   "depth textures with unsupported number of samples")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest::deinit()
{
	/* Delete texture in case the test case failed */
	deinitInternalIteration();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Deinitializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest::deinitInternalIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		/* Delete texture object */
		gl.deleteTextures(1, &to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");
}

/** Initializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest::initInternalIteration()
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

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest::iterate()
{
	/* Test case variables */
	const glw::GLboolean fixedsamplelocations_list[] = { GL_FALSE, GL_TRUE };
	const int			 fixedsamplelocations_list_count =
		sizeof(fixedsamplelocations_list) / sizeof(fixedsamplelocations_list[0]);
	const glw::Functions& gl								  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_depth_texture_samples_value  = -1; /* Will be determined later */
	glw::GLint			  gl_max_internalformat_samples_value = -1; /* Will be determined later */
	glw::GLint			  gl_max_samples_value				  = -1; /* Will be determined later */
	const glw::GLsizei	height							  = 1;
	const glw::GLenum	 internalformat_list[] = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8 };
	const int			  internalformat_list_count = sizeof(internalformat_list) / sizeof(internalformat_list[0]);
	const glw::GLenum	 target = GL_TEXTURE_2D_MULTISAMPLE; /* Test case uses GL_TEXTURE_2D_MULTISAMPLE target */
	const glw::GLsizei	width  = 1;

	/* Iterate through all internal formats test case should check */
	for (int internalformat_index = 0; internalformat_index < internalformat_list_count; internalformat_index++)
	{
		/* Iteration-specific internalformat */
		glw::GLenum internalformat = internalformat_list[internalformat_index];

		/* Iterate through all fixedsamplelocations test case should check */
		for (int fixedsamplelocations_index = 0; fixedsamplelocations_index < fixedsamplelocations_list_count;
			 fixedsamplelocations_index++)
		{
			/* Iteration-specific fixedsamplelocations */
			glw::GLboolean fixedsamplelocations = fixedsamplelocations_list[fixedsamplelocations_index];

			/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
			initInternalIteration();

			/* Get GL_MAX_SAMPLES value */
			gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLES value");

			/* Get GL_MAX_DEPTH_TEXTURE_SAMPLES value */
			gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &gl_max_depth_texture_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_DEPTH_TEXTURE_SAMPLES value");

			/* Retrieve maximum amount of samples available for the texture target considered */
			gl.getInternalformativ(target, internalformat, GL_SAMPLES, 1, &gl_max_internalformat_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed to retrieve GL_SAMPLES");

			/* Issue call with valid parameters, but samples argument might be invalid */
			gl.texStorage2DMultisample(target, gl_max_internalformat_samples_value + 1, internalformat, width, height,
									   fixedsamplelocations);

			/* Expect GL_INVALID_OPERATION error code. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "glTexStorage2DMultisample() did not generate GL_INVALID_OPERATION error.");

			/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
			deinitInternalIteration();
		}
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest::
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest(Context& context)
	: TestCase(context,
			   "multisample_texture_tex_storage_2d_unsupported_samples_count_for_depth_stencil_textures_rejected",
			   "Verifies TexStorage2DMultisample() rejects requests to set up multisample "
			   "depth+stencil textures with unsupported number of samples")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest::deinit()
{
	/* Delete texture in case the test case failed */
	deinitInternalIteration();

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Deinitializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest::deinitInternalIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id != 0)
	{
		/* Delete texture object */
		gl.deleteTextures(1, &to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");
}

/** Initializes GL ES objects specific to internal iteration */
void MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest::initInternalIteration()
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

	/* Bind generated texture object ID to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest::iterate()
{
	/* Test case variables */
	const glw::GLboolean fixedsamplelocations_list[] = { GL_FALSE, GL_TRUE };
	const int			 fixedsamplelocations_list_count =
		sizeof(fixedsamplelocations_list) / sizeof(fixedsamplelocations_list[0]);
	const glw::Functions& gl								  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_depth_texture_samples_value  = -1; /* Will be determined later */
	glw::GLint			  gl_max_internalformat_samples_value = -1; /* Will be determined later */
	glw::GLint			  gl_max_samples_value				  = -1; /* Will be determined later */
	const glw::GLsizei	height							  = 1;
	const glw::GLenum	 internalformat_list[]				  = { GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8 };
	const int			  internalformat_list_count = sizeof(internalformat_list) / sizeof(internalformat_list[0]);
	const glw::GLenum	 target = GL_TEXTURE_2D_MULTISAMPLE; /* Test case uses GL_TEXTURE_2D_MULTISAMPLE target */
	const glw::GLsizei	width  = 1;

	/* Iterate through all internal formats test case should check */
	for (int internalformat_index = 0; internalformat_index < internalformat_list_count; internalformat_index++)
	{
		/* Iteration-specific internalformat */
		glw::GLenum internalformat = internalformat_list[internalformat_index];

		/* Iterate through all fixedsamplelocations test case should check */
		for (int fixedsamplelocations_index = 0; fixedsamplelocations_index < fixedsamplelocations_list_count;
			 fixedsamplelocations_index++)
		{
			/* Iteration-specific fixedsamplelocations */
			glw::GLboolean fixedsamplelocations = fixedsamplelocations_list[fixedsamplelocations_index];

			/* Initialize texture object and bind it to GL_TEXTURE_2D_MULTISAMPLE target */
			initInternalIteration();

			/* Get GL_MAX_SAMPLES value */
			gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLES value");

			/* Get GL_MAX_DEPTH_TEXTURE_SAMPLES value */
			gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &gl_max_depth_texture_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_DEPTH_TEXTURE_SAMPLES value");

			/* Retrieve maximum amount of samples available for the texture target considered */
			gl.getInternalformativ(target, internalformat, GL_SAMPLES, 1, &gl_max_internalformat_samples_value);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed to retrieve GL_SAMPLES");

			/* Issue call with valid parameters, but samples argument might be invalid */
			gl.texStorage2DMultisample(target, gl_max_internalformat_samples_value + 1, internalformat, width, height,
									   fixedsamplelocations);

			/* Expect GL_INVALID_OPERATION error code. */
			TCU_CHECK_MSG(gl.getError() == GL_INVALID_OPERATION,
						  "glTexStorage2DMultisample() did not generate GL_INVALID_OPERATION error.");

			/* Deinitialize texture object and unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE target */
			deinitInternalIteration();
		}
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DValidCallsTest::MultisampleTextureTexStorage2DValidCallsTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_valid_calls",
			   "Verifies TexStorage2DMultisample() does not generate an error "
			   "when asked to set up multisample color/depth/textures in various configurations.")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DValidCallsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");

	/* Delete texture object */
	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

		to_id = 0;
	}

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Initializes ES objects required for test execution */
void MultisampleTextureTexStorage2DValidCallsTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a texture object id */
	gl.genTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed to generate a texture object ID");

	/* Bind texture to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DValidCallsTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Retrieve GL_MAX_COLOR_TEXTURE_SAMPLES pname value */
	glw::GLint gl_max_color_texture_samples_value = 0;

	gl.getIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &gl_max_color_texture_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_COLOR_TEXTURE_SAMPLES pname value");

	/* Retrieve GL_MAX_DEPTH_TEXTURE_SAMPLES pname value */
	glw::GLint gl_max_depth_texture_samples_value = 0;

	gl.getIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &gl_max_depth_texture_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_DEPTH_TEXTURE_SAMPLES pname value");

	/* Iterate through color-, depth- and stencil-renderable internalformats */
	const glw::GLenum  color_internalformats[]   = { GL_R8, GL_RGB565, GL_RGB10_A2UI, GL_SRGB8_ALPHA8, GL_R8I };
	const glw::GLenum  depth_internalformats[]   = { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8 };
	const glw::GLenum  stencil_internalformats[] = { GL_DEPTH24_STENCIL8, GL_DEPTH32F_STENCIL8 };
	const unsigned int n_color_internalformats   = sizeof(color_internalformats) / sizeof(color_internalformats[0]);
	const unsigned int n_depth_internalformats   = sizeof(depth_internalformats) / sizeof(depth_internalformats[0]);
	const unsigned int n_stencil_internalformats = sizeof(stencil_internalformats) / sizeof(stencil_internalformats[0]);

	for (unsigned int n_iteration = 0; n_iteration < 3 /* color/depth/stencil */; ++n_iteration)
	{
		const glw::GLenum* internalformats						   = NULL;
		glw::GLint		   max_iteration_specific_gl_samples_value = 0;
		unsigned int	   n_internalformats					   = 0;

		switch (n_iteration)
		{
		case 0:
		{
			internalformats							= color_internalformats;
			max_iteration_specific_gl_samples_value = gl_max_color_texture_samples_value;
			n_internalformats						= n_color_internalformats;

			break;
		}

		case 1:
		{
			internalformats							= depth_internalformats;
			max_iteration_specific_gl_samples_value = gl_max_depth_texture_samples_value;
			n_internalformats						= n_depth_internalformats;

			break;
		}

		case 2:
		{
			internalformats							= stencil_internalformats;
			max_iteration_specific_gl_samples_value = gl_max_depth_texture_samples_value;
			n_internalformats						= n_stencil_internalformats;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized iteration index");
		}
		} /* switch (n_iteration) */

		/* Iterate through valid fixedsamplelocations argument values */
		const glw::GLboolean fixedsamplelocations_values[] = { GL_FALSE, GL_TRUE };
		const unsigned int   n_fixedsamplelocations_values =
			sizeof(fixedsamplelocations_values) / sizeof(fixedsamplelocations_values[0]);

		for (unsigned int n_fixedsamplelocations_value = 0;
			 n_fixedsamplelocations_value < n_fixedsamplelocations_values; ++n_fixedsamplelocations_value)
		{
			glw::GLboolean fixedsamplelocations = fixedsamplelocations_values[n_fixedsamplelocations_value];

			/* Iterate through internalformats */
			for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
			{
				glw::GLenum internalformat			   = internalformats[n_internalformat];
				glw::GLint  internalformat_max_samples = 0;

				/* Retrieve internalformat-specific GL_MAX_SAMPLES value */
				gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE,	 /* target */
									   internalformat, GL_SAMPLES, 1, /* bufSize */
									   &internalformat_max_samples);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed");

				/* Determine maximum amount of samples we can use for the test*/
				glw::GLint max_samples =
					de::max(1, de::min(internalformat_max_samples, max_iteration_specific_gl_samples_value));

				/* Iterate through all valid samples argument values */
				for (int n_samples = 1; n_samples <= max_samples; ++n_samples)
				{
					gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, n_samples, internalformat, 1, /* width */
											   1,														/* height */
											   fixedsamplelocations); /* fixedsamplelocations */

					GLU_EXPECT_NO_ERROR(gl.getError(), "A valid glTexStorage2DMultisample() call failed");

					/* Re-create the texture object before we continue */
					gl.deleteTextures(1, &to_id);
					gl.genTextures(1, &to_id);

					gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to re-create a texture object");
				} /* for (all samples argument values) */
			}	 /* for (all color-renderable internalformats) */
		}		  /* for (all fixedsamplelocations argument values) */
	}			  /* for (all iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureTexStorage2DZeroSampleTest::MultisampleTextureTexStorage2DZeroSampleTest(Context& context)
	: TestCase(context, "multisample_texture_tex_storage_2d_zero_sample",
			   "Verifies TexStorage2DMultisample() rejects zero "
			   "sample requests by generating a GL_INVALID_VALUE error.")
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureTexStorage2DZeroSampleTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind texture object bound to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Failed to unbind a texture object from GL_TEXTURE_2D_MULTISAMPLE texture target");

	/* Delete texture object */
	gl.deleteTextures(1, &to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to delete texture object");

	to_id = 0;

	/* Call base class deinitialization routine */
	glcts::TestCase::deinit();
}

/** Initializes ES objects required for test execution */
void MultisampleTextureTexStorage2DZeroSampleTest::initInternals()
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

	/* Bind texture to GL_TEXTURE_2D_MULTISAMPLE */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() reported an error");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureTexStorage2DZeroSampleTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Issue call function for target GL_TEXTURE_2D_MULTISAMPLE, but provide zero for samples argument */
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_RGBA8, 1, 1, true);

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

} /* glcts namespace */
