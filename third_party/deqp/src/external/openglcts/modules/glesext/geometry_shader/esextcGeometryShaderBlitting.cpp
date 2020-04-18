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

#include "esextcGeometryShaderBlitting.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

/* 3d texture layers count */
#define TEXTURE_DEPTH (4)
/* 3d texture width */
#define TEXTURE_WIDTH (4)
/* 3d texture height */
#define TEXTURE_HEIGHT (4)
/* texture texel components */
#define TEXTURE_TEXEL_COMPONENTS (4)

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderBlitting::GeometryShaderBlitting(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_draw_fbo_completeness(GL_NONE)
	, m_read_fbo_completeness(GL_NONE)
	, m_fbo_draw_id(0)
	, m_fbo_read_id(0)
	, m_to_draw(0)
	, m_to_read(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderBlitting::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Generate and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up vertex array object");

	/* Generate texture objects */
	gl.genTextures(1, &m_to_draw);
	gl.genTextures(1, &m_to_read);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture objects");

	/* Generate framebuffer objects */
	gl.genFramebuffers(1, &m_fbo_draw_id);
	gl.genFramebuffers(1, &m_fbo_read_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up framebuffer objects");

	/* Configure texture storage */
	gl.bindTexture(GL_TEXTURE_3D, m_to_draw);
	gl.texImage3D(GL_TEXTURE_3D, 0 /* level */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH, 0 /* border */,
				  GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up texture objects");

	gl.bindTexture(GL_TEXTURE_3D, m_to_read);
	gl.texImage3D(GL_TEXTURE_3D, 0 /* level */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH, 0 /* border */,
				  GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up texture objects");

	unsigned char layer_source1[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
	unsigned char layer_source2[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
	unsigned char layer_source3[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
	unsigned char layer_source4[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
	unsigned char layer_target[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
	unsigned char result_data[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];

	memset(layer_source1, 0, sizeof(layer_source1));
	memset(layer_source2, 0, sizeof(layer_source2));
	memset(layer_source3, 0, sizeof(layer_source3));
	memset(layer_source4, 0, sizeof(layer_source4));
	memset(layer_target, 0, sizeof(layer_target));

	for (unsigned int n = 0; n < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++n)
	{
		layer_source1[n * TEXTURE_TEXEL_COMPONENTS + 0] = 255;
		layer_source2[n * TEXTURE_TEXEL_COMPONENTS + 1] = 255;
		layer_source3[n * TEXTURE_TEXEL_COMPONENTS + 2] = 255;
		layer_source4[n * TEXTURE_TEXEL_COMPONENTS + 3] = 255;
	} /* for (all pixels) */

	/* Set up draw FBO */
	setUpFramebuffersForRendering(m_fbo_draw_id, m_fbo_read_id, m_to_draw, m_to_read);

	/* Fill texture objects with data */
	gl.bindTexture(GL_TEXTURE_3D, m_to_read);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_source1);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_source2);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_source3);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_source4);

	gl.bindTexture(GL_TEXTURE_3D, m_to_draw);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_target);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 1 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_target);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 2 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_target);
	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 3 /* layer */, TEXTURE_WIDTH,
					 TEXTURE_HEIGHT, 1 /* depth */, GL_RGBA, GL_UNSIGNED_BYTE, layer_target);

	/* Check FB completeness */
	m_draw_fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	m_read_fbo_completeness = gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	if (m_draw_fbo_completeness != GL_FRAMEBUFFER_COMPLETE)
	{
		TCU_FAIL("Draw framebuffer is reported as incomplete, though expected it to be complete");
	}

	if (m_read_fbo_completeness != GL_FRAMEBUFFER_COMPLETE)
	{
		TCU_FAIL("Read framebuffer is reported as incomplete, though expected it to be complete");
	}

	/* Blit! */
	gl.blitFramebuffer(0 /* srcX0 */, 0 /* srcY0 */, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0 /* dstX0 */, 0 /* dstY0 */,
					   TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error blitting");

	for (unsigned int n = 0; n < TEXTURE_DEPTH /* layers */; ++n)
	{
		unsigned char expected_result_data[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS];
		memset(expected_result_data, 0, sizeof(expected_result_data));

		if (n == 0)
		{
			memcpy(expected_result_data, layer_source1, sizeof(expected_result_data));
		}

		/* Set up read FBO */
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_draw_id);

		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_draw, 0, n);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up FBOs setUpFramebuffersForReading");

		/* Read the rendered data */
		gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, result_data);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to read rendered pixel data");

		/* Compare the rendered data with reference data */
		for (unsigned int index = 0; index < TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_TEXEL_COMPONENTS;
			 index += TEXTURE_TEXEL_COMPONENTS)
		{
			if (memcmp(&expected_result_data[index], &result_data[index], TEXTURE_TEXEL_COMPONENTS) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << (int)result_data[index + 0] << ", "
								   << (int)result_data[index + 1] << ", " << (int)result_data[index + 2] << ", "
								   << (int)result_data[index + 3] << "] is different from reference data ["
								   << (int)expected_result_data[index + 0] << ", "
								   << (int)expected_result_data[index + 1] << ", "
								   << (int)expected_result_data[index + 2] << ", "
								   << (int)expected_result_data[index + 3] << "] !" << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		} /* for (each index) */
	}	 /* for (all layers) */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderBlitting::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.bindTexture(GL_TEXTURE_3D, 0);
	gl.bindVertexArray(0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	/* Clean up */
	if (m_fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_draw_id);
	}

	if (m_fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_read_id);
	}

	if (m_to_draw != 0)
	{
		gl.deleteTextures(1, &m_to_draw);
	}

	if (m_to_read != 0)
	{
		gl.deleteTextures(1, &m_to_read);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderBlittingLayeredToNonLayered::GeometryShaderBlittingLayeredToNonLayered(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: GeometryShaderBlitting(context, extParams, name, description)
{
	/* Left blank on purpose */
}

/** Setup framebuffers for rendering
 *
 */
void GeometryShaderBlittingLayeredToNonLayered::setUpFramebuffersForRendering(glw::GLuint fbo_draw_id,
																			  glw::GLuint fbo_read_id,
																			  glw::GLint to_draw, glw::GLint to_read)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind framebuffers */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_read_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_draw_id);

	/* Blitting from a layered read framebuffer to a non-layered draw framebuffer*/
	gl.framebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_read, 0 /* level */);
	gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_draw, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up FBOs setUpFramebuffersForRendering");
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderBlittingNonLayeredToLayered::GeometryShaderBlittingNonLayeredToLayered(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: GeometryShaderBlitting(context, extParams, name, description)
{
	/* Left blank on purpose */
}

/** Setup framebuffers for rendering
 *
 */
void GeometryShaderBlittingNonLayeredToLayered::setUpFramebuffersForRendering(glw::GLuint fbo_draw_id,
																			  glw::GLuint fbo_read_id,
																			  glw::GLint to_draw, glw::GLint to_read)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind framebuffers */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_read_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_draw_id);

	/*  Blitting from a non-layered read framebuffer to a layered draw framebuffer */
	gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_read, 0, 0);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_draw, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up FBOs setUpFramebuffersForRendering");
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderBlittingLayeredToLayered::GeometryShaderBlittingLayeredToLayered(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: GeometryShaderBlitting(context, extParams, name, description)
{
	/* Left blank on purpose */
}

/** Setup framebuffers for rendering
 *
 */
void GeometryShaderBlittingLayeredToLayered::setUpFramebuffersForRendering(glw::GLuint fbo_draw_id,
																		   glw::GLuint fbo_read_id, glw::GLint to_draw,
																		   glw::GLint to_read)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind framebuffers */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_read_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_draw_id);

	/* Blitting from a layered read framebuffer to a layered draw framebuffer */
	gl.framebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_read, 0 /* level */);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_draw, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up FBOs setUpFramebuffersForRendering");
}

} // namespace glcts
