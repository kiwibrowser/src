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
 * \file  es31cTextureStorageMultisampleDependenciesTests.cpp
 * \brief Implements conformance tests that verify dependencies of
 *        multisample textures on other parts of core ES3.1 API
 *        (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleDependenciesTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <memory.h>
#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesFBOIncompleteness1Test::MultisampleTextureDependenciesFBOIncompleteness1Test(
	Context& context)
	: TestCase(context, "fbo_with_attachments_of_varying_amount_of_samples",
			   "FBOs with multisample texture attachments, whose amount"
			   " of samples differs between attachments, should be "
			   "considered incomplete")
	, fbo_id(0)
	, to_id_multisample_2d_array(0)
{
	memset(to_ids_multisample_2d, 0, sizeof(to_ids_multisample_2d));
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesFBOIncompleteness1Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_ids_multisample_2d[0] != 0)
	{
		gl.deleteTextures(1, to_ids_multisample_2d + 0);

		to_ids_multisample_2d[0] = 0;
	}

	if (to_ids_multisample_2d[1] != 0)
	{
		gl.deleteTextures(1, to_ids_multisample_2d + 1);

		to_ids_multisample_2d[1] = 0;
	}

	if (to_id_multisample_2d_array != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d_array);

		to_id_multisample_2d_array = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesFBOIncompleteness1Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	bool				  are_2d_ms_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");

	/* Set up texture objects */
	gl.genTextures(1, to_ids_multisample_2d);

	if (are_2d_ms_array_tos_supported)
	{
		gl.genTextures(1, &to_id_multisample_2d_array);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_multisample_2d_array);
	}
	else
	{
		gl.genTextures(1, to_ids_multisample_2d + 1);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	/* Query possible sample count values for both texture targets
	 and format used in test */
	glw::GLint num_sample_counts_2dms   = 1;
	glw::GLint num_sample_counts_2dms_a = 1;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &num_sample_counts_2dms);

	if (are_2d_ms_array_tos_supported)
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1,
							   &num_sample_counts_2dms_a);
	}

	std::vector<glw::GLint> sample_counts_2dms(num_sample_counts_2dms);
	std::vector<glw::GLint> sample_counts_2dms_a(num_sample_counts_2dms_a);

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, num_sample_counts_2dms,
						   &sample_counts_2dms[0]);

	if (are_2d_ms_array_tos_supported)
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, GL_RGBA8, GL_SAMPLES, num_sample_counts_2dms_a,
							   &sample_counts_2dms_a[0]);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set query internal formats");

	/* This will store actual sample counts to be used in test */
	glw::GLint samples_attachment_1 = 0;
	glw::GLint samples_attachment_2 = 0;

	/* Choose two different sample counts, supported by implementation */
	if (are_2d_ms_array_tos_supported)
	{
		for (glw::GLint i_2dms = 0; i_2dms < num_sample_counts_2dms; i_2dms++)
		{
			for (glw::GLint i_2dms_a = 0; i_2dms_a < num_sample_counts_2dms_a; i_2dms_a++)
			{
				if (sample_counts_2dms[i_2dms] != sample_counts_2dms_a[i_2dms_a] && sample_counts_2dms[i_2dms] != 1 &&
					sample_counts_2dms_a[i_2dms_a] != 1)
				{
					/* found two differing non-1 sample counts ! */
					samples_attachment_1 = sample_counts_2dms[i_2dms];
					samples_attachment_2 = sample_counts_2dms_a[i_2dms_a];
				}
			}
		}
	} /* if (are_2d_ms_array_tos_supported) */
	else
	{
		for (glw::GLuint index = 1; index < sample_counts_2dms.size(); ++index)
		{
			if (sample_counts_2dms[index - 1] != 1 && sample_counts_2dms[index] != 1 &&
				sample_counts_2dms[index - 1] != sample_counts_2dms[index])
			{
				samples_attachment_1 = sample_counts_2dms[index - 1];
				samples_attachment_2 = sample_counts_2dms[index];

				break;
			}
		}
	}

	if (samples_attachment_1 == 0 || samples_attachment_2 == 0)
	{
		/* It may be the case implementation support only one
		 sample count on both targets with used format.

		 In such case cannot perform the test - cannot make
		 FBO incomplete due to sample count mismatch
		 */
		m_testCtx.setTestResult(
			QP_TEST_RESULT_NOT_SUPPORTED,
			"Can't test incomplete FBO due to mismatch sample count: only 1 sample count available");

		return STOP;
	}

	for (int n_texture_2d = 0; n_texture_2d < (are_2d_ms_array_tos_supported ? 1 : 2); ++n_texture_2d)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_ids_multisample_2d[n_texture_2d]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
								   (n_texture_2d == 0) ? samples_attachment_1 : samples_attachment_2, GL_RGBA8,
								   2,		  /* width */
								   2,		  /* height */
								   GL_FALSE); /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"glTexStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");
	}

	if (are_2d_ms_array_tos_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples_attachment_2, /* samples */
								   GL_RGBA8, 2,												  /* width */
								   2,														  /* height */
								   2,														  /* depth */
								   GL_FALSE);												  /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"gltexStorage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");
	}

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Set up FBO attachments */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
							to_ids_multisample_2d[0], 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up zeroth color attachment");

	if (are_2d_ms_array_tos_supported)
	{
		gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to_id_multisample_2d_array, 0, /* level */
								   0);																		 /* layer */
	}
	else
	{
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE,
								to_ids_multisample_2d[1], 0); /* level */
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up first color attachment");

	/* Make sure the draw framebuffer is considered incomplete */
	glw::GLenum fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_status != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Draw framebuffer's completeness status is: " << fbo_status
						   << "as opposed to expected status: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesFBOIncompleteness2Test::MultisampleTextureDependenciesFBOIncompleteness2Test(
	Context& context)
	: TestCase(context, "fbo_with_single_and_multisample_attachments",
			   "FBOs with multisample texture and normal 2D texture attachments "
			   "should be considered incomplete")
	, fbo_id(0)
	, to_id_2d(0)
	, to_id_multisample_2d(0)
	, to_id_multisample_2d_array(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesFBOIncompleteness2Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id_2d != 0)
	{
		gl.deleteTextures(1, &to_id_2d);

		to_id_2d = 0;
	}

	if (to_id_multisample_2d != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d);

		to_id_multisample_2d = 0;
	}

	if (to_id_multisample_2d_array != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d_array);

		to_id_multisample_2d_array = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesFBOIncompleteness2Test::iterate()
{
	bool are_2d_ms_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up texture objects */
	gl.genTextures(1, &to_id_2d);
	gl.genTextures(1, &to_id_multisample_2d);

	if (are_2d_ms_array_tos_supported)
	{
		gl.genTextures(1, &to_id_multisample_2d_array);
	}

	gl.bindTexture(GL_TEXTURE_2D, to_id_2d);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d);

	if (are_2d_ms_array_tos_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_multisample_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	gl.texImage2D(GL_TEXTURE_2D, 0, /* level */
				  GL_RGB565, 2,		/* width */
				  2,				/* height */
				  0,				/* border */
				  GL_RGB,			/* format */
				  GL_UNSIGNED_BYTE, /* type */
				  NULL);			/* pixels */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D() call failed for GL_TEXTURE_2D texture target");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGB565, 2,				 /* width */
							   2,							 /* height */
							   GL_FALSE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glTexStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

	if (are_2d_ms_array_tos_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGB565, 2,						   /* width */
								   2,									   /* height */
								   2,									   /* depth */
								   GL_FALSE);							   /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"gltexStorage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");
	}

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Set up FBO attachments */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to_id_2d, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up zeroth color attachment");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d,
							0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up first color attachment");

	/* Make sure the draw framebuffer is considered incomplete */
	glw::GLenum fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_status != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Draw framebuffer's completeness status is: " << fbo_status
						   << "as opposed to expected status: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* Detach the first color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, 0, /* texture */
							0);																		 /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach first color attachment from the draw FBO");

	/* Verify the FBO is now considered complete */
	fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Draw framebuffer's completeness status is: " << fbo_status
						   << "as opposed to expected status: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	if (are_2d_ms_array_tos_supported)
	{
		/* Attach the arrayed multisample texture object */
		gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to_id_multisample_2d_array, 0, /* level */
								   0);																		 /* layer */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up first color attachment");

		/* Make sure the draw framebuffer is considered incomplete */
		fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		if (fbo_status != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Draw framebuffer's completeness status is: " << fbo_status
							   << "as opposed to expected status: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FBO completeness status reported.");
		}
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesFBOIncompleteness3Test::MultisampleTextureDependenciesFBOIncompleteness3Test(
	Context& context)
	: TestCase(context, "fbo_with_fixed_and_varying_sample_locations_attachments",
			   "FBOs with multisample texture attachments of different fixed "
			   "sample location settings should be considered incomplete")
	, fbo_id(0)
	, to_id_2d_multisample_color_1(0)
	, to_id_2d_multisample_color_2(0)
	, to_id_2d_multisample_depth(0)
	, to_id_2d_multisample_depth_stencil(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesFBOIncompleteness3Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id_2d_multisample_color_1 != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_color_1);

		to_id_2d_multisample_color_1 = 0;
	}

	if (to_id_2d_multisample_color_2 != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_color_2);

		to_id_2d_multisample_color_2 = 0;
	}

	if (to_id_2d_multisample_depth != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_depth);

		to_id_2d_multisample_depth = 0;
	}

	if (to_id_2d_multisample_depth_stencil != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_depth_stencil);

		to_id_2d_multisample_depth_stencil = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesFBOIncompleteness3Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Only execute if GL_MAX_SAMPLES pname value >= 2 */
	glw::GLint gl_max_samples_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() generated an error for GL_MAX_SAMPLES pname");

	if (gl_max_samples_value < 2)
	{
		throw tcu::NotSupportedError("GL_MAX_SAMPLES pname value < 2, skipping");
	}

	/* Only execute if GL_RGBA8, GL_DEPTH_COMPONENT16, GL_DEPTH24_STENCIL8 internalformats
	 * can be rendered to with at least 2 samples per fragment.
	 */
	glw::GLint depth_component16_internalformat_max_samples = 0;
	glw::GLint depth24_stencil8_internalformat_max_samples  = 0;
	glw::GLint rgba8_internalformat_max_samples				= 0;

	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_DEPTH_COMPONENT16, GL_SAMPLES, 1, /* bufSize */
						   &depth_component16_internalformat_max_samples);
	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_DEPTH24_STENCIL8, GL_SAMPLES, 1, /* bufSize */
						   &depth24_stencil8_internalformat_max_samples);
	gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, 1, /* bufSize */
						   &rgba8_internalformat_max_samples);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed for at least one GL_SAMPLES query");

	if (depth_component16_internalformat_max_samples < 2)
	{
		throw tcu::NotSupportedError("GL_SAMPLES is lower than 2 for GL_DEPTH_COMPONENT16 internalformat");
	}

	if (depth24_stencil8_internalformat_max_samples < 2)
	{
		throw tcu::NotSupportedError("GL_SAMPLES is lower than 2 for GL_DEPTH24_STENCIL8 internalformat");
	}

	if (rgba8_internalformat_max_samples < 2)
	{
		throw tcu::NotSupportedError("GL_SAMPLES is lower than 2 for GL_RGBA8 internalformat");
	}

	/* Set up texture objects */
	gl.genTextures(1, &to_id_2d_multisample_color_1);
	gl.genTextures(1, &to_id_2d_multisample_color_2);
	gl.genTextures(1, &to_id_2d_multisample_depth);
	gl.genTextures(1, &to_id_2d_multisample_depth_stencil);

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGenTextures() call failed");

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Set up first GL_RGBA8 multisample texture storage */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample_color_1);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 2,					 /* width */
							   2,							 /* height */
							   GL_FALSE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up first GL_RGBA8 multisample texture storage.");

	/* Set up second GL_RGBA8 multisample texture storage */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample_color_2);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 2,					 /* width */
							   2,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up second GL_RGBA8 multisample texture storage.");

	/* Set up GL_DEPTH_COMPONENT16 multisample texture storage */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample_depth);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_DEPTH_COMPONENT16, 2,		 /* width */
							   2,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up GL_DEPTH_COMPONENT16 multisample texture storage.");

	/* Set up GL_DEPTH24_STENCIL8 multisample texture storage */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample_depth_stencil);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_DEPTH24_STENCIL8, 2,		 /* width */
							   2,							 /* height */
							   GL_TRUE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up GL_DEPTH24_STENCIL8 multisample texture storage.");

	/* Set up FBO's zeroth color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
							to_id_2d_multisample_color_1, 0); /* level */

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE,
							to_id_2d_multisample_color_2, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up FBO color attachments");

	/* FBO should now be considered incomplete */
	glw::GLenum fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid FBO completeness status reported:" << fbo_completeness
						   << " expected: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* Detach the first color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, /* texture */
							0);															 /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach FBO's first color attachment");

	/* Configure FBO's depth attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE,
							to_id_2d_multisample_depth, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure FBO's depth attachment");

	/* FBO should now be considered incomplete */
	fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid FBO completeness status reported:" << fbo_completeness
						   << " expected: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* Detach depth attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, /* texture */
							0);															/* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach FBO's depth attachment");

	/* Configure FBO's depth+stencil attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE,
							to_id_2d_multisample_depth_stencil, 0); /* level */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure FBO's depth+stencil attachment");

	/* FBO should now be considered incomplete */
	fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid FBO completeness status reported:" << fbo_completeness
						   << " expected: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesFBOIncompleteness4Test::MultisampleTextureDependenciesFBOIncompleteness4Test(
	Context& context)
	: TestCase(context, "fbo_with_different_fixedsamplelocations_texture_and_renderbuffer_attachments",
			   "FBOs with multisample texture attachments of different 'fixed sample location' "
			   "settings and with multisampled renderbuffers (of the same amount of samples)"
			   "should be considered incomplete")
	, fbo_id(0)
	, rbo_id(0)
	, to_id_2d_multisample_array_color(0)
	, to_id_2d_multisample_color(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesFBOIncompleteness4Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (rbo_id != 0)
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
		gl.deleteRenderbuffers(1, &rbo_id);
	}

	if (to_id_2d_multisample_color != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_color);

		to_id_2d_multisample_color = 0;
	}

	if (to_id_2d_multisample_array_color != 0)
	{
		gl.deleteTextures(1, &to_id_2d_multisample_array_color);

		to_id_2d_multisample_array_color = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesFBOIncompleteness4Test::iterate()
{
	bool are_2d_ms_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Only execute if GL_MAX_SAMPLES pname value >= 3 */
	glw::GLint gl_max_samples_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() generated an error for GL_MAX_SAMPLES pname");

	if (gl_max_samples_value < 3)
	{
		throw tcu::NotSupportedError("GL_MAX_SAMPLES pname value < 3, skipping");
	}

	/* Set up texture objects */
	if (are_2d_ms_array_tos_supported)
	{
		gl.genTextures(1, &to_id_2d_multisample_array_color);
	}

	gl.genTextures(1, &to_id_2d_multisample_color);

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGenTextures() call failed");

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Set up a renderbuffer object */
	gl.genRenderbuffers(1, &rbo_id);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a renderbuffer object");

	/* Set up first GL_RGBA8 multisample texture storage */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_2d_multisample_color);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, /* samples */
							   GL_RGBA8, 2,					 /* width */
							   2,							 /* height */
							   GL_FALSE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up GL_RGBA8 multisample texture storage.");

	if (are_2d_ms_array_tos_supported)
	{
		/* Set up second GL_RGBA8 multisample texture storage */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_2d_multisample_array_color);
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 2, /* samples */
								   GL_RGBA8, 2,							   /* width */
								   2,									   /* height */
								   2,									   /* depth */
								   GL_TRUE);							   /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up second GL_RGBA8 multisample texture storage.");
	}

	/* Set up renderbuffer storage */
	gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 3, /* samples */
									  GL_RGBA8, 2,		  /* width */
									  2);				  /* height */

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up renderbuffer storage.");

	/* Set up FBO's zeroth color attachment */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
							to_id_2d_multisample_color, 0); /* level */

	/* Make sure FBO is considered complete at this point */
	glw::GLenum fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid FBO completeness status reported:" << fbo_completeness
						   << " expected: GL_FRAMEBUFFER_COMPLETE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	if (are_2d_ms_array_tos_supported)
	{
		/* Set up FBO's first color attachment */
		gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to_id_2d_multisample_array_color,
								   0,  /* level */
								   0); /* layer */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up FBO color attachments");

		/* FBO should now be considered incomplete */
		fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		if (fbo_completeness != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid FBO completeness status reported:" << fbo_completeness
							   << " expected: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid FBO completeness status reported.");
		}
	}

	/* Set up FBO's second color attachment */
	gl.framebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, rbo_id);

	/* FBO should now be considered incomplete */
	fbo_completeness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fbo_completeness != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid FBO completeness status reported:" << fbo_completeness
						   << " expected: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid FBO completeness status reported.");
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesFBOIncompleteness5Test::MultisampleTextureDependenciesFBOIncompleteness5Test(
	Context& context)
	: TestCase(context, "fbo_with_renderbuffer_and_multisample_texture_attachments_with_different_number_of_samples",
			   "FBOs with renderbuffer and multisample texture attachments, where amount "
			   "of samples used for multisample texture attachments differs from the "
			   "amount of samples used for renderbuffer attachments, should be considered "
			   "incomplete")
	, fbo_id(0)
	, rbo_id(0)
	, to_id_multisample_2d(0)
	, to_id_multisample_2d_array(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesFBOIncompleteness5Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (rbo_id != 0)
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
		gl.deleteRenderbuffers(1, &rbo_id);
	}

	if (to_id_multisample_2d != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d);

		to_id_multisample_2d = 0;
	}

	if (to_id_multisample_2d_array != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d_array);

		to_id_multisample_2d_array = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesFBOIncompleteness5Test::iterate()
{
	bool are_multisample_2d_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve GL_MAX_INTEGER_SAMPLES and GL_MAX_SAMPLES values */
	glw::GLint gl_max_integer_samples_value = 0;
	glw::GLint gl_max_samples_value			= 0;

	gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &gl_max_integer_samples_value);
	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() generated an error");

	/* Set up texture objects */
	gl.genTextures(1, &to_id_multisample_2d);

	if (are_multisample_2d_array_tos_supported)
	{
		gl.genTextures(1, &to_id_multisample_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGenTextures() call failed");

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Set up a renderbuffer object */
	gl.genRenderbuffers(1, &rbo_id);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a renderbuffer object");

	/* Bind texture objects to relevant texture targets */
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d);

	if (are_multisample_2d_array_tos_supported)
	{
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_multisample_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glBindTexture() call failed.");

	/* Iterate through internalformats. Current internalformat will be used
	 * by the 2D multisample attachment */
	const glw::GLenum  internalformats[] = { GL_R8, GL_RGB565, GL_RGB10_A2UI, GL_SRGB8_ALPHA8, GL_R8I };
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_2d_multisample_internalformat = 0; n_2d_multisample_internalformat < n_internalformats;
		 ++n_2d_multisample_internalformat)
	{
		glw::GLenum internalformat_2d_multisample = internalformats[n_2d_multisample_internalformat];

		/* Query sample counts supported for 2DMS texture on given internal format */
		glw::GLint num_sample_counts_2dms;

		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat_2d_multisample, GL_NUM_SAMPLE_COUNTS, 1,
							   &num_sample_counts_2dms);

		std::vector<glw::GLint> sample_counts_2dms(num_sample_counts_2dms);

		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat_2d_multisample, GL_SAMPLES,
							   num_sample_counts_2dms, &sample_counts_2dms[0]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve sample counts supported for 2DMS");

		/* Iterate again through the internalformats. This internalformat will be used
		 * by the 2D multisample array attacmhent.
		 *
		 * NOTE: Under implementations which do not support 2DMS Array textures, we will
		 *       not attach the 2DMS Array textures to the FBO at all. This fits the conformance
		 *       test idea and does not break existing test implementation.
		 *       However, since 2DMS Array textures are unavailable, we only run a single inner
		 *       loop iteration. More iterations would not bring anything to the table at all.
		 */
		for (unsigned int n_2d_multisample_array_internalformat = 0;
			 n_2d_multisample_array_internalformat < ((are_multisample_2d_array_tos_supported) ? n_internalformats : 1);
			 ++n_2d_multisample_array_internalformat)
		{
			glw::GLenum internalformat_2d_multisample_array = internalformats[n_2d_multisample_array_internalformat];
			glw::GLint  num_sample_counts_2dms_array		= 1;
			std::vector<glw::GLint> sample_counts_2dms_array(num_sample_counts_2dms_array);

			if (are_multisample_2d_array_tos_supported)
			{
				/* Query sample counts supported for 2DMS_ARRAY texture on given internal format */
				gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformat_2d_multisample_array,
									   GL_NUM_SAMPLE_COUNTS, 1, &num_sample_counts_2dms_array);

				sample_counts_2dms_array.resize(num_sample_counts_2dms_array);

				gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, internalformat_2d_multisample_array,
									   GL_SAMPLES, num_sample_counts_2dms_array, &sample_counts_2dms_array[0]);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve sample counts supported for 2DMS_ARRAY");
			}
			else
			{
				/* Add a single entry to the 2d ms sample count array */
				num_sample_counts_2dms_array = 1;
				sample_counts_2dms_array[0]  = 0;
			}

			/* One more iteration for renderbuffer attachment */
			for (unsigned int n_rbo_internalformat = 0; n_rbo_internalformat < n_internalformats;
				 ++n_rbo_internalformat)
			{
				glw::GLenum internalformat_rbo = internalformats[n_rbo_internalformat];

				/* Query sample counts supported for RBO on given internal format */
				glw::GLint num_sample_counts_rbo;

				gl.getInternalformativ(GL_RENDERBUFFER, internalformat_rbo, GL_NUM_SAMPLE_COUNTS, 1,
									   &num_sample_counts_rbo);

				std::vector<glw::GLint> sample_counts_rbo(num_sample_counts_rbo);

				gl.getInternalformativ(GL_RENDERBUFFER, internalformat_rbo, GL_SAMPLES, num_sample_counts_rbo,
									   &sample_counts_rbo[0]);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve sample counts supported for rbo");

				/* Now iterate over all samples argument we will use for the 2D multisample attachment */
				for (int i_2dms = 0; i_2dms < num_sample_counts_2dms; ++i_2dms)
				{
					int samples_2d_multisample = sample_counts_2dms[i_2dms];

					/* ..and yet another iteration for the 2D multisample array attachment */
					for (int i_2dms_array = 0; i_2dms_array < num_sample_counts_2dms_array; ++i_2dms_array)
					{
						int samples_2d_multisample_array = sample_counts_2dms_array[i_2dms_array];

						/* Finally, iterate over values to be used for samples argument of
						 * a glRenderbufferStorageMultisample() call.
						 */
						for (int i_rbo = 0; i_rbo < num_sample_counts_rbo; ++i_rbo)
						{
							int samples_rbo = sample_counts_rbo[i_rbo];

							/* This is a negative test. Hence, skip an iteration where all the
							 * samples arguments used for the multisample 2d/multisample 2d array/rbo
							 * triple match.
							 */
							if (((samples_rbo == samples_2d_multisample) &&
								 (samples_rbo == samples_2d_multisample_array)) ||
								(samples_rbo == 1) || (samples_2d_multisample == 1) ||
								(samples_2d_multisample_array == 1))
							{
								/* Skip the iteration */
								continue;
							}

							/* Set up 2D multisample texture storage. */
							gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples_2d_multisample,
													   internalformat_2d_multisample, 2, /* width */
													   2,								 /* height */
													   GL_FALSE);
							GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed.");

							if (are_multisample_2d_array_tos_supported)
							{
								/* Set up 2D multisample array texture storage. */
								gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES,
														   samples_2d_multisample_array,
														   internalformat_2d_multisample_array, 2, /* width */
														   2,									   /* height */
														   2,									   /* depth */
														   GL_FALSE);
								GLU_EXPECT_NO_ERROR(gl.getError(), "gltexStorage3DMultisample() call failed.");
							}

							/* Set up renderbuffer storage */
							gl.renderbufferStorageMultisample(GL_RENDERBUFFER, samples_rbo, internalformat_rbo,
															  2,  /* width */
															  2); /* height */
							GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorageMultisample() call failed.");

							/* Set up FBO's color attachments */
							gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
													GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d, 0); /* level */
							GLU_EXPECT_NO_ERROR(
								gl.getError(),
								"glFramebufferTexture2D() call failed for GL_COLOR_ATTACHMENT0 color attachment.");

							if (are_multisample_2d_array_tos_supported)
							{
								gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
														   to_id_multisample_2d_array, 0, /* level */
														   0);							  /* layer */
								GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTextureLayer() call failed for "
																   "GL_COLOR_ATTACHMENT1 color attachment.");
							}

							gl.framebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER,
													   rbo_id);
							GLU_EXPECT_NO_ERROR(
								gl.getError(),
								"glFramebufferRenderbuffer() call failed for GL_COLOR_ATTACHMENT2 color attachment.");

							/* Make sure the FBO is incomplete */
							glw::GLenum fbo_completeness_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

							if (fbo_completeness_status != GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
							{
								m_testCtx.getLog() << tcu::TestLog::Message
												   << "Invalid FBO completeness status reported ["
												   << fbo_completeness_status
												   << "]"
													  " instead of expected GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE."
												   << " Sample count 2D_MS:" << samples_2d_multisample
												   << " Sample count 2D_MS_ARRAY:" << samples_2d_multisample_array
												   << " Sample count RBO:" << samples_rbo << tcu::TestLog::EndMessage;

								TCU_FAIL("Invalid FBO completeness status reported.");
							}

							/* Re-create texture objects */
							gl.deleteTextures(1, &to_id_multisample_2d);

							if (are_multisample_2d_array_tos_supported)
							{
								gl.deleteTextures(1, &to_id_multisample_2d_array);
							}

							GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glDeleteTextures() call failed.");

							gl.genTextures(1, &to_id_multisample_2d);

							if (are_multisample_2d_array_tos_supported)
							{
								gl.genTextures(1, &to_id_multisample_2d_array);
							}

							GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glGenTextures() call failed.");

							gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d);

							if (are_multisample_2d_array_tos_supported)
							{
								gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_multisample_2d_array);
							}

							GLU_EXPECT_NO_ERROR(gl.getError(), "At least one glBindTexture() call failed.");
						} /* for (all samples argument values to be used for renderbuffer attachment) */
					}	 /* for (all samples argument values to be used for 2D multisample array attachment) */
				}		  /* for (all samples argument values to be used for 2D multisample attachment) */
			}			  /* for (all internalformats used by renderbuffer attachment) */
		}				  /* for (all internalformats used by 2D multisample array attachment) */
	}					  /* for (all internalformats used by 2D multisample attachment) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test::
	MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test(Context& context)
	: TestCase(context, "framebuffer_texture2d_used_with_invalid_texture_target",
			   "Checks GL_INVALID_OPERATION is reported if 2D or cube-map texture "
			   "target is used with a multisample 2D texture for a "
			   "glFramebufferTexture2D() call")
	, fbo_id(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Generate and bind a texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a texture object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing internalformat specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_INTEGER_SAMPLES and/or GL_MAX_SAMPLES values");

	/* Iterate through all internalformats to be used for the test */
	const glw::GLenum internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_RGBA32I
	};

	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat				= internalformats[n_internalformat];
		bool		is_color_renderable			= false;
		bool		is_depth_stencil_renderable = false;

		if (internalformat == GL_DEPTH24_STENCIL8)
		{
			is_depth_stencil_renderable = true;
		}
		else if (internalformat != GL_DEPTH_COMPONENT32F)
		{
			is_color_renderable = true;
		}

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat, GL_SAMPLES, 1,
							   &gl_max_internalformat_samples);

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		glw::GLint samples = gl_max_internalformat_samples;

		/* Skip formats that are not multisampled in the implementation */
		if (samples <= 1)
		{
			continue;
		}

		/* Set the texture object storage up */
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, 1, /* width */
								   1,													  /* height */
								   GL_FALSE);											  /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call generated an unexpected error.");

		/* Try to issue the invalid glFramebufferTexture2D() call */
		glw::GLenum attachment = (is_color_renderable) ?
									 GL_COLOR_ATTACHMENT0 :
									 (is_depth_stencil_renderable) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, to_id, 0); /* level */

		/* Make sure GL_INVALID_OPERATION error was generated */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message << "An unexpected error code " << error_code
				<< " instead of GL_INVALID_OPERATION was generated by an invalid glFramebufferTexture2D() call"
				<< tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code reported by glFramebufferTexture2D() call.");
		}

		/* Re-create the texture object */
		gl.deleteTextures(1, &to_id);
		gl.genTextures(1, &to_id);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create the texture object.");
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test::
	MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test(Context& context)
	: TestCase(context, "framebuffer_texture2d_used_with_invalid_level",
			   "Checks GL_INVALID_VALUE is reported if glFramebufferTexture2D() "
			   "is called with invalid level argument.")
	, fbo_id(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Generate and bind a texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a texture object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing format specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_INTEGER_SAMPLES and/or GL_MAX_SAMPLE values");

	/* Iterate through all internalformats to be used for the test */
	const glw::GLenum internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_RGBA32I
	};
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat		= internalformats[n_internalformat];
		bool		is_color_renderable = false;
		bool		is_depth_renderable = false;

		if (internalformat == GL_DEPTH_COMPONENT32F)
		{
			is_depth_renderable = true;
		}
		else if (internalformat != GL_DEPTH24_STENCIL8)
		{
			is_color_renderable = true;
		}

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat, GL_SAMPLES, 1,
							   &gl_max_internalformat_samples);

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		glw::GLint samples = gl_max_internalformat_samples;

		/* Skip formats that are not multisampled in implementation */
		if (samples <= 1)
		{
			continue;
		}

		/* Set the texture object storage up */
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, 1, /* width */
								   1,													  /* height */
								   GL_FALSE);											  /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call generated an unexpected error.");

		/* Try to issue the invalid glFramebufferTexture2D() call */
		glw::GLenum attachment = (is_color_renderable) ?
									 GL_COLOR_ATTACHMENT0 :
									 (is_depth_renderable) ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;

		/* From spec:
		 *
		 * If textarget is TEXTURE_2D_MULTISAMPLE, then level must be zero. If textarget
		 * is one of the cube map face targets from table 3.21, then level must be greater
		 * than or equal to zero and less than or equal to log2 of the value of MAX_CUBE_-
		 * MAP_TEXTURE_SIZE. If textarget is TEXTURE_2D, level must be greater than or
		 * equal to zero and no larger than log2 of the value of MAX_TEXTURE_SIZE.
		 */
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, to_id, 1); /* level */

		/* Make sure GL_INVALID_VALUE error was generated */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_VALUE)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message << "An unexpected error code " << error_code
				<< " instead of GL_INVALID_VALUE was generated by an invalid glFramebufferTexture2D() call"
				<< tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code reported by glFramebufferTexture2D() call.");
		}

		/* Re-create the texture object */
		gl.deleteTextures(1, &to_id);
		gl.genTextures(1, &to_id);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create the texture object.");
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test::
	MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test(Context& context)
	: TestCase(context, "framebuffer_texture_layer_used_for_invalid_texture_target",
			   "Checks GL_INVALID_OPERATION is reported if 2D multisample texture is used for a "
			   "glFramebufferTextureLayer() call")
	, fbo_id(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Generate and bind a texture object to GL_TEXTURE_2D_MULTISAMPLE texture target */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a texture object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing format specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	/* Iterate through all internalformats to be used for the test */
	const glw::GLenum internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_RGBA32I
	};
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat		= internalformats[n_internalformat];
		bool		is_color_renderable = false;
		bool		is_depth_renderable = false;

		if (internalformat == GL_DEPTH_COMPONENT32F)
		{
			is_depth_renderable = true;
		}
		else if (internalformat != GL_DEPTH24_STENCIL8)
		{
			is_color_renderable = true;
		}

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat, GL_SAMPLES, 1,
							   &gl_max_internalformat_samples);

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		glw::GLint samples = gl_max_internalformat_samples;

		/* Skip formats that are not multisampled in implementation */
		if (samples <= 1)
		{
			continue;
		}

		/* Set the texture object storage up */
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, 2, /* width */
								   2,													  /* height */
								   GL_FALSE);											  /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call generated an unexpected error.");

		/* Try to issue the invalid glFramebufferTextureLayer() call */
		glw::GLenum attachment = (is_color_renderable) ?
									 GL_COLOR_ATTACHMENT0 :
									 (is_depth_renderable) ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;

		gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachment, to_id, 0, /* level */
								   0);										  /* layer */

		/* Make sure GL_INVALID_OPERATION error was generated */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message << "An unexpected error code " << error_code
				<< " instead of GL_INVALID_OPERATION was generated by an invalid glFramebufferTextureLayer() call"
				<< tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code reported by glFramebufferTextureLayer() call.");
		}

		/* Re-create the texture object */
		gl.deleteTextures(1, &to_id);
		gl.genTextures(1, &to_id);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create the texture object.");
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test::
	MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test(Context& context)
	: TestCase(context, "framebuffer_texture_layer_used_with_invalid_level_argument",
			   "Checks GL_INVALID_VALUE error is reported if a glFramebufferTextureLayer() call"
			   " is made with level exceeding amount of layers defined for a 2D multisample"
			   " array texture")
	, fbo_id(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");

	/* Generate and bind a texture object to GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target */
	gl.genTextures(1, &to_id);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a texture object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "OES_texture_storage_multisample_2d_array");

		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing format specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	/* Iterate through all internalformats to be used for the test */
	const glw::GLenum internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_RGBA32I
	};
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat		= internalformats[n_internalformat];
		bool		is_color_renderable = false;
		bool		is_depth_renderable = false;

		if (internalformat == GL_DEPTH_COMPONENT32F)
		{
			is_depth_renderable = true;
		}
		else if (internalformat != GL_DEPTH24_STENCIL8)
		{
			is_color_renderable = true;
		}

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat, GL_SAMPLES, 1,
							   &gl_max_internalformat_samples);

		/* Get MAX_TEXTURE_SIZE and calculate max level */
		glw::GLint gl_max_texture_size = 0;
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
		const glw::GLint max_level = glw::GLint(log(double(gl_max_texture_size)) / log(2.0));

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		glw::GLint samples = gl_max_internalformat_samples;

		/* Set the texture object storage up */
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, samples, internalformat, 2, /* width */
								   2,																/* height */
								   2,																/* depth */
								   GL_FALSE); /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "gltexStorage3DMultisample() call generated an unexpected error.");

		/* Try to issue the invalid glFramebufferTextureLayer() call */
		glw::GLenum attachment = (is_color_renderable) ?
									 GL_COLOR_ATTACHMENT0 :
									 (is_depth_renderable) ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;

		gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, attachment, to_id,
								   max_level + 1, /* level - must be <= log_2(MAX_TEXTURE_SIZE) */
								   0);			  /* layer */

		/* Make sure GL_INVALID_VALUE error was generated */
		glw::GLenum error_code = gl.getError();

		if (error_code != GL_INVALID_VALUE)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message << "An unexpected error code " << error_code
				<< " instead of GL_INVALID_VALUE was generated by an invalid glFramebufferTextureLayer() call"
				<< tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code reported by glFramebufferTextureLayer() call.");
		}

		/* Re-create the texture object */
		gl.deleteTextures(1, &to_id);
		gl.genTextures(1, &to_id);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not re-create the texture object.");
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test::
	MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test(Context& context)
	: TestCase(context, "renderbuffer_storage_multisample_invalid_samples_argument_for_noninteger_internalformats",
			   "GL_INVALID_OPERATION error is reported for glRenderbufferStorageMultisample() "
			   "calls, for which samples argument > MAX_SAMPLES for non-integer internalformats")
	, rbo_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (rbo_id != 0)
	{
		gl.deleteRenderbuffers(1, &rbo_id);

		rbo_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &rbo_id);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a renderbuffer object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Retrieve GL_MAX_SAMPLES pname value */
	glw::GLint gl_max_samples_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLES, &gl_max_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_SAMPLES value.");

	/* Iterate through a set of valid non-integer internalformats */
	const glw::GLenum noninteger_internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_STENCIL_INDEX8
	};
	const unsigned int n_noninteger_internalformats =
		sizeof(noninteger_internalformats) / sizeof(noninteger_internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_noninteger_internalformats; ++n_internalformat)
	{
		glw::GLenum		  error_code						  = GL_NO_ERROR;
		const glw::GLenum internalformat					  = noninteger_internalformats[n_internalformat];
		glw::GLint		  gl_max_internalformat_samples_value = -1;

		/* Retrieve maximum amount of samples available for the texture target considered */
		gl.getInternalformativ(GL_RENDERBUFFER, internalformat, GL_SAMPLES, 1, &gl_max_internalformat_samples_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() failed to retrieve GL_SAMPLES");

		/* Execute the test */
		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, gl_max_internalformat_samples_value + 1, internalformat,
										  1,  /* width */
										  1); /* height */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glRenderbufferStorageMultisample() generated error code "
							   << error_code << " when GL_INVALID_OPERATION was expected." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code generated by glRenderbufferStorageMultisample() call.");
		}
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test::
	MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test(Context& context)
	: TestCase(context, "renderbuffer_storage_multisample_invalid_samples_argument_for_integer_internalformats",
			   "GL_INVALID_OPERATION error is reported for glRenderbufferStorageMultisample() calls, "
			   "for which samples argument > MAX_INTEGER_SAMPLES for integer internalformats")
	, rbo_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (rbo_id != 0)
	{
		gl.deleteRenderbuffers(1, &rbo_id);

		rbo_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &rbo_id);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a renderbuffer object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* Retrieve GL_MAX_INTEGER_SAMPLES pname value */
	glw::GLint gl_max_integer_samples_value = 0;

	gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &gl_max_integer_samples_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve GL_MAX_INTEGER_SAMPLES value.");

	/* Iterate through a set of valid integer internalformats */
	const glw::GLenum  integer_internalformats[] = { GL_RG8UI, GL_RGBA32I };
	const unsigned int n_integer_internalformats = sizeof(integer_internalformats) / sizeof(integer_internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_integer_internalformats; ++n_internalformat)
	{
		glw::GLenum		  error_code	 = GL_NO_ERROR;
		const glw::GLenum internalformat = integer_internalformats[n_internalformat];

		/* Execute the test */
		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, gl_max_integer_samples_value + 1, internalformat,
										  1,  /* width */
										  1); /* height */

		error_code = gl.getError();
		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glRenderbufferStorageMultisample() generated error code "
							   << error_code << " when GL_INVALID_OPERATION was expected." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid error code generated by glRenderbufferStorageMultisample() call.");
		}
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest::
	MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest(Context& context)
	: TestCase(context, "no_error_generated_for_valid_framebuffer_texture2d_calls",
			   "No error is reported for glFramebufferTexture2D() calls using "
			   "GL_TEXTURE_2D_MULTISAMPLE texture target.")
	, fbo_id(0)
	, to_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &fbo_id);

		fbo_id = 0;
	}

	if (to_id != 0)
	{
		gl.deleteTextures(1, &to_id);

		to_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up a framebuffer object */
	gl.genFramebuffers(1, &fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a framebuffer object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest::
	iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing format specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	/* Iterate through all internalformats */
	const glw::GLenum internalformats[] = {
		GL_RGB8, GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8, GL_RGBA32I
	};
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat = internalformats[n_internalformat];

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, internalformat, GL_SAMPLES, 1,
							   &gl_max_internalformat_samples);

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		glw::GLint samples = gl_max_internalformat_samples;

		/* Skip formats that are not multisampled in implementation */
		if (samples <= 1)
		{
			continue;
		}

		/* Set up a texture object. */
		gl.genTextures(1, &to_id);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id);

		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalformat, 1, /* width */
								   1,													  /* height */
								   GL_FALSE);											  /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a texture object");

		/* Determine attachment type for internalformat considered */
		glw::GLenum attachment_type = GL_COLOR_ATTACHMENT0;

		if (internalformat == GL_DEPTH_COMPONENT32F)
		{
			attachment_type = GL_DEPTH_ATTACHMENT;
		}
		else if (internalformat == GL_DEPTH24_STENCIL8)
		{
			attachment_type = GL_DEPTH_STENCIL_ATTACHMENT;
		}

		/* Attach it to the FBO */
		gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment_type, GL_TEXTURE_2D_MULTISAMPLE, to_id, 0); /* level */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

		/* Release the texture object */
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		gl.deleteTextures(1, &to_id);

		to_id = 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not release the texture object");
	} /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest::
	MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest(Context& context)
	: TestCase(context, "no_error_generated_for_valid_renderbuffer_storage_multisample_calls",
			   "No error is reported for valid glRenderbufferStorageMultisample() calls.")
	, rbo_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (rbo_id != 0)
	{
		gl.deleteRenderbuffers(1, &rbo_id);

		rbo_id = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Initializes ES objects created during test execution */
void MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest::initInternals()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &rbo_id);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a renderbuffer object");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest::
	iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initInternals();

	/* For storing format specific maximum number of samples */
	glw::GLint gl_max_internalformat_samples = 0;

	/* Iterate through a set of valid non-integer and integer
	 internalformats and a set of all legal samples argument values */
	const glw::GLenum internalformats[] = {
		GL_RGB8,		   GL_RGB565, GL_SRGB8_ALPHA8, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8,
		GL_STENCIL_INDEX8, GL_RG8UI,  GL_RGBA32I
	};
	const unsigned int n_internalformats = sizeof(internalformats) / sizeof(internalformats[0]);

	for (unsigned int n_internalformat = 0; n_internalformat < n_internalformats; ++n_internalformat)
	{
		glw::GLenum internalformat = internalformats[n_internalformat];

		/* Determine a value to be used for samples argument in subsequent
		 * glTexStorage2DMultisample() call. */
		gl.getInternalformativ(GL_RENDERBUFFER, internalformat, GL_SAMPLES, 1, &gl_max_internalformat_samples);

		GLU_EXPECT_NO_ERROR(gl.getError(),
							"Could not retrieve maximum supported amount of samples for internal format");

		for (int samples = 1; samples <= gl_max_internalformat_samples; ++samples)
		{
			/* Execute the test */
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalformat, 1, 1);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"A valid glRenderbufferStorageMultisample() call has reported an error.");
		} /* for (all legal samples argument values) */
	}	 /* for (all internalformats) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context CTS context handle.
 **/
