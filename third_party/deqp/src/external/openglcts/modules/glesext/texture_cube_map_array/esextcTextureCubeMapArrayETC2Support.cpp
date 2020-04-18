/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file  esextcTextureCubeMapArrayETC2Support.cpp
 * \brief texture_cube_map_array ETC2 support (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureCubeMapArrayETC2Support.hpp"
#include "glcTestCase.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "glwEnums.inl"
#include "glwTypes.inl"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

enum TextureSize
{
	RENDER_WIDTH  = 8,
	RENDER_HEIGHT = 8
};

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TextureCubeMapArrayETC2Support::TextureCubeMapArrayETC2Support(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fbo(0), m_rbo(0), m_vao(0), m_texture(0), m_program(0)
{
}

/** Deinitializes test
 *
 **/
void TextureCubeMapArrayETC2Support::deinit(void)
{
	glcts::TestCaseBase::deinit();
}

/** @brief Iterate Functional Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestCase::IterateResult TextureCubeMapArrayETC2Support::iterate(void)
{
	prepareFramebuffer();
	prepareProgram();
	prepareVertexArrayObject();
	prepareTexture();
	draw();

	if (isRenderedImageValid())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	clean();
	return STOP;
}

/** @brief Bind default framebuffer object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void TextureCubeMapArrayETC2Support::prepareFramebuffer()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, RENDER_WIDTH, RENDER_HEIGHT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");
}

/** @brief Function generate and bind empty vertex array object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void TextureCubeMapArrayETC2Support::prepareVertexArrayObject()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

/** @brief Function builds test's GLSL program.
 *         If succeded, the program will be set to be used.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void TextureCubeMapArrayETC2Support::prepareProgram()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { s_vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	bool			  programPreparationFailed = false;
	glw::GLuint const shader_count			   = sizeof(shader) / sizeof(shader[0]);

	/* Create program. */
	m_program = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

	/* Shader compilation. */
	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (DE_NULL != shader[i].source)
		{
			shader[i].id = gl.createShader(shader[i].type);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

			gl.attachShader(m_program, shader[i].id);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

			glu::ContextType contextType = m_context.getRenderContext().getType();
			glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(contextType);
			std::string		 shaderSource(glu::getGLSLVersionDeclaration(glslVersion));
			shaderSource += shader[i].source;
			const char* source = shaderSource.c_str();

			gl.shaderSource(shader[i].id, 1, &source, NULL);

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

				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation error, log:\n"
													<< log_text << "\n"
													<< "Shader source code:\n"
													<< shaderSource << "\n"
													<< tcu::TestLog::EndMessage;

				delete[] log_text;

				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

				programPreparationFailed = true;
				break;
			}
		}
	}

	if (programPreparationFailed)
	{
		if (m_program)
		{
			gl.deleteProgram(m_program);
			m_program = 0;
		}
	}
	else
	{
		/* Link. */
		gl.linkProgram(m_program);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glLlinkProgram call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_program, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_program, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_program, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_program, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

			programPreparationFailed = true;
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

	if (m_program)
	{
		glw::GLint textureSampler = gl.getUniformLocation(m_program, "texture_sampler");
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation call failed.");
		gl.useProgram(m_program);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
		gl.uniform1i(textureSampler, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i call failed.");
	}
	else
		TCU_FAIL("Failed to prepare program");
}

/** @brief Function prepares texture object with test's data.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void TextureCubeMapArrayETC2Support::prepareTexture()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture call failed.");

	/* Texture creation and binding. */
	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures call failed.");

	const glw::GLuint target = GL_TEXTURE_CUBE_MAP_ARRAY;
	gl.bindTexture(target, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	/* Uploading texture. */
	gl.compressedTexImage3D(target, 0, GL_COMPRESSED_RGB8_ETC2, RENDER_WIDTH, RENDER_HEIGHT, 6, 0,
							s_compressed_RGB_texture_data_size, s_compressed_RGB_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage3D call failed.");

	gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri call failed.");
}

/** @brief Function draws a quad.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void TextureCubeMapArrayETC2Support::draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.viewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");
}

/** @brief Check if drawn image is same as reference.
 */
bool TextureCubeMapArrayETC2Support::isRenderedImageValid()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Construct reference image. */
	const tcu::TextureFormat	textureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);
	tcu::ConstPixelBufferAccess referenceImg(textureFormat, RENDER_WIDTH, RENDER_HEIGHT, 1, s_RGB_texture_data);

	/* Read GL image. */
	GLubyte				   empty_data[RENDER_WIDTH * RENDER_HEIGHT * 4];
	tcu::PixelBufferAccess renderedImg(textureFormat, RENDER_WIDTH, RENDER_HEIGHT, 1, empty_data);
	glu::readPixels(m_context.getRenderContext(), 0, 0, renderedImg);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::readPixels call failed.");

	/* Compare images using a big thresholdbecause as compression brings inaccuracy. */
	const float compareThreshold = 0.05f;
	return tcu::fuzzyCompare(m_testCtx.getLog(), "CompareResult", "Image Comparison Result", referenceImg, renderedImg,
							 compareThreshold, tcu::COMPARE_LOG_RESULT);
}

