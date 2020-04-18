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
 * \file  gl4cSyncTests.cpp
 * \brief Declares test classes for synchronization functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cIncompleteTextureAccessTests.hpp"

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

namespace gl4cts
{
namespace IncompleteTextureAccess
{
/****************************************** Incomplete Texture Access Tests Group ***********************************************/

/** @brief Incomplete Texture Access Tests Group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "incomplete_texture_access", "Incomplete Texture Access Tests Suite")
{
}

/** @brief Incomplete Texture Access Tests initializer. */
void Tests::init()
{
	addChild(new IncompleteTextureAccess::SamplerTest(m_context));
}

/*************************************** Sampler Incomplete Texture Access Test Test *******************************************/

/** @brief Sampler Incomplete Texture Access Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
SamplerTest::SamplerTest(deqp::Context& context)
	: deqp::TestCase(context, "sampler", "Fetch using sampler test"), m_po(0), m_to(0), m_fbo(0), m_rbo(0), m_vao(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Incomplete Texture Access Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult SamplerTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));

	if (!is_at_least_gl_45)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		PrepareFramebuffer();
		PrepareVertexArrays();

		for (glw::GLuint i = 0; i < s_configurations_count; ++i)
		{
			PrepareProgram(s_configurations[i]);
			PrepareTexture(s_configurations[i]);

			Draw();

			if (!Check(s_configurations[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Incomplete texture sampler access test failed with sampler "
					<< s_configurations[i].sampler_template << "." << tcu::TestLog::EndMessage;

				is_ok = false;
			}

			CleanCase();
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanCase();
	CleanTest();

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

void SamplerTest::PrepareProgram(Configuration configuration)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source[5];
		glw::GLsizei const count;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = { { { s_vertex_shader, NULL, NULL, NULL, NULL }, 1, GL_VERTEX_SHADER, 0 },
				   { { s_fragment_shader_head, configuration.sampler_template, s_fragment_shader_body,
					   configuration.fetch_template, s_fragment_shader_tail },
					 5,
					 GL_FRAGMENT_SHADER,
					 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, shader[i].count, shader[i].source, NULL);

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

					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Shader compilation has failed.\n"
						<< "Shader type: " << glu::getShaderTypeStr(shader[i].type) << "\n"
						<< "Shader compilation error log:\n"
						<< log_text << "\n"
						<< "Shader source code:\n"
						<< shader[i].source[0] << (shader[i].source[1] ? shader[i].source[1] : "")
						<< (shader[i].source[2] ? shader[i].source[2] : "")
						<< (shader[i].source[3] ? shader[i].source[3] : "")
						<< (shader[i].source[4] ? shader[i].source[4] : "") << "\n"
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
	else
	{
		gl.useProgram(m_po);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUsePrograms has failed");
	}
}

void SamplerTest::PrepareTexture(Configuration configuration)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_to)
	{
		throw 0;
	}

	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures has failed");

	gl.bindTexture(configuration.texture_target, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture has failed");
}

void SamplerTest::PrepareFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sanity checks. */
	if (m_fbo || m_rbo)
	{
		throw 0;
	}

	/* Framebuffer creation. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers has failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer has failed");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers has failed");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer has failed");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 1 /* x size */, 1 /* y size */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage has failed");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer has failed");

	/* Sanity checks. */
	if (GL_FRAMEBUFFER_COMPLETE != gl.checkFramebufferStatus(GL_FRAMEBUFFER))
	{
		throw 0;
	}
}

