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

/*!
 * \file  esextcTextureCubeMapArrayFBOIncompleteness.cpp
 * \brief texture_cube_map_array extenstion - FBO incompleteness (Test 9)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayFBOIncompleteness.hpp"

#include "gluContextInfo.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
TextureCubeMapArrayFBOIncompleteness::TextureCubeMapArrayFBOIncompleteness(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_lots_of_layers_to_id(0)
	, m_non_layered_to_id(0)
	, m_small_to_id(0)
{
	/* Nothing to be done here */
}

/**  Verifies an expected FBO completeness status is reported for FBO that is currently
 *   bound to GL_DRAW_FRAMEBUFFER FBO binding.
 *
 *   @param expected_state Anticipated FBO completeness status;
 *   @param message        Error message to prepend logged error message.
 *
 *   @return              true if framebuffer state is equal to @param expectedState;
 *                        false otherwise
 */
glw::GLboolean TextureCubeMapArrayFBOIncompleteness::checkState(glw::GLint expected_state, const char* message)
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLenum			  fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	glw::GLboolean		  result		   = true;

	/* if fboCompleteness is different than expectedState log error */
	if (fbo_completeness != (glw::GLuint)expected_state)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << message << "; FBO should be in "
						   << glu::getErrorStr(expected_state)
						   << " state, but is in : " << glu::getErrorStr(fbo_completeness) << " state"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	return result;
}

/** Deinitialize test case
 *
 **/
void TextureCubeMapArrayFBOIncompleteness::deinit()
{
	/* Retrieve GL entry point */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset FBO and texture bindings */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
	gl.bindTexture(GL_TEXTURE_2D, 0);

	/* Release all ES objects the test may have created */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_non_layered_to_id != 0)
	{
		gl.deleteTextures(1, &m_non_layered_to_id);

		m_non_layered_to_id = 0;
	}

	if (m_small_to_id != 0)
	{
		gl.deleteTextures(1, &m_small_to_id);

		m_small_to_id = 0;
	}

	if (m_lots_of_layers_to_id != 0)
	{
		gl.deleteTextures(1, &m_lots_of_layers_to_id);

		m_lots_of_layers_to_id = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Initializes all ES objects necessary to execute the test */
void TextureCubeMapArrayFBOIncompleteness::initTest()
{
	/* Check if texture_cube_map_array extension is supported */
	if (!m_is_texture_cube_map_array_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_CUBE_MAP_ARRAY_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve GL entry point */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create texture objects.*/
	gl.genTextures(1, &m_small_to_id);
	gl.genTextures(1, &m_lots_of_layers_to_id);
	gl.genTextures(1, &m_non_layered_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call(s) failed");

	/* Create a framebuffer object we will use for the test */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");
}

/** Executes the test.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return Always STOP.
 **/
tcu::TestCase::IterateResult TextureCubeMapArrayFBOIncompleteness::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	glw::GLboolean		  result = true;

	/* Create all ES objects necessary to run the test first */
	initTest();

	/* Modify the draw framebuffer binding */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object!");

	/* Set up texture storage for small texture */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_small_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to GL_TEXTURE_CUBE_MAP_ARRAY target.");

	gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_levels, GL_RGBA8, m_texture_width, m_texture_height,
					m_small_texture_depth);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	/* Verify that a FBO is considered incomplete, if a non-existing layer
	 * (whose index is larger or equal to the layer count) of a cube-map
	 * array texture is attached to any of the framebuffer's non-layered
	 * attachments.
	 */
	gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_small_to_id, 0, /* level */
							   m_small_texture_depth);										/* layer */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");

	result = checkState(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "Attachment of an invalid texture layer ");

	/* The remaining parts of the test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "EXT_geometry_shader is not supported, skip remaining tests."
						   << tcu::TestLog::EndMessage;
		if (result)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}

		return STOP;
	}

	/* In order to check that a framebuffer is considered incomplete if the layer count
	 * of a cube-map array texture that has been bound to a layered framebuffer is
	 * larger than or equal to GL_MAX_FRAMEBUFFER_LAYERS_EXT, we need to create a cube-map
	 * array texture object that we'll use for the purpose of the test */
	glw::GLint gl_max_framebuffer_layers_ext_value = 0;
	glw::GLint gl_max_array_texture_layers		   = 0;
	glw::GLint required_to_depth				   = 0;

	gl.getIntegerv(m_glExtTokens.MAX_FRAMEBUFFER_LAYERS, &gl_max_framebuffer_layers_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_FRAMEBUFFER_LAYERS_EXT pname");

	gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &gl_max_array_texture_layers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_ARRAY_TEXTURE_LAYERS pname");

	/* Calculate the amount of layers we need our texture object to use. The number has to
	 * be larger than GL_MAX_FRAMEBUFFER_LAYERS_EXT and be a multiple of 6
	 */
	required_to_depth = ((gl_max_framebuffer_layers_ext_value / 6) + 1) * 6;

	/* Execute only if allowed number of layers for GL_TEXTURE_CUBE_MAP_ARRAY
	 * is greater than GL_MAX_FRAMEBUFFER_LAYERS_EXT
	 */
	if (required_to_depth <= gl_max_array_texture_layers && required_to_depth > gl_max_framebuffer_layers_ext_value)
	{
		/* Bind a new ID to GL_TEXTURE_CUBE_MAP_ARRAY texture target */
		gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_lots_of_layers_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		/* Configure texture storage */
		gl.texStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture_levels, GL_RGBA8, m_texture_width, m_texture_height,
						required_to_depth);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture!");

		/* Update the FBO binding */
		gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_lots_of_layers_to_id, 0); /* level */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureEXT() call failed");

		result =
			checkState(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "Attachment of too many layers to a layered framebuffer ");
	}

	/* Configure storage for a 2D array texture object. We'll need it to verify
	 * that a layered framebuffer is considered incomplete and its status is
	 * reported as GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT, if a single level of
	 * a cube-map array texture AND of a 2D array texture are attached to two
	 * different color attachments of the same framebuffer object.
	 **/
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_non_layered_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, m_texture_levels, GL_RGBA8, m_texture_width, m_texture_height,
					m_small_texture_depth);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	/* Configure the FBO attachments */
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_small_to_id, 0);		  /* level */
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_non_layered_to_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureEXT() call(s) failed");

	/* Perform the check */
	result = checkState(m_glExtTokens.FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,
						"Attachment of two textures from different targets to the same FBO ");

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} /* glcts */