/** @brief Release all GL objects.
 */
void TextureCubeMapArrayETC2Support::clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

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

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}

	if (m_texture)
	{
		gl.deleteTextures(1, &m_texture);
		m_texture = 0;
	}

	if (m_program)
	{
		gl.useProgram(0);
		gl.deleteProgram(m_program);
		m_program = 0;
	}
}

/* Vertex shader source code. */
const glw::GLchar TextureCubeMapArrayETC2Support::s_vertex_shader[] =
	"\n"
	"out highp vec4 texCoord;\n"
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
	"    texCoord = vec4(gl_Position.xy, 1.0, 0.0);"
	"}\n";

/* Fragment shader source program. */
const glw::GLchar TextureCubeMapArrayETC2Support::s_fragment_shader[] =
	"\n"
	"uniform highp samplerCubeArray texture_sampler;\n"
	"in highp vec4 texCoord;"
	"out highp vec4 color;\n"
	"void main()\n"
	"{\n"
	"    color = texture(texture_sampler, texCoord);\n"
	"}\n";

/* Texture data, 8x8 RGBA. */
const glw::GLubyte TextureCubeMapArrayETC2Support::s_RGB_texture_data[] = {
	0, 224,   0, 255, 0, 224,  32, 255, 0, 224,  64, 255, 0, 224,  96, 255,
	0, 224, 128, 255, 0, 224, 160, 255, 0, 224, 192, 255, 0, 224, 224, 255,
	0, 192,   0, 255, 0, 192,  32, 255, 0, 192,  64, 255, 0, 192,  96, 255,
	0, 192, 128, 255, 0, 192, 160, 255, 0, 192, 192, 255, 0, 192, 224, 255,

	0, 160,   0, 255, 0, 160,  32, 255, 0, 160,  64, 255, 0, 160,  96, 255,
	0, 160, 128, 255, 0, 160, 160, 255, 0, 160, 192, 255, 0, 160, 224, 255,
	0, 128,   0, 255, 0, 128,  32, 255, 0, 128,  64, 255, 0, 128,  96, 255,
	0, 128, 128, 255, 0, 128, 160, 255, 0, 128, 192, 255, 0, 128, 224, 255,

	0,  96,   0, 255, 0,  96,  32, 255, 0,  96,  64, 255, 0,  96,  96, 255,
	0,  96, 128, 255, 0,  96, 160, 255, 0,  96, 192, 255, 0,  96, 224, 255,
	0,  64,   0, 255, 0,  64,  32, 255, 0,  64,  64, 255, 0,  64,  96, 255,
	0,  64, 128, 255, 0,  64, 160, 255, 0,  64, 192, 255, 0,  64, 224, 255,

	0,  32,   0, 255, 0,  32,  32, 255, 0,  32,  64, 255, 0,  32,  96, 255,
	0,  32, 128, 255, 0,  32, 160, 255, 0,  32, 192, 255, 0,  32, 224, 255,
	0,   0,   0, 255, 0,   0,  32, 255, 0,   0,  64, 255, 0,   0,  96, 255,
	0,   0, 128, 255, 0,   0, 160, 255, 0,   0, 192, 255, 0,   0, 224, 255,
};

const glw::GLsizei TextureCubeMapArrayETC2Support::s_RGB_texture_data_size =
	sizeof(TextureCubeMapArrayETC2Support::s_RGB_texture_data);

/* Compressed texture data 8x8x6 RGB - all layers are the same. */
const glw::GLubyte TextureCubeMapArrayETC2Support::s_compressed_RGB_texture_data[] = {
	/* Layer 0 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,

	/* Layer 1 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,

	/* Layer 2 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,

	/* Layer 3 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,

	/* Layer 4 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,

	/* Layer 5 */
	0x0, 0x0, 0x4, 0x2, 0x1, 0x0, 0x6f, 0xc0, 0x0, 0x1, 0x4, 0x2, 0x1, 0xf8, 0x6f, 0xe0, 0x1, 0x0, 0x4, 0x2, 0x80, 0xf8,
	0x5f, 0xc0, 0x1, 0x0, 0xfb, 0x82, 0x81, 0xf8, 0x5f, 0xe0,
};

/* Compressed texture width. */
const glw::GLsizei TextureCubeMapArrayETC2Support::s_compressed_RGB_texture_data_size =
	sizeof(TextureCubeMapArrayETC2Support::s_compressed_RGB_texture_data);

} // namespace glcts