void SamplerTest::PrepareVertexArrays()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sanity checks.*/
	if (m_vao)
	{
		throw 0;
	}

	/* Empty vao creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gGenVertexArrays has failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gBindVertexArrays has failed");
}

void SamplerTest::Draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Draw setup. */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture has failed");

	gl.uniform1i(gl.getUniformLocation(m_po, "texture_input"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i has failed");

	/* Draw. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays has failed");
}

bool SamplerTest::Check(Configuration configuration)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Return storage. */
	glw::GLfloat result[4] = { 7.f, 7.f, 7.f, 7.f };

	/* Fetch. */
	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels has failed");

	/* Comparison. */
	for (glw::GLuint i = 0; i < 4 /* # components */; ++i)
	{
		if (de::abs(configuration.expected_result[i] - result[i]) > 0.0125 /* precision */)
		{
			/* Fail.*/
			return false;
		}
	}

	/* Comparsion passed.*/
	return true;
}

void SamplerTest::CleanCase()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Program cleanup. */
	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}

	/* Texture cleanup. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	/* Errors cleanup. */
	while (gl.getError())
		;
}

void SamplerTest::CleanTest()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Framebuffer cleanup. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Renderbuffer cleanup. */
	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Vertex arrays cleanup. */
	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	/* Errors cleanup. */
	while (gl.getError())
		;
}

const struct SamplerTest::Configuration SamplerTest::s_configurations[] = {
	/* regular floating point sampling */
	{ GL_TEXTURE_1D, "sampler1D", "texture(texture_input, 0.0)", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_2D, "sampler2D", "texture(texture_input, vec2(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_3D, "sampler3D", "texture(texture_input, vec3(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_CUBE_MAP, "samplerCube", "texture(texture_input, vec3(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_RECTANGLE, "sampler2DRect", "texture(texture_input, vec2(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_1D_ARRAY, "sampler1DArray", "texture(texture_input, vec2(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_2D_ARRAY, "sampler2DArray", "texture(texture_input, vec3(0.0))", { 0.f, 0.f, 0.f, 1.f } },
	{ GL_TEXTURE_CUBE_MAP_ARRAY, "samplerCubeArray", "texture(texture_input, vec4(0.0))", { 0.f, 0.f, 0.f, 1.f } },

	/* Shadow textures. */
	{ GL_TEXTURE_1D,
	  "sampler1DShadow",
	  "vec4(texture(texture_input, vec3(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_2D,
	  "sampler2DShadow",
	  "vec4(texture(texture_input, vec3(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_CUBE_MAP,
	  "samplerCubeShadow",
	  "vec4(texture(texture_input, vec4(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_RECTANGLE,
	  "sampler2DRectShadow",
	  "vec4(texture(texture_input, vec3(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_1D_ARRAY,
	  "sampler1DArrayShadow",
	  "vec4(texture(texture_input, vec3(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_2D_ARRAY,
	  "sampler2DArrayShadow",
	  "vec4(texture(texture_input, vec4(0.0)), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } },
	{ GL_TEXTURE_CUBE_MAP_ARRAY,
	  "samplerCubeArrayShadow",
	  "vec4(texture(texture_input, vec4(0.0), 1.0), 0.0, 0.0, 0.0)",
	  { 0.f, 0.f, 0.f, 0.f } }
};

const glw::GLuint SamplerTest::s_configurations_count = sizeof(s_configurations) / sizeof(s_configurations[0]);

const glw::GLchar* SamplerTest::s_vertex_shader = "#version 450\n"
												  "\n"
												  "void main()\n"
												  "{\n"
												  "    switch(gl_VertexID)\n"
												  "    {\n"
												  "        case 0:\n"
												  "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
												  "            break;\n"
												  "        case 1:\n"
												  "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
												  "            break;\n"
												  "        case 2:\n"
												  "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
												  "            break;\n"
												  "        case 3:\n"
												  "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
												  "            break;\n"
												  "    }\n"
												  "}\n";

const glw::GLchar* SamplerTest::s_fragment_shader_head = "#version 450\n"
														 "\n"
														 "uniform ";

const glw::GLchar* SamplerTest::s_fragment_shader_body = " texture_input;\n"
														 "out vec4 texture_output;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    texture_output = ";

const glw::GLchar* SamplerTest::s_fragment_shader_tail = ";\n"
														 "}\n";

} /* IncompleteTextureAccess namespace */
} /* gl4cts namespace */
