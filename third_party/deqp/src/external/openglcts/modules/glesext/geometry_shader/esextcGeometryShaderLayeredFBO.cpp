/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
#include "esextcGeometryShaderLayeredFBO.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{

const unsigned int GeometryShaderLayeredFBOShared::n_shared_fbo_ids = 4; /* as per test spec */
const unsigned int GeometryShaderLayeredFBOShared::n_shared_to_ids  = 7; /* as per test spec */
const glw::GLuint  GeometryShaderLayeredFBOShared::shared_to_depth  = 4; /* as per test spec */
const glw::GLuint  GeometryShaderLayeredFBOShared::shared_to_height = 4; /* as per test spec */
const glw::GLuint  GeometryShaderLayeredFBOShared::shared_to_width  = 4; /* as per test spec */

/** Checks if the bound draw framebuffer's completeness status is equal to the value
 *  specified by the caller.
 *
 *  @param test_context                 Test context used by the conformance test.
 *  @param gl                           ES / GL entry-points.
 *  @param fbo_id                       ID of the framebuffer to use for the query.
 *  @param expected_completeness_status Expected completeness status (described by a GLenum value)
 *
 *  @return true if the draw framebuffer's completeness status matches the expected value,
 *          false otherwise
 */
bool GeometryShaderLayeredFBOShared::checkFBOCompleteness(tcu::TestContext& test_context, const glw::Functions& gl,
														  glw::GLenum fbo_id, glw::GLenum expected_completeness_status)
{
	glw::GLenum current_fbo_status = GL_NONE;
	bool		result			   = true;

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	current_fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCheckFramebufferStatus() call failed.");

	if (current_fbo_status != expected_completeness_status)
	{
		/* Please check doxygen documentation of GeometryShaderIncompleteLayeredFBOTest class
		 * for more details, if you ever reach this location */
		test_context.getLog() << tcu::TestLog::Message << "Test iteration [" << fbo_id << "] failed. "
																						  "Expected: ["
							  << expected_completeness_status << "], "
																 "got: ["
							  << current_fbo_status << "]" << tcu::TestLog::EndMessage;

		result = false;
	}

	return result;
}

/** Deinitializes framebuffer objects, whose IDs are passed by the caller.
 *
 *  @param gl      ES / GL entry-points
 *  @param fbo_ids Exactly GeometryShaderLayeredFBOShared::n_shared_fbo_ids FBO
 *                 ids to be deinitialized.
 */
void GeometryShaderLayeredFBOShared::deinitFBOs(const glw::Functions& gl, const glw::GLuint* fbo_ids)
{
	if (fbo_ids != DE_NULL)
	{
		gl.deleteFramebuffers(GeometryShaderLayeredFBOShared::n_shared_fbo_ids, fbo_ids);
	}
}

/** Deinitializes texture obejcts, whose IDs are passed by the caller.
 *
 *  @param gl     ES / GL entry-points
 *  @param to_ids Exactly GeometryShaderLayeredFBOShared::n_shared_to_ids Texture
 *                Object IDs to be deinitialized.
 */
void GeometryShaderLayeredFBOShared::deinitTOs(const glw::Functions& gl, const glw::GLuint* to_ids)
{
	if (to_ids != DE_NULL)
	{
		gl.deleteTextures(GeometryShaderLayeredFBOShared::n_shared_to_ids, to_ids);
	}
}

/** Initializes all framebuffer objects required to run layered framebufer object conformance test.
 *
 *  @param gl                    ES / GL entry-points
 *  @param pGLFramebufferTexture glFramebufferTexture{EXT}() entry-point func ptr.
 *  @param to_ids                Exactly 7 Texture Object IDs, initialized as per test spec.
 *  @param out_fbo_ids           Deref will be used to store 4 FBO ids, initialized as per test spec.
 **/
