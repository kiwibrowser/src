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

/**
 */ /*!
 * \file  gl4cDirectStateAccessFramebuffersAndRenderbuffersTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Framebuffers and Renderbuffer access part).
 */ /*------------------------------------------------------------------------------------------------------------------------*/

/* Includes. */
#include "gl4cDirectStateAccessTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace gl4cts
{
namespace DirectStateAccess
{
namespace Framebuffers
{
/******************************** Framebuffer Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_creation", "Framebuffer Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Framebuffers' objects */
	static const glw::GLuint framebuffers_count = 2;

	glw::GLuint framebuffers_legacy[framebuffers_count] = {};
	glw::GLuint framebuffers_dsa[framebuffers_count]	= {};

	try
	{
		/* Check legacy state creation. */
		gl.genFramebuffers(framebuffers_count, framebuffers_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		for (glw::GLuint i = 0; i < framebuffers_count; ++i)
		{
			if (gl.isFramebuffer(framebuffers_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenFramebuffers has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		gl.createFramebuffers(framebuffers_count, framebuffers_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

		for (glw::GLuint i = 0; i < framebuffers_count; ++i)
		{
			if (!gl.isFramebuffer(framebuffers_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateFramebuffers has not created default objects."
													<< tcu::TestLog::EndMessage;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < framebuffers_count; ++i)
	{
		if (framebuffers_legacy[i])
		{
			gl.deleteFramebuffers(1, &framebuffers_legacy[i]);

			framebuffers_legacy[i] = 0;
		}

		if (framebuffers_dsa[i])
		{
			gl.deleteFramebuffers(1, &framebuffers_dsa[i]);

			framebuffers_dsa[i] = 0;
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Framebuffer Renderbuffer Attachment Test Implementation   ********************************/

/** @brief Framebuffer Renderbuffer Attachment Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
RenderbufferAttachmentTest::RenderbufferAttachmentTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_renderbuffer_attachment", "Framebuffer Renderbuffer Attachment Test")
	, m_fbo(0)
	, m_rbo(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Framebuffer Renderbuffer Attachment Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult RenderbufferAttachmentTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

		gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			is_ok &= Test(GL_COLOR_ATTACHMENT0 + i, GL_RGBA8);
			Clean();
		}

		is_ok &= Test(GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT24);
		Clean();

		is_ok &= Test(GL_STENCIL_ATTACHMENT, GL_STENCIL_INDEX8);
		Clean();

		is_ok &= Test(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8);
		Clean();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Test functionality.
 *
 *  @param [in] attachment         Framebuffer attachment.
 *  @param [in] internalformat     Internal format.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool RenderbufferAttachmentTest::Test(glw::GLenum attachment, glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* RBO creation. */
	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, internalformat, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	/* FBO creation. */
	gl.createFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

	gl.namedFramebufferRenderbuffer(m_fbo, attachment, GL_RENDERBUFFER, m_rbo);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "NamedFramebufferRenderbuffer for "
											<< glu::getFramebufferAttachmentStr(attachment)
											<< " attachment failed with error value " << glu::getErrorStr(error) << "."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	glw::GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);

	if (GL_FRAMEBUFFER_COMPLETE != status)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Named Framebuffer Renderbuffer Attachment test failed because of framebuffer "
			<< glu::getFramebufferStatusStr(status) << " with renderbuffer set up as "
			<< glu::getFramebufferAttachmentStr(attachment) << " attachment." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Clean up GL state.
 */
void RenderbufferAttachmentTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Framebuffer Texture Attachment Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
TextureAttachmentTest::TextureAttachmentTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_texture_attachment", "Framebuffer Texture Attachment Test")
	, m_fbo(0)
	, m_to(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult TextureAttachmentTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

		gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		for (glw::GLuint i = 0; i < s_targets_count; ++i)
		{
			for (glw::GLuint j = 1; j <= MaxTextureLevels(s_targets[i]); ++j)
			{
				for (glw::GLint k = 0; k < max_color_attachments; ++k)
				{
					is_ok &= Test(GL_COLOR_ATTACHMENT0 + k, true, s_targets[i], GL_RGBA8, j);
					Clean();
				}

				is_ok &= Test(GL_DEPTH_ATTACHMENT, false, s_targets[i], GL_DEPTH_COMPONENT24, j);
				Clean();

				is_ok &= Test(GL_STENCIL_ATTACHMENT, false, s_targets[i], GL_STENCIL_INDEX8, j);
				Clean();

				is_ok &= Test(GL_DEPTH_STENCIL_ATTACHMENT, false, s_targets[i], GL_DEPTH24_STENCIL8, j);
				Clean();
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Test functionality.
 *
 *  @param [in]  attachment            Framebuffer attachment.
 *  @param [in] is_color_attachment    Is color attachment tested.
 *  @param [in] texture_target         Texture target.
 *  @param [in] internalformat         Internal format ot be tested.
 *  @param [in] levels                 Number of levels.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool TextureAttachmentTest::Test(glw::GLenum attachment, bool is_color_attachment, glw::GLenum texture_target,
								 glw::GLenum internalformat, glw::GLuint levels)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* RBO creation. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(texture_target, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	if (GL_TEXTURE_2D_MULTISAMPLE == texture_target)
	{
		gl.texStorage2DMultisample(texture_target, 1, internalformat, (glw::GLuint)std::pow((double)2, (double)levels),
								   (glw::GLuint)std::pow((double)2, (double)levels), GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");
	}
	else
	{
		gl.texStorage2D(texture_target, levels, internalformat, (glw::GLuint)std::pow((double)2, (double)levels),
						(glw::GLuint)std::pow((double)2, (double)levels));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");
	}

	gl.bindTexture(texture_target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	/* FBO creation. */
	gl.createFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

	for (glw::GLuint i = 0; i < levels; ++i)
	{
		gl.namedFramebufferTexture(m_fbo, attachment, m_to, i);

		SubTestAttachmentError(attachment, texture_target, i, levels);

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		if (is_color_attachment)
		{
			gl.drawBuffer(attachment);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

			gl.readBuffer(attachment);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer has failed");
		}

		SubTestStatus(attachment, texture_target, i, levels);

		if (GL_TEXTURE_2D_MULTISAMPLE != texture_target)
		{
			Clear();

			if (!SubTestContent(attachment, texture_target, internalformat, i, levels))
			{
				return false;
			}
		}

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");
	}

	return true;
}

/** @brief Check error and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] level                  Tested level.
 *  @param [in] levels                 Number of levels.
 *
 *  @return True if no error, false otherwise.
 */
bool TextureAttachmentTest::SubTestAttachmentError(glw::GLenum attachment, glw::GLenum texture_target,
												   glw::GLuint level, glw::GLuint levels)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "NamedFramebufferTexture for " << glu::getFramebufferAttachmentStr(attachment)
			<< " attachment of " << glu::getTextureTargetStr(texture_target) << " texture and texture level " << level
			<< " of texture with " << levels << " levels failed with error value " << glu::getErrorStr(error) << "."
			<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check status and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] level                  Tested level.
 *  @param [in] levels                 Number of levels.
 *
 *  @return True if FRAMEBUFFER_COMPLETE, false otherwise.
 */
bool TextureAttachmentTest::SubTestStatus(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level,
										  glw::GLuint levels)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);

	if (GL_FRAMEBUFFER_COMPLETE != status)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Named Framebuffer Texture Attachment test failed because of framebuffer "
			<< glu::getFramebufferStatusStr(status) << " status with " << glu::getTextureTargetStr(texture_target)
			<< " texture set up as " << glu::getFramebufferAttachmentStr(attachment) << " attachment and texture level "
			<< level << " of texture with " << levels << " levels." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check framebuffer content and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] internalformat         Tested internal format.
 *  @param [in] level                  Tested level.
 *  @param [in] levels                 Number of levels.
 *
 *  @return True if FRAMEBUFFER_COMPLETE, false otherwise.
 */
bool TextureAttachmentTest::SubTestContent(glw::GLenum attachment, glw::GLenum texture_target,
										   glw::GLenum internalformat, glw::GLuint level, glw::GLuint levels)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check framebuffer's color content. */
	if (GL_RGBA8 == internalformat)
	{
		glw::GLfloat color[4] = { 0.f };

		gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		for (int i = 0; i < 4 /* color components */; ++i)
		{
			if (de::abs(s_reference_color[i] - color[i]) > 0.0625 /* precision */)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed with "
					<< glu::getTextureTargetStr(texture_target) << " texture set up as "
					<< glu::getFramebufferAttachmentStr(attachment) << " attachment and texture level " << level
					<< " of texture with " << levels << " levels. The color content of the framebuffer was ["
					<< color[0] << ", " << color[1] << ", " << color[2] << ", " << color[3] << "], but ["
					<< s_reference_color[0] << ", " << s_reference_color[1] << ", " << s_reference_color[2] << ", "
					<< s_reference_color[3] << "] was expected." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	/* Check framebuffer's depth content. */
	if ((GL_DEPTH_COMPONENT24 == internalformat) || (GL_DEPTH24_STENCIL8 == internalformat))
	{
		glw::GLfloat depth = 0.f;

		gl.readPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		if (de::abs(s_reference_depth - depth) > 0.0625 /* precision */)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed "
				<< "with texture set up as " << glu::getFramebufferAttachmentStr(attachment)
				<< " attachment and texture level " << level << " of texture with " << levels
				<< " levels. The depth content of the framebuffer was [" << depth << "], but [" << s_reference_depth
				<< "] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	/* Check framebuffer's stencil content. */
	if ((GL_STENCIL_INDEX8 == internalformat) || (GL_DEPTH24_STENCIL8 == internalformat))
	{
		glw::GLint stencil = 0;

		gl.readPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_INT, &stencil);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		if (s_reference_stencil != stencil)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed "
				<< "with texture set up as " << glu::getFramebufferAttachmentStr(attachment)
				<< " attachment and texture level " << level << " of texture with " << levels
				<< " levels. The stencil content of the framebuffer was [" << stencil << "], but ["
				<< s_reference_stencil << "] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Query max texture levels.
 *
 *  @param [in] texture_target         Texture target.
 *
 *  @return Max texture levels.
 */
glw::GLuint TextureAttachmentTest::MaxTextureLevels(glw::GLenum texture_target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint max_texture_size = 1024 /* Specification default. */;

	switch (texture_target)
	{
	case GL_TEXTURE_RECTANGLE:
	case GL_TEXTURE_2D_MULTISAMPLE:
		return 1;

	case GL_TEXTURE_2D:
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (glw::GLuint)std::log((double)max_texture_size);

	case GL_TEXTURE_CUBE_MAP:
		gl.getIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (glw::GLuint)std::log((double)max_texture_size);

	default:
		throw 0;
	}

	/* For compiler warnings only. */
	return 0;
}

/** @brief Clear texture.
 */
void TextureAttachmentTest::Clear()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup clear values. */
	gl.clearColor(s_reference_color[0], s_reference_color[1], s_reference_color[2], s_reference_color[3]);
	gl.clearDepth(s_reference_depth);
	gl.clearStencil(s_reference_stencil);

	/* Clear rbo/fbo. */
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/** @brief Clean up GL state.
 */
void TextureAttachmentTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	/* Returning to default clear values. */
	gl.clearColor(0.f, 0.f, 0.f, 0.f);
	gl.clearDepth(1.f);
	gl.clearStencil(0);

	/* Errors clean up. */
	while (gl.getError())
		;
}

/** Tested targets. */
const glw::GLenum TextureAttachmentTest::s_targets[] = { GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D, GL_TEXTURE_2D_MULTISAMPLE,
														 GL_TEXTURE_CUBE_MAP };

/** Tested targets count. */
const glw::GLuint TextureAttachmentTest::s_targets_count = sizeof(s_targets) / sizeof(s_targets[0]);

const glw::GLfloat TextureAttachmentTest::s_reference_color[4]		   = { 0.25, 0.5, 0.75, 1.0 }; //!< Reference color.
const glw::GLint   TextureAttachmentTest::s_reference_color_integer[4] = { 1, 2, 3,
																		 4 }; //!< Reference color for integer format.
const glw::GLfloat TextureAttachmentTest::s_reference_depth   = 0.5;		  //!< Reference depth value.
const glw::GLint   TextureAttachmentTest::s_reference_stencil = 7;			  //!< Reference stencil value.

/******************************** Framebuffer Texture Layer Attachment Test Implementation   ********************************/

/** @brief Framebuffer Texture Layer Attachment Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
TextureLayerAttachmentTest::TextureLayerAttachmentTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_texture_layer_attachment", "Framebuffer Texture Layer Attachment Test")
	, m_fbo(0)
	, m_to(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Framebuffer Texture Layer Attachment Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult TextureLayerAttachmentTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

		gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		for (glw::GLuint i = 0; i < s_targets_count; ++i)
		{
			glw::GLuint layers_counts[] = { (GL_TEXTURE_CUBE_MAP_ARRAY == (glw::GLuint)s_targets[i]) ? 6u : 1u,
											(GL_TEXTURE_CUBE_MAP_ARRAY == (glw::GLuint)s_targets[i]) ? 192u : 192u,
											MaxTextureLayers(s_targets[i]) };

			glw::GLuint layers_counts_count = sizeof(layers_counts) / sizeof(layers_counts[0]);

			for (glw::GLuint j = 1; j <= MaxTextureLevels(s_targets[i]); ++j)
			{
				for (glw::GLuint k = 0; k < layers_counts_count; ++k)
				{
					for (glw::GLint l = 0; l < max_color_attachments; ++l)
					{
						is_ok &= Test(GL_COLOR_ATTACHMENT0 + l, true, s_targets[i], GL_RGBA8, j, layers_counts[k]);
						Clean();
					}

					if (GL_TEXTURE_3D != s_targets[i])
					{
						is_ok &=
							Test(GL_DEPTH_ATTACHMENT, false, s_targets[i], GL_DEPTH_COMPONENT24, j, layers_counts[k]);
						Clean();

						is_ok &= Test(GL_DEPTH_STENCIL_ATTACHMENT, false, s_targets[i], GL_DEPTH24_STENCIL8, j,
									  layers_counts[k]);
						Clean();

						is_ok &=
							Test(GL_STENCIL_ATTACHMENT, false, s_targets[i], GL_STENCIL_INDEX8, j, layers_counts[k]);
						Clean();
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Test texture layer attachment.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] is_color_attachment    Is color attachment tested.
 *  @param [in] texture_target         Texture target.
 *  @param [in] internalformat         Tested internal format.
 *  @param [in] level                  Tested level.
 *  @param [in] levels                 Number of levels.
 *  @param [in] layers                 Number of layers.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool TextureLayerAttachmentTest::Test(glw::GLenum attachment, bool is_color_attachment, glw::GLenum texture_target,
									  glw::GLenum internalformat, glw::GLuint levels, glw::GLint layers)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* RBO creation. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(texture_target, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	// Lower the layers count when multiple levels are requested to limit the amount of memory required
	layers = deMax32(1, (glw::GLint)((deUint64)layers / (1ull<<(deUint64)(2*(levels-1)))));
	if (GL_TEXTURE_CUBE_MAP_ARRAY == texture_target)
	{
		layers = deMax32(6, (layers / 6) * 6);
	}

	if (GL_TEXTURE_2D_MULTISAMPLE_ARRAY == texture_target)
	{
		gl.texStorage3DMultisample(texture_target, 1, internalformat, (glw::GLuint)std::pow((double)2, (double)levels),
								   (glw::GLuint)std::pow((double)2, (double)levels), layers, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");
	}
	else
	{
		gl.texStorage3D(texture_target, levels, internalformat, (glw::GLuint)std::pow((double)2, (double)levels),
						(glw::GLuint)std::pow((double)2, (double)levels), layers);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");
	}

	gl.bindTexture(texture_target, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	/* FBO creation. */
	gl.createFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

	for (glw::GLuint i = 0; i < levels; ++i)
	{
		glw::GLuint j = 0;
		glw::GLuint k = 1;

		/* 3D textures are mipmapped also in depth directio, so number of layers to be tested must be limited. */
		glw::GLuint layers_at_level = (GL_TEXTURE_3D == texture_target) ?
										  (de::min(layers, layers / (glw::GLint)std::pow(2.0, (double)i))) :
										  layers;

		while (j < layers_at_level) /* Only layers with Fibonacci number index are tested to reduce the test time. */
		{
			/* Attach texture layer. */
			gl.namedFramebufferTextureLayer(m_fbo, attachment, m_to, i, j);

			if (!SubTestAttachmentError(attachment, texture_target, i, j, levels, layers))
			{
				return false;
			}

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

			if (is_color_attachment)
			{
				gl.drawBuffer(attachment);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

				gl.readBuffer(attachment);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer has failed");
			}

			if (!SubTestStatus(attachment, texture_target, i, j, levels, layers))
			{
				return false;
			}

			if (GL_TEXTURE_2D_MULTISAMPLE_ARRAY != texture_target)
			{
				Clear();

				if (!SubTestContent(attachment, texture_target, internalformat, i, j, levels, layers))
				{
					return false;
				}
			}

			gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

			/* Fibonacci number iteration. */
			int l = j;
			j	 = j + k;
			k	 = l;
		}
	}

	return true;
}

/** @brief Check error and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] level                  Tested level.
 *  @param [in] layer                  Tested layer.
 *  @param [in] levels                 Number of levels.
 *  @param [in] layers                 Number of layers.
 *
 *  @return True if no error, false otherwise.
 */
bool TextureLayerAttachmentTest::SubTestAttachmentError(glw::GLenum attachment, glw::GLenum texture_target,
														glw::GLuint level, glw::GLint layer, glw::GLuint levels,
														glw::GLint layers)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check and log. */
	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "NamedFramebufferTexture for " << glu::getFramebufferAttachmentStr(attachment)
			<< " attachment of " << glu::getTextureTargetStr(texture_target) << " texture at level " << level
			<< " and at layer " << layer << " where texture has " << levels << " levels and " << layers
			<< " layers failed with error value " << glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check framebuffer completness.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] level                  Tested level.
 *  @param [in] layer                  Tested layer.
 *  @param [in] levels                 Number of levels.
 *  @param [in] layers                 Number of layers.
 *
 *  @return True if framebuffer is complete, false otherwise.
 */
bool TextureLayerAttachmentTest::SubTestStatus(glw::GLenum attachment, glw::GLenum texture_target, glw::GLuint level,
											   glw::GLint layer, glw::GLuint levels, glw::GLint layers)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check framebuffer status. */
	glw::GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);

	if (GL_FRAMEBUFFER_COMPLETE != status)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed because of framebuffer "
			<< glu::getFramebufferStatusStr(status) << " with " << glu::getTextureTargetStr(texture_target)
			<< " texture set up as " << glu::getFramebufferAttachmentStr(attachment) << " attachment and texture level "
			<< level << " and texture layer " << layer << " of texture with " << levels << " levels and " << layers
			<< " layers." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check framebuffer cntent.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *  @param [in] texture_target         Texture target.
 *  @param [in] internalformat         Tested internal format.
 *  @param [in] level                  Tested level.
 *  @param [in] layer                  Tested layer.
 *  @param [in] levels                 Number of levels.
 *  @param [in] layers                 Number of layers.
 *
 *  @return True if framebuffer content is equal to the reference, false otherwise.
 */
bool TextureLayerAttachmentTest::SubTestContent(glw::GLenum attachment, glw::GLenum texture_target,
												glw::GLenum internalformat, glw::GLuint level, glw::GLint layer,
												glw::GLuint levels, glw::GLint layers)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check framebuffer's color content. */
	if (GL_RGBA8 == internalformat)
	{
		glw::GLfloat color[4] = { 0.f };

		gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		for (int i = 0; i < 4 /* color components */; ++i)
		{
			if (de::abs(s_reference_color[i] - color[i]) > 0.0625 /* precision */)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed with "
					<< glu::getTextureTargetStr(texture_target) << " texture set up as "
					<< glu::getFramebufferAttachmentStr(attachment) << " attachment and texture level " << level
					<< " and texture layer " << layer << " of texture with " << levels << " levels and " << layers
					<< " layers. The color content of the framebuffer was [" << color[0] << ", " << color[1] << ", "
					<< color[2] << ", " << color[3] << "], but [" << s_reference_color[0] << ", "
					<< s_reference_color[1] << ", " << s_reference_color[2] << ", " << s_reference_color[3]
					<< "] was expected." << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	/* Check framebuffer's depth content. */
	if ((GL_DEPTH_COMPONENT24 == internalformat) || (GL_DEPTH24_STENCIL8 == internalformat))
	{
		glw::GLfloat depth = 0.f;

		gl.readPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		if (de::abs(s_reference_depth - depth) > 0.0625 /* precision */)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed "
				<< "with texture set up as " << glu::getFramebufferAttachmentStr(attachment)
				<< " attachment and texture level " << level << " and texture layer " << layer << " of texture with "
				<< levels << " levels and " << layers << " layers. The depth content of the framebuffer was [" << depth
				<< "], but [" << s_reference_depth << "] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	/* Check framebuffer's stencil content. */
	if ((GL_STENCIL_INDEX8 == internalformat) || (GL_DEPTH24_STENCIL8 == internalformat))
	{
		glw::GLint stencil = 0;

		gl.readPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_INT, &stencil);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

		if (s_reference_stencil != stencil)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Named Framebuffer Texture Layer Attachment test failed "
				<< "with texture set up as " << glu::getFramebufferAttachmentStr(attachment)
				<< " attachment and texture level " << level << " and texture layer " << layer << " of texture with "
				<< levels << " levels and " << layers << " layers. The stencil content of the framebuffer was ["
				<< stencil << "], but [" << s_reference_stencil << "] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clear framebuffer.
 */
void TextureLayerAttachmentTest::Clear()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup clear values. */
	gl.clearColor(s_reference_color[0], s_reference_color[1], s_reference_color[2], s_reference_color[3]);
	gl.clearDepth(s_reference_depth);
	gl.clearStencil(s_reference_stencil);

	/* Clear rbo/fbo. */
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/** @brief Query maximum number of texture levels.
 *
 *  @return True if max number of texture levels, false otherwise.
 */
glw::GLuint TextureLayerAttachmentTest::MaxTextureLevels(glw::GLenum texture_target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint max_texture_size = 1024 /* Specification default. */;

	switch (texture_target)
	{
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return 1;

	case GL_TEXTURE_2D_ARRAY:

		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (glw::GLuint)std::log((double)max_texture_size);

	case GL_TEXTURE_CUBE_MAP_ARRAY:

		gl.getIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE , &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (glw::GLuint)std::log((double)max_texture_size);

	case GL_TEXTURE_3D:

		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (glw::GLuint)std::log((double)max_texture_size);
	default:
		throw 0;
	}

	/* For compiler warnings only. */
	return 0;
}

/** @brief Query maximum number of texture layers.
 *
 *  @return True if max number of texture layers, false otherwise.
 */
glw::GLuint TextureLayerAttachmentTest::MaxTextureLayers(glw::GLenum texture_target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint max_texture_size = 1024 /* Specification default. */;

	switch (texture_target)
	{
	case GL_TEXTURE_3D:
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return max_texture_size;

	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
	case GL_TEXTURE_2D_ARRAY:

		gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return max_texture_size;

	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

		return (max_texture_size / 6) * 6; /* Make sure that max_texture_size is dividable by 6 */

	default:
		throw 0;
	}

	/* For compiler warnings only. */
	return 0;
}

/** @brief Clean up GL state.
 */
void TextureLayerAttachmentTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleanup. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	/* Returning to default clear values. */
	gl.clearColor(0.f, 0.f, 0.f, 0.f);
	gl.clearDepth(1.f);
	gl.clearStencil(0);

	/* Errors clean up. */
	while (gl.getError())
		;
}

const glw::GLenum TextureLayerAttachmentTest::s_targets[] = //!< Targets to be tested.
	{ GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_3D };

const glw::GLuint TextureLayerAttachmentTest::s_targets_count =
	sizeof(s_targets) / sizeof(s_targets[0]); //!< Number of tested targets.

const glw::GLfloat TextureLayerAttachmentTest::s_reference_color[4] = { 0.25, 0.5, 0.75, 1.0 }; //!< Reference color.
const glw::GLint   TextureLayerAttachmentTest::s_reference_color_integer[4] = { 1, 2, 3,
																			  4 }; //!< Reference integer color.
const glw::GLfloat TextureLayerAttachmentTest::s_reference_depth   = 0.5;		   //!< Reference depth.
const glw::GLint   TextureLayerAttachmentTest::s_reference_stencil = 7;			   //!< Reference stencil index.

/******************************** Named Framebuffer Read / Draw Buffer Test Implementation   ********************************/

/** @brief Named Framebuffer Read / Draw Buffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DrawReadBufferTest::DrawReadBufferTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_read_draw_buffer", "Framebuffer Read and Draw Buffer Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Named Framebuffer Read / Draw Buffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DrawReadBufferTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Framebuffers' objects */
	glw::GLuint framebuffer = 0;

	/* Get number of color attachments. */
	glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	std::vector<glw::GLuint> renderbuffers(max_color_attachments);

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		renderbuffers[i] = 0;
	}

	try
	{
		/* Prepare framebuffer... */
		gl.genFramebuffers(1, &framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(max_color_attachments, &(renderbuffers[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		/* .. with renderbuffer color attachments. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 1, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
		}

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}

		/* Clear each of the framebuffer's attachments with unique color using NamedFramebufferDrawBuffer for attachment selection. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.clearColor((float)i / (float)max_color_attachments, (float)i / (float)max_color_attachments,
						  (float)i / (float)max_color_attachments, (float)i / (float)max_color_attachments);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");

			gl.namedFramebufferDrawBuffer(framebuffer, GL_COLOR_ATTACHMENT0 + i);

			if (glw::GLenum error = gl.getError())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "NamedFramebufferDrawBuffer unexpectedly generated "
					<< glu::getErrorStr(error) << " error with GL_COLOR_ATTACHMENT" << i << ". Test fails."
					<< tcu::TestLog::EndMessage;
				is_ok = false;
			}

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClear has failed");
		}

		/* Fetch framebuffer's content and compare it with reference using NamedFramebufferReadBuffer for attachment selection. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.namedFramebufferReadBuffer(framebuffer, GL_COLOR_ATTACHMENT0 + i);

			if (glw::GLenum error = gl.getError())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "NamedFramebufferReadBuffer unexpectedly generated "
					<< glu::getErrorStr(error) << " error with GL_COLOR_ATTACHMENT" << i << ". Test fails."
					<< tcu::TestLog::EndMessage;
				is_ok = false;
			}

			glw::GLfloat rgba[4] = { -1.f, -1.f, -1.f, -1.f };

			gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, rgba);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

			float expected_value = (float)i / (float)max_color_attachments;

			for (glw::GLuint j = 0; j < 4 /* number of components */; ++j)
			{
				if (de::abs(expected_value - rgba[j]) > 0.0001 /* Precision */)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "Named Framebuffer Draw And Read Buffer failed because resulting color value was ["
						<< rgba[0] << ", " << rgba[1] << ", " << rgba[2] << ", " << rgba[3] << "] but ["
						<< expected_value << ", " << expected_value << ", " << expected_value << ", " << expected_value
						<< "] had been expected." << tcu::TestLog::EndMessage;

					is_ok = false;

					break;
				}
			}
		}

		/* Check that NamedFramebufferDrawBuffer accepts GL_NONE as mode. */
		gl.namedFramebufferDrawBuffer(framebuffer, GL_NONE);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "NamedFramebufferDrawBuffer unexpectedly generated "
				<< glu::getErrorStr(error) << " error with GL_NONE mode. Test fails." << tcu::TestLog::EndMessage;
			is_ok = false;
		}

		/* Check that NamedFramebufferReadBuffer accepts GL_NONE as mode. */
		gl.namedFramebufferReadBuffer(framebuffer, GL_NONE);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "NamedFramebufferReadBuffer unexpectedly generated "
				<< glu::getErrorStr(error) << " error with GL_NONE mode. Test fails." << tcu::TestLog::EndMessage;
			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (framebuffer)
	{
		gl.deleteFramebuffers(1, &framebuffer);

		framebuffer = 0;
	}

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		if (renderbuffers[i])
		{
			gl.deleteRenderbuffers(1, &(renderbuffers[i]));
		}
	}

	gl.clearColor(0.f, 0.f, 0.f, 0.f);

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Named Framebuffer Draw Buffers Test Implementation   ********************************/

/** @brief Named Framebuffer Draw Buffers Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DrawBuffersTest::DrawBuffersTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_draw_buffers", "Framebuffer Draw Buffers Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Named Framebuffer Read / Draw Buffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DrawBuffersTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Framebuffers' objects */
	glw::GLuint framebuffer = 0;

	/* Get number of color attachments. */
	glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	std::vector<glw::GLuint> renderbuffers(max_color_attachments);
	std::vector<glw::GLuint> color_attachments(max_color_attachments);

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		renderbuffers[i]	 = 0;
		color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	try
	{
		/* Prepare framebuffer... */
		gl.genFramebuffers(1, &framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(max_color_attachments, &(renderbuffers[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		/* .. with renderbuffer color attachments. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 1, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
		}

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}

		/* Set up all attachments as draw buffer. */
		gl.namedFramebufferDrawBuffers(framebuffer, max_color_attachments, &(color_attachments[0]));

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "NamedFramebufferDrawBuffers unexpectedly generated "
				<< glu::getErrorStr(error) << " error. Test fails." << tcu::TestLog::EndMessage;
			is_ok = false;
		}

		/* Clear each of the framebuffer's attachments with unique color using NamedFramebufferDrawBuffer for attachment selection. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.clearColor(s_rgba[0], s_rgba[1], s_rgba[2], s_rgba[3]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClear has failed");
		}

		/* Fetch framebuffer's content and compare it with reference using NamedFramebufferReadBuffer for attachment selection. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.readBuffer(GL_COLOR_ATTACHMENT0 + i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer has failed");

			glw::GLfloat rgba[4] = { -1.f, -1.f, -1.f, -1.f };

			gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, rgba);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

			for (glw::GLuint j = 0; j < 4 /* number of components */; ++j)
			{
				if (de::abs(s_rgba[j] - rgba[j]) > 0.0001 /* Precision */)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "Named Framebuffer Draw Buffers test have failed because resulting color value was ["
						<< rgba[0] << ", " << rgba[1] << ", " << rgba[2] << ", " << rgba[3] << "] but [" << s_rgba[0]
						<< ", " << s_rgba[1] << ", " << s_rgba[2] << ", " << s_rgba[3] << "] had been expected."
						<< tcu::TestLog::EndMessage;

					is_ok = false;

					break;
				}
			}
		}

		/* Check that NamedFramebufferDrawBuffers accepts GL_NONE as mode. */
		glw::GLenum none_bufs = GL_NONE;
		gl.namedFramebufferDrawBuffers(framebuffer, 1, &none_bufs);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "NamedFramebufferDrawBuffers unexpectedly generated "
				<< glu::getErrorStr(error) << " error with GL_NONE mode. Test fails." << tcu::TestLog::EndMessage;
			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (framebuffer)
	{
		gl.deleteFramebuffers(1, &framebuffer);

		framebuffer = 0;
	}

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		if (renderbuffers[i])
		{
			gl.deleteRenderbuffers(1, &(renderbuffers[i]));
		}
	}

	gl.clearColor(0.f, 0.f, 0.f, 0.f);

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

const glw::GLfloat DrawBuffersTest::s_rgba[4] = { 0.f, 0.25f, 0.5f, 0.75f };

/******************************** Named Framebuffer Invalidate Data Test Implementation   ********************************/

/** @brief Named Framebuffer Invalidate Data Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
InvalidateDataTest::InvalidateDataTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_invalidate_data", "Framebuffer Invalidate Data Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Named Framebuffer Read / Draw Buffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult InvalidateDataTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Framebuffers' objects */
	glw::GLuint framebuffer = 0;

	/* Get number of color attachments. */
	glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	std::vector<glw::GLuint> renderbuffers(max_color_attachments);
	std::vector<glw::GLuint> color_attachments(max_color_attachments);
	static const glw::GLenum default_attachments[]	 = { GL_COLOR, GL_DEPTH, GL_STENCIL };
	static const glw::GLuint default_attachments_count = sizeof(default_attachments) / sizeof(default_attachments[0]);

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		renderbuffers[i]	 = 0;
		color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	try
	{
		/* Invalidate Default Framebuffer data */
		gl.invalidateNamedFramebufferData(0, default_attachments_count, &(default_attachments[0]));
		is_ok &= CheckErrorAndLog(default_attachments, default_attachments_count);

		for (glw::GLuint i = 0; i < default_attachments_count; ++i)
		{
			gl.invalidateNamedFramebufferData(0, 1, &(default_attachments[i]));
			is_ok &= CheckErrorAndLog(default_attachments[i]);
		}

		/* Prepare framebuffer... */
		gl.genFramebuffers(1, &framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(max_color_attachments, &(renderbuffers[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		/* .. with renderbuffer color attachments. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 1, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
		}

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}

		gl.invalidateNamedFramebufferData(framebuffer, max_color_attachments, &(color_attachments[0]));
		is_ok &= CheckErrorAndLog(&(color_attachments[0]), max_color_attachments);

		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.invalidateNamedFramebufferData(framebuffer, 1, &(color_attachments[i]));
			is_ok &= CheckErrorAndLog(color_attachments[i]);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (framebuffer)
	{
		gl.deleteFramebuffers(1, &framebuffer);

		framebuffer = 0;
	}

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		if (renderbuffers[i])
		{
			gl.deleteRenderbuffers(1, &(renderbuffers[i]));
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Check error and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *
 *  @return True if no error, false otherwise.
 */
bool InvalidateDataTest::CheckErrorAndLog(const glw::GLenum attachment)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check error. */
	if (glw::GLenum error = gl.getError())
	{
		/* There is an error. Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "InvalidateDataTest unexpectedly generated "
											<< glu::getErrorStr(error) << " error for attachment "
											<< glu::getFramebufferAttachmentStr(attachment) << ". Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check error and log.
 *
 *  @param [in] attachments             Framebuffer attachments.
 *  @param [in] attachments_count       Framebuffer attachments count.
 *
 *  @return True if no error, false otherwise.
 */
bool InvalidateDataTest::CheckErrorAndLog(const glw::GLenum attachments[], glw::GLuint count)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check error. */
	if (glw::GLenum error = gl.getError())
	{
		/* There is an error. Log. */
		std::string attachments_names = "";

		for (glw::GLuint i = 0; i < count; ++i)
		{
			attachments_names.append(glu::getFramebufferAttachmentStr(attachments[i]).toString());

			if ((count - 1) != i)
			{
				attachments_names.append(", ");
			}
		}

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "InvalidateDataTest unexpectedly generated "
											<< glu::getErrorStr(error) << " error for following attachments ["
											<< attachments_names << "]. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Named Framebuffer Invalidate Sub Data Test Implementation   ********************************/

/** @brief Named Framebuffer Invalidate Sub Data Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
InvalidateSubDataTest::InvalidateSubDataTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_invalidate_subdata", "Framebuffer Invalidate Sub Data Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Named Framebuffer Read / Draw Buffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult InvalidateSubDataTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Framebuffers' objects */
	glw::GLuint framebuffer = 0;

	/* Get number of color attachments. */
	glw::GLint max_color_attachments = 8 /* Specification minimum OpenGL 4.5 Core Profile p. 627 */;

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	std::vector<glw::GLuint> renderbuffers(max_color_attachments);
	std::vector<glw::GLuint> color_attachments(max_color_attachments);
	static const glw::GLenum default_attachments[]	 = { GL_COLOR, GL_DEPTH, GL_STENCIL };
	static const glw::GLuint default_attachments_count = sizeof(default_attachments) / sizeof(default_attachments[0]);

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		renderbuffers[i]	 = 0;
		color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	try
	{
		/* Invalidate Default Framebuffer data */
		gl.invalidateNamedFramebufferSubData(0, default_attachments_count, &(default_attachments[0]), 0, 0, 1, 1);
		is_ok &= CheckErrorAndLog(default_attachments, default_attachments_count);

		for (glw::GLuint i = 0; i < default_attachments_count; ++i)
		{
			gl.invalidateNamedFramebufferSubData(0, 1, &(default_attachments[i]), 0, 0, 1, 1);
			is_ok &= CheckErrorAndLog(default_attachments[i]);
		}

		/* Prepare framebuffer... */
		gl.genFramebuffers(1, &framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(max_color_attachments, &(renderbuffers[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		/* .. with renderbuffer color attachments. */
		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 4, 4);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, renderbuffers[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
		}

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}

		gl.invalidateNamedFramebufferSubData(framebuffer, max_color_attachments, &(color_attachments[0]), 1, 1, 2, 2);
		is_ok &= CheckErrorAndLog(&(color_attachments[0]), max_color_attachments);

		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			gl.invalidateNamedFramebufferSubData(framebuffer, 1, &(color_attachments[i]), 1, 1, 2, 2);
			is_ok &= CheckErrorAndLog(color_attachments[i]);
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (framebuffer)
	{
		gl.deleteFramebuffers(1, &framebuffer);

		framebuffer = 0;
	}

	for (glw::GLint i = 0; i < max_color_attachments; ++i)
	{
		if (renderbuffers[i])
		{
			gl.deleteRenderbuffers(1, &(renderbuffers[i]));
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Check error and log.
 *
 *  @param [in] attachment             Framebuffer attachment.
 *
 *  @return True if no error, false otherwise.
 */
bool InvalidateSubDataTest::CheckErrorAndLog(const glw::GLenum attachment)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check error. */
	if (glw::GLenum error = gl.getError())
	{
		/* There is an error. Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "InvalidateSubDataTest unexpectedly generated "
											<< glu::getErrorStr(error) << " error for attachment "
											<< glu::getFramebufferAttachmentStr(attachment) << ". Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check error and log.
 *
 *  @param [in] attachments             Framebuffer attachments.
 *  @param [in] attachments_count       Framebuffer attachments count.
 *
 *  @return True if no error, false otherwise.
 */
bool InvalidateSubDataTest::CheckErrorAndLog(const glw::GLenum attachments[], glw::GLuint count)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check error. */
	if (glw::GLenum error = gl.getError())
	{
		/* There is an error. Log. */
		std::string attachments_names = "";

		for (glw::GLuint i = 0; i < count; ++i)
		{
			attachments_names.append(glu::getFramebufferAttachmentStr(attachments[i]).toString());

			if ((count - 1) != i)
			{
				attachments_names.append(", ");
			}
		}

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "InvalidateSubDataTest unexpectedly generated "
											<< glu::getErrorStr(error) << " error for following attachments ["
											<< attachments_names << "]. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Clear Named Framebuffer Test Implementation   ********************************/

/** @brief Clear Named Framebuffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ClearTest::ClearTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_clear", "Clear Named Framebuffer Test")
	, m_fbo(0)
	, m_renderbuffers(0)
	, m_renderbuffers_count(0)
{
	/* Intentionally left blank. */
}

/** @brief Compare two floats (template specialization).
 *
 *  @param [in] first        First  float to be compared.
 *  @param [in] second       Second float to be compared.
 *
 *  @return True if floats are equal within +-(1.f/64.f) precision range, false otherwise.
 */
template <>
bool ClearTest::Compare<glw::GLfloat>(const glw::GLfloat first, const glw::GLfloat second)
{
	return (de::abs(first - second) < (1.f / 64.f) /* Precission. */);
}

/** @brief Compare two objects (template general specialization).
 *
 *  @param [in] first        First  objetc to be compared.
 *  @param [in] second       Second object to be compared.
 *
 *  @return True if floats are equal, false otherwise.
 */
template <typename T>
bool ClearTest::Compare(const T first, const T second)
{
	return (first == second);
}

/** @brief Clear color buffer (float specialization), check errors and log.
 *
 *  @param [in] buffer      Buffer to be cleared.
 *  @param [in] drawbuffer  Drawbuffer to be cleared.
 *  @param [in] value       Value to be cleared with.
 *
 *  @return True if succeeded without errors, false otherwise.
 */
template <>
bool ClearTest::ClearColor<glw::GLfloat>(glw::GLenum buffer, glw::GLint drawbuffer, glw::GLfloat value)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLfloat value_vector[4] = { value, 0, 0, 0 };

	gl.clearNamedFramebufferfv(m_fbo, buffer, drawbuffer, value_vector);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "ClearNamedFramebufferfv unexpectedly generated " << glu::getErrorStr(error)
			<< " error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Clear color buffer (int specialization), check errors and log.
 *
 *  @param [in] buffer      Buffer to be cleared.
 *  @param [in] drawbuffer  Drawbuffer to be cleared.
 *  @param [in] value       Value to be cleared with.
 *
 *  @return True if succeeded without errors, false otherwise.
 */
template <>
bool ClearTest::ClearColor<glw::GLint>(glw::GLenum buffer, glw::GLint drawbuffer, glw::GLint value)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint value_vector[4] = { value, 0, 0, 0 };

	gl.clearNamedFramebufferiv(m_fbo, buffer, drawbuffer, value_vector);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "ClearNamedFramebufferiv unexpectedly generated " << glu::getErrorStr(error)
			<< " error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Clear color buffer (uint specialization), check errors and log.
 *
 *  @param [in] buffer      Buffer to be cleared.
 *  @param [in] drawbuffer  Drawbuffer to be cleared.
 *  @param [in] value       Value to be cleared with.
 *
 *  @return True if succeeded without errors, false otherwise.
 */
template <>
bool ClearTest::ClearColor<glw::GLuint>(glw::GLenum buffer, glw::GLint drawbuffer, glw::GLuint value)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint value_vector[4] = { value, 0, 0, 0 };

	gl.clearNamedFramebufferuiv(m_fbo, buffer, drawbuffer, value_vector);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "ClearNamedFramebufferuiv unexpectedly generated " << glu::getErrorStr(error)
			<< " error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Format of the buffer (float specialization).
 *
 *  @return Format.
 */
template <>
glw::GLenum ClearTest::Format<GLfloat>()
{
	return GL_RED;
}

/** @brief Format of the buffer (int and uint specialization).
 *
 *  @return Format.
 */
template <typename T>
glw::GLenum ClearTest::Format()
{
	return GL_RED_INTEGER;
}

/** @brief Type of the buffer (float specialization).
 *
 *  @return Type.
 */
template <>
glw::GLenum ClearTest::Type<glw::GLfloat>()
{
	return GL_FLOAT;
}

/** @brief Type of the buffer (int specialization).
 *
 *  @return Type.
 */
template <>
glw::GLenum ClearTest::Type<glw::GLint>()
{
	return GL_INT;
}

/** @brief Type of the buffer (uint specialization).
 *
 *  @return Type.
 */
template <>
glw::GLenum ClearTest::Type<glw::GLuint>()
{
	return GL_UNSIGNED_INT;
}

/** @brief Test DSA Clear function (color).
 *
 *  @param [in] buffer      Buffer to be cleared.
 *  @param [in] attachment  Attachment to be tested.
 *  @param [in] value       Value to be cleared with.
 *
 *  @return True if test succeeded, false otherwise.
 */
template <typename T>
bool ClearTest::TestClearColor(glw::GLenum buffer, glw::GLenum attachment, T value)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawBuffer(attachment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer has failed");

	/* Clear. */
	if (ClearColor<T>(buffer, 0, value))
	{
		/* Fetching framebuffer content. */
		T pixel = (T)0;

		gl.readBuffer(attachment);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer has failed");

		gl.readPixels(0, 0, 1, 1, Format<T>(), Type<T>(), &pixel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixel has failed");

		/* Comparison with reference value. */
		if (Compare(pixel, value))
		{
			return true;
		}

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "ClearNamedFramebuffer did not cleared color attachment "
			<< glu::getFramebufferAttachmentStr(attachment) << " of the framebuffer." << tcu::TestLog::EndMessage;
	}

	return false;
}

/** @brief Test DSA Clear function (depth/stencil).
 *
 *  @param [in] stencil     Stencil value to be cleared with.
 *  @param [in] depth       Depth value to be cleared with.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool ClearTest::TestClearDepthAndStencil(glw::GLfloat depth, glw::GLint stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clearing depth and stencil. */
	gl.clearNamedFramebufferfi(m_fbo, GL_DEPTH_STENCIL, 0, depth, stencil);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "ClearNamedFramebufferufi unexpectedly generated " << glu::getErrorStr(error)
			<< " error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	/* Clear. */
	/* Fetching framebuffer content. */
	glw::GLfloat the_depth   = 0.f;
	glw::GLint   the_stencil = 0;

	gl.readPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &the_depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixel has failed");

	gl.readPixels(0, 0, 1, 1, GL_STENCIL_INDEX, GL_INT, &the_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixel has failed");

	/* Comparison with reference value. */
	if (Compare(the_depth, depth) || Compare(the_stencil, stencil))
	{
		return true;
	}

	m_context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "ClearNamedFramebufferfi did not cleared depth/stencil attachment of the framebuffer."
		<< tcu::TestLog::EndMessage;

	return true;
}

/** @brief Iterate Clear Named Framebuffer Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ClearTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Fixed point color test. */
		PrepareFramebuffer(GL_COLOR, GL_R8);

		for (glw::GLint i = 0; i < (glw::GLint)m_renderbuffers_count; ++i)
		{
			is_ok &= TestClearColor<glw::GLfloat>(GL_COLOR, GL_COLOR_ATTACHMENT0 + i, 0.5);
		}

		Clean();

		/* Floating point color test. */
		PrepareFramebuffer(GL_COLOR, GL_R32F);

		for (glw::GLint i = 0; i < (glw::GLint)m_renderbuffers_count; ++i)
		{
			is_ok &= TestClearColor<glw::GLfloat>(GL_COLOR, GL_COLOR_ATTACHMENT0 + i, 0.5);
		}

		Clean();

		/* Signed integer color test. */
		PrepareFramebuffer(GL_COLOR, GL_R8I);

		for (glw::GLint i = 0; i < (glw::GLint)m_renderbuffers_count; ++i)
		{
			is_ok &= TestClearColor<glw::GLint>(GL_COLOR, GL_COLOR_ATTACHMENT0 + i, -16);
		}

		Clean();

		/* Unsigned integer color test. */
		PrepareFramebuffer(GL_COLOR, GL_R8UI);

		for (glw::GLint i = 0; i < (glw::GLint)m_renderbuffers_count; ++i)
		{
			is_ok &= TestClearColor<glw::GLuint>(GL_COLOR, GL_COLOR_ATTACHMENT0 + i, 16);
		}

		Clean();

		/* Depth / stencil test. */
		PrepareFramebuffer(GL_DEPTH_STENCIL, GL_DEPTH24_STENCIL8);

		is_ok &= TestClearDepthAndStencil(1, 1);

		Clean();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare framebuffer.
 *
 *  @param [in] buffer          Buffer to be prepared.
 *  @param [in] internalformat  Internal format to be prepared
 */
void ClearTest::PrepareFramebuffer(glw::GLenum buffer, glw::GLenum internalformat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check that ther is no other fbo. */
	if ((0 != m_fbo) || (DE_NULL != m_renderbuffers))
	{
		throw 0;
	}

	/* Prepare framebuffer... */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	if (buffer == GL_COLOR)
	{
		glw::GLint max_color_attachments =
			8; /* OpenGL 4.5 specification default (see Implementation Dependent Values tables). */

		gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

		m_renderbuffers = new glw::GLuint[max_color_attachments];

		if (m_renderbuffers)
		{
			/* ... with renderbuffer color attachments. */

			gl.genRenderbuffers(max_color_attachments, m_renderbuffers);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

			m_renderbuffers_count = max_color_attachments;

			for (glw::GLint i = 0; i < max_color_attachments; ++i)
			{
				gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

				gl.renderbufferStorage(GL_RENDERBUFFER, internalformat, 1, 1);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

				gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER,
										   m_renderbuffers[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
			}
		}
	}

	if (buffer == GL_DEPTH_STENCIL)
	{
		/* ... with depth and stencil attachments. */

		m_renderbuffers = new glw::GLuint[1];

		if (m_renderbuffers)
		{
			gl.genRenderbuffers(1, m_renderbuffers);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

			gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderbuffers[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			m_renderbuffers_count = 1;

			gl.renderbufferStorage(GL_RENDERBUFFER, internalformat, 1, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
									   m_renderbuffers[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
		}
	}

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** @brief Clean up GL state.
 */
void ClearTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releasing objects. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (DE_NULL != m_renderbuffers)
	{
		if (m_renderbuffers_count)
		{
			gl.deleteRenderbuffers(m_renderbuffers_count, m_renderbuffers);
		}

		delete[] m_renderbuffers;

		m_renderbuffers		  = DE_NULL;
		m_renderbuffers_count = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Blit Named Framebuffer Test Implementation   ********************************/

/** @brief Named Framebuffer blit Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BlitTest::BlitTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_blit", "Framebuffer Blit Test")
	, m_fbo_src(0)
	, m_rbo_color_src(0)
	, m_rbo_depth_stencil_src(0)
	, m_fbo_dst(0)
	, m_rbo_color_dst(0)
	, m_rbo_depth_stencil_dst(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Named Framebuffer Blit Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BlitTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		PrepareFramebuffers();

		is_ok = Test();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare framebuffer.
 */
void BlitTest::PrepareFramebuffers()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare source framebuffer */
	gl.genFramebuffers(1, &m_fbo_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	gl.genRenderbuffers(1, &m_rbo_color_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_color_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 2, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_depth_stencil_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_depth_stencil_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 2, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth_stencil_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	/* Prepare destination framebuffer */
	gl.genFramebuffers(1, &m_fbo_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	gl.genRenderbuffers(1, &m_rbo_color_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_color_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 3, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_depth_stencil_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_depth_stencil_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 3, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth_stencil_dst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Framebuffer is unexpectedly incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** @brief Do the blit test.
 */
bool BlitTest::Test()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_src);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	ClearFramebuffer(1.f, 0.f, 0.f, 0.5f, 1);

	gl.blitNamedFramebuffer(m_fbo_src, m_fbo_dst, 0, 0, 1, 1, 0, 0, 1, 1,
							GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	if (CheckErrorAndLog())
	{
		ClearFramebuffer(0.f, 1.f, 0.f, 0.25f, 2);

		gl.blitNamedFramebuffer(m_fbo_src, m_fbo_dst, 0, 0, 1, 1, 1, 0, 2, 1, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		gl.blitNamedFramebuffer(m_fbo_src, m_fbo_dst, 0, 0, 1, 1, 1, 0, 2, 1,
								GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

		if (CheckErrorAndLog())
		{
			ClearFramebuffer(0.f, 0.f, 1.f, 0.125f, 3);

			gl.blitNamedFramebuffer(m_fbo_src, m_fbo_dst, 0, 0, 2, 2, 2, 0, 3, 1,
									GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

			if (CheckErrorAndLog())
			{
				ClearFramebuffer(1.f, 1.f, 0.f, 0.0625f, 4);

				gl.blitNamedFramebuffer(m_fbo_src, m_fbo_dst, 0, 0, 1, 1, 0, 1, 3, 2,
										GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

				if (CheckErrorAndLog())
				{
					gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_dst);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

					if (CheckColor() && CheckDepth() && CheckStencil())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

/** @brief Check error and log.
 *
 *  @return true if no error, false otherwise.
 */
bool BlitTest::CheckErrorAndLog()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Error query. */
	if (glw::GLenum error = gl.getError())
	{
		/* Log. */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "BlitNamedFramebuffer unexpectedly generated "
											<< glu::getErrorStr(error) << " error." << tcu::TestLog::EndMessage;

		/* Returning result. */
		return false;
	}

	/* Returning result. */
	return true;
}

/** @brief Check color and log.
 *
 *  @return true if color matches reference, false otherwise.
 */
bool BlitTest::CheckColor()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reference values. */
	static const glw::GLfloat reference[2][3][4] = {
		{ { 1.f, 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f, 1.f }, { 0.f, 0.f, 1.f, 1.f } },
		{ { 1.f, 1.f, 0.f, 1.f }, { 1.f, 1.f, 0.f, 1.f }, { 1.f, 1.f, 0.f, 1.f } }
	};

	/* Copy buffer. */
	glw::GLfloat color[2][3][4] = { { { 0 } } };

	/* Reading from GL. */
	gl.readPixels(0, 0, 3, 2, GL_RGBA, GL_FLOAT, color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison against the reference. */
	for (glw::GLuint j = 0; j < 2; ++j)
	{
		for (glw::GLuint i = 0; i < 3; ++i)
		{
			for (glw::GLuint k = 0; k < 4; ++k)
			{
				if (de::abs(reference[j][i][k] - color[j][i][k]) > (1.f / 64.f) /* Precision. */)
				{
					/* Log. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Blitted framebuffer color buffer contains [[" << color[0][0][0]
						<< ", " << color[0][0][1] << ", " << color[0][0][2] << ", " << color[0][0][3] << "], ["
						<< color[0][1][0] << ", " << color[0][1][1] << ", " << color[0][1][2] << ", " << color[0][1][3]
						<< "], [" << color[0][2][0] << ", " << color[0][2][1] << ", " << color[0][2][2] << ", "
						<< color[0][2][3] << "],\n[" << color[1][0][0] << ", " << color[1][0][1] << ", "
						<< color[1][0][2] << ", " << color[1][0][3] << "], [" << color[1][1][0] << ", "
						<< color[1][1][1] << ", " << color[1][1][2] << ", " << color[1][1][3] << "], ["
						<< color[1][2][0] << ", " << color[1][2][1] << ", " << color[1][2][2] << ", " << color[1][2][3]
						<< "]], but\n"
						<< reference[0][0][0] << ", " << reference[0][0][1] << ", " << reference[0][0][2] << ", "
						<< reference[0][0][3] << "], [" << reference[0][1][0] << ", " << reference[0][1][1] << ", "
						<< reference[0][1][2] << ", " << reference[0][1][3] << "], [" << reference[0][2][0] << ", "
						<< reference[0][2][1] << ", " << reference[0][2][2] << ", " << reference[0][2][3] << "],\n["
						<< reference[1][0][0] << ", " << reference[1][0][1] << ", " << reference[1][0][2] << ", "
						<< reference[1][0][3] << "], [" << reference[1][1][0] << ", " << reference[1][1][1] << ", "
						<< reference[1][1][2] << ", " << reference[1][1][3] << "], [" << reference[1][2][0] << ", "
						<< reference[1][2][1] << ", " << reference[1][2][2] << ", " << reference[1][2][3]
						<< "]] was expected.\n"
						<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** @brief Check depth and log.
 *
 *  @return true if depth matches reference, false otherwise.
 */
bool BlitTest::CheckDepth()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reference values. */
	static const glw::GLfloat reference[2][3] = { { 0.5, 0.25, 0.125 }, { 0.0625, 0.0625, 0.0625 } };

	/* Copy buffer. */
	glw::GLfloat depth[2][3] = { { 0 } };

	/* Reading from GL. */
	gl.readPixels(0, 0, 3, 2, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison against the reference. */
	for (glw::GLuint j = 0; j < 2; ++j)
	{
		for (glw::GLuint i = 0; i < 3; ++i)
		{
			if (de::abs(reference[j][i] - depth[j][i]) > (1.f / 64.f) /* Precision. */)
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Blitted framebuffer depth buffer contains [" << depth[0][0] << ", "
					<< depth[0][1] << ", " << depth[0][2] << ", \n"
					<< depth[1][0] << ", " << depth[1][1] << ", " << depth[1][2] << "], but " << reference[0][0] << ", "
					<< reference[0][1] << ", " << reference[0][2] << ", \n"
					<< reference[1][0] << ", " << reference[1][1] << ", " << reference[1][2] << "] was expected."
					<< tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Check stencil and log.
 *
 *  @return true if stencil matches reference, false otherwise.
 */
bool BlitTest::CheckStencil()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reference values. */
	static const glw::GLint reference[2][3] = { { 1, 2, 3 }, { 4, 4, 4 } };

	/* Copy buffer. */
	glw::GLint stencil[2][3] = { { 0 } };

	/* Reading from GL. */
	gl.readPixels(0, 0, 3, 2, GL_STENCIL_INDEX, GL_INT, stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison against the reference. */
	for (glw::GLuint j = 0; j < 2; ++j)
	{
		for (glw::GLuint i = 0; i < 3; ++i)
		{
			if (reference[j][i] != stencil[j][i])
			{
				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Blitted framebuffer stencil buffer contains [" << stencil[0][0] << ", "
					<< stencil[0][1] << ", " << stencil[0][2] << ", \n"
					<< stencil[1][0] << ", " << stencil[1][1] << ", " << stencil[1][2] << "], but " << reference[0][0]
					<< ", " << reference[0][1] << ", " << reference[0][2] << ", \n"
					<< reference[1][0] << ", " << reference[1][1] << ", " << reference[1][2] << "] was expected."
					<< tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Clear framebuffer.
 *
 *  @param [in] red         Color component.
 *  @param [in] green       Color component.
 *  @param [in] blue        Color component.
 *  @param [in] alpha       Color component.
 *  @param [in] depth       Depth component.
 *  @param [in] stencil     Stencil index.
 */
void BlitTest::ClearFramebuffer(glw::GLfloat red, glw::GLfloat green, glw::GLfloat blue, glw::GLfloat depth,
								glw::GLint stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup clear values. */
	gl.clearColor(red, green, blue, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");

	gl.clearDepth(depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");

	gl.clearStencil(stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");

	/* Clearing. */
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor has failed");
}

/** @brief Clean up GL state.
 */
void BlitTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releasing objects. */
	if (m_fbo_src)
	{
		gl.deleteFramebuffers(1, &m_fbo_src);

		m_fbo_src = 0;
	}

	if (m_fbo_dst)
	{
		gl.deleteFramebuffers(1, &m_fbo_dst);

		m_fbo_dst = 0;
	}

	if (m_rbo_color_src)
	{
		gl.deleteRenderbuffers(1, &m_rbo_color_src);

		m_rbo_color_src = 0;
	}

	if (m_rbo_color_dst)
	{
		gl.deleteRenderbuffers(1, &m_rbo_color_dst);

		m_rbo_color_dst = 0;
	}

	if (m_rbo_depth_stencil_src)
	{
		gl.deleteRenderbuffers(1, &m_rbo_depth_stencil_src);

		m_rbo_depth_stencil_src = 0;
	}

	if (m_rbo_depth_stencil_dst)
	{
		gl.deleteRenderbuffers(1, &m_rbo_depth_stencil_dst);

		m_rbo_depth_stencil_dst = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Framebuffer Check Status Test Implementation   ********************************/

/** @brief Check Status Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CheckStatusTest::CheckStatusTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_check_status", "Framebuffer Check Status Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Check Status Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CheckStatusTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool maybe_ok = true;
	bool is_error = false;

	try
	{
		/* The specification is not clear about framebuffer completness. OpenGL 4.5 core profile
		 specification in chapter 9.4.2 says:
		 "The framebuffer object bound to target is said to be framebuffer complete if all
		 the following conditions are true [...]"
		 It does not say that framebuffer is incomplete when any of the conditions are not met.
		 Due to this wording, except for obvious cases (incomplete attachment and missing attachments)
		 other tests ar optional and may result in QP_TEST_RESULT_COMPATIBILITY_WARNING when fail. */
		is_ok &= IncompleteAttachmentTestCase();
		is_ok &= MissingAttachmentTestCase();

		maybe_ok &= IncompleteMultisampleRenderbufferTestCase();
		maybe_ok &= IncompleteMultisampleTextureTestCase();
		maybe_ok &= IncompleteLayerTargetsTestCase();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		if (maybe_ok)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_COMPATIBILITY_WARNING, "Pass with Compatibility Warning");
		}
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Incomplete Attachment Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool CheckStatusTest::IncompleteAttachmentTestCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool is_ok	= true;
	bool is_error = false;

	/* Test objects. */
	glw::GLuint fbo = 0;
	glw::GLuint rbo = 0;

	try
	{
		gl.genFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(1, &rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		glw::GLenum status = 0;

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT != (status = gl.checkNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CheckNamedFramebufferStatus did not return FRAMEBUFFER_INCOMPLETE_ATTACHMENT value. "
				<< glu::getFramebufferStatusStr(status) << " was observed instead." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing obnjects. */
	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
	}

	if (rbo)
	{
		gl.deleteRenderbuffers(1, &rbo);
	}

	if (is_error)
	{
		throw 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	return is_ok;
}

/** Missing Attachment Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool CheckStatusTest::MissingAttachmentTestCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool is_ok	= true;
	bool is_error = false;

	/* Test objects. */
	glw::GLuint fbo = 0;

	try
	{
		gl.genFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		glw::GLenum status = 0;

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT !=
			(status = gl.checkNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CheckNamedFramebufferStatus did not return FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT value. "
				<< glu::getFramebufferStatusStr(status) << " was observed instead." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing obnjects. */
	if (fbo)
	{
		gl.deleteRenderbuffers(1, &fbo);
	}

	if (is_error)
	{
		throw 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	return is_ok;
}

/** Incomplete Multisample Renderbuffer Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool CheckStatusTest::IncompleteMultisampleRenderbufferTestCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool is_ok	= true;
	bool is_error = false;

	/* Test objects. */
	glw::GLuint fbo	= 0;
	glw::GLuint rbo[2] = { 0 };

	try
	{
		gl.genFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genRenderbuffers(2, rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_R8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_R8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		glw::GLenum status = 0;

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE != (status = gl.checkNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CheckNamedFramebufferStatus did not return FRAMEBUFFER_INCOMPLETE_MULTISAMPLE value. "
				<< glu::getFramebufferStatusStr(status) << " was observed instead." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing obnjects. */
	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
	}

	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (rbo[i])
		{
			gl.deleteRenderbuffers(1, &rbo[i]);
		}
	}

	if (is_error)
	{
		throw 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	return is_ok;
}

/** Incomplete Multisample Texture Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool CheckStatusTest::IncompleteMultisampleTextureTestCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool is_ok	= true;
	bool is_error = false;

	/* Test objects. */
	glw::GLuint fbo   = 0;
	glw::GLuint rbo   = 0;
	glw::GLuint to[2] = { 0 };

	try
	{
		gl.genFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genTextures(2, to);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_R8, 1, 1, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample has failed");

		gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, to[0], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, to[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_R8, 1, 1, GL_TRUE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample has failed");

		gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, to[1], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.genRenderbuffers(1, &rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_R8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, rbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		glw::GLenum status = 0;

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE != (status = gl.checkNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CheckNamedFramebufferStatus did not return FRAMEBUFFER_INCOMPLETE_MULTISAMPLE value. "
				<< glu::getFramebufferStatusStr(status) << " was observed instead." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing obnjects. */
	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
	}

	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (to[i])
		{
			gl.deleteTextures(1, &to[i]);
		}
	}

	if (rbo)
	{
		gl.deleteRenderbuffers(1, &rbo);
	}

	if (is_error)
	{
		throw 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	return is_ok;
}

/** Incomplete Layer Targets Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool CheckStatusTest::IncompleteLayerTargetsTestCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool is_ok	= true;
	bool is_error = false;

	/* Test objects. */
	glw::GLuint fbo   = 0;
	glw::GLuint to[2] = { 0 };

	try
	{
		gl.genFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		gl.genTextures(2, to);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

		gl.bindTexture(GL_TEXTURE_3D, to[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texStorage3D(GL_TEXTURE_3D, 1, GL_R8, 2, 2, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample has failed");

		gl.framebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, to[0], 0, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindTexture(GL_TEXTURE_2D, to[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample has failed");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, to[1], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		glw::GLenum status = 0;

		/* Check that framebuffer is complete. */
		if (GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS != (status = gl.checkNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "CheckNamedFramebufferStatus did not return FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS value. "
				<< glu::getFramebufferStatusStr(status) << " was observed instead." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing obnjects. */
	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
	}

	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (to[i])
		{
			gl.deleteTextures(1, &to[i]);
		}
	}

	if (is_error)
	{
		throw 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	return is_ok;
}

/******************************** Get Named Framebuffer Parameters Test Implementation   ********************************/

/** @brief Get Named Framebuffer Parameters Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetParametersTest::GetParametersTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_get_parameters", "Get Named Framebuffer Parameters Test")
	, m_fbo(0)
	, m_rbo(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Check Status Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetParametersTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		is_ok &= TestDefaultFramebuffer();

		PrepareFramebuffer();

		is_ok &= TestCustomFramebuffer();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Prepare framebuffer.
 */
void GetParametersTest::PrepareFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, 1, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Unexpectedly framebuffer was incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** Default framebuffer Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool GetParametersTest::TestDefaultFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	static const glw::GLenum pnames[] = { GL_DOUBLEBUFFER,
										  GL_IMPLEMENTATION_COLOR_READ_FORMAT,
										  GL_IMPLEMENTATION_COLOR_READ_TYPE,
										  GL_SAMPLES,
										  GL_SAMPLE_BUFFERS,
										  GL_STEREO };

	static const glw::GLchar* pnames_strings[] = { "GL_DOUBLEBUFFER",
												   "GL_IMPLEMENTATION_COLOR_READ_FORMAT",
												   "GL_IMPLEMENTATION_COLOR_READ_TYPE",
												   "GL_SAMPLES",
												   "GL_SAMPLE_BUFFERS",
												   "GL_STEREO" };

	glw::GLuint pnames_count = sizeof(pnames) / sizeof(pnames[0]);

	for (glw::GLuint i = 0; i < pnames_count; ++i)
	{
		glw::GLint parameter_legacy = 0;
		glw::GLint parameter_dsa	= 0;

		gl.getFramebufferParameteriv(GL_FRAMEBUFFER, pnames[i], &parameter_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

		gl.getNamedFramebufferParameteriv(0, pnames[i], &parameter_dsa);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferParameteriv unexpectedly generated "
				<< glu::getErrorStr(error) << " error when called with " << pnames_strings[i]
				<< " parameter name for default framebuffer." << tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_legacy != parameter_dsa)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetNamedFramebufferParameteriv returned "
												<< parameter_dsa << ", but " << parameter_legacy << " was expected for "
												<< pnames_strings[i] << " parameter name of default object."
												<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}

	return is_ok;
}

/** Framebuffer Object Test Case
 *
 *  @return True if test case succeeded, false otherwise.
 */
bool GetParametersTest::TestCustomFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	static const glw::GLenum pnames[] = { GL_DOUBLEBUFFER,
										  GL_IMPLEMENTATION_COLOR_READ_FORMAT,
										  GL_IMPLEMENTATION_COLOR_READ_TYPE,
										  GL_SAMPLES,
										  GL_SAMPLE_BUFFERS,
										  GL_STEREO,
										  GL_FRAMEBUFFER_DEFAULT_WIDTH,
										  GL_FRAMEBUFFER_DEFAULT_HEIGHT,
										  GL_FRAMEBUFFER_DEFAULT_LAYERS,
										  GL_FRAMEBUFFER_DEFAULT_SAMPLES,
										  GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS };

	static const glw::GLchar* pnames_strings[] = { "GL_DOUBLEBUFFER",
												   "GL_IMPLEMENTATION_COLOR_READ_FORMAT",
												   "GL_IMPLEMENTATION_COLOR_READ_TYPE",
												   "GL_SAMPLES",
												   "GL_SAMPLE_BUFFERS",
												   "GL_STEREO",
												   "FRAMEBUFFER_DEFAULT_WIDTH",
												   "FRAMEBUFFER_DEFAULT_HEIGHT",
												   "FRAMEBUFFER_DEFAULT_LAYERS",
												   "FRAMEBUFFER_DEFAULT_SAMPLES",
												   "FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS" };

	glw::GLuint pnames_count = sizeof(pnames) / sizeof(pnames[0]);

	for (glw::GLuint i = 0; i < pnames_count; ++i)
	{
		glw::GLint parameter_legacy = 0;
		glw::GLint parameter_dsa	= 0;

		gl.getFramebufferParameteriv(GL_FRAMEBUFFER, pnames[i], &parameter_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

		gl.getNamedFramebufferParameteriv(m_fbo, pnames[i], &parameter_dsa);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferParameteriv unexpectedly generated "
				<< glu::getErrorStr(error) << " error when called with " << pnames_strings[i]
				<< " parameter name for framebuffer object." << tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_legacy != parameter_dsa)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetNamedFramebufferParameteriv returned "
												<< parameter_dsa << ", but " << parameter_legacy << " was expected for "
												<< pnames_strings[i] << " parameter name of framebuffer object."
												<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}

	return is_ok;
}

/** @brief Clean up GL state.
 */
void GetParametersTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releasing obnjects. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Get Named Framebuffer Attachment Parameters Test Implementation   ********************************/

/** @brief Get Named Framebuffer Parameters Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetAttachmentParametersTest::GetAttachmentParametersTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_get_attachment_parameters",
					 "Get Named Framebuffer Attachment Parameters Test")
	, m_fbo(0)
{
	memset(m_rbo, 0, sizeof(m_rbo));
	memset(m_to, 0, sizeof(m_to));
}

/** @brief Iterate Get Named Framebuffer Attachment Parameters Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetAttachmentParametersTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Default framebuffer. */
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

		is_ok &= TestDefaultFramebuffer();

		/* Framebuffer object with renderbuffer attachments (depth only). */
		CreateRenderbufferFramebuffer(true, false);

		is_ok &= TestRenderbufferFramebuffer(false);

		Clean();

		/* Framebuffer object with renderbuffer attachments (stencil only). */
		CreateRenderbufferFramebuffer(false, true);

		is_ok &= TestRenderbufferFramebuffer(false);

		Clean();

		/* Framebuffer object with renderbuffer attachments (depth-stencil merged). */
		CreateRenderbufferFramebuffer(true, true);

		is_ok &= TestRenderbufferFramebuffer(true);

		Clean();

		/* Framebuffer object with texture attachments (depth only). */
		CreateTextureFramebuffer(true, false);

		is_ok &= TestTextureFramebuffer(false);

		Clean();

		/* Framebuffer object with texture attachments (stencil only). */
		CreateTextureFramebuffer(false, true);

		is_ok &= TestTextureFramebuffer(false);

		Clean();

		/* Framebuffer object with texture attachments (depth-stencil merged). */
		CreateTextureFramebuffer(true, true);

		is_ok &= TestTextureFramebuffer(true);

		Clean();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Create framebuffer with renderbuffer
 *
 *  @param [in] depth   Prepare framebuffer with depth buffer.
 *  @param [in] stencil   Prepare framebuffer with stencil buffer.
 *
 *  @note If both depth and stencil, the joined dept_stencil buffer will be created.
 */
void GetAttachmentParametersTest::CreateRenderbufferFramebuffer(bool depth, bool stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	gl.genRenderbuffers(2, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	if (depth && (!stencil))
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	if ((!depth) && stencil)
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	if (depth && stencil)
	{
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Unexpectedly framebuffer was incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** Create framebuffer with tetxure
 *
 *  @param [in] depth   Prepare framebuffer with depth buffer.
 *  @param [in] stencil   Prepare framebuffer with stencil buffer.
 *
 *  @note If both depth and stencil, the joined dept_stencil buffer will be created.
 */
void GetAttachmentParametersTest::CreateTextureFramebuffer(bool depth, bool stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers has failed");

	gl.genTextures(2, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to[0], 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	if (depth && (!stencil))
	{
		gl.bindTexture(GL_TEXTURE_2D, m_to[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_to[1], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	if ((!depth) && stencil)
	{
		gl.bindTexture(GL_TEXTURE_2D, m_to[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_STENCIL_INDEX8, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_to[1], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	if (depth && stencil)
	{
		gl.bindTexture(GL_TEXTURE_2D, m_to[1]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffers has failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, 1, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_to[1], 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");
	}

	/* Check that framebuffer is complete. */
	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Unexpectedly framebuffer was incomplete."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** Test default framebuffer.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool GetAttachmentParametersTest::TestDefaultFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	static const glw::GLenum attachments[] = { GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT,
											   GL_BACK_RIGHT, GL_DEPTH,		  GL_STENCIL };

	static const glw::GLchar* attachments_strings[] = { "GL_FRONT_LEFT", "GL_FRONT_RIGHT", "GL_BACK_LEFT",
														"GL_BACK_RIGHT", "GL_DEPTH",	   "GL_STENCIL" };

	static const glw::GLuint attachments_count = sizeof(attachments) / sizeof(attachments[0]);

	for (glw::GLuint j = 0; j < attachments_count; ++j)
	{
		glw::GLint parameter_legacy = 0;
		glw::GLint parameter_dsa	= 0;

		gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
											   &parameter_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

		gl.getNamedFramebufferAttachmentParameteriv(0, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
													&parameter_dsa);

		/* Error check. */
		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
				<< glu::getErrorStr(error)
				<< " error when called with GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter name for "
				<< attachments_strings[j] << " attachment of default framebuffer." << tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_legacy != parameter_dsa)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned " << parameter_dsa
				<< ", but " << parameter_legacy
				<< " was expected for GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter name of default framebuffer."
				<< tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_dsa != GL_NONE)
		{
			static const glw::GLenum optional_pnames[] = {
				GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,		  GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,	  GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,	 GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
			};

			static const glw::GLchar* optional_pnames_strings[] = {
				"GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE",		"GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE",		"GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE",		"GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE", "GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING"
			};

			static const glw::GLuint optional_pnames_count = sizeof(optional_pnames) / sizeof(optional_pnames[0]);

			for (glw::GLuint i = 0; i < optional_pnames_count; ++i)
			{
				glw::GLint optional_parameter_legacy = 0;
				glw::GLint optional_parameter_dsa	= 0;

				gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], optional_pnames[i],
													   &optional_parameter_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

				gl.getNamedFramebufferAttachmentParameteriv(0, attachments[j], optional_pnames[i],
															&optional_parameter_dsa);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
						<< glu::getErrorStr(error) << " error when called with " << optional_pnames_strings[i]
						<< " parameter name for " << attachments_strings[j]
						<< " attachment of default framebuffer. The GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE was "
						<< parameter_legacy << "." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}

				if (optional_parameter_legacy != optional_parameter_dsa)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned "
						<< optional_parameter_dsa << ", but " << optional_parameter_legacy << " was expected for "
						<< optional_pnames_strings << " parameter name  for " << attachments_strings[j]
						<< " attachment of default framebuffer." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}
			}
		}
	}

	return is_ok;
}

/** Test framebuffer object (with renderbuffer).
 *
 *  @param [in] depth_stencil       Has framebuffer depth_stencil attachment.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool GetAttachmentParametersTest::TestRenderbufferFramebuffer(bool depth_stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	static const glw::GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT,
											   GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	static const glw::GLchar* attachments_strings[] = { "GL_DEPTH_ATTACHMENT", "GL_STENCIL_ATTACHMENT",
														"GL_DEPTH_STENCIL_ATTACHMENT", "GL_COLOR_ATTACHMENT0",
														"GL_COLOR_ATTACHMENT1" };

	static const glw::GLuint attachments_count = sizeof(attachments) / sizeof(attachments[0]);

	for (glw::GLuint j = 0; j < attachments_count; ++j)
	{
		glw::GLint parameter_legacy = 0;
		glw::GLint parameter_dsa	= 0;

		/* Omit DEPTH_STENCIL_ATTACHMENT attachment if the renderbuffer is not depth-stencil. */
		if (attachments[j] == GL_DEPTH_STENCIL_ATTACHMENT)
		{
			if (!depth_stencil)
			{
				continue;
			}
		}

		gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
											   &parameter_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

		gl.getNamedFramebufferAttachmentParameteriv(m_fbo, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
													&parameter_dsa);

		/* Error check. */
		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
				<< glu::getErrorStr(error)
				<< " error when called with GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter name for "
				<< attachments_strings[j] << " attachment of renderbuffer framebuffer object."
				<< tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_legacy != parameter_dsa)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned " << parameter_dsa
				<< ", but " << parameter_legacy << " was expected for GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter "
												   "name of renderbuffer framebuffer object."
				<< tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_dsa != GL_NONE)
		{
			static const glw::GLenum optional_pnames[] = {
				GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,   GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,	GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,	GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,  GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
				GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
			};

			static const glw::GLchar* optional_pnames_strings[] = {
				"GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME",   "GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE",	"GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE",	"GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE",  "GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE",
				"GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING"
			};

			static const glw::GLuint optional_pnames_count = sizeof(optional_pnames) / sizeof(optional_pnames[0]);

			for (glw::GLuint i = 0; i < optional_pnames_count; ++i)
			{
				/* Omit FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE pname when DEPTH_STENCIL_ATTACHMENT attachment is used. */
				if (attachments[j] == GL_DEPTH_STENCIL_ATTACHMENT)
				{
					if (optional_pnames[i] == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE)
					{
						continue;
					}
				}

				glw::GLint optional_parameter_legacy = 0;
				glw::GLint optional_parameter_dsa	= 0;

				gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], optional_pnames[i],
													   &optional_parameter_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

				gl.getNamedFramebufferAttachmentParameteriv(m_fbo, attachments[j], optional_pnames[i],
															&optional_parameter_dsa);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
						<< glu::getErrorStr(error) << " error when called with " << optional_pnames_strings[i]
						<< " parameter name for " << attachments_strings[j]
						<< " attachment of renderbuffer framebuffer object. The GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE "
						   "was "
						<< parameter_legacy << "." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}

				if (optional_parameter_legacy != optional_parameter_dsa)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned "
						<< optional_parameter_dsa << ", but " << optional_parameter_legacy << " was expected for "
						<< optional_pnames_strings << " parameter name  for " << attachments_strings[j]
						<< " attachment of renderbuffer framebuffer object." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}
			}
		}
	}

	return is_ok;
}

/** Test framebuffer object (with texture).
 *
 *  @param [in] depth_stencil       Has framebuffer depth_stencil attachment.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool GetAttachmentParametersTest::TestTextureFramebuffer(bool depth_stencil)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result. */
	bool is_ok = true;

	static const glw::GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT,
											   GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	static const glw::GLchar* attachments_strings[] = { "GL_DEPTH_ATTACHMENT", "GL_STENCIL_ATTACHMENT",
														"GL_DEPTH_STENCIL_ATTACHMENT", "GL_COLOR_ATTACHMENT0",
														"GL_COLOR_ATTACHMENT1" };

	static const glw::GLuint attachments_count = sizeof(attachments) / sizeof(attachments[0]);

	for (glw::GLuint j = 0; j < attachments_count; ++j)
	{
		/* Omit DEPTH_STENCIL_ATTACHMENT attachment if the renderbuffer is not depth-stencil. */
		if (attachments[j] == GL_DEPTH_STENCIL_ATTACHMENT)
		{
			if (!depth_stencil)
			{
				continue;
			}
		}

		glw::GLint parameter_legacy = 0;
		glw::GLint parameter_dsa	= 0;

		gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
											   &parameter_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

		gl.getNamedFramebufferAttachmentParameteriv(m_fbo, attachments[j], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
													&parameter_dsa);

		/* Error check. */
		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
				<< glu::getErrorStr(error)
				<< " error when called with GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter name for "
				<< attachments_strings[j] << " attachment of texture framebuffer object." << tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_legacy != parameter_dsa)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned " << parameter_dsa
				<< ", but " << parameter_legacy << " was expected for GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE parameter "
												   "name of texture framebuffer object."
				<< tcu::TestLog::EndMessage;

			is_ok = false;

			continue;
		}

		if (parameter_dsa != GL_NONE)
		{
			static const glw::GLenum optional_pnames[] = { GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
														   GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
														   GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
														   GL_FRAMEBUFFER_ATTACHMENT_LAYERED,
														   GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER,
														   GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
														   GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
														   GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING };

			static const glw::GLchar* optional_pnames_strings[] = { "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME",
																	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL",
																	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE",
																	"GL_FRAMEBUFFER_ATTACHMENT_LAYERED",
																	"GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER",
																	"GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE",
																	"GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE",
																	"GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING" };

			static const glw::GLuint optional_pnames_count = sizeof(optional_pnames) / sizeof(optional_pnames[0]);

			for (glw::GLuint i = 0; i < optional_pnames_count; ++i)
			{
				/* Omit FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE pname when DEPTH_STENCIL_ATTACHMENT attachment is used. */
				if (attachments[j] == GL_DEPTH_STENCIL_ATTACHMENT)
				{
					if (optional_pnames[i] == GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE)
					{
						continue;
					}
				}

				glw::GLint optional_parameter_legacy = 0;
				glw::GLint optional_parameter_dsa	= 0;

				gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachments[j], optional_pnames[i],
													   &optional_parameter_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFramebufferParameteriv has failed");

				gl.getNamedFramebufferAttachmentParameteriv(m_fbo, attachments[j], optional_pnames[i],
															&optional_parameter_dsa);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv unexpectedly generated "
						<< glu::getErrorStr(error) << " error when called with " << optional_pnames_strings[i]
						<< " parameter name for " << attachments_strings[j]
						<< " attachment of texture framebuffer object. The GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE was "
						<< parameter_legacy << "." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}

				if (optional_parameter_legacy != optional_parameter_dsa)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedFramebufferAttachmentParameteriv returned "
						<< optional_parameter_dsa << ", but " << optional_parameter_legacy << " was expected for "
						<< optional_pnames_strings << " parameter name  for " << attachments_strings[j]
						<< " attachment of texture framebuffer object." << tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}
			}
		}
	}

	return is_ok;
}

/** @brief Clean up GL state.
 */
void GetAttachmentParametersTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releasing obnjects. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Releasing renderbuffers. */
	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (m_rbo[i])
		{
			gl.deleteRenderbuffers(1, &m_rbo[i]);

			m_rbo[i] = 0;
		}
	}

	/* Releasing textures. */
	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (m_to[i])
		{
			gl.deleteRenderbuffers(1, &m_to[i]);

			m_to[i] = 0;
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Framebuffer Creation Errors Test Implementation   ********************************/

/** @brief Creation Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationErrorsTest::CreationErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_creation_errors", "Framebuffer Objects Creation Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok = true;

	/* Framebuffer object */
	glw::GLuint framebuffer = 0;

	/* Check direct state creation of negative numbers of framebuffers. */
	gl.createFramebuffers(-1, &framebuffer);

	glw::GLenum error = GL_NO_ERROR;

	if (GL_INVALID_VALUE != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "CreateFramebuffers generated " << glu::getErrorStr(error)
			<< " error when called with negative number of framebuffers, but GL_INVALID_VALUE was expected."
			<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Cleanup (sanity). */
	if (framebuffer)
	{
		gl.deleteFramebuffers(1, &framebuffer);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/******************************** Renderbuffer Attachment Errors Test Implementation   ********************************/

/** @brief Renderbuffer Attachment Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
RenderbufferAttachmentErrorsTest::RenderbufferAttachmentErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_renderbuffer_attachment_errors", "Renderbuffer Attachment Errors Test")
	, m_fbo_valid(0)
	, m_rbo_valid(0)
	, m_fbo_invalid(0)
	, m_rbo_invalid(0)
	, m_color_attachment_invalid(0)
	, m_attachment_invalid(0)
	, m_renderbuffer_target_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Renderbuffer Attachment Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult RenderbufferAttachmentErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/* Invalid Framebuffer ID. */
		gl.namedFramebufferRenderbuffer(m_fbo_invalid, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_valid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, false, true, false, true, true);

		/* Invalid color attachment. */
		gl.namedFramebufferRenderbuffer(m_fbo_valid, m_color_attachment_invalid, GL_RENDERBUFFER, m_rbo_valid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, true, false, true, true, true);

		/* Invalid attachment. */
		gl.namedFramebufferRenderbuffer(m_fbo_valid, m_attachment_invalid, GL_RENDERBUFFER, m_rbo_valid);

		is_ok &= ExpectError(GL_INVALID_ENUM, true, false, false, true, true);

		/* Invalid Renderbuffer Target. */
		gl.namedFramebufferRenderbuffer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_renderbuffer_target_invalid, m_rbo_valid);

		is_ok &= ExpectError(GL_INVALID_ENUM, true, true, false, false, true);

		/* Invalid Renderbuffer ID. */
		gl.namedFramebufferRenderbuffer(m_fbo_valid, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_invalid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, true, true, false, true, false);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare test objects.
 */
void RenderbufferAttachmentErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
	while (gl.isRenderbuffer(++m_rbo_invalid))
		;

	/* Max color attachments query. */
	glw::GLint max_color_attachments = 8; /* Spec default. */
	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Invalid color attachment */
	m_color_attachment_invalid = GL_COLOR_ATTACHMENT0 + max_color_attachments;

	/* Invalid attachment. */
	bool is_attachment = true;

	while (is_attachment)
	{
		++m_attachment_invalid;

		is_attachment = false;

		if ((GL_DEPTH_ATTACHMENT == m_attachment_invalid) || (GL_STENCIL_ATTACHMENT == m_attachment_invalid) ||
			(GL_DEPTH_STENCIL_ATTACHMENT == m_attachment_invalid))
		{
			is_attachment = true;
		}

		if (GL_COLOR_ATTACHMENT0 < m_attachment_invalid)
		{
			/* If this unlikely happen this mean that we cannot create invalid attachment which is not DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT and
			 GL_COLOR_ATTACHMENTm where m IS any positive integer number (for m < MAX_COLOR_ATTACHMENTS attachments are valid, and for m >= MAX_COLOR_ATTACHMENTS is invalid, but
			 INVALID_OPERATION shall be generated instead of INVALID_ENUM. Such a situation may need change in the test or in the specification. */
			throw 0;
		}
	}

	/* Invalid renderbuffer target. */
	m_renderbuffer_target_invalid = GL_RENDERBUFFER + 1;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] framebuffer         Framebuffer name to be logged.
 *  @param [in] attachment          Attachment name to be logged.
 *  @param [in] renderbuffertarget  Renderbuffertarget name to be logged.
 *  @param [in] renderbuffer        Renderbuffer name to be logged.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool RenderbufferAttachmentErrorsTest::ExpectError(glw::GLenum expected_error, bool framebuffer, bool attachment,
												   bool color_attachment, bool renderbuffertarget, bool renderbuffer)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "NamedFramebufferRenderbuffer called with "
			<< (framebuffer ? "valid" : "invalid") << " framebuffer, " << (attachment ? "valid" : "invalid")
			<< (color_attachment ? " color" : "") << " attachment, " << (renderbuffertarget ? "valid" : "invalid")
			<< " renderbuffer target, " << (renderbuffer ? "valid" : "invalid")
			<< " renderbuffer was expected to generate " << glu::getErrorStr(expected_error) << ", but "
			<< glu::getErrorStr(error) << " was observed instead." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void RenderbufferAttachmentErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	if (m_rbo_valid)
	{
		gl.deleteRenderbuffers(1, &m_rbo_valid);
		m_rbo_valid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid				  = 0;
	m_rbo_invalid				  = 0;
	m_attachment_invalid		  = 0;
	m_color_attachment_invalid	= 0;
	m_renderbuffer_target_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Texture Attachment Errors Test Implementation   ********************************/

/** @brief Texture Attachment Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
TextureAttachmentErrorsTest::TextureAttachmentErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_texture_attachment_errors", "Texture Attachment Errors Test")
	, m_fbo_valid(0)
	, m_to_valid(0)
	, m_to_3d_valid(0)
	, m_to_array_valid(0)
	, m_to_cubearray_valid(0)
	, m_tbo_valid(0)
	, m_fbo_invalid(0)
	, m_to_invalid(0)
	, m_to_layer_invalid(0)
	, m_color_attachment_invalid(0)
	, m_attachment_invalid(0)
	, m_level_invalid(0)
	, m_max_3d_texture_size(2048)		 /* OpenGL 4.5 Core Profile default values (Table 23.53). */
	, m_max_3d_texture_depth(2048)		 /* == m_max_3d_texture_size or value of GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV. */
	, m_max_array_texture_layers(2048)   /* OpenGL 4.5 Core Profile default values (Table 23.53). */
	, m_max_cube_map_texture_size(16384) /* OpenGL 4.5 Core Profile default values (Table 23.53). */
{
	/* Intentionally left blank. */
}

/** @brief Iterate Texture Attachment Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult TextureAttachmentErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/********** NamedFramebufferTexture **************/

		/* Invalid Framebuffer ID. */
		gl.namedFramebufferTexture(m_fbo_invalid, GL_COLOR_ATTACHMENT0, m_to_valid, 0);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTexture", false, true, false, true, true, "", true);

		/* Invalid Color Attachment. */
		gl.namedFramebufferTexture(m_fbo_valid, m_color_attachment_invalid, m_to_valid, 0);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTexture", true, false, true, true, true, "", true);

		/* Invalid Attachment. */
		gl.namedFramebufferTexture(m_fbo_valid, m_attachment_invalid, m_to_valid, 0);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferTexture", true, false, false, true, true, "", true);

		/* Invalid Texture ID. */
		gl.namedFramebufferTexture(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_invalid, 0);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferTexture", true, true, false, false, true, "", true);

		/* Invalid Level. */
		gl.namedFramebufferTexture(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_valid, m_level_invalid);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferTexture", true, true, false, true, false, "", true);

		/* Buffer texture. */
		gl.namedFramebufferTexture(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_tbo_valid, 0);

		is_ok &=
			ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTexture", true, true, false, true, true, "buffer", true);

		/********** NamedFramebufferTextureLayer **************/

		/* Invalid Framebuffer ID. */
		gl.namedFramebufferTextureLayer(m_fbo_invalid, GL_COLOR_ATTACHMENT0, m_to_array_valid, 0, 0);

		is_ok &=
			ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTextureLayer", false, true, false, true, true, "", true);

		/* Invalid Color Attachment. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, m_color_attachment_invalid, m_to_array_valid, 0, 0);

		is_ok &=
			ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTextureLayer", true, false, true, true, true, "", true);

		/* Invalid Attachment. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, m_attachment_invalid, m_to_array_valid, 0, 0);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferTextureLayer", true, false, false, true, true, "", true);

		/* Invalid Texture ID. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_invalid, 0, 0);

		is_ok &=
			ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTextureLayer", true, true, false, false, true, "", true);

		/* Invalid Level. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_array_valid, m_level_invalid, 0);

		is_ok &=
			ExpectError(GL_INVALID_VALUE, "NamedFramebufferTextureLayer", true, true, false, true, false, "", true);

		/* Buffer texture. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_tbo_valid, 0, 0);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTextureLayer", true, true, false, true, true,
							 "buffer", true);

		/* Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is a three-dimensional
		 texture, and layer is larger than the value of MAX_3D_TEXTURE_SIZE or GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV (if available) minus one. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_3d_valid, 0, m_max_3d_texture_depth);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferTextureLayer", true, true, false, true, true,
							 "3D texture", false);

		/* Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is an array texture,
		 and layer is larger than the value of MAX_ARRAY_TEXTURE_LAYERS minus one. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_array_valid, 0,
										m_max_array_texture_layers);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferTextureLayer", true, true, false, true, true, "array",
							 false);

		/* Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is a cube map array texture,
		 and (layer / 6) is larger than the value of MAX_CUBE_MAP_TEXTURE_SIZE minus one (see section 9.8).
		 Check that INVALID_VALUE error is generated by NamedFramebufferTextureLayer if texture is non-zero
		 and layer is negative. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_cubearray_valid, 0,
										m_max_cube_map_texture_size);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferTextureLayer", true, true, false, true, true,
							 "cuba map array", false);

		/* Check that INVALID_OPERATION error is generated by NamedFramebufferTextureLayer if texture is non-zero
		 and is not the name of a three-dimensional, two-dimensional multisample array, one- or two-dimensional array,
		 or cube map array texture. */
		gl.namedFramebufferTextureLayer(m_fbo_valid, GL_COLOR_ATTACHMENT0, m_to_layer_invalid, 0, 0);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferTextureLayer", true, true, false, false, true,
							 "rectangle", true);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare test GL objects.
 */
void TextureAttachmentErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genTextures(1, &m_to_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.genTextures(1, &m_tbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_BUFFER, m_tbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genTextures(1, &m_to_3d_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_3D, m_to_3d_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genTextures(1, &m_to_array_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_array_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genTextures(1, &m_to_cubearray_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_to_cubearray_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genTextures(1, &m_to_layer_invalid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_RECTANGLE, m_to_layer_invalid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
	while (gl.isTexture(++m_to_invalid))
		;

	/* Max color attachments query. */
	glw::GLint max_color_attachments = 8; /* Spec default. */
	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Invalid color attachment */
	m_color_attachment_invalid = GL_COLOR_ATTACHMENT0 + max_color_attachments;

	/* Invalid attachment. */
	bool is_attachment = true;

	while (is_attachment)
	{
		++m_attachment_invalid;

		is_attachment = false;

		if ((GL_DEPTH_ATTACHMENT == m_attachment_invalid) || (GL_STENCIL_ATTACHMENT == m_attachment_invalid) ||
			(GL_DEPTH_STENCIL_ATTACHMENT == m_attachment_invalid))
		{
			is_attachment = true;
		}

		for (glw::GLint i = 0; i < max_color_attachments; ++i)
		{
			if (GL_COLOR_ATTACHMENT0 == m_attachment_invalid)
			{
				is_attachment = true;
			}
		}
	}

	/* Maximum values */
	gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &m_max_3d_texture_size);
	gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_max_array_texture_layers);
	gl.getIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &m_max_cube_map_texture_size);

	if (m_context.getContextInfo().isExtensionSupported("GL_NV_deep_texture3D"))
	{
		gl.getIntegerv(GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV, &m_max_3d_texture_depth);
	}
	else
	{
		m_max_3d_texture_depth = m_max_3d_texture_size;
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Invalid level. */
	m_level_invalid = -1;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] framebuffer         Framebuffer name to be logged.
 *  @param [in] attachment          Attachment name to be logged.
 *  @param [in] texture             Texture name to be logged.
 *  @param [in] level               Level # to be logged.
 *  @param [in] buffer_texture      Is this buffer texture (special logging case).
 *
 *  @return True if test succeeded, false otherwise.
 */
bool TextureAttachmentErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function_name,
											  bool framebuffer, bool attachment, bool color_attachment, bool texture,
											  bool level, const glw::GLchar* texture_type, bool layer)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << function_name << " called with " << (framebuffer ? "valid" : "invalid")
			<< " framebuffer, " << (attachment ? "valid" : "invalid") << (color_attachment ? " color" : "")
			<< " attachment, " << (texture ? "valid " : "invalid ") << texture_type << " texture, "
			<< (level ? "valid" : "invalid") << " level" << (layer ? "" : ", with invalid layer number")
			<< " was expected to generate " << glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
			<< " was observed instead." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void TextureAttachmentErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	if (m_to_valid)
	{
		gl.deleteTextures(1, &m_to_valid);
		m_to_valid = 0;
	}

	if (m_tbo_valid)
	{
		gl.deleteTextures(1, &m_tbo_valid);
		m_tbo_valid = 0;
	}

	if (m_to_3d_valid)
	{
		gl.deleteTextures(1, &m_to_3d_valid);
		m_to_3d_valid = 0;
	}

	if (m_to_array_valid)
	{
		gl.deleteTextures(1, &m_to_array_valid);
		m_to_array_valid = 0;
	}

	if (m_to_cubearray_valid)
	{
		gl.deleteTextures(1, &m_to_cubearray_valid);
		m_to_cubearray_valid = 0;
	}

	if (m_to_layer_invalid)
	{
		gl.deleteTextures(1, &m_to_layer_invalid);
		m_to_layer_invalid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid		 = 0;
	m_to_invalid		 = 0;
	m_attachment_invalid = 0;
	m_level_invalid		 = 0;

	m_max_3d_texture_size		= 2048;
	m_max_3d_texture_depth		= m_max_3d_texture_size;
	m_max_array_texture_layers  = 2048;
	m_max_cube_map_texture_size = 16384;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Draw Read Buffers Errors Test Implementation   ********************************/

/** @brief Draw Read Buffers Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DrawReadBuffersErrorsTest::DrawReadBuffersErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_draw_read_buffers_errors", "Draw Read Buffers Errors Test Test")
	, m_fbo_valid(0)
	, m_fbo_invalid(0)
	, m_attachment_color(GL_COLOR_ATTACHMENT0)
	, m_attachment_back_left(GL_BACK_LEFT)
	, m_attachment_right(GL_RIGHT)
	, m_attachment_left(GL_LEFT)
	, m_attachment_front(GL_FRONT)
	, m_attachment_front_and_back(GL_FRONT_AND_BACK)
	, m_attachment_back(GL_BACK)
	, m_attachment_invalid(0)
	, m_max_color_attachments(8) /* GL 4.5 default, updated later */
{
	m_attachments_invalid[0] = 0;
	m_attachments_invalid[1] = 0;

	m_attachments_back_invalid[0] = GL_BACK_LEFT;
	m_attachments_back_invalid[1] = GL_BACK;

	m_attachments_too_many_count = 0;

	m_attachments_too_many = DE_NULL;
}

/** @brief Iterate Draw Read Buffers Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DrawReadBuffersErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/*  Check that INVALID_OPERATION error is generated by
		 NamedFramebufferDrawBuffer if framebuffer is not zero or the name of an
		 existing framebuffer object. */
		gl.namedFramebufferDrawBuffer(m_fbo_invalid, m_attachment_color);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffer",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM is generated by NamedFramebufferDrawBuffer if
		 buf is not an accepted value. */
		gl.namedFramebufferDrawBuffer(m_fbo_valid, m_attachment_invalid);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffer", "buf is not an accepted value.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffer
		 if the GL is bound to a draw framebuffer object and the ith argument is
		 a value other than COLOR_ATTACHMENTi or NONE. */
		gl.namedFramebufferDrawBuffer(m_fbo_valid, m_attachment_back_left);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffer",
							 "the GL is bound to a draw framebuffer object and the ith argument is a value other than "
							 "COLOR_ATTACHMENTi or NONE.");

		/*  Check that INVALID_OPERATION error is generated by
		 NamedFramebufferDrawBuffers if framebuffer is not zero or the name of an
		 existing framebuffer object. */
		gl.namedFramebufferDrawBuffers(m_fbo_invalid, 1, &m_attachment_color);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffers",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_VALUE is generated by NamedFramebufferDrawBuffers if n
		 is less than 0. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, -1, &m_attachment_color);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferDrawBuffers", "n is less than 0.");

		/*  Check that INVALID_VALUE is generated by NamedFramebufferDrawBuffers if
		 n is greater than MAX_DRAW_BUFFERS. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, m_attachments_too_many_count, m_attachments_too_many);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedFramebufferDrawBuffers", "n is greater than MAX_DRAW_BUFFERS.");

		/*  Check that INVALID_ENUM is generated by NamedFramebufferDrawBuffers if
		 one of the values in bufs is not an accepted value. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_invalid);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "one of the values in bufs is not an accepted value.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
		 if a symbolic constant other than GL_NONE appears more than once in
		 bufs. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, 2, m_attachments_invalid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffers",
							 "a symbolic constant other than GL_NONE appears more than once in bufs.");

		/*  Check that INVALID_ENUM error is generated by NamedFramebufferDrawBuffers if any value in bufs is FRONT, LEFT, RIGHT, or FRONT_AND_BACK.
		 This restriction applies to both the default framebuffer and framebuffer objects, and exists because these constants may themselves
		 refer to multiple buffers, as shown in table 17.4. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_front);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a framebuffer object is tested and a value in bufs is FRONT.");

		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_left);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a framebuffer object is tested and a value in bufs is LEFT.");

		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_right);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a framebuffer object is tested and a value in bufs is RIGHT.");

		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_front_and_back);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a framebuffer object is tested and a value in bufs is FRONT_AND_BACK.");

		gl.namedFramebufferDrawBuffers(0, 1, &m_attachment_front);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a default framebuffer is tested and a value in bufs is FRONT.");

		gl.namedFramebufferDrawBuffers(0, 1, &m_attachment_left);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a default framebuffer is tested and a value in bufs is LEFT.");

		gl.namedFramebufferDrawBuffers(0, 1, &m_attachment_right);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a default framebuffer is tested and a value in bufs is RIGHT.");

		gl.namedFramebufferDrawBuffers(0, 1, &m_attachment_front_and_back);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedFramebufferDrawBuffers",
							 "a default framebuffer is tested and a value in bufs is FRONT_AND_BACK.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers
		 if any value in bufs is BACK, and n is not one. */
		gl.namedFramebufferDrawBuffers(0, 2, m_attachments_back_invalid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffers",
							 "any value in bufs is BACK, and n is not one.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers if
		 the API call refers to a framebuffer object and one or more of the
		 values in bufs is anything other than NONE or one of the
		 COLOR_ATTACHMENTn tokens. */
		gl.namedFramebufferDrawBuffers(m_fbo_valid, 1, &m_attachment_back_left);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffers",
							 "the API call refers to a framebuffer object and one or more of the values in bufs is "
							 "anything other than NONE or one of the COLOR_ATTACHMENTn tokens.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferDrawBuffers if
		 the API call refers to the default framebuffer and one or more of the
		 values in bufs is one of the COLOR_ATTACHMENTn tokens. */
		gl.namedFramebufferDrawBuffers(0, 1, &m_attachment_color);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferDrawBuffers",
							 "the API call refers to the default framebuffer and one or more of the values in bufs is "
							 "one of the COLOR_ATTACHMENTn tokens.");

		/*  Check that INVALID_OPERATION is generated by NamedFramebufferReadBuffer
		 if framebuffer is not zero or the name of an existing framebuffer
		 object. */
		gl.namedFramebufferReadBuffer(m_fbo_invalid, m_attachment_color);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferReadBuffer",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM is generated by NamedFramebufferReadBuffer if
		 src is not one of the accepted values (tables 17.4 and 17.5 of OpenGL 4.5 Core Profile Specification). */
		gl.namedFramebufferReadBuffer(m_fbo_valid, m_attachment_invalid);

		is_ok &= ExpectError(
			GL_INVALID_ENUM, "NamedFramebufferReadBuffer",
			"src is not one of the accepted values (tables 17.4 and 17.5 of OpenGL 4.5 Core Profile Specification).");

		/* Check that INVALID_OPERATION error is generated by NamedFramebufferReadBuffer if the default framebuffer is
		 affected and src is a value (other than NONE) that does not indicate any of the
		 color buffers allocated to the default framebuffer. */
		gl.namedFramebufferReadBuffer(0, GL_COLOR_ATTACHMENT0);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferReadBuffer",
							 "the default framebuffer is affected and src is a value (other than NONE) that does not "
							 "indicate any of the color buffers allocated to the default framebuffer.");

		/* Check that INVALID_OPERATION error is generated by NamedFramebufferReadBuffer if a framebuffer object is
		 affected, and src is one of the  constants from table 17.4 (other than NONE, or COLOR_ATTACHMENTm where m
		 is greater than or equal to the value of MAX_COLOR_ATTACHMENTS). */
		gl.namedFramebufferReadBuffer(m_fbo_valid, m_attachment_front_and_back);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferReadBuffer",
							 "a framebuffer object is affected, and src is one of the  constants from table 17.4.");

		gl.namedFramebufferReadBuffer(m_fbo_valid, GL_COLOR_ATTACHMENT0 + m_max_color_attachments);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedFramebufferReadBuffer",
							 "a framebuffer object is affected, and src is one of the COLOR_ATTACHMENTm where mis "
							 "greater than or equal to the value of MAX_COLOR_ATTACHMENTS.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void DrawReadBuffersErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;

	/* Invalid attachment. */
	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_max_color_attachments);

	bool is_attachment = true;

	while (is_attachment)
	{
		++m_attachment_invalid;

		is_attachment = false;

		/* Valid attachments are those from tables 17.4, 17.5 and a 17.6 (valid for various functions and framebuffer types).
		 (see OpenGL 4.5 Core Profile Specification) */
		switch (m_attachment_invalid)
		{
		case GL_NONE:
		case GL_FRONT_LEFT:
		case GL_FRONT_RIGHT:
		case GL_BACK_LEFT:
		case GL_BACK_RIGHT:
		case GL_FRONT:
		case GL_BACK:
		case GL_LEFT:
		case GL_RIGHT:
		case GL_FRONT_AND_BACK:
			is_attachment = true;
		};

		for (glw::GLint i = 0; i < m_max_color_attachments; ++i)
		{
			if ((glw::GLenum)(GL_COLOR_ATTACHMENT0 + i) == m_attachment_invalid)
			{
				is_attachment = true;
				break;
			}
		}
	}

	m_attachments_invalid[0] = GL_COLOR_ATTACHMENT0;
	m_attachments_invalid[1] = GL_COLOR_ATTACHMENT0;

	glw::GLint max_draw_buffers = 8; /* Spec default. */
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

	m_attachments_too_many_count = max_draw_buffers + 1;

	m_attachments_too_many = new glw::GLenum[m_attachments_too_many_count];

	m_attachments_too_many[0] = GL_COLOR_ATTACHMENT0;

	for (glw::GLint i = 1; i < m_attachments_too_many_count; ++i)
	{
		m_attachments_too_many[i] = GL_NONE;
	}
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool DrawReadBuffersErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
											const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void DrawReadBuffersErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid			 = 0;
	m_attachment_invalid	 = 0;
	m_attachments_invalid[0] = 0;
	m_attachments_invalid[1] = 0;

	delete m_attachments_too_many;

	m_attachments_too_many = DE_NULL;

	m_attachments_too_many_count = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Invalidate Data and SubData Errors Test Implementation   ********************************/

/** @brief Invalidate SubData Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
InvalidateDataAndSubDataErrorsTest::InvalidateDataAndSubDataErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "invalidate_data_and_subdata_errors", "Invalidate Data and SubData Errors Test")
	, m_fbo_valid(0)
	, m_rbo(0)
	, m_fbo_invalid(0)
	, m_fbo_attachment_valid(0)
	, m_fbo_attachment_invalid(0)
	, m_color_attachment_invalid(0)
	, m_default_attachment_invalid(0)

{
}

/** @brief Iterate Invalidate Data and SubData Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult InvalidateDataAndSubDataErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/******* InvalidateNamedFramebufferData *******/

		/*  Check that INVALID_OPERATION error is generated by InvalidateNamedFramebufferData if framebuffer is not zero or the name of an existing framebuffer object. */
		gl.invalidateNamedFramebufferData(m_fbo_invalid, 1, &m_fbo_attachment_valid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "InvalidateNamedFramebufferData",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM error is generated by InvalidateNamedFramebufferData if a framebuffer object is affected, and
		 any element of of attachments is not one of the values in table {COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT }. */
		gl.invalidateNamedFramebufferData(m_fbo_valid, 1, &m_fbo_attachment_invalid);

		is_ok &= ExpectError(GL_INVALID_ENUM, "InvalidateNamedFramebufferData",
							 "a framebuffer object is affected, and any element of of attachments is not one of the "
							 "values in table { COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, "
							 "DEPTH_STENCIL_ATTACHMENT }.");

		/*  Check that INVALID_OPERATION error is generated by InvalidateNamedFramebufferData if attachments contains COLOR_ATTACHMENTm
		 where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS. */
		gl.invalidateNamedFramebufferData(m_fbo_valid, 1, &m_color_attachment_invalid);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "InvalidateNamedFramebufferData",
							 "attachments contains COLOR_ATTACHMENTm where m is greater than or equal to the value of "
							 "MAX_COLOR_ATTACHMENTS");

		/*  Check that INVALID_ENUM error is generated by
		 InvalidateNamedFramebufferData if the default framebuffer is affected,
		 and any elements of attachments are not one of:
		 -  FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, and BACK_RIGHT, identifying that
		 specific buffer,
		 -  COLOR, which is treated as BACK_LEFT for a double-buffered context
		 and FRONT_LEFT for a single-buffered context,
		 -  DEPTH, identifying the depth buffer,
		 -  STENCIL, identifying the stencil buffer. */
		gl.invalidateNamedFramebufferData(0, 1, &m_default_attachment_invalid);

		is_ok &= ExpectError(GL_INVALID_ENUM, "InvalidateNamedFramebufferData",
							 "the default framebuffer is affected, and any elements of attachments are not one of "
							 "FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT, COLOR, DEPTH, STENCIL.");

		/******* InvalidateNamedFramebufferSubData *******/

		/*  Check that INVALID_OPERATION error is generated by InvalidateNamedFramebufferSubData if framebuffer is not zero or the name of an existing framebuffer object. */
		gl.invalidateNamedFramebufferSubData(m_fbo_invalid, 1, &m_fbo_attachment_valid, 0, 0, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "InvalidateNamedFramebufferSubData",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_VALUE error is generated by InvalidateNamedFramebufferSubData if numAttachments, width, or height is negative. */
		gl.invalidateNamedFramebufferSubData(m_fbo_valid, -1, &m_fbo_attachment_valid, 0, 0, 1, 1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "InvalidateNamedFramebufferSubData", "numAttachments is negative.");

		gl.invalidateNamedFramebufferSubData(m_fbo_valid, 1, &m_fbo_attachment_valid, 0, 0, -1, 1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "InvalidateNamedFramebufferSubData", "width is negative.");

		gl.invalidateNamedFramebufferSubData(m_fbo_valid, 1, &m_fbo_attachment_valid, 0, 0, 1, -1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "InvalidateNamedFramebufferSubData", "height is negative.");

		/*  Check that INVALID_ENUM error is generated by InvalidateNamedFramebufferSubData if a framebuffer object is affected, and
		 any element of of attachments is not one of the values in table {COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT }. */
		gl.invalidateNamedFramebufferSubData(m_fbo_valid, 1, &m_fbo_attachment_invalid, 0, 0, 1, 1);

		is_ok &= ExpectError(GL_INVALID_ENUM, "InvalidateNamedFramebufferSubData",
							 "a framebuffer object is affected, and any element of of attachments is not one of the "
							 "values in table { COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, "
							 "DEPTH_STENCIL_ATTACHMENT }.");

		/*  Check that INVALID_OPERATION error is generated by InvalidateNamedFramebufferSubData if attachments contains COLOR_ATTACHMENTm
		 where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS. */
		gl.invalidateNamedFramebufferSubData(m_fbo_valid, 1, &m_color_attachment_invalid, 0, 0, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "InvalidateNamedFramebufferSubData",
							 "attachments contains COLOR_ATTACHMENTm where m is greater than or equal to the value of "
							 "MAX_COLOR_ATTACHMENTS");

		/*  Check that INVALID_ENUM error is generated by InvalidateNamedFramebufferSubData if the default framebuffer is affected,
		 and any elements of attachments are not one of:
		 -  FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, and BACK_RIGHT, identifying that
		 specific buffer,
		 -  COLOR, which is treated as BACK_LEFT for a double-buffered context
		 and FRONT_LEFT for a single-buffered context,
		 -  DEPTH, identifying the depth buffer,
		 -  STENCIL, identifying the stencil buffer. */
		gl.invalidateNamedFramebufferSubData(0, 1, &m_default_attachment_invalid, 0, 0, 1, 1);

		is_ok &= ExpectError(GL_INVALID_ENUM, "InvalidateNamedFramebufferSubData",
							 "the default framebuffer is affected, and any elements of attachments are not one of "
							 "FRONT_LEFT, FRONT_RIGHT, BACK_LEFT, BACK_RIGHT, COLOR, DEPTH, STENCIL.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void InvalidateDataAndSubDataErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid attachments. */
	m_fbo_attachment_valid = GL_COLOR_ATTACHMENT0;

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, m_fbo_attachment_valid, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
	{
		throw 0;
	}

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;

	/* Invalid framebuffer object attachment. */
	while (true)
	{
		if (GL_COLOR_ATTACHMENT0 < m_fbo_attachment_invalid)
		{
			/* If this unlikely happen this mean that we cannot create invalid attachment which is not DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT and
			 GL_COLOR_ATTACHMENTm where m IS any number (for m < MAX_COLOR_ATTACHMENTS attachments are valid, and for m >= MAX_COLOR_ATTACHMENTS is invalid, but
			 INVALID_OPERATION shall be generated instead of INVALID_ENUM. Such a situation may need change in the test or in the specification. */
			throw 0;
		}

		switch (++m_fbo_attachment_invalid)
		{
		case GL_DEPTH_ATTACHMENT:
		case GL_STENCIL_ATTACHMENT:
		case GL_DEPTH_STENCIL_ATTACHMENT:
			continue;
		};

		break;
	};

	/* Invalid color attachment. */
	glw::GLint max_color_attachments = 8; /* Spec default. */

	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	m_color_attachment_invalid = GL_COLOR_ATTACHMENT0 + max_color_attachments;

	/* Invalid default attachment. */
	while (true)
	{
		switch (++m_default_attachment_invalid)
		{
		case GL_FRONT_LEFT:
		case GL_FRONT_RIGHT:
		case GL_BACK_LEFT:
		case GL_BACK_RIGHT:
		case GL_COLOR:
		case GL_DEPTH:
		case GL_STENCIL:
			continue;
		};

		break;
	}
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool InvalidateDataAndSubDataErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
													 const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void InvalidateDataAndSubDataErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid				 = 0;
	m_fbo_attachment_valid		 = 0;
	m_fbo_attachment_invalid	 = 0;
	m_default_attachment_invalid = 0;
	m_color_attachment_invalid   = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Clear Named Framebuffer Errors Test Implementation   ********************************/

/** @brief Clear Named Framebuffer Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ClearNamedFramebufferErrorsTest::ClearNamedFramebufferErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_clear_errors", "Clear Named Framebuffer Errors Test")
	, m_fbo_valid(0)
	, m_rbo_color(0)
	, m_rbo_depth_stencil(0)
	, m_fbo_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Clear Named Framebuffer Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ClearNamedFramebufferErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		glw::GLint   icolor[4] = {};
		glw::GLuint  ucolor[4] = {};
		glw::GLfloat fcolor[4] = {};

		/*  Check that INVALID_OPERATION is generated by ClearNamedFramebuffer* if
		 framebuffer is not zero or the name of an existing framebuffer object. */
		gl.clearNamedFramebufferiv(m_fbo_invalid, GL_COLOR, 0, icolor);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "ClearNamedFramebufferiv",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		gl.clearNamedFramebufferuiv(m_fbo_invalid, GL_COLOR, 0, ucolor);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "ClearNamedFramebufferuiv",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		gl.clearNamedFramebufferfv(m_fbo_invalid, GL_COLOR, 0, fcolor);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "ClearNamedFramebufferfv",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		gl.clearNamedFramebufferfi(m_fbo_invalid, GL_DEPTH_STENCIL, 0, fcolor[0], icolor[0]);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "ClearNamedFramebufferfi",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM is generated by ClearNamedFramebufferiv if buffer
		 is not COLOR or STENCIL. */
		gl.clearNamedFramebufferiv(m_fbo_valid, GL_DEPTH, 0, icolor);

		is_ok &=
			ExpectError(GL_INVALID_ENUM, "ClearNamedFramebufferiv", "buffer is not COLOR or STENCIL (it is DEPTH).");

		/*  Check that INVALID_ENUM is generated by ClearNamedFramebufferuiv if buffer
		 is not COLOR. */
		gl.clearNamedFramebufferuiv(m_fbo_valid, GL_DEPTH, 0, ucolor);

		is_ok &= ExpectError(GL_INVALID_ENUM, "ClearNamedFramebufferuiv", "buffer is not COLOR (it is DEPTH).");

		/*  Check that INVALID_ENUM is generated by ClearNamedFramebufferfv buffer
		 is not COLOR or DEPTH. */
		gl.clearNamedFramebufferfv(m_fbo_valid, GL_STENCIL, 0, fcolor);

		is_ok &=
			ExpectError(GL_INVALID_ENUM, "ClearNamedFramebufferfv", "buffer is not COLOR or DEPTH (it is STENCIL).");

		/*  Check that INVALID_ENUM is generated by ClearNamedFramebufferfi if buffer
		 is not DEPTH_STENCIL. */
		gl.clearNamedFramebufferfi(m_fbo_valid, GL_COLOR, 0, fcolor[0], icolor[0]);

		is_ok &= ExpectError(GL_INVALID_ENUM, "ClearNamedFramebufferfi", "buffer is not DEPTH_STENCIL.");

		/*  Check that INVALID_VALUE is generated by ClearNamedFramebuffer* if buffer is COLOR drawbuffer is
		 negative, or greater than the value of MAX_DRAW_BUFFERS minus one. */
		gl.clearNamedFramebufferiv(m_fbo_valid, GL_COLOR, -1, icolor);

		is_ok &= ExpectError(
			GL_INVALID_VALUE, "ClearNamedFramebufferiv",
			"buffer is COLOR drawbuffer is negative, or greater than the value of MAX_DRAW_BUFFERS minus one.");

		gl.clearNamedFramebufferuiv(m_fbo_valid, GL_COLOR, -1, ucolor);

		is_ok &= ExpectError(
			GL_INVALID_VALUE, "ClearNamedFramebufferuiv",
			"buffer is COLOR drawbuffer is negative, or greater than the value of MAX_DRAW_BUFFERS minus one.");

		/*  Check that INVALID_VALUE is generated by ClearNamedFramebuffer* if buffer is DEPTH, STENCIL or
		 DEPTH_STENCIL and drawbuffer is not zero. */

		gl.clearNamedFramebufferiv(m_fbo_valid, GL_STENCIL, 1, icolor);

		is_ok &= ExpectError(GL_INVALID_VALUE, "ClearNamedFramebufferiv",
							 "buffer is DEPTH, STENCIL or DEPTH_STENCIL and drawbuffer is not zero.");

		gl.clearNamedFramebufferfv(m_fbo_valid, GL_DEPTH, 1, fcolor);

		is_ok &= ExpectError(GL_INVALID_VALUE, "ClearNamedFramebufferfv",
							 "buffer is DEPTH, STENCIL or DEPTH_STENCIL and drawbuffer is not zero.");

		gl.clearNamedFramebufferfi(m_fbo_valid, GL_DEPTH_STENCIL, 1, fcolor[0], icolor[0]);

		is_ok &= ExpectError(GL_INVALID_VALUE, "ClearNamedFramebufferfi",
							 "buffer is DEPTH, STENCIL or DEPTH_STENCIL and drawbuffer is not zero.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void ClearNamedFramebufferErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool ClearNamedFramebufferErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
												  const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void ClearNamedFramebufferErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	if (m_rbo_color)
	{
		gl.deleteRenderbuffers(1, &m_rbo_color);
		m_rbo_color = 0;
	}

	if (m_rbo_depth_stencil)
	{
		gl.deleteRenderbuffers(1, &m_rbo_depth_stencil);
		m_rbo_depth_stencil = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Check Status Errors Test Implementation   ********************************/

/** @brief Clear Named Framebuffer Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CheckStatusErrorsTest::CheckStatusErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_check_status_errors", "Check Status Errors Test")
	, m_fbo_valid(0)
	, m_fbo_invalid(0)
	, m_target_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Clear Named Framebuffer Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CheckStatusErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/*  Check that INVALID_ENUM is generated by CheckNamedFramebufferStatus if
		 target is not DRAW_FRAMEBUFFER, READ_FRAMEBUFFER or FRAMEBUFFER. */
		gl.checkNamedFramebufferStatus(m_fbo_valid, m_target_invalid);

		is_ok &= ExpectError(GL_INVALID_ENUM, "CheckNamedFramebufferStatus",
							 "target is not DRAW_FRAMEBUFFER, READ_FRAMEBUFFER or FRAMEBUFFER.");

		/*  Check that INVALID_OPERATION is generated by CheckNamedFramebufferStatus
		 if framebuffer is not zero or the name of an existing framebuffer
		 object. */
		gl.checkNamedFramebufferStatus(m_fbo_invalid, GL_FRAMEBUFFER);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "CheckNamedFramebufferStatus",
							 "framebuffer is not zero or the name of an existing framebuffer object.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void CheckStatusErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid target. */
	bool is_target = true;

	while (is_target)
	{
		is_target = false;

		++m_target_invalid;

		if (GL_FRAMEBUFFER == m_target_invalid)
		{
			is_target = true;
		}

		if (GL_READ_FRAMEBUFFER == m_target_invalid)
		{
			is_target = true;
		}

		if (GL_DRAW_FRAMEBUFFER == m_target_invalid)
		{
			is_target = true;
		}
	}
	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool CheckStatusErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
										const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void CheckStatusErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid	= 0;
	m_target_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Get Parameter Errors Test Implementation   ********************************/

/** @brief Get Parameter Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetParameterErrorsTest::GetParameterErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_get_parameter_errors", "Get Parameter Errors Test")
	, m_fbo_valid(0)
	, m_fbo_invalid(0)
	, m_parameter_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Get Parameter Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetParameterErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		glw::GLint return_values_dummy_storage[4];

		/*  Check that INVALID_OPERATION is generated by
		 GetNamedFramebufferParameteriv if framebuffer is not zero or the name of
		 an existing framebuffer object. */
		gl.getNamedFramebufferParameteriv(m_fbo_invalid, GL_SAMPLES, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferParameteriv",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM is generated by GetNamedFramebufferParameteriv
		 if pname is not one of the accepted parameter names. */
		gl.getNamedFramebufferParameteriv(m_fbo_valid, m_parameter_invalid, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_ENUM, "GetNamedFramebufferParameteriv",
							 "pname is not one of the accepted parameter names.");

		/*  Check that INVALID_OPERATION is generated if a default framebuffer is
		 queried, and pname is not one of DOUBLEBUFFER,
		 IMPLEMENTATION_COLOR_READ_FORMAT, IMPLEMENTATION_COLOR_READ_TYPE,
		 SAMPLES, SAMPLE_BUFFERS or STEREO. */
		gl.getNamedFramebufferParameteriv(0, GL_FRAMEBUFFER_DEFAULT_WIDTH, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferParameteriv",
							 "a default framebuffer is queried, and pname is not one of DOUBLEBUFFER, "
							 "IMPLEMENTATION_COLOR_READ_FORMAT, IMPLEMENTATION_COLOR_READ_TYPE, SAMPLES, "
							 "SAMPLE_BUFFERS or STEREO.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void GetParameterErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid target. */
	bool is_parameter = true;

	while (is_parameter)
	{
		is_parameter = false;

		++m_parameter_invalid;

		static const glw::GLenum valid_parameters[] = { GL_FRAMEBUFFER_DEFAULT_WIDTH,
														GL_FRAMEBUFFER_DEFAULT_HEIGHT,
														GL_FRAMEBUFFER_DEFAULT_LAYERS,
														GL_FRAMEBUFFER_DEFAULT_SAMPLES,
														GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS,
														GL_DOUBLEBUFFER,
														GL_IMPLEMENTATION_COLOR_READ_FORMAT,
														GL_IMPLEMENTATION_COLOR_READ_TYPE,
														GL_SAMPLES,
														GL_SAMPLE_BUFFERS,
														GL_STEREO };

		static const glw::GLuint valid_parameters_count = sizeof(valid_parameters) / sizeof(valid_parameters[0]);

		for (glw::GLuint i = 0; i < valid_parameters_count; ++i)
		{
			if (valid_parameters[i] == m_parameter_invalid)
			{
				is_parameter = true;
			}
		}
	}

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool GetParameterErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
										 const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void GetParameterErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid		= 0;
	m_parameter_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Get Attachment Parameter Errors Test Implementation   ********************************/

/** @brief Get Attachment Parameter Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetAttachmentParameterErrorsTest::GetAttachmentParameterErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_get_attachment_parameter_errors", "Get Attachment Parameter Errors Test")
	, m_fbo_valid(0)
	, m_rbo_color(0)
	, m_rbo_depth_stencil(0)
	, m_fbo_invalid(0)
	, m_parameter_invalid(0)
	, m_attachment_invalid(0)
	, m_default_attachment_invalid(0)
	, m_max_color_attachments(8) /* Spec default. */
{
	/* Intentionally left blank. */
}

/** @brief Iterate Get Attachment Parameter Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetAttachmentParameterErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		glw::GLint return_values_dummy_storage[4];

		/*  Check that GL_INVALID_OPERATION is generated by
		 GetNamedFramebufferAttachmentParameteriv if framebuffer is not zero or
		 the name of an existing framebuffer object. */
		gl.getNamedFramebufferAttachmentParameteriv(
			m_fbo_invalid, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferAttachmentParameteriv",
							 "framebuffer is not zero or the name of an existing framebuffer object.");

		/*  Check that INVALID_ENUM is generated by
		 GetNamedFramebufferAttachmentParameteriv if pname is not valid for the
		 value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, as described above. */
		gl.getNamedFramebufferAttachmentParameteriv(
			m_fbo_valid, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, return_values_dummy_storage);

		is_ok &= ExpectError(
			GL_INVALID_ENUM, "GetNamedFramebufferAttachmentParameteriv",
			"pname is not valid for the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, as described above.");

		/*  Check that INVALID_ENUM error is generated if a framebuffer object is queried, attachment
		 is not one of the attachments in table 9.2 (COLOR_ATTACHMENTi, DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT), and attachment is not
		 COLOR_ATTACHMENTm where m is greater than or equal to the value of MAX_COLOR_ATTACHMENTS. */
		gl.getNamedFramebufferAttachmentParameteriv(
			m_fbo_valid, m_attachment_invalid, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, return_values_dummy_storage);

		is_ok &= ExpectError(
			GL_INVALID_ENUM, "GetNamedFramebufferAttachmentParameteriv",
			"attachment is not one of the accepted framebuffer attachment points, as described in specification.");

		/*  Check that INVALID_OPERATION is generated by
		 GetNamedFramebufferAttachmentParameteriv if the value of
		 FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE and pname is not
		 FRAMEBUFFER_ATTACHMENT_OBJECT_NAME or
		 FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE. */
		gl.getNamedFramebufferAttachmentParameteriv(
			m_fbo_valid, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferAttachmentParameteriv",
							 "the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE and pname is not "
							 "FRAMEBUFFER_ATTACHMENT_OBJECT_NAME or FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE.");

		/*  Check that INVALID_OPERATION is generated by
		 GetNamedFramebufferAttachmentParameteriv if attachment is
		 DEPTH_STENCIL_ATTACHMENT and pname is FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE. */
		gl.getNamedFramebufferAttachmentParameteriv(m_fbo_valid, GL_DEPTH_STENCIL_ATTACHMENT,
													GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
													return_values_dummy_storage);

		is_ok &=
			ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferAttachmentParameteriv",
						"attachment is DEPTH_STENCIL_ATTACHMENT and pname is FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE.");

		/*  Check that an INVALID_ENUM error is generated if the default framebuffer is
		 queried and attachment is not one the values FRONT, FRONT_LEFT, FRONT_RIGHT,
		 BACK, BACK_LEFT, BACK_RIGHT, DEPTH, STENCIL. */
		gl.getNamedFramebufferAttachmentParameteriv(
			0, m_default_attachment_invalid, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_ENUM, "GetNamedFramebufferAttachmentParameteriv",
							 "the default framebuffer is queried and attachment is not one the values FRONT, "
							 "FRONT_LEFT, FRONT_RIGHT, BACK, BACK_LEFT, BACK_RIGHT, DEPTH, STENCIL.");

		/*  Check that an INVALID_OPERATION error is generated if a framebuffer object is
		 bound to target and attachment is COLOR_ATTACHMENTm where m is greater than or
		 equal to the value of MAX_COLOR_ATTACHMENTS. */
		gl.getNamedFramebufferAttachmentParameteriv(m_fbo_valid, GL_COLOR_ATTACHMENT0 + m_max_color_attachments,
													GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
													return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferAttachmentParameteriv",
							 "a framebuffer object is bound to target and attachment is COLOR_ATTACHMENTm where m is "
							 "equal to the value of MAX_COLOR_ATTACHMENTS.");

		gl.getNamedFramebufferAttachmentParameteriv(m_fbo_valid, GL_COLOR_ATTACHMENT0 + m_max_color_attachments + 1,
													GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
													return_values_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedFramebufferAttachmentParameteriv",
							 "a framebuffer object is bound to target and attachment is COLOR_ATTACHMENTm where m is "
							 "greater than the value of MAX_COLOR_ATTACHMENTS.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void GetAttachmentParameterErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genFramebuffers(1, &m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	gl.genRenderbuffers(1, &m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth_stencil);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Max color attachments. */
	gl.getIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_max_color_attachments);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Invalid attachment. */
	bool is_attachment = true;

	while (is_attachment)
	{
		is_attachment = false;

		if ((GL_DEPTH_ATTACHMENT == m_attachment_invalid) || (GL_STENCIL_ATTACHMENT == m_attachment_invalid) ||
			(GL_DEPTH_STENCIL_ATTACHMENT == m_attachment_invalid))
		{
			++m_attachment_invalid;
			is_attachment = true;
		}

		if (GL_COLOR_ATTACHMENT0 < m_attachment_invalid)
		{
			/* If this unlikely happen this mean that we cannot create invalid attachment which is not DEPTH_ATTACHMENT, STENCIL_ATTACHMENT, DEPTH_STENCIL_ATTACHMENT and
			 GL_COLOR_ATTACHMENTm where m IS any number (for m < MAX_COLOR_ATTACHMENTS attachments are valid, and for m >= MAX_COLOR_ATTACHMENTS is invalid, but
			 INVALID_OPERATION shall be generated instead of INVALID_ENUM. Such a situation may need change in the test or in the specification. */
			throw 0;
		}
	}

	/* Invalid default framebuffer attachment. */
	bool is_default_attachment = true;

	while (is_default_attachment)
	{
		is_default_attachment = false;

		static const glw::GLenum valid_values[] = { GL_FRONT,	 GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK,
													GL_BACK_LEFT, GL_BACK_RIGHT, GL_DEPTH,		 GL_STENCIL };

		static const glw::GLuint valid_values_count = sizeof(valid_values) / sizeof(valid_values[0]);

		for (glw::GLuint i = 0; i < valid_values_count; ++i)
		{
			if (valid_values[i] == m_default_attachment_invalid)
			{
				m_default_attachment_invalid++;
				is_default_attachment = true;
				break;
			}
		}
	}

	/* Invalid parameter. */
	bool is_parameter = true;

	while (is_parameter)
	{
		is_parameter = false;

		++m_parameter_invalid;

		static const glw::GLenum valid_parameters[] = { GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
														GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
														GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
														GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
														GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
														GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,
														GL_FRAMEBUFFER_ATTACHMENT_LAYERED,
														GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER };

		static const glw::GLuint valid_parameters_count = sizeof(valid_parameters) / sizeof(valid_parameters[0]);

		for (glw::GLuint i = 0; i < valid_parameters_count; ++i)
		{
			if (valid_parameters[i] == m_parameter_invalid)
			{
				is_parameter = true;
			}
		}
	}

	/* Invalid objects. */
	while (gl.isFramebuffer(++m_fbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool GetAttachmentParameterErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
												   const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void GetAttachmentParameterErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_fbo_valid)
	{
		gl.deleteFramebuffers(1, &m_fbo_valid);
		m_fbo_valid = 0;
	}

	if (m_rbo_color)
	{
		gl.deleteRenderbuffers(1, &m_rbo_color);
		m_rbo_color = 0;
	}

	if (m_rbo_depth_stencil)
	{
		gl.deleteRenderbuffers(1, &m_rbo_depth_stencil);
		m_rbo_depth_stencil = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_fbo_invalid				 = 0;
	m_parameter_invalid			 = 0;
	m_attachment_invalid		 = 0;
	m_default_attachment_invalid = 0;
	m_max_color_attachments		 = 8;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Get Attachment Parameter Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "framebuffers_renderbuffers_functional", "Functional Test")
	, m_fbo_1st(0)
	, m_fbo_2nd(0)
	, m_rbo_color(0)
	, m_rbo_depth_stencil(0)
	, m_to_color(0)
	, m_po(0)
	, m_vao_stencil_pass_quad(0)
	, m_vao_depth_pass_quad(0)
	, m_vao_color_pass_quad(0)
	, m_bo_stencil_pass_quad(0)
	, m_bo_depth_pass_quad(0)
	, m_bo_color_pass_quad(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Get Attachment Parameter Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Test. */
		is_ok &= PrepareFirstFramebuffer();
		is_ok &= PrepareSecondFramebuffer();
		is_ok &= ClearFramebuffers();
		PrepareProgram();
		PrepareBuffersAndVertexArrays();
		is_ok &= DrawAndBlit();
		is_ok &= CheckSecondFramebufferContent();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare first framebuffer.
 *
 *  @return True if there is no error, false otherwise.
 */
bool FunctionalTest::PrepareFirstFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Failure of this part shall result in test failure (it is DSA functionality failure). */
	try
	{
		gl.createFramebuffers(1, &m_fbo_1st);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

		gl.createRenderbuffers(1, &m_rbo_color);
		gl.createRenderbuffers(1, &m_rbo_depth_stencil);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateRenderbuffers has failed");

		gl.namedRenderbufferStorage(m_rbo_color, GL_R8, 8, 8);
		gl.namedRenderbufferStorage(m_rbo_depth_stencil, GL_DEPTH24_STENCIL8, 8, 8);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedRenderbufferStorage has failed");

		gl.namedFramebufferRenderbuffer(m_fbo_1st, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_color);
		gl.namedFramebufferRenderbuffer(m_fbo_1st, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo_depth_stencil);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedFramebufferRenderbuffer has failed");

		if (GL_FRAMEBUFFER_COMPLETE != gl.checkNamedFramebufferStatus(m_fbo_1st, GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "CheckNamedFramebufferStatus is incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

/** Prepare second framebuffer.
 *
 *  @return True if there is no error, false otherwise.
 */
bool FunctionalTest::PrepareSecondFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Failure of this part shall result in test internal error (it does not test the DSA functionality). */
	gl.genTextures(1, &m_to_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_R8, 4, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D has failed");

	/* Failure of this part shall result in test failure (it is DSA functionality failure). */
	try
	{
		gl.createFramebuffers(1, &m_fbo_2nd);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateFramebuffers has failed");

		gl.namedFramebufferTexture(m_fbo_2nd, GL_COLOR_ATTACHMENT0, m_to_color, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedFramebufferTexture has failed");

		if (GL_FRAMEBUFFER_COMPLETE != gl.checkNamedFramebufferStatus(m_fbo_2nd, GL_FRAMEBUFFER))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "CheckNamedFramebufferStatus is incomplete."
												<< tcu::TestLog::EndMessage;

			throw 0;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}

/** Clear framebuffers.
 *
 *  @return True if there is no error, false otherwise.
 */
bool FunctionalTest::ClearFramebuffers()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Failure of this part shall result in test failure (it is DSA functionality failure). */
	try
	{
		glw::GLfloat color_value[] = { 0.f, 0.f, 0.f, 0.f };
		glw::GLfloat depth_value   = 0.f;
		glw::GLint   stencil_value = 0;

		/* 1st framebuffer. */
		gl.clearNamedFramebufferfv(m_fbo_1st, GL_COLOR, 0, color_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearNamedFramebufferfv has failed");

		gl.clearNamedFramebufferfi(m_fbo_1st, GL_DEPTH_STENCIL, 0, depth_value, stencil_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearNamedFramebufferfi has failed");

		/* 2nd framebuffer. */
		gl.clearNamedFramebufferfv(m_fbo_1st, GL_COLOR, 0, color_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearNamedFramebufferfv has failed");
	}
	catch (...)
	{
		return false;
	}

	return true;
}

/** Prepare test's GLSL program.
 */
void FunctionalTest::PrepareProgram()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { s_vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */
		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &(shader[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation has failed.\n"
														<< "Shader type: " << glu::getShaderTypeStr(shader[i].type)
														<< "\n"
														<< "Shader compilation error log:\n"
														<< log_text << "\n"
														<< "Shader source code:\n"
														<< shader[i].source << "\n"
														<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** Prepare Vertex Array Objects (one for each draw pass).
 */
void FunctionalTest::PrepareBuffersAndVertexArrays()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query program attribute. */
	glw::GLuint program_attribute = gl.getAttribLocation(m_po, s_attribute);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation call failed.");

	/* Create stencil pass buffer. */
	gl.genVertexArrays(1, &m_vao_stencil_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_stencil_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.genBuffers(1, &m_bo_stencil_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_stencil_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, s_stencil_pass_quad_size, s_stencil_pass_quad, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribPointer(program_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(program_attribute);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	/* Create depth pass buffer. */
	gl.genVertexArrays(1, &m_vao_depth_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_depth_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.genBuffers(1, &m_bo_depth_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_depth_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, s_depth_pass_quad_size, s_depth_pass_quad, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribPointer(program_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(program_attribute);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	/* Create color pass buffer. */
	gl.genVertexArrays(1, &m_vao_color_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao_color_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.genBuffers(1, &m_bo_color_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_color_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, s_color_pass_quad_size, s_color_pass_quad, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribPointer(program_attribute, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(program_attribute);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");
}

/** Do the test draww/blit calls.
 *
 *  @return True if there is no error in DSA functionality, false otherwise.
 */
bool FunctionalTest::DrawAndBlit()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_1st);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.viewport(0, 0, 8, 8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Draw to stencil buffer. */
	gl.bindVertexArray(m_vao_stencil_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.stencilFunc(GL_NEVER, 0, 0xff);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilFunc call failed.");

	gl.stencilOp(GL_INCR, GL_KEEP, GL_KEEP);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilOp call failed.");

	gl.stencilMask(0xff);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilMask call failed.");

	gl.colorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glColorMask call failed.");

	gl.depthMask(GL_FALSE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthMask call failed.");

	gl.enable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.disable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* Draw to depth buffer. */
	gl.bindVertexArray(m_vao_depth_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.stencilFunc(GL_ALWAYS, 0, 0xff);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilFunc call failed.");

	gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilOp call failed.");

	gl.stencilMask(0xff);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilMask call failed.");

	gl.depthFunc(GL_ALWAYS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthFunc call failed.");

	gl.disable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable call failed.");

	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.depthMask(GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthMask call failed.");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* Draw to color buffer. */
	gl.bindVertexArray(m_vao_color_pass_quad);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glColorMask call failed.");

	gl.stencilFunc(GL_EQUAL, 1, 0xff);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilFunc call failed.");

	gl.enable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable call failed.");

	gl.depthFunc(GL_GREATER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthFunc call failed.");

	gl.depthMask(GL_FALSE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthMask call failed.");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* Blit framebuffer content. */
	gl.blitNamedFramebuffer(m_fbo_1st, m_fbo_2nd, 0, 0, 8, 8, 0, 0, 4, 3, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	if (gl.getError())
	{
		return false;
	}

	return true;
}

/** Check resulting framebuffer content.
 *
 *  @return True if content matches the reference false otherwise.
 */
bool FunctionalTest::CheckSecondFramebufferContent()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_2nd);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	glw::GLubyte framebuffer_values[3][4] = {
		{ 0 } /* , ... */
	};

	static const glw::GLubyte reference_values[3][4] = { { 0, 0, 0, 0 }, { 0, 0, 255, 0 }, { 0, 0, 0, 0 } };

	gl.readPixels(0, 0, 4, 3, GL_RED, GL_UNSIGNED_BYTE, framebuffer_values);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

	for (glw::GLuint j = 0; j < 3; ++j)
	{
		for (glw::GLuint i = 0; i < 4; ++i)
		{
			if (reference_values[j][i] != framebuffer_values[j][i])
			{
				return false;
			}
		}
	}

	return true;
}

/** @brief Clean up GL state.
 */
void FunctionalTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releas GL objects. */
	if (m_fbo_1st)
	{
		gl.deleteFramebuffers(1, &m_fbo_1st);

		m_fbo_1st = 0;
	}

	if (m_fbo_2nd)
	{
		gl.deleteFramebuffers(1, &m_fbo_2nd);

		m_fbo_2nd = 0;
	}

	if (m_rbo_color)
	{
		gl.deleteRenderbuffers(1, &m_rbo_color);

		m_rbo_color = 0;
	}

	if (m_rbo_depth_stencil)
	{
		gl.deleteRenderbuffers(1, &m_rbo_depth_stencil);

		m_rbo_depth_stencil = 0;
	}

	if (m_to_color)
	{
		gl.deleteTextures(1, &m_to_color);

		m_to_color = 0;
	}

	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao_stencil_pass_quad)
	{
		gl.deleteBuffers(1, &m_vao_stencil_pass_quad);

		m_vao_stencil_pass_quad = 0;
	}

	if (m_vao_depth_pass_quad)
	{
		gl.deleteBuffers(1, &m_vao_depth_pass_quad);

		m_vao_depth_pass_quad = 0;
	}

	if (m_vao_color_pass_quad)
	{
		gl.deleteBuffers(1, &m_vao_color_pass_quad);

		m_vao_color_pass_quad = 0;
	}

	if (m_bo_stencil_pass_quad)
	{
		gl.deleteBuffers(1, &m_bo_stencil_pass_quad);

		m_bo_stencil_pass_quad = 0;
	}

	if (m_bo_depth_pass_quad)
	{
		gl.deleteBuffers(1, &m_bo_depth_pass_quad);

		m_bo_depth_pass_quad = 0;
	}

	if (m_bo_color_pass_quad)
	{
		gl.deleteBuffers(1, &m_bo_color_pass_quad);

		m_bo_color_pass_quad = 0;
	}

	/* Reseting state. */
	gl.stencilFunc(GL_ALWAYS, 0, 0xff);
	gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	gl.stencilMask(0xff);
	gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	gl.depthFunc(GL_LESS);
	gl.depthMask(GL_TRUE);
	gl.disable(GL_STENCIL_TEST);
	gl.disable(GL_DEPTH_TEST);

	/* Clean errors. */
	while (gl.getError())
		;
}

/** Vertex shader source code. */
const glw::GLchar FunctionalTest::s_vertex_shader[] = "#version 330\n"
													  "\n"
													  "in vec3 position;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    gl_Position = vec4(position, 1.0);\n"
													  "}\n";

/** Fragment shader source code. */
const glw::GLchar FunctionalTest::s_fragment_shader[] = "#version 330\n"
														"\n"
														"out vec4 color;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    color = vec4(1.0);\n"
														"}\n";

/** Vertex shader source code attribute name. */
const glw::GLchar FunctionalTest::s_attribute[] = "position";

/** Stencil pass' geometry to be passed to vertex shader attribute. */
const glw::GLfloat FunctionalTest::s_stencil_pass_quad[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
															 0.5f,  -0.5f, 0.5f, 0.5f,  0.5f, 0.5f };

/** Depth pass' geometry to be passed to vertex shader attribute. */
const glw::GLfloat FunctionalTest::s_depth_pass_quad[] = { -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,
														   1.0f,  -1.0f, -1.0f, 1.0f,  1.0f, -1.0f };

/** Color pass' geometry to be passed to vertex shader attribute. */
const glw::GLfloat FunctionalTest::s_color_pass_quad[] = { -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
														   1.0f,  -1.0f, 0.0f, 1.0f,  1.0f, 0.0f };

const glw::GLuint FunctionalTest::s_stencil_pass_quad_size =
	sizeof(s_stencil_pass_quad); //!< Size of stencil pass' geometry.
const glw::GLuint FunctionalTest::s_depth_pass_quad_size =
	sizeof(s_depth_pass_quad); //!< Size of depth   pass' geometry.
const glw::GLuint FunctionalTest::s_color_pass_quad_size =
	sizeof(s_color_pass_quad); //!< Size of color   pass' geometry.

} // namespace Framebuffers

namespace Renderbuffers
{
/******************************** Renderbuffer Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_creation", "Renderbuffer Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Renderbuffers' objects */
	static const glw::GLuint renderbuffers_count = 2;

	glw::GLuint renderbuffers_legacy[renderbuffers_count] = {};
	glw::GLuint renderbuffers_dsa[renderbuffers_count]	= {};

	try
	{
		/* Check legacy state creation. */
		gl.genRenderbuffers(renderbuffers_count, renderbuffers_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

		for (glw::GLuint i = 0; i < renderbuffers_count; ++i)
		{
			if (gl.isRenderbuffer(renderbuffers_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenRenderbuffers has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		gl.createRenderbuffers(renderbuffers_count, renderbuffers_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateRenderbuffers has failed");

		for (glw::GLuint i = 0; i < renderbuffers_count; ++i)
		{
			if (!gl.isRenderbuffer(renderbuffers_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateRenderbuffers has not created default objects."
													<< tcu::TestLog::EndMessage;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < renderbuffers_count; ++i)
	{
		if (renderbuffers_legacy[i])
		{
			gl.deleteRenderbuffers(1, &renderbuffers_legacy[i]);

			renderbuffers_legacy[i] = 0;
		}

		if (renderbuffers_dsa[i])
		{
			gl.deleteRenderbuffers(1, &renderbuffers_dsa[i]);

			renderbuffers_dsa[i] = 0;
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Renderbuffer Storage Test Implementation   ********************************/

/** @brief Renderbuffer Storage Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageTest::StorageTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_storage", "Renderbuffer Objects Storage Test"), m_fbo(0), m_rbo(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		glw::GLint max_renderbuffer_size = 16384 /* Specification minimum. */;

		gl.getIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		const struct
		{
			glw::GLuint width;
			glw::GLuint height;
		} test_cases[] = { { 1, 1 },
						   { 256, 512 },
						   { 1280, 720 },
						   { (glw::GLuint)max_renderbuffer_size, 1 },
						   { 1, (glw::GLuint)max_renderbuffer_size } };

		const glw::GLuint test_cases_count = sizeof(test_cases) / sizeof(test_cases[0]);

		for (glw::GLuint i = 0; i < test_cases_count; ++i)
		{
			for (glw::GLuint j = 0; j < s_renderbuffer_internalformat_configuration_count; ++j)
			{
				if (PrepareRenderbuffer(s_renderbuffer_internalformat_configuration[j], test_cases[i].width,
										test_cases[i].height))
				{
					Clear(s_renderbuffer_internalformat_configuration[j].isColorIntegralFormat);
					is_ok &= Check(s_renderbuffer_internalformat_configuration[j], test_cases[i].width,
								   test_cases[i].height);
				}
				else
				{
					is_ok = false;
				}

				Clean();
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		Clean();
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare renderbuffer.
 *
 *  @param [in] format              Internal format to be prepared.
 *  @param [in] width               Width of the framebuffer.
 *  @param [in] height              Height of the framebuffer.
 *
 *  @return True if there is no error, false otherwise.
 */
bool StorageTest::PrepareRenderbuffer(StorageTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width,
									  glw::GLuint height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.createRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateRenderbuffers call failed.");

	gl.namedRenderbufferStorage(m_rbo, format.internalformat, width, height);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Renderbuffer storage test failed because NamedRenderbufferStorage generated "
			<< glu::getErrorStr(error) << " error value. Renderbuffers format was "
			<< glu::getInternalFormatParameterStr(format.internalformat) << ", width was " << width << ", height was "
			<< height << "." << tcu::TestLog::EndMessage;
		return false;
	}

	if (format.hasRedComponent || format.hasGreenComponent || format.hasBlueComponent || format.hasAlphaComponent)
	{
		gl.namedFramebufferRenderbuffer(m_fbo, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	}

	if (format.hasDepthComponent)
	{
		gl.namedFramebufferRenderbuffer(m_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
	}

	if (format.hasStencilComponent)
	{
		gl.namedFramebufferRenderbuffer(m_fbo, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);
	}

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		/* Log. */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Renderbuffer storage test failed due to incomplete framebuffer status. Renderbuffers format was "
			<< glu::getInternalFormatParameterStr(format.internalformat) << ", width was " << width << ", height was "
			<< height << "." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** Clear renderbuffer.
 *
 *  @param [in] isColorIntegralFormat       Is this color integral format.
 */
void StorageTest::Clear(bool isColorIntegralFormat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (isColorIntegralFormat)
	{
		gl.clearBufferiv(GL_COLOR, 0, s_reference_color_integer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferiv has failed");
	}
	else
	{
		/* Setup clear values. */
		gl.clearColor(s_reference_color[0], s_reference_color[1], s_reference_color[2], s_reference_color[3]);
		gl.clearDepth(s_reference_depth);
		gl.clearStencil(s_reference_stencil);

		/* Clear rbo/fbo. */
		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

/** Check renderbuffer's content.
 *
 *  @param [in] format              Internal format to be prepared.
 *  @param [in] width               Width of the framebuffer.
 *  @param [in] height              Height of the framebuffer.
 *
 *  @return True if content matches the reference, false otherwise.
 */
bool StorageTest::Check(StorageTest::RenderbufferInternalFormatConfiguration format, glw::GLuint width,
						glw::GLuint height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint size = width * height;

	if (format.hasRedComponent || format.hasGreenComponent || format.hasBlueComponent || format.hasAlphaComponent)
	{
		if (format.isColorIntegralFormat)
		{
			std::vector<glw::GLint> color(size * 4);

			gl.readPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_INT, &color[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

			const bool hasComponent[] = { format.hasRedComponent, format.hasGreenComponent, format.hasBlueComponent,
										  format.hasAlphaComponent };

			static const char* componentName[] = { "red", "green", "blue", "alpha" };

			for (glw::GLuint i = 0; i < size; ++i)
			{
				if (hasComponent[i % 4 /* color components count*/])
				{
					if (de::abs(s_reference_color_integer[i % 4 /* color components count*/] - color[i]) >
						2 /* Precision */)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Renderbuffer storage was cleared with color "
							<< componentName[i % 4 /* color components count*/] << " component equal to "
							<< s_reference_color_integer << ", but fetched value " << color[i]
							<< " is not the same. Renderbuffers format was " << format.internalformat_name
							<< ", width was " << width << ", height was " << height << "." << tcu::TestLog::EndMessage;

						return false;
					}
				}
			}
		}
		else
		{
			std::vector<glw::GLfloat> color(size * 4);

			gl.readPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, &color[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

			const bool hasComponent[] = { format.hasRedComponent, format.hasGreenComponent, format.hasBlueComponent,
										  format.hasAlphaComponent };

			static const char* componentName[] = { "red", "green", "blue", "alpha" };

			for (glw::GLuint i = 0; i < size; ++i)
			{
				if (hasComponent[i % 4 /* color components count*/])
				{
					if (de::abs(s_reference_color[i % 4 /* color components count*/] - color[i]) >
						0.0625 /* precision */)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Renderbuffer storage was cleared with color "
							<< componentName[i % 4 /* color components count*/] << " component equal to "
							<< s_reference_color[i % 4 /* color components count*/] << ", but fetched value "
							<< color[i] << " is not the same. Renderbuffers format was " << format.internalformat_name
							<< ", width was " << width << ", height was " << height << "." << tcu::TestLog::EndMessage;

						return false;
					}
				}
			}
		}
	}

	if (format.hasDepthComponent)
	{
		std::vector<glw::GLfloat> depth(size);

		gl.readPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, &depth[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

		for (glw::GLuint i = 0; i < size; ++i)
		{
			if (de::abs(s_reference_depth - depth[i]) > 0.0625 /* 1/16 precision */)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Renderbuffer storage was cleared with depth component equal to "
					<< s_reference_depth << ", but fetched value " << depth[i]
					<< " is not the same. Renderbuffers format was " << format.internalformat_name << ", width was "
					<< width << ", height was " << height << "." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	if (format.hasStencilComponent)
	{
		std::vector<glw::GLint> stencil(size);

		gl.readPixels(0, 0, width, height, GL_STENCIL_INDEX, GL_INT, &stencil[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

		for (glw::GLuint i = 0; i < size; ++i)
		{
			if (s_reference_stencil != stencil[i])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Renderbuffer storage was cleared with alpha component equal to "
					<< s_reference_stencil << ", but fetched value " << stencil[i]
					<< " is not the same. Renderbuffers format was " << format.internalformat_name << ", width was "
					<< width << ", height was " << height << "." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Clean up GL state.
 */
void StorageTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release objects. */
	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Returning to default clear values. */
	gl.clearColor(0.f, 0.f, 0.f, 0.f);
	gl.clearDepth(1.f);
	gl.clearStencil(0);

	/* Errors clean up. */
	while (gl.getError())
		;
}

/** Internal formats to be tested*/
const struct StorageTest::RenderbufferInternalFormatConfiguration
	StorageTest::s_renderbuffer_internalformat_configuration[] = {
		{ GL_R8, "GL_R8", true, false, false, false, false, false, false },
		{ GL_R16, "GL_R16", true, false, false, false, false, false, false },
		{ GL_RG8, "GL_RG8", true, true, false, false, false, false, false },
		{ GL_RG16, "GL_RG16", true, true, false, false, false, false, false },
		{ GL_RGB565, "GL_RGB56", true, true, true, false, false, false, false },
		{ GL_RGBA4, "GL_RGBA4", true, true, true, true, false, false, false },
		{ GL_RGB5_A1, "GL_RGB5_A1", true, true, true, true, false, false, false },
		{ GL_RGBA8, "GL_RGBA8", true, true, true, true, false, false, false },
		{ GL_RGB10_A2, "GL_RGB10_A2", true, true, true, true, false, false, false },
		{ GL_RGB10_A2UI, "GL_RGB10_A2UI", true, true, true, true, false, false, true },
		{ GL_RGBA16, "GL_RGBA16", true, true, true, true, false, false, false },
		{ GL_SRGB8_ALPHA8, "GL_SRGB8_ALPHA8", true, true, true, true, false, false, false },
		{ GL_R16F, "GL_R16F", true, false, false, false, false, false, false },
		{ GL_RG16F, "GL_RG16F", true, true, false, false, false, false, false },
		{ GL_RGBA16F, "GL_RGBA16F", true, true, true, true, false, false, false },
		{ GL_R32F, "GL_R32F", true, false, false, false, false, false, false },
		{ GL_RG32F, "GL_RG32F", true, true, false, false, false, false, false },
		{ GL_RGBA32F, "GL_RGBA32F", true, true, true, true, false, false, false },
		{ GL_R11F_G11F_B10F, "GL_R11F_G11F_B10F", true, true, true, false, false, false, false },
		{ GL_R8I, "GL_R8I", true, false, false, false, false, false, true },
		{ GL_R8UI, "GL_R8UI", true, false, false, false, false, false, true },
		{ GL_R16I, "GL_R16I", true, false, false, false, false, false, true },
		{ GL_R16UI, "GL_R16UI", true, false, false, false, false, false, true },
		{ GL_R32I, "GL_R32I", true, false, false, false, false, false, true },
		{ GL_R32UI, "GL_R32UI", true, false, false, false, false, false, true },
		{ GL_RG8I, "GL_RG8I", true, true, false, false, false, false, true },
		{ GL_RG8UI, "GL_RG8UI", true, true, false, false, false, false, true },
		{ GL_RG16I, "GL_RG16I", true, true, false, false, false, false, true },
		{ GL_RG16UI, "GL_RG16UI", true, true, false, false, false, false, true },
		{ GL_RG32I, "GL_RG32I", true, true, false, false, false, false, true },
		{ GL_RG32UI, "GL_RG32UI", true, true, false, false, false, false, true },
		{ GL_RGBA8I, "GL_RGBA8I", true, true, true, true, false, false, true },
		{ GL_RGBA8UI, "GL_RGBA8UI", true, true, true, true, false, false, true },
		{ GL_RGBA16I, "GL_RGBA16I", true, true, true, true, false, false, true },
		{ GL_RGBA16UI, "GL_RGBA16UI", true, true, true, true, false, false, true },
		{ GL_RGBA32I, "GL_RGBA32I", true, true, true, true, false, false, true },
		{ GL_RGBA32UI, "GL_RGBA32UI", true, true, true, true, false, false, true },
		{ GL_DEPTH_COMPONENT16, "GL_DEPTH_COMPONENT16", false, false, false, false, true, false, false },
		{ GL_DEPTH_COMPONENT24, "GL_DEPTH_COMPONENT24", false, false, false, false, true, false, false },
		{ GL_DEPTH_COMPONENT32F, "GL_DEPTH_COMPONENT32F", false, false, false, false, true, false, false },
		{ GL_DEPTH24_STENCIL8, "GL_DEPTH24_STENCIL8", false, false, false, false, true, true, false },
		{ GL_DEPTH32F_STENCIL8, "GL_DEPTH32F_STENCIL8", false, false, false, false, true, true, false },
		{ GL_STENCIL_INDEX8, "GL_STENCIL_INDEX8", false, false, false, false, false, true, false }
	};

/** Internal formats count */
const glw::GLuint StorageTest::s_renderbuffer_internalformat_configuration_count =
	sizeof(s_renderbuffer_internalformat_configuration) / sizeof(s_renderbuffer_internalformat_configuration[0]);

const glw::GLfloat StorageTest::s_reference_color[4]		 = { 0.25, 0.5, 0.75, 1.0 }; //!< Reference color.
const glw::GLint   StorageTest::s_reference_color_integer[4] = { 1, 2, 3, 4 };			 //!< Reference integral color.
const glw::GLfloat StorageTest::s_reference_depth			 = 0.5;						 //!< Reference depth.
const glw::GLint   StorageTest::s_reference_stencil			 = 7;						 //!< Reference stencil.

/***************************** Renderbuffer Storage Multisample Test Implementation   ***************************/

/** @brief Renderbuffer Storage Multisample Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageMultisampleTest::StorageMultisampleTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_storage_multisample", "Renderbuffer Objects Storage Multisample Test")
{
	for (glw::GLuint i = 0; i < 2; ++i)
	{
		m_fbo[i] = 0;
		m_rbo[i] = 0;
	}
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageMultisampleTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		glw::GLint max_renderbuffer_size = 16384 /* Specification minimum. */;

		gl.getIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		glw::GLint max_integer_samples = 1 /* Specification minimum. */;

		gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &max_integer_samples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		const struct
		{
			glw::GLuint width;
			glw::GLuint height;
		} test_cases[] = { { 1, 1 },
						   { (glw::GLuint)max_renderbuffer_size / 2, 1 },
						   { 1, (glw::GLuint)max_renderbuffer_size / 2 } };

		const glw::GLuint test_cases_count = sizeof(test_cases) / sizeof(test_cases[0]);

		for (glw::GLuint i = 0; i < test_cases_count; ++i)
		{
			for (glw::GLuint j = 0; j < s_renderbuffer_internalformat_configuration_count; ++j)
			{
				for (glw::GLint k = 0; k <= max_integer_samples; ++k)
				{
					if (PrepareRenderbuffer(s_renderbuffer_internalformat_configuration[j], test_cases[i].width,
											test_cases[i].height, k))
					{
						Bind(GL_DRAW_FRAMEBUFFER, 0);
						Clear(s_renderbuffer_internalformat_configuration[j].isColorIntegralFormat);
						Bind(GL_READ_FRAMEBUFFER, 0);
						Bind(GL_DRAW_FRAMEBUFFER, 1);
						Blit(test_cases[i].width, test_cases[i].height);
						Bind(GL_READ_FRAMEBUFFER, 1);
						is_ok &= Check(s_renderbuffer_internalformat_configuration[j], test_cases[i].width,
									   test_cases[i].height);
					}
					else
					{
						is_ok = false;
					}

					Clean();
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		Clean();
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Prepare renderbuffer.
 *
 *  @param [in] format              Internal format to be prepared.
 *  @param [in] width               Width of the framebuffer.
 *  @param [in] height              Height of the framebuffer.
 *
 *  @return True if there is no error, false otherwise.
 */
bool StorageMultisampleTest::PrepareRenderbuffer(StorageMultisampleTest::RenderbufferInternalFormatConfiguration format,
												 glw::GLuint width, glw::GLuint height, glw::GLsizei samples)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(2, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.createRenderbuffers(2, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateRenderbuffers call failed.");

	for (glw::GLuint i = 0; i < 2; ++i)
	{
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

		if (i)
		{
			/* 2nd is not multisampled. */
			gl.namedRenderbufferStorageMultisample(m_rbo[i], 0, format.internalformat, width, height);

			if (glw::GLenum error = gl.getError())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Renderbuffer storage multisample test failed because "
												"NamedRenderbufferStorageMultisample generated "
					<< glu::getErrorStr(error) << " error value. Renderbuffers format was "
					<< format.internalformat_name << ", samples was " << 0 << ", width was " << width << ", height was "
					<< height << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		else
		{
			/* 1st is multisampled. */
			gl.namedRenderbufferStorageMultisample(m_rbo[i], samples, format.internalformat, width, height);

			if (glw::GLenum error = gl.getError())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Renderbuffer storage multisample test failed because "
												"NamedRenderbufferStorageMultisample generated "
					<< glu::getErrorStr(error) << " error value. Renderbuffers format was "
					<< format.internalformat_name << ", samples was " << samples << ", width was " << width
					<< ", height was " << height << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		if (format.hasRedComponent || format.hasGreenComponent || format.hasBlueComponent || format.hasAlphaComponent)
		{
			gl.namedFramebufferRenderbuffer(m_fbo[i], GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo[i]);
		}

		if (format.hasDepthComponent)
		{
			gl.namedFramebufferRenderbuffer(m_fbo[i], GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo[i]);
		}

		if (format.hasStencilComponent)
		{
			gl.namedFramebufferRenderbuffer(m_fbo[i], GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo[i]);
		}

		glw::GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Renderbuffer storage multisample test failed due to "
				<< glu::getFramebufferStatusStr(status) << " framebuffer status. Renderbuffers format was "
				<< format.internalformat_name << ", samples was " << (i ? 0 : samples) << ", width was " << width
				<< ", height was " << height << "." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** Bind framebuffer to the target.
 *
 *  @param [in] target              Bind to target.
 *  @param [in] selector            Index of the framebuffer in framebuffers' arrays.
 */
void StorageMultisampleTest::Bind(glw::GLenum target, glw::GLuint selector)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Binding framebuffer. */
	gl.bindFramebuffer(target, m_fbo[selector]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");
}

/** Blit one framebuffer to the second.
 *
 *  @param [in] width               Width of the framebuffer.
 *  @param [in] height              Height of the framebuffer.
 *
 *  @return True if there is no error, false otherwise.
 */
void StorageMultisampleTest::Blit(glw::GLuint width, glw::GLuint height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Binding framebuffer. */
	gl.blitFramebuffer(0, 0, width, height, 0, 0, width, height,
					   GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");
}

/** Clear framebuffer.
 *
 *  @param [in] isColorIntegralFormat       Is framebuffer a color integral type.
 */
void StorageMultisampleTest::Clear(bool isColorIntegralFormat)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (isColorIntegralFormat)
	{
		gl.clearBufferiv(GL_COLOR, 0, s_reference_color_integer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferiv has failed");
	}
	else
	{
		/* Setup clear values. */
		gl.clearColor(s_reference_color[0], s_reference_color[1], s_reference_color[2], s_reference_color[3]);
		gl.clearDepth(s_reference_depth);
		gl.clearStencil(s_reference_stencil);

		/* Clear rbo/fbo. */
		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

/** Check renderbuffer's content.
 *
 *  @param [in] format              Internal format to be prepared.
 *  @param [in] width               Width of the framebuffer.
 *  @param [in] height              Height of the framebuffer.
 *
 *  @return True if content matches the reference, false otherwise.
 */
bool StorageMultisampleTest::Check(StorageMultisampleTest::RenderbufferInternalFormatConfiguration format,
								   glw::GLuint width, glw::GLuint height)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint size = width * height;

	if (format.hasRedComponent || format.hasGreenComponent || format.hasBlueComponent || format.hasAlphaComponent)
	{
		if (format.isColorIntegralFormat)
		{
			std::vector<glw::GLint> color(size * 4);

			gl.readPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_INT, &color[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

			const bool hasComponent[] = { format.hasRedComponent, format.hasGreenComponent, format.hasBlueComponent,
										  format.hasAlphaComponent };

			static const char* componentName[] = { "red", "green", "blue", "alpha" };

			for (glw::GLuint i = 0; i < size; ++i)
			{
				if (hasComponent[i % 4 /* color components count*/])
				{
					if (de::abs(s_reference_color_integer[i % 4 /* color components count*/] - color[i]) >
						2 /* Precision */)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Renderbuffer storage multisample was cleared with color "
							<< componentName[i % 4 /* color components count*/] << " component equal to "
							<< s_reference_color_integer << ", but fetched value " << color[i]
							<< " is not the same. Renderbuffers format was " << format.internalformat_name
							<< ", width was " << width << ", height was " << height << "." << tcu::TestLog::EndMessage;

						return false;
					}
				}
			}
		}
		else
		{
			std::vector<glw::GLfloat> color(size * 4);

			gl.readPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, &color[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

			const bool hasComponent[] = { format.hasRedComponent, format.hasGreenComponent, format.hasBlueComponent,
										  format.hasAlphaComponent };

			static const char* componentName[] = { "red", "green", "blue", "alpha" };

			for (glw::GLuint i = 0; i < size; ++i)
			{
				if (hasComponent[i % 4 /* color components count*/])
				{
					if (de::abs(s_reference_color[i % 4 /* color components count*/] - color[i]) >
						0.0625 /* precision */)
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Renderbuffer storage multisample was cleared with color "
							<< componentName[i % 4 /* color components count*/] << " component equal to "
							<< s_reference_color[i % 4 /* color components count*/] << ", but fetched value "
							<< color[i] << " is not the same. Renderbuffers format was " << format.internalformat_name
							<< ", width was " << width << ", height was " << height << "." << tcu::TestLog::EndMessage;

						return false;
					}
				}
			}
		}
	}

	if (format.hasDepthComponent)
	{
		std::vector<glw::GLfloat> depth(size);

		gl.readPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, &depth[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

		for (glw::GLuint i = 0; i < size; ++i)
		{
			if (de::abs(s_reference_depth - depth[i]) > 0.0625 /* 1/16 precision */)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Renderbuffer storage multisample was cleared with depth component equal to "
					<< s_reference_depth << ", but fetched value " << depth[i]
					<< " is not the same. Renderbuffers format was " << format.internalformat_name << ", width was "
					<< width << ", height was " << height << "." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	if (format.hasStencilComponent)
	{
		std::vector<glw::GLint> stencil(size);

		gl.readPixels(0, 0, width, height, GL_STENCIL_INDEX, GL_INT, &stencil[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels call failed.");

		for (glw::GLuint i = 0; i < size; ++i)
		{
			if (s_reference_stencil != stencil[i])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Renderbuffer storage multisample was cleared with alpha component equal to "
					<< s_reference_stencil << ", but fetched value " << stencil[i]
					<< " is not the same. Renderbuffers format was " << format.internalformat_name << ", width was "
					<< width << ", height was " << height << "." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Clean up GL state.
 */
void StorageMultisampleTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release objects. */
	for (glw::GLuint i = 0; i < 2; ++i)
	{
		if (m_rbo[i])
		{
			gl.deleteRenderbuffers(1, &m_rbo[i]);

			m_rbo[i] = 0;
		}

		if (m_fbo[i])
		{
			gl.deleteFramebuffers(1, &m_fbo[i]);

			m_fbo[i] = 0;
		}
	}

	/* Returning to default clear values. */
	gl.clearColor(0.f, 0.f, 0.f, 0.f);
	gl.clearDepth(1.f);
	gl.clearStencil(0);

	/* Errors clean up. */
	while (gl.getError())
		;
}

/** Tested internal format */
const struct StorageMultisampleTest::RenderbufferInternalFormatConfiguration
	StorageMultisampleTest::s_renderbuffer_internalformat_configuration[] = {
		{ GL_R8, "GL_R8", true, false, false, false, false, false, false },
		{ GL_R16, "GL_R16", true, false, false, false, false, false, false },
		{ GL_RG8, "GL_RG8", true, true, false, false, false, false, false },
		{ GL_RG16, "GL_RG16", true, true, false, false, false, false, false },
		{ GL_RGB565, "GL_RGB56", true, true, true, false, false, false, false },
		{ GL_RGBA4, "GL_RGBA4", true, true, true, true, false, false, false },
		{ GL_RGB5_A1, "GL_RGB5_A1", true, true, true, true, false, false, false },
		{ GL_RGBA8, "GL_RGBA8", true, true, true, true, false, false, false },
		{ GL_RGB10_A2, "GL_RGB10_A2", true, true, true, true, false, false, false },
		{ GL_RGB10_A2UI, "GL_RGB10_A2UI", true, true, true, true, false, false, true },
		{ GL_RGBA16, "GL_RGBA16", true, true, true, true, false, false, false },
		{ GL_SRGB8_ALPHA8, "GL_SRGB8_ALPHA8", true, true, true, true, false, false, false },
		{ GL_R16F, "GL_R16F", true, false, false, false, false, false, false },
		{ GL_RG16F, "GL_RG16F", true, true, false, false, false, false, false },
		{ GL_RGBA16F, "GL_RGBA16F", true, true, true, true, false, false, false },
		{ GL_R32F, "GL_R32F", true, false, false, false, false, false, false },
		{ GL_RG32F, "GL_RG32F", true, true, false, false, false, false, false },
		{ GL_RGBA32F, "GL_RGBA32F", true, true, true, true, false, false, false },
		{ GL_R11F_G11F_B10F, "GL_R11F_G11F_B10F", true, true, true, false, false, false, false },
		{ GL_R8I, "GL_R8I", true, false, false, false, false, false, true },
		{ GL_R8UI, "GL_R8UI", true, false, false, false, false, false, true },
		{ GL_R16I, "GL_R16I", true, false, false, false, false, false, true },
		{ GL_R16UI, "GL_R16UI", true, false, false, false, false, false, true },
		{ GL_R32I, "GL_R32I", true, false, false, false, false, false, true },
		{ GL_R32UI, "GL_R32UI", true, false, false, false, false, false, true },
		{ GL_RG8I, "GL_RG8I", true, true, false, false, false, false, true },
		{ GL_RG8UI, "GL_RG8UI", true, true, false, false, false, false, true },
		{ GL_RG16I, "GL_RG16I", true, true, false, false, false, false, true },
		{ GL_RG16UI, "GL_RG16UI", true, true, false, false, false, false, true },
		{ GL_RG32I, "GL_RG32I", true, true, false, false, false, false, true },
		{ GL_RG32UI, "GL_RG32UI", true, true, false, false, false, false, true },
		{ GL_RGBA8I, "GL_RGBA8I", true, true, true, true, false, false, true },
		{ GL_RGBA8UI, "GL_RGBA8UI", true, true, true, true, false, false, true },
		{ GL_RGBA16I, "GL_RGBA16I", true, true, true, true, false, false, true },
		{ GL_RGBA16UI, "GL_RGBA16UI", true, true, true, true, false, false, true },
		{ GL_RGBA32I, "GL_RGBA32I", true, true, true, true, false, false, true },
		{ GL_RGBA32UI, "GL_RGBA32UI", true, true, true, true, false, false, true },
		{ GL_DEPTH_COMPONENT16, "GL_DEPTH_COMPONENT16", false, false, false, false, true, false, false },
		{ GL_DEPTH_COMPONENT24, "GL_DEPTH_COMPONENT24", false, false, false, false, true, false, false },
		{ GL_DEPTH_COMPONENT32F, "GL_DEPTH_COMPONENT32F", false, false, false, false, true, false, false },
		{ GL_DEPTH24_STENCIL8, "GL_DEPTH24_STENCIL8", false, false, false, false, true, true, false },
		{ GL_DEPTH32F_STENCIL8, "GL_DEPTH32F_STENCIL8", false, false, false, false, true, true, false },
		{ GL_STENCIL_INDEX8, "GL_STENCIL_INDEX8", false, false, false, false, false, true, false }
	};

/** Tesetd internal format count */
const glw::GLuint StorageMultisampleTest::s_renderbuffer_internalformat_configuration_count =
	sizeof(s_renderbuffer_internalformat_configuration) / sizeof(s_renderbuffer_internalformat_configuration[0]);

const glw::GLfloat StorageMultisampleTest::s_reference_color[4] = { 0.25, 0.5, 0.75, 1.0 }; //!< Reference color value.
const glw::GLint   StorageMultisampleTest::s_reference_color_integer[4] = {
	1, 2, 3, 4
}; //!< Reference color value for integral color internal formats.
const glw::GLfloat StorageMultisampleTest::s_reference_depth   = 0.5; //!< Reference depth value.
const glw::GLint   StorageMultisampleTest::s_reference_stencil = 7;   //!< Reference stencil value.

/******************************** Get Named Renderbuffer Parameters Test Implementation   ********************************/

/** @brief Get Named Renderbuffer Parameters Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetParametersTest::GetParametersTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_get_parameters", "Get Named Renderbuffer Parameters Test")
	, m_fbo(0)
	, m_rbo(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Check Status Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetParametersTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Test renderbuffer. */
	glw::GLuint renderbuffer = 0;

	/* Test. */
	try
	{
		static const glw::GLenum internalformats[] = { GL_RGBA8, GL_DEPTH_COMPONENT24, GL_STENCIL_INDEX8,
													   GL_DEPTH24_STENCIL8 };

		static const glw::GLuint internalformats_count = sizeof(internalformats) / sizeof(internalformats[0]);

		for (glw::GLuint i = 0; i < internalformats_count; ++i)
		{
			gl.genRenderbuffers(1, &renderbuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

			gl.bindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

			gl.renderbufferStorage(GL_RENDERBUFFER, internalformats[i], 1, 2);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

			static const glw::GLenum pnames[] = { GL_RENDERBUFFER_WIDTH,		   GL_RENDERBUFFER_HEIGHT,
												  GL_RENDERBUFFER_INTERNAL_FORMAT, GL_RENDERBUFFER_SAMPLES,
												  GL_RENDERBUFFER_RED_SIZE,		   GL_RENDERBUFFER_GREEN_SIZE,
												  GL_RENDERBUFFER_BLUE_SIZE,	   GL_RENDERBUFFER_ALPHA_SIZE,
												  GL_RENDERBUFFER_DEPTH_SIZE,	  GL_RENDERBUFFER_STENCIL_SIZE };

			static const glw::GLchar* pnames_strings[] = {
				"GL_RENDERBUFFER_WIDTH",	   "GL_RENDERBUFFER_HEIGHT",	 "GL_RENDERBUFFER_INTERNAL_FORMAT",
				"GL_RENDERBUFFER_SAMPLES",	 "GL_RENDERBUFFER_RED_SIZE",   "GL_RENDERBUFFER_GREEN_SIZE",
				"GL_RENDERBUFFER_BLUE_SIZE",   "GL_RENDERBUFFER_ALPHA_SIZE", "GL_RENDERBUFFER_DEPTH_SIZE",
				"GL_RENDERBUFFER_STENCIL_SIZE"
			};

			for (glw::GLuint j = 0; j < internalformats_count; ++j)
			{
				glw::GLint parameter_legacy = 0;
				glw::GLint parameter_dsa	= 0;

				gl.getRenderbufferParameteriv(GL_RENDERBUFFER, pnames[j], &parameter_legacy);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetRenderbufferParameteriv has failed");

				gl.getNamedRenderbufferParameteriv(renderbuffer, pnames[j], &parameter_dsa);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedRenderbufferParameteriv unexpectedly generated "
						<< glu::getErrorStr(error) << " error when called with " << pnames_strings[i]
						<< " parameter name of renderbuffer with  internalformat = "
						<< glu::getInternalFormatParameterStr(internalformats[i]) << ", width = 1, height = 2."
						<< tcu::TestLog::EndMessage;

					is_ok = false;

					continue;
				}

				if (parameter_legacy != parameter_dsa)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "GetNamedRenderbufferParameteriv returned " << parameter_dsa
						<< ", but " << parameter_legacy << " was expected for " << pnames_strings[i]
						<< " parameter name of renderbuffer with  internalformat = "
						<< glu::getInternalFormatParameterStr(internalformats[i]) << ", width = 1, height = 2."
						<< tcu::TestLog::EndMessage;

					is_ok = false;
				}
			}

			gl.deleteRenderbuffers(1, &renderbuffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteRenderbuffers has failed");

			renderbuffer = 0;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (renderbuffer)
	{
		gl.deleteRenderbuffers(1, &renderbuffer);
	}

	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Renderbuffer Creation Errors Test Implementation   ********************************/

/** @brief Creation Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationErrorsTest::CreationErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_creation_errors", "Renderbuffer Objects Creation Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok = true;

	/* Framebuffer object */
	glw::GLuint renderbuffer = 0;

	/* Check direct state creation of negative numbers of framebuffers. */
	gl.createRenderbuffers(-1, &renderbuffer);

	glw::GLenum error = GL_NO_ERROR;

	if (GL_INVALID_VALUE != (error = gl.getError()))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "CreateRenderbuffers generated " << glu::getErrorStr(error)
			<< " error when called with negative number of renderbuffers, but GL_INVALID_VALUE was expected."
			<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Cleanup (sanity). */
	if (renderbuffer)
	{
		gl.deleteRenderbuffers(1, &renderbuffer);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/******************************** Storage Errors Test Implementation   ********************************/

/** @brief Storage Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageErrorsTest::StorageErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_storage_errors", "Storage Errors Test")
	, m_rbo_valid(0)
	, m_rbo_invalid(0)
	, m_internalformat_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/*  Check that INVALID_OPERATION is generated by NamedRenderbufferStorage if
		 renderbuffer is not the name of an existing renderbuffer object. */
		gl.namedRenderbufferStorage(m_rbo_invalid, GL_RGBA8, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedRenderbufferStorage",
							 "renderbuffer is not the name of an existing renderbuffer object.");

		/*  Check that INVALID_VALUE is generated by NamedRenderbufferStorage if
		 either of width or height is negative, or greater than the value of
		 MAX_RENDERBUFFER_SIZE. */
		gl.namedRenderbufferStorage(m_rbo_valid, GL_RGBA8, -1, 1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedRenderbufferStorage", "either of width is negative.");

		gl.namedRenderbufferStorage(m_rbo_valid, GL_RGBA8, 1, -1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedRenderbufferStorage", "either of height is negative.");

		/*  Check that INVALID_ENUM is generated by NamedRenderbufferStorage if
		 internalformat is not a color-renderable, depth-renderable, or
		 stencil-renderable format. */
		gl.namedRenderbufferStorage(m_rbo_valid, m_internalformat_invalid, 1, 1);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedRenderbufferStorage", "internalformat is not a color-renderable, "
																		  "depth-renderable, or stencil-renderable "
																		  "format (it is COMPRESSED_RED).");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void StorageErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genRenderbuffers(1, &m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid objects. */
	while (gl.isRenderbuffer(++m_rbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool StorageErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
									const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void StorageErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_rbo_valid)
	{
		gl.deleteRenderbuffers(1, &m_rbo_valid);
		m_rbo_valid = 0;
	}

	/*  COmpressed internal formats are not color renderable (OpenGL 4.5 Core PRofile SPecification Chapter 9.4 )*/
	m_internalformat_invalid = GL_COMPRESSED_RED;

	/* Set initial values - all test shall have the same environment. */
	m_rbo_valid   = 0;
	m_rbo_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Storage Multisample Errors Test Implementation   ********************************/

/** @brief Storage Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
StorageMultisampleErrorsTest::StorageMultisampleErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_storage_multisample_errors", "Storage Multisample Errors Test")
	, m_rbo_valid(0)
	, m_rbo_invalid(0)
	, m_internalformat_invalid(0)
	, m_max_samples(0)
	, m_max_integer_samples(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult StorageMultisampleErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		/*  Check that INVALID_OPERATION is generated by NamedRenderbufferStorage if
		 renderbuffer is not the name of an existing renderbuffer object. */
		gl.namedRenderbufferStorageMultisample(m_rbo_invalid, 1, GL_RGBA8, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedRenderbufferStorageMultisample",
							 "renderbuffer is not the name of an existing renderbuffer object.");

		/*  Check that INVALID_VALUE is generated by
		 NamedRenderbufferStorageMultisample if samples is greater than
		 MAX_SAMPLES. */
		gl.namedRenderbufferStorageMultisample(m_rbo_valid, m_max_samples + 1, GL_RGBA8, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedRenderbufferStorageMultisample",
							 "samples is greater than MAX_SAMPLES.");

		/*  Check that INVALID_VALUE is generated by NamedRenderbufferStorage if
		 either of width or height is negative, or greater than the value of
		 MAX_RENDERBUFFER_SIZE. */
		gl.namedRenderbufferStorageMultisample(m_rbo_valid, 1, GL_RGBA8, -1, 1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedRenderbufferStorageMultisample", "either of width is negative.");

		gl.namedRenderbufferStorageMultisample(m_rbo_valid, 1, GL_RGBA8, 1, -1);

		is_ok &= ExpectError(GL_INVALID_VALUE, "NamedRenderbufferStorageMultisample", "either of height is negative.");

		/*  Check that INVALID_OPERATION is generated by
		 NamedRenderbufferStorageMultisample if internalformat is a signed or
		 unsigned integer format and samples is greater than the value of
		 MAX_INTEGER_SAMPLES. */
		gl.namedRenderbufferStorageMultisample(m_rbo_valid, m_max_samples + 1, GL_RGB10_A2UI, 1, 1);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "NamedRenderbufferStorageMultisample",
							 "internalformat is a signed or unsigned integer format and samples is greater than the "
							 "value of MAX_INTEGER_SAMPLES.");

		/*  Check that INVALID_ENUM is generated by NamedRenderbufferStorage if
		 internalformat is not a color-renderable, depth-renderable, or
		 stencil-renderable format. */
		gl.namedRenderbufferStorageMultisample(m_rbo_valid, 1, m_internalformat_invalid, 1, 1);

		is_ok &= ExpectError(GL_INVALID_ENUM, "NamedRenderbufferStorageMultisample",
							 "internalformat is not a color-renderable, depth-renderable, or stencil-renderable format "
							 "(it is COMPRESSED_RED).");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void StorageMultisampleErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genRenderbuffers(1, &m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Limits. */
	gl.getIntegerv(GL_MAX_SAMPLES, &m_max_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &m_max_integer_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv has failed");

	/* Invalid objects. */
	while (gl.isRenderbuffer(++m_rbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool StorageMultisampleErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
											   const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void StorageMultisampleErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_rbo_valid)
	{
		gl.deleteRenderbuffers(1, &m_rbo_valid);
		m_rbo_valid = 0;
	}

	/*  COmpressed internal formats are not color renderable (OpenGL 4.5 Core PRofile SPecification Chapter 9.4 )*/
	m_internalformat_invalid = GL_COMPRESSED_RED;

	/* Set initial values - all test shall have the same environment. */
	m_rbo_valid			  = 0;
	m_rbo_invalid		  = 0;
	m_max_samples		  = 0;
	m_max_integer_samples = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

/******************************** Get Parameter Errors Test Implementation   ********************************/

/** @brief Parameter Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetParameterErrorsTest::GetParameterErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "renderbuffers_get_parameters_errors", "Get Parameter Errors Test")
	, m_rbo_valid(0)
	, m_rbo_invalid(0)
	, m_parameter_invalid(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Parameter Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetParameterErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		/* Prepare objects. */
		PrepareObjects();

		glw::GLint return_value_dummy_storage;

		/*  Check that INVALID_OPERATION is generated by
		 GetNamedRenderbufferParameteriv if renderbuffer is not the name of an
		 existing renderbuffer object. */
		gl.getNamedRenderbufferParameteriv(m_rbo_invalid, GL_RENDERBUFFER_WIDTH, &return_value_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_OPERATION, "GetNamedRenderbufferParameteriv",
							 "renderbuffer is not the name of an existing renderbuffer object.");

		/*  Check that INVALID_ENUM is generated by GetNamedRenderbufferParameteriv
		 if parameter name is not one of the accepted parameter names described
		 in specification. */
		gl.getNamedRenderbufferParameteriv(m_rbo_valid, m_parameter_invalid, &return_value_dummy_storage);

		is_ok &= ExpectError(GL_INVALID_ENUM, "GetNamedRenderbufferParameteriv",
							 "parameter name is not one of the accepted parameter names described in specification.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** Check Prepare test's GL objects.
 */
void GetParameterErrorsTest::PrepareObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Valid objects. */
	gl.genRenderbuffers(1, &m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_valid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	/* Invalid parameter. */
	bool is_parameter = true;

	while (is_parameter)
	{
		is_parameter = false;

		++m_parameter_invalid;

		static const glw::GLenum valid_parameters[] = { GL_RENDERBUFFER_WIDTH,			 GL_RENDERBUFFER_HEIGHT,
														GL_RENDERBUFFER_INTERNAL_FORMAT, GL_RENDERBUFFER_SAMPLES,
														GL_RENDERBUFFER_RED_SIZE,		 GL_RENDERBUFFER_GREEN_SIZE,
														GL_RENDERBUFFER_BLUE_SIZE,		 GL_RENDERBUFFER_ALPHA_SIZE,
														GL_RENDERBUFFER_DEPTH_SIZE,		 GL_RENDERBUFFER_STENCIL_SIZE };

		static const glw::GLuint valid_parameters_count = sizeof(valid_parameters) / sizeof(valid_parameters[0]);

		for (glw::GLuint i = 0; i < valid_parameters_count; ++i)
		{
			if (valid_parameters[i] == m_parameter_invalid)
			{
				is_parameter = true;
			}
		}
	}

	/* Invalid objects. */
	while (gl.isRenderbuffer(++m_rbo_invalid))
		;
}

/** Check if error is equal to the expected, log if not.
 *
 *  @param [in] expected_error      Error to be expected.
 *  @param [in] function            Function name which is being tested (to be logged).
 *  @param [in] conditions          Conditions when the expected error shall occure (to be logged).
 *
 *  @return True if there is no error, false otherwise.
 */
bool GetParameterErrorsTest::ExpectError(glw::GLenum expected_error, const glw::GLchar* function,
										 const glw::GLchar* conditions)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool is_ok = true;

	glw::GLenum error = GL_NO_ERROR;

	if (expected_error != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << function << " was expected to generate "
											<< glu::getErrorStr(expected_error) << ", but " << glu::getErrorStr(error)
											<< " was observed instead when " << conditions << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	/* Clean additional possible errors. */
	while (gl.getError())
		;

	return is_ok;
}

/** @brief Clean up GL state.
 */
void GetParameterErrorsTest::Clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release GL objects. */
	if (m_rbo_valid)
	{
		gl.deleteRenderbuffers(1, &m_rbo_valid);
		m_rbo_valid = 0;
	}

	/* Set initial values - all test shall have the same environment. */
	m_rbo_valid   = 0;
	m_rbo_invalid = 0;

	/* Errors clean up. */
	while (gl.getError())
		;
}

} // namespace Renderbuffers
} // namespace DirectStateAccess
} // namespace gl4cts
