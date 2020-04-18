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
 * \file  gl3GPUShader5Tests.cpp
 * \brief Implements conformance tests for "GPU Shader 5" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl3cGPUShader5Tests.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include "tcuTestLog.hpp"

#include <iomanip>

#include <deMath.h>
#include <tcuMatrixUtil.hpp>
#include <tcuVectorUtil.hpp>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>

namespace gl3cts
{

/** Constructor
 *
 * @param context Test context
 **/
Utils::programInfo::programInfo(deqp::Context& context)
	: m_context(context), m_fragment_shader_id(0), m_program_object_id(0), m_vertex_shader_id(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::programInfo::~programInfo()
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure program object is no longer used by GL */
	gl.useProgram(0);

	/* Clean program object */
	if (0 != m_program_object_id)
	{
		gl.deleteProgram(m_program_object_id);
		m_program_object_id = 0;
	}

	/* Clean shaders */
	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Build program
 *
 * @param fragment_shader_code               Fragment shader source code
 * @param vertex_shader_code                 Vertex shader source code
 * @param varying_names                      Array of strings containing names of varyings to be captured with transfrom feedback
 * @param n_varying_names                    Number of varyings to be captured with transfrom feedback
 **/
void Utils::programInfo::build(const glw::GLchar* fragment_shader_code, const glw::GLchar* vertex_shader_code)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects and compile */
	if (0 != fragment_shader_code)
	{
		m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_fragment_shader_id, fragment_shader_code);
	}

	if (0 != vertex_shader_code)
	{
		m_vertex_shader_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_vertex_shader_id, vertex_shader_code);
	}

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	/* Link program */
	link();
}

/** Compile shader
 *
 * @param shader_id   Shader object id
 * @param shader_code Shader source code
 **/
void Utils::programInfo::compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &shader_code, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	/* Compile */
	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	/* Get compilation status */
	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	/* Log compilation error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Error log length */
		gl.getShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length);

		/* Get error log */
		gl.getShaderInfoLog(shader_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to compile shader:\n"
											<< &message[0] << "\nShader source\n"
											<< shader_code << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to compile shader");
	}
}

/** Attach shaders and link program
 *
 **/