void GeometryShaderLayeredFBOShared::initFBOs(const glw::Functions&			gl,
											  glw::glFramebufferTextureFunc pGLFramebufferTexture,
											  const glw::GLuint* to_ids, glw::GLuint* out_fbo_ids)
{
	const glw::GLuint to_id_a	  = to_ids[0];
	const glw::GLuint to_id_a_prim = to_ids[1];
	const glw::GLuint to_id_b	  = to_ids[2];
	const glw::GLuint to_id_c	  = to_ids[3];
	const glw::GLuint to_id_d	  = to_ids[4];
	const glw::GLuint to_id_e	  = to_ids[5];
	const glw::GLuint to_id_f	  = to_ids[6];

	gl.genFramebuffers(n_shared_fbo_ids, out_fbo_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	/* Set up framebuffer object A */
	const glw::GLenum  fbo_a_draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_NONE, GL_COLOR_ATTACHMENT2 };
	const glw::GLenum  fbo_b_draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	const glw::GLenum  fbo_c_draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
	const glw::GLenum  fbo_d_draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	const glw::GLuint  fbo_id_a				= out_fbo_ids[0];
	const glw::GLuint  fbo_id_b				= out_fbo_ids[1];
	const glw::GLuint  fbo_id_c				= out_fbo_ids[2];
	const glw::GLuint  fbo_id_d				= out_fbo_ids[3];
	const unsigned int n_fbo_a_draw_buffers = sizeof(fbo_a_draw_buffers) / sizeof(fbo_a_draw_buffers[0]);
	const unsigned int n_fbo_b_draw_buffers = sizeof(fbo_b_draw_buffers) / sizeof(fbo_b_draw_buffers[0]);
	const unsigned int n_fbo_c_draw_buffers = sizeof(fbo_c_draw_buffers) / sizeof(fbo_c_draw_buffers[0]);
	const unsigned int n_fbo_d_draw_buffers = sizeof(fbo_d_draw_buffers) / sizeof(fbo_d_draw_buffers[0]);

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id_a);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id_a, 0);	 /* level */
	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, to_id_b, 0);	 /* level */
	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, to_id_a_prim, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture() call(s) failed.");

	gl.drawBuffers(n_fbo_a_draw_buffers, fbo_a_draw_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffers() call failed.");

	/* Set up framebuffer object B */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id_b);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id_c, 0); /* level */
	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to_id_a, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture() call(s) failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, to_id_d, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");

	gl.drawBuffers(n_fbo_b_draw_buffers, fbo_b_draw_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffers() call failed.");

	/* Set up framebuffer object C */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id_c);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id_d, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, to_id_a_prim, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture() call failed.");

	gl.drawBuffers(n_fbo_c_draw_buffers, fbo_c_draw_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBufers() call failed.");

	/* Set up framebuffer object D */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id_d);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to_id_e, 0); /* level */
	pGLFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to_id_f, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture() call(s) failed.");

	gl.drawBuffers(n_fbo_d_draw_buffers, fbo_d_draw_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffers() call failed.");
}

/** Initializes exactly seven texture objects, as per test spec.
 *
 *  @param gl                         ES / GL entry-points
 *  @param pGLTexStorage3DMultisample glTexStorage3DMultisample{EXT}() entry-point func ptr.
 *  @param out_to_ids                 Deref will be used to store 7 texture object IDs, initialized
 *                                    as per test spec.
 */
