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

#include "gl4cStencilTexturingTests.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <string>
#include <vector>

#define DEBUG_REPLACE_TOKEN 0

using namespace glw;

namespace gl4cts
{
namespace StencilTexturing
{
class Utils
{
public:
	static GLuint createAndBuildProgram(deqp::Context& context, const GLchar* cs_code, const GLchar* fs_code,
										const GLchar* gs_code, const GLchar* tcs_code, const GLchar* tes_code,
										const GLchar* vs_code);

	static GLuint createAndCompileShader(deqp::Context& context, const GLenum type, const GLchar* code);

	static GLuint createAndFill2DTexture(deqp::Context& context, GLuint width, GLuint height, GLenum internal_format,
										 GLenum format, GLenum type, const GLvoid* data);

	static void deleteProgram(deqp::Context& context, const GLuint id);
	static void deleteShader(deqp::Context& context, const GLuint id);
	static void deleteTexture(deqp::Context& context, const GLuint id);
	static bool isExtensionSupported(deqp::Context& context, const GLchar* extension_name);

	static void replaceToken(const GLchar* token, size_t& search_position, const GLchar* text, std::string& string);
};

/** Create and build program from provided sources
 *
 * @param context  Test context
 * @param cs_code  Source code for compute shader stage
 * @param fs_code  Source code for fragment shader stage
 * @param gs_code  Source code for geometry shader stage
 * @param tcs_code Source code for tesselation control shader stage
 * @param tes_code Source code for tesselation evaluation shader stage
 * @param vs_code  Source code for vertex shader stage
 *
 * @return ID of program object
 **/
GLuint Utils::createAndBuildProgram(deqp::Context& context, const GLchar* cs_code, const GLchar* fs_code,
									const GLchar* gs_code, const GLchar* tcs_code, const GLchar* tes_code,
									const GLchar* vs_code)
{
#define N_SHADER_STAGES 6

	const Functions& gl							 = context.getRenderContext().getFunctions();
	GLuint			 id							 = 0;
	GLuint			 shader_ids[N_SHADER_STAGES] = { 0 };

	const GLchar* shader_sources[N_SHADER_STAGES] = { cs_code, fs_code, gs_code, tcs_code, tes_code, vs_code };

	const GLenum shader_types[N_SHADER_STAGES] = { GL_COMPUTE_SHADER,		  GL_FRAGMENT_SHADER,
												   GL_GEOMETRY_SHADER,		  GL_TESS_CONTROL_SHADER,
												   GL_TESS_EVALUATION_SHADER, GL_VERTEX_SHADER };
	GLint status = GL_FALSE;

	/* Compile all shaders */
	try
	{
		for (GLuint i = 0; i < N_SHADER_STAGES; ++i)
		{
			if (0 != shader_sources[i])
			{
				shader_ids[i] = createAndCompileShader(context, shader_types[i], shader_sources[i]);
			}
		}

		/* Check compilation */
		for (GLuint i = 0; i < N_SHADER_STAGES; ++i)
		{
			if ((0 != shader_sources[i]) && (0 == shader_ids[i]))
			{
				context.getTestContext().getLog() << tcu::TestLog::Message
												  << "Failed to build program due to compilation problems"
												  << tcu::TestLog::EndMessage;

				/* Delete shaders */
				for (GLuint j = 0; j < N_SHADER_STAGES; ++j)
				{
					deleteShader(context, shader_ids[j]);
				}

				/* Done */
				return 0;
			}
		}

		/* Create program */
		id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

		/* Attach shaders */
		for (GLuint i = 0; i < N_SHADER_STAGES; ++i)
		{
			if (0 != shader_ids[i])
			{
				gl.attachShader(id, shader_ids[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
			}
		}

		/* Link program */
		gl.linkProgram(id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

		/* Clean shaders */
		for (GLuint j = 0; j < N_SHADER_STAGES; ++j)
		{
			deleteShader(context, shader_ids[j]);
		}

		/* Get link status */
		gl.getProgramiv(id, GL_LINK_STATUS, &status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		/* Log link error */
		if (GL_TRUE != status)
		{
			glw::GLint  length = 0;
			std::string message;

			/* Get error log length */
			gl.getProgramiv(id, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

			message.resize(length, 0);

			/* Get error log */
			gl.getProgramInfoLog(id, length, 0, &message[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

			context.getTestContext().getLog() << tcu::TestLog::Message << "Program linking failed: " << message
											  << tcu::TestLog::EndMessage;

			/* Clean program */
			deleteProgram(context, id);

			/* Done */
			return 0;
		}
	}
	catch (std::exception& exc)
	{
		/* Delete shaders */
		for (GLuint j = 0; j < N_SHADER_STAGES; ++j)
		{
			deleteShader(context, shader_ids[j]);
		}

		throw exc;
	}

	return id;
}

/** Create and compile shader
 *
 * @param context Test context
 * @param type    Type of shader
 * @param code    Source code for shader
 *
 * @return ID of shader object
 **/
GLuint Utils::createAndCompileShader(deqp::Context& context, const GLenum type, const GLchar* code)
{
	const Functions& gl		= context.getRenderContext().getFunctions();
	GLuint			 id		= gl.createShader(type);
	GLint			 status = GL_FALSE;

	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	try
	{
		gl.shaderSource(id, 1 /* count */, &code, 0 /* lengths */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

		/* Compile */
		gl.compileShader(id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

		/* Get compilation status */
		gl.getShaderiv(id, GL_COMPILE_STATUS, &status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Log compilation error */
		if (GL_TRUE != status)
		{
			glw::GLint  length = 0;
			std::string message;

			/* Error log length */
			gl.getShaderiv(id, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

			/* Prepare storage */
			message.resize(length, 0);

			/* Get error log */
			gl.getShaderInfoLog(id, length, 0, &message[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

			context.getTestContext().getLog() << tcu::TestLog::Message << "Shader (" << glu::getShaderTypeStr(type)
											  << ") compilation failed: " << message << tcu::TestLog::EndMessage;

			deleteShader(context, id);
			id = 0;
		}
	}
	catch (std::exception& exc)
	{
		deleteShader(context, id);
		throw exc;
	}

	return id;
}

/** Generate and fill 2d texture
 *
 * @param context         Test context
 * @param width           Width of texture
 * @param height          Height of texture
 * @param internal_format Internal format of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param data            Data
 *
 * @return ID of texture object
 **/
GLuint Utils::createAndFill2DTexture(deqp::Context& context, GLuint width, GLuint height, GLenum internal_format,
									 GLenum format, GLenum type, const GLvoid* data)
{
	const Functions& gl = context.getRenderContext().getFunctions();
	GLuint			 id = 0;

	gl.genTextures(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	try
	{
		gl.bindTexture(GL_TEXTURE_2D, id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, internal_format, width, height, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

		gl.bindTexture(GL_TEXTURE_2D, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	}
	catch (std::exception& exc)
	{
		gl.deleteTextures(1, &id);
		id = 0;

		throw exc;
	}

	return id;
}

/** Delete program
 *
 * @param context Test context
 * @param id      ID of program
 **/
void Utils::deleteProgram(deqp::Context& context, const GLuint id)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	if (0 != id)
	{
		gl.deleteProgram(id);
	}
}

/** Delete shader
 *
 * @param context Test context
 * @param id      ID of shader
 **/
void Utils::deleteShader(deqp::Context& context, const GLuint id)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	if (0 != id)
	{
		gl.deleteShader(id);
	}
}

/** Delete texture
 *
 * @param context Test context
 * @param id      ID of texture
 **/
void Utils::deleteTexture(deqp::Context& context, const GLuint id)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	if (0 != id)
	{
		gl.deleteTextures(1, &id);
	}
}

/** Checks if extensions is not available.
 *
 * @param context        Test context
 * @param extension_name Name of extension
 *
 * @return true if extension is reported as available, false otherwise
 **/
bool Utils::isExtensionSupported(deqp::Context& context, const GLchar* extension_name)
{
	const std::vector<std::string>& extensions = context.getContextInfo().getExtensions();

	if (std::find(extensions.begin(), extensions.end(), extension_name) == extensions.end())
	{
		std::string message = "Required extension is not supported: ";
		message.append(extension_name);

		return false;
	}

	return true;
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void Utils::replaceToken(const GLchar* token, size_t& search_position, const GLchar* text, std::string& string)
{
	const size_t text_length	= strlen(text);
	const size_t token_length   = strlen(token);
	const size_t token_position = string.find(token, search_position);

#if DEBUG_REPLACE_TOKEN
	if (std::string::npos == token_position)
	{
		string.append("\n\nInvalid token: ");
		string.append(token);

		TCU_FAIL(string.c_str());
	}
#endif /* DEBUG_REPLACE_TOKEN */

	string.replace(token_position, token_length, text, text_length);

	search_position = token_position + text_length;
}

/* FunctionalTest */
/* Shader sources */
const GLchar* FunctionalTest::m_compute_shader_code =
	"#version 430 core\n"
	"\n"
	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
	"\n"
	"IMAGE_DEFINITION;\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec2 tex_coord = vec2(gl_GlobalInvocationID.xy) / 8.0;\n"
	"\n"
	"    imageStore(uni_image,\n"
	"               ivec2(gl_GlobalInvocationID.xy),\n"
	"               TYPE(texture(uni_sampler, tex_coord).r, 0, 0, 0));\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_fragment_shader_code =
	"#version 430 core\n"
	"\n"
	"     in  vec2 gs_fs_tex_coord;\n"
	"flat in  uint gs_fs_result;\n"
	"     out TYPE fs_out_result;\n"
	"\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    if (1 != gs_fs_result)\n"
	"    {\n"
	"        fs_out_result = texture(uni_sampler, vec2(0.9375, 0.9375));\n"
	"    }\n"
	"    else\n"
	"    {\n"
	"        fs_out_result = texture(uni_sampler, gs_fs_tex_coord);\n"
	"    }\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_geometry_shader_code =
	"#version 430 core\n"
	"\n"
	"layout(points)                           in;\n"
	"layout(triangle_strip, max_vertices = 4) out;\n"
	"\n"
	"     in  uint tes_gs_result[];\n"
	"flat out uint gs_fs_result;\n"
	"     out vec2 gs_fs_tex_coord;\n"
	"\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint result = 1u;\n"
	"\n"
	"    if (1 != tes_gs_result[0])\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    if (EXPECTED_VALUE != texture(uni_sampler, vec2(0.9375, 0.9375)).r)\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    gs_fs_result    = result;\n"
	"    gs_fs_tex_coord = vec2(0, 0);\n"
	"    gl_Position     = vec4(-1, -1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gs_fs_result    = result;\n"
	"    gs_fs_tex_coord = vec2(0, 1);\n"
	"    gl_Position     = vec4(-1, 1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gs_fs_result    = result;\n"
	"    gs_fs_tex_coord = vec2(1, 0);\n"
	"    gl_Position     = vec4(1, -1, 0, 1);\n"
	"    EmitVertex();\n"
	"    gs_fs_result    = result;\n"
	"    gs_fs_tex_coord = vec2(1, 1);\n"
	"    gl_Position     = vec4(1, 1, 0, 1);\n"
	"    EmitVertex();\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_tesselation_control_shader_code =
	"#version 430 core\n"
	"\n"
	"layout(vertices = 1) out;\n"
	"\n"
	"in  uint vs_tcs_result[];\n"
	"out uint tcs_tes_result[];\n"
	"\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint result = 1u;\n"
	"\n"
	"    if (1u != vs_tcs_result[gl_InvocationID])\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    if (EXPECTED_VALUE != texture(uni_sampler, vec2(0.9375, 0.9375)).r)\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    tcs_tes_result[gl_InvocationID] = result;\n"
	"\n"
	"    gl_TessLevelOuter[0] = 1.0;\n"
	"    gl_TessLevelOuter[1] = 1.0;\n"
	"    gl_TessLevelOuter[2] = 1.0;\n"
	"    gl_TessLevelOuter[3] = 1.0;\n"
	"    gl_TessLevelInner[0] = 1.0;\n"
	"    gl_TessLevelInner[1] = 1.0;\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_tesselation_evaluation_shader_code =
	"#version 430 core\n"
	"\n"
	"layout(isolines, point_mode) in;\n"
	"\n"
	"in  uint tcs_tes_result[];\n"
	"out uint tes_gs_result;\n"
	"\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint result = 1u;\n"
	"\n"
	"    if (1u != tcs_tes_result[0])\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    if (EXPECTED_VALUE != texture(uni_sampler, vec2(0.9375, 0.9375)).r)\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    tes_gs_result = result;\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_vertex_shader_code =
	"#version 430 core\n"
	"\n"
	"out uint vs_tcs_result;\n"
	"\n"
	"SAMPLER_DEFINITION;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint result = 1u;\n"
	"\n"
	"    if (EXPECTED_VALUE != texture(uni_sampler, vec2(0.9375, 0.9375)).r)\n"
	"    {\n"
	"        result = 0u;\n"
	"    }\n"
	"\n"
	"    vs_tcs_result = result;\n"
	"}\n"
	"\n";

const GLchar* FunctionalTest::m_expected_value_depth = "0.0";

const GLchar* FunctionalTest::m_expected_value_stencil = "15u";

const GLchar* FunctionalTest::m_image_definition_depth = "writeonly uniform image2D uni_image";

const GLchar* FunctionalTest::m_image_definition_stencil = "writeonly uniform uimage2D uni_image";

const GLchar* FunctionalTest::m_output_type_depth = "vec4";

const GLchar* FunctionalTest::m_output_type_stencil = "uvec4";

const GLchar* FunctionalTest::m_sampler_definition_depth = "uniform sampler2D uni_sampler";

const GLchar* FunctionalTest::m_sampler_definition_stencil = "uniform usampler2D uni_sampler";

/* Constants */
const GLuint FunctionalTest::m_height		= 8;
const GLint  FunctionalTest::m_image_unit   = 1;
const GLint  FunctionalTest::m_texture_unit = 1;
const GLuint FunctionalTest::m_width		= 8;

/** Constructor
 *
 * @param context Test context
 **/
FunctionalTest::FunctionalTest(deqp::Context& context)
	: TestCase(context, "functional", "Checks if sampling stencil texture gives expected results")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	bool test_result = true;

	if (false == test(GL_DEPTH24_STENCIL8, true))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Test failed. Case format: GL_DEPTH24_STENCIL8, channel: S"
											<< tcu::TestLog::EndMessage;
		test_result = false;
	}

	if (false == test(GL_DEPTH24_STENCIL8, false))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Test failed. Case format: GL_DEPTH24_STENCIL8, channel: D"
											<< tcu::TestLog::EndMessage;
		test_result = false;
	}

	if (false == test(GL_DEPTH32F_STENCIL8, true))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Test failed. Case format: GL_DEPTH32F_STENCIL8, channel: S"
											<< tcu::TestLog::EndMessage;
		test_result = false;
	}

	if (false == test(GL_DEPTH32F_STENCIL8, false))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Test failed. Case format: GL_DEPTH32F_STENCIL8, channel: D"
											<< tcu::TestLog::EndMessage;
		test_result = false;
	}

	/* Set result */
	if (true == test_result)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Execute compute program
 *
 * @param program_id     ID of program
 * @param is_stencil     Selects if stencil or depth channel is sampled
 * @param dst_texture_id ID of destination texture
 * @param src_texture_id ID of source texture
 **/
void FunctionalTest::dispatch(GLuint program_id, bool is_stencil, GLuint dst_texture_id, GLuint src_texture_id)
{
	const Functions& gl				 = m_context.getRenderContext().getFunctions();
	GLenum			 internal_format = GL_R8UI;
	GLint			 uni_image_loc   = -1;
	GLint			 uni_sampler_loc = -1;

	if (false == is_stencil)
	{
		internal_format = GL_R32F;
	}

	/* Set program */
	gl.useProgram(program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

	/* Get uniform location and bind texture to proper image unit */
	uni_image_loc = gl.getUniformLocation(program_id, "uni_image");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	gl.bindImageTexture(m_image_unit, dst_texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* Layer */,
						GL_WRITE_ONLY, internal_format);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	gl.uniform1i(uni_image_loc, m_image_unit);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	/* Get uniform location and bind texture to proper texture unit */
	uni_sampler_loc = gl.getUniformLocation(program_id, "uni_sampler");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	gl.activeTexture(GL_TEXTURE0 + m_texture_unit);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

	gl.bindTexture(GL_TEXTURE_2D, src_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.uniform1i(uni_sampler_loc, m_texture_unit);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	/* Dispatch program */
	gl.dispatchCompute(m_width, m_height, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	/* Sync */
	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");
}

/** Execute draw program
 *
 * @param program_id     ID of program
 * @param dst_texture_id ID of destination texture
 * @param src_texture_id ID of source texture
 **/
void FunctionalTest::draw(GLuint program_id, GLuint dst_texture_id, GLuint src_texture_id)
{
	GLuint			 fb_id			 = 0;
	const Functions& gl				 = m_context.getRenderContext().getFunctions();
	GLint			 uni_sampler_loc = -1;
	GLuint			 vao_id			 = 0;

	try
	{
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		/* Prepare VAO */
		gl.genVertexArrays(1, &vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

		gl.bindVertexArray(vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");

		/* Prepare FBO */
		gl.genFramebuffers(1, &fb_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

		gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst_texture_id, 0 /* level */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture");

		gl.viewport(0 /* x */, 0 /* y */, m_width, m_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

		/* Set program */
		gl.useProgram(program_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		/* Get uniform location and bind texture to proper texture unit */
		uni_sampler_loc = gl.getUniformLocation(program_id, "uni_sampler");
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

		gl.activeTexture(GL_TEXTURE0 + m_texture_unit);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

		gl.bindTexture(GL_TEXTURE_2D, src_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.uniform1i(uni_sampler_loc, m_texture_unit);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		/* Draw */
		gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

		/* Sync */
		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");
	}
	catch (std::exception& exc)
	{
		gl.bindVertexArray(0);
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.bindTexture(GL_TEXTURE_2D, 0);

		if (0 != vao_id)
		{
			gl.deleteVertexArrays(1, &vao_id);
		}

		if (0 != fb_id)
		{
			gl.deleteFramebuffers(1, &fb_id);
		}

		throw exc;
	}

	gl.bindVertexArray(0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindTexture(GL_TEXTURE_2D, 0);

	if (0 != vao_id)
	{
		gl.deleteVertexArrays(1, &vao_id);
	}

	if (0 != fb_id)
	{
		gl.deleteFramebuffers(1, &fb_id);
	}
}

/** Prepare destination texture
 *
 * @param is_stencil Selects if stencil or depth channel is sampled
 *
 * @return ID of texture
 **/
GLuint FunctionalTest::prepareDestinationTexture(bool is_stencil)
{
	static const GLuint  n_pixels		 = m_width * m_height;
	GLenum				 format			 = 0;
	GLenum				 internal_format = 0;
	GLuint				 pixel_size		 = 0;
	std::vector<GLubyte> texture_data;
	GLuint				 texture_id   = 0;
	GLuint				 texture_size = 0;
	GLenum				 type		  = 0;

	/* Select size of pixel */
	if (true == is_stencil)
	{
		format			= GL_RED_INTEGER;
		internal_format = GL_R8UI;
		pixel_size		= 1;
		type			= GL_UNSIGNED_BYTE;
	}
	else
	{
		format			= GL_RED;
		internal_format = GL_R32F;
		pixel_size		= 4;
		type			= GL_FLOAT;
	}

	/* Allocate storage */
	texture_size = pixel_size * n_pixels;
	texture_data.resize(texture_size);

	/* Fill texture data */
	memset(&texture_data[0], 0, texture_size);

	/* Create texture */
	texture_id =
		Utils::createAndFill2DTexture(m_context, m_width, m_height, internal_format, format, type, &texture_data[0]);

	/* Done */
	return texture_id;
}

/** Prepare program
 *
 * @param is_draw    Selects if draw or compute program is prepared
 * @param is_stencil Selects if stencil or depth channel is sampled
 *
 * @return ID of texture
 **/
GLuint FunctionalTest::prepareProgram(bool is_draw, bool is_stencil)
{
	GLuint program_id = 0;

	if (true != is_draw)
	{
		std::string   cs_code			 = m_compute_shader_code;
		const GLchar* image_definition   = m_image_definition_stencil;
		size_t		  position			 = 0;
		const GLchar* sampler_definition = m_sampler_definition_stencil;
		const GLchar* type				 = m_output_type_stencil;

		if (false == is_stencil)
		{
			image_definition   = m_image_definition_depth;
			sampler_definition = m_sampler_definition_depth;
			type			   = m_output_type_depth;
		}

		Utils::replaceToken("IMAGE_DEFINITION", position, image_definition, cs_code);
		Utils::replaceToken("SAMPLER_DEFINITION", position, sampler_definition, cs_code);
		Utils::replaceToken("TYPE", position, type, cs_code);

		program_id = Utils::createAndBuildProgram(m_context, cs_code.c_str(), 0 /* fs_code */, 0 /* gs_code */,
												  0 /* tcs_code */, 0 /* tes_code */, 0 /* vs_code */);
	}
	else
	{
#define N_FUNCTIONAL_TEST_SHADER_STAGES 5

		const GLchar* expected_value	 = m_expected_value_stencil;
		const GLchar* sampler_definition = m_sampler_definition_stencil;
		std::string   shader_code[N_FUNCTIONAL_TEST_SHADER_STAGES];
		const GLchar* shader_templates[N_FUNCTIONAL_TEST_SHADER_STAGES] = {
			m_fragment_shader_code, m_geometry_shader_code, m_tesselation_control_shader_code,
			m_tesselation_evaluation_shader_code, m_vertex_shader_code
		};
		const GLchar* type = m_output_type_stencil;

		if (false == is_stencil)
		{
			expected_value	 = m_expected_value_depth;
			sampler_definition = m_sampler_definition_depth;
			type			   = m_output_type_depth;
		}

		for (GLuint i = 0; i < N_FUNCTIONAL_TEST_SHADER_STAGES; ++i)
		{
			size_t position = 0;

			shader_code[i] = shader_templates[i];

			if (0 == i)
			{
				Utils::replaceToken("TYPE", position, type, shader_code[i]);
				Utils::replaceToken("SAMPLER_DEFINITION", position, sampler_definition, shader_code[i]);
				//Utils::replaceToken("TYPE",               position, type,               shader_code[i]);
			}
			else
			{
				Utils::replaceToken("SAMPLER_DEFINITION", position, sampler_definition, shader_code[i]);
				Utils::replaceToken("EXPECTED_VALUE", position, expected_value, shader_code[i]);
			}
		}

		program_id =
			Utils::createAndBuildProgram(m_context, 0 /* cs_code */, shader_code[0].c_str() /* fs_code  */,
										 shader_code[1].c_str() /* gs_code  */, shader_code[2].c_str() /* tcs_code */,
										 shader_code[3].c_str() /* tes_code */, shader_code[4].c_str() /* vs_code  */);
	}

	/* Done */
	return program_id;
}

/** Prepare source texture
 *
 * @param internal_format Internal format of texture
 * @param is_stencil      Selects if stencil or depth channel is sampled
 * @param texture_data    Texture contents
 *
 * @return ID of texture
 **/
GLuint FunctionalTest::prepareSourceTexture(GLenum internal_format, bool is_stencil,
											const std::vector<glw::GLubyte>& texture_data)
{
	const Functions& gl			= m_context.getRenderContext().getFunctions();
	GLuint			 texture_id = 0;
	GLenum			 type		= 0;

	/* Select size of pixel */
	switch (internal_format)
	{
	case GL_DEPTH24_STENCIL8:
		type = GL_UNSIGNED_INT_24_8;
		break;
	case GL_DEPTH32F_STENCIL8:
		type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Create texture */
	texture_id = Utils::createAndFill2DTexture(m_context, m_width, m_height, internal_format, GL_DEPTH_STENCIL, type,
											   &texture_data[0]);

	/* Set DS texture mode */
	gl.bindTexture(GL_TEXTURE_2D, texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	if (true == is_stencil)
	{
		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	}
	else
	{
		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");
	}

	/* Set nearest filtering */
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

	/* Unbind */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* Done */
	return texture_id;
}

/** Prepare data for source texture
 *
 * @param internal_format Internal format of texture
 * @param texture_data    Texture contents
 *
 * @return ID of texture
 **/
void FunctionalTest::prepareSourceTextureData(GLenum internal_format, std::vector<GLubyte>& texture_data)
{
	static const GLfloat depth_step_h   = -0.5f / ((GLfloat)(m_width - 1));
	static const GLfloat depth_step_v   = -0.5f / ((GLfloat)(m_height - 1));
	static const GLuint  stencil_step_h = 1;
	static const GLuint  stencil_step_v = 1;
	static const GLuint  n_pixels		= m_width * m_height;
	GLuint				 pixel_size		= 0;
	GLuint				 line_size		= 0;
	GLuint				 texture_size   = 0;

	/* Select size of pixel */
	switch (internal_format)
	{
	case GL_DEPTH24_STENCIL8:
		pixel_size = 4;
		break;
	case GL_DEPTH32F_STENCIL8:
		pixel_size = 8;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	line_size	= pixel_size * m_width;
	texture_size = pixel_size * n_pixels;

	/* Allocate storage */
	texture_data.resize(texture_size);

	/* Fill texture data */
	for (GLuint y = 0; y < m_height; ++y)
	{
		const GLfloat depth_v	 = depth_step_v * (GLfloat)y;
		const GLuint  line_offset = line_size * y;
		const GLuint  stencil_v   = stencil_step_v * y;

		for (GLuint x = 0; x < m_width; ++x)
		{
			const GLfloat depth_h	  = depth_step_h * (GLfloat)x;
			const GLfloat depth_f	  = 1 + depth_h + depth_v;
			const GLuint  depth_i	  = (GLuint)(((GLfloat)0xffffff) * depth_f);
			const GLuint  pixel_offset = pixel_size * x;
			const GLuint  stencil_h	= stencil_step_h * x;
			const GLuint  stencil	  = 1 + stencil_h + stencil_v;

			GLubyte* depth_f_data = (GLubyte*)&depth_f;
			GLubyte* depth_i_data = (GLubyte*)&depth_i;
			GLubyte* pixel_data   = &texture_data[0] + line_offset + pixel_offset;
			GLubyte* stencil_data = (GLubyte*)&stencil;

			switch (pixel_size)
			{
			case 4:
				memcpy(pixel_data, stencil_data, 1);
				memcpy(pixel_data + 1, depth_i_data, 3);
				break;
			case 8:
				memcpy(pixel_data, depth_f_data, 4);
				memcpy(pixel_data + 4, stencil_data, 1);
				break;
			default:
				TCU_FAIL("Invalid value");
				break;
			}
		}
	}
}

/** Verifies that destination texture contents match expectations
 *
 * @param id                     ID of destination texture
 * @param source_internal_format Internal format of source texture
 * @param is_stencil             Selects if stencil of depth channel is sampled
 * @param src_texture_data       Contents of source texture
 *
 * @return true if everything is fine, false otherwise
 **/
bool FunctionalTest::verifyTexture(GLuint id, GLenum source_internal_format, bool is_stencil,
								   const std::vector<GLubyte>& src_texture_data)
{
	static const GLuint  n_pixels		= m_width * m_height;
	const Functions&	 gl				= m_context.getRenderContext().getFunctions();
	GLuint				 dst_pixel_size = 0;
	std::vector<GLubyte> dst_texture_data;
	GLuint				 dst_texture_size = 0;
	GLenum				 format			  = 0;
	GLuint				 src_pixel_size   = 0;
	GLuint				 src_stencil_off  = 0;
	GLenum				 type			  = 0;

	/* Select size of pixel */
	if (true == is_stencil)
	{
		format		   = GL_RED_INTEGER;
		dst_pixel_size = 1;
		type		   = GL_UNSIGNED_BYTE;
	}
	else
	{
		format		   = GL_RED;
		dst_pixel_size = 4;
		type		   = GL_FLOAT;
	}

	if (GL_DEPTH24_STENCIL8 == source_internal_format)
	{
		src_pixel_size = 4;
	}
	else
	{
		src_pixel_size  = 8;
		src_stencil_off = 4;
	}

	/* Allocate storage */
	dst_texture_size = dst_pixel_size * n_pixels;
	dst_texture_data.resize(dst_texture_size);

	/* Get texture contents */
	gl.bindTexture(GL_TEXTURE_2D, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, format, type, &dst_texture_data[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	/* For each pixel */
	for (GLuint i = 0; i < n_pixels; ++i)
	{
		const GLuint dst_pixel_offset = dst_pixel_size * i;
		const GLuint src_pixel_offset = src_pixel_size * i;

		const GLubyte* dst_pixel_data = &dst_texture_data[0] + dst_pixel_offset;
		const GLubyte* src_pixel_data = &src_texture_data[0] + src_pixel_offset;

		if (true == is_stencil) /* Stencil channel */
		{
			const GLubyte dst_stencil = dst_pixel_data[0];
			const GLubyte src_stencil = src_pixel_data[src_stencil_off];

			if (src_stencil != dst_stencil)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid pixel [" << i
													<< "], got: " << (GLuint)dst_stencil
													<< " expected: " << (GLuint)src_stencil << tcu::TestLog::EndMessage;

				return false;
			}
		}
		else /* Depth channel */
		{
			if (GL_DEPTH24_STENCIL8 == source_internal_format) /* DEPTH24 */
			{
				GLfloat dst_depth   = 0.0f;
				GLuint  src_depth_i = 0;
				GLfloat src_depth_f = 0.0f;

				memcpy(&dst_depth, dst_pixel_data, 4);
				memcpy(&src_depth_i, src_pixel_data + 1, 3);

				src_depth_f = ((GLfloat)src_depth_i) / ((GLfloat)0xffffff);

				if (de::abs(src_depth_f - dst_depth) > 0.0001f)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid pixel [" << i
														<< "], got: " << dst_depth << " expected: " << src_depth_f
														<< tcu::TestLog::EndMessage;

					return false;
				}
			}
			else /* DEPTH32F */
			{
				GLfloat dst_depth = 0.0f;
				GLfloat src_depth = 0.0f;

				memcpy(&dst_depth, dst_pixel_data, 4);
				memcpy(&src_depth, src_pixel_data, 4);

				if (de::abs(src_depth - dst_depth) > 0.0001f)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid pixel [" << i
														<< "], got: " << dst_depth << " expected: " << src_depth
														<< tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** Test given internal format and channel
 *
 * @param internal_format Internal fromat of source texture
 * @param is_stencil      Selects if stencil or depth channel is sampled
 *
 * @return true if results from compute and draw programs are positive, false otherwise
 **/
bool FunctionalTest::test(GLenum internal_format, bool is_stencil)
{
	GLuint				 compute_dst_tex_id = 0;
	GLuint				 compute_program_id = 0;
	GLuint				 compute_src_tex_id = 0;
	GLuint				 draw_dst_tex_id	= 0;
	GLuint				 draw_program_id	= 0;
	GLuint				 draw_src_tex_id	= 0;
	const Functions&	 gl					= m_context.getRenderContext().getFunctions();
	bool				 test_result		= true;
	std::vector<GLubyte> texture_data;

	prepareSourceTextureData(internal_format, texture_data);

	try
	{
		if (true == Utils::isExtensionSupported(m_context, "GL_ARB_compute_shader"))
		{
			compute_dst_tex_id = prepareDestinationTexture(is_stencil);
			compute_program_id = prepareProgram(false, is_stencil);
			compute_src_tex_id = prepareSourceTexture(internal_format, is_stencil, texture_data);

			dispatch(compute_program_id, is_stencil, compute_dst_tex_id, compute_src_tex_id);

			if (false == verifyTexture(compute_dst_tex_id, internal_format, is_stencil, texture_data))
			{
				test_result = false;
			}
		}

		{
			draw_dst_tex_id = prepareDestinationTexture(is_stencil);
			draw_program_id = prepareProgram(true, is_stencil);
			draw_src_tex_id = prepareSourceTexture(internal_format, is_stencil, texture_data);

			draw(draw_program_id, draw_dst_tex_id, draw_src_tex_id);

			if (false == verifyTexture(draw_dst_tex_id, internal_format, is_stencil, texture_data))
			{
				test_result = false;
			}
		}
	}
	catch (std::exception& exc)
	{
		gl.bindVertexArray(0);
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl.useProgram(0);

		Utils::deleteProgram(m_context, compute_program_id);
		Utils::deleteProgram(m_context, draw_program_id);

		Utils::deleteTexture(m_context, compute_dst_tex_id);
		Utils::deleteTexture(m_context, compute_src_tex_id);
		Utils::deleteTexture(m_context, draw_dst_tex_id);
		Utils::deleteTexture(m_context, draw_src_tex_id);

		TCU_FAIL(exc.what());
	}

	gl.bindVertexArray(0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.useProgram(0);

	Utils::deleteProgram(m_context, compute_program_id);
	Utils::deleteProgram(m_context, draw_program_id);

	Utils::deleteTexture(m_context, compute_dst_tex_id);
	Utils::deleteTexture(m_context, compute_src_tex_id);
	Utils::deleteTexture(m_context, draw_dst_tex_id);
	Utils::deleteTexture(m_context, draw_src_tex_id);

	/* Done */
	return test_result;
}
} /* namespace StencilTexturing */

StencilTexturingTests::StencilTexturingTests(deqp::Context& context) : TestCaseGroup(context, "stencil_texturing", "")
{
}

StencilTexturingTests::~StencilTexturingTests(void)
{
}

void StencilTexturingTests::init()
{
	addChild(new StencilTexturing::FunctionalTest(m_context));
}
} /* namespace gl4cts */