void Utils::programInfo::link() const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	if (0 != m_fragment_shader_id)
	{
		gl.attachShader(m_program_object_id, m_fragment_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_vertex_shader_id)
	{
		gl.attachShader(m_program_object_id, m_vertex_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	/* Link */
	gl.linkProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	/* Get link status */
	gl.getProgramiv(m_program_object_id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Log link error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Get error log length */
		gl.getProgramiv(m_program_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(length);

		/* Get error log */
		gl.getProgramInfoLog(m_program_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		/* Log */
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to link program:\n"
											<< &message[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to link program");
	}
}

/** Set the uniform variable with provided data.
 *
 * @param type       Type of variable
 * @param name       Name of variable
 * @param data       Data
 */
void Utils::programInfo::setUniform(Utils::_variable_type type, const glw::GLchar* name, const glw::GLvoid* data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const glw::GLfloat* f_data = (glw::GLfloat*)data;
	const glw::GLint*   i_data = (glw::GLint*)data;
	const glw::GLuint*  u_data = (glw::GLuint*)data;

	/* Get location */
	glw::GLint location = gl.getUniformLocation(m_program_object_id, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed.");

	if (-1 == location)
	{
		TCU_FAIL("Uniform variable is unavailable");
	}

	/* Set data */
	switch (type)
	{
	case Utils::VARIABLE_TYPE_FLOAT:
		gl.uniform1fv(location, 1, f_data);
		break;
	case Utils::VARIABLE_TYPE_INT:
		gl.uniform1iv(location, 1, i_data);
		break;
	case Utils::VARIABLE_TYPE_IVEC2:
		gl.uniform2iv(location, 1, i_data);
		break;
	case Utils::VARIABLE_TYPE_IVEC3:
		gl.uniform3iv(location, 1, i_data);
		break;
	case Utils::VARIABLE_TYPE_IVEC4:
		gl.uniform4iv(location, 1, i_data);
		break;
	case Utils::VARIABLE_TYPE_UINT:
		gl.uniform1uiv(location, 1, u_data);
		break;
	case Utils::VARIABLE_TYPE_UVEC2:
		gl.uniform2uiv(location, 1, u_data);
		break;
	case Utils::VARIABLE_TYPE_UVEC3:
		gl.uniform3uiv(location, 1, u_data);
		break;
	case Utils::VARIABLE_TYPE_UVEC4:
		gl.uniform4uiv(location, 1, u_data);
		break;
	case Utils::VARIABLE_TYPE_VEC2:
		gl.uniform2fv(location, 1, f_data);
		break;
	case Utils::VARIABLE_TYPE_VEC3:
		gl.uniform3fv(location, 1, f_data);
		break;
	case Utils::VARIABLE_TYPE_VEC4:
		gl.uniform4fv(location, 1, f_data);
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform");
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void Utils::replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
						 std::string& string)
{
	const size_t text_length	= strlen(text);
	const size_t token_length   = strlen(token);
	const size_t token_position = string.find(token, search_position);

	string.replace(token_position, token_length, text, text_length);

	search_position = token_position + text_length;
}

/* Constants used by GPUShader5ImplicitConversionsTest */
const glw::GLsizei GPUShader5ImplicitConversionsTest::m_width  = 8;
const glw::GLsizei GPUShader5ImplicitConversionsTest::m_height = 8;

/** Constructor.
 *
 * @param context Rendering context.
 **/
GPUShader5ImplicitConversionsTest::GPUShader5ImplicitConversionsTest(deqp::Context& context)
	: TestCase(context, "implicit_conversions",
			   "Verifies that implicit conversions are accepted and executed as explicit ones")
	, m_fbo_id(0)
	, m_tex_id(0)
	, m_vao_id(0)

{
	/* Left blank intentionally */
}

/** Constructor.
 *
 * @param context     Rendering context.
 * @param name        Name of test
 * @param description Describes test
 **/
GPUShader5ImplicitConversionsTest::GPUShader5ImplicitConversionsTest(deqp::Context& context, const char* name,
																	 const char* description)
	: TestCase(context, name, description), m_fbo_id(0), m_tex_id(0), m_vao_id(0)

{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during
 *  test execution.
 **/
void GPUShader5ImplicitConversionsTest::deinit()
{
	if (m_fbo_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_tex_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteTextures(1, &m_tex_id);

		m_tex_id = 0;
	}

	if (m_vao_id != 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult GPUShader5ImplicitConversionsTest::iterate()
{
	/* Defines data used as u1 and u2 uniforms */
	static const glw::GLint  uni_data_int_1[4]  = { 112, -1122, 111222, -1222111222 };
	static const glw::GLint  uni_data_int_2[4]  = { -112, 1122, -111222, 1222111222 };
	static const glw::GLuint uni_data_uint_1[4] = { 0xffff0000, 0x0000ffff, 0x00ffffff, 0xffffffff };
	static const glw::GLuint uni_data_uint_2[4] = { 0xfff70000, 0x00007fff, 0x007fffff, 0xfffffff7 };

	/* Defines test cases */
	static const testCase test_cases[] = {
		{ "uint", false, "int", Utils::VARIABLE_TYPE_INT, uni_data_int_1, uni_data_int_2 } /* int >> uint */,
		{ "uint", true, "int", Utils::VARIABLE_TYPE_INT, uni_data_int_1, uni_data_int_1 },
		{ "float", false, "int", Utils::VARIABLE_TYPE_INT, uni_data_int_1, uni_data_int_2 } /* int >> float */,
		{ "float", true, "int", Utils::VARIABLE_TYPE_INT, uni_data_int_2, uni_data_int_2 },
		{ "uvec2", false, "ivec2", Utils::VARIABLE_TYPE_IVEC2, uni_data_int_1, uni_data_int_2 } /* ivec2 >> uvec2 */,
		{ "uvec2", true, "ivec2", Utils::VARIABLE_TYPE_IVEC2, uni_data_int_1, uni_data_int_1 },
		{ "vec2", false, "ivec2", Utils::VARIABLE_TYPE_IVEC2, uni_data_int_1, uni_data_int_2 } /* ivec2 >> vec2 */,
		{ "vec2", true, "ivec2", Utils::VARIABLE_TYPE_IVEC2, uni_data_int_1, uni_data_int_1 },
		{ "uvec3", false, "ivec3", Utils::VARIABLE_TYPE_IVEC3, uni_data_int_1, uni_data_int_2 } /* ivec3 >> uvec3 */,
		{ "uvec3", true, "ivec3", Utils::VARIABLE_TYPE_IVEC3, uni_data_int_2, uni_data_int_2 },
		{ "vec3", false, "ivec3", Utils::VARIABLE_TYPE_IVEC3, uni_data_int_1, uni_data_int_2 } /* ivec3 >> vec3 */,
		{ "vec3", true, "ivec3", Utils::VARIABLE_TYPE_IVEC3, uni_data_int_2, uni_data_int_2 },
		{ "uvec4", false, "ivec4", Utils::VARIABLE_TYPE_IVEC4, uni_data_int_1, uni_data_int_2 } /* ivec4 >> uvec4 */,
		{ "uvec4", true, "ivec4", Utils::VARIABLE_TYPE_IVEC4, uni_data_int_1, uni_data_int_1 },
		{ "vec4", false, "ivec4", Utils::VARIABLE_TYPE_IVEC4, uni_data_int_1, uni_data_int_2 } /* ivec4 >> vec4 */,
		{ "vec4", true, "ivec4", Utils::VARIABLE_TYPE_IVEC4, uni_data_int_1, uni_data_int_1 },
		{ "float", false, "uint", Utils::VARIABLE_TYPE_UINT, uni_data_uint_1, uni_data_uint_2 } /* uint >> float */,
		{ "float", true, "uint", Utils::VARIABLE_TYPE_UINT, uni_data_uint_2, uni_data_uint_2 },
		{ "vec2", false, "uvec2", Utils::VARIABLE_TYPE_UVEC2, uni_data_uint_1, uni_data_uint_2 } /* uvec2 >> vec2 */,
		{ "vec2", true, "uvec2", Utils::VARIABLE_TYPE_UVEC2, uni_data_uint_1, uni_data_uint_1 },
		{ "vec3", false, "uvec3", Utils::VARIABLE_TYPE_UVEC3, uni_data_uint_1, uni_data_uint_2 } /* uvec3 >> vec3 */,
		{ "vec3", true, "uvec3", Utils::VARIABLE_TYPE_UVEC3, uni_data_uint_2, uni_data_uint_2 },
		{ "vec4", false, "uvec4", Utils::VARIABLE_TYPE_UVEC4, uni_data_uint_1, uni_data_uint_2 } /* uvec4 >> vec4 */,
		{ "vec4", true, "uvec4", Utils::VARIABLE_TYPE_UVEC4, uni_data_uint_1, uni_data_uint_1 },
	};
	static const size_t n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	testInit();

	/* Execute test cases */
	for (size_t i = 0; i < n_test_cases; ++i)
	{
		const testCase& test_case = test_cases[i];

		executeTestCase(test_case);
	}

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Initializes frame buffer and vertex array
 *
 **/
void GPUShader5ImplicitConversionsTest::testInit()
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare texture for color attachment 0 */
	gl.genTextures(1, &m_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_2D, m_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");

	/* Prepare FBO with color attachment 0 */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	/* Set Viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

	/* Prepare blank VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genVertexArrays");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindVertexArray");
}

/** Verifies if image is filled with <color>
 *
 * @param color       Color to be checked
 * @param is_expected Selects if image is expected to be filled with given color or not
 **/
void GPUShader5ImplicitConversionsTest::verifyImage(glw::GLuint color, bool is_expected) const
{
	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Storage for image data */
	glw::GLuint result_image[m_width * m_height];

	/* Get image data */
	gl.getTexImage(GL_TEXTURE_2D, 0 /* level */, GL_RGBA, GL_UNSIGNED_BYTE, result_image);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexImage");

	/* Inspect data */
	if (true == is_expected)
	{
		for (size_t i = 0; i < m_width * m_height; ++i)
		{
			const glw::GLuint pixel_data = result_image[i];

			if (color != pixel_data)
			{
				TCU_FAIL("Found invalid pixel during verification of drawn image");
			}
		}
	}
	else
	{
		for (size_t i = 0; i < m_width * m_height; ++i)
		{
			const glw::GLuint pixel_data = result_image[i];

			if (color == pixel_data)
			{
				TCU_FAIL("Found invalid pixel during verification of drawn image");
			}
		}
	}
}

/** Executes test case
 *
 * @param test_case Defines test case parameters
 */
void GPUShader5ImplicitConversionsTest::executeTestCase(const testCase& test_case)
{
	static const glw::GLuint white_color = 0xffffffff;

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Run test case */
	{
		/* Get shaders */
		const std::string& fs = getFragmentShader();
		const std::string& vs = getVertexShader(test_case.m_destination_type, test_case.m_source_type);

		/* Prepare program */
		Utils::programInfo program(m_context);

		program.build(fs.c_str(), vs.c_str());

		gl.useProgram(program.m_program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		program.setUniform(test_case.m_source_variable_type, "u1", test_case.m_u1_data);
		program.setUniform(test_case.m_source_variable_type, "u2", test_case.m_u2_data);

		/* Clear FBO */
		gl.clearColor(0.5f, 0.5f, 0.5f, 0.5f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clearColor");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

		/* Draw a triangle strip */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");
	}

	/* Verification */
	verifyImage(white_color, test_case.m_is_white_expected);
}

/** Get vertex shader source.
 *
 * @param destination_type Name of type
 * @param source_type      Name of type
 *
 * @return String with source of shader
 */
std::string GPUShader5ImplicitConversionsTest::getVertexShader(const glw::GLchar* destination_type,
															   const glw::GLchar* source_type)
{
	/* Vertex shader template */
	const char* vs_body_template = "#version 150\n"
								   "#extension GL_ARB_gpu_shader5 : require\n"
								   "\n"
								   "uniform SOURCE_TYPE u1;\n"
								   "uniform SOURCE_TYPE u2;\n"
								   "\n"
								   "out     vec4 result;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    DESTINATION_TYPE v = ZERO;\n"
								   "\n"
								   "    v = DESTINATION_TYPE(u2) - u1;\n"
								   "\n"
								   "    result = vec4(0.0, 0.0, 0.0, 0.0);\n"
								   "    if (ZERO == v)\n"
								   "    {\n"
								   "      result = vec4(1.0, 1.0, 1.0, 1.0);\n"
								   "    }\n"
								   "\n"
								   "    switch (gl_VertexID)\n"
								   "    {\n"
								   "      case 0: gl_Position = vec4(-1.0, 1.0, 0.0, 1.0); break; \n"
								   "      case 1: gl_Position = vec4( 1.0, 1.0, 0.0, 1.0); break; \n"
								   "      case 2: gl_Position = vec4(-1.0,-1.0, 0.0, 1.0); break; \n"
								   "      case 3: gl_Position = vec4( 1.0,-1.0, 0.0, 1.0); break; \n"
								   "    }\n"
								   "}\n"
								   "\n";

	std::string vs_body = vs_body_template;

	/* Tokens */
	size_t search_position = 0;

	Utils::replaceToken("SOURCE_TYPE", search_position, source_type, vs_body);
	Utils::replaceToken("SOURCE_TYPE", search_position, source_type, vs_body);

	search_position = 0;
	Utils::replaceToken("DESTINATION_TYPE", search_position, destination_type, vs_body);
	Utils::replaceToken("DESTINATION_TYPE", search_position, destination_type, vs_body);

	search_position = 0;
	if (!strcmp(destination_type, "int") || !strcmp(destination_type, "uint"))
	{
		Utils::replaceToken("ZERO", search_position, "0", vs_body);
		Utils::replaceToken("ZERO", search_position, "0", vs_body);
	}
	else if (!strcmp(destination_type, "float"))
	{
		Utils::replaceToken("ZERO", search_position, "0.0", vs_body);
		Utils::replaceToken("ZERO", search_position, "0.0", vs_body);
	}
	else if (!strcmp(destination_type, "ivec2"))
	{
		Utils::replaceToken("ZERO", search_position, "ivec2(0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "ivec2(0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "ivec3"))
	{
		Utils::replaceToken("ZERO", search_position, "ivec3(0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "ivec3(0,0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "ivec4"))
	{
		Utils::replaceToken("ZERO", search_position, "ivec4(0,0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "ivec4(0,0,0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "uvec2"))
	{
		Utils::replaceToken("ZERO", search_position, "uvec2(0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "uvec2(0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "uvec3"))
	{
		Utils::replaceToken("ZERO", search_position, "uvec3(0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "uvec3(0,0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "uvec4"))
	{
		Utils::replaceToken("ZERO", search_position, "uvec4(0,0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "uvec4(0,0,0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "vec2"))
	{
		Utils::replaceToken("ZERO", search_position, "vec2(0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "vec2(0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "vec3"))
	{
		Utils::replaceToken("ZERO", search_position, "vec3(0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "vec3(0,0,0)", vs_body);
	}
	else if (!strcmp(destination_type, "vec4"))
	{
		Utils::replaceToken("ZERO", search_position, "vec4(0,0,0,0)", vs_body);
		Utils::replaceToken("ZERO", search_position, "vec4(0,0,0,0)", vs_body);
	}

	return vs_body;
}

/** Get fragment shader source.
 *
 * @return String with source of shader
 */
std::string GPUShader5ImplicitConversionsTest::getFragmentShader()
{
	const char* fs_body_template = "#version 150\n"
								   "\n"
								   "in  vec4 result;\n"
								   "out vec4 color;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    color = result;\n"
								   "}\n"
								   "\n";

	std::string fs_body = fs_body_template;

	return fs_body;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
GPUShader5FunctionOverloadingTest::GPUShader5FunctionOverloadingTest(deqp::Context& context)
	: GPUShader5ImplicitConversionsTest(context, "function_overloading",
										"Verifies that function overloading is accepted")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult GPUShader5FunctionOverloadingTest::iterate()
{
	/* Defines data used as u1 and u2 uniforms */
	static const glw::GLint  u1_data_1[4] = { (glw::GLint)0xffff0000, 0x0000ffff, 0x00ffffff, (glw::GLint)0xffffffff };
	static const glw::GLint  u1_data_2[4] = { -112, 1122, -111222, 1222111222 };
	static const glw::GLuint u2_data_1[4] = { 0xffff0000, 0x0000ffff, 0x00ffffff, 0xffffffff };
	static const glw::GLuint u2_data_2[4] = { 0xfff70000, 0x00007fff, 0x007fffff, 0xfffffff7 };

	testInit();

	/* Execute test case */
	execute(u1_data_1, u2_data_1, true);
	execute(u1_data_2, u2_data_2, false);

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Executes test case
 *
 * @param u1_data   Pointer to data that will used as u1 uniform
 * @param u2_data   Pointer to data that will used as u2 uniform
 * @param test_case Defines test case parameters
 */
void GPUShader5FunctionOverloadingTest::execute(const glw::GLint* u1_data, const glw::GLuint* u2_data,
												bool is_black_expected)
{
	static const glw::GLuint black_color = 0x00000000;

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Run test case */
	{
		/* Shaders */
		const char* fs = "#version 150\n"
						 "\n"
						 "in  vec4 result;\n"
						 "out vec4 color;\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "    color = result;\n"
						 "}\n"
						 "\n";

		const char* vs = "#version 150\n"
						 "#extension GL_ARB_gpu_shader5 : require\n"
						 "\n"
						 "uniform ivec4 u1;\n"
						 "uniform uvec4 u2;\n"
						 "\n"
						 "out     vec4  result;\n"
						 "\n"
						 "vec4 f(in vec4 a, in vec4 b)\n"
						 "{\n"
						 "    return a * b;\n"
						 "}\n"
						 "\n"
						 "vec4 f(in uvec4 a, in uvec4 b)\n"
						 "{\n"
						 "    return vec4(a - b);\n"
						 "}\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "    result = f(u1, u2);\n"
						 "\n"
						 "    switch (gl_VertexID)\n"
						 "    {\n"
						 "      case 0: gl_Position = vec4(-1.0, 1.0, 0.0, 1.0); break; \n"
						 "      case 1: gl_Position = vec4( 1.0, 1.0, 0.0, 1.0); break; \n"
						 "      case 2: gl_Position = vec4(-1.0,-1.0, 0.0, 1.0); break; \n"
						 "      case 3: gl_Position = vec4( 1.0,-1.0, 0.0, 1.0); break; \n"
						 "    }\n"
						 "}\n"
						 "\n";

		/* Prepare program */
		Utils::programInfo program(m_context);

		program.build(fs, vs);

		gl.useProgram(program.m_program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		program.setUniform(Utils::VARIABLE_TYPE_IVEC4, "u1", u1_data);
		program.setUniform(Utils::VARIABLE_TYPE_UVEC4, "u2", u2_data);

		/* Clear FBO */
		gl.clearColor(0.5f, 0.5f, 0.5f, 0.5f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clearColor");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

		/* Draw a triangle strip */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");
	}

	/* Verification */
	verifyImage(black_color, is_black_expected);
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
GPUShader5FloatEncodingTest::GPUShader5FloatEncodingTest(deqp::Context& context)
	: GPUShader5ImplicitConversionsTest(context, "float_encoding",
										"Verifies that functions encoding floats as bits work as expected")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP.
 */
tcu::TestNode::IterateResult GPUShader5FloatEncodingTest::iterate()
{
	/* Defines data used as u1 and u2 uniforms */
	static const glw::GLfloat floats[4] = { -1.0f, -1234.0f, 1.0f, 1234.0f };
	static const glw::GLint   ints[4]   = { -1, -1234, 1, 1234 };
	static const glw::GLuint  uints[4]  = { 0xffffffff, 0xfffffb2e, 1, 0x4d2 };

	/* Defines tested cases */
	static const testCase test_cases[] = {
		{ /* float >> int - invalid */
		  { Utils::VARIABLE_TYPE_INT, "int", ints },
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  "floatBitsToInt",
		  false },
		{ /* float >> int - valid */
		  { Utils::VARIABLE_TYPE_INT, "int", floats },
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  "floatBitsToInt",
		  true },
		{ /* vec2 >> ivec2 - invalid */
		  { Utils::VARIABLE_TYPE_IVEC2, "ivec2", ints },
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  "floatBitsToInt",
		  false },
		{ /* vec2 >> ivec2 - valid */
		  { Utils::VARIABLE_TYPE_IVEC2, "ivec2", floats },
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  "floatBitsToInt",
		  true },
		{ /* vec3 >> ivec3 - invalid */
		  { Utils::VARIABLE_TYPE_IVEC3, "ivec3", ints },
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  "floatBitsToInt",
		  false },
		{ /* vec3 >> ivec3 - valid */
		  { Utils::VARIABLE_TYPE_IVEC3, "ivec3", floats },
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  "floatBitsToInt",
		  true },
		{ /* vec4 >> ivec4 - invalid */
		  { Utils::VARIABLE_TYPE_IVEC4, "ivec4", ints },
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  "floatBitsToInt",
		  false },
		{ /* vec4 >> ivec4 - valid */
		  { Utils::VARIABLE_TYPE_IVEC4, "ivec4", floats },
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  "floatBitsToInt",
		  true },
		{ /* float >> uint - invalid */
		  { Utils::VARIABLE_TYPE_UINT, "uint", uints },
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  "floatBitsToUint",
		  false },
		{ /* float >> uint - valid */
		  { Utils::VARIABLE_TYPE_UINT, "uint", floats },
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  "floatBitsToUint",
		  true },
		{ /* vec2 >> uvec2 - invalid */
		  { Utils::VARIABLE_TYPE_UVEC2, "uvec2", uints },
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  "floatBitsToUint",
		  false },
		{ /* vec2 >> uvec2 - valid */
		  { Utils::VARIABLE_TYPE_UVEC2, "uvec2", floats },
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  "floatBitsToUint",
		  true },
		{ /* vec3 >> uvec3 - invalid */
		  { Utils::VARIABLE_TYPE_UVEC3, "uvec3", uints },
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  "floatBitsToUint",
		  false },
		{ /* vec3 >> uvec3 - valid */
		  { Utils::VARIABLE_TYPE_UVEC3, "uvec3", floats },
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  "floatBitsToUint",
		  true },
		{ /* vec4 >> ivec4 - invalid */
		  { Utils::VARIABLE_TYPE_UVEC4, "uvec4", uints },
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  "floatBitsToUint",
		  false },
		{ /* vec4 >> uvec4 - valid */
		  { Utils::VARIABLE_TYPE_UVEC4, "uvec4", floats },
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  "floatBitsToUint",
		  true },
		{ /* int >> float - invalid */
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  { Utils::VARIABLE_TYPE_INT, "int", ints },
		  "intBitsToFloat",
		  false },
		{ /* int >> float - valid */
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  { Utils::VARIABLE_TYPE_INT, "int", floats },
		  "intBitsToFloat",
		  true },
		{ /* ivec2 >> vec2 - invalid */
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  { Utils::VARIABLE_TYPE_IVEC2, "ivec2", ints },
		  "intBitsToFloat",
		  false },
		{ /* ivec2 >> vec2 - valid */
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  { Utils::VARIABLE_TYPE_IVEC2, "ivec2", floats },
		  "intBitsToFloat",
		  true },
		{ /* ivec3 >> vec3 - invalid */
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  { Utils::VARIABLE_TYPE_IVEC3, "ivec3", ints },
		  "intBitsToFloat",
		  false },
		{ /* ivec3 >> vec3 - valid */
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  { Utils::VARIABLE_TYPE_IVEC3, "ivec3", floats },
		  "intBitsToFloat",
		  true },
		{ /* ivec4 >> vec4 - invalid */
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  { Utils::VARIABLE_TYPE_IVEC4, "ivec4", ints },
		  "intBitsToFloat",
		  false },
		{ /* ivec4 >> vec4 - valid */
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  { Utils::VARIABLE_TYPE_IVEC4, "ivec4", floats },
		  "intBitsToFloat",
		  true },
		{ /* uint >> float - invalid */
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  { Utils::VARIABLE_TYPE_UINT, "uint", uints },
		  "uintBitsToFloat",
		  false },
		{ /* uint >> float - valid */
		  { Utils::VARIABLE_TYPE_FLOAT, "float", floats },
		  { Utils::VARIABLE_TYPE_UINT, "uint", floats },
		  "uintBitsToFloat",
		  true },
		{ /* uvec2 >> vec2 - invalid */
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  { Utils::VARIABLE_TYPE_UVEC2, "uvec2", uints },
		  "uintBitsToFloat",
		  false },
		{ /* uvec2 >> vec2 - valid */
		  { Utils::VARIABLE_TYPE_VEC2, "vec2", floats },
		  { Utils::VARIABLE_TYPE_UVEC2, "uvec2", floats },
		  "uintBitsToFloat",
		  true },
		{ /* uvec3 >> vec3 - invalid */
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  { Utils::VARIABLE_TYPE_UVEC3, "uvec3", uints },
		  "uintBitsToFloat",
		  false },
		{ /* uvec3 >> vec3 - valid */
		  { Utils::VARIABLE_TYPE_VEC3, "vec3", floats },
		  { Utils::VARIABLE_TYPE_UVEC3, "uvec3", floats },
		  "uintBitsToFloat",
		  true },
		{ /* uvec4 >> vec4 - invalid */
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  { Utils::VARIABLE_TYPE_UVEC4, "uvec4", uints },
		  "uintBitsToFloat",
		  false },
		{ /* uvec4 >> vec4 - valid */
		  { Utils::VARIABLE_TYPE_VEC4, "vec4", floats },
		  { Utils::VARIABLE_TYPE_UVEC4, "uvec4", floats },
		  "uintBitsToFloat",
		  true },
	};
	static const size_t n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	testInit();

	/* Execute test case */
	for (size_t i = 0; i < n_test_cases; ++i)
	{
		const testCase& test_case = test_cases[i];

		execute(test_case);
	}

	/* Set result - exceptions are thrown in case of any error */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return STOP;
}

/** Executes test case
 *
 * @param test_case Tested case
 *
 * @param test_case Defines test case parameters
 */
void GPUShader5FloatEncodingTest::execute(const testCase& test_case)
{
	static const glw::GLuint white_color = 0xffffffff;

	/*  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Run test case */
	{
		/* Shaders */
		const char* fs = "#version 150\n"
						 "\n"
						 "in  vec4 result;\n"
						 "out vec4 color;\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "    color = result;\n"
						 "}\n"
						 "\n";

		const std::string& vs = getVertexShader(test_case);

		/* Prepare program */
		Utils::programInfo program(m_context);

		program.build(fs, vs.c_str());

		gl.useProgram(program.m_program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		program.setUniform(test_case.m_expected_value.m_type, "expected_value", test_case.m_expected_value.m_data);
		program.setUniform(test_case.m_value.m_type, "value", test_case.m_value.m_data);

		/* Clear FBO */
		gl.clearColor(0.5f, 0.5f, 0.5f, 0.5f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clearColor");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

		/* Draw a triangle strip */
		gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");
	}

	/* Verification */
	verifyImage(white_color, test_case.m_is_white_expected);
}

/** Get vertex shader source.
 *
 * @param test_case Tested case
 *
 * @return String with source of shader
 */
std::string GPUShader5FloatEncodingTest::getVertexShader(const testCase& test_case) const
{
	/* Vertex shader template */
	const char* vs_body_template = "#version 150\n"
								   "#extension GL_ARB_gpu_shader5 : require\n"
								   "\n"
								   "uniform EXPECTED_VALUE_TYPE expected_value;\n"
								   "uniform VALUE_TYPE value;\n"
								   "\n"
								   "out     vec4 result;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    result = vec4(1.0, 1.0, 1.0, 1.0);\n"
								   "\n"
								   "    EXPECTED_VALUE_TYPE ret_val = TESTED_FUNCTION(value);\n"
								   "\n"
								   "    if (expected_value != ret_val)\n"
								   "    {\n"
								   "        result = vec4(0.0, 0.0, 0.0, 0.0);\n"
								   "    }\n"
								   "\n"
								   "    switch (gl_VertexID)\n"
								   "    {\n"
								   "      case 0: gl_Position = vec4(-1.0, 1.0, 0.0, 1.0); break; \n"
								   "      case 1: gl_Position = vec4( 1.0, 1.0, 0.0, 1.0); break; \n"
								   "      case 2: gl_Position = vec4(-1.0,-1.0, 0.0, 1.0); break; \n"
								   "      case 3: gl_Position = vec4( 1.0,-1.0, 0.0, 1.0); break; \n"
								   "    }\n"
								   "}\n"
								   "\n";

	std::string vs_body = vs_body_template;

	/* Tokens */
	size_t search_position = 0;

	Utils::replaceToken("EXPECTED_VALUE_TYPE", search_position, test_case.m_expected_value.m_type_name, vs_body);
	Utils::replaceToken("VALUE_TYPE", search_position, test_case.m_value.m_type_name, vs_body);
	Utils::replaceToken("EXPECTED_VALUE_TYPE", search_position, test_case.m_expected_value.m_type_name, vs_body);
	Utils::replaceToken("TESTED_FUNCTION", search_position, test_case.m_function_name, vs_body);

	return vs_body;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
GPUShader5Tests::GPUShader5Tests(deqp::Context& context)
	: TestCaseGroup(context, "gpu_shader5_gl", "Verifies \"gpu_shader5\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void GPUShader5Tests::init(void)
{
	addChild(new GPUShader5ImplicitConversionsTest(m_context));
	addChild(new GPUShader5FunctionOverloadingTest(m_context));
	addChild(new GPUShader5FloatEncodingTest(m_context));
}
} /* glcts namespace */