MultisampleTextureDependenciesTexParameterTest::MultisampleTextureDependenciesTexParameterTest(Context& context)
	: TestCase(context, "tex_parameter_support",
			   "Verifies glTexParameter*() behavior when used against multisample texture targets")
	, to_id_multisample_2d(0)
	, to_id_multisample_2d_array(0)
{
	/* Left blank on purpose */
}

/* Calls glTexParameterf(), glTexParameterfv(), glTexParameteri() and
 * glTexParameteriv(). For each invocation, the function checks if
 * the error code reported after each call matches the expected value.
 * If the values differ, an info message is logged and TestError exception
 * is thrown.
 *
 * @param expected_error_code Expected GL error code.
 * @param value               Integer value to use. For glTexParameterf()
 *                            or glTexParameterfv(), the value will be cast
 *                            onto a float type prior to calling.
 * @param pname               GL pname to use for glTexParameter*() calls.
 * @param texture_target      Texture target to use for glTexParameter*() calls.
 */
void MultisampleTextureDependenciesTexParameterTest::checkAllTexParameterInvocations(glw::GLenum expected_error_code,
																					 glw::GLint  value,
																					 glw::GLenum pname,
																					 glw::GLenum texture_target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum		   error_code  = GL_NO_ERROR;
	const glw::GLfloat float_value = (glw::GLfloat)value;
	const glw::GLint   int_value   = value;

	/* glTexParameterf() */
	gl.texParameterf(texture_target, pname, float_value);

	error_code = gl.getError();

	if (error_code != expected_error_code)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterf() call generated an error " << error_code
						   << " instead of the expected error code " << expected_error_code << tcu::TestLog::EndMessage;

		TCU_FAIL("glTexParameterf() call generated an unexpected error.");
	}

	/* glTexParameteri() */
	gl.texParameteri(texture_target, pname, int_value);

	error_code = gl.getError();

	if (error_code != expected_error_code)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteri() call generated an error " << error_code
						   << " instead of the expected error code " << expected_error_code << tcu::TestLog::EndMessage;

		TCU_FAIL("glTexParameterf() call generated an unexpected error.");
	}

	/* glTexParameterfv() */
	gl.texParameterfv(texture_target, pname, &float_value);

	error_code = gl.getError();

	if (error_code != expected_error_code)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameterfv() call generated an error " << error_code
						   << " instead of the expected error code " << expected_error_code << tcu::TestLog::EndMessage;

		TCU_FAIL("glTexParameterfv() call generated an unexpected error.");
	}

	/* glTexParameteriv() */
	gl.texParameteriv(texture_target, pname, &int_value);

	error_code = gl.getError();

	if (error_code != expected_error_code)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glTexParameteriv() call generated an error " << error_code
						   << " instead of the expected error code " << expected_error_code << tcu::TestLog::EndMessage;

		TCU_FAIL("glTexParameteriv() call generated an unexpected error.");
	}
}

