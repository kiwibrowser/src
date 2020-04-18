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
 * \file  es31cTextureStorageMultisampleFunctionalTests.cpp
 * \brief Implements conformance tests that verify behavior of
 *        multisample texture functionality (ES3.1 only).
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleFunctionalTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <string.h>
#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsBlittingTest::MultisampleTextureFunctionalTestsBlittingTest(Context& context)
	: TestCase(context, "multisampled_fbo_to_singlesampled_fbo_blit",
			   "Verifies that blitting from a multi-sampled framebuffer object "
			   "to a single-sampled framebuffer object does not generate an error.")
	, dst_fbo_id(0)
	, dst_to_color_id(0)
	, dst_to_depth_stencil_id(0)
	, src_fbo_id(0)
	, src_to_color_id(0)
	, src_to_depth_stencil_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsBlittingTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (dst_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &dst_fbo_id);

		dst_fbo_id = 0;
	}

	if (src_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &src_fbo_id);

		src_fbo_id = 0;
	}

	if (dst_to_color_id != 0)
	{
		gl.deleteTextures(1, &dst_to_color_id);

		dst_to_color_id = 0;
	}

	if (dst_to_depth_stencil_id != 0)
	{
		gl.deleteTextures(1, &dst_to_depth_stencil_id);

		dst_to_depth_stencil_id = 0;
	}

	if (src_to_color_id != 0)
	{
		gl.deleteTextures(1, &src_to_color_id);

		src_to_color_id = 0;
	}

	if (src_to_depth_stencil_id != 0)
	{
		gl.deleteTextures(1, &src_to_depth_stencil_id);

		src_to_depth_stencil_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsBlittingTest::iterate()
{
	bool are_2d_ms_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up texture objects */
	gl.genTextures(1, &dst_to_color_id);
	gl.genTextures(1, &dst_to_depth_stencil_id);
	gl.genTextures(1, &src_to_color_id);
	gl.genTextures(1, &src_to_depth_stencil_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	if (are_2d_ms_array_tos_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, src_to_color_id);
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGBA8, 4,							   /* width */
								   4,									   /* height */
								   4,									   /* depth */
								   GL_TRUE);							   /* fixedsamplelocations */
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"gltexStorage3DMultisample() call failed for texture object src_to_color_id");
	}
	else
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, src_to_color_id);
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
								   GL_RGBA8, 4,					 /* width */
								   4,							 /* height */
								   GL_TRUE);					 /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"gltexStorage3DMultisample() call failed for texture object src_to_color_id");
	}

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, src_to_depth_stencil_id);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_DEPTH32F_STENCIL8, 4,		 /* width */
							   4,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glTexStorage2DMultisample() call failed for texture object src_to_depth_stencil_id");

	gl.bindTexture(GL_TEXTURE_2D, dst_to_color_id);
	gl.texImage2D(GL_TEXTURE_2D, 0,					/* level */
				  GL_RGBA8, 4,						/* width */
				  4,								/* height */
				  0,								/* border */
				  GL_RGBA, GL_UNSIGNED_BYTE, NULL); /* data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D() call failed for texture object dst_to_color_id");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, dst_to_depth_stencil_id);
	gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0,									  /* level */
				  GL_DEPTH32F_STENCIL8, 4,									  /* width */
				  4,														  /* height */
				  4,														  /* depth */
				  0,														  /* border */
				  GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL); /* data */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D9) call failed for texture object dst_to_depth_stencil_id");

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &dst_fbo_id);
	gl.genFramebuffers(1, &src_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up framebuffer objects");

	/* Set up source FBO attachments. */
	glw::GLenum fbo_completeness_status = 0;

	if (are_2d_ms_array_tos_supported)
	{
		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, src_to_color_id, 0, /* level */
								   0);															  /* layer */
	}
	else
	{
		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, src_to_color_id,
								0); /* level */
	}

	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE,
							src_to_depth_stencil_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up source framebuffer's attachments");

	fbo_completeness_status = gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER);

	if (fbo_completeness_status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Source FBO completeness status is: " << fbo_completeness_status
						   << ", expected: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Source FBO is considered incomplete which is invalid");
	}

	/* Set up draw FBO attachments */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_to_color_id, 0);   /* level */
	gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, dst_to_depth_stencil_id, 0, /* level */
							   0);																			 /* layer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up draw framebuffer's attachments");

	fbo_completeness_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness_status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Draw FBO completeness status is: " << fbo_completeness_status
						   << ", expected: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Draw FBO is considered incomplete which is invalid");
	}

	/* Perform the blitting operation */
	gl.blitFramebuffer(0, /* srcX0 */
					   0, /* srcY0 */
					   4, /* srcX1 */
					   4, /* srcY1 */
					   0, /* dstX0 */
					   0, /* dstY0 */
					   4, /* dstX1 */
					   4, /* dstY1 */
					   GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
					   GL_NEAREST); /* An INVALID_OPERATION error is generated if ...., and filter is not NEAREST. */

	/* Make sure no error was generated */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBlitFramebuffer() call failed.");

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest::
	MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest(Context& context)
	: TestCase(context, "blitting_multisampled_depth_attachment",
			   "Verifies that blitting a multi-sampled depth attachment to "
			   "a single-sampled depth attachment gives valid results")
	, fbo_dst_id(0)
	, fbo_src_id(0)
	, fs_depth_preview_id(0)
	, fs_id(0)
	, po_depth_preview_id(0)
	, po_id(0)
	, to_dst_preview_id(0)
	, to_dst_id(0)
	, to_src_id(0)
	, vao_id(0)
	, vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_dst_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_dst_id);

		fbo_dst_id = 0;
	}

	if (fbo_src_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_src_id);

		fbo_src_id = 0;
	}

	if (to_dst_preview_id != 0)
	{
		gl.deleteTextures(1, &to_dst_preview_id);

		to_dst_preview_id = 0;
	}

	if (to_dst_id != 0)
	{
		gl.deleteTextures(1, &to_dst_id);

		to_dst_id = 0;
	}

	if (to_src_id != 0)
	{
		gl.deleteTextures(1, &to_src_id);

		to_src_id = 0;
	}

	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);

		fs_id = 0;
	}

	if (fs_depth_preview_id != 0)
	{
		gl.deleteShader(fs_depth_preview_id);

		fs_depth_preview_id = 0;
	}

	if (po_depth_preview_id != 0)
	{
		gl.deleteProgram(po_depth_preview_id);

		po_depth_preview_id = 0;
	}

	if (po_id != 0)
	{
		gl.deleteProgram(po_id);

		po_id = 0;
	}

	if (vao_id != 0)
	{
		gl.deleteVertexArrays(1, &vao_id);

		vao_id = 0;
	}

	if (vs_id != 0)
	{
		gl.deleteShader(vs_id);

		vs_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate all objects this test will use */
	fs_depth_preview_id = gl.createShader(GL_FRAGMENT_SHADER);
	fs_id				= gl.createShader(GL_FRAGMENT_SHADER);
	vs_id				= gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	po_depth_preview_id = gl.createProgram();
	po_id				= gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	gl.genTextures(1, &to_src_id);
	gl.genTextures(1, &to_dst_preview_id);
	gl.genTextures(1, &to_dst_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed");

	gl.genFramebuffers(1, &fbo_src_id);
	gl.genFramebuffers(1, &fbo_dst_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed");

	gl.genVertexArrays(1, &vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed");

	/* Bind FBOs to relevant FB targets */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_dst_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_src_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call(s) failed");

	/* Configure body of vs_id vertex shader */
	const glw::GLchar* vs_body = "#version 310 es\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "out vec2 uv;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    switch(gl_VertexID)\n"
								 "    {\n"
								 "        case 0: gl_Position = vec4(-1, -1, 0, 1); uv = vec2(0, 0); break;\n"
								 "        case 1: gl_Position = vec4(-1,  1, 0, 1); uv = vec2(0, 1); break;\n"
								 "        case 2: gl_Position = vec4( 1,  1, 0, 1); uv = vec2(1, 1); break;\n"
								 "        case 3: gl_Position = vec4( 1, -1, 0, 1); uv = vec2(1, 0); break;\n"
								 "    }\n"
								 "}\n";

	gl.shaderSource(vs_id, 1 /* count */, &vs_body, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader of vs_id id");

	/* Configure body of fs_id fragment shader */
	const glw::GLchar* fs_body = "#version 310 es\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "in vec2 uv;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_FragDepth = uv.x * uv.y;\n"
								 "}\n";

	gl.shaderSource(fs_id, 1 /* count */, &fs_body, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader of fs_id id");

	/* Configure body of fs_depth_preview_id fragment shader */
	const glw::GLchar* fs_depth_preview_body = "#version 310 es\n"
											   "\n"
											   "precision highp int;\n"
											   "precision highp float;\n"
											   "\n"
											   "in      vec2      uv;\n"
											   "out     uint      result;\n"
											   "uniform sampler2D tex;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    result = floatBitsToUint(textureLod(tex, uv, 0.0).x);\n"
											   "}\n";

	gl.shaderSource(fs_depth_preview_id, 1 /* count */, &fs_depth_preview_body, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader of fs_depth_preview_id id");

	/* Compile all the shaders */
	const glw::GLuint  shader_ids[] = { fs_id, fs_depth_preview_id, vs_id };
	const unsigned int n_shader_ids = sizeof(shader_ids) / sizeof(shader_ids[0]);

	for (unsigned int n_shader_id = 0; n_shader_id < n_shader_ids; ++n_shader_id)
	{
		glw::GLint compile_status = GL_FALSE;

		gl.compileShader(shader_ids[n_shader_id]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

		gl.getShaderiv(shader_ids[n_shader_id], GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

		if (compile_status != GL_TRUE)
		{
			TCU_FAIL("Shader compilation failure");
		}
	} /* for (all shader ids) */

	/* Configure and link both program objects */
	const glw::GLuint  program_ids[] = { po_depth_preview_id, po_id };
	const unsigned int n_program_ids = sizeof(program_ids) / sizeof(program_ids[0]);

	gl.attachShader(po_depth_preview_id, fs_depth_preview_id);
	gl.attachShader(po_depth_preview_id, vs_id);
	gl.attachShader(po_id, fs_id);
	gl.attachShader(po_id, vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	for (unsigned int n_program_id = 0; n_program_id < n_program_ids; ++n_program_id)
	{
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(program_ids[n_program_id]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

		gl.getProgramiv(program_ids[n_program_id], GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

		if (link_status != GL_TRUE)
		{
			TCU_FAIL("Program linking failure");
		}
	} /* for (all program ids) */

	/* Retrieve maximum amount of samples that can be used for GL_DEPTH_COMPONENT32F internalformat */
	glw::GLint n_depth_component32f_max_samples = 0;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_DEPTH_COMPONENT32F, GL_SAMPLES, 1, /* bufSize */
						   &n_depth_component32f_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed");

	/* Configure texture storage for to_src_id */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_src_id);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, n_depth_component32f_max_samples, GL_DEPTH_COMPONENT32F,
							   64,		  /* width */
							   64,		  /* height */
							   GL_FALSE); /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure texture storage for texture object to_src_id");

	gl.texParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	/* When <target> is TEXTURE_2D_MULTISAMPLE  or
	 TEXTURE_2D_MULTISAMPLE_ARRAY_XXX, certain texture parameters may not be
	 specified. In this case, an INVALID_ENUM error is generated if the
	 parameter is any sampler state value from table 6.10. */
	glw::GLenum error_code = gl.getError();
	if (error_code != GL_INVALID_ENUM)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteri() call generated an error " << error_code
						   << " instead of the expected error code " << GL_INVALID_ENUM << tcu::TestLog::EndMessage;

		TCU_FAIL("glTexParameterf() with target GL_TEXTURE_2D_MULTISAMPLE and pname=GL_TEXTURE_MIN_FILTER did not "
				 "generate GL_INVALID_ENUM.");
	}

	/* Configure texture storage for to_dst_id */
	gl.bindTexture(GL_TEXTURE_2D, to_dst_id);
	gl.texStorage2D(GL_TEXTURE_2D, 1,		   /* levels */
					GL_DEPTH_COMPONENT32F, 64, /* width */
					64);					   /* height */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure texture storage for texture object to_dst_id");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set nearest minification filter for texture object to_dst_id");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set nearest magnification filter for texture object to_dst_id");

	/* Configure draw framebuffer */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, to_src_id,
							0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure draw framebuffer for depth data rendering");

	/* Render the test geometry.
	 *
	 * NOTE: Original test spec rendered to fbo_dst_id which would have been invalid,
	 *       since fbo_dst_id is the assumed destination for the blitting operation.
	 *       Actual FBO management code has been rewritten to improve code clarity.
	 *
	 * NOTE: Original test spec didn't enable the depth tests, which are necessary
	 *       for the depth buffer to be updated.
	 **/
	gl.viewport(0, 0, 64 /* rt width */, 64 /* rt height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed");

	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_DEPTH_TEST) failed");

	gl.depthFunc(GL_ALWAYS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gldepthFunc(GL_ALWAYS) failed");

	gl.useProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() for program object po_id failed");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed");

	gl.drawArrays(GL_TRIANGLE_FAN, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

	/* Configure FBOs for blitting */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, to_src_id,
							0); /* level */

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, to_dst_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure FBOs for blitting");

	/* Blit source FBO to destination FBO */
	gl.blitFramebuffer(0,  /* srcX0 */
					   0,  /* srcY0 */
					   64, /* srcX1 */
					   64, /* srcY1 */
					   0,  /* dstX0 */
					   0,  /* dstY0 */
					   64, /* dstX1 */
					   64, /* dstY1 */
					   GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBlitFramebuffer() failed");

	/* Single-sampled depth data is now stored in dst_to_id in GL_DEPTH_COMPONENT32F which may
	 * not work too well with glReadPixels().
	 *
	 * We will now convert it to GL_R32UI using the other program */
	gl.useProgram(po_depth_preview_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed for program object po_depth_preview_id");

	/* Configure texture storage for the destination texture object */
	gl.bindTexture(GL_TEXTURE_2D, to_dst_preview_id);
	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_R32UI, 64,	 /* width */
					64);			  /* height */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texcture storage for texture object to_dst_preview_id");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri() failed for texture object to_dst_preview_id");

	/* Update draw framebuffer configuration */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, /* texture */
							0);															/* level */

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to_dst_preview_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed for depth preview pass");

	/* Render a full-screen quad */
	gl.bindTexture(GL_TEXTURE_2D, to_dst_id);

	gl.drawArrays(GL_TRIANGLE_FAN, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed for depth preview pass");

	/* to_dst_id now contains the result data.
	 *
	 * Before we do a glReadPixels() call, we need to re-configure the read FBO */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, /* texture */
							0);															/* level */

	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to_dst_preview_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed for depth data download pass");

	/* Read the data */
	unsigned int buffer[64 /* width */ * 64 /* height */ * 4 /* components */];

	gl.readPixels(0,  /* x */
				  0,  /* y */
				  64, /* width */
				  64, /* height */
				  GL_RGBA_INTEGER, GL_UNSIGNED_INT, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed for depth data download pass");

	/* Verify the rendered data */
	const float epsilon = 1.0f / 64.0f + (float)1e-5;

	for (int y = 0; y < 64 /* height */; ++y)
	{
		/* NOTE: data_y is used to take the bottom-top orientation of the data downloaded
		 *       by glReadPixels() into account.
		 */
		const int data_y	 = (64 /* height */ - y - 1 /* counting starts from 0 */);
		const int pixel_size = 4; /* components */
		const int row_width  = 64 /* width */ * pixel_size;
		float*	row_ptr	= (float*)buffer + row_width * data_y;

		for (int x = 0; x < 64 /* width */; ++x)
		{
			float* data_ptr			  = row_ptr + x * pixel_size;
			float  depth			  = data_ptr[0];
			float  expected_value_max = float(x) * float(data_y) / (64.0f * 64.0f) + epsilon;
			float  expected_value_min = float(x) * float(data_y) / (64.0f * 64.0f) - epsilon;

			if (!(depth >= expected_value_min && depth <= expected_value_max))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data rendered at (" << x << ", " << y << "): "
								   << "Expected value from " << expected_value_min << " to " << expected_value_max
								   << ", "
								   << "rendered: " << depth << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid data rendered");
			}
		} /* for (all x argument values) */
	}	 /* for (all y argument values) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest::
	MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest(Context& context)
	: TestCase(context, "blitting_multisampled_integer_attachment",
			   "Verifies that blitting a multi-sampled integer attachment "
			   "to a single-sampled integer attachment gives valid results")
	, dst_to_id(0)
	, fbo_draw_id(0)
	, fbo_read_id(0)
	, fs_id(0)
	, po_id(0)
	, src_to_id(0)
	, vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_draw_id);

		fbo_draw_id = 0;
	}

	if (fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_read_id);

		fbo_read_id = 0;
	}

	if (dst_to_id != 0)
	{
		gl.deleteTextures(1, &dst_to_id);

		dst_to_id = 0;
	}

	if (src_to_id != 0)
	{
		gl.deleteTextures(1, &src_to_id);

		src_to_id = 0;
	}

	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);

		fs_id = 0;
	}

	if (po_id != 0)
	{
		gl.deleteProgram(po_id);

		po_id = 0;
	}

	if (vs_id != 0)
	{
		gl.deleteShader(vs_id);

		vs_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure GL_MAX_INTEGER_SAMPLES is at least 2 */
	glw::GLint gl_max_integer_samples_value = 0;

	gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &gl_max_integer_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query GL_MAX_INTEGER_SAMPLES pname value");

	if (gl_max_integer_samples_value < 2)
	{
		throw tcu::NotSupportedError("GL_MAX_INTEGER_SAMPLES is lower than 2");
	}

	/* Retrieve maximum amount of samples for GL_R16UI internalformat */
	glw::GLint r16ui_max_samples = 0;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_R16UI, GL_SAMPLES, 1 /* bufSize */, &r16ui_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() call failed.");

	/* Set up texture objects */
	gl.genTextures(1, &dst_to_id);
	gl.genTextures(1, &src_to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, src_to_id);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, r16ui_max_samples, GL_R16UI, 16, /* width */
							   16,														   /* height */
							   GL_FALSE);												   /* fixedsamplelocations */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed for texture object src_to_id");

	gl.bindTexture(GL_TEXTURE_2D, dst_to_id);
	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_R16UI, 16,	 /* width */
					16);			  /* height */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed for texture object dst_to_id");

	/* Create a program object */
	po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

	/* Create two shader objects */
	fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Either of the glCreateShader() calls failed");

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &fbo_draw_id);
	gl.genFramebuffers(1, &fbo_read_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_draw_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_read_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up framebuffer objects");

	/* Set up draw FBO's color attachment. */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, src_to_id,
							0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed");

	/* Set up shader objects.
	 *
	 * NOTE: Original test spec specified an invalid case 3 for the vertex shader.
	 * NOTE: Original test case used a float output value. This would not be valid,
	 *       as the color buffer's attachment uses GL_R16UI format.
	 */
	glw::GLint		   compile_status = GL_FALSE;
	static const char* fs_body		  = "#version 310 es\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "uniform uint  n_sample;\n"
								 "uniform uint  n_max_samples;\n"
								 "uniform highp uvec2 x1y1;\n"
								 "out     uint  result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    uint row_index    = x1y1.y;\n"
								 "    uint column_index = x1y1.x;\n"
								 "\n"
								 "    if (n_sample < n_max_samples / 2u)\n"
								 "    {\n"
								 "        result = row_index * 16u + column_index;\n"
								 "    }\n"
								 "    else\n"
								 "    {\n"
								 "        result = row_index * 16u + column_index + 1u;\n"
								 "    }\n"
								 "}\n";
	static const char* vs_body = "#version 310 es\n"
								 "\n"
								 "uniform uvec2 x1y1;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    switch (gl_VertexID)\n"
								 "    {\n"
								 "        case 0: gl_Position = vec4( (float(x1y1.x) + 0.0 - 8.0)/8.0, (float(x1y1.y) "
								 "+ 0.0 - 8.0)/8.0, 0.0, 1.0); break;\n"
								 "        case 1: gl_Position = vec4( (float(x1y1.x) + 1.0 - 8.0)/8.0, (float(x1y1.y) "
								 "+ 0.0 - 8.0)/8.0, 0.0, 1.0); break;\n"
								 "        case 2: gl_Position = vec4( (float(x1y1.x) + 1.0 - 8.0)/8.0, (float(x1y1.y) "
								 "+ 1.0 - 8.0)/8.0, 0.0, 1.0); break;\n"
								 "        case 3: gl_Position = vec4( (float(x1y1.x) + 0.0 - 8.0)/8.0, (float(x1y1.y) "
								 "+ 1.0 - 8.0)/8.0, 0.0, 1.0); break;\n"
								 "    }\n"
								 "}\n";

	gl.attachShader(po_id, fs_id);
	gl.attachShader(po_id, vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.shaderSource(fs_id, 1 /* count */, &fs_body, NULL);
	gl.shaderSource(vs_id, 1 /* count */, &vs_body, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	gl.compileShader(fs_id);
	gl.compileShader(vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call(s) failed.");

	gl.getShaderiv(fs_id, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Could not compile fragment shader");
	}

	gl.getShaderiv(vs_id, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE)
	{
		/* Retrieve and dump compilation fail reason */
		char		 infoLog[1024];
		glw::GLsizei length = 0;

		gl.getShaderInfoLog(vs_id, 1024, &length, infoLog);

		m_testCtx.getLog() << tcu::TestLog::Message << "Vertex shader compilation failed, infoLog: " << infoLog
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Could not compile vertex shader");
	}

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Could not link program object");
	}

	/* Enable GL_SAMPLE_MASK mode */
	gl.enable(GL_SAMPLE_MASK);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK) call generated an unexpected error");

	/* Prepare for rendering */
	glw::GLint n_max_samples_uniform_location = gl.getUniformLocation(po_id, "n_max_samples");
	glw::GLint n_sample_uniform_location	  = gl.getUniformLocation(po_id, "n_sample");
	glw::GLint x1y1_uniform_location		  = gl.getUniformLocation(po_id, "x1y1");

	if (n_max_samples_uniform_location == -1)
	{
		TCU_FAIL("n_max_samples is not considered an active uniform.");
	}

	if (n_sample_uniform_location == -1)
	{
		TCU_FAIL("n_sample is not considered an active uniform.");
	}

	if (x1y1_uniform_location == -1)
	{
		TCU_FAIL("x1y1 is not considered an active uniform.");
	}

	gl.viewport(0 /* x */, 0 /* y */, 16 /* width */, 16 /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

	gl.useProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	gl.uniform1ui(n_max_samples_uniform_location, r16ui_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1ui() call failed");

	/* Clear color buffer before drawing to it, not strictly neccesary
	 * but helps debugging */
	glw::GLuint clear_color_src[] = { 11, 22, 33, 44 };

	gl.clearBufferuiv(GL_COLOR, 0, clear_color_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferiv() call failed");

	/* Render */
	for (int x = 0; x < 16 /* width */; ++x)
	{
		for (int y = 0; y < 16 /* height */; ++y)
		{
			for (int n_sample = 0; n_sample < r16ui_max_samples; ++n_sample)
			{
				gl.uniform1ui(n_sample_uniform_location, n_sample);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1ui() call failed");

				gl.uniform2ui(x1y1_uniform_location, x, y);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2f() call failed");

				gl.sampleMaski(n_sample / 32, 1 << (n_sample % 32));
				GLU_EXPECT_NO_ERROR(gl.getError(), "glSampleMaski() call failed.");

				gl.drawArrays(GL_TRIANGLE_FAN, 0 /* first */, 4 /* count */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
			} /* for (all n_sample values) */
		}	 /* for (all y values) */
	}		  /* for (all x values) */

	/* Now, configure the framebuffer binding points for the blitting operation */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_to_id, 0); /* level */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, src_to_id,
							0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up framebuffer objects for the blitting operation");

	/* Clear color buffer before drawing to it, not strictly neccesary
	 * but helps debugging */
	glw::GLuint clear_color_dst[] = { 55, 66, 77, 88 };

	gl.clearBufferuiv(GL_COLOR, 0, clear_color_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferiv() call failed");

	/* Blit the data.
	 *
	 * NOTE: Original test spec specified GL_LINEAR filter which would not have worked
	 *       because the read buffer contains integer data.
	 **/
	gl.blitFramebuffer(0,  /* srcX0 */
					   0,  /* srcY0 */
					   16, /* srcX1 */
					   16, /* srcY1 */
					   0,  /* dstX0 */
					   0,  /* dstY0 */
					   16, /* dstX1 */
					   16, /* dstY1 */
					   GL_COLOR_BUFFER_BIT, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBlitFramebuffer() call failed.");

	/* Configure the read framebuffer for upcoming glReadPixels() call */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure the read framebuffer");

	/* Allocate a buffer to hold the result data and then download the result texture's
	 * contents */
	unsigned int* buffer = new unsigned int[16 /* width */ * 16 /* height */ * 4 /* components */];

	gl.readPixels(0,  /* x */
				  0,  /* y */
				  16, /* width */
				  16, /* height */
				  GL_RGBA_INTEGER, GL_UNSIGNED_INT, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	/* Verify the downloaded contents */
	int has_test_fail = 0;
	int pixel_size	= sizeof(unsigned int) * 4 /* components */;
	int row_width	 = pixel_size * 16 /* width */;

	for (unsigned int y = 0; y < 16 /* height */; ++y)
	{
		/* NOTE: Vertical flipping should not be needed, but we cannot confirm this at the moment.
		 *       Should it be the case, please change data_y to 15 - y */
		/* TODO: Remove NOTE above when verified on actual ES3.1 implementation */
		unsigned int		data_y  = y;
		const unsigned int* row_ptr = (unsigned int*)((char*)buffer + data_y * row_width);

		for (unsigned int x = 0; x < 16 /* width */; ++x)
		{
			const unsigned int* data_ptr   = (unsigned int*)((char*)row_ptr + x * pixel_size);
			const unsigned int  r		   = data_ptr[0];
			const unsigned int  g		   = data_ptr[1];
			const unsigned int  b		   = data_ptr[2];
			const unsigned int  a		   = data_ptr[3];
			bool				is_r_valid = (r == (data_y * 16 + x)) || (r == (data_y * 16 + x + 1));
			bool				is_g_valid = (g == 0);
			bool				is_b_valid = (b == 0);
			bool				is_a_valid = (a == 1);

			if (!is_r_valid || !is_g_valid || !is_b_valid || !is_a_valid)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texture data at (" << x << ", " << y << "):"
								   << " Expected (" << (data_y * 16 + x) << ", 0, 0, 1)"
								   << " or (" << (data_y * 16 + x + 1) << ", 0, 0, 1)"
								   << ", found (" << r << ", " << g << ", " << b << ", " << a << ")."
								   << tcu::TestLog::EndMessage;

				has_test_fail++;
			}
		} /* for (all x values) */
	}	 /* for (all y values) */

	if (has_test_fail)
	{
		TCU_FAIL("Invalid texture data");
	}

	/* All done */
	if (buffer != NULL)
	{
		delete[] buffer;
		buffer = NULL;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest::
	MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest(Context& context)
	: TestCase(context, "blitting_to_multisampled_fbo_is_forbidden",
			   "Verifies that blitting to a multisampled framebuffer "
			   "object results in a GL_INVALID_OPERATION error.")
	, dst_to_id(0)
	, fbo_draw_id(0)
	, fbo_read_id(0)
	, src_to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_draw_id);

		fbo_draw_id = 0;
	}

	if (fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_read_id);

		fbo_read_id = 0;
	}

	if (dst_to_id != 0)
	{
		gl.deleteTextures(1, &dst_to_id);

		dst_to_id = 0;
	}

	if (src_to_id != 0)
	{
		gl.deleteTextures(1, &src_to_id);

		src_to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest::iterate()
{
	const std::vector<std::string>& exts = m_context.getContextInfo().getExtensions();
	const bool has_NV_framebuffer_blit   = std::find(exts.begin(), exts.end(), "GL_NV_framebuffer_blit") != exts.end();
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();

	/* Set up texture objects */
	gl.genTextures(1, &dst_to_id);
	gl.genTextures(1, &src_to_id);

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, dst_to_id);
	gl.bindTexture(GL_TEXTURE_2D, src_to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	gl.texStorage2D(GL_TEXTURE_2D, 2, /* levels */
					GL_RGB10_A2, 64,  /* width */
					64);			  /* height */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed for GL_TEXTURE_2D texture target");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGB10_A2, 64,				 /* width */
							   64,							 /* height */
							   GL_FALSE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"gl.texStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &fbo_draw_id);
	gl.genFramebuffers(1, &fbo_read_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_draw_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo_read_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up framebuffer objects");

	/* Set up FBO attachments.
	 *
	 * NOTE: The draw/read FBO configuration in original test spec was the other
	 *       way around which was wrong.
	 */
	gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src_to_id, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up read framebuffer's attachments");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
							dst_to_id, /* texture */
							0);		   /* layer */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up draw framebuffer's attachments");

	/* Try to perform blitting operations. */
	const glw::GLenum  filters[] = { GL_NEAREST, GL_LINEAR };
	const unsigned int n_filters = sizeof(filters) / sizeof(filters[0]);

	for (unsigned int n_filter = 0; n_filter < n_filters; ++n_filter)
	{
		glw::GLenum filter = filters[n_filter];

		// This blit would be supported by NV_framebuffer_blit if sizes match.
		// Alter the size of destination if extension is present to make it invalid.
		int dstY1 = has_NV_framebuffer_blit ? 63 : 64;
		gl.blitFramebuffer(0,	 /* srcX0 */
						   0,	 /* srcY0 */
						   64,	/* srcX1 */
						   64,	/* srcY1 */
						   0,	 /* dstX0 */
						   0,	 /* dstY0 */
						   64,	/* dstX1 */
						   dstY1, /* dstY1 */
						   GL_COLOR_BUFFER_BIT, filter);

		/* Verify GL_INVALID_OPERATION was returned */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid glBlitFramebuffer() call for filter argument ["
							   << filter << "]"
											"should have generated a GL_INVALID_OPERATION error. Instead, "
											"["
							   << error_code << "] error was reported." << tcu::TestLog::EndMessage;

			TCU_FAIL("GL_INVALID_OPERATION was not returned by invalid glBlitFramebuffer() call.");
		}
	} /* for (all valid filter argument values) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::
	MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest(Context& context)
	: TestCase(context, "verify_sample_masking_for_non_integer_color_renderable_internalformats",
			   "Verifies sample masking mechanism for non-integer color-renderable "
			   "internalformats used for 2D multisample textures")
	, bo_id(0)
	, fbo_id(0)
	, fs_draw_id(0)
	, po_draw_id(0)
	, po_verify_id(0)
	, tfo_id(0)
	, to_2d_multisample_id(0)
	, vs_draw_id(0)
	, vs_verify_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unset used program */
	gl.useProgram(0);

	/* Unbind transform feedback object bound to GL_TRANSFORM_FEEDBACK target */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

	/* Unbind buffer object bound to GL_TRANSFORM_FEEDBACK_BUFFER target */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

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

	/* Delete a buffer object of id bo_id */
	if (bo_id != 0)
	{
		gl.deleteBuffers(1, &bo_id);

		bo_id = 0;
	}

	/* Delete a framebuffer object of id fbo_id */
	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	/* Delete a transform feedback object of id tfo_id */
	if (tfo_id != 0)
	{
		gl.deleteTransformFeedbacks(1, &tfo_id);

		tfo_id = 0;
	}

	/* Delete fs_draw_id shader */
	if (fs_draw_id != 0)
	{
		gl.deleteShader(fs_draw_id);

		fs_draw_id = 0;
	}

	/* Delete vs_verify_id shader */
	if (vs_verify_id != 0)
	{
		gl.deleteShader(vs_verify_id);

		vs_verify_id = 0;
	}

	/* Delete vs_draw_id shader */
	if (vs_draw_id != 0)
	{
		gl.deleteShader(vs_draw_id);

		vs_draw_id = 0;
	}

	/* Delete program objects po_verify_id */
	if (po_verify_id != 0)
	{
		gl.deleteProgram(po_verify_id);

		po_verify_id = 0;
	}

	/* Delete program objects po_draw_id */
	if (po_draw_id != 0)
	{
		gl.deleteProgram(po_draw_id);

		po_draw_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes test-specific ES objects */
void MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a 2D multisample texture object of id to_2d_multisample_id */
	gl.genTextures(1, &to_2d_multisample_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	if (to_2d_multisample_id == 0)
	{
		TCU_FAIL("Texture object has not been generated properly");
	}

	/* Generate a buffer object of id bo_id */
	gl.genBuffers(1, &bo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	if (bo_id == 0)
	{
		TCU_FAIL("Buffer object has not been generated properly");
	}

	/* Generate a framebuffer object of id fbo_id */
	gl.genFramebuffers(1, &fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");

	if (fbo_id == 0)
	{
		TCU_FAIL("Framebuffer object has not been generated properly");
	}

	/* Generate a transform feedback object of id tfo_id */
	gl.genTransformFeedbacks(1, &tfo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks() call failed");

	if (tfo_id == 0)
	{
		TCU_FAIL("Transform feedback object has not been generated properly");
	}

	/* Create a vertex shader vs_draw_id */
	vs_draw_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for vs_draw_id");

	/* Create a vertex shader vs_verify_id */
	vs_verify_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for vs_verify_id");

	/* Create a fragment shader fs_draw_id */
	fs_draw_id = gl.createShader(GL_FRAGMENT_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for fs_draw_id");

	/* Create program objects po_draw_id */
	po_draw_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed for po_draw_id");

	/* Create program objects po_verify_id */
	po_verify_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed for po_verify_id");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::
	iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Constants */
	const glw::GLfloat epsilon		  = 1e-5f;
	const glw::GLfloat expected_value = 1.0f;
	const glw::GLchar* fs_draw_body   = "#version 310 es\n"
									  "\n"
									  "precision highp float;\n"
									  "\n"
									  "out vec4 out_color;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    out_color = vec4(1, 1, 1, 1);\n"
									  "}\n";

	const glw::GLchar* vs_draw_body = "#version 310 es\n"
									  "\n"
									  "precision highp float;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    switch (gl_VertexID)\n"
									  "    {\n"
									  "        case 0: gl_Position = vec4(-1,  1, 0, 1); break;\n"
									  "        case 1: gl_Position = vec4( 1,  1, 0, 1); break;\n"
									  "        case 2: gl_Position = vec4( 1, -1, 0, 1); break;\n"
									  "        case 3: gl_Position = vec4(-1, -1, 0, 1); break;\n"
									  "    }\n"
									  "}\n";

	const glw::GLchar* vs_verify_body = "#version 310 es\n"
										"\n"
										"precision highp float;\n"
										"\n"
										"uniform uint              n_bit_on;\n"
										"uniform uint              n_bits;\n"
										"uniform highp sampler2DMS sampler;\n"
										"\n"
										"out float result;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    vec4 one  = vec4(1);\n"
										"    vec4 zero = vec4(0.0, 0.0, 0.0, 1.0);\n"
										"\n"
										"    result = 1.0;\n"
										"\n"
										"    for (uint n_current_bit = 0u; n_current_bit < n_bits; n_current_bit++)\n"
										"    {\n"
										"        vec4 value = texelFetch(sampler, ivec2(0), int(n_current_bit));\n"
										"\n"
										"        if (n_bit_on == n_current_bit)\n"
										"        {\n"
										"            if (any(notEqual(value, one)))\n"
										"            {\n"
										"                result = 0.1 + float(n_current_bit)/1000.0;\n"
										"                break;\n"
										"            }\n"
										"        }\n"
										"        else\n"
										"        {\n"
										"            if (any(notEqual(value, zero)))\n"
										"            {\n"
										"                result = 0.2 + float(n_current_bit)/1000.0;\n"
										"                break;\n"
										"            }\n"
										"        }\n"
										"    }\n"
										"}\n";

	/* Configure the vertex shader vs_draw_id */
	compileShader(vs_draw_id, vs_draw_body);

	/* Configure the vertex shader vs_verify_id */
	compileShader(vs_verify_id, vs_verify_body);

	/* Configure the fragment shader fs_draw_id */
	compileShader(fs_draw_id, fs_draw_body);

	/* Attach the shaders vs_draw_id and fs_draw_id to program object po_draw_id */
	gl.attachShader(po_draw_id, vs_draw_id);
	gl.attachShader(po_draw_id, fs_draw_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure po_draw_id program object");

	/* Attach the shaders vs_verify_id and fs_draw_id to program object po_verify_id */
	gl.attachShader(po_verify_id, vs_verify_id);
	gl.attachShader(po_verify_id, fs_draw_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure po_verify_id program object");

	/* Configure vs_verify_id for transform feedback - our varying of choice is result and we're happy to use either of the TF modes since we'll be outputting a single float anyway. */
	const glw::GLchar* vs_verify_varying_name = "result";

	gl.transformFeedbackVaryings(po_verify_id, 1, &vs_verify_varying_name, GL_SEPARATE_ATTRIBS);

	/* Link the program objects po_draw_id */
	linkProgram(po_draw_id);

	/* Link the program objects po_verify_id */
	linkProgram(po_verify_id);

	/* Retrieve uniform locations */
	glw::GLuint n_bits_location   = gl.getUniformLocation(po_verify_id, "n_bits");
	glw::GLuint n_bit_on_location = gl.getUniformLocation(po_verify_id, "n_bit_on");

	/* Bind the to_2d_multisample_id texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

	/* Bind the fbo_id framebuffer object to GL_DRAW_FRAMEBUFFER target */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Bind the bo_id buffer object to GL_TRANSFORM_FEEDBACK_BUFFER generic target */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, bo_id);

	/* Bind the tfo_id transform feedback object go GL_TRANSFORM_FEEDBACK target */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_id);

	/* Bind the bo_id buffer object to zeroth binding point of GL_TRANSFORM_FEEDBACK_BUFFER */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bo_id);

	/* Initialize buffer object's storage to hold a total of 4 bytes (sizeof(float) );*/
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLfloat), NULL, GL_STATIC_DRAW);

	/* Attach the 2D multisample texture object to the framebuffer object's zeroth color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id,
							0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up zeroth color attachment");

	/* Enable GL_SAMPLE_MASK mode */
	gl.enable(GL_SAMPLE_MASK);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK) call generated an unexpected error");

	/* Color-renderable internalformats to test, note that GL_R8 will not work since sampling from
	 * such format will never return {1,1,1,1} or {0,0,0,1} which current shader uses for sample validation */
	const glw::GLenum internalformat_list[]		= { GL_RGBA8, GL_RGB565, GL_SRGB8_ALPHA8 };
	const int		  internalformat_list_count = sizeof(internalformat_list) / sizeof(internalformat_list[0]);

	/* Get GL_MAX_SAMPLES value */
	glw::GLint gl_max_samples_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLES value");

	if (gl_max_samples_value > 32)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "The test case checks only first 32 samples out of "
						   << gl_max_samples_value << " reported by GL_MAX_SAMPLES." << tcu::TestLog::EndMessage;
	}

	/* Work with no more than 32 bits of mask's first word */
	const glw::GLint mask_bits_to_check = de::min(gl_max_samples_value, 32);

	/* Keep the results but continue running all cases */
	bool test_fail = false;

	/* Iterate through all internal formats test case should check */
	for (int internalformat_index = 0; internalformat_index < internalformat_list_count; internalformat_index++)
	{
		/* Configure the texture object storage */
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, mask_bits_to_check, /* samples */
								   internalformat_list[internalformat_index], 1,  /* width */
								   1,											  /* height */
								   GL_TRUE);									  /* fixedsamplelocations */

		/* Make sure no errors were reported. The framebuffer should be considered complete at this moment. */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");

		/* Following code does not affect test method. Just checks if FBO is complete
		 To catch errors earlier */

		glw::GLenum fbo_completeness_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		if (fbo_completeness_status != GL_FRAMEBUFFER_COMPLETE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Draw FBO completeness status is: " << fbo_completeness_status
							   << ", expected: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

			TCU_FAIL("Draw FBO is considered incomplete which is invalid");
		}

		/* For all values of n_bit from range <0, GL_MAX_SAMPLES pname value) */
		for (int n_bit = 0; n_bit < mask_bits_to_check; n_bit++)
		{

			/* We need to clear render buffer, otherwise masked samples will have undefined values */
			glw::GLfloat clear_color_src[] = { 0.0f, 0.0f, 0.0f, 1.0f };

			gl.clearBufferfv(GL_COLOR, 0, clear_color_src);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clearBufferfv() call failed");

			/* Use program object po_draw_id */
			gl.useProgram(po_draw_id);

			/* Configure sample mask to only render n_bit-th sample using glSampleMaski() function */
			gl.sampleMaski(0, 1 << n_bit);

			/* Draw a triangle fan of 4 vertices. This fills to_2d_id with multisampled data.
			 * However, due to active GL_SAMPLE_MASK and the way we configured it, only one sample should have been rendered.
			 * The only way we can check if this is the case is by using a special shader,
			 * because we have no way of downloading multisampled data to process space in ES3.0+.
			 */
			gl.drawArrays(GL_TRIANGLE_FAN, 0, /* first */
						  4 /* count */);

			/* Unbind fbo_id before sourcing from the texture attached to it */
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			/* Enable GL_RASTERIZER_DISCARD mode */
			gl.enable(GL_RASTERIZER_DISCARD);
			{
				/* Use program object po_verify_id */
				gl.useProgram(po_verify_id);

				/* Specify input arguments for vertex shader */
				gl.uniform1ui(n_bits_location, mask_bits_to_check);
				gl.uniform1ui(n_bit_on_location, n_bit);

				/* Bind to_2d_multisample_id to GL_TEXTURE_2D_MULTISAMPLE texture target. Current texture unit is GL_TEXTURE0 */
				gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

				/* Begin transform feedback (primitiveMode: GL_POINTS) */
				gl.beginTransformFeedback(GL_POINTS);
				{
					/* Draw a single point. This will fill our transform feedback buffer with result */
					gl.drawArrays(GL_POINTS, 0, 1);
				}
				/* End transform feedback */
				gl.endTransformFeedback();
			}
			/* Disable GL_RASTERIZER_DISCARD mode */
			gl.disable(GL_RASTERIZER_DISCARD);

			/* Make sure no errors were generated */
			GLU_EXPECT_NO_ERROR(gl.getError(), "Transform feedback failed");

			/* Rebind the fbo_id framebuffer object to GL_DRAW_FRAMEBUFFER target */
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

			/* Map buffer object bo_id's contents to user-space */
			const glw::GLfloat* mapped_bo =
				(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 4, GL_MAP_READ_BIT);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

			/* Copy result from buffer */
			glw::GLfloat result = *mapped_bo;

			/* Unmap the buffer object */
			gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

			/* Verify the value stored by verification program is not 0. If it is 0, the test has failed. */
			if (de::abs(result - expected_value) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected value stored by verification program: ["
								   << result << "],"
								   << "Format index: " << internalformat_index << ", "
								   << "Bit to check: [" << n_bit << "." << tcu::TestLog::EndMessage;
				/* Notice test failure */
				test_fail = true;
			}
		}

		/* Delete the 2D multisample texture object with glDeleteTextures() call and re-bind the object to a GL_TEXTURE_2D_MULTISAMPLE texture target with a glBindTexture() call */
		gl.deleteTextures(1, &to_2d_multisample_id);

		to_2d_multisample_id = 0;

		/* Recreate to_2d_multisample_id texture object. */
		/* Generate a 2D multisample texture object of id to_2d_multisample_id */
		gl.genTextures(1, &to_2d_multisample_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

		if (to_2d_multisample_id == 0)
		{
			TCU_FAIL("Texture object has not been generated properly");
		}

		/* Bind the to_2d_id texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

		/* Attach the 2D multisample texture object to the framebuffer object's zeroth color attachment */
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
								to_2d_multisample_id, 0); /* level */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up zeroth color attachment");
	}

	if (test_fail)
	{
		TCU_FAIL("Value stored by verification program is not 1.0. Test has failed.");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Links a program object. Shaders should be attached to program id before call.
 *  If an error reported throws an exception.
 *
 *  @param id Program id
 */
void MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::linkProgram(glw::GLuint id)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Link the test program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

	gl.getProgramiv(id, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}
}

/** Compiles the shader. Should the shader not compile, a TestError exception will be thrown.
 *
 *  @param id     Generated shader id
 *  @param source NULL-terminated shader source code string
 */
void MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest::compileShader(
	glw::GLuint id, const glw::GLchar* source)
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();

	gl.shaderSource(id, 1, &source, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	gl.compileShader(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

	gl.getShaderiv(id, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		/* Retrieve adn dump compliation fail reason */
		char		 infoLog[1024];
		glw::GLsizei length = 0;

		gl.getShaderInfoLog(id, 1024, &length, infoLog);

		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed, shader id=" << id
						   << ", infoLog: " << infoLog << tcu::TestLog::EndMessage;

		TCU_FAIL("Shader compilation failed");
	}
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::MultisampleTextureFunctionalTestsSampleMaskingTexturesTest(
	Context& context)
	: TestCase(context, "verify_sample_masking_textures",
			   "Verifies sample masking mechanism for non-integer, integer/unsigned, "
			   "integer/signed color-renderable internalformats and "
			   "depth-renderable internalformats. All internalformats "
			   "are used for 2D multisample textures.")
	, bo_id(0)
	, fbo_id(0)
	, fs_draw_id(0)
	, po_draw_id(0)
	, po_verify_id(0)
	, tfo_id(0)
	, to_2d_multisample_id(0)
	, vs_draw_id(0)
	, vs_verify_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unset used program */
	gl.useProgram(0);

	/* Unbind transform feedback object bound to GL_TRANSFORM_FEEDBACK target */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

	/* Unbind buffer object bound to GL_TRANSFORM_FEEDBACK_BUFFER target */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

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

	/* Delete a buffer object of id bo_id */
	if (bo_id != 0)
	{
		gl.deleteBuffers(1, &bo_id);

		bo_id = 0;
	}

	/* Delete a framebuffer object of id fbo_id */
	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	/* Delete a transform feedback object of id tfo_id */
	if (tfo_id != 0)
	{
		gl.deleteTransformFeedbacks(1, &tfo_id);

		tfo_id = 0;
	}

	/* Delete fs_draw_id shader */
	if (fs_draw_id != 0)
	{
		gl.deleteShader(fs_draw_id);

		fs_draw_id = 0;
	}

	/* Delete vs_verify_id shader */
	if (vs_verify_id != 0)
	{
		gl.deleteShader(vs_verify_id);

		vs_verify_id = 0;
	}

	/* Delete vs_draw_id shader */
	if (vs_draw_id != 0)
	{
		gl.deleteShader(vs_draw_id);

		vs_draw_id = 0;
	}

	/* Delete program objects po_verify_id */
	if (po_verify_id != 0)
	{
		gl.deleteProgram(po_verify_id);

		po_verify_id = 0;
	}

	/* Delete program objects po_draw_id */
	if (po_draw_id != 0)
	{
		gl.deleteProgram(po_draw_id);

		po_draw_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes test-specific ES objects */
void MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a 2D multisample texture object of id to_2d_multisample_id */
	gl.genTextures(1, &to_2d_multisample_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	if (to_2d_multisample_id == 0)
	{
		TCU_FAIL("Texture object has not been generated properly");
	}

	/* Generate a buffer object of id bo_id */
	gl.genBuffers(1, &bo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	if (bo_id == 0)
	{
		TCU_FAIL("Buffer object has not been generated properly");
	}

	/* Generate a framebuffer object of id fbo_id */
	gl.genFramebuffers(1, &fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed");

	if (fbo_id == 0)
	{
		TCU_FAIL("Framebuffer object has not been generated properly");
	}

	/* Generate a transform feedback object of id tfo_id */
	gl.genTransformFeedbacks(1, &tfo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks() call failed");

	if (tfo_id == 0)
	{
		TCU_FAIL("Transform feedback object has not been generated properly");
	}

	/* Create a vertex shader vs_draw_id */
	vs_draw_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for vs_draw_id");

	/* Create a vertex shader vs_verify_id */
	vs_verify_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for vs_verify_id");

	/* Create a fragment shader fs_draw_id */
	fs_draw_id = gl.createShader(GL_FRAGMENT_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for fs_draw_id");

	/* Create program objects po_draw_id */
	po_draw_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed for po_draw_id");

	/* Create program objects po_verify_id */
	po_verify_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed for po_verify_id");
}

/** Executes test iteration.
 *
 *  @return Always STOP.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Constants */
	const glw::GLfloat epsilon		= 1e-5f;
	const glw::GLchar* fs_draw_body = "#version 310 es\n"
									  "\n"
									  "out vec4 out_color;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    out_color = vec4(1, 1, 1, 1);\n"
									  "}\n";

	const glw::GLchar* vs_draw_body = "#version 310 es\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    switch (gl_VertexID)\n"
									  "    {\n"
									  "        case 0: gl_Position = vec4(-1,  1, 0, 1); break;\n"
									  "        case 1: gl_Position = vec4( 1,  1, 0, 1); break;\n"
									  "        case 2: gl_Position = vec4( 1, -1, 0, 1); break;\n"
									  "        case 3: gl_Position = vec4(-1, -1, 0, 1); break;\n"
									  "    }\n"
									  "}\n";

	const glw::GLchar* vs_verify_body = "#version 310 es\n"
										"\n"
										"precision highp float;\n"
										"\n"
										"uniform uint              n_bit_on;\n"
										"uniform uint              n_bits;\n"
										"uniform highp sampler2DMS sampler;\n"
										"\n"
										"out float result;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    vec4 one  = vec4(1);\n"
										"    vec4 zero = vec4(0);\n"
										"\n"
										"    result = 1.0;\n"
										"\n"
										"    for (uint n_current_bit = 0u; n_current_bit < n_bits; n_current_bit++)\n"
										"    {\n"
										"        vec4 value = texelFetch(sampler, ivec2(0), int(n_current_bit));\n"
										"\n"
										"        if (n_bit_on == n_current_bit)\n"
										"        {\n"
										"            if (any(notEqual(value, one)))\n"
										"            {\n"
										"                result = 0.0;\n"
										"            }\n"
										"        }\n"
										"        else\n"
										"        {\n"
										"            if (any(notEqual(value, zero)))\n"
										"            {\n"
										"                result = 0.0;\n"
										"            }\n"
										"        }\n"
										"    }\n"
										"}\n";

	/* Configure the vertex shader vs_draw_id */
	compileShader(vs_draw_id, vs_draw_body);

	/* Configure the vertex shader vs_verify_id */
	compileShader(vs_verify_id, vs_verify_body);

	/* Configure the fragment shader fs_draw_id */
	compileShader(fs_draw_id, fs_draw_body);

	/* Attach the shaders vs_draw_id and fs_draw_id to program object po_draw_id */
	gl.attachShader(po_draw_id, vs_draw_id);
	gl.attachShader(po_draw_id, fs_draw_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure po_draw_id program object");

	/* Attach the shaders vs_verify_id and fs_draw_id to program object po_verify_id */
	gl.attachShader(po_verify_id, vs_verify_id);
	gl.attachShader(po_verify_id, fs_draw_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure po_verify_id program object");

	/* Configure vs_verify_id for transform feedback - our varying of choice is result and we're happy to use either of the TF modes since we'll be outputting a single float anyway. */
	const glw::GLchar* vs_verify_varying_name = "result";

	gl.transformFeedbackVaryings(po_verify_id, 1, &vs_verify_varying_name, GL_SEPARATE_ATTRIBS);

	/* Link the program objects po_draw_id */
	linkProgram(po_draw_id);

	/* Link the program objects po_verify_id */
	linkProgram(po_verify_id);

	/* Retrieve uniform locations */
	glw::GLuint n_bits_location   = gl.getUniformLocation(po_verify_id, "n_bits");
	glw::GLuint n_bit_on_location = gl.getUniformLocation(po_verify_id, "n_bit_on");

	/* Bind the to_2d_multisample_id texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

	/* Bind the fbo_id framebuffer object to GL_DRAW_FRAMEBUFFER target */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Bind the bo_id buffer object to GL_TRANSFORM_FEEDBACK_BUFFER generic target */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, bo_id);

	/* Bind the bo_id buffer object to zeroth binding point of GL_TRANSFORM_FEEDBACK_BUFFER */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bo_id);

	/* Bind the tfo_id transform feedback object go GL_TRANSFORM_FEEDBACK target */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_id);

	/* Initialize buffer object's storage to hold a total of 4 bytes (sizeof(float) );*/
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLfloat), NULL, GL_STATIC_DRAW);

	/* Attach the 2D multisample texture object to the framebuffer object's zeroth color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id,
							0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up zeroth color attachment");

	/* Enable GL_SAMPLE_MASK mode */
	gl.enable(GL_SAMPLE_MASK);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_SAMPLE_MASK) call generated an unexpected error");

	/* Iterate through color-normalized-, color-unsigned-integer-, color-signed-integer- and depth-renderable internalformats */
	const glw::GLenum  normalized_color_internalformats[] = { GL_R8, GL_RGB565, GL_SRGB8_ALPHA8 };
	const glw::GLenum  unsigned_color_internalformats[]   = { GL_RGBA32UI, GL_RG16UI };
	const glw::GLenum  signed_color_internalformats[]	 = { GL_RGBA32I, GL_RG16I };
	const glw::GLenum  depth_internalformats[]			  = { GL_DEPTH_COMPONENT32F };
	const unsigned int n_normalized_color_internalformats =
		sizeof(normalized_color_internalformats) / sizeof(normalized_color_internalformats[0]);
	const unsigned int n_unsigned_color_internalformats =
		sizeof(unsigned_color_internalformats) / sizeof(unsigned_color_internalformats[0]);
	const unsigned int n_signed_color_internalformats =
		sizeof(signed_color_internalformats) / sizeof(signed_color_internalformats[0]);
	const unsigned int n_depth_internalformats = sizeof(depth_internalformats) / sizeof(depth_internalformats[0]);

	for (unsigned int n_iteration = 0; n_iteration < 4 /* normalized/unsigned/signed/depth */; ++n_iteration)
	{
		glw::GLenum		   attachment		 = 0;
		const glw::GLenum* internalformats   = NULL;
		unsigned int	   n_internalformats = 0;

		switch (n_iteration)
		{
		case 0:
		{
			attachment		  = GL_COLOR_ATTACHMENT0;
			internalformats   = normalized_color_internalformats;
			n_internalformats = n_normalized_color_internalformats;

			break;
		}

		case 1:
		{
			attachment		  = GL_COLOR_ATTACHMENT0;
			internalformats   = unsigned_color_internalformats;
			n_internalformats = n_unsigned_color_internalformats;

			break;
		}

		case 2:
		{
			attachment		  = GL_COLOR_ATTACHMENT0;
			internalformats   = signed_color_internalformats;
			n_internalformats = n_signed_color_internalformats;

			break;
		}

		case 3:
		{
			attachment		  = GL_DEPTH_ATTACHMENT;
			internalformats   = depth_internalformats;
			n_internalformats = n_depth_internalformats;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized iteration index");
		}
		} /* switch (n_iteration) */

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

			if (internalformat_max_samples > 32)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "The test case checks only first 32 samples out of "
								   << internalformat_max_samples
								   << " reported by GL_SAMPLES reported by getInternalformativ() "
								   << "for internalformat " << internalformat << "." << tcu::TestLog::EndMessage;
			}

			/* Work with no more than 32 bits of mask's first word */
			const glw::GLint mask_bits_to_check = de::min(internalformat_max_samples, 32);

			/* Recreate to_2d_multisample_id texture object. */
			if (to_2d_multisample_id == 0)
			{
				/* Generate a 2D multisample texture object of id to_2d_multisample_id */
				gl.genTextures(1, &to_2d_multisample_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

				if (to_2d_multisample_id == 0)
				{
					TCU_FAIL("Texture object has not been generated properly");
				}

				/* Bind the to_2d_id texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
				gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

				/* Attach the 2D multisample texture object to the framebuffer object's attachment */
				gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE,
										to_2d_multisample_id, 0); /* level */

				GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up attachment");
			}

			/* Configure the texture object storage */
			gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, internalformat_max_samples, /* samples */
									   internalformat, 1,									  /* width */
									   1,													  /* height */
									   GL_TRUE);											  /* fixedsamplelocations */

			/* Make sure no errors were reported. The framebuffer should be considered complete at this moment. */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");

			/* For all values of n_bit from range <0, GL_MAX_SAMPLES pname value) */
			for (int n_bit = 0; n_bit < mask_bits_to_check; n_bit++)
			{
				/* Use program object po_draw_id */
				gl.useProgram(po_draw_id);

				/* Configure sample mask to only render n_bit-th sample using glSampleMaski() function */
				gl.sampleMaski(0, 1 << n_bit);

				/* Draw a triangle fan of 4 vertices. This fills to_2d_id with multisampled data.
				 * However, due to active GL_SAMPLE_MASK and the way we configured it, only one sample should have been rendered.
				 * The only way we can check if this is the case is by using a special shader,
				 * because we have no way of downloading multisampled data to process space in ES3.0+.
				 */
				gl.drawArrays(GL_TRIANGLE_FAN, 0, /* first */
							  4 /* count */);

				/* Enable GL_RASTERIZER_DISCARD mode */
				gl.enable(GL_RASTERIZER_DISCARD);
				{
					/* Use program object po_verify_id */
					gl.useProgram(po_verify_id);

					/* Specify input arguments for vertex shader */
					gl.uniform1i(n_bits_location, mask_bits_to_check);
					gl.uniform1i(n_bit_on_location, n_bit);

					/* Bind to_2d_multisample_id to GL_TEXTURE_2D_MULTISAMPLE texture target. Current texture unit is GL_TEXTURE0 */
					gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

					/* Begin transform feedback (primitiveMode: GL_POINTS) */
					gl.beginTransformFeedback(GL_POINTS);
					{
						/* Draw a single point. This will fill our transform feedback buffer with result */
						gl.drawArrays(GL_POINTS, 0, 1);
					}
					/* End transform feedback */
					gl.endTransformFeedback();
				}
				/* Disable GL_RASTERIZER_DISCARD mode */
				gl.disable(GL_RASTERIZER_DISCARD);

				/* Make sure no errors were generated */
				GLU_EXPECT_NO_ERROR(gl.getError(), "Transform feedback failed");

				/* Map buffer object bo_id's contents to user-space */
				const glw::GLfloat* mapped_bo = (glw::GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed");

				/* Copy result from buffer */
				glw::GLfloat result = *mapped_bo;

				/* Unmap the buffer object */
				gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

				/* Verify the value stored by verification program is not 0. If it is 0, the test has failed. */
				if (de::abs(result) < epsilon)
				{
					TCU_FAIL("Value stored by verification program is zero. Test has failed.");
				}
			}

			/* Delete the 2D multisample texture object with glDeleteTextures() call and re-bind the object to a GL_TEXTURE_2D_MULTISAMPLE texture target with a glBindTexture() call */
			gl.deleteTextures(1, &to_2d_multisample_id);

			to_2d_multisample_id = 0;
		} /* for (all renderable internalformats) */
	}	 /* for (all iterations) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Links a program object. Shaders should be attached to program id before call.
 *  If an error reported throws an exception.
 *
 *  @param id Program id
 */
void MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::linkProgram(glw::GLuint id)
{
	/* Link the test program object */
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status = GL_FALSE;

	gl.linkProgram(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

	gl.getProgramiv(id, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}
}

/** Compiles the shader. Should the shader not compile, a TestError exception will be thrown.
 *
 *  @param id     Generated shader id
 *  @param source NULL-terminated shader source code string
 */
void MultisampleTextureFunctionalTestsSampleMaskingTexturesTest::compileShader(glw::GLuint		  id,
																			   const glw::GLchar* source)
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();

	gl.shaderSource(id, 1, &source, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	gl.compileShader(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

	gl.getShaderiv(id, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Shader compilation failed");
	}
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest::
	MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest(Context& context)
	: TestCase(context, "texture_size_in_fragment_shaders",
			   "Verifies textureSize() works for multisample textures when used in fragment shaders")
	, fbo_id(0)
	, fs_id(0)
	, po_id(0)
	, to_2d_multisample_id(0)
	, to_2d_multisample_array_id(0)
	, vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);

		fs_id = 0;
	}

	if (po_id != 0)
	{
		gl.deleteProgram(po_id);

		po_id = 0;
	}

	if (to_2d_multisample_id != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_id);

		to_2d_multisample_id = 0;
	}

	if (to_2d_multisample_array_id != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_array_id);

		to_2d_multisample_array_id = 0;
	}

	if (vs_id != 0)
	{
		gl.deleteShader(vs_id);

		vs_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest::iterate()
{
	bool are_2d_array_ms_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up texture objects */
	if (are_2d_array_ms_tos_supported)
	{
		gl.genTextures(1, &to_2d_multisample_array_id);
	}

	gl.genTextures(1, &to_2d_multisample_id);

	if (are_2d_array_ms_tos_supported)
	{
		gl.activeTexture(GL_TEXTURE0);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_2d_multisample_array_id);
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGBA8, 16,						   /* width */
								   32,									   /* height */
								   8,									   /* depth */
								   GL_TRUE);							   /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up 2D multisample array texture storage");
	}

	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 16,				 /* width */
							   32,							 /* height */
							   GL_TRUE);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up 2D multisample texture storage");

	/* Set up a fragment shader */
	static const char* fs_body =
		"#version 310 es\n"
		"\n"
		"#ifdef GL_OES_texture_storage_multisample_2d_array\n"
		"    #extension GL_OES_texture_storage_multisample_2d_array : enable\n"
		"#endif\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"uniform highp sampler2DMS sampler_2d;\n"
		"\n"
		"#ifdef GL_OES_texture_storage_multisample_2d_array\n"
		"    uniform highp sampler2DMSArray sampler_2d_array;\n"
		"#endif"
		"\n"
		"out vec4 result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    #ifdef GL_OES_texture_storage_multisample_2d_array\n"
		"        ivec3 sampler_2d_array_size = textureSize(sampler_2d_array);\n"
		"    #else\n"
		"        ivec3 sampler_2d_array_size = ivec3(16, 32, 8);\n"
		"    #endif\n"
		"\n"
		"    ivec2 sampler_2d_size = textureSize(sampler_2d);\n"
		"\n"
		"    if (sampler_2d_size.x       == 16 && sampler_2d_size.y       == 32 &&\n"
		"        sampler_2d_array_size.x == 16 && sampler_2d_array_size.y == 32 && sampler_2d_array_size.z == 8)\n"
		"    {\n"
		"        result = vec4(0, 1, 0, 0);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        result = vec4(1, 0, 0, 0);\n"
		"    }\n"
		"}\n";
	glw::GLint compile_status = GL_FALSE;

	fs_id = gl.createShader(GL_FRAGMENT_SHADER);

	gl.shaderSource(fs_id, 1 /* count */, &fs_body, NULL);
	gl.compileShader(fs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a fragment shader");

	gl.getShaderiv(fs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query fragment shader's compile status");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Could not compile fragment shader");
	}

	/* Set up a vertex shader for 2D multisample texture case */
	static const char* vs_body = "#version 310 es\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "   switch (gl_VertexID)\n"
								 "   {\n"
								 "       case 0: gl_Position = vec4(-1, -1, 0, 1); break;\n"
								 "       case 1: gl_Position = vec4(-1,  1, 0, 1); break;\n"
								 "       case 2: gl_Position = vec4( 1,  1, 0, 1); break;\n"
								 "       case 3: gl_Position = vec4( 1, -1, 0, 1); break;\n"
								 "   }\n"
								 "}\n";

	vs_id = gl.createShader(GL_VERTEX_SHADER);

	gl.shaderSource(vs_id, 1 /* count */, &vs_body, NULL);
	gl.compileShader(vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a vertex shader");

	gl.getShaderiv(vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query vertex shader's compile status");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Could not compile vertex shader");
	}

	/* Set up a program object */
	po_id = gl.createProgram();

	gl.attachShader(po_id, fs_id);
	gl.attachShader(po_id, vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a program object");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program object linking failed");
	}

	/* Set up uniforms */
	glw::GLint sampler_2d_array_location = gl.getUniformLocation(po_id, "sampler_2d_array");
	glw::GLint sampler_2d_location		 = gl.getUniformLocation(po_id, "sampler_2d");

	if ((sampler_2d_array_location == -1 && are_2d_array_ms_tos_supported) || sampler_2d_location == -1)
	{
		TCU_FAIL("At least one of the required uniforms is not considered active");
	}

	gl.useProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.uniform1i(sampler_2d_array_location, 0);
	gl.uniform1i(sampler_2d_location, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call(s) failed.");

	/* Render a full-screen quad */
	gl.drawArrays(GL_TRIANGLE_FAN, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed");

	/* Verify the results */
	const tcu::RenderTarget& rt = m_context.getRenderContext().getRenderTarget();

	unsigned char* buffer		 = NULL;
	unsigned char* data_ptr		 = NULL;
	int			   rt_bits_alpha = 0;
	int			   rt_bits_blue  = 0;
	int			   rt_bits_green = 0;
	int			   rt_bits_red   = 0;
	int			   rt_height	 = rt.getHeight();
	int			   rt_width		 = rt.getWidth();
	const int	  row_width	 = 4 /* RGBA */ * rt_width;

	gl.getIntegerv(GL_ALPHA_BITS, &rt_bits_alpha);
	gl.getIntegerv(GL_BLUE_BITS, &rt_bits_blue);
	gl.getIntegerv(GL_GREEN_BITS, &rt_bits_green);
	gl.getIntegerv(GL_RED_BITS, &rt_bits_red);

	buffer = new unsigned char[rt_height * rt_width * 4];

	gl.readPixels(0, 0, rt_width, rt_height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed");

	data_ptr = buffer + row_width * (rt_height / 2) + rt_width / 2 * 4;

	if (((data_ptr[0] != 0) && (rt_bits_red != 0)) || ((data_ptr[1] != 255) && (rt_bits_green != 0)) ||
		((data_ptr[2] != 0) && (rt_bits_blue != 0)) || ((data_ptr[3] != 0) && (rt_bits_alpha != 0)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data rendered: expected:(0, 255, 0, 0) rendered:"
						   << "(" << data_ptr[0] << ", " << data_ptr[1] << ", " << data_ptr[2] << ", " << data_ptr[3]
						   << ")" << tcu::TestLog::EndMessage;

		delete[] buffer;
		buffer = NULL;

		TCU_FAIL("Invalid data rendered");
	}

	if (buffer != NULL)
	{
		delete[] buffer;
		buffer = NULL;
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest::
	MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest(Context& context)
	: TestCase(context, "texture_size_in_vertex_shaders",
			   "Verifies textureSize() works for multisample textures when used in vertex shaders")
	, bo_id(0)
	, fbo_id(0)
	, fs_id(0)
	, po_id(0)
	, tfo_id(0)
	, to_2d_multisample_id(0)
	, to_2d_multisample_array_id(0)
	, vs_2d_array_id(0)
	, vs_2d_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (bo_id != 0)
	{
		gl.deleteBuffers(1, &bo_id);

		bo_id = 0;
	}

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (fs_id != 0)
	{
		gl.deleteShader(fs_id);

		fs_id = 0;
	}

	if (po_id != 0)
	{
		gl.deleteProgram(po_id);

		po_id = 0;
	}

	if (tfo_id != 0)
	{
		gl.deleteTransformFeedbacks(1, &tfo_id);

		tfo_id = 0;
	}

	if (to_2d_multisample_id != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_id);

		to_2d_multisample_id = 0;
	}

	if (to_2d_multisample_array_id != 0)
	{
		gl.deleteTextures(1, &to_2d_multisample_array_id);

		to_2d_multisample_array_id = 0;
	}

	if (vs_2d_id != 0)
	{
		gl.deleteShader(vs_2d_id);

		vs_2d_id = 0;
	}

	if (vs_2d_array_id != 0)
	{
		gl.deleteShader(vs_2d_array_id);

		vs_2d_array_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest::iterate()
{
	bool are_multisample_2d_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up a TFO */
	gl.genTransformFeedbacks(1, &tfo_id);
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a transform feedback object");

	/* Set up a buffer object */
	gl.genBuffers(1, &bo_id);
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, bo_id);
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(float), NULL, GL_STATIC_READ);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a buffer object");

	/* Set up texture objects */
	if (are_multisample_2d_array_tos_supported)
	{
		gl.genTextures(1, &to_2d_multisample_array_id);
	}

	gl.genTextures(1, &to_2d_multisample_id);

	/* NOTE: Since we're binding the textures to zero texture unit,
	 *       we don't need to do glUniform1i() calls to configure
	 *       the texture samplers in the vertex shaders later on.
	 */
	if (are_multisample_2d_array_tos_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_2d_multisample_array_id);
	}

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_2d_multisample_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 16,				 /* width */
							   32,							 /* height */
							   GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glTexStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

	if (are_multisample_2d_array_tos_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGBA8, 16,						   /* width */
								   32,									   /* height */
								   8,									   /* depth */
								   GL_TRUE);							   /* fixedsamplelocations */
		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"gltexStorage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");
	}

	/* Set up a fragment shader */
	glw::GLint		   compile_status = GL_FALSE;
	static const char* fs_body		  = "#version 310 es\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "}\n";

	fs_id = gl.createShader(GL_FRAGMENT_SHADER);

	gl.shaderSource(fs_id, 1 /* count */, &fs_body, NULL);
	gl.compileShader(fs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a fragment shader");

	gl.getShaderiv(fs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query fragment shader's compile status");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Could not compile fragment shader");
	}

	/* Set up a vertex shader for 2D multisample texture case */
	static const char* vs_2d_body = "#version 310 es\n"
									"\n"
									"precision highp float;\n"
									"\n"
									"uniform highp sampler2DMS sampler;\n"
									"out           float       is_size_correct;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    ivec2 size = textureSize(sampler);\n"
									"\n"
									"    if (size.x == 16 && size.y == 32)\n"
									"    {\n"
									"        is_size_correct = 1.0f;\n"
									"    }\n"
									"    else\n"
									"    {\n"
									"        is_size_correct = 0.0f;\n"
									"    }\n"
									"}\n";

	vs_2d_id = gl.createShader(GL_VERTEX_SHADER);

	gl.shaderSource(vs_2d_id, 1 /* count */, &vs_2d_body, NULL);
	gl.compileShader(vs_2d_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a vertex shader for 2D multisample texture case");

	gl.getShaderiv(vs_2d_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query 2D multisample texture vertex shader's compile status");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Could not compile vertex shader for 2D multisample texture");
	}

	/* Set up a vertex shader for 2D multisample array texture case */
	if (are_multisample_2d_array_tos_supported)
	{
		static const char* vs_2d_array_body = "#version 310 es\n"
											  "\n"
											  "#extension GL_OES_texture_storage_multisample_2d_array : enable\n"
											  "precision highp float;\n"
											  "\n"
											  "uniform highp sampler2DMSArray sampler;\n"
											  "out           float            is_size_correct;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    ivec3 size = textureSize(sampler);\n"
											  "\n"
											  "    if (size.x == 16 && size.y == 32 && size.z == 8)\n"
											  "    {\n"
											  "        is_size_correct = 1.0f;\n"
											  "    }\n"
											  "    else\n"
											  "    {\n"
											  "        is_size_correct = 0.0f;\n"
											  "    }\n"
											  "}\n";

		vs_2d_array_id = gl.createShader(GL_VERTEX_SHADER);

		gl.shaderSource(vs_2d_array_id, 1 /* count */, &vs_2d_array_body, NULL);
		gl.compileShader(vs_2d_array_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a vertex shader for 2D multisample array texture case");

		gl.getShaderiv(vs_2d_array_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not query 2D multisample array texture vertex shader's compile status");

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("Could not compile vertex shader for 2D multisample array texture");
		}
	}

	/* Execute two iterations:
	 *
	 * a) Create a program object using fs and vs_2d shaders;
	 * b) Create a program object using fs and vs_2d_array shaders.
	 *
	 * Case b) should only be executed if 2D Array MS textures are
	 * supported.
	 */
	for (int n_iteration = 0; n_iteration < 2 /* iterations */; ++n_iteration)
	{
		if (n_iteration == 1 && !are_multisample_2d_array_tos_supported)
		{
			/* Skip the iteration */
			continue;
		}

		if (po_id != 0)
		{
			gl.deleteProgram(po_id);

			po_id = 0;
		}

		po_id = gl.createProgram();

		/* Attach iteration-specific shaders */
		switch (n_iteration)
		{
		case 0:
		{
			gl.attachShader(po_id, fs_id);
			gl.attachShader(po_id, vs_2d_id);

			break;
		}

		case 1:
		{
			gl.attachShader(po_id, fs_id);
			gl.attachShader(po_id, vs_2d_array_id);

			break;
		}

		default:
			TCU_FAIL("Unrecognized iteration index");
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

		/* Configure the program object for XFB */
		const char* varying_name = "is_size_correct";

		gl.transformFeedbackVaryings(po_id, 1 /* count */, &varying_name, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

		/* Link the program object */
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

		if (link_status != GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Linking failed for program object in iteration "
							   << n_iteration << tcu::TestLog::EndMessage;

			TCU_FAIL("Program object linking failed");
		}

		/* Render a point using the program */
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

		gl.useProgram(po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		gl.beginTransformFeedback(GL_POINTS);
		{
			gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		}
		gl.endTransformFeedback();

		GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed");

		/* Read the captured data. Reset the contents of the BO before the buffer
		 * object is unmapped.
		 */
		void*		data_ptr = NULL;
		const float epsilon  = (float)1e-5;
		float		result   = 0.0f;

		data_ptr = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, sizeof(float) /* size */,
									 GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

		result = *((const float*)data_ptr);
		memset(data_ptr, 0, sizeof(float));

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

		if (de::abs(result - 1.0f) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Retrieved value: " << result << ", expected: 1.0"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported.");
		}
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