void GeometryShaderLayeredFBOShared::initTOs(const glw::Functions&				gl,
											 glw::glTexStorage3DMultisampleFunc pGLTexStorage3DMultisample,
											 glw::GLuint*						out_to_ids)
{
	gl.genTextures(n_shared_to_ids, out_to_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	/* Set up texture object A */
	const glw::GLuint to_id_a	  = out_to_ids[0];
	const glw::GLuint to_id_a_prim = out_to_ids[1];
	const glw::GLuint to_id_b	  = out_to_ids[2];
	const glw::GLuint to_id_c	  = out_to_ids[3];
	const glw::GLuint to_id_d	  = out_to_ids[4];
	const glw::GLuint to_id_e	  = out_to_ids[5];
	const glw::GLuint to_id_f	  = out_to_ids[6];

	gl.bindTexture(GL_TEXTURE_2D, to_id_a);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8,		  /* color-renderable internalformat */
					shared_to_width, shared_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up texture object A' */
	gl.bindTexture(GL_TEXTURE_2D, to_id_a_prim);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1,	 /* levels */
					GL_DEPTH_COMPONENT16, /* depth-renderable internalformat */
					shared_to_width, shared_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up texture object B */
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, to_id_b);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, /* levels */
					GL_RGBA8,				/* color-renderable internalformat */
					shared_to_width, shared_to_height, shared_to_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	/* Set up texture object C */
	gl.bindTexture(GL_TEXTURE_3D, to_id_c);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage3D(GL_TEXTURE_3D, 1, /* levels */
					GL_RGBA8,		  /* color-renderable internalformats */
					shared_to_width, shared_to_height, shared_to_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	/* Set up texture object D */
	gl.bindTexture(GL_TEXTURE_CUBE_MAP, to_id_d);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_CUBE_MAP, 1, /* levels */
					GL_RGBA8,				/* color-renderable internalformat */
					shared_to_width, shared_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up texture object E */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_e);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2,				/* samples */
							   GL_RGBA8,									/* color-renderable internalformat */
							   shared_to_width, shared_to_height, GL_TRUE); /* fixedsamplelocations */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed.");

	/* Set up texture object F */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, to_id_f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	pGLTexStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 2, /* samples */
							   GL_RGBA8,						   /* color-renderable internalformat */
							   shared_to_width, shared_to_height, shared_to_depth, GL_TRUE); /* fixedsamplelocations */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3DMultisample() call failed.");
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderIncompleteLayeredFBOTest::GeometryShaderIncompleteLayeredFBOTest(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: TestCaseBase(context, extParams, name, description), m_fbo_ids(DE_NULL), m_to_ids(DE_NULL)
{
	// left blank intentionally
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderIncompleteLayeredFBOTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release framebuffer objects */
	GeometryShaderLayeredFBOShared::deinitFBOs(gl, m_fbo_ids);

	if (m_fbo_ids != DE_NULL)
	{
		delete[] m_fbo_ids;

		m_fbo_ids = DE_NULL;
	}

	/* Release texture objects */
	GeometryShaderLayeredFBOShared::deinitTOs(gl, m_to_ids);

	if (m_to_ids != DE_NULL)
	{
		delete[] m_to_ids;

		m_to_ids = DE_NULL;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderIncompleteLayeredFBOTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up texture objects */
	m_to_ids = new glw::GLuint[GeometryShaderLayeredFBOShared::n_shared_to_ids];

	GeometryShaderLayeredFBOShared::initTOs(gl, gl.texStorage3DMultisample, m_to_ids);

	/* Set up framebuffer objects */
	m_fbo_ids = new glw::GLuint[GeometryShaderLayeredFBOShared::n_shared_fbo_ids];

	GeometryShaderLayeredFBOShared::initFBOs(gl, gl.framebufferTexture, m_to_ids, m_fbo_ids);

	/* Verify framebuffer completeness */
	const glw::GLuint  incomplete_fbo_ids[] = { m_fbo_ids[0], m_fbo_ids[1], m_fbo_ids[2], m_fbo_ids[3] };
	const unsigned int n_incomplete_fbo_ids = sizeof(incomplete_fbo_ids) / sizeof(incomplete_fbo_ids[0]);

	for (unsigned int n_incomplete_fbo_id = 0; n_incomplete_fbo_id < n_incomplete_fbo_ids; ++n_incomplete_fbo_id)
	{
		result &= GeometryShaderLayeredFBOShared::checkFBOCompleteness(
			m_testCtx, gl, incomplete_fbo_ids[n_incomplete_fbo_id], m_glExtTokens.FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
	} /* for (all FBO ids) */

	/* All done */
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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderIncompleteLayeredAttachmentsTest::GeometryShaderIncompleteLayeredAttachmentsTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fbo_ids(DE_NULL), m_to_ids(DE_NULL)
{
	// left blank intentionally
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderIncompleteLayeredAttachmentsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release framebuffer objects */
	GeometryShaderLayeredFBOShared::deinitFBOs(gl, m_fbo_ids);

	if (m_fbo_ids != DE_NULL)
	{
		delete[] m_fbo_ids;

		m_fbo_ids = DE_NULL;
	}

	/* Release texture objects */
	GeometryShaderLayeredFBOShared::deinitTOs(gl, m_to_ids);

	if (m_to_ids != DE_NULL)
	{
		delete[] m_to_ids;

		m_to_ids = DE_NULL;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderIncompleteLayeredAttachmentsTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up texture objects */
	m_to_ids = new glw::GLuint[GeometryShaderLayeredFBOShared::n_shared_to_ids];

	GeometryShaderLayeredFBOShared::initTOs(gl, gl.texStorage3DMultisample, m_to_ids);

	/* Set up framebuffer objects */
	m_fbo_ids = new glw::GLuint[GeometryShaderLayeredFBOShared::n_shared_fbo_ids];

	GeometryShaderLayeredFBOShared::initFBOs(gl, gl.framebufferTexture, m_to_ids, m_fbo_ids);

	/* Verify query results for FBO A attachments - as per test spec */
	glw::GLint is_fbo_color_attachment0_layered = GL_TRUE;
	glw::GLint is_fbo_color_attachment1_layered = GL_TRUE;
	glw::GLint is_fbo_color_attachment2_layered = GL_TRUE;
	glw::GLint is_fbo_depth_attachment_layered  = GL_TRUE;

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment0_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment2_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_depth_attachment_layered);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferAttachmentParameteriv() call failed.");

	if (is_fbo_color_attachment0_layered != GL_FALSE || is_fbo_color_attachment2_layered != GL_TRUE ||
		is_fbo_depth_attachment_layered != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT query results retrieved for FBO A"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Verify query results for FBO B attachments - as per test spec */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_ids[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment0_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment1_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment2_layered);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferAttachmentParameteriv() call failed.");

	if (is_fbo_color_attachment0_layered != GL_TRUE || is_fbo_color_attachment1_layered != GL_FALSE ||
		is_fbo_color_attachment2_layered != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT query results retrieved for FBO B"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Verify query results for FBO C attachments - as per test spec */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_ids[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment0_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_depth_attachment_layered);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferAttachmentParameteriv() call failed.");

	if (is_fbo_color_attachment0_layered != GL_TRUE || is_fbo_depth_attachment_layered != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT query results retrieved for FBO C"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Verify query results for FBO D attachments - as per test spec */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_ids[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment0_layered);
	gl.getFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
										   m_glExtTokens.FRAMEBUFFER_ATTACHMENT_LAYERED,
										   &is_fbo_color_attachment1_layered);

	if (is_fbo_color_attachment0_layered != GL_FALSE || is_fbo_color_attachment1_layered != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT query results retrieved for FBO D"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* All done */
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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureInvalidTarget::GeometryShaderFramebufferTextureInvalidTarget(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fbo_id(0), m_to_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureInvalidTarget::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureInvalidTarget::iterate()
{
	bool result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::GLubyte pixels[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA, 2 /* width */, 2 /* height */, 0 /* border */, GL_RGBA,
				  GL_UNSIGNED_BYTE, pixels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D() call failed.");

	gl.generateMipmap(GL_TEXTURE_2D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap() call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed with GL_DRAW_FRAMEBUFFER pname.");

	glw::GLuint errorEnum;

	gl.framebufferTexture(GL_TEXTURE_3D, GL_COLOR_ATTACHMENT0, m_to_id /* texture */, 1 /* level */);
	errorEnum = gl.getError();

	if (errorEnum != GL_INVALID_ENUM)
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_ENUM was generated."
						   << tcu::TestLog::EndMessage;
	}

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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureNoFBOBoundToTarget::GeometryShaderFramebufferTextureNoFBOBoundToTarget(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_to_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureNoFBOBoundToTarget::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureNoFBOBoundToTarget::iterate()
{
	const glw::GLuint fbEnums[] = { GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER };

	const size_t numberOfEnums = sizeof(fbEnums) / sizeof(fbEnums[0]);

	bool result = false;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	glw::GLuint errorEnum;

	for (size_t i = 0; i < numberOfEnums; ++i)
	{
		gl.bindFramebuffer(fbEnums[i], 0 /* framebuffer */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		gl.framebufferTexture(fbEnums[i], GL_COLOR_ATTACHMENT0, m_to_id /* texture */, 1 /* level */);
		errorEnum = gl.getError();

		if (errorEnum == GL_INVALID_OPERATION)
		{
			result = true;
		}
		else
		{
			result = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_OPERATION was generated."
							   << tcu::TestLog::EndMessage;

			break;
		}
	}

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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureInvalidAttachment::GeometryShaderFramebufferTextureInvalidAttachment(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fbo_id(0), m_to_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureInvalidAttachment::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureInvalidAttachment::iterate()
{
	const glw::GLuint fbEnums[] = { GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER };

	const size_t numberOfEnums = sizeof(fbEnums) / sizeof(fbEnums[0]);

	bool result = false;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint errorEnum;
	glw::GLint  maxColorAttachments = 0;

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed with pname GL_MAX_COLOR_ATTACHMENTS.");

	for (size_t i = 0; i < numberOfEnums; ++i)
	{
		gl.bindFramebuffer(fbEnums[i], m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		gl.framebufferTexture(fbEnums[i], GL_COLOR_ATTACHMENT0 + maxColorAttachments, m_to_id /* texture */,
							  0 /* level */);
		errorEnum = gl.getError();

		if (errorEnum == GL_INVALID_OPERATION)
		{
			result = true;
		}
		else
		{
			result = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_OPERATION was generated."
							   << tcu::TestLog::EndMessage;

			break;
		}
	}

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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureInvalidValue::GeometryShaderFramebufferTextureInvalidValue(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fbo_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureInvalidValue::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureInvalidValue::iterate()
{
	const glw::GLuint fbEnums[]		= { GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER, GL_FRAMEBUFFER };
	const size_t	  numberOfEnums = sizeof(fbEnums) / sizeof(fbEnums[0]);
	bool			  result		= false;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	glw::GLuint errorEnum;
	glw::GLuint invalidValue = 1;

	for (size_t i = 0; i < numberOfEnums; ++i)
	{
		gl.bindFramebuffer(fbEnums[i], m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		gl.framebufferTexture(fbEnums[i], GL_COLOR_ATTACHMENT0, invalidValue /* texture */, 1 /* level */);
		errorEnum = gl.getError();

		invalidValue *= 10;

		if (errorEnum == GL_INVALID_VALUE)
		{
			result = true;
		}
		else
		{
			result = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_VALUE was generated."
							   << tcu::TestLog::EndMessage;

			break;
		}
	}

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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureInvalidLevelNumber::GeometryShaderFramebufferTextureInvalidLevelNumber(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_tex_depth(4)
	, m_tex_height(4)
	, m_tex_width(4)
	, m_to_2d_array_id(0)
	, m_to_3d_id(0)
{
	/* Allocate memory for m_tex_depth * m_tex_height * m_tex_width texels, with each texel being 4 GLubytes. */
	m_texels = new glw::GLubyte[m_tex_depth * m_tex_height * m_tex_width * 4];

	memset(m_texels, 255, m_tex_depth * m_tex_height * m_tex_width * 4);
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureInvalidLevelNumber::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if (m_texels != NULL)
	{
		delete[] m_texels;
		m_texels = NULL;
	}

	if (m_to_2d_array_id != 0)
	{
		gl.deleteTextures(1, &m_to_2d_array_id);
		m_to_2d_array_id = 0;
	}

	if (m_to_3d_id != 0)
	{
		gl.deleteTextures(1, &m_to_3d_id);
		m_to_3d_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureInvalidLevelNumber::iterate()
{
	bool result = false;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Prepare texture 3D and generate its mipmaps */
	gl.genTextures(1, &m_to_3d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_3D, m_to_3d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage3D(GL_TEXTURE_3D, 2 /* levels */, GL_RGBA8, m_tex_width, m_tex_height, m_tex_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	gl.texSubImage3D(GL_TEXTURE_3D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */, m_tex_width,
					 m_tex_height, m_tex_depth, GL_RGBA, GL_UNSIGNED_BYTE, m_texels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D() call failed.");

	gl.generateMipmap(GL_TEXTURE_3D);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap() call failed with pname GL_TEXTURE_3D.");

	/* Prepare texture array 2D */
	gl.genTextures(1, &m_to_2d_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_2d_array_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 2 /* levels */, GL_RGBA8, m_tex_width, m_tex_height,
					m_tex_depth /* layerCount */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed.");

	gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, 0 /* zoffset */, m_tex_width,
					 m_tex_height, m_tex_depth, GL_RGBA, GL_UNSIGNED_BYTE, m_texels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D() call failed.");

	gl.generateMipmap(GL_TEXTURE_2D_ARRAY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenerateMipmap() call failed with pname GL_TEXTURE_2D_ARRAY.");

	glw::GLuint errorEnum;

	/* Test for texture 3D */
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_3d_id /* texture */, 2 /* level */);
	errorEnum = gl.getError();

	if (errorEnum == GL_INVALID_VALUE)
	{
		result = true;
	}
	else
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_VALUE was generated."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

	/* Test for texture array 2D */
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_2d_array_id /* texture */, 2 /* level */);
	errorEnum = gl.getError();

	if (errorEnum == GL_INVALID_VALUE)
	{
		result = true;
	}
	else
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_VALUE was generated."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

end:
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

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFramebufferTextureArgumentRefersToBufferTexture::
	GeometryShaderFramebufferTextureArgumentRefersToBufferTexture(Context& context, const ExtParameters& extParams,
																  const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_bo_id(0), m_fbo_id(0), m_tbo_id(0)
{
	m_tex_width  = 64;
	m_tex_height = 64;

	/* Allocate memory for m_tex_height * m_tex_width texels, with each texel being 3 GLints. */
	m_texels = new glw::GLint[m_tex_height * m_tex_width * 3];

	memset(m_texels, 255, m_tex_height * m_tex_width * 3);
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFramebufferTextureArgumentRefersToBufferTexture::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);
		m_bo_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if (m_tbo_id != 0)
	{
		gl.deleteTextures(1, &m_tbo_id);
		m_tbo_id = 0;
	}

	if (m_texels != NULL)
	{
		delete[] m_texels;
		m_texels = NULL;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFramebufferTextureArgumentRefersToBufferTexture::iterate()
{
	bool result = false;

	/* This test should only run if EXT_geometry_shader and EXT_texture_buffer are supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate buffer object */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(m_texels), m_texels, GL_DYNAMIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Generate and bind framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	/* Prepare texture buffer */
	gl.genTextures(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texBuffer(GL_TEXTURE_BUFFER, GL_RGB32I, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexBuffer() call failed.");

	glw::GLuint errorEnum;

	/* Test for texture 3D */
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tbo_id /* texture */, 0 /* level */);
	errorEnum = gl.getError();

	if (errorEnum == GL_INVALID_OPERATION)
	{
		result = true;
	}
	else
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_OPERATION was generated."
						   << tcu::TestLog::EndMessage;

		goto end;
	}

end:
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

} // namespace glcts