/** Deinitializes ES objects created during test execution */
void MultisampleTextureDependenciesTexParameterTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (to_id_multisample_2d != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d);

		to_id_multisample_2d = 0;
	}

	if (to_id_multisample_2d_array != 0)
	{
		gl.deleteTextures(1, &to_id_multisample_2d_array);

		to_id_multisample_2d_array = 0;
	}

	/* Call base class' deinit() */
	TestCase::deinit();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing.
 */
tcu::TestNode::IterateResult MultisampleTextureDependenciesTexParameterTest::iterate()
{
	bool are_multisample_2d_array_tos_supported =
		m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array");
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up texture objects */
	gl.genTextures(1, &to_id_multisample_2d);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to_id_multisample_2d);

	if (are_multisample_2d_array_tos_supported)
	{
		gl.genTextures(1, &to_id_multisample_2d_array);
		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, to_id_multisample_2d_array);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up texture objects");

	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, /* samples */
							   GL_RGBA8, 1,					 /* width */
							   1,							 /* height */
							   GL_FALSE);					 /* fixedsamplelocations */

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glTexStorage2DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE texture target");

	if (are_multisample_2d_array_tos_supported)
	{
		gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 1, /* samples */
								   GL_RGBA8, 1,							   /* width */
								   1,									   /* height */
								   1,									   /* depth */
								   GL_FALSE);							   /* fixedsamplelocations */

		GLU_EXPECT_NO_ERROR(
			gl.getError(),
			"gltexStorage3DMultisample() call failed for GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture target");
	}

	/* Run the test for both multisample texture targets */
	const glw::GLenum  texture_targets[] = { GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES };
	const unsigned int n_texture_targets = sizeof(texture_targets) / sizeof(texture_targets[0]);

	for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
	{
		glw::GLenum texture_target = texture_targets[n_texture_target];

		if (!are_multisample_2d_array_tos_supported && texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES)
		{
			/* Skip the iteration */
			continue;
		}

		/* Verify that setting GL_TEXTURE_BASE_LEVEL to 0 does not generate any 0. Using any other
		 * value should generate GL_INVALID_OPERATION
		 */
		for (int n_iteration = 0; n_iteration < 2 /* iterations */; ++n_iteration)
		{
			glw::GLenum expected_error_code = (n_iteration == 0) ? GL_NO_ERROR : GL_INVALID_OPERATION;
			glw::GLint  int_value			= (n_iteration == 0) ? 0 : 1;

			checkAllTexParameterInvocations(expected_error_code, int_value, GL_TEXTURE_BASE_LEVEL, texture_target);
		} /* for (all iterations) */
	}	 /* for (both texture targets) */

	/* Make sure that modifying sampler state information results in an error
	 * for multisample texture targets. */
	const glw::GLenum sampler_pnames[] = { GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER,   GL_TEXTURE_WRAP_S,
										   GL_TEXTURE_WRAP_T,	 GL_TEXTURE_WRAP_R,	   GL_TEXTURE_MIN_LOD,
										   GL_TEXTURE_MAX_LOD,	GL_TEXTURE_COMPARE_MODE, GL_TEXTURE_COMPARE_FUNC };
	const unsigned int n_sampler_pnames = sizeof(sampler_pnames) / sizeof(sampler_pnames[0]);

	for (unsigned int n_sampler_pname = 0; n_sampler_pname < n_sampler_pnames; ++n_sampler_pname)
	{
		glw::GLenum pname = sampler_pnames[n_sampler_pname];

		for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
		{
			glw::GLenum texture_target = texture_targets[n_texture_target];

			if (!are_multisample_2d_array_tos_supported && texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES)
			{
				/* Skip the iteration */
				continue;
			}

			/* When <target> is TEXTURE_2D_MULTISAMPLE or
			 TEXTURE_2D_MULTISAMPLE_ARRAY, certain texture parameters may not be
			 specified. In this case, an INVALID_ENUM */
			checkAllTexParameterInvocations(GL_INVALID_ENUM, 0, pname, texture_target);

		} /* for (all texture targets) */
	}	 /* for (all sampler properties) */

	/* Make sure that modifying remaining texture parameters does not result in an error for
	 * multisample texture targets. */
	for (unsigned int n_texture_target = 0; n_texture_target < n_texture_targets; ++n_texture_target)
	{
		glw::GLenum texture_target = texture_targets[n_texture_target];

		if (!are_multisample_2d_array_tos_supported && texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES)
		{
			/* Skip the iteration */
			continue;
		}

		checkAllTexParameterInvocations(GL_NO_ERROR, 10, GL_TEXTURE_MAX_LEVEL, texture_target);
		checkAllTexParameterInvocations(GL_NO_ERROR, GL_GREEN, GL_TEXTURE_SWIZZLE_R, texture_target);
		checkAllTexParameterInvocations(GL_NO_ERROR, GL_BLUE, GL_TEXTURE_SWIZZLE_G, texture_target);
		checkAllTexParameterInvocations(GL_NO_ERROR, GL_ALPHA, GL_TEXTURE_SWIZZLE_B, texture_target);
		checkAllTexParameterInvocations(GL_NO_ERROR, GL_RED, GL_TEXTURE_SWIZZLE_A, texture_target);
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
