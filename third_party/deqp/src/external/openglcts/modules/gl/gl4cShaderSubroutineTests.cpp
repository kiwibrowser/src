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
 * \file  gl4cShaderSubroutineTests.cpp
 * \brief Implements conformance tests for "Shader Subroutine" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cShaderSubroutineTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include <cmath>
#include <cstring>
#include <deMath.h>

using namespace glw;

namespace gl4cts
{
namespace ShaderSubroutine
{
/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::buffer::buffer(deqp::Context& context) : m_id(0), m_context(context)
{
}

/** Destructor
 *
 **/
Utils::buffer::~buffer()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteBuffers(1, &m_id);
		m_id = 0;
	}
}

/** Execute BindBufferRange
 *
 * @param target <target> parameter
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Utils::buffer::bindRange(glw::GLenum target, glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBufferRange(target, index, m_id, offset, size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");
}

/** Execute GenBuffer
 *
 **/
void Utils::buffer::generate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");
}

/** Execute BufferData
 *
 * @param target <target> parameter
 * @param size   <size> parameter
 * @param data   <data> parameter
 * @param usage  <usage> parameter
 **/
void Utils::buffer::update(glw::GLenum target, glw::GLsizeiptr size, glw::GLvoid* data, glw::GLenum usage)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	gl.bufferData(target, size, data, usage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bufferData");
}

/** Constructor
 *
 * @param context CTS context
 **/
Utils::framebuffer::framebuffer(deqp::Context& context) : m_id(0), m_context(context)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::framebuffer::~framebuffer()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteFramebuffers(1, &m_id);
		m_id = 0;
	}
}

/** Attach texture to specified attachment
 *
 * @param attachment Attachment
 * @param texture_id Texture id
 * @param width      Texture width
 * @param height     Texture height
 **/
void Utils::framebuffer::attachTexture(glw::GLenum attachment, glw::GLuint texture_id, glw::GLuint width,
									   glw::GLuint height)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bind();

	gl.bindTexture(GL_TEXTURE_2D, texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	gl.viewport(0 /* x */, 0 /* y */, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}

/** Binds framebuffer to DRAW_FRAMEBUFFER
 *
 **/
void Utils::framebuffer::bind()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
}

/** Clear framebuffer
 *
 * @param mask <mask> parameter of glClear. Decides which shall be cleared
 **/
void Utils::framebuffer::clear(glw::GLenum mask)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clear(mask);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
}

/** Specifie clear color
 *
 * @param red   Red channel
 * @param green Green channel
 * @param blue  Blue channel
 * @param alpha Alpha channel
 **/
void Utils::framebuffer::clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clearColor(red, green, blue, alpha);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");
}

/** Generate framebuffer
 *
 **/
void Utils::framebuffer::generate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genFramebuffers(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");
}

const glw::GLenum Utils::program::ARB_COMPUTE_SHADER = 0x91B9;

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::program::program(deqp::Context& context)
	: m_compute_shader_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_tesselation_control_shader_id(0)
	, m_tesselation_evaluation_shader_id(0)
	, m_vertex_shader_id(0)
	, m_context(context)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Utils::program::~program()
{
	remove();
}

/** Build program
 *
 * @param compute_shader_code                Compute shader source code
 * @param fragment_shader_code               Fragment shader source code
 * @param geometry_shader_code               Geometry shader source code
 * @param tesselation_control_shader_code    Tesselation control shader source code
 * @param tesselation_evaluation_shader_code Tesselation evaluation shader source code
 * @param vertex_shader_code                 Vertex shader source code
 * @param varying_names                      Array of strings containing names of varyings to be captured with transfrom feedback
 * @param n_varying_names                    Number of varyings to be captured with transfrom feedback
 * @param is_separable                       Selects if monolithis or separable program should be built. Defaults to false
 **/
void Utils::program::build(const glw::GLchar* compute_shader_code, const glw::GLchar* fragment_shader_code,
						   const glw::GLchar* geometry_shader_code, const glw::GLchar* tesselation_control_shader_code,
						   const glw::GLchar* tesselation_evaluation_shader_code, const glw::GLchar* vertex_shader_code,
						   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names, bool is_separable)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects and compile */
	if (0 != compute_shader_code)
	{
		m_compute_shader_id = gl.createShader(ARB_COMPUTE_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_compute_shader_id, compute_shader_code);
	}

	if (0 != fragment_shader_code)
	{
		m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_fragment_shader_id, fragment_shader_code);
	}

	if (0 != geometry_shader_code)
	{
		m_geometry_shader_id = gl.createShader(GL_GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_geometry_shader_id, geometry_shader_code);
	}

	if (0 != tesselation_control_shader_code)
	{
		m_tesselation_control_shader_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_control_shader_id, tesselation_control_shader_code);
	}

	if (0 != tesselation_evaluation_shader_code)
	{
		m_tesselation_evaluation_shader_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

		compile(m_tesselation_evaluation_shader_id, tesselation_evaluation_shader_code);
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

	/* Set up captyured varyings' names */
	if (0 != n_varying_names)
	{
		gl.transformFeedbackVaryings(m_program_object_id, n_varying_names, varying_names, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TransformFeedbackVaryings");
	}

	/* Set separable parameter */
	if (true == is_separable)
	{
		gl.programParameteri(m_program_object_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramParameteri");
	}

	/* Link program */
	link();
}

/** Compile shader
 *
 * @param shader_id   Shader object id
 * @param shader_code Shader source code
 **/
void Utils::program::compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const
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

/** Checks whether the tested driver supports GL_ARB_get_program_binary
 *
 *  @return true if the extension is supported and, also, at least one binary format.
 **/
bool Utils::program::isProgramBinarySupported() const
{
	glw::GLint n_program_binary_formats = 0;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_get_program_binary"))
	{
		gl.getIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &n_program_binary_formats);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed.");
	}

	return n_program_binary_formats > 0;
}

/** Create program from provided binary
 *
 * @param binary        Buffer with binary form of program
 * @param binary_format Format of <binary> data
 **/
void Utils::program::createFromBinary(const std::vector<GLubyte>& binary, GLenum binary_format)
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object */
	m_program_object_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	gl.programBinary(m_program_object_id, binary_format, &binary[0], (GLsizei)binary.size());
	GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramBinary");
}

/** Get binary form of program
 *
 * @param binary        Buffer for binary data
 * @param binary_format Format of binary data
 **/
void Utils::program::getBinary(std::vector<GLubyte>& binary, GLenum& binary_format) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get binary size */
	GLint length = 0;
	gl.getProgramiv(m_program_object_id, GL_PROGRAM_BINARY_LENGTH, &length);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Allocate storage */
	binary.resize(length);

	/* Get binary */
	gl.getProgramBinary(m_program_object_id, (GLsizei)binary.size(), &length, &binary_format, &binary[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramBinary");
}

/** Get subroutine index
 *
 * @param subroutine_name Subroutine name
 *
 * @return Index of subroutine
 **/
GLuint Utils::program::getSubroutineIndex(const glw::GLchar* subroutine_name, glw::GLenum shader_stage) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLuint				  index = -1;

	index = gl.getSubroutineIndex(m_program_object_id, shader_stage, subroutine_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineIndex");

	if (GL_INVALID_INDEX == index)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Subroutine: " << subroutine_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Subroutine is not available");
	}

	return index;
}

/** Get subroutine uniform location
 *
 * @param uniform_name Subroutine uniform name
 *
 * @return Location of subroutine uniform
 **/
GLint Utils::program::getSubroutineUniformLocation(const glw::GLchar* uniform_name, glw::GLenum shader_stage) const
{
	const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
	GLint				  location = -1;

	location = gl.getSubroutineUniformLocation(m_program_object_id, shader_stage, uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineUniformLocation");

	if (-1 == location)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Subroutine uniform: " << uniform_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Subroutine uniform is not available");
	}

	return location;
}

/** Get uniform location
 *
 * @param uniform_name Subroutine uniform name
 *
 * @return Location of uniform
 **/
GLint Utils::program::getUniformLocation(const glw::GLchar* uniform_name) const
{
	const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
	GLint				  location = -1;

	location = gl.getUniformLocation(m_program_object_id, uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	if (-1 == location)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name
											<< " is not available" << tcu::TestLog::EndMessage;

		TCU_FAIL("Uniform is not available");
	}

	return location;
}

/** Attach shaders and link program
 *
 **/
void Utils::program::link() const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	if (0 != m_compute_shader_id)
	{
		gl.attachShader(m_program_object_id, m_compute_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_fragment_shader_id)
	{
		gl.attachShader(m_program_object_id, m_fragment_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_geometry_shader_id)
	{
		gl.attachShader(m_program_object_id, m_geometry_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_tesselation_control_shader_id)
	{
		gl.attachShader(m_program_object_id, m_tesselation_control_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != m_tesselation_evaluation_shader_id)
	{
		gl.attachShader(m_program_object_id, m_tesselation_evaluation_shader_id);
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

/** Delete program object and all attached shaders
 *
 **/
void Utils::program::remove()
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
	if (0 != m_compute_shader_id)
	{
		gl.deleteShader(m_compute_shader_id);
		m_compute_shader_id = 0;
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
		m_fragment_shader_id = 0;
	}

	if (0 != m_geometry_shader_id)
	{
		gl.deleteShader(m_geometry_shader_id);
		m_geometry_shader_id = 0;
	}

	if (0 != m_tesselation_control_shader_id)
	{
		gl.deleteShader(m_tesselation_control_shader_id);
		m_tesselation_control_shader_id = 0;
	}

	if (0 != m_tesselation_evaluation_shader_id)
	{
		gl.deleteShader(m_tesselation_evaluation_shader_id);
		m_tesselation_evaluation_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
		m_vertex_shader_id = 0;
	}
}

/** Execute UseProgram
 *
 **/
void Utils::program::use() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::texture::texture(deqp::Context& context) : m_id(0), m_context(context)
{
	/* Nothing to done here */
}

/** Destructor
 *
 **/
Utils::texture::~texture()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = 0;
	}
}

/** Bind texture to GL_TEXTURE_2D
 *
 **/
void Utils::texture::bind()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(GL_TEXTURE_2D, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Create 2d texture
 *
 * @param width           Width of texture
 * @param height          Height of texture
 * @param internal_format Internal format of texture
 **/
void Utils::texture::create(glw::GLuint width, glw::GLuint height, glw::GLenum internal_format)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, internal_format, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
}

/** Get contents of texture
 *
 * @param format   Format of image
 * @param type     Type of image
 * @param out_data Buffer for image
 **/
void Utils::texture::get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bind();

	gl.getTexImage(GL_TEXTURE_2D, 0, format, type, out_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
}

/** Update contents of texture
 *
 * @param width  Width of texture
 * @param height Height of texture
 * @param format Format of data
 * @param type   Type of data
 * @param data   Buffer with image
 **/
void Utils::texture::update(glw::GLuint width, glw::GLuint height, glw::GLenum format, glw::GLenum type,
							glw::GLvoid* data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bind();

	gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, width, height, format, type, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::vertexArray::vertexArray(deqp::Context& context) : m_id(0), m_context(context)
{
}

/** Destructor
 *
 **/
Utils::vertexArray::~vertexArray()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteVertexArrays(1, &m_id);

		m_id = 0;
	}
}

/** Execute BindVertexArray
 *
 **/
void Utils::vertexArray::bind()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindVertexArray(m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");
}

/** Execute GenVertexArrays
 *
 **/
void Utils::vertexArray::generate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");
}

/** Builds a program object consisting of up to 5 shader stages
 *  (vertex/tessellation control/tessellation evaluation/geometry/fragment).
 *  The shaders are attached to the program object, then compiled. Finally,
 *  the program object is linked.
 *
 *  XFB can be optionally configured for the program object.
 *
 *  Should an error be reported by GL implementation, a TestError
 *  exception will be thrown.
 *
 *  @param gl             OpenGL functions from the active rendering context.
 *  @param vs_body        Body to use for the vertex shader. Can be an empty string.
 *  @param tc_body        Body to use for the tessellation control shader. Can be
 *                        an empty string.
 *  @param te_body        Body to use for the tessellation evaluation shader. Can be
 *                        an empty string.
 *  @param gs_body        Body to use for the geometry shader. Can be an empty string.
 *  @param fs_body        Body to use for the fragment shader. Can be an empty string.
 *  @param xfb_varyings   An array of names of varyings to use for XFB. Can be NULL.
 *  @param n_xfb_varyings Amount of XFB varyings defined in @param xfb_varyings.Can be 0.
 *  @param out_vs_id      Deref will be used to store GL id of a generated vertex shader.
 *                        Can be NULL in which case no vertex shader will be used for the
 *                        program object.
 *  @param out_tc_id      Deref will be used to store GL id of a generated tess control shader.
 *                        Can be NULL in which case no tess control shader will be used for the
 *                        program object.
 *  @param out_te_id      Deref will be used to store GL id of a generated tess evaluation shader.
 *                        Can be NULL in which case no tess evaluation shader will be used for the
 *                        program object.
 *  @param out_gs_id      Deref will be used to store GL id of a generated geometry shader.
 *                        Can be NULL in which case no geometry shader will be used for the
 *                        program object.
 *  @param out_fs_id      Deref will be used to store GL id of a generated fragment shader.
 *                        Can be NULL in which case no fragment shader will be used for the
 *                        program object.
 *  @param out_po_id      Deref will be used to store GL id of a generated program object.
 *                        Must not be NULL.
 *
 *  @return true if the program was built successfully, false otherwise.
 *  */
bool Utils::buildProgram(const glw::Functions& gl, const std::string& vs_body, const std::string& tc_body,
						 const std::string& te_body, const std::string& gs_body, const std::string& fs_body,
						 const glw::GLchar** xfb_varyings, const unsigned int& n_xfb_varyings, glw::GLuint* out_vs_id,
						 glw::GLuint* out_tc_id, glw::GLuint* out_te_id, glw::GLuint* out_gs_id, glw::GLuint* out_fs_id,
						 glw::GLuint* out_po_id)
{
	bool result = false;

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	/* Create objects, set up shader bodies and attach all requested shaders to the program object */
	*out_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	if (out_vs_id != DE_NULL)
	{
		const char* vs_body_raw_ptr = vs_body.c_str();

		*out_vs_id = gl.createShader(GL_VERTEX_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

		gl.attachShader(*out_po_id, *out_vs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		gl.shaderSource(*out_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	if (out_tc_id != DE_NULL)
	{
		const char* tc_body_raw_ptr = tc_body.c_str();

		*out_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

		gl.attachShader(*out_po_id, *out_tc_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		gl.shaderSource(*out_tc_id, 1 /* count */, &tc_body_raw_ptr, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	if (out_te_id != DE_NULL)
	{
		const char* te_body_raw_ptr = te_body.c_str();

		*out_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

		gl.attachShader(*out_po_id, *out_te_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		gl.shaderSource(*out_te_id, 1 /* count */, &te_body_raw_ptr, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	if (out_gs_id != DE_NULL)
	{
		const char* gs_body_raw_ptr = gs_body.c_str();

		*out_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

		gl.attachShader(*out_po_id, *out_gs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		gl.shaderSource(*out_gs_id, 1 /* count */, &gs_body_raw_ptr, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	if (out_fs_id != DE_NULL)
	{
		const char* fs_body_raw_ptr = fs_body.c_str();

		*out_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

		gl.attachShader(*out_po_id, *out_fs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		gl.shaderSource(*out_fs_id, 1 /* count */, &fs_body_raw_ptr, DE_NULL /* length */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");
	}

	/* Compile all shaders */
	const glw::GLuint so_ids[] = { (out_vs_id != DE_NULL) ? *out_vs_id : 0, (out_tc_id != DE_NULL) ? *out_tc_id : 0,
								   (out_te_id != DE_NULL) ? *out_te_id : 0, (out_gs_id != DE_NULL) ? *out_gs_id : 0,
								   (out_fs_id != DE_NULL) ? *out_fs_id : 0 };
	const unsigned int n_so_ids = sizeof(so_ids) / sizeof(so_ids[0]);

	for (unsigned int n_so_id = 0; n_so_id < n_so_ids; ++n_so_id)
	{
		glw::GLuint so_id = so_ids[n_so_id];

		if (so_id != 0)
		{
			glw::GLint compile_status = GL_FALSE;

			gl.compileShader(so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

			gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			if (compile_status != GL_TRUE)
			{
				goto end;
			}
		} /* if (so_id != 0) */
	}	 /* for (all shader objects) */

	/* Set up XFB */
	if (xfb_varyings != NULL)
	{
		gl.transformFeedbackVaryings(*out_po_id, n_xfb_varyings, xfb_varyings, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");
	}

	gl.linkProgram(*out_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(*out_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		goto end;
	}

	/* All done */
	result = true;

end:
	return result;
}

/** Retrieves base variable type for user-specified variable type
 *  (eg. float for vec4)
 *
 *  @param variable_type Variable type to use for the query.
 *
 *  @return As per description.
 **/
Utils::_variable_type Utils::getBaseVariableType(const _variable_type& variable_type)
{
	_variable_type result = VARIABLE_TYPE_UNKNOWN;

	switch (variable_type)
	{
	case VARIABLE_TYPE_BOOL:
	case VARIABLE_TYPE_BVEC2:
	case VARIABLE_TYPE_BVEC3:
	case VARIABLE_TYPE_BVEC4:
	{
		result = VARIABLE_TYPE_BOOL;

		break;
	}

	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_DVEC4:
	{
		result = VARIABLE_TYPE_DOUBLE;

		break;
	}

	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_MAT2:
	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT2X4:
	case VARIABLE_TYPE_MAT3:
	case VARIABLE_TYPE_MAT3X2:
	case VARIABLE_TYPE_MAT3X4:
	case VARIABLE_TYPE_MAT4:
	case VARIABLE_TYPE_MAT4X2:
	case VARIABLE_TYPE_MAT4X3:
	case VARIABLE_TYPE_VEC2:
	case VARIABLE_TYPE_VEC3:
	case VARIABLE_TYPE_VEC4:
	{
		result = VARIABLE_TYPE_FLOAT;

		break;
	}

	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_IVEC4:
	{
		result = VARIABLE_TYPE_INT;

		break;
	}

	case VARIABLE_TYPE_UINT:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_UVEC4:
	{
		result = VARIABLE_TYPE_UINT;

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	} /* switch (variable_type) */

	return result;
}

/** Retrieves size of a single component (in bytes) for user-specified
 *  variable type.
 *
 *  @param variable_type Variable type to use for the query.
 *
 *  @return As per description.
 **/
unsigned int Utils::getComponentSizeForVariableType(const _variable_type& variable_type)
{
	_variable_type base_variable_type = getBaseVariableType(variable_type);
	unsigned int   result			  = 0;

	switch (base_variable_type)
	{
	case VARIABLE_TYPE_BOOL:
		result = sizeof(bool);
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = sizeof(double);
		break;
	case VARIABLE_TYPE_FLOAT:
		result = sizeof(float);
		break;
	case VARIABLE_TYPE_INT:
		result = sizeof(int);
		break;
	case VARIABLE_TYPE_UINT:
		result = sizeof(unsigned int);
		break;

	default:
	{
		TCU_FAIL("Unrecognized base variable type");
	}
	} /* switch (variable_type) */

	return result;
}

/** Retrieves a GLenum value corresponding to internal shader stage
 *  representation.
 *
 *  @param shader_stage Shader stage to user for the query.
 *
 *  @return Requested value or GL_NONE if the stage was not recognized.
 **/
glw::GLenum Utils::getGLenumForShaderStage(const _shader_stage& shader_stage)
{
	glw::GLenum result = GL_NONE;

	switch (shader_stage)
	{
	case SHADER_STAGE_VERTEX:
		result = GL_VERTEX_SHADER;
		break;
	case SHADER_STAGE_TESSELLATION_CONTROL:
		result = GL_TESS_CONTROL_SHADER;
		break;
	case SHADER_STAGE_TESSELLATION_EVALUATION:
		result = GL_TESS_EVALUATION_SHADER;
		break;
	case SHADER_STAGE_GEOMETRY:
		result = GL_GEOMETRY_SHADER;
		break;
	case SHADER_STAGE_FRAGMENT:
		result = GL_FRAGMENT_SHADER;
		break;

	default:
	{
		TCU_FAIL("Unrecognized shader stage requested");
	}
	} /* switch (shader_stage) */

	return result;
}

/** Retrieves number of components that user-specified variable type supports.
 *
 *  @param variable_type GLSL variable type to use for the query.
 *
 *  @return As per description.
 **/
unsigned int Utils::getNumberOfComponentsForVariableType(const _variable_type& variable_type)
{
	unsigned int result = 0;

	switch (variable_type)
	{
	case VARIABLE_TYPE_BOOL:
	case VARIABLE_TYPE_DOUBLE:
	case VARIABLE_TYPE_FLOAT:
	case VARIABLE_TYPE_INT:
	case VARIABLE_TYPE_UINT:
	{
		result = 1;

		break;
	}

	case VARIABLE_TYPE_BVEC2:
	case VARIABLE_TYPE_DVEC2:
	case VARIABLE_TYPE_IVEC2:
	case VARIABLE_TYPE_UVEC2:
	case VARIABLE_TYPE_VEC2:
	{
		result = 2;

		break;
	}

	case VARIABLE_TYPE_BVEC3:
	case VARIABLE_TYPE_DVEC3:
	case VARIABLE_TYPE_IVEC3:
	case VARIABLE_TYPE_UVEC3:
	case VARIABLE_TYPE_VEC3:
	{
		result = 3;

		break;
	}

	case VARIABLE_TYPE_BVEC4:
	case VARIABLE_TYPE_DVEC4:
	case VARIABLE_TYPE_IVEC4:
	case VARIABLE_TYPE_MAT2:
	case VARIABLE_TYPE_UVEC4:
	case VARIABLE_TYPE_VEC4:
	{
		result = 4;

		break;
	}

	case VARIABLE_TYPE_MAT2X3:
	case VARIABLE_TYPE_MAT3X2:
	{
		result = 6;

		break;
	}

	case VARIABLE_TYPE_MAT2X4:
	case VARIABLE_TYPE_MAT4X2:
	{
		result = 8;

		break;
	}

	case VARIABLE_TYPE_MAT3:
	{
		result = 9;

		break;
	}

	case VARIABLE_TYPE_MAT3X4:
	case VARIABLE_TYPE_MAT4X3:
	{
		result = 12;

		break;
	}

	case VARIABLE_TYPE_MAT4:
	{
		result = 16;

		break;
	}

	default:
		break;
	} /* switch (variable_type) */

	return result;
}

/** Retrieves a literal defining user-specified shader stage enum.
 *
 *  @param shader_stage Shader stage to use for the query.
 *
 *  @return Requested string or "?" if the stage was not recognized.
 **/
std::string Utils::getShaderStageString(const _shader_stage& shader_stage)
{
	std::string result = "?";

	switch (shader_stage)
	{
	case SHADER_STAGE_FRAGMENT:
		result = "Fragment Shader";
		break;
	case SHADER_STAGE_GEOMETRY:
		result = "Geometry Shader";
		break;
	case SHADER_STAGE_TESSELLATION_CONTROL:
		result = "Tessellation Control Shader";
		break;
	case SHADER_STAGE_TESSELLATION_EVALUATION:
		result = "Tessellation Evaluation Shader";
		break;
	case SHADER_STAGE_VERTEX:
		result = "Vertex Shader";
		break;

	default:
	{
		TCU_FAIL("Unrecognized shader stage");
	}
	} /* switch (shader_stage) */

	return result;
}

/** Retrieves a literal defining user-specified shader stage enum.
 *
 *  @param shader_stage_glenum Shader stage to use for the query.
 *
 *  @return Requested string or "?" if the stage was not recognized.
 **/
std::string Utils::getShaderStageStringFromGLEnum(const glw::GLenum shader_stage_glenum)
{
	std::string result = "?";

	switch (shader_stage_glenum)
	{
	case GL_FRAGMENT_SHADER:
		result = "Fragment Shader";
		break;
	case GL_GEOMETRY_SHADER:
		result = "Geometry Shader";
		break;
	case GL_TESS_CONTROL_SHADER:
		result = "Tessellation Control Shader";
		break;
	case GL_TESS_EVALUATION_SHADER:
		result = "Tessellation Evaluation Shader";
		break;
	case GL_VERTEX_SHADER:
		result = "Vertex Shader";
		break;

	default:
	{
		TCU_FAIL("Unrecognized shader string");
	}
	} /* switch (shader_stage_glenum) */

	return result;
}

/** Returns string that represents program interface name
 *
 * @param program_interface Program interface
 *
 * @return String representation of known program interface
 **/
const GLchar* Utils::programInterfaceToStr(glw::GLenum program_interface)
{
	const GLchar* string = "Unknown program interface";

	switch (program_interface)
	{
	case GL_VERTEX_SUBROUTINE:
		string = "GL_VERTEX_SUBROUTINE";
		break;
	case GL_VERTEX_SUBROUTINE_UNIFORM:
		string = "GL_VERTEX_SUBROUTINE_UNIFORM";
		break;
	default:
		TCU_FAIL("Not implemented");
		break;
	};

	return string;
}

/** Returns string that represents pname's name
 *
 * @param pname pname
 *
 * @return String representation of known pnames
 **/
const GLchar* Utils::pnameToStr(glw::GLenum pname)
{
	const GLchar* string = "Unknown pname";

	switch (pname)
	{
	case GL_ACTIVE_SUBROUTINE_UNIFORMS:
		string = "GL_ACTIVE_SUBROUTINE_UNIFORMS";
		break;
	case GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS:
		string = "GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS";
		break;
	case GL_ACTIVE_SUBROUTINES:
		string = "GL_ACTIVE_SUBROUTINES";
		break;
	case GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH:
		string = "GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH";
		break;
	case GL_ACTIVE_SUBROUTINE_MAX_LENGTH:
		string = "GL_ACTIVE_SUBROUTINE_MAX_LENGTH";
		break;
	case GL_NUM_COMPATIBLE_SUBROUTINES:
		string = "GL_NUM_COMPATIBLE_SUBROUTINES";
		break;
	case GL_UNIFORM_SIZE:
		string = "GL_UNIFORM_SIZE";
		break;
	case GL_COMPATIBLE_SUBROUTINES:
		string = "GL_COMPATIBLE_SUBROUTINES";
		break;
	case GL_UNIFORM_NAME_LENGTH:
		string = "GL_UNIFORM_NAME_LENGTH";
		break;
	case GL_ACTIVE_RESOURCES:
		string = "GL_ACTIVE_RESOURCES";
		break;
	case GL_MAX_NAME_LENGTH:
		string = "GL_MAX_NAME_LENGTH";
		break;
	case GL_MAX_NUM_COMPATIBLE_SUBROUTINES:
		string = "GL_MAX_NUM_COMPATIBLE_SUBROUTINES";
		break;
	case GL_NAME_LENGTH:
		string = "GL_NAME_LENGTH";
		break;
	case GL_ARRAY_SIZE:
		string = "GL_ARRAY_SIZE";
		break;
	case GL_LOCATION:
		string = "GL_LOCATION";
		break;
	default:
		TCU_FAIL("Not implemented");
		break;
	};

	return string;
}

bool Utils::compare(const glw::GLfloat& left, const glw::GLfloat& right)
{
	static const glw::GLfloat m_epsilon = 0.00001f;

	if (m_epsilon < std::abs(right - left))
	{
		return false;
	}
	else
	{
		return true;
	}
}

/** Returns a variable type enum corresponding to user-specified base variable type
 *  and the number of components it should support.
 *
 *  @param base_variable_type Base variable type to use for the query.
 *  @param n_components       Number of components to consider for the query.
 *
 *  @return As per description.
 **/
Utils::_variable_type Utils::getVariableTypeFromProperties(const _variable_type& base_variable_type,
														   const unsigned int&   n_components)
{
	_variable_type result = VARIABLE_TYPE_UNKNOWN;

	switch (base_variable_type)
	{
	case VARIABLE_TYPE_BOOL:
	{
		switch (n_components)
		{
		case 1:
			result = VARIABLE_TYPE_BOOL;
			break;
		case 2:
			result = VARIABLE_TYPE_BVEC2;
			break;
		case 3:
			result = VARIABLE_TYPE_BVEC3;
			break;
		case 4:
			result = VARIABLE_TYPE_BVEC4;
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components requested");
		}
		} /* switch (n_components) */

		break;
	}

	case VARIABLE_TYPE_DOUBLE:
	{
		switch (n_components)
		{
		case 1:
			result = VARIABLE_TYPE_DOUBLE;
			break;
		case 2:
			result = VARIABLE_TYPE_DVEC2;
			break;
		case 3:
			result = VARIABLE_TYPE_DVEC3;
			break;
		case 4:
			result = VARIABLE_TYPE_DVEC4;
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components requested");
		}
		} /* switch (n_components) */

		break;
	}

	case VARIABLE_TYPE_FLOAT:
	{
		switch (n_components)
		{
		case 1:
			result = VARIABLE_TYPE_FLOAT;
			break;
		case 2:
			result = VARIABLE_TYPE_VEC2;
			break;
		case 3:
			result = VARIABLE_TYPE_VEC3;
			break;
		case 4:
			result = VARIABLE_TYPE_VEC4;
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components requested");
		}
		} /* switch (n_components) */

		break;
	}

	case VARIABLE_TYPE_INT:
	{
		switch (n_components)
		{
		case 1:
			result = VARIABLE_TYPE_INT;
			break;
		case 2:
			result = VARIABLE_TYPE_IVEC2;
			break;
		case 3:
			result = VARIABLE_TYPE_IVEC3;
			break;
		case 4:
			result = VARIABLE_TYPE_IVEC4;
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components requested");
		}
		} /* switch (n_components) */

		break;
	}

	case VARIABLE_TYPE_UINT:
	{
		switch (n_components)
		{
		case 1:
			result = VARIABLE_TYPE_UINT;
			break;
		case 2:
			result = VARIABLE_TYPE_UVEC2;
			break;
		case 3:
			result = VARIABLE_TYPE_UVEC3;
			break;
		case 4:
			result = VARIABLE_TYPE_UVEC4;
			break;

		default:
		{
			TCU_FAIL("Unsupported number of components requested");
		}
		} /* switch (n_components) */

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized base variable type");
	}
	} /* switch (base_variable_type) */

	return result;
}

/** Returns a GLSL literal corresponding to user-specified variable type.
 *
 *  @param variable_type Variable type to use for the query.
 *
 *  @return As per description or [?] if @param variable_type was not
 *          recognized.
 **/
std::string Utils::getVariableTypeGLSLString(const _variable_type& variable_type)
{
	std::string result = "[?]";

	switch (variable_type)
	{
	case VARIABLE_TYPE_BOOL:
		result = "bool";
		break;
	case VARIABLE_TYPE_BVEC2:
		result = "bvec2";
		break;
	case VARIABLE_TYPE_BVEC3:
		result = "bvec3";
		break;
	case VARIABLE_TYPE_BVEC4:
		result = "bvec4";
		break;
	case VARIABLE_TYPE_DOUBLE:
		result = "double";
		break;
	case VARIABLE_TYPE_DVEC2:
		result = "dvec2";
		break;
	case VARIABLE_TYPE_DVEC3:
		result = "dvec3";
		break;
	case VARIABLE_TYPE_DVEC4:
		result = "dvec4";
		break;
	case VARIABLE_TYPE_FLOAT:
		result = "float";
		break;
	case VARIABLE_TYPE_INT:
		result = "int";
		break;
	case VARIABLE_TYPE_IVEC2:
		result = "ivec2";
		break;
	case VARIABLE_TYPE_IVEC3:
		result = "ivec3";
		break;
	case VARIABLE_TYPE_IVEC4:
		result = "ivec4";
		break;
	case VARIABLE_TYPE_MAT2:
		result = "mat2";
		break;
	case VARIABLE_TYPE_MAT2X3:
		result = "mat2x3";
		break;
	case VARIABLE_TYPE_MAT2X4:
		result = "mat2x4";
		break;
	case VARIABLE_TYPE_MAT3:
		result = "mat3";
		break;
	case VARIABLE_TYPE_MAT3X2:
		result = "mat3x2";
		break;
	case VARIABLE_TYPE_MAT3X4:
		result = "mat3x4";
		break;
	case VARIABLE_TYPE_MAT4:
		result = "mat4";
		break;
	case VARIABLE_TYPE_MAT4X2:
		result = "mat4x2";
		break;
	case VARIABLE_TYPE_MAT4X3:
		result = "mat4x3";
		break;
	case VARIABLE_TYPE_UINT:
		result = "uint";
		break;
	case VARIABLE_TYPE_UVEC2:
		result = "uvec2";
		break;
	case VARIABLE_TYPE_UVEC3:
		result = "uvec3";
		break;
	case VARIABLE_TYPE_UVEC4:
		result = "uvec4";
		break;
	case VARIABLE_TYPE_VEC2:
		result = "vec2";
		break;
	case VARIABLE_TYPE_VEC3:
		result = "vec3";
		break;
	case VARIABLE_TYPE_VEC4:
		result = "vec4";
		break;

	default:
	{
		TCU_FAIL("Unrecognized variable type");
	}
	} /* switch (variable_type) */

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
APITest1::APITest1(deqp::Context& context)
	: TestCase(context, "min_maxes", "Verifies the implementation returns valid GL_MAX_SUBROUTINE* pnames "
									 "which meet the minimum maximum requirements enforced by the spec.")
	, m_has_test_passed(true)
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult APITest1::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all pnames */
	const struct
	{
		glw::GLenum pname;
		const char* pname_string;
		glw::GLint  min_value;
	} pnames[] = { { GL_MAX_SUBROUTINES, "GL_MAX_SUBROUTINES", 256 },
				   { GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, "GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS", 1024 } };
	const unsigned int n_pnames = sizeof(pnames) / sizeof(pnames[0]);

	for (unsigned int n_pname = 0; n_pname < n_pnames; ++n_pname)
	{
		glw::GLboolean	 bool_value   = GL_FALSE;
		glw::GLdouble	  double_value = 0.0;
		glw::GLfloat	   float_value  = 0.0f;
		glw::GLint		   int_value	= 0;
		glw::GLint64	   int64_value  = 0;
		const glw::GLint   min_value	= pnames[n_pname].min_value;
		const glw::GLenum& pname		= pnames[n_pname].pname;
		const char*		   pname_string = pnames[n_pname].pname_string;

		/* Retrieve the pname values */
		gl.getBooleanv(pname, &bool_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() call failed.");

		gl.getDoublev(pname, &double_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetDoublev() call failed.");

		gl.getFloatv(pname, &float_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() call failed.");

		gl.getIntegerv(pname, &int_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed.");

		gl.getInteger64v(pname, &int64_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInteger64v() call failed.");

		/* Make sure the value reported meets the min max requirement */
		if (int_value < min_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "GL implementation reports a value of [" << int_value
							   << "]"
								  " for property ["
							   << pname_string << "]"
												  ", whereas the min max for the property is ["
							   << min_value << "]." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		/* Verify the other getters reported valid values */
		const float epsilon = 1e-5f;

		if (((int_value == 0) && (bool_value == GL_TRUE)) || ((int_value != 0) && (bool_value != GL_TRUE)))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid boolean value [" << bool_value
							   << "]"
								  " reported for property ["
							   << pname_string << "]"
												  " (int value:["
							   << int_value << "])" << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		if (de::abs(double_value - (double)int_value) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid double value [" << double_value
							   << "]"
								  " reported for property ["
							   << pname_string << "]"
												  " (int value:["
							   << int_value << "])" << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		if (de::abs(float_value - (float)int_value) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid float value [" << float_value
							   << "]"
								  " reported for property ["
							   << pname_string << "]"
												  " (int value:["
							   << int_value << "])" << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		if (int64_value != int_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid 64-bit integer value [" << float_value
							   << "]"
								  " reported for property ["
							   << pname_string << "]"
												  " (int value:["
							   << int_value << "])" << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}
	} /* for (all pnames) */

	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
APITest2::APITest2(deqp::Context& context)
	: TestCase(context, "name_getters", "Verifies glGetActiveSubroutineName() and glGetActiveSubroutineUniformName() "
										"functions work correctly.")
	, m_buffer(DE_NULL)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_subroutine_name1("subroutine1")
	, m_subroutine_name2("subroutine2")
	, m_subroutine_uniform_name("data_provider")
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Destroys all ES objects that may have been created during test initialization,
 *  as well as releases any buffers that may have been allocated during the process.
 */
void APITest2::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_buffer != DE_NULL)
	{
		delete[] m_buffer;

		m_buffer = DE_NULL;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns body of a vertex shader that should be used for the test.
 *
 *  @return As per description.
 **/
std::string APITest2::getVertexShaderBody()
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "subroutine int ExampleSubroutineType(int example_argument);\n"
		   "\n"
		   "subroutine(ExampleSubroutineType) int subroutine1(int example_argument)\n"
		   "{\n"
		   "    return 1;\n"
		   "}\n"
		   "\n"
		   "subroutine(ExampleSubroutineType) int subroutine2(int example_argument)\n"
		   "{\n"
		   "    return 2;\n"
		   "}\n"
		   "\n"
		   "subroutine uniform ExampleSubroutineType data_provider;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = vec4(float(data_provider(0)), vec3(1) );\n"
		   "}\n";
}

/** Initializes all ES objects required to run the test. */
void APITest2::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate program & shader objects */
	m_po_id = gl.createProgram();
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() or glCreateShader() call(s) failed.");

	/* Attach the shader to the program object */
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	/* Compile the shader */
	glw::GLint  compile_status  = GL_FALSE;
	std::string vs_body			= getVertexShaderBody();
	const char* vs_body_raw_ptr = vs_body.c_str();

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		TCU_FAIL("Shader compilation failed.");
	}

	/* Try to link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed.");
	}

	/* Perform a few sanity checks */
	glw::GLint n_active_subroutines			= 0;
	glw::GLint n_active_subroutine_uniforms = 0;

	gl.getProgramStageiv(m_po_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &n_active_subroutines);
	gl.getProgramStageiv(m_po_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &n_active_subroutine_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramStageiv() call failed.");

	if (n_active_subroutines != 2 /* subroutines declared in vertex shader */)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid amount of active subroutines reported; expected: 2,"
													   " reported:"
						   << n_active_subroutines << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid GL_ACTIVE_SUBROUTINES property value.");
	}

	if (n_active_subroutine_uniforms != 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid amount of active subroutine uniforms reported: expected: 1,"
							  " reported: "
						   << n_active_subroutine_uniforms << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid GL_ACTIVE_SUBROUTINE_UNIFORMS property value.");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult APITest2::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Initialize a test program object */
	initTest();

	/* Verify glGetActiveSubroutineName() works correctly */
	verifyGLGetActiveSubroutineNameFunctionality();

	/* Verify glGetActiveSubroutineUniformName() works correctly */
	verifyGLGetActiveSubroutineUniformNameFunctionality();

	/* Done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies glGetActiveSubroutineName() behaves as per GL_ARB_shader_subroutine
 *  specification.
 **/
void APITest2::verifyGLGetActiveSubroutineNameFunctionality()
{
	GLsizei				  expected_length1 = (GLsizei)strlen(m_subroutine_name1) + 1;
	GLsizei				  expected_length2 = (GLsizei)strlen(m_subroutine_name1) + 1;
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	GLsizei				  reported_length  = 0;

	gl.getActiveSubroutineName(m_po_id, GL_VERTEX_SHADER, 0, /* index */
							   0,							 /* bufsize */
							   DE_NULL,						 /* length */
							   DE_NULL);					 /* name */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveSubroutineName() call failed.");

	gl.getProgramInterfaceiv(m_po_id, GL_VERTEX_SUBROUTINE, GL_MAX_NAME_LENGTH, &reported_length);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveSubroutineName() call failed.");

	if ((reported_length != expected_length1) && (reported_length != expected_length2))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid active subroutine name length reported:" << reported_length
						   << ", instead of: " << expected_length1 << " or " << expected_length2
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Incorrect length of active subroutine name");
	}

	m_buffer = new glw::GLchar[reported_length];

	memset(m_buffer, 0, reported_length);

	gl.getActiveSubroutineName(m_po_id, GL_VERTEX_SHADER, 0, reported_length, DE_NULL, /* length */
							   m_buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveSubroutineName() call failed.");

	if (strcmp(m_buffer, m_subroutine_name1) != 0 && strcmp(m_buffer, m_subroutine_name2) != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid active subroutine name reported:[" << m_buffer
						   << "]"
							  " instead of:["
						   << m_subroutine_name1 << "]"
													" or:["
						   << m_subroutine_name2 << "]." << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid active subroutine name reported.");
	}

	delete[] m_buffer;
	m_buffer = DE_NULL;
}

/** Verifies glGetActiveSubroutineUniformName() behaves as per GL_ARB_shader_subroutine
 *  specification.
 **/
void APITest2::verifyGLGetActiveSubroutineUniformNameFunctionality()
{
	GLsizei				  expected_length = (GLsizei)strlen(m_subroutine_uniform_name);
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	GLsizei				  reported_length = 0;

	gl.getActiveSubroutineUniformName(m_po_id, GL_VERTEX_SHADER, 0, /* index */
									  0,							/* bufsize */
									  DE_NULL,						/* length */
									  DE_NULL);						/* name */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveSubroutineUniformName() call failed.");

	gl.getActiveSubroutineUniformName(m_po_id, GL_VERTEX_SHADER, 0, /* index */
									  0,							/* bufsize */
									  &reported_length, DE_NULL);   /* name */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetActiveSubroutineUniformName() call failed.");

	// reported_length is the actual number of characters written into <name>
	// If <bufSize> is 0, reported_length should be 0
	if (reported_length != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid active subroutine uniform name length reported:" << reported_length
						   << ", instead of: " << 0 << tcu::TestLog::EndMessage;

		TCU_FAIL("Incorrect length of active subroutine uniform name");
	}

	m_buffer = new glw::GLchar[expected_length + 1];

	memset(m_buffer, 0, expected_length + 1);

	gl.getActiveSubroutineUniformName(m_po_id, GL_VERTEX_SHADER, 0, expected_length + 1, &reported_length, m_buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getActiveSubroutineUniformName() call failed.");

	if (reported_length != expected_length)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid active subroutine uniform name length reported:" << reported_length
						   << ", instead of: " << expected_length << tcu::TestLog::EndMessage;

		TCU_FAIL("Incorrect length of active subroutine uniform name");
	}

	if (strcmp(m_buffer, m_subroutine_uniform_name) != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid active subroutine uniform name reported:[" << m_buffer
						   << "]"
							  " instead of:["
						   << m_subroutine_uniform_name << "]" << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid active subroutine uniform name reported.");
	}

	delete[] m_buffer;
	m_buffer = DE_NULL;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest1_2::FunctionalTest1_2(deqp::Context& context)
	: TestCase(context, "two_subroutines_single_subroutine_uniform",
			   "Verifies the subroutines work correctly in a vertex shader for"
			   " bool/float/int/uint/double/*vec*/*mat* argument and return types")
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_po_getter0_subroutine_index(GL_INVALID_INDEX)
	, m_po_getter1_subroutine_index(GL_INVALID_INDEX)
	, m_po_subroutine_uniform_index(-1)
	, m_xfb_bo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Destroys all ES objects that may have been created during test initialization,
 *  as well as releases any buffers that may have been allocated during the process.
 */
void FunctionalTest1_2::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitTestIteration();

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Deinitializes GL objects that are iteration-specific */
void FunctionalTest1_2::deinitTestIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration using user-specified test case propertiesz.
 *
 *  @param test-case Test case descriptor.
 *
 *  @return true if the test iteration passed, false otherwise.
 **/
bool FunctionalTest1_2::executeTestIteration(const _test_case& test_case)
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Build the test program */
	std::string		   empty_body;
	std::string		   vs_body		  = getVertexShaderBody(test_case.variable_type, test_case.array_size);
	const glw::GLchar* xfb_varyings[] = { "result" };
	const unsigned int n_xfb_varyings = sizeof(xfb_varyings) / sizeof(xfb_varyings[0]);

	if (!Utils::buildProgram(gl, vs_body, empty_body, empty_body, empty_body, empty_body, xfb_varyings, n_xfb_varyings,
							 &m_vs_id, NULL, /* out_tc_id */
							 NULL,			 /* out_te_id */
							 NULL,			 /* out_gs_id */
							 NULL, &m_po_id))
	{
		TCU_FAIL("Test program failed to build.");
	}

	/* Retrieve subroutine locations */
	m_po_getter0_subroutine_index = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "getter0");
	m_po_getter1_subroutine_index = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "getter1");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() call(s) failed.");

	if (m_po_getter0_subroutine_index == GL_INVALID_INDEX || m_po_getter1_subroutine_index == GL_INVALID_INDEX)
	{
		TCU_FAIL("At least one subroutine is considered inactive which is invalid.");
	}

	/* Retrieve subroutine uniform location */
	m_po_subroutine_uniform_index = gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "colorGetterUniform");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() call failed.");

	if (m_po_subroutine_uniform_index == -1)
	{
		TCU_FAIL("Subroutine uniform is considered inactive which is invalid.");
	}

	/* Set up XFB BO storage */
	const Utils::_variable_type base_variable_type	= Utils::getBaseVariableType(test_case.variable_type);
	unsigned int				iteration_xfb_bo_size = Utils::getComponentSizeForVariableType(base_variable_type) *
										 Utils::getNumberOfComponentsForVariableType(test_case.variable_type);
	unsigned int total_xfb_bo_size = 0;

	if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
	{
		/* Boolean varyings are not supported by OpenGL. Instead, we use ints to output
		 * boolean values. */
		iteration_xfb_bo_size = static_cast<unsigned int>(iteration_xfb_bo_size * sizeof(int));
	}

	total_xfb_bo_size = iteration_xfb_bo_size * 2 /* subroutines we will be testing */;

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, total_xfb_bo_size, DE_NULL /* data */, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Activate test program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Run two iterations. Each iteration should invoke different subroutine. */
	const glw::GLuint  subroutine_indices[] = { m_po_getter0_subroutine_index, m_po_getter1_subroutine_index };
	const unsigned int n_subroutine_indices = sizeof(subroutine_indices) / sizeof(subroutine_indices[0]);

	for (unsigned int n_subroutine_index = 0; n_subroutine_index < n_subroutine_indices; ++n_subroutine_index)
	{
		/* Configure which subroutine should be used for the draw call */
		glw::GLuint current_subroutine_index = subroutine_indices[n_subroutine_index];

		gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, 1 /* count */, &current_subroutine_index);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() call failed.");

		/* Update XFB binding so that we do not overwrite data XFBed in previous iterations */
		gl.bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
						   m_xfb_bo_id, iteration_xfb_bo_size * n_subroutine_index, iteration_xfb_bo_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() call failed.");

		/* Draw a single point */
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
		{
			gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");
	} /* for (all subroutine indices) */

	/* Map the BO storage into process space */
	const void* xfb_data_ptr = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	result &= verifyXFBData(xfb_data_ptr, test_case.variable_type);

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffeR() call failed.");

	return result;
}

/** Retrieves body of a vertex shader that should be used to verify
 *  subroutine support, given user-specified test iteration properties.
 *
 *  @param variable_type GLSL type that should be used for argument and
 *                       return type definition in a subroutine. This setting
 *                       also affects type of the only output variable in the shader.
 *  @param array_size    1 if non-arrayed arguments/return types should be tested;
 *                       2 if arrayed arguments/return types should be tested.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest1_2::getVertexShaderBody(const Utils::_variable_type& variable_type, unsigned int array_size)
{
	Utils::_variable_type base_variable_type		 = Utils::getBaseVariableType(variable_type);
	unsigned int		  n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
	std::stringstream	 result_sstream;
	std::string			  variable_type_glsl = Utils::getVariableTypeGLSLString(variable_type);
	std::stringstream	 variable_type_glsl_array_sstream;
	std::stringstream	 variable_type_glsl_arrayed_sstream;

	variable_type_glsl_arrayed_sstream << variable_type_glsl;

	if (array_size > 1)
	{
		variable_type_glsl_array_sstream << "[" << array_size << "]";
		variable_type_glsl_arrayed_sstream << variable_type_glsl_array_sstream.str();
	}

	/* Form pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n";

	if (variable_type == Utils::VARIABLE_TYPE_DOUBLE)
	{
		result_sstream << "#extension GL_ARB_gpu_shader_fp64 : require\n";
	}

	/* Form subroutine type declaration */
	result_sstream << "\n"
					  "subroutine "
				   << variable_type_glsl_arrayed_sstream.str() << " colorGetter(in " << variable_type_glsl
				   << " in_value" << variable_type_glsl_array_sstream.str() << ");\n"
																			   "\n";

	/* Declare getter functions */
	for (int n_getter = 0; n_getter < 2; ++n_getter)
	{
		result_sstream << "subroutine(colorGetter) " << variable_type_glsl_arrayed_sstream.str() << " getter"
					   << n_getter << "(in " << variable_type_glsl << " in_value"
					   << variable_type_glsl_array_sstream.str() << ")\n"
																	"{\n";

		if (array_size > 1)
		{
			result_sstream << variable_type_glsl << " temp" << variable_type_glsl_array_sstream.str() << ";\n";
		}

		if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
		{
			if (array_size > 1)
			{
				for (unsigned int array_index = 0; array_index < array_size; ++array_index)
				{
					result_sstream << "    temp[" << array_index << "]"
																	" = "
								   << ((n_getter == 0) ? ((variable_type_glsl == "bool") ? "!" : "not") : "")
								   << "(in_value[" << array_index << "]);\n";
				}

				result_sstream << "    return temp;\n";
			}
			else
			{
				result_sstream << "    return "
							   << ((n_getter == 0) ? ((variable_type_glsl == "bool") ? "!" : "not") : "")
							   << "(in_value);\n";
			}
		} /* if (base_variable_type == Utils::VARIABLE_TYPE_BOOL) */
		else
		{
			if (array_size > 1)
			{
				for (unsigned int array_index = 0; array_index < array_size; ++array_index)
				{
					result_sstream << "    temp[" << array_index << "]"
																	" = in_value["
								   << array_index << "] + " << (n_getter + 1) << ";\n";
				}

				result_sstream << "    return temp;\n";
			}
			else
			{
				result_sstream << "    return (in_value + " << (n_getter + 1) << ");\n";
			}
		}

		result_sstream << "}\n";
	} /* for (both getter functions) */

	/* Declare subroutine uniform */
	result_sstream << "subroutine uniform colorGetter colorGetterUniform;\n"
					  "\n";

	/* Declare output variable */
	result_sstream << "out ";

	if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
	{
		Utils::_variable_type result_as_int_variable_type =
			Utils::getVariableTypeFromProperties(Utils::VARIABLE_TYPE_INT, n_variable_type_components);
		std::string variable_type_glsl_as_int = Utils::getVariableTypeGLSLString(result_as_int_variable_type);

		result_sstream << variable_type_glsl_as_int;
	}
	else
	{
		result_sstream << variable_type_glsl;
	}

	result_sstream << " result;\n"
					  "\n";

	/* Declare main(): prepare input argument for the subroutine function */
	result_sstream << "void main()\n"
					  "{\n"
					  "    "
				   << variable_type_glsl << " temp";

	if (array_size > 1)
	{
		result_sstream << "[" << array_size << "]";
	};

	result_sstream << ";\n";

	for (unsigned int array_index = 0; array_index < array_size; ++array_index)
	{
		result_sstream << "    temp";

		if (array_size > 1)
		{
			result_sstream << "[" << array_index << "]";
		}

		result_sstream << " = " << variable_type_glsl << "(";

		if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
		{
			result_sstream << "true";
		}
		else
		{
			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				result_sstream << "3";

				if (n_component != (n_variable_type_components - 1))
				{
					result_sstream << ", ";
				}
			} /* for (all components) */
		}

		result_sstream << ");\n";
	} /* for (all array indices) */

	/* Declare main(): call the subroutine. Verify the input and write the result
	 *                 to the output variable.
	 **/
	if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
	{
		Utils::_variable_type result_as_int_variable_type =
			Utils::getVariableTypeFromProperties(Utils::VARIABLE_TYPE_INT, n_variable_type_components);
		std::string variable_type_glsl_as_int = Utils::getVariableTypeGLSLString(result_as_int_variable_type);

		result_sstream << variable_type_glsl_arrayed_sstream.str() << " subroutine_result = colorGetterUniform(temp);\n"
																	  "result = ";

		for (unsigned int array_index = 0; array_index < array_size; ++array_index)
		{
			if (variable_type_glsl == "bool")
				result_sstream << "bool(subroutine_result";
			else
				result_sstream << "all(subroutine_result";

			if (array_size > 1)
			{
				result_sstream << "[" << array_index << "]";
			}

			result_sstream << ")";

			if (array_index != (array_size - 1))
			{
				result_sstream << "&& ";
			}
		}

		result_sstream << " == true ? " << variable_type_glsl_as_int << "(1) : " << variable_type_glsl_as_int << "(0);";
	}
	else
	{
		if (array_size > 1)
		{
			DE_ASSERT(array_size == 2);

			result_sstream << variable_type_glsl << " subroutine_result" << variable_type_glsl_array_sstream.str()
						   << " = colorGetterUniform(temp);\n"
							  "\n"
							  "if (subroutine_result[0] == subroutine_result[1]) result = subroutine_result[0];\n"
							  "else\n"
							  "result = "
						   << variable_type_glsl << "(-1);\n";
		}
		else
		{
			result_sstream << "result = colorGetterUniform(temp);\n";
		}
	}

	/* All done */
	result_sstream << "}\n";

	return result_sstream.str();
}

/** Initializes all GL objects required to run the test. */
void FunctionalTest1_2::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate buffer object to hold result XFB data */
	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	/* Set up XFB BO bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Generate VAO to use for the draw calls */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest1_2::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Initialize a test program object */
	initTest();

	/* Construct test case descriptors: first, iIerate over all
	 * variable types we want to cover */
	const Utils::_variable_type variable_types[] = {
		Utils::VARIABLE_TYPE_BOOL,   Utils::VARIABLE_TYPE_BVEC2,  Utils::VARIABLE_TYPE_BVEC3,
		Utils::VARIABLE_TYPE_BVEC4,  Utils::VARIABLE_TYPE_DOUBLE, Utils::VARIABLE_TYPE_FLOAT,
		Utils::VARIABLE_TYPE_INT,	Utils::VARIABLE_TYPE_IVEC2,  Utils::VARIABLE_TYPE_IVEC3,
		Utils::VARIABLE_TYPE_IVEC4,  Utils::VARIABLE_TYPE_MAT2,   Utils::VARIABLE_TYPE_MAT2X3,
		Utils::VARIABLE_TYPE_MAT2X4, Utils::VARIABLE_TYPE_MAT3,   Utils::VARIABLE_TYPE_MAT3X2,
		Utils::VARIABLE_TYPE_MAT3X4, Utils::VARIABLE_TYPE_MAT4,   Utils::VARIABLE_TYPE_MAT4X2,
		Utils::VARIABLE_TYPE_MAT4X3, Utils::VARIABLE_TYPE_UINT,   Utils::VARIABLE_TYPE_UVEC2,
		Utils::VARIABLE_TYPE_UVEC3,  Utils::VARIABLE_TYPE_UVEC4,  Utils::VARIABLE_TYPE_VEC2,
		Utils::VARIABLE_TYPE_VEC3,   Utils::VARIABLE_TYPE_VEC4
	};
	const unsigned int n_variable_types = sizeof(variable_types) / sizeof(variable_types[0]);

	for (unsigned int n_variable_type = 0; n_variable_type < n_variable_types; ++n_variable_type)
	{
		Utils::_variable_type current_variable_type = variable_types[n_variable_type];

		/* We need to test both arrayed and non-arrayed arguments */
		for (unsigned int array_size = 1; array_size < 3; ++array_size)
		{
			/* Exclude double variables if the relevant extension is unavailable */
			if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_fp64") &&
				current_variable_type == Utils::VARIABLE_TYPE_DOUBLE)
			{
				continue;
			}

			/* Form the descriptor */
			_test_case test_case;

			test_case.array_size	= array_size;
			test_case.variable_type = current_variable_type;

			/* Store the test case descriptor */
			m_test_cases.push_back(test_case);
		} /* for (both arrayed and non-arrayed arguments) */
	}	 /* for (all variable types) */

	/* Iterate over all test cases and execute the test */
	for (_test_cases_const_iterator test_case_iterator = m_test_cases.begin(); test_case_iterator != m_test_cases.end();
		 ++test_case_iterator)
	{
		const _test_case& test_case = *test_case_iterator;

		m_has_test_passed &= executeTestIteration(test_case);

		/* Release GL objects that were created during the execution */
		deinitTestIteration();
	} /* for (all test cases) */

	/* Done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies data that has been XFBed out by the vertex shader.
 *
 *  @param xfb_data      Buffer holding the data.
 *  @param variable_type GLSL type used for the test iteration
 *                       that generated the data at @param xfb_data.
 *
 *  @return true if the data was found to be valid, false if it
 *               was detected to be incorrect.
 **/
bool FunctionalTest1_2::verifyXFBData(const void* xfb_data, const Utils::_variable_type& variable_type)
{
	const Utils::_variable_type base_variable_type		   = Utils::getBaseVariableType(variable_type);
	const float					epsilon					   = 1e-5f;
	const unsigned int			n_variable_type_components = Utils::getNumberOfComponentsForVariableType(variable_type);
	bool						result					   = true;
	const unsigned char*		traveller_ptr			   = (const unsigned char*)xfb_data;

	/* Boolean arguments/return types are tested with a slightly different shader so we
	 * need to test them in a separate code-path.
	 */
	if (base_variable_type == Utils::VARIABLE_TYPE_BOOL)
	{
		/* 0 should be returned when getter0 is used, 1 otherwise */
		const unsigned int ref_values[] = { 0, 1 };
		const unsigned int n_ref_values = sizeof(ref_values) / sizeof(ref_values[0]);

		for (unsigned int n_ref_value = 0; n_ref_value < n_ref_values; ++n_ref_value)
		{
			const unsigned int ref_value = ref_values[n_ref_value];

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				int* result_value_ptr = (int*)(traveller_ptr);

				if (*result_value_ptr != (int)ref_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported by subroutine using "
																   "["
									   << Utils::getVariableTypeGLSLString(variable_type) << "]"
									   << " argument/return types ("
										  "expected:["
									   << ref_value << "], found:[" << *result_value_ptr << "])"
									   << tcu::TestLog::EndMessage;

					result = false;
					break;
				}

				traveller_ptr += sizeof(int);
			} /* for (all components) */
		}	 /* for (all reference values) */
	}		  /* if (base_variable_type == Utils::VARIABLE_TYPE_BOOL) */
	else
	{
		/* 4 should be returned when getter0 is used, 5 otherwise */
		const unsigned int ref_values[] = { 4, 5 };
		const unsigned int n_ref_values = sizeof(ref_values) / sizeof(ref_values[0]);

		for (unsigned int n_ref_value = 0; n_ref_value < n_ref_values; ++n_ref_value)
		{
			const unsigned int ref_value = ref_values[n_ref_value];

			DE_ASSERT(
				base_variable_type == Utils::VARIABLE_TYPE_DOUBLE || base_variable_type == Utils::VARIABLE_TYPE_FLOAT ||
				base_variable_type == Utils::VARIABLE_TYPE_INT || base_variable_type == Utils::VARIABLE_TYPE_UINT);

			for (unsigned int n_component = 0; n_component < n_variable_type_components; ++n_component)
			{
				const double* double_value_ptr = (double*)traveller_ptr;
				const float*  float_value_ptr  = (float*)traveller_ptr;
				const int*	int_value_ptr	= (int*)traveller_ptr;

				switch (base_variable_type)
				{
				case Utils::VARIABLE_TYPE_DOUBLE:
				{
					if (de::abs(*double_value_ptr - (double)ref_value) > epsilon)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported by subroutine using "
																	   "["
										   << Utils::getVariableTypeGLSLString(variable_type) << "]"
										   << " argument/return types ("
											  "expected:["
										   << ref_value << "], found:[" << *double_value_ptr << "])"
										   << tcu::TestLog::EndMessage;

						result = false;
					}

					traveller_ptr += sizeof(double);
					break;
				}

				case Utils::VARIABLE_TYPE_FLOAT:
				{
					if (de::abs(*float_value_ptr - (float)ref_value) > epsilon)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported by subroutine using "
																	   "["
										   << Utils::getVariableTypeGLSLString(variable_type) << "]"
										   << " argument/return types ("
											  "expected:["
										   << ref_value << "], found:[" << *float_value_ptr << "])"
										   << tcu::TestLog::EndMessage;

						result = false;
					}

					traveller_ptr += sizeof(float);
					break;
				}

				case Utils::VARIABLE_TYPE_INT:
				case Utils::VARIABLE_TYPE_UINT:
				{
					if (*int_value_ptr != (int)ref_value)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported by subroutine using "
																	   "["
										   << Utils::getVariableTypeGLSLString(variable_type) << "]"
										   << " argument/return types ("
											  "expected:["
										   << ref_value << "], found:[" << *int_value_ptr << "])"
										   << tcu::TestLog::EndMessage;

						result = false;
					}

					traveller_ptr += sizeof(int);
					break;
				}

				default:
					break;
				} /* switch (base_variable_type) */
			}	 /* for (all components) */
		}		  /* for (all reference values) */
	}

	return result;
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest3_4::FunctionalTest3_4(deqp::Context& context)
	: TestCase(context, "four_subroutines_with_two_uniforms", "Verify Get* API and draw calls")
	, m_n_active_subroutine_uniforms(0)
	, m_n_active_subroutine_uniform_locations(0)
	, m_n_active_subroutines(0)
	, m_n_active_subroutine_uniform_name_length(0)
	, m_n_active_subroutine_name_length(0)
	, m_n_active_subroutine_uniform_size(0)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest3_4::iterate()
{
	static const glw::GLchar* vertex_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"// Sub routine type declaration\n"
		"subroutine vec4 routine_type(in vec4 iparam);\n"
		"\n"
		"// Sub routine definitions\n"
		"subroutine(routine_type) vec4 inverse_order(in vec4 iparam)\n"
		"{\n"
		"    return iparam.wzyx;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 negate(in vec4 iparam)\n"
		"{\n"
		"    return -iparam;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 inverse(in vec4 iparam)\n"
		"{\n"
		"    return 1 / iparam;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 square(in vec4 iparam)\n"
		"{\n"
		"    return iparam * iparam;\n"
		"}\n"
		"\n"
		"// Sub routine uniforms\n"
		"subroutine uniform routine_type first_routine;\n"
		"subroutine uniform routine_type second_routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 input_data;\n"
		"\n"
		"// Output\n"
		"out vec4 out_input_data;\n"
		"out vec4 out_result_from_first_routine;\n"
		"out vec4 out_result_from_second_routine;\n"
		"out vec4 out_result_from_combined_routines;\n"
		"out vec4 out_result_from_routines_combined_in_reveresed_order;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_input_data                                       = input_data;\n"
		"    out_result_from_first_routine                        = first_routine(input_data);\n"
		"    out_result_from_second_routine                       = second_routine(input_data);\n"
		"    out_result_from_combined_routines                    = second_routine(first_routine(input_data));\n"
		"    out_result_from_routines_combined_in_reveresed_order = first_routine(second_routine(input_data));\n"
		"}\n"
		"\n";

	static const GLchar* varying_names[] = {
		"out_input_data",
		"out_result_from_first_routine",
		"out_result_from_second_routine",
		"out_result_from_combined_routines",
		"out_result_from_routines_combined_in_reveresed_order",
	};

	static const GLchar* subroutine_uniform_names[] = { "first_routine", "second_routine" };

	static const GLchar* subroutine_names[] = { "inverse_order", "negate", "inverse", "square" };

	static const GLuint n_varyings					   = sizeof(varying_names) / sizeof(varying_names[0]);
	static const GLuint transform_feedback_buffer_size = n_varyings * sizeof(GLfloat) * 4 /* vec4 */;

	static const GLuint inverse_order_routine_index = 0;
	static const GLuint negate_routine_index		= 1;
	static const GLuint inverse_routine_index		= 2;
	static const GLuint square_routine_index		= 3;

	/* Test data */
	static const Utils::vec4<GLfloat> inverse_order_negate_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f), Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f),
		Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f), Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f),
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f),
	};

	static const Utils::vec4<GLfloat> inverse_order_inverse_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f), Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f),
		Utils::vec4<GLfloat>(-0.5f, -1.0f, 1.0f, 0.5f), Utils::vec4<GLfloat>(0.5f, 1.0f, -1.0f, -0.5f),
		Utils::vec4<GLfloat>(0.5f, 1.0f, -1.0f, -0.5f),
	};

	static const Utils::vec4<GLfloat> inverse_order_square_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f), Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f),
		Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),   Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),
		Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),
	};

	static const Utils::vec4<GLfloat> negate_inverse_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f), Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f),
		Utils::vec4<GLfloat>(-0.5f, -1.0f, 1.0f, 0.5f), Utils::vec4<GLfloat>(0.5f, 1.0f, -1.0f, -0.5f),
		Utils::vec4<GLfloat>(0.5f, 1.0f, -1.0f, -0.5f),
	};

	static const Utils::vec4<GLfloat> negate_square_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f),   Utils::vec4<GLfloat>(2.0f, 1.0f, -1.0f, -2.0f),
		Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),	 Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),
		Utils::vec4<GLfloat>(-4.0f, -1.0f, -1.0f, -4.0f),
	};

	static const Utils::vec4<GLfloat> inverse_square_data[5] = {
		Utils::vec4<GLfloat>(-2.0f, -1.0f, 1.0f, 2.0f), Utils::vec4<GLfloat>(-0.5f, -1.0f, 1.0f, 0.5f),
		Utils::vec4<GLfloat>(4.0f, 1.0f, 1.0f, 4.0f),   Utils::vec4<GLfloat>(0.25f, 1.0f, 1.0f, 0.25f),
		Utils::vec4<GLfloat>(0.25f, 1.0f, 1.0f, 0.25f),
	};

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	m_n_active_subroutine_uniforms			  = 2;
	m_n_active_subroutine_uniform_locations   = 2;
	m_n_active_subroutines					  = 4;
	m_n_active_subroutine_uniform_name_length = 0;
	m_n_active_subroutine_name_length		  = 0;
	m_n_active_subroutine_uniform_size		  = 1;

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	bool result = true;

	/* Calculate max name lengths for subroutines and subroutine uniforms */
	for (GLint i = 0; i < m_n_active_subroutine_uniforms; ++i)
	{
		const GLsizei length = (GLsizei)strlen(subroutine_uniform_names[i]);

		if (length > m_n_active_subroutine_uniform_name_length)
		{
			m_n_active_subroutine_uniform_name_length = length;
		}
	}

	for (GLint i = 0; i < m_n_active_subroutines; ++i)
	{
		const GLsizei length = (GLsizei)strlen(subroutine_names[i]);

		if (length > m_n_active_subroutine_name_length)
		{
			m_n_active_subroutine_name_length = length;
		}
	}

	/* Init */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code, varying_names,
				  n_varyings);

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();
	transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
									 GL_DYNAMIC_COPY);
	transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

	program.use();

	/* Inspect Get* API */
	if ((false == inspectProgramStageiv(program.m_program_object_id)) ||
		(false == inspectActiveSubroutineUniformiv(program.m_program_object_id, subroutine_uniform_names)) ||
		(false == inspectActiveSubroutineUniformName(program.m_program_object_id, subroutine_uniform_names)) ||
		(false == inspectActiveSubroutineName(program.m_program_object_id, subroutine_names)) ||
		(false ==
		 inspectSubroutineBinding(program.m_program_object_id, subroutine_names, subroutine_uniform_names, false)))
	{
		result = false;
	}

	/* Inspect GetProgram* API */
	if (true == m_context.getContextInfo().isExtensionSupported("GL_ARB_program_interface_query"))
	{
		if ((false == inspectProgramInterfaceiv(program.m_program_object_id)) ||
			(false ==
			 inspectProgramResourceiv(program.m_program_object_id, subroutine_names, subroutine_uniform_names)) ||
			(false ==
			 inspectSubroutineBinding(program.m_program_object_id, subroutine_names, subroutine_uniform_names, true)))
		{
			result = false;
		}
	}

	/* Test shader execution */
	if ((false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
						   subroutine_names[negate_routine_index], subroutine_uniform_names, inverse_order_negate_data,
						   false)) ||
		(false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
						   subroutine_names[inverse_routine_index], subroutine_uniform_names,
						   inverse_order_inverse_data, false)) ||
		(false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
						   subroutine_names[square_routine_index], subroutine_uniform_names, inverse_order_square_data,
						   false)) ||
		(false == testDraw(program.m_program_object_id, subroutine_names[negate_routine_index],
						   subroutine_names[inverse_routine_index], subroutine_uniform_names, negate_inverse_data,
						   false)) ||
		(false == testDraw(program.m_program_object_id, subroutine_names[negate_routine_index],
						   subroutine_names[square_routine_index], subroutine_uniform_names, negate_square_data,
						   false)) ||
		(false == testDraw(program.m_program_object_id, subroutine_names[inverse_routine_index],
						   subroutine_names[square_routine_index], subroutine_uniform_names, inverse_square_data,
						   false)))
	{
		result = false;
	}

	if (true == m_context.getContextInfo().isExtensionSupported("GL_ARB_program_interface_query"))
	{
		if ((false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
							   subroutine_names[negate_routine_index], subroutine_uniform_names,
							   inverse_order_negate_data, true)) ||
			(false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
							   subroutine_names[inverse_routine_index], subroutine_uniform_names,
							   inverse_order_inverse_data, true)) ||
			(false == testDraw(program.m_program_object_id, subroutine_names[inverse_order_routine_index],
							   subroutine_names[square_routine_index], subroutine_uniform_names,
							   inverse_order_square_data, true)) ||
			(false == testDraw(program.m_program_object_id, subroutine_names[negate_routine_index],
							   subroutine_names[inverse_routine_index], subroutine_uniform_names, negate_inverse_data,
							   true)) ||
			(false == testDraw(program.m_program_object_id, subroutine_names[negate_routine_index],
							   subroutine_names[square_routine_index], subroutine_uniform_names, negate_square_data,
							   true)) ||
			(false == testDraw(program.m_program_object_id, subroutine_names[inverse_routine_index],
							   subroutine_names[square_routine_index], subroutine_uniform_names, inverse_square_data,
							   true)))
		{
			result = false;
		}
	}

	/* Done */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return tcu::TestNode::STOP;
}

/** Verify result of getProgramStageiv
 *
 * @param program_id Program object id
 * @param pname      <pname> parameter for getProgramStageiv
 * @param expected   Expected value
 *
 * @return true if result is equal to expected value, flase otherwise
 **/
bool FunctionalTest3_4::checkProgramStageiv(glw::GLuint program_id, glw::GLenum pname, glw::GLint expected) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLint				  value = 0;

	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	if (expected != value)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Error. Invalid result. Function: getProgramStageiv. "
											<< "pname: " << Utils::pnameToStr(pname) << ". "
											<< "Result: " << value << ". "
											<< "Expected: " << expected << "." << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Verify result of getProgramResourceiv
 *
 * @param program_id        Program object id
 * @param program_interface Program interface
 * @param pname             <pname> parameter for getProgramStageiv
 * @param resource_name     Resource name
 * @param expected          Expected value
 *
 * @return true if result is equal to expected value, false otherwise
 **/
bool FunctionalTest3_4::checkProgramResourceiv(GLuint program_id, GLenum program_interface, GLenum pname,
											   const glw::GLchar* resource_name, GLint expected) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLuint				  index = gl.getProgramResourceIndex(program_id, program_interface, resource_name);
	GLint				  value = 0;

	if (GL_INVALID_INDEX == index)
	{
		return false;
	}

	gl.getProgramResourceiv(program_id, program_interface, index, 1, &pname, 1, 0, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramResourceiv");

	if (expected != value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Error. Invalid result. Function: getProgramResourceiv. "
			<< "Program interface: " << Utils::programInterfaceToStr(program_interface) << ". "
			<< "Resource name: " << resource_name << ". "
			<< "Property: " << Utils::pnameToStr(pname) << ". "
			<< "Result: " << value << ". "
			<< "Expected: " << expected << "." << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Verify result of getProgramInterfaceiv
 *
 * @param program_id        Program object id
 * @param program_interface Program interface
 * @param pname             <pname> parameter for getProgramStageiv
 * @param expected          Expected value
 *
 * @return true if result is equal to expected value, flase otherwise
 **/
bool FunctionalTest3_4::checkProgramInterfaceiv(GLuint program_id, GLenum program_interface, GLenum pname,
												GLint expected) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLint				  value = 0;

	gl.getProgramInterfaceiv(program_id, program_interface, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInterfaceiv");

	if (expected != value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Error. Invalid result. Function: getProgramInterfaceiv. "
			<< "Program interface: " << Utils::programInterfaceToStr(program_interface) << ". "
			<< "pname: " << Utils::pnameToStr(pname) << ". "
			<< "Result: " << value << ". "
			<< "Expected: " << expected << "." << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Verify result of getActiveSubroutineUniformiv
 *
 * @param program_id Program object id
 * @param index      <index> parameter for getActiveSubroutineUniformiv
 * @param pname      <pname> parameter for getActiveSubroutineUniformiv
 * @param expected   Expected value
 *
 * @return true if result is equal to expected value, flase otherwise
 **/
bool FunctionalTest3_4::checkActiveSubroutineUniformiv(GLuint program_id, GLuint index, GLenum pname,
													   GLint expected) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLint				  value = 0;

	gl.getActiveSubroutineUniformiv(program_id, GL_VERTEX_SHADER, index, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getActiveSubroutineUniformiv");

	if (expected != value)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Error. Invalid result. Function: getActiveSubroutineUniformiv. "
											<< "idnex: " << index << ". "
											<< "pname: " << Utils::pnameToStr(pname) << ". "
											<< "Result: " << value << ". "
											<< "Expected: " << expected << "." << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Returns index of program resource
 *
 * @param program_id        Program object id
 * @param program_interface Program interface
 * @param resource_name     Name of resource
 *
 * @return Index of specified resource
 **/
GLuint FunctionalTest3_4::getProgramResourceIndex(GLuint program_id, GLenum program_interface,
												  const glw::GLchar* resource_name) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLuint				  index = gl.getProgramResourceIndex(program_id, program_interface, resource_name);

	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramResourceIndex");

	if (GL_INVALID_INDEX == index)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Program resource is not available. "
											<< "Program interface: " << Utils::programInterfaceToStr(program_interface)
											<< ". "
											<< "Resource name: " << resource_name << "." << tcu::TestLog::EndMessage;
	}

	return index;
}

/** Get subroutine index
 *
 * @param program_id        Program object id
 * @param subroutine_name   Subroutine name
 * @param use_program_query If true getProgramResourceIndex is used, otherwise getSubroutineIndex
 *
 * @return Index of subroutine
 **/
GLuint FunctionalTest3_4::getSubroutineIndex(GLuint program_id, const glw::GLchar* subroutine_name,
											 bool use_program_query) const
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	GLuint				  index = -1;

	if (false == use_program_query)
	{
		index = gl.getSubroutineIndex(program_id, GL_VERTEX_SHADER, subroutine_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineIndex");
	}
	else
	{
		index = gl.getProgramResourceIndex(program_id, GL_VERTEX_SUBROUTINE, subroutine_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramResourceIndex");
	}

	if (GL_INVALID_INDEX == index)
	{
		TCU_FAIL("Subroutine is not available");
	}

	return index;
}

/** Get subroutine uniform location
 *
 * @param program_id        Program object id
 * @param uniform_name      Subroutine uniform name
 * @param use_program_query If true getProgramResourceLocation is used, otherwise getSubroutineUniformLocation
 *
 * @return Location of subroutine uniform
 **/
GLint FunctionalTest3_4::getSubroutineUniformLocation(GLuint program_id, const glw::GLchar* uniform_name,
													  bool use_program_query) const
{
	const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
	GLint				  location = -1;

	if (false == use_program_query)
	{
		location = gl.getSubroutineUniformLocation(program_id, GL_VERTEX_SHADER, uniform_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetSubroutineUniformLocation");
	}
	else
	{
		location = gl.getProgramResourceLocation(program_id, GL_VERTEX_SUBROUTINE_UNIFORM, uniform_name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramResourceLocation");
	}

	if (-1 == location)
	{
		TCU_FAIL("Subroutine uniform is not available");
	}

	return location;
}

/** Test if getProgramStageiv results are as expected
 *
 * @param program_id Program object id
 *
 * @result false in case of invalid result for any pname, true otherwise
 **/
bool FunctionalTest3_4::inspectProgramStageiv(glw::GLuint program_id) const
{
	bool result = true;

	const inspectionDetails details[] = {
		{ GL_ACTIVE_SUBROUTINE_UNIFORMS, m_n_active_subroutine_uniforms },
		{ GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, m_n_active_subroutine_uniform_locations },
		{ GL_ACTIVE_SUBROUTINES, m_n_active_subroutines },
		{ GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH, m_n_active_subroutine_uniform_name_length + 1 },
		{ GL_ACTIVE_SUBROUTINE_MAX_LENGTH, m_n_active_subroutine_name_length + 1 }
	};
	const GLuint n_details = sizeof(details) / sizeof(details[0]);

	for (GLuint i = 0; i < n_details; ++i)
	{
		if (false == checkProgramStageiv(program_id, details[i].pname, details[i].expected_value))
		{
			result = false;
		}
	}

	return result;
}

/** Test if checkProgramInterfaceiv results are as expected
 *
 * @param program_id Program object id
 *
 * @result false in case of invalid result for any pname, true otherwise
 **/
bool FunctionalTest3_4::inspectProgramInterfaceiv(glw::GLuint program_id) const
{
	bool result = true;

	const inspectionDetailsForProgramInterface details[] = {
		{ GL_VERTEX_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, m_n_active_subroutine_uniforms },
		{ GL_VERTEX_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, m_n_active_subroutine_uniform_name_length + 1 },
		{ GL_VERTEX_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, m_n_active_subroutines },
		{ GL_VERTEX_SUBROUTINE, GL_ACTIVE_RESOURCES, m_n_active_subroutines },
		{ GL_VERTEX_SUBROUTINE, GL_MAX_NAME_LENGTH, m_n_active_subroutine_name_length + 1 }
	};
	const GLuint n_details = sizeof(details) / sizeof(details[0]);

	for (GLuint i = 0; i < n_details; ++i)
	{
		if (false == checkProgramInterfaceiv(program_id, details[i].program_interface, details[i].pname,
											 details[i].expected_value))
		{
			result = false;
		}
	}

	return result;
}

/** Test if checkProgramResourceiv results are as expected
 *
 * @param program_id       Program object id
 * @param subroutine_names Array of subroutine names
 * @param uniform_names    Array of uniform names
 *
 * @result false in case of invalid result for any pname, true otherwise
 **/
bool FunctionalTest3_4::inspectProgramResourceiv(GLuint program_id, const GLchar** subroutine_names,
												 const GLchar** uniform_names) const
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	for (GLint subroutine = 0; subroutine < m_n_active_subroutines; ++subroutine)
	{
		const GLchar* subroutine_name = subroutine_names[subroutine];
		const GLint   length		  = (GLint)strlen(subroutine_name) + 1;

		if (false == checkProgramResourceiv(program_id, GL_VERTEX_SUBROUTINE, GL_NAME_LENGTH, subroutine_name, length))
		{
			result = false;
		}
	}

	inspectionDetails details[] = {
		{ GL_NAME_LENGTH, 0 },
		{ GL_ARRAY_SIZE, 1 },
		{ GL_NUM_COMPATIBLE_SUBROUTINES, m_n_active_subroutines },
		{ GL_LOCATION, 0 },
	};
	const GLuint n_details = sizeof(details) / sizeof(details[0]);

	for (GLint uniform = 0; uniform < m_n_active_subroutine_uniforms; ++uniform)
	{
		const GLchar* uniform_name = uniform_names[uniform];
		const GLint   length	   = (GLint)strlen(uniform_name) + 1;
		const GLint   location	 = getSubroutineUniformLocation(program_id, uniform_name, true);

		details[0].expected_value = length;
		details[3].expected_value = location;

		for (GLuint i = 0; i < n_details; ++i)
		{
			if (false == checkProgramResourceiv(program_id, GL_VERTEX_SUBROUTINE_UNIFORM, details[i].pname,
												uniform_name, details[i].expected_value))
			{
				result = false;
			}
		}

		/* Check compatible subroutines */
		GLuint index = getProgramResourceIndex(program_id, GL_VERTEX_SUBROUTINE_UNIFORM, uniform_name);

		if (GL_INVALID_INDEX != index)
		{
			std::vector<GLint> compatible_subroutines;
			GLint			   index_sum = 0;
			GLenum			   prop		 = GL_COMPATIBLE_SUBROUTINES;

			compatible_subroutines.resize(m_n_active_subroutines);

			gl.getProgramResourceiv(program_id, GL_VERTEX_SUBROUTINE_UNIFORM, index, 1, &prop, m_n_active_subroutines,
									0, &compatible_subroutines[0]);

			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramResourceiv");

			/* Expected indices are 0, 1, 2, ... N */
			for (GLint i = 0; i < m_n_active_subroutines; ++i)
			{
				index_sum += compatible_subroutines[i];
			}

			/* Sum of E1, ..., EN = (E1 + EN) * N / 2 */
			if (((m_n_active_subroutines - 1) * m_n_active_subroutines) / 2 != index_sum)
			{
				tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

				message << "Error. Invalid result. Function: getProgramResourceiv. "
						<< "Program interface: GL_VERTEX_SUBROUTINE_UNIFORM. "
						<< "Resource name: " << uniform_name << ". "
						<< "Property: GL_COMPATIBLE_SUBROUTINES. "
						<< "Results: ";

				for (GLint i = 1; i < m_n_active_subroutines; ++i)
				{
					message << compatible_subroutines[i];
				}

				message << tcu::TestLog::EndMessage;

				result = false;
			}
		}
	}

	return result;
}

/** Test if getActiveSubroutineUniformiv results are as expected
 *
 * @param program_id    Program object id
 * @param uniform_names Array of subroutine uniform names available in program
 *
 * @result false in case of invalid result for any pname, true otherwise
 **/
bool FunctionalTest3_4::inspectActiveSubroutineUniformiv(GLuint program_id, const GLchar** uniform_names) const
{
	const glw::Functions& gl						   = m_context.getRenderContext().getFunctions();
	bool				  result					   = true;
	GLint				  n_active_subroutine_uniforms = 0;

	inspectionDetails details[] = {
		{ GL_NUM_COMPATIBLE_SUBROUTINES, m_n_active_subroutines },
		{ GL_UNIFORM_SIZE, m_n_active_subroutine_uniform_size },
		{ GL_UNIFORM_NAME_LENGTH, 0 },
	};
	const GLuint n_details = sizeof(details) / sizeof(details[0]);

	/* Get amount of active subroutine uniforms */
	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &n_active_subroutine_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	for (GLint uniform = 0; uniform < n_active_subroutine_uniforms; ++uniform)
	{
		GLint name_length = (GLint)strlen(uniform_names[uniform]);

		details[2].expected_value = name_length + 1;

		/* Checks from "details" */
		for (GLuint i = 0; i < n_details; ++i)
		{
			if (false ==
				checkActiveSubroutineUniformiv(program_id, uniform, details[i].pname, details[i].expected_value))
			{
				result = false;
			}
		}

		/* Check compatible subroutines */
		std::vector<GLint> compatible_subroutines;
		compatible_subroutines.resize(m_n_active_subroutines);
		GLint index_sum = 0;

		gl.getActiveSubroutineUniformiv(program_id, GL_VERTEX_SHADER, uniform, GL_COMPATIBLE_SUBROUTINES,
										&compatible_subroutines[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getActiveSubroutineUniformiv");

		/* Expected indices are 0, 1, 2, ... N */
		for (GLint i = 0; i < m_n_active_subroutines; ++i)
		{
			index_sum += compatible_subroutines[i];
		}

		/* Sum of E1, ..., EN = (E1 + EN) * N / 2 */
		if (((m_n_active_subroutines - 1) * m_n_active_subroutines) / 2 != index_sum)
		{
			tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

			message << "Error. Invalid result. Function: getActiveSubroutineUniformiv. idnex: " << uniform
					<< ". pname: " << Utils::pnameToStr(GL_COMPATIBLE_SUBROUTINES) << ". Results: ";

			for (GLint i = 1; i < m_n_active_subroutines; ++i)
			{
				message << compatible_subroutines[i];
			}

			message << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	return result;
}

/** Test if getActiveSubroutineUniformName results are as expected
 *
 * @param program_id    Program object id
 * @param uniform_names Array of subroutine uniform names available in program
 *
 * @result false in case of invalid result, true otherwise
 **/
bool FunctionalTest3_4::inspectActiveSubroutineUniformName(GLuint program_id, const GLchar** uniform_names) const
{
	const glw::Functions& gl						   = m_context.getRenderContext().getFunctions();
	bool				  result					   = true;
	GLint				  n_active_subroutine_uniforms = 0;
	std::vector<GLchar>   active_uniform_name;

	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &n_active_subroutine_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	active_uniform_name.resize(m_n_active_subroutine_uniform_name_length + 1);

	for (GLint uniform = 0; uniform < n_active_subroutine_uniforms; ++uniform)
	{
		bool is_name_ok = false;

		gl.getActiveSubroutineUniformName(program_id, GL_VERTEX_SHADER, uniform, (GLsizei)active_uniform_name.size(),
										  0 /* length */, &active_uniform_name[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetActiveSubroutineUniformName");

		for (GLint name = 0; name < n_active_subroutine_uniforms; ++name)
		{
			if (0 == strcmp(uniform_names[name], &active_uniform_name[0]))
			{
				is_name_ok = true;
				break;
			}
		}

		if (false == is_name_ok)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Error. Invalid result. Function: getActiveSubroutineUniformName. idnex: " << uniform
				<< ". Result: " << &active_uniform_name[0] << tcu::TestLog::EndMessage;

			result = false;
			break;
		}
	}

	return result;
}

/** Test if getActiveSubroutineUniformName results are as expected
 *
 * @param program_id       Program object id
 * @param subroutine_names Array of subroutine names available in program
 *
 * @result false in case of invalid result, true otherwise
 **/
bool FunctionalTest3_4::inspectActiveSubroutineName(GLuint program_id, const GLchar** subroutine_names) const
{
	const glw::Functions& gl				   = m_context.getRenderContext().getFunctions();
	bool				  result			   = true;
	GLint				  n_active_subroutines = 0;
	std::vector<GLchar>   active_subroutine_name;

	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &n_active_subroutines);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	active_subroutine_name.resize(m_n_active_subroutine_name_length + 1);

	for (GLint uniform = 0; uniform < n_active_subroutines; ++uniform)
	{
		bool is_name_ok = false;

		gl.getActiveSubroutineName(program_id, GL_VERTEX_SHADER, uniform, (GLsizei)active_subroutine_name.size(),
								   0 /* length */, &active_subroutine_name[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getActiveSubroutineName");

		for (GLint name = 0; name < n_active_subroutines; ++name)
		{
			if (0 == strcmp(subroutine_names[name], &active_subroutine_name[0]))
			{
				is_name_ok = true;
				break;
			}
		}

		if (false == is_name_ok)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Error. Invalid result. Function: getActiveSubroutineName. idnex: " << uniform
				<< ". Result: " << &active_subroutine_name[0] << tcu::TestLog::EndMessage;

			result = false;
			break;
		}
	}

	return result;
}

/** Test if it is possible to "bind" all subroutines uniforms with all subroutines
 *
 * @param program_id       Program object id
 * @param subroutine_names Array of subroutine names available in program
 * @param uniform_names    Array of subroutine uniform names available in program
 *
 * @result false in case of invalid result, true otherwise
 **/
bool FunctionalTest3_4::inspectSubroutineBinding(GLuint program_id, const GLchar** subroutine_names,
												 const GLchar** uniform_names, bool use_program_query) const
{
	const glw::Functions& gl						   = m_context.getRenderContext().getFunctions();
	bool				  result					   = true;
	GLint				  n_active_subroutines		   = 0;
	GLint				  n_active_subroutine_uniforms = 0;
	std::vector<GLuint>   subroutine_uniforms;
	GLuint				  queried_subroutine_index = 0;

	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &n_active_subroutines);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	gl.getProgramStageiv(program_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &n_active_subroutine_uniforms);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramStageiv");

	subroutine_uniforms.resize(n_active_subroutine_uniforms);

	for (GLint uniform = 0; uniform < n_active_subroutine_uniforms; ++uniform)
	{
		GLuint uniform_location = getSubroutineUniformLocation(program_id, uniform_names[uniform], use_program_query);

		for (GLint routine = 0; routine < n_active_subroutines; ++routine)
		{
			GLuint routine_index = getSubroutineIndex(program_id, subroutine_names[routine], use_program_query);

			subroutine_uniforms[uniform] = routine_index;

			gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, n_active_subroutine_uniforms, &subroutine_uniforms[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

			gl.getUniformSubroutineuiv(GL_VERTEX_SHADER, uniform_location, &queried_subroutine_index);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");

			if (queried_subroutine_index != routine_index)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Error. Invalid result. Function: gl.getUniformSubroutineuiv."
					<< " Subroutine uniform: " << uniform << ", name: " << uniform_names[uniform]
					<< ", location: " << uniform_location << ". Subroutine: " << routine
					<< ", name: " << subroutine_names[routine] << ", index: " << routine_index
					<< ". Result: " << queried_subroutine_index << tcu::TestLog::EndMessage;

				result = false;
			}
		}
	}

	return result;
}

/** Execute draw call and verify results
 *
 * @param program_id                   Program object id
 * @param first_routine_name           Name of subroutine that shall be used aas first_routine
 * @param second_routine_name          Name of subroutine that shall be used aas second_routine
 * @param uniform_names                Name of uniforms
 * @param expected_results             Test data. [0] is used as input data. All are used as expected_results
 * @param use_program_query            If true GetProgram* API will be used
 *
 * @return false in case of invalid result, true otherwise
 **/
bool FunctionalTest3_4::testDraw(GLuint program_id, const GLchar* first_routine_name, const GLchar* second_routine_name,
								 const GLchar** uniform_names, const Utils::vec4<GLfloat> expected_results[5],
								 bool use_program_query) const
{
	static const GLuint   n_varyings			 = 5;
	const glw::Functions& gl					 = m_context.getRenderContext().getFunctions();
	bool				  result				 = true;
	GLuint				  subroutine_uniforms[2] = { 0 };

	/* Get subroutine uniform locations */
	GLint first_routine_location = getSubroutineUniformLocation(program_id, uniform_names[0], use_program_query);

	GLint second_routine_location = getSubroutineUniformLocation(program_id, uniform_names[1], use_program_query);

	/* Get subroutine indices */
	GLuint first_routine_index = getSubroutineIndex(program_id, first_routine_name, use_program_query);

	GLuint second_routine_index = getSubroutineIndex(program_id, second_routine_name, use_program_query);

	/* Map uniforms with subroutines */
	subroutine_uniforms[first_routine_location]  = first_routine_index;
	subroutine_uniforms[second_routine_location] = second_routine_index;

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, 2 /* number of uniforms */, &subroutine_uniforms[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Get location of input_data */
	GLint input_data_location = gl.getUniformLocation(program_id, "input_data");
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

	if (-1 == input_data_location)
	{
		TCU_FAIL("Uniform is not available");
	}

	/* Set up input_data */
	gl.uniform4f(input_data_location, expected_results[0].m_x, expected_results[0].m_y, expected_results[0].m_z,
				 expected_results[0].m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Verify results */
	GLfloat* feedback_data = (GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	Utils::vec4<GLfloat> results[5];

	results[0].m_x = feedback_data[0];
	results[0].m_y = feedback_data[1];
	results[0].m_z = feedback_data[2];
	results[0].m_w = feedback_data[3];

	results[1].m_x = feedback_data[4];
	results[1].m_y = feedback_data[5];
	results[1].m_z = feedback_data[6];
	results[1].m_w = feedback_data[7];

	results[2].m_x = feedback_data[8];
	results[2].m_y = feedback_data[9];
	results[2].m_z = feedback_data[10];
	results[2].m_w = feedback_data[11];

	results[3].m_x = feedback_data[12];
	results[3].m_y = feedback_data[13];
	results[3].m_z = feedback_data[14];
	results[3].m_w = feedback_data[15];

	results[4].m_x = feedback_data[16];
	results[4].m_y = feedback_data[17];
	results[4].m_z = feedback_data[18];
	results[4].m_w = feedback_data[19];

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	for (GLuint i = 0; i < n_varyings; ++i)
	{
		result = result && (results[i] == expected_results[i]);
	}

	if (false == result)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Error. Invalid result. First routine: " << first_routine_name
											<< ". Second routine: " << second_routine_name << tcu::TestLog::EndMessage;

		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Results:";

		for (GLuint i = 0; i < n_varyings; ++i)
		{
			results[i].log(message);
		}

		message << tcu::TestLog::EndMessage;

		message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Expected:";

		for (GLuint i = 0; i < n_varyings; ++i)
		{
			expected_results[i].log(message);
		}

		message << tcu::TestLog::EndMessage;
	}

	return result;
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest5::FunctionalTest5(deqp::Context& context)
	: TestCase(context, "eight_subroutines_four_uniforms", "Verify multiple subroutine sets")
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest5::iterate()
{
	static const GLchar* vertex_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"// Subroutine types\n"
		"subroutine vec4  routine_type_1(in vec4 left, in vec4 right);\n"
		"subroutine vec4  routine_type_2(in vec4 iparam);\n"
		"subroutine vec4  routine_type_3(in vec4 a,    in vec4 b,    in vec4 c);\n"
		"subroutine bvec4 routine_type_4(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definitions\n"
		"// 1st type\n"
		"subroutine(routine_type_1) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type_1) vec4 subtract(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left - right;\n"
		"}\n"
		"\n"
		"// 2nd type\n"
		"subroutine(routine_type_2) vec4 square(in vec4 iparam)\n"
		"{\n"
		"    return iparam * iparam;\n"
		"}\n"
		"\n"
		"subroutine(routine_type_2) vec4 square_root(in vec4 iparam)\n"
		"{\n"
		"    return sqrt(iparam);\n"
		"}\n"
		"\n"
		"// 3rd type\n"
		"subroutine(routine_type_3) vec4 do_fma(in vec4 a, in vec4 b, in vec4 c)\n"
		"{\n"
		"    return fma(a, b, c);\n"
		"}\n"
		"\n"
		"subroutine(routine_type_3) vec4 blend(in vec4 a, in vec4 b, in vec4 c)\n"
		"{\n"
		"    return c * a + (vec4(1) - c) * b;\n"
		"}\n"
		"\n"
		"// 4th type\n"
		"subroutine(routine_type_4) bvec4 are_equal(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return equal(left, right);\n"
		"}\n"
		"\n"
		"subroutine(routine_type_4) bvec4 are_greater(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return greaterThan(left, right);\n"
		"}\n"
		"\n"
		"// Sub routine uniforms\n"
		"subroutine uniform routine_type_1 first_routine;\n"
		"subroutine uniform routine_type_2 second_routine;\n"
		"subroutine uniform routine_type_3 third_routine;\n"
		"subroutine uniform routine_type_4 fourth_routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 first_input;\n"
		"uniform vec4 second_input;\n"
		"uniform vec4 third_input;\n"
		"\n"
		"// Output\n"
		"out  vec4 out_result_from_first_routine;\n"
		"out  vec4 out_result_from_second_routine;\n"
		"out  vec4 out_result_from_third_routine;\n"
		"out uvec4 out_result_from_fourth_routine;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_result_from_first_routine  =       first_routine (first_input, second_input);\n"
		"    out_result_from_second_routine =       second_routine(first_input);\n"
		"    out_result_from_third_routine  =       third_routine (first_input, second_input, third_input);\n"
		"    out_result_from_fourth_routine = uvec4(fourth_routine(first_input, second_input));\n"
		"}\n"
		"\n";

	static const GLchar* subroutine_names[4][2] = {
		{ "add", "subtract" }, { "square", "square_root" }, { "do_fma", "blend" }, { "are_equal", "are_greater" }
	};

	static const GLchar* subroutine_uniform_names[4][1] = {
		{ "first_routine" }, { "second_routine" }, { "third_routine" }, { "fourth_routine" }
	};

	static const GLuint n_subroutine_types	 = sizeof(subroutine_names) / sizeof(subroutine_names[0]);
	static const GLuint n_subroutines_per_type = sizeof(subroutine_names[0]) / sizeof(subroutine_names[0][0]);
	static const GLuint n_subroutine_uniforms_per_type =
		sizeof(subroutine_uniform_names[0]) / sizeof(subroutine_uniform_names[0][0]);

	static const GLchar* uniform_names[] = { "first_input", "second_input", "third_input" };
	static const GLuint  n_uniform_names = sizeof(uniform_names) / sizeof(uniform_names[0]);

	static const GLchar* varying_names[] = { "out_result_from_first_routine", "out_result_from_second_routine",
											 "out_result_from_third_routine", "out_result_from_fourth_routine" };
	static const GLuint n_varyings					   = sizeof(varying_names) / sizeof(varying_names[0]);
	static const GLuint transform_feedback_buffer_size = n_varyings * sizeof(GLfloat) * 4 /* vec4 */;

	/* Test data */
	static const Utils::vec4<GLfloat> input_data[3] = { Utils::vec4<GLfloat>(1.0f, 4.0f, 9.0f, 16.0f),
														Utils::vec4<GLfloat>(16.0f, 9.0f, 4.0f, 1.0f),
														Utils::vec4<GLfloat>(0.25f, 0.5f, 0.75f, 1.0f) };

	static const Utils::vec4<GLfloat> expected_result_from_first_routine[2] = {
		Utils::vec4<GLfloat>(17.0f, 13.0f, 13.0f, 17.0f), Utils::vec4<GLfloat>(-15.0f, -5.0f, 5.0f, 15.0f)
	};

	static const Utils::vec4<GLfloat> expected_result_from_second_routine[2] = {
		Utils::vec4<GLfloat>(1.0f, 16.0f, 81.0f, 256.0f), Utils::vec4<GLfloat>(1.0f, 2.0f, 3.0f, 4.0f)
	};

	static const Utils::vec4<GLfloat> expected_result_from_third_routine[2] = {
		Utils::vec4<GLfloat>(16.25f, 36.5f, 36.75f, 17.0f), Utils::vec4<GLfloat>(12.25f, 6.5f, 7.75f, 16.0f)
	};

	static const Utils::vec4<GLuint> expected_result_from_fourth_routine[2] = { Utils::vec4<GLuint>(0, 0, 0, 0),
																				Utils::vec4<GLuint>(0, 0, 1, 1) };

	/* All combinations of subroutines */
	static const GLuint subroutine_combinations[][4] = {
		{ 0, 0, 0, 0 }, { 0, 0, 0, 1 }, { 0, 0, 1, 0 }, { 0, 0, 1, 1 }, { 0, 1, 0, 0 }, { 0, 1, 0, 1 },
		{ 0, 1, 1, 0 }, { 0, 1, 1, 1 }, { 1, 0, 0, 0 }, { 1, 0, 0, 1 }, { 1, 0, 1, 0 }, { 1, 0, 1, 1 },
		{ 1, 1, 0, 0 }, { 1, 1, 0, 1 }, { 1, 1, 1, 0 }, { 1, 1, 1, 1 }
	};
	static const GLuint n_subroutine_combinations =
		sizeof(subroutine_combinations) / sizeof(subroutine_combinations[0]);

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Result */
	bool result = true;

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code, varying_names,
				  n_varyings);

	program.use();

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();
	transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
									 GL_DYNAMIC_COPY);
	transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

	/* Get subroutine uniform locations and subroutine indices */
	for (GLuint type = 0; type < n_subroutine_types; ++type)
	{
		for (GLuint uniform = 0; uniform < n_subroutine_uniforms_per_type; ++uniform)
		{
			m_subroutine_uniform_locations[type][uniform] =
				program.getSubroutineUniformLocation(subroutine_uniform_names[type][uniform], GL_VERTEX_SHADER);
		}

		for (GLuint routine = 0; routine < n_subroutines_per_type; ++routine)
		{
			m_subroutine_indices[type][routine] =
				program.getSubroutineIndex(subroutine_names[type][routine], GL_VERTEX_SHADER);
		}
	}

	/* Get uniform locations */
	for (GLuint i = 0; i < n_uniform_names; ++i)
	{
		m_uniform_locations[i] = program.getUniformLocation(uniform_names[i]);
	}

	/* Draw with each routine combination */
	for (GLuint i = 0; i < n_subroutine_combinations; ++i)
	{
		Utils::vec4<GLfloat> first_routine_result;
		Utils::vec4<GLfloat> second_routine_result;
		Utils::vec4<GLfloat> third_routine_result;
		Utils::vec4<GLuint>  fourth_routine_result;

		testDraw(subroutine_combinations[i], input_data, first_routine_result, second_routine_result,
				 third_routine_result, fourth_routine_result);

		if (false == verify(first_routine_result, second_routine_result, third_routine_result, fourth_routine_result,
							expected_result_from_first_routine[subroutine_combinations[i][0]],
							expected_result_from_second_routine[subroutine_combinations[i][1]],
							expected_result_from_third_routine[subroutine_combinations[i][2]],
							expected_result_from_fourth_routine[subroutine_combinations[i][3]]))
		{
			logError(subroutine_names, subroutine_combinations[i], input_data, first_routine_result,
					 second_routine_result, third_routine_result, fourth_routine_result,
					 expected_result_from_first_routine[subroutine_combinations[i][0]],
					 expected_result_from_second_routine[subroutine_combinations[i][1]],
					 expected_result_from_third_routine[subroutine_combinations[i][2]],
					 expected_result_from_fourth_routine[subroutine_combinations[i][3]]);

			result = false;
		}
	}

	/* Done */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return tcu::TestNode::STOP;
}

/** Log error message
 *
 * @param subroutine_names               Array of subroutine names
 * @param subroutine_combination         Combination of subroutines
 * @param input_data                     Input data
 * @param first_routine_result           Result of first routine
 * @param second_routine_result          Result of second routine
 * @param third_routine_result           Result of third routine
 * @param fourth_routine_result          Result of fourth routine
 * @param first_routine_expected_result  Expected result of first routine
 * @param second_routine_expected_result Expected result of second routine
 * @param third_routine_expected_result  Expected result of third routine
 * @param fourth_routine_expected_result Expected result of fourth routine
 **/
void FunctionalTest5::logError(const glw::GLchar* subroutine_names[4][2], const glw::GLuint subroutine_combination[4],
							   const Utils::vec4<glw::GLfloat>  input_data[3],
							   const Utils::vec4<glw::GLfloat>& first_routine_result,
							   const Utils::vec4<glw::GLfloat>& second_routine_result,
							   const Utils::vec4<glw::GLfloat>& third_routine_result,
							   const Utils::vec4<glw::GLuint>&  fourth_routine_result,
							   const Utils::vec4<glw::GLfloat>& first_routine_expected_result,
							   const Utils::vec4<glw::GLfloat>& second_routine_expected_result,
							   const Utils::vec4<glw::GLfloat>& third_routine_expected_result,
							   const Utils::vec4<glw::GLuint>&  fourth_routine_expected_result) const
{
	m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
										<< tcu::TestLog::EndMessage;

	tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	message << "Function: " << subroutine_names[0][subroutine_combination[0]] << "( ";
	input_data[0].log(message);
	message << ", ";
	input_data[1].log(message);
	message << " ). Result: ";
	first_routine_result.log(message);
	message << ". Expected: ";
	first_routine_expected_result.log(message);

	message << tcu::TestLog::EndMessage;

	message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	message << "Function: " << subroutine_names[1][subroutine_combination[1]] << "( ";
	input_data[0].log(message);
	message << " ). Result: ";
	second_routine_result.log(message);
	message << ". Expected: ";
	second_routine_expected_result.log(message);

	message << tcu::TestLog::EndMessage;

	message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	message << "Function: " << subroutine_names[2][subroutine_combination[2]] << "( ";
	input_data[0].log(message);
	message << ", ";
	input_data[1].log(message);
	message << ", ";
	input_data[2].log(message);
	message << "). Result: ";
	third_routine_result.log(message);
	message << ". Expected: ";
	third_routine_expected_result.log(message);

	message << tcu::TestLog::EndMessage;

	message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

	message << "Function: " << subroutine_names[3][subroutine_combination[3]] << "( ";
	input_data[0].log(message);
	message << ", ";
	input_data[1].log(message);
	message << ", ";
	message << " ). Result: ";
	fourth_routine_result.log(message);
	message << ". Expected: ";
	fourth_routine_expected_result.log(message);

	message << tcu::TestLog::EndMessage;
}

/** Execute draw call and capture results
 *
 * @param subroutine_combination    Combination of subroutines
 * @param input_data                Input data
 * @param out_first_routine_result  Result of first routine
 * @param out_second_routine_result Result of second routine
 * @param out_third_routine_result  Result of third routine
 * @param out_fourth_routine_result Result of fourth routine
 **/
void FunctionalTest5::testDraw(const glw::GLuint			   subroutine_combination[4],
							   const Utils::vec4<glw::GLfloat> input_data[3],
							   Utils::vec4<glw::GLfloat>&	  out_first_routine_result,
							   Utils::vec4<glw::GLfloat>&	  out_second_routine_result,
							   Utils::vec4<glw::GLfloat>&	  out_third_routine_result,
							   Utils::vec4<glw::GLuint>&	   out_fourth_routine_result) const
{
	static const GLuint   n_uniforms = sizeof(m_uniform_locations) / sizeof(m_uniform_locations[0]);
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	GLuint				  subroutine_indices[4];
	static const GLuint   n_subroutine_uniforms = sizeof(subroutine_indices) / sizeof(subroutine_indices[0]);

	/* Prepare subroutine uniform data */
	for (GLuint i = 0; i < n_subroutine_uniforms; ++i)
	{
		const GLuint location = m_subroutine_uniform_locations[i][0];

		subroutine_indices[location] = m_subroutine_indices[i][subroutine_combination[i]];
	}

	/* Set up subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, n_subroutine_uniforms, &subroutine_indices[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Set up input data uniforms */
	for (GLuint i = 0; i < n_uniforms; ++i)
	{
		gl.uniform4f(m_uniform_locations[i], input_data[i].m_x, input_data[i].m_y, input_data[i].m_z,
					 input_data[i].m_w);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");
	}

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Capture results */
	GLvoid* feedback_data = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	GLfloat* float_ptr = (GLfloat*)feedback_data;

	/* First result */
	out_first_routine_result.m_x = float_ptr[0];
	out_first_routine_result.m_y = float_ptr[1];
	out_first_routine_result.m_z = float_ptr[2];
	out_first_routine_result.m_w = float_ptr[3];

	/* Second result */
	out_second_routine_result.m_x = float_ptr[4];
	out_second_routine_result.m_y = float_ptr[5];
	out_second_routine_result.m_z = float_ptr[6];
	out_second_routine_result.m_w = float_ptr[7];

	/* Third result */
	out_third_routine_result.m_x = float_ptr[8];
	out_third_routine_result.m_y = float_ptr[9];
	out_third_routine_result.m_z = float_ptr[10];
	out_third_routine_result.m_w = float_ptr[11];

	/* Fourth result */
	GLuint* uint_ptr			  = (GLuint*)(float_ptr + 12);
	out_fourth_routine_result.m_x = uint_ptr[0];
	out_fourth_routine_result.m_y = uint_ptr[1];
	out_fourth_routine_result.m_z = uint_ptr[2];
	out_fourth_routine_result.m_w = uint_ptr[3];

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
}

/** Verify if results match expected results
 *
 * @param first_routine_result           Result of first routine
 * @param second_routine_result          Result of second routine
 * @param third_routine_result           Result of third routine
 * @param fourth_routine_result          Result of fourth routine
 * @param first_routine_expected_result  Expected result of first routine
 * @param second_routine_expected_result Expected result of second routine
 * @param third_routine_expected_result  Expected result of third routine
 * @param fourth_routine_expected_result Expected result of fourth routine
 **/
bool FunctionalTest5::verify(const Utils::vec4<glw::GLfloat>& first_routine_result,
							 const Utils::vec4<glw::GLfloat>& second_routine_result,
							 const Utils::vec4<glw::GLfloat>& third_routine_result,
							 const Utils::vec4<glw::GLuint>&  fourth_routine_result,
							 const Utils::vec4<glw::GLfloat>& first_routine_expected_result,
							 const Utils::vec4<glw::GLfloat>& second_routine_expected_result,
							 const Utils::vec4<glw::GLfloat>& third_routine_expected_result,
							 const Utils::vec4<glw::GLuint>&  fourth_routine_expected_result) const
{
	bool result = true;

	result = result && (first_routine_result == first_routine_expected_result);
	result = result && (second_routine_result == second_routine_expected_result);
	result = result && (third_routine_result == third_routine_expected_result);
	result = result && (fourth_routine_result == fourth_routine_expected_result);

	return result;
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest6::FunctionalTest6(deqp::Context& context)
	: TestCase(context, "static_subroutine_call", "Verify that subroutine can be called in a static manner")
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest6::iterate()
{
	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "// Subroutine type\n"
											  "subroutine vec4 routine_type(in vec4 iparam);\n"
											  "\n"
											  "// Subroutine definition\n"
											  "subroutine(routine_type) vec4 square(in vec4 iparam)\n"
											  "{\n"
											  "    return iparam * iparam;\n"
											  "}\n"
											  "\n"
											  "// Sub routine uniform\n"
											  "subroutine uniform routine_type routine;\n"
											  "\n"
											  "// Input data\n"
											  "uniform vec4 input_data;\n"
											  "\n"
											  "// Output\n"
											  "out  vec4 out_result;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    out_result  = square(input_data);\n"
											  "}\n"
											  "\n";

	static const GLchar* varying_name = "out_result";

	/* Test data */
	static const Utils::vec4<GLfloat> input_data(1.0f, 4.0f, 9.0f, 16.0f);

	static const Utils::vec4<GLfloat> expected_result(1.0f, 16.0f, 81.0f, 256.0f);

	static const GLuint transform_feedback_buffer_size = 4 * sizeof(GLfloat);

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code, &varying_name,
				  1 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();
	transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
									 GL_DYNAMIC_COPY);
	transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

	/* Test */
	{
		const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
		const GLint			  uniform_location = gl.getUniformLocation(program.m_program_object_id, "input_data");

		GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformLocation");

		if (-1 == uniform_location)
		{
			TCU_FAIL("Uniform is not available");
		}

		/* Set up input data uniforms */
		gl.uniform4f(uniform_location, input_data.m_x, input_data.m_y, input_data.m_z, input_data.m_w);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

		/* Execute draw call with transform feedback */
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

		/* Capture results */
		GLfloat* feedback_data = (GLfloat*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

		Utils::vec4<GLfloat> result(feedback_data[0], feedback_data[1], feedback_data[2], feedback_data[3]);

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

		/* Verify */
		if (expected_result == result)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
												<< tcu::TestLog::EndMessage;

			tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

			message << "Function: square( ";
			input_data.log(message);
			message << " ). Result: ";
			result.log(message);
			message << ". Expected: ";
			expected_result.log(message);

			message << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest7_8::FunctionalTest7_8(deqp::Context& context)
	: TestCase(context, "arrayed_subroutine_uniforms", "Verify that subroutine can be called in a static manner")
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest7_8::iterate()
{
	static const GLchar* vertex_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"// Subroutine type\n"
		"subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definitions\n"
		"subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left * right;\n"
		"}\n"
		"\n"
		"// Sub routine uniform\n"
		"subroutine uniform routine_type routine[4];\n"
		"\n"
		"// Input data\n"
		"uniform vec4  uni_left;\n"
		"uniform vec4  uni_right;\n"
		"uniform uvec4 uni_indices;\n"
		"\n"
		"// Output\n"
		"out vec4 out_combined;\n"
		"out vec4 out_combined_inverted;\n"
		"out vec4 out_constant;\n"
		"out vec4 out_constant_inverted;\n"
		"out vec4 out_dynamic;\n"
		"out vec4 out_dynamic_inverted;\n"
		"out vec4 out_loop;\n"
		"out uint out_array_length;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_combined          = routine[3](routine[2](routine[1](routine[0](uni_left, uni_right), uni_right), "
		"uni_right), uni_right);\n"
		"    out_combined_inverted = routine[0](routine[1](routine[2](routine[3](uni_left, uni_right), uni_right), "
		"uni_right), uni_right);\n"
		"    \n"
		"    out_constant          = routine[3](routine[2](routine[1](routine[0](vec4(1, 2, 3, 4), vec4(-5, -6, -7, "
		"-8)), vec4(-1, -2, -3, -4)), vec4(5, 6, 7, 8)), vec4(1, 2, 3, 4));\n"
		"    out_constant_inverted = routine[0](routine[1](routine[2](routine[3](vec4(1, 2, 3, 4), vec4(-5, -6, -7, "
		"-8)), vec4(-1, -2, -3, -4)), vec4(5, 6, 7, 8)), vec4(1, 2, 3, 4));\n"
		"    \n"
		"    out_dynamic           = "
		"routine[uni_indices.w](routine[uni_indices.z](routine[uni_indices.y](routine[uni_indices.x](uni_left, "
		"uni_right), uni_right), uni_right), uni_right);\n"
		"    out_dynamic_inverted  = "
		"routine[uni_indices.x](routine[uni_indices.y](routine[uni_indices.z](routine[uni_indices.w](uni_left, "
		"uni_right), uni_right), uni_right), uni_right);\n"
		"    \n"
		"    out_loop              = uni_left;\n"
		"    for (uint i = 0u; i < routine.length(); ++i)\n"
		"    {\n"
		"        out_loop          = routine[i](out_loop, uni_right);\n"
		"    }\n"
		"    \n"
		"    out_array_length      = routine.length() + 6 - (uni_indices.x + uni_indices.y + uni_indices.z + "
		"uni_indices.w);\n"
		"}\n"
		"\n";

	static const GLchar* subroutine_names[] = {
		"add", "multiply",
	};
	static const GLuint n_subroutine_names = sizeof(subroutine_names) / sizeof(subroutine_names[0]);

	static const GLchar* subroutine_uniform_names[] = { "routine[0]", "routine[1]", "routine[2]", "routine[3]" };
	static const GLuint  n_subroutine_uniform_names =
		sizeof(subroutine_uniform_names) / sizeof(subroutine_uniform_names[0]);

	static const GLchar* uniform_names[] = {
		"uni_left", "uni_right", "uni_indices",
	};
	static const GLuint n_uniform_names = sizeof(uniform_names) / sizeof(uniform_names[0]);

	static const GLchar* varying_names[] = { "out_combined", "out_combined_inverted",
											 "out_constant", "out_constant_inverted",
											 "out_dynamic",  "out_dynamic_inverted",
											 "out_loop",	 "out_array_length" };

	static const GLuint n_varyings					   = sizeof(varying_names) / sizeof(varying_names[0]);
	static const GLuint transform_feedback_buffer_size = n_varyings * 4 * sizeof(GLfloat);

	/* Test data */
	static const Utils::vec4<GLfloat> uni_left(-1.0f, 0.75f, -0.5f, 0.25f);
	static const Utils::vec4<GLfloat> uni_right(1.0f, -0.75f, 0.5f, -0.25f);
	static const Utils::vec4<GLuint>  uni_indices(1, 2, 0, 3);

	static const GLuint subroutine_combinations[][4] = {
		{ 0, 0, 0, 0 }, /* + + + + */
		{ 0, 0, 0, 1 }, /* + + + * */
		{ 0, 0, 1, 0 }, /* + + * + */
		{ 0, 0, 1, 1 }, /* + + * * */
		{ 0, 1, 0, 0 }, /* + * + + */
		{ 0, 1, 0, 1 }, /* + * + * */
		{ 0, 1, 1, 0 }, /* + * * + */
		{ 0, 1, 1, 1 }, /* + * * * */
		{ 1, 0, 0, 0 }, /* * + + + */
		{ 1, 0, 0, 1 }, /* * + + * */
		{ 1, 0, 1, 0 }, /* * + * + */
		{ 1, 0, 1, 1 }, /* * + * * */
		{ 1, 1, 0, 0 }, /* * * + + */
		{ 1, 1, 0, 1 }, /* * * + * */
		{ 1, 1, 1, 0 }, /* * * * + */
		{ 1, 1, 1, 1 }  /* * * * * */
	};
	static const GLuint n_subroutine_combinations =
		sizeof(subroutine_combinations) / sizeof(subroutine_combinations[0]);

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	bool result = true;

	/* Init GL objects */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code, varying_names,
				  n_varyings);

	program.use();

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();

	/* Get subroutine indices */
	for (GLuint routine = 0; routine < n_subroutine_names; ++routine)
	{
		m_subroutine_indices[routine] = program.getSubroutineIndex(subroutine_names[routine], GL_VERTEX_SHADER);
	}

	/* Get subroutine uniform locations */
	for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
	{
		m_subroutine_uniform_locations[uniform] =
			program.getSubroutineUniformLocation(subroutine_uniform_names[uniform], GL_VERTEX_SHADER);
	}

	/* Get uniform locations */
	for (GLuint i = 0; i < n_uniform_names; ++i)
	{
		m_uniform_locations[i] = program.getUniformLocation(uniform_names[i]);
	}

	/* Test */
	for (GLuint i = 0; i < n_subroutine_combinations; ++i)
	{
		/* Clean */
		transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
										 GL_DYNAMIC_COPY);
		transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

		/* Verify */
		if (false == testDraw(subroutine_combinations[i], uni_left, uni_right, uni_indices))
		{
			result = false;
		}
	}

	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/* Calculate result of function applied to operands
 *
 * @param function Function id, 0 is sum, 1 is multiplication
 * @param left     Left operand
 * @param right    Right operand
 * @param out      Function result
 **/
void FunctionalTest7_8::calculate(glw::GLuint function, const Utils::vec4<glw::GLfloat>& left,
								  const Utils::vec4<glw::GLfloat>& right, Utils::vec4<glw::GLfloat>& out) const
{
	if (0 == function)
	{
		out.m_x = left.m_x + right.m_x;
		out.m_y = left.m_y + right.m_y;
		out.m_z = left.m_z + right.m_z;
		out.m_w = left.m_w + right.m_w;
	}
	else
	{
		out.m_x = left.m_x * right.m_x;
		out.m_y = left.m_y * right.m_y;
		out.m_z = left.m_z * right.m_z;
		out.m_w = left.m_w * right.m_w;
	}
}

/** Calculate expected values for all operations
 *
 * @param combination           Function combination, first applied function is at index [0]
 * @param left                  Left operand
 * @param right                 Right operand
 * @param indices               Indices used by dynamic calls
 * @param out_combined          Expected result of "combined" operation
 * @param out_combined_inverted Expected result of "combined_inverted" operation
 * @param out_constant          Expected result of "constant" operation
 * @param out_constant_inverted Expected result of "constant_inverted" operation
 * @param out_dynamic           Expected result of "dynamic" operation
 * @param out_dynamic_inverted  Expected result of "out_dynamic_inverted" operation
 * @param out_loop              Expected result of "loop" operation
 **/
void FunctionalTest7_8::calculate(
	const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left, const Utils::vec4<glw::GLfloat>& right,
	const Utils::vec4<glw::GLuint>& indices, Utils::vec4<glw::GLfloat>& out_combined,
	Utils::vec4<glw::GLfloat>& out_combined_inverted, Utils::vec4<glw::GLfloat>& out_constant,
	Utils::vec4<glw::GLfloat>& out_constant_inverted, Utils::vec4<glw::GLfloat>& out_dynamic,
	Utils::vec4<glw::GLfloat>& out_dynamic_inverted, Utils::vec4<glw::GLfloat>& out_loop) const
{
	/* Indices used by "dynamic" operations, range <0..4> */
	const GLuint dynamic_combination[4] = { combination[indices.m_x], combination[indices.m_y],
											combination[indices.m_z], combination[indices.m_w] };

	/* Values used by "constant" operations, come from shader code */
	const Utils::vec4<glw::GLfloat> constant_values[] = { Utils::vec4<glw::GLfloat>(1, 2, 3, 4),
														  Utils::vec4<glw::GLfloat>(-5, -6, -7, -8),
														  Utils::vec4<glw::GLfloat>(-1, -2, -3, -4),
														  Utils::vec4<glw::GLfloat>(5, 6, 7, 8),
														  Utils::vec4<glw::GLfloat>(1, 2, 3, 4) };

	/* Start values */
	Utils::vec4<glw::GLfloat> combined			= left;
	Utils::vec4<glw::GLfloat> combined_inverted = left;
	Utils::vec4<glw::GLfloat> constant			= constant_values[0];
	Utils::vec4<glw::GLfloat> constant_inverted = constant_values[0];
	Utils::vec4<glw::GLfloat> dynamic			= left;
	Utils::vec4<glw::GLfloat> dynamic_inverted  = left;

	/* Calculate expected results */
	for (GLuint i = 0; i < 4; ++i)
	{
		GLuint function					 = combination[i];
		GLuint function_inverted		 = combination[3 - i];
		GLuint dynamic_function			 = dynamic_combination[i];
		GLuint dynamic_function_inverted = dynamic_combination[3 - i];

		calculate(function, combined, right, combined);
		calculate(function_inverted, combined_inverted, right, combined_inverted);
		calculate(function, constant, constant_values[i + 1], constant);
		calculate(function_inverted, constant_inverted, constant_values[i + 1], constant_inverted);
		calculate(dynamic_function, dynamic, right, dynamic);
		calculate(dynamic_function_inverted, dynamic_inverted, right, dynamic_inverted);
	}

	/* Store results */
	out_combined		  = combined;
	out_combined_inverted = combined_inverted;
	out_constant		  = constant;
	out_constant_inverted = constant_inverted;
	out_dynamic			  = dynamic;
	out_dynamic_inverted  = dynamic_inverted;
	out_loop			  = combined;
}

/** Log error
 *
 * @param combination   Operations combination
 * @param left          Left operand
 * @param right         Right operand
 * @param indices       Inidices used by "dynamic" calls
 * @param vec4_expected Expected results
 * @param vec4_result   Results
 * @param array_length  Length of array
 * @param result        Comparison results
 **/
void FunctionalTest7_8::logError(const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left,
								 const Utils::vec4<glw::GLfloat>& right, const Utils::vec4<glw::GLuint>& indices,
								 const Utils::vec4<glw::GLfloat> vec4_expected[7],
								 const Utils::vec4<glw::GLfloat> vec4_result[7], glw::GLuint array_length,
								 bool result[7]) const
{
	static const GLuint n_functions  = 4;
	static const GLuint n_operations = 7;

	/* Indices used by "dynamic" operations, range <0..4> */
	const GLuint dynamic_combination[4] = { combination[indices.m_x], combination[indices.m_y],
											combination[indices.m_z], combination[indices.m_w] };

	/* Function symbols */
	GLchar functions[4];
	GLchar functions_inverted[4];
	GLchar functions_dynamic[4];
	GLchar functions_dynamic_inverted[4];

	for (GLuint i = 0; i < n_functions; ++i)
	{
		GLchar function			= (0 == combination[i]) ? '+' : '*';
		GLchar dynamic_function = (0 == dynamic_combination[i]) ? '+' : '*';

		functions[i]									= function;
		functions_inverted[n_functions - i - 1]			= function;
		functions_dynamic[i]							= dynamic_function;
		functions_dynamic_inverted[n_functions - i - 1] = dynamic_function;
	}

	/* Values used by "constant" operations, come from shader code */
	const Utils::vec4<glw::GLfloat> constant_values[] = { Utils::vec4<glw::GLfloat>(1, 2, 3, 4),
														  Utils::vec4<glw::GLfloat>(-5, -6, -7, -8),
														  Utils::vec4<glw::GLfloat>(-1, -2, -3, -4),
														  Utils::vec4<glw::GLfloat>(5, 6, 7, 8),
														  Utils::vec4<glw::GLfloat>(1, 2, 3, 4) };

	/* Values used by non-"constant" operations */
	Utils::vec4<glw::GLfloat> dynamic_values[5];
	dynamic_values[0] = left;
	dynamic_values[1] = right;
	dynamic_values[2] = right;
	dynamic_values[3] = right;
	dynamic_values[4] = right;

	/* For each operation */
	for (GLuint i = 0; i < n_operations; ++i)
	{
		/* If result is failure */
		if (false == result[i])
		{
			const GLchar*					 description = 0;
			const Utils::vec4<glw::GLfloat>* input		 = 0;
			const GLchar*					 operation   = 0;

			switch (i)
			{
			case 0:
				description = "Call made with predefined array indices";
				input		= dynamic_values;
				operation   = functions;
				break;
			case 1:
				description = "Call made with predefined array indices in inverted order";
				input		= dynamic_values;
				operation   = functions_inverted;
				break;
			case 2:
				description = "Call made with predefined array indices, for constant values";
				input		= constant_values;
				operation   = functions;
				break;
			case 3:
				description = "Call made with predefined array indices in inverted order, for constant values";
				input		= constant_values;
				operation   = functions_inverted;
				break;
			case 4:
				description = "Call made with dynamic array indices";
				input		= dynamic_values;
				operation   = functions_dynamic;
				break;
			case 5:
				description = "Call made with dynamic array indices in inverted order";
				input		= dynamic_values;
				operation   = functions_dynamic_inverted;
				break;
			case 6:
				description = "Call made with loop";
				input		= dynamic_values;
				operation   = functions;
				break;
			}

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
												<< tcu::TestLog::EndMessage;

			m_context.getTestContext().getLog() << tcu::TestLog::Message << description << tcu::TestLog::EndMessage;

			tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

			message << "Operation: ((((";
			input[0].log(message);
			for (GLuint function = 0; function < n_functions; ++function)
			{
				message << " " << operation[function] << " ";

				input[function + 1].log(message);

				message << ")";
			}

			message << tcu::TestLog::EndMessage;

			message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

			message << "Result: ";
			vec4_result[i].log(message);

			message << tcu::TestLog::EndMessage;

			message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

			message << "Expected: ";
			vec4_expected[i].log(message);

			message << tcu::TestLog::EndMessage;
		}

		/* Check array length, it should be 4 */
		if (4 != array_length)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Error. Invalid array length: " << array_length << ". Expected 4."
												<< tcu::TestLog::EndMessage;
		}
	}
}

/** Execute draw call and verifies captrued varyings
 *
 * @param combination           Function combination, first applied function is at index [0]
 * @param left                  Left operand
 * @param right                 Right operand
 * @param indices               Indices used by dynamic calls
 *
 * @return true if all results match expected values, false otherwise
 **/
bool FunctionalTest7_8::testDraw(const glw::GLuint combination[4], const Utils::vec4<glw::GLfloat>& left,
								 const Utils::vec4<glw::GLfloat>& right, const Utils::vec4<glw::GLuint>& indices) const
{
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	static const GLuint   n_vec4_varyings = 7;
	bool				  result		  = true;
	GLuint				  subroutine_indices[4];
	static const GLuint   n_subroutine_uniforms = sizeof(subroutine_indices) / sizeof(subroutine_indices[0]);

	/* Prepare expected results */
	Utils::vec4<glw::GLfloat> expected_results[7];
	calculate(combination, left, right, indices, expected_results[0], expected_results[1], expected_results[2],
			  expected_results[3], expected_results[4], expected_results[5], expected_results[6]);

	/* Set up input data uniforms */
	gl.uniform4f(m_uniform_locations[0], left.m_x, left.m_y, left.m_z, left.m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

	gl.uniform4f(m_uniform_locations[1], right.m_x, right.m_y, right.m_z, right.m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

	gl.uniform4ui(m_uniform_locations[2], indices.m_x, indices.m_y, indices.m_z, indices.m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4ui");

	/* Prepare subroutine uniform data */
	for (GLuint i = 0; i < n_subroutine_uniforms; ++i)
	{
		const GLuint location = m_subroutine_uniform_locations[i];

		subroutine_indices[location] = m_subroutine_indices[combination[i]];
	}

	/* Set up subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, n_subroutine_uniforms, &subroutine_indices[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Capture results */
	GLvoid* feedback_data = (GLvoid*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	Utils::vec4<GLfloat> vec4_results[7];
	bool				 results[7];
	GLfloat*			 float_data = (GLfloat*)feedback_data;
	for (GLuint i = 0; i < n_vec4_varyings; ++i)
	{
		vec4_results[i].m_x = float_data[i * 4 + 0];
		vec4_results[i].m_y = float_data[i * 4 + 1];
		vec4_results[i].m_z = float_data[i * 4 + 2];
		vec4_results[i].m_w = float_data[i * 4 + 3];
	}

	GLuint* uint_data	= (GLuint*)(float_data + (n_vec4_varyings)*4);
	GLuint  array_length = uint_data[0];

	/* Unmap buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Verification */
	for (GLuint i = 0; i < n_vec4_varyings; ++i)
	{
		results[i] = (vec4_results[i] == expected_results[i]);
		result	 = result && results[i];
	}

	result = result && (4 == array_length);

	/* Log error if any */
	if (false == result)
	{
		logError(combination, left, right, indices, expected_results, vec4_results, array_length, results);
	}

	/* Done */
	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest9::FunctionalTest9(deqp::Context& context)
	: TestCase(context, "subroutines_3_subroutine_types_and_subroutine_uniforms_one_function",
			   "Makes sure that program with one function associated with 3 different "
			   "subroutine types and 3 subroutine uniforms using that function compiles "
			   "and works as expected")
	, m_has_test_passed(true)
	, m_n_points_to_draw(16) /* arbitrary value */
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_xfb_bo_id(0)
{
	/* Left blank intentionally */
}

/** De-initializes GL objects that may have been created during test execution. */
void FunctionalTest9::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}
}

/** Retrieves body of a vertex shader that should be used
 *  for the testing purposes.
 **/
std::string FunctionalTest9::getVertexShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "subroutine void subroutineType1(inout float);\n"
		   "subroutine void subroutineType2(inout float);\n"
		   "subroutine void subroutineType3(inout float);\n"
		   "\n"
		   "subroutine(subroutineType1, subroutineType2, subroutineType3) void function(inout float result)\n"
		   "{\n"
		   "    result += float(0.123) + float(gl_VertexID);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineType1 subroutine_uniform1;\n"
		   "subroutine uniform subroutineType2 subroutine_uniform2;\n"
		   "subroutine uniform subroutineType3 subroutine_uniform3;\n"
		   "\n"
		   "out vec4 result;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    result = vec4(0, 1, 2, 3);\n"
		   "\n"
		   "    subroutine_uniform1(result.x);\n"
		   "    subroutine_uniform2(result.y);\n"
		   "    subroutine_uniform3(result.z);\n"
		   "\n"
		   "    result.w += result.x + result.y + result.z;\n"
		   "}\n";
}

/** Initializes all GL objects required to run the test. */
void FunctionalTest9::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up program object */
	const char* xfb_varyings[] = { "result" };

	const unsigned int n_xfb_varyings = sizeof(xfb_varyings) / sizeof(xfb_varyings[0]);
	if (!Utils::buildProgram(gl, getVertexShaderBody(), "",					  /* tc_body */
							 "",											  /* te_body */
							 "",											  /* gs_body */
							 "",											  /* fs_body */
							 xfb_varyings, n_xfb_varyings, &m_vs_id, DE_NULL, /* out_tc_id */
							 DE_NULL,										  /* out_te_id */
							 DE_NULL,										  /* out_gs_id */
							 DE_NULL,										  /* out_fs_id */
							 &m_po_id))
	{
		TCU_FAIL("Program failed to link successfully");
	}

	/* Set up a buffer object we will use to hold XFB data */
	const unsigned int xfb_bo_size = static_cast<unsigned int>(sizeof(float) * 4 /* components */ * m_n_points_to_draw);

	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_bo_size, DE_NULL, /* data */
				  GL_STATIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Generate & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest9::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}
	initTest();

	/* Issue a draw call to make use of the three subroutine uniforms that we've defined */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");
	{
		gl.drawArrays(GL_POINTS, 0 /* first */, m_n_points_to_draw);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Map the XFB BO storage into process space */
	const glw::GLvoid* xfb_data_ptr = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	verifyXFBData(xfb_data_ptr);

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/* All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies the data XFBed out by the vertex shader. Should the data
 *  be found invalid, m_has_test_passed will be set to false.
 *
 *  @param data_ptr XFB data.
 **/
void FunctionalTest9::verifyXFBData(const glw::GLvoid* data_ptr)
{
	const float			epsilon			= 1e-5f;
	bool				should_continue = true;
	const glw::GLfloat* traveller_ptr   = (const glw::GLfloat*)data_ptr;

	for (unsigned int n_point = 0; n_point < m_n_points_to_draw && should_continue; ++n_point)
	{
		tcu::Vec4 expected_result(0, 1, 2, 3);

		for (unsigned int n_component = 0; n_component < 3 /* xyz */; ++n_component)
		{
			expected_result[n_component] += 0.123f + float(n_point);
		}

		expected_result[3 /* w */] += expected_result[0] + expected_result[1] + expected_result[2];

		if (de::abs(expected_result[0] - traveller_ptr[0]) > epsilon ||
			de::abs(expected_result[1] - traveller_ptr[1]) > epsilon ||
			de::abs(expected_result[2] - traveller_ptr[2]) > epsilon ||
			de::abs(expected_result[3] - traveller_ptr[3]) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "XFBed data is invalid. Expected:"
														   "("
							   << expected_result[0] << ", " << expected_result[1] << ", " << expected_result[2] << ", "
							   << expected_result[3] << "), found:(" << traveller_ptr[0] << ", " << traveller_ptr[1]
							   << ", " << traveller_ptr[2] << ", " << traveller_ptr[3] << ")."
							   << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
			should_continue   = false;
		}

		traveller_ptr += 4; /* xyzw */
	}						/* for (all rendered points) */
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest10::FunctionalTest10(deqp::Context& context)
	: TestCase(context, "arrays_of_arrays_of_uniforms", "Verify that arrays of arrays of uniforms works as expected")
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest10::iterate()
{
	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_arrays_of_arrays  : require\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "// Subroutine type\n"
											  "subroutine int routine_type(in int iparam);\n"
											  "\n"
											  "// Subroutine definitions\n"
											  "subroutine(routine_type) int increment(in int iparam)\n"
											  "{\n"
											  "    return iparam + 1;\n"
											  "}\n"
											  "\n"
											  "subroutine(routine_type) int decrement(in int iparam)\n"
											  "{\n"
											  "    return iparam - 1;\n"
											  "}\n"
											  "\n"
											  "// Sub routine uniform\n"
											  "subroutine uniform routine_type routine[4][4];\n"
											  "\n"
											  "// Output\n"
											  "out int out_result;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    int result = 0;\n"
											  "    \n"
											  "    for (uint j = 0; j < routine.length(); ++j)\n"
											  "    {\n"
											  "        for (uint i = 0; i < routine[j].length(); ++i)\n"
											  "        {\n"
											  "            result = routine[j][i](result);\n"
											  "        }\n"
											  "    }\n"
											  "    \n"
											  "    out_result = result;\n"
											  "}\n"
											  "\n";

	static const GLchar* subroutine_names[] = {
		"increment", "decrement",
	};
	static const GLuint n_subroutine_names = sizeof(subroutine_names) / sizeof(subroutine_names[0]);

	static const GLchar* subroutine_uniform_names[] = {
		"routine[0][0]", "routine[1][0]", "routine[2][0]", "routine[3][0]", "routine[0][1]", "routine[1][1]",
		"routine[2][1]", "routine[3][1]", "routine[0][2]", "routine[1][2]", "routine[2][2]", "routine[3][2]",
		"routine[0][3]", "routine[1][3]", "routine[2][3]", "routine[3][3]"
	};
	static const GLuint n_subroutine_uniform_names =
		sizeof(subroutine_uniform_names) / sizeof(subroutine_uniform_names[0]);

	static const GLchar* varying_name					= "out_result";
	static const GLuint  transform_feedback_buffer_size = sizeof(GLint);

	static const GLuint configuration_increment[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	static const GLuint configuration_decrement[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

	static const GLuint configuration_mix[16] = { 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 };

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Do not execute the test if GL_ARB_arrays_of_arrays is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_arrays_of_arrays"))
	{
		throw tcu::NotSupportedError("GL_ARB_arrays_of_arrays is not supported.");
	}

	bool result = true;

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code, &varying_name,
				  1 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();
	transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
									 GL_DYNAMIC_COPY);
	transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

	/* Get subroutine indices */
	for (GLuint routine = 0; routine < n_subroutine_names; ++routine)
	{
		m_subroutine_indices[routine] = program.getSubroutineIndex(subroutine_names[routine], GL_VERTEX_SHADER);
	}

	/* Get subroutine uniform locations */
	for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
	{
		m_subroutine_uniform_locations[uniform] =
			program.getSubroutineUniformLocation(subroutine_uniform_names[uniform], GL_VERTEX_SHADER);
	}

	/* Test */
	GLint increment_result = testDraw(configuration_increment);
	GLint decrement_result = testDraw(configuration_decrement);
	GLint mix_result	   = testDraw(configuration_mix);

	/* Verify */
	if (16 != increment_result)
	{
		result = false;
	}

	if (-16 != decrement_result)
	{
		result = false;
	}
	if (0 != mix_result)
	{
		result = false;
	}

	/* Set test result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
											<< " Incrementation applied 16 times: " << increment_result
											<< ". Decrementation applied 16 times: " << decrement_result
											<< ". Incrementation and decrementation applied 8 times: " << mix_result
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Execute draw call and return captured varying
 *
 * @param routine_indices Configuration of subroutine uniforms
 *
 * @return Value of varying captured with transform feedback
 **/
GLint FunctionalTest10::testDraw(const GLuint routine_indices[16]) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	GLuint				  subroutine_indices[16];
	static const GLuint   n_subroutine_uniforms = sizeof(subroutine_indices) / sizeof(subroutine_indices[0]);

	/* Prepare subroutine uniform data */
	for (GLuint i = 0; i < n_subroutine_uniforms; ++i)
	{
		const GLuint location = m_subroutine_uniform_locations[i];

		subroutine_indices[location] = m_subroutine_indices[routine_indices[i]];
	}

	/* Set up subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, n_subroutine_uniforms, &subroutine_indices[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Capture results */
	GLint* feedback_data = (GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	GLint result = feedback_data[0];

	/* Unmap buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	return result;
}

/* Definitions of constants used by FunctionalTest11 */
const GLuint FunctionalTest11::m_texture_height = 32;
const GLuint FunctionalTest11::m_texture_width  = 32;

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest11::FunctionalTest11(deqp::Context& context)
	: TestCase(context, "globals_sampling_output_discard_function_calls", "Verify that global variables, texture "
																		  "sampling, fragment output, fragment discard "
																		  "and function calls work as expected")
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest11::iterate()
{
	static const GLchar* fragment_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"// Output\n"
		"layout(location = 0) out vec4 out_color;\n"
		"\n"
		"// Global variables\n"
		"vec4 success_color;\n"
		"vec4 failure_color;\n"
		"\n"
		"// Samplers\n"
		"uniform sampler2D sampler_1;\n"
		"uniform sampler2D sampler_2;\n"
		"\n"
		"// Functions\n"
		"bool are_same(in vec4 left, in vec4 right)\n"
		"{\n"
		"    bvec4 result;\n"
		"\n"
		"    result.x = (left.x == right.x);\n"
		"    result.y = (left.y == right.y);\n"
		"    result.z = (left.z == right.z);\n"
		"    result.w = (left.w == right.w);\n"
		"\n"
		"    return all(result);\n"
		"}\n"
		"\n"
		"bool are_different(in vec4 left, in vec4 right)\n"
		"{\n"
		"    bvec4 result;\n"
		"\n"
		"    result.x = (left.x != right.x);\n"
		"    result.y = (left.y != right.y);\n"
		"    result.z = (left.z != right.z);\n"
		"    result.w = (left.w != right.w);\n"
		"\n"
		"    return any(result);\n"
		"}\n"
		"\n"
		"// Subroutine types\n"
		"subroutine void discard_fragment_type(void);\n"
		"subroutine void set_global_colors_type(void);\n"
		"subroutine vec4 sample_texture_type(in vec2);\n"
		"subroutine bool comparison_type(in vec4 left, in vec4 right);\n"
		"subroutine void test_type(void);\n"
		"\n"
		"// Subroutine definitions\n"
		"// discard_fragment_type\n"
		"subroutine(discard_fragment_type) void discard_yes(void)\n"
		"{\n"
		"    discard;\n"
		"}\n"
		"\n"
		"subroutine(discard_fragment_type) void discard_no(void)\n"
		"{\n"
		"}\n"
		"\n"
		"// set_global_colors_type\n"
		"subroutine(set_global_colors_type) void red_pass_blue_fail(void)\n"
		"{\n"
		"    success_color = vec4(1, 0, 0, 1);\n"
		"    failure_color = vec4(0, 0, 1, 1);\n"
		"}\n"
		"\n"
		"subroutine(set_global_colors_type) void blue_pass_red_fail(void)\n"
		"{\n"
		"    success_color = vec4(0, 0, 1, 1);\n"
		"    failure_color = vec4(1, 0, 0, 1);\n"
		"}\n"
		"\n"
		"// sample_texture_type\n"
		"subroutine(sample_texture_type) vec4 first_sampler(in vec2 coord)\n"
		"{\n"
		"    return texture(sampler_1, coord);\n"
		"}\n"
		"\n"
		"subroutine(sample_texture_type) vec4 second_sampler(in vec2 coord)\n"
		"{\n"
		"    return texture(sampler_2, coord);\n"
		"}\n"
		"\n"
		"// comparison_type\n"
		"subroutine(comparison_type) bool check_equal(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return are_same(left, right);\n"
		"}\n"
		"\n"
		"subroutine(comparison_type) bool check_not_equal(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return are_different(left, right);\n"
		"}\n"
		"\n"
		"// Subroutine uniforms\n"
		"subroutine uniform discard_fragment_type  discard_fragment;\n"
		"subroutine uniform set_global_colors_type set_global_colors;\n"
		"subroutine uniform sample_texture_type    sample_texture;\n"
		"subroutine uniform comparison_type        compare;\n"
		"\n"
		"// Subroutine definitions\n"
		"// test_type\n"
		"subroutine(test_type) void test_with_discard(void)\n"
		"{\n"
		"    discard_fragment();"
		"\n"
		"    out_color = failure_color;\n"
		"\n"
		"    set_global_colors();\n"
		"\n"
		"    vec4 sampled_color = sample_texture(gl_PointCoord);\n"
		"\n"
		"    bool comparison_result = compare(success_color, sampled_color);\n"
		"\n"
		"    if (true == comparison_result)\n"
		"    {\n"
		"        out_color = success_color;\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        out_color = failure_color;\n"
		"    }\n"
		"}\n"
		"\n"
		"subroutine(test_type) void test_without_discard(void)\n"
		"{\n"
		"    set_global_colors();\n"
		"\n"
		"    vec4 sampled_color = sample_texture(gl_PointCoord);\n"
		"\n"
		"    bool comparison_result = compare(success_color, sampled_color);\n"
		"\n"
		"    if (true == comparison_result)\n"
		"    {\n"
		"        out_color = success_color;\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        out_color = failure_color;\n"
		"    }\n"
		"}\n"
		"\n"
		"// Subroutine uniforms\n"
		"subroutine uniform test_type test;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    // Set colors\n"
		"    success_color = vec4(0.5, 0.5, 0.5, 0.5);\n"
		"    failure_color = vec4(0.5, 0.5, 0.5, 0.5);\n"
		"\n"
		"    test();\n"
		"}\n"
		"\n";

	static const GLchar* geometry_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_subroutine : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(points)                           in;\n"
												"layout(triangle_strip, max_vertices = 4) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position = vec4(-1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4(-1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    EndPrimitive();\n"
												"}\n"
												"\n";

	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "}\n"
											  "\n";

	static const GLchar* subroutine_names[][2] = { { "discard_yes", "discard_no" },
												   { "red_pass_blue_fail", "blue_pass_red_fail" },
												   { "first_sampler", "second_sampler" },
												   { "check_equal", "check_not_equal" },
												   { "test_with_discard", "test_without_discard" } };
	static const GLuint n_subroutine_types = sizeof(subroutine_names) / sizeof(subroutine_names[0]);

	static const GLchar* subroutine_uniform_names[] = { "discard_fragment", "set_global_colors", "sample_texture",
														"compare", "test" };
	static const GLuint n_subroutine_uniform_names =
		sizeof(subroutine_uniform_names) / sizeof(subroutine_uniform_names[0]);

	static const GLchar* uniform_names[] = {
		"sampler_1", "sampler_2",
	};
	static const GLuint n_uniform_names = sizeof(uniform_names) / sizeof(uniform_names[0]);

	/* Colors */
	static const GLubyte blue_color[4]  = { 0, 0, 255, 255 };
	static const GLubyte clean_color[4] = { 0, 0, 0, 0 };
	static const GLubyte red_color[4]   = { 255, 0, 0, 255 };

	/* Configurations */
	static const testConfiguration test_configurations[] = {
		testConfiguration(
			"Expect red color from 1st sampler", red_color, 1 /* discard_fragment  : discard_no         */,
			0 /* set_global_colors : red_pass_blue_fail */, 0 /* sample_texture    : first_sampler      */,
			0 /* compare           : check_equal        */, 0 /* test              : test_with_discard  */, 1 /* red */,
			0 /* blue */),

		testConfiguration(
			"Test \"without discard\" option, expect no blue color from 2nd sampler", blue_color,
			0 /* discard_fragment  : discard_yes           */, 1 /* set_global_colors : blue_pass_red_fail    */,
			1 /* sample_texture    : second_sampler        */, 1 /* compare           : check_not_equal       */,
			1 /* test              : test_without_discard  */, 0 /* blue */, 1 /* red */),

		testConfiguration("Fragment shoud be discarded", clean_color, 0 /* discard_fragment  : discard_yes        */,
						  0 /* set_global_colors : red_pass_blue_fail */,
						  0 /* sample_texture    : first_sampler      */,
						  0 /* compare           : check_equal        */,
						  0 /* test              : test_with_discard  */, 1 /* red */, 0 /* blue */),

		testConfiguration(
			"Expect blue color from 1st sampler", blue_color, 1 /* discard_fragment  : discard_no         */,
			1 /* set_global_colors : blue_pass_red_fail */, 0 /* sample_texture    : first_sampler      */,
			0 /* compare           : check_equal        */, 0 /* test              : test_with_discard  */,
			0 /* blue */, 1 /* red */),

		testConfiguration(
			"Expect red color from 2nd sampler", red_color, 1 /* discard_fragment  : discard_no         */,
			0 /* set_global_colors : red_pass_blue_fail */, 1 /* sample_texture    : second_sampler     */,
			0 /* compare           : check_equal        */, 0 /* test              : test_with_discard  */,
			0 /* blue */, 1 /* red */),

		testConfiguration(
			"Expect no blue color from 2nd sampler", blue_color, 1 /* discard_fragment  : discard_no         */,
			1 /* set_global_colors : blue_pass_red_fail */, 1 /* sample_texture    : second_sampler     */,
			1 /* compare           : check_not_equal    */, 0 /* test              : test_with_discard  */,
			0 /* blue */, 1 /* red */),
	};
	static const GLuint n_test_cases = sizeof(test_configurations) / sizeof(test_configurations[0]);

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* GL objects */
	Utils::texture	 blue_texture(m_context);
	Utils::texture	 color_texture(m_context);
	Utils::framebuffer framebuffer(m_context);
	Utils::program	 program(m_context);
	Utils::texture	 red_texture(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, fragment_shader_code /* fs */, geometry_shader_code /* gs */, 0 /* tcs */, 0 /* test */,
				  vertex_shader_code, 0 /* varying_names */, 0 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	blue_texture.create(m_texture_width, m_texture_height, GL_RGBA8);
	color_texture.create(m_texture_width, m_texture_height, GL_RGBA8);
	red_texture.create(m_texture_width, m_texture_height, GL_RGBA8);

	framebuffer.generate();
	framebuffer.bind();
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, color_texture.m_id, m_texture_width, m_texture_height);

	/* Get subroutine indices */
	for (GLuint type = 0; type < n_subroutine_types; ++type)
	{
		m_subroutine_indices[type][0] = program.getSubroutineIndex(subroutine_names[type][0], GL_FRAGMENT_SHADER);
		m_subroutine_indices[type][1] = program.getSubroutineIndex(subroutine_names[type][1], GL_FRAGMENT_SHADER);
	}

	/* Get subroutine uniform locations */
	for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
	{
		m_subroutine_uniform_locations[uniform] =
			program.getSubroutineUniformLocation(subroutine_uniform_names[uniform], GL_FRAGMENT_SHADER);
	}

	/* Get uniform locations */
	for (GLuint i = 0; i < n_uniform_names; ++i)
	{
		m_uniform_locations[i] = program.getUniformLocation(uniform_names[i]);
	}

	/* Prepare textures */
	fillTexture(blue_texture, blue_color);
	fillTexture(color_texture, clean_color);
	fillTexture(red_texture, red_color);

	m_source_textures[0] = blue_texture.m_id;
	m_source_textures[1] = red_texture.m_id;

	framebuffer.clearColor(0.0f, 0.0f, 0.0f, 0.0f);

	/* Test */
	bool result = true;
	for (GLuint i = 0; i < n_test_cases; ++i)
	{
		/* Clean output texture */
		framebuffer.clear(GL_COLOR_BUFFER_BIT);

		/* Execute test */
		if (false == testDraw(test_configurations[i].m_routines, test_configurations[i].m_samplers,
							  test_configurations[i].m_expected_color, color_texture))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Error. Failure for configuration: " << test_configurations[i].m_description
				<< tcu::TestLog::EndMessage;

			result = false;
		}
	}

	/* Set result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Fill texture with specified color
 *
 * @param texture Texture instance
 * @param color   Color
 **/
void FunctionalTest11::fillTexture(Utils::texture& texture, const glw::GLubyte color[4]) const
{
	std::vector<GLubyte> texture_data;

	/* Prepare texture data */
	texture_data.resize(m_texture_width * m_texture_height * 4);

	for (GLuint y = 0; y < m_texture_height; ++y)
	{
		const GLuint line_offset = y * m_texture_width * 4;

		for (GLuint x = 0; x < m_texture_width; ++x)
		{
			const GLuint point_offset = x * 4 + line_offset;

			texture_data[point_offset + 0] = color[0]; /* red */
			texture_data[point_offset + 1] = color[1]; /* green */
			texture_data[point_offset + 2] = color[2]; /* blue */
			texture_data[point_offset + 3] = color[3]; /* alpha */
		}
	}

	texture.update(m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
}

/** Execute draw call and verify results
 *
 * @param routine_configuration Configurations of routines to be used
 * @param sampler_configuration Configuration of textures to be bound to samplers
 * @param expected_color        Expected color of result image
 *
 * @return true if result image is filled with expected color, false otherwise
 **/
bool FunctionalTest11::testDraw(const glw::GLuint routine_configuration[5], const glw::GLuint sampler_configuration[2],
								const glw::GLubyte expected_color[4], Utils::texture& color_texture) const
{
	const glw::Functions& gl					= m_context.getRenderContext().getFunctions();
	static const GLint	n_samplers			= 2;
	static const GLint	n_subroutine_uniforms = 5;
	GLuint				  subroutine_indices[5];

	/* Set samplers */
	for (GLuint i = 0; i < n_samplers; ++i)
	{
		const GLuint location = m_uniform_locations[i];
		const GLuint texture  = m_source_textures[sampler_configuration[i]];

		gl.activeTexture(GL_TEXTURE0 + i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");

		gl.bindTexture(GL_TEXTURE_2D, texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

		gl.uniform1i(location, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
	}

	gl.activeTexture(GL_TEXTURE0 + 0);

	/* Set subroutine uniforms */
	for (GLuint i = 0; i < n_subroutine_uniforms; ++i)
	{
		const GLuint location = m_subroutine_uniform_locations[i];
		const GLuint routine  = routine_configuration[i];

		subroutine_indices[location] = m_subroutine_indices[i][routine];
	}

	gl.uniformSubroutinesuiv(GL_FRAGMENT_SHADER, 5, subroutine_indices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Draw */
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Capture result */
	std::vector<GLubyte> captured_data;
	captured_data.resize(m_texture_width * m_texture_height * 4);

	color_texture.get(GL_RGBA, GL_UNSIGNED_BYTE, &captured_data[0]);

	/* Verify result */
	for (GLuint y = 0; y < m_texture_height; ++y)
	{
		const GLuint line_offset = y * m_texture_width * 4;

		for (GLuint x = 0; x < m_texture_width; ++x)
		{
			const GLuint point_offset   = x * 4 + line_offset;
			bool		 is_as_expected = true;

			is_as_expected = is_as_expected && (expected_color[0] == captured_data[point_offset + 0]); /* red */
			is_as_expected = is_as_expected && (expected_color[1] == captured_data[point_offset + 1]); /* green */
			is_as_expected = is_as_expected && (expected_color[2] == captured_data[point_offset + 2]); /* blue */
			is_as_expected = is_as_expected && (expected_color[3] == captured_data[point_offset + 3]); /* alpha */

			if (false == is_as_expected)
			{
				return false;
			}
		}
	}

	/* Done */
	return true;
}

/* Constatns used by FunctionalTest12 */
const glw::GLuint FunctionalTest12::m_texture_height = 16;
const glw::GLuint FunctionalTest12::m_texture_width  = 16;

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest12::FunctionalTest12(deqp::Context& context)
	: TestCase(context, "ssbo_atomic_image_load_store",
			   "Verify that SSBO, atomic counters and image load store work as expected")
	, m_left_image(0)
	, m_right_image(0)
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest12::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	bool result = true;

	/* Test atomic counters */
	if (true == m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_atomic_counters"))
	{
		if (false == testAtomic())
		{
			result = false;
		}
	}

	/* Test shader storage buffer */
	if (true == m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_storage_buffer_object"))
	{
		if (false == testSSBO())
		{
			result = false;
		}
	}

	/* Test image load store */
	if (true == m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_image_load_store"))
	{
		if (false == testImage())
		{
			result = false;
		}
	}

	/* Set result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Fill texture with specified color
 *
 * @param texture Texture instance
 * @param color   Color
 **/
void FunctionalTest12::fillTexture(Utils::texture& texture, const glw::GLuint color[4]) const
{
	std::vector<GLuint> texture_data;

	/* Prepare texture data */
	texture_data.resize(m_texture_width * m_texture_height * 4);

	for (GLuint y = 0; y < m_texture_height; ++y)
	{
		const GLuint line_offset = y * m_texture_width * 4;

		for (GLuint x = 0; x < m_texture_width; ++x)
		{
			const GLuint point_offset = x * 4 + line_offset;

			texture_data[point_offset + 0] = color[0]; /* red */
			texture_data[point_offset + 1] = color[1]; /* green */
			texture_data[point_offset + 2] = color[2]; /* blue */
			texture_data[point_offset + 3] = color[3]; /* alpha */
		}
	}

	texture.update(m_texture_width, m_texture_height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &texture_data[0]);
}

/** Test atomic counters
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest12::testAtomic()
{
	static const GLchar* fragment_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_atomic_counters : require\n"
												"#extension GL_ARB_shader_subroutine      : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(location = 0) out uint out_color;\n"
												"\n"
												"layout(binding = 0, offset = 8) uniform atomic_uint one;\n"
												"layout(binding = 0, offset = 4) uniform atomic_uint two;\n"
												"layout(binding = 0, offset = 0) uniform atomic_uint three;\n"
												"\n"
												"subroutine void atomic_routine(void)\n;"
												"\n"
												"subroutine(atomic_routine) void increment_two(void)\n"
												"{\n"
												"    out_color = atomicCounterIncrement(two);\n"
												"}\n"
												"\n"
												"subroutine(atomic_routine) void decrement_three(void)\n"
												"{\n"
												"    out_color = atomicCounterDecrement(three);\n"
												"}\n"
												"\n"
												"subroutine(atomic_routine) void read_one(void)\n"
												"{\n"
												"    out_color = atomicCounter(one);\n"
												"}\n"
												"\n"
												"subroutine uniform atomic_routine routine;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    routine();\n"
												"}\n"
												"\n";

	static const GLchar* geometry_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_subroutine : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(points)                           in;\n"
												"layout(triangle_strip, max_vertices = 4) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position = vec4(-1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4(-1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    EndPrimitive();\n"
												"}\n"
												"\n";

	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "}\n"
											  "\n";

	static const GLchar* subroutine_names[] = { "increment_two", "decrement_three", "read_one" };

	/* Test data */
	static const glw::GLuint atomic_buffer_data[] = { m_texture_width * m_texture_height,
													  m_texture_width * m_texture_height,
													  m_texture_width * m_texture_height };

	static const glw::GLuint expected_incremented_two[] = { atomic_buffer_data[0], 2 * atomic_buffer_data[1],
															atomic_buffer_data[2] };

	static const glw::GLuint expected_decremented_three[] = { 0, expected_incremented_two[1],
															  expected_incremented_two[2] };

	static const glw::GLuint expected_read_one[] = { expected_decremented_three[0], expected_decremented_three[1],
													 expected_decremented_three[2] };

	/* GL objects */
	Utils::buffer	  atomic_buffer(m_context);
	Utils::texture	 color_texture(m_context);
	Utils::framebuffer framebuffer(m_context);
	Utils::program	 program(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, fragment_shader_code /* fs */, geometry_shader_code /* gs */, 0 /* tcs */, 0 /* test */,
				  vertex_shader_code, 0 /* varying_names */, 0 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	color_texture.create(m_texture_width, m_texture_height, GL_R32UI);

	atomic_buffer.generate();
	atomic_buffer.update(GL_ATOMIC_COUNTER_BUFFER, sizeof(atomic_buffer_data), (GLvoid*)atomic_buffer_data,
						 GL_STATIC_DRAW);
	atomic_buffer.bindRange(GL_ATOMIC_COUNTER_BUFFER, 0 /* index */, 0 /* offset */, sizeof(atomic_buffer_data));

	framebuffer.generate();
	framebuffer.bind();
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, color_texture.m_id, m_texture_width, m_texture_height);
	framebuffer.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	framebuffer.clear(GL_COLOR_BUFFER_BIT);

	/* Subroutine indices */
	GLuint increment_two   = program.getSubroutineIndex(subroutine_names[0], GL_FRAGMENT_SHADER);
	GLuint decrement_three = program.getSubroutineIndex(subroutine_names[1], GL_FRAGMENT_SHADER);
	GLuint read_one		   = program.getSubroutineIndex(subroutine_names[2], GL_FRAGMENT_SHADER);

	/* Test */
	bool result = true;

	if (false == testAtomicDraw(increment_two, expected_incremented_two))
	{
		result = false;
	}

	if (false == testAtomicDraw(decrement_three, expected_decremented_three))
	{
		result = false;
	}

	if (false == testAtomicDraw(read_one, expected_read_one))
	{
		result = false;
	}

	/* Done */
	return result;
}

/** Execture draw call and verify results
 *
 * @param subroutine_index Index of subroutine that shall be used during draw call
 * @param expected_results Expected results
 *
 * @return true if results are as expected, false otherwise
 **/
bool FunctionalTest12::testAtomicDraw(GLuint subroutine_index, const GLuint expected_results[3]) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Draw */
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Capture results */
	GLuint* atomic_results = (GLuint*)gl.mapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	/* Verify */
	bool result = (0 == memcmp(expected_results, atomic_results, 3 * sizeof(GLuint)));

	if (false == result)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Error. Invalid result. "
			<< "Result: [ " << atomic_results[0] << ", " << atomic_results[1] << ", " << atomic_results[2] << " ] "
			<< "Expected: [ " << expected_results[0] << ", " << expected_results[1] << ", " << expected_results[2]
			<< " ]" << tcu::TestLog::EndMessage;
	}

	/* Unmap buffer */
	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Done */
	return result;
}

/** Test image load store
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest12::testImage()
{
	static const GLchar* fragment_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_image_load_store : require\n"
		"#extension GL_ARB_shader_subroutine       : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout(location = 0) out uvec4 out_color;\n"
		"\n"
		"layout(rgba32ui) uniform uimage2D left_image;\n"
		"layout(rgba32ui) uniform uimage2D right_image;\n"
		"\n"
		"subroutine void image_routine(void);\n"
		"\n"
		"subroutine(image_routine) void left_to_right(void)\n"
		"{\n"
		"    out_color = imageLoad (left_image,  ivec2(gl_FragCoord.xy));\n"
		"                imageStore(right_image, ivec2(gl_FragCoord.xy), out_color);\n"
		"}\n"
		"\n"
		"subroutine(image_routine) void right_to_left(void)\n"
		"{\n"
		"    out_color = imageLoad (right_image, ivec2(gl_FragCoord.xy));\n"
		"                imageStore(left_image,  ivec2(gl_FragCoord.xy), out_color);\n"
		"}\n"
		"\n"
		"subroutine uniform image_routine routine;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    routine();\n"
		"}\n"
		"\n";

	static const GLchar* geometry_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_subroutine : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(points)                           in;\n"
												"layout(triangle_strip, max_vertices = 4) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position = vec4(-1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4(-1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    EndPrimitive();\n"
												"}\n"
												"\n";

	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "}\n"
											  "\n";

	static const GLchar* subroutine_names[] = { "left_to_right", "right_to_left" };

	static const GLchar* uniform_names[] = { "left_image", "right_image" };

	/* Test data */
	static const GLuint blue_color[4]  = { 0, 0, 255, 255 };
	static const GLuint clean_color[4] = { 16, 32, 64, 128 };
	static const GLuint red_color[4]   = { 255, 0, 0, 255 };

	/* GL objects */
	Utils::texture	 blue_texture(m_context);
	Utils::texture	 destination_texture(m_context);
	Utils::texture	 color_texture(m_context);
	Utils::framebuffer framebuffer(m_context);
	Utils::program	 program(m_context);
	Utils::texture	 red_texture(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, fragment_shader_code /* fs */, geometry_shader_code /* gs */, 0 /* tcs */, 0 /* test */,
				  vertex_shader_code, 0 /* varying_names */, 0 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	blue_texture.create(m_texture_width, m_texture_height, GL_RGBA32UI);
	destination_texture.create(m_texture_width, m_texture_height, GL_RGBA32UI);
	color_texture.create(m_texture_width, m_texture_height, GL_RGBA32UI);
	red_texture.create(m_texture_width, m_texture_height, GL_RGBA32UI);

	fillTexture(blue_texture, blue_color);
	fillTexture(destination_texture, clean_color);
	fillTexture(red_texture, red_color);

	framebuffer.generate();
	framebuffer.bind();
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, color_texture.m_id, m_texture_width, m_texture_height);
	framebuffer.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	framebuffer.clear(GL_COLOR_BUFFER_BIT);

	/* Subroutine indices */
	GLuint left_to_right = program.getSubroutineIndex(subroutine_names[0], GL_FRAGMENT_SHADER);
	GLuint right_to_left = program.getSubroutineIndex(subroutine_names[1], GL_FRAGMENT_SHADER);

	/* Uniform locations */
	m_left_image  = program.getUniformLocation(uniform_names[0]);
	m_right_image = program.getUniformLocation(uniform_names[1]);

	/* Test */
	bool result = true;

	if (false == testImageDraw(left_to_right, blue_texture, destination_texture, blue_color, blue_color))
	{
		result = false;
	}

	if (false == testImageDraw(left_to_right, red_texture, destination_texture, red_color, red_color))
	{
		result = false;
	}

	if (false == testImageDraw(right_to_left, destination_texture, blue_texture, blue_color, blue_color))
	{
		result = false;
	}

	if (false == testImageDraw(right_to_left, destination_texture, red_texture, red_color, red_color))
	{
		result = false;
	}

	if (false == testImageDraw(left_to_right, blue_texture, red_texture, blue_color, blue_color))
	{
		result = false;
	}

	/* Done */
	return result;
}

/** Execute draw call and verifies results
 *
 * @param subroutine_index     Index of subroutine that shall be used during draw call
 * @param left                 "Left" texture
 * @param right                "Right" texture
 * @param expected_left_color  Expected color of "left" texture
 * @param expected_right_color Expected color of "right" texture
 *
 * @return true if verification result is positive, false otherwise
 **/
bool FunctionalTest12::testImageDraw(GLuint subroutine_index, Utils::texture& left, Utils::texture& right,
									 const GLuint expected_left_color[4], const GLuint expected_right_color[4]) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Set up image units */
	gl.uniform1i(m_left_image, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	gl.uniform1i(m_right_image, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	gl.bindImageTexture(0, left.m_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	gl.bindImageTexture(1, right.m_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	/* Draw */
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Verify results */
	bool result = true;

	if (false == verifyTexture(left, expected_left_color))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Error. Invalid result. Left texture is filled with wrong color."
											<< tcu::TestLog::EndMessage;
		result = false;
	}

	if (false == verifyTexture(right, expected_right_color))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Error. Invalid result. Right texture is filled with wrong color."
											<< tcu::TestLog::EndMessage;
		result = false;
	}

	/* Done */
	return result;
}

/** Test shader storage buffer
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest12::testSSBO()
{
	static const GLchar* fragment_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_storage_buffer_object : require\n"
												"#extension GL_ARB_shader_subroutine            : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(location = 0) out uvec4 out_color;\n"
												"\n"
												"layout(std140, binding = 0) buffer Buffer\n"
												"{\n"
												"    uvec4 entry;\n"
												"};\n"
												"\n"
												"subroutine void ssbo_routine(void)\n;"
												"\n"
												"subroutine(ssbo_routine) void increment(void)\n"
												"{\n"
												"    out_color.x = atomicAdd(entry.x, 1);\n"
												"    out_color.y = atomicAdd(entry.y, 1);\n"
												"    out_color.z = atomicAdd(entry.z, 1);\n"
												"    out_color.w = atomicAdd(entry.w, 1);\n"
												"}\n"
												"\n"
												"subroutine(ssbo_routine) void decrement(void)\n"
												"{\n"
												"    out_color.x = atomicAdd(entry.x, -1);\n"
												"    out_color.y = atomicAdd(entry.y, -1);\n"
												"    out_color.z = atomicAdd(entry.z, -1);\n"
												"    out_color.w = atomicAdd(entry.w, -1);\n"
												"}\n"
												"\n"
												"subroutine uniform ssbo_routine routine;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    routine();\n"
												"}\n"
												"\n";

	static const GLchar* geometry_shader_code = "#version 400 core\n"
												"#extension GL_ARB_shader_subroutine : require\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(points)                           in;\n"
												"layout(triangle_strip, max_vertices = 4) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position = vec4(-1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4(-1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    gl_Position = vec4( 1,  1, 0, 1);\n"
												"    EmitVertex();\n"
												"    \n"
												"    EndPrimitive();\n"
												"}\n"
												"\n";

	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "}\n"
											  "\n";

	static const GLchar* subroutine_names[] = { "increment", "decrement" };

	/* Test data */
	static const glw::GLuint buffer_data[] = { m_texture_width * m_texture_height + 1,
											   m_texture_width * m_texture_height + 2,
											   m_texture_width * m_texture_height + 3,
											   m_texture_width * m_texture_height + 4 };

	static const glw::GLuint expected_incremented[] = { m_texture_width * m_texture_height + buffer_data[0],
														m_texture_width * m_texture_height + buffer_data[1],
														m_texture_width * m_texture_height + buffer_data[2],
														m_texture_width * m_texture_height + buffer_data[3] };

	static const glw::GLuint expected_decremented[] = { buffer_data[0], buffer_data[1], buffer_data[2],
														buffer_data[3] };

	/* GL objects */
	Utils::buffer	  buffer(m_context);
	Utils::texture	 color_texture(m_context);
	Utils::framebuffer framebuffer(m_context);
	Utils::program	 program(m_context);
	Utils::vertexArray vao(m_context);

	/* Init GL objects */
	program.build(0 /* cs */, fragment_shader_code /* fs */, geometry_shader_code /* gs */, 0 /* tcs */, 0 /* test */,
				  vertex_shader_code, 0 /* varying_names */, 0 /* n_varyings */);

	program.use();

	vao.generate();
	vao.bind();

	color_texture.create(m_texture_width, m_texture_height, GL_RGBA32UI);

	buffer.generate();
	buffer.update(GL_SHADER_STORAGE_BUFFER, sizeof(buffer_data), (GLvoid*)buffer_data, GL_STATIC_DRAW);
	buffer.bindRange(GL_SHADER_STORAGE_BUFFER, 0 /* index */, 0 /* offset */, sizeof(buffer_data));

	framebuffer.generate();
	framebuffer.bind();
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, color_texture.m_id, m_texture_width, m_texture_height);
	framebuffer.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	framebuffer.clear(GL_COLOR_BUFFER_BIT);

	/* Subroutine indices */
	GLuint increment = program.getSubroutineIndex(subroutine_names[0], GL_FRAGMENT_SHADER);
	GLuint decrement = program.getSubroutineIndex(subroutine_names[1], GL_FRAGMENT_SHADER);

	/* Test */
	bool result = true;

	if (false == testSSBODraw(increment, expected_incremented))
	{
		result = false;
	}

	if (false == testSSBODraw(decrement, expected_decremented))
	{
		result = false;
	}

	/* Done */
	return result;
}

/** Execute draw call and verify results
 *
 * @param subroutine_index Index of subroutine that shall be used by draw call
 * @param expected_results Expected results
 *
 *
 **/
bool FunctionalTest12::testSSBODraw(GLuint subroutine_index, const GLuint expected_results[4]) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Draw */
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	/* Capture results */
	GLuint* ssbo_results = (GLuint*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	/* Verify */
	bool result = (0 == memcmp(expected_results, ssbo_results, 4 * sizeof(GLuint)));

	if (false == result)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result. "
											<< "Result: [ " << ssbo_results[0] << ", " << ssbo_results[1] << ", "
											<< ssbo_results[2] << ", " << ssbo_results[3] << " ] "
											<< "Expected: [ " << expected_results[0] << ", " << expected_results[1]
											<< ", " << expected_results[2] << ", " << expected_results[3] << " ]"
											<< tcu::TestLog::EndMessage;
	}

	/* Unmap buffer */
	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Done */
	return result;
}

/** Check if texture is filled with expected color
 *
 * @param texture        Texture instance
 * @param expected_color Expected color
 *
 * @return true if texture is filled with specified color, false otherwise
 **/
bool FunctionalTest12::verifyTexture(Utils::texture& texture, const GLuint expected_color[4]) const
{
	std::vector<GLuint> results;
	results.resize(m_texture_width * m_texture_height * 4);

	texture.get(GL_RGBA_INTEGER, GL_UNSIGNED_INT, &results[0]);

	for (GLuint y = 0; y < m_texture_height; ++y)
	{
		const GLuint line_offset = y * m_texture_width * 4;

		for (GLuint x = 0; x < m_texture_width; ++x)
		{
			const GLuint point_offset = line_offset + x * 4;
			bool		 result		  = true;

			result = result && (results[point_offset + 0] == expected_color[0]);
			result = result && (results[point_offset + 1] == expected_color[1]);
			result = result && (results[point_offset + 2] == expected_color[2]);
			result = result && (results[point_offset + 3] == expected_color[3]);

			if (false == result)
			{
				return false;
			}
		}
	}

	return true;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest13::FunctionalTest13(deqp::Context& context)
	: TestCase(context, "subroutines_with_separate_shader_objects",
			   "Verifies that subroutines work correctly when used in separate "
			   "shader objects")
	, m_fbo_id(0)
	, m_pipeline_id(0)
	, m_read_buffer(DE_NULL)
	, m_to_height(4)
	, m_to_id(0)
	, m_to_width(4)
	, m_vao_id(0)
	, m_has_test_passed(true)
{
	memset(m_fs_po_ids, 0, sizeof(m_fs_po_ids));
	memset(m_gs_po_ids, 0, sizeof(m_gs_po_ids));
	memset(m_tc_po_ids, 0, sizeof(m_tc_po_ids));
	memset(m_te_po_ids, 0, sizeof(m_te_po_ids));
	memset(m_vs_po_ids, 0, sizeof(m_vs_po_ids));
}

/** Deinitializes all GL objects that may have been created during test
 *  execution, as well as releases all process-side buffers that may have
 *  been allocated during the process.
 *  The function also restores default GL state configuration.
 **/
void FunctionalTest13::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_pipeline_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_id);

		m_pipeline_id = 0;
	}

	if (m_read_buffer != DE_NULL)
	{
		delete[] m_read_buffer;

		m_read_buffer = DE_NULL;
	}

	for (unsigned int n_id = 0; n_id < 2 /* po id variants */; ++n_id)
	{
		if (m_fs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_fs_po_ids[n_id]);

			m_fs_po_ids[n_id] = 0;
		}

		if (m_gs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_gs_po_ids[n_id]);

			m_gs_po_ids[n_id] = 0;
		}

		if (m_tc_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_tc_po_ids[n_id]);

			m_tc_po_ids[n_id] = 0;
		}

		if (m_te_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_te_po_ids[n_id]);

			m_te_po_ids[n_id] = 0;
		}

		if (m_vs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_vs_po_ids[n_id]);

			m_vs_po_ids[n_id] = 0;
		}
	} /* for (both shader program object variants) */

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Restore default GL_PATCH_VERTICES setting value */
	gl.patchParameteri(GL_PATCH_VERTICES, 3);

	/* Restore default GL_PACK_ALIGNMENT setting value */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
}

/** Retrieves body of a fragment shader that should be used for the test.
 *  The subroutine implementations are slightly changed, depending on the
 *  index of the shader, as specified by the caller.
 *
 *  @param n_id Index of the shader.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest13::getFragmentShaderBody(unsigned int n_id)
{
	std::stringstream result_sstream;

	/* Pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  /* Sub-routine */
					  "subroutine void SubroutineFSType(inout vec4 result);\n"
					  "\n"
					  "subroutine(SubroutineFSType) void SubroutineFS1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4("
				   << float(n_id + 1) / 10.0f << ", " << float(n_id + 2) / 10.0f << ", " << float(n_id + 3) / 10.0f
				   << ", " << float(n_id + 4) / 10.0f
				   << ");\n"
					  "}\n"
					  "subroutine(SubroutineFSType) void SubroutineFS2(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4("
				   << float(n_id + 1) / 20.0f << ", " << float(n_id + 2) / 20.0f << ", " << float(n_id + 3) / 20.0f
				   << ", " << float(n_id + 4) / 20.0f << ");\n"
														 "}\n"
														 "\n"
														 "subroutine uniform SubroutineFSType function;\n"
														 "\n"
														 /* Input block */
														 "in GS_DATA\n"
														 "{\n"
														 "    vec4 data;\n"
														 "} in_gs;\n"
														 "\n"
														 "out vec4 result;\n"
														 /* main() declaration */
														 "void main()\n"
														 "{\n"
														 "    vec4 data = in_gs.data;\n"
														 "    function(data);\n"
														 "\n"
														 "    result = data;\n"
														 "}\n";

	return result_sstream.str();
}

/** Retrieves body of a geometry shader that should be used for the test.
 *  The subroutine implementations are slightly changed, depending on the
 *  index of the shader, as specified by the caller.
 *
 *  @param n_id Index of the shader.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest13::getGeometryShaderBody(unsigned int n_id)
{
	std::stringstream result_sstream;

	/* Pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout(points)                           in;\n"
					  "layout(triangle_strip, max_vertices = 4) out;\n"
					  /* Sub-routine */
					  "subroutine void SubroutineGSType(inout vec4 result);\n"
					  "\n"
					  "subroutine(SubroutineGSType) void SubroutineGS1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4(0, 0, 0, "
				   << float(n_id + 1) * 0.425f << ");\n"
												  "}\n"
												  "subroutine(SubroutineGSType) void SubroutineGS2(inout vec4 result)\n"
												  "{\n"
												  "    result += vec4(0, 0, 0, "
				   << float(n_id + 1) * 0.0425f << ");\n"
												   "}\n"
												   "\n"
												   "subroutine uniform SubroutineGSType function;\n"
												   "\n"
												   /* Input block */
												   "in TE_DATA\n"
												   "{\n"
												   "    vec4 data;\n"
												   "} in_te[];\n"
												   "\n"
												   /* Output block */
												   "out GS_DATA\n"
												   "{\n"
												   "    vec4 data;\n"
												   "} out_gs;\n"
												   "\n"
												   "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
												   "out gl_PerVertex { vec4 gl_Position; };\n"
												   /* main() declaration */
												   "void main()\n"
												   "{\n"
												   "    vec4 data = in_te[0].data;\n"
												   "\n"
												   "    function(data);\n"
												   "\n"
												   "    gl_Position = vec4(1, -1, 0, 1);\n"
												   "    out_gs.data = data;\n"
												   "    EmitVertex();\n"
												   "\n"
												   "    gl_Position = vec4(-1, -1, 0, 1);\n"
												   "    out_gs.data = data;\n"
												   "    EmitVertex();\n"
												   "\n"
												   "    gl_Position = vec4(1, 1, 0, 1);\n"
												   "    out_gs.data = data;\n"
												   "    EmitVertex();\n"
												   "\n"
												   "    gl_Position = vec4(-1, 1, 0, 1);\n"
												   "    out_gs.data = data;\n"
												   "    EmitVertex();\n"
												   "    EndPrimitive();\n"
												   "}\n";

	return result_sstream.str();
}

/** Retrieves body of a tessellation control shader that should be used for the test.
 *  The subroutine implementations are slightly changed, depending on the
 *  index of the shader, as specified by the caller.
 *
 *  @param n_id Index of the shader.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest13::getTessellationControlShaderBody(unsigned int n_id)
{
	std::stringstream result_sstream;

	/* Pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout(vertices = 4) out;\n"
					  /* Sub-routine */
					  "subroutine void SubroutineTCType(inout vec4 result);\n"
					  "\n"
					  "subroutine(SubroutineTCType) void SubroutineTC1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4(0, "
				   << float(n_id + 1) * 0.25f << ", 0, 0);\n"
												 "}\n"
												 "subroutine(SubroutineTCType) void SubroutineTC2(inout vec4 result)\n"
												 "{\n"
												 "    result += vec4(0, "
				   << float(n_id + 1) * 0.025f
				   << ", 0, 0);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform SubroutineTCType function;\n"
					  "\n"
					  /* Input block */
					  "in VS_DATA\n"
					  "{\n"
					  "    vec4 data;\n"
					  "} in_vs[];\n"
					  "\n"
					  /* Output block */
					  "out TC_DATA\n"
					  "{\n"
					  "    vec4 data;\n"
					  "} out_tc[];\n"
					  "\n"
					  "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
					  "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
					  /* main() declaration */
					  "void main()\n"
					  "{\n"
					  "    gl_TessLevelOuter[0]                = 1.0;\n"
					  "    gl_TessLevelOuter[1]                = 1.0;\n"
					  "    gl_TessLevelOuter[2]                = 1.0;\n"
					  "    gl_TessLevelOuter[3]                = 1.0;\n"
					  "    gl_TessLevelInner[0]                = 1.0;\n"
					  "    gl_TessLevelInner[1]                = 1.0;\n"
					  "    gl_out[gl_InvocationID].gl_Position = gl_in[0].gl_Position;\n"
					  "    out_tc[gl_InvocationID].data        = in_vs[0].data;\n"
					  "\n"
					  "    function(out_tc[gl_InvocationID].data);\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves body of a tessellation evaluation shader that should be used for the test.
 *  The subroutine implementations are slightly changed, depending on the
 *  index of the shader, as specified by the caller.
 *
 *  @param n_id Index of the shader.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest13::getTessellationEvaluationShaderBody(unsigned int n_id)
{
	std::stringstream result_sstream;

	/* Pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout(quads, point_mode) in;\n"
					  /* Sub-routine */
					  "subroutine void SubroutineTEType(inout vec4 result);\n"
					  "\n"
					  "subroutine(SubroutineTEType) void SubroutineTE1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4(0, 0, "
				   << float(n_id + 1) * 0.325f << ", 0);\n"
												  "}\n"
												  "subroutine(SubroutineTEType) void SubroutineTE2(inout vec4 result)\n"
												  "{\n"
												  "    result += vec4(0, 0, "
				   << float(n_id + 1) * 0.0325f << ", 0);\n"
												   "}\n"
												   "\n"
												   "subroutine uniform SubroutineTEType function;\n"
												   "\n"
												   /* Input block */
												   "in TC_DATA\n"
												   "{\n"
												   "    vec4 data;\n"
												   "} in_tc[];\n"
												   "\n"
												   /* Output block */
												   "out TE_DATA\n"
												   "{\n"
												   "    vec4 data;\n"
												   "} out_te;\n"
												   "\n"
												   "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
												   "out gl_PerVertex { vec4 gl_Position; };\n"
												   /* main() declaration */
												   "void main()\n"
												   "{\n"
												   "    gl_Position = gl_in[0].gl_Position;\n"
												   "    out_te.data = in_tc[0].data;\n"
												   "\n"
												   "    function(out_te.data);\n"
												   "}\n";

	return result_sstream.str();
}

/** Retrieves body of a vertex shader that should be used for the test.
 *  The subroutine implementations are slightly changed, depending on the
 *  index of the shader, as specified by the caller.
 *
 *  @param n_id Index of the shader.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest13::getVertexShaderBody(unsigned int n_id)
{
	std::stringstream result_sstream;

	/* Pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  /* Sub-routine */
					  "subroutine void SubroutineVSType(inout vec4 result);\n"
					  "\n"
					  "subroutine(SubroutineVSType) void SubroutineVS1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4("
				   << float(n_id + 1) * 0.125f << ", 0, 0, 0);\n"
												  "}\n"
												  "subroutine(SubroutineVSType) void SubroutineVS2(inout vec4 result)\n"
												  "{\n"
												  "    result += vec4("
				   << float(n_id + 1) * 0.0125f << ", 0, 0, 0);\n"
												   "}\n"
												   "\n"
												   "subroutine uniform SubroutineVSType function;\n"
												   "\n"
												   /* Output block */
												   "out VS_DATA\n"
												   "{\n"
												   "    vec4 data;\n"
												   "} out_vs;\n"
												   "\n"
												   "out gl_PerVertex { vec4 gl_Position; };\n"
												   /* main() declaration */
												   "void main()\n"
												   "{\n"
												   "    gl_Position = vec4(0, 0, 0, 1);\n"
												   "    out_vs.data = vec4(0);\n"
												   "\n"
												   "    function(out_vs.data);\n"
												   "\n"
												   "}\n";

	return result_sstream.str();
}

/** Initializes all GL objects required to run the test. Also modifies a few
 *  GL states in order for the test to run correctly.
 **/
void FunctionalTest13::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	/* Make sure no program is used */
	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Generate a pipeline object */
	gl.genProgramPipelines(1, &m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.bindProgramPipeline(m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

	/* Initialize all shader programs */
	for (unsigned int n_id = 0; n_id < 2 /* variants for each shader type */; ++n_id)
	{
		std::string fs_body			= getFragmentShaderBody(n_id);
		const char* fs_body_raw_ptr = fs_body.c_str();
		std::string gs_body			= getGeometryShaderBody(n_id);
		const char* gs_body_raw_ptr = gs_body.c_str();
		std::string tc_body			= getTessellationControlShaderBody(n_id);
		const char* tc_body_raw_ptr = tc_body.c_str();
		std::string te_body			= getTessellationEvaluationShaderBody(n_id);
		const char* te_body_raw_ptr = te_body.c_str();
		std::string vs_body			= getVertexShaderBody(n_id);
		const char* vs_body_raw_ptr = vs_body.c_str();

		m_fs_po_ids[n_id] = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1 /* count */, &fs_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

		m_gs_po_ids[n_id] = gl.createShaderProgramv(GL_GEOMETRY_SHADER, 1 /* count */, &gs_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

		m_tc_po_ids[n_id] = gl.createShaderProgramv(GL_TESS_CONTROL_SHADER, 1 /* count */, &tc_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

		m_te_po_ids[n_id] = gl.createShaderProgramv(GL_TESS_EVALUATION_SHADER, 1 /* count */, &te_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

		m_vs_po_ids[n_id] = gl.createShaderProgramv(GL_VERTEX_SHADER, 1 /* count */, &vs_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

		/* Verify that all shader program objects have been linked successfully */
		const glw::GLuint po_ids[] = {
			m_fs_po_ids[n_id], m_gs_po_ids[n_id], m_tc_po_ids[n_id], m_te_po_ids[n_id], m_vs_po_ids[n_id],
		};
		const unsigned int n_po_ids = sizeof(po_ids) / sizeof(po_ids[0]);

		for (unsigned int n_po_id = 0; n_po_id < n_po_ids; ++n_po_id)
		{
			glw::GLint  link_status = GL_FALSE;
			glw::GLuint po_id		= po_ids[n_po_id];

			gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			if (link_status != GL_TRUE)
			{
				TCU_FAIL("Shader program object linking failed.");
			}
		} /* for (all shader program objects) */
	}	 /* for (both shader program object variants) */

	/* Generate a texture object. We will use the base mip-map as a render-target */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA32F, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");

	/* Generate and configure a FBO we will use for the draw call */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Generate & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set up tessellation */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");

	/* Set up pixel storage alignment */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed.");

	/* Allocate enough space to hold color attachment data */
	m_read_buffer = (unsigned char*)new float[m_to_width * m_to_height * 4 /* rgba */];
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest13::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine and GL_ARB_separate_shader_objects
	 * are not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_separate_shader_objects"))
	{
		throw tcu::NotSupportedError("GL_ARB_separate_shader_objects is not supported");
	}

	/* Initialize all GL objects before we continue */
	initTest();

	/* Iterate over all possible FS/GS/TC/TE/VS permutations */
	for (int n_shader_permutation = 0; n_shader_permutation < 32 /* 2^5 */; ++n_shader_permutation)
	{
		const unsigned int n_fs_idx = ((n_shader_permutation & (1 << 0)) != 0) ? 1 : 0;
		const unsigned int n_gs_idx = ((n_shader_permutation & (1 << 1)) != 0) ? 1 : 0;
		const unsigned int n_tc_idx = ((n_shader_permutation & (1 << 2)) != 0) ? 1 : 0;
		const unsigned int n_te_idx = ((n_shader_permutation & (1 << 3)) != 0) ? 1 : 0;
		const unsigned int n_vs_idx = ((n_shader_permutation & (1 << 4)) != 0) ? 1 : 0;
		const unsigned int fs_po_id = m_fs_po_ids[n_fs_idx];
		const unsigned int gs_po_id = m_gs_po_ids[n_gs_idx];
		const unsigned int tc_po_id = m_tc_po_ids[n_tc_idx];
		const unsigned int te_po_id = m_te_po_ids[n_te_idx];
		const unsigned int vs_po_id = m_vs_po_ids[n_vs_idx];

		/* Configure fragment shader stage */
		gl.useProgramStages(m_pipeline_id, GL_FRAGMENT_SHADER_BIT, fs_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed for GL_FRAGMENT_SHADER_BIT bit");

		/* Configure geometry shader stage */
		gl.useProgramStages(m_pipeline_id, GL_GEOMETRY_SHADER_BIT, gs_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed for GL_GEOMETRY_SHADER_BIT bit");

		/* Configure tessellation control shader stage */
		gl.useProgramStages(m_pipeline_id, GL_TESS_CONTROL_SHADER_BIT, tc_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed for GL_TESS_CONTROL_SHADER_BIT bit");

		/* Configure tessellation evaluation shader stage */
		gl.useProgramStages(m_pipeline_id, GL_TESS_EVALUATION_SHADER_BIT, te_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed for GL_TESS_EVALUATION_SHADER_BIT bit");

		/* Configure vertex shader stage */
		gl.useProgramStages(m_pipeline_id, GL_VERTEX_SHADER_BIT, vs_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed for GL_VERTEX_SHADER_BIT bit");

		/* Validate the pipeline */
		glw::GLint validate_status = GL_FALSE;

		gl.validateProgramPipeline(m_pipeline_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgramPipeline() call failed.");

		gl.getProgramPipelineiv(m_pipeline_id, GL_VALIDATE_STATUS, &validate_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() call failed.");

		if (validate_status != GL_TRUE)
		{
			TCU_FAIL("Program pipeline has not been validated successfully.");
		}

		/* Retrieve subroutine indices */
		GLuint fs_subroutine_indices[2]	= { (GLuint)-1 };
		GLint  fs_subroutine_uniform_index = 0;
		GLuint gs_subroutine_indices[2]	= { (GLuint)-1 };
		GLint  gs_subroutine_uniform_index = 0;
		GLuint tc_subroutine_indices[2]	= { (GLuint)-1 };
		GLint  tc_subroutine_uniform_index = 0;
		GLuint te_subroutine_indices[2]	= { (GLuint)-1 };
		GLint  te_subroutine_uniform_index = 0;
		GLuint vs_subroutine_indices[2]	= { (GLuint)-1 };
		GLint  vs_subroutine_uniform_index = 0;

		for (unsigned int n_subroutine = 0; n_subroutine < 2; ++n_subroutine)
		{
			std::stringstream fs_subroutine_name_sstream;
			std::stringstream gs_subroutine_name_sstream;
			std::stringstream tc_subroutine_name_sstream;
			std::stringstream te_subroutine_name_sstream;
			std::stringstream vs_subroutine_name_sstream;

			fs_subroutine_name_sstream << "SubroutineFS" << (n_subroutine + 1);
			gs_subroutine_name_sstream << "SubroutineGS" << (n_subroutine + 1);
			tc_subroutine_name_sstream << "SubroutineTC" << (n_subroutine + 1);
			te_subroutine_name_sstream << "SubroutineTE" << (n_subroutine + 1);
			vs_subroutine_name_sstream << "SubroutineVS" << (n_subroutine + 1);

			fs_subroutine_indices[n_subroutine] =
				gl.getSubroutineIndex(fs_po_id, GL_FRAGMENT_SHADER, fs_subroutine_name_sstream.str().c_str());
			gs_subroutine_indices[n_subroutine] =
				gl.getSubroutineIndex(gs_po_id, GL_GEOMETRY_SHADER, gs_subroutine_name_sstream.str().c_str());
			tc_subroutine_indices[n_subroutine] =
				gl.getSubroutineIndex(tc_po_id, GL_TESS_CONTROL_SHADER, tc_subroutine_name_sstream.str().c_str());
			te_subroutine_indices[n_subroutine] =
				gl.getSubroutineIndex(te_po_id, GL_TESS_EVALUATION_SHADER, te_subroutine_name_sstream.str().c_str());
			vs_subroutine_indices[n_subroutine] =
				gl.getSubroutineIndex(vs_po_id, GL_VERTEX_SHADER, vs_subroutine_name_sstream.str().c_str());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() call(s) failed.");

			if (fs_subroutine_indices[n_subroutine] == (GLuint)-1 ||
				gs_subroutine_indices[n_subroutine] == (GLuint)-1 ||
				tc_subroutine_indices[n_subroutine] == (GLuint)-1 ||
				te_subroutine_indices[n_subroutine] == (GLuint)-1 || vs_subroutine_indices[n_subroutine] == (GLuint)-1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "At least one subroutine was not recognized by glGetSubroutineIndex() call. "
									  "(fs:"
								   << fs_subroutine_indices[n_subroutine]
								   << ", gs:" << gs_subroutine_indices[n_subroutine]
								   << ", tc:" << tc_subroutine_indices[n_subroutine]
								   << ", te:" << te_subroutine_indices[n_subroutine]
								   << ", vs:" << vs_subroutine_indices[n_subroutine] << ")."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("At least one subroutine was not recognized");
			}
		} /* for (both subroutines) */

		/* Retrieve subroutine uniform indices */
		fs_subroutine_uniform_index = gl.getSubroutineUniformLocation(fs_po_id, GL_FRAGMENT_SHADER, "function");
		gs_subroutine_uniform_index = gl.getSubroutineUniformLocation(gs_po_id, GL_GEOMETRY_SHADER, "function");
		tc_subroutine_uniform_index = gl.getSubroutineUniformLocation(tc_po_id, GL_TESS_CONTROL_SHADER, "function");
		te_subroutine_uniform_index = gl.getSubroutineUniformLocation(te_po_id, GL_TESS_EVALUATION_SHADER, "function");
		vs_subroutine_uniform_index = gl.getSubroutineUniformLocation(vs_po_id, GL_VERTEX_SHADER, "function");

		if (fs_subroutine_uniform_index == -1 || gs_subroutine_uniform_index == -1 ||
			tc_subroutine_uniform_index == -1 || te_subroutine_uniform_index == -1 || vs_subroutine_uniform_index == -1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "At least one subroutine uniform is considered inactive by "
														   "glGetSubroutineUniformLocation ("
														   "fs:"
							   << fs_subroutine_uniform_index << ", gs:" << gs_subroutine_uniform_index
							   << ", tc:" << tc_subroutine_uniform_index << ", te:" << te_subroutine_uniform_index
							   << ", vs:" << vs_subroutine_uniform_index << ")." << tcu::TestLog::EndMessage;

			TCU_FAIL("At least one subroutine uniform is considered inactive");
		}

		/* Check if both subroutines work correctly in each stage */
		for (int n_subroutine_permutation = 0; n_subroutine_permutation < 32; /* 2^5 */
			 ++n_subroutine_permutation)
		{
			unsigned int n_fs_subroutine = ((n_subroutine_permutation & (1 << 0)) != 0) ? 1 : 0;
			unsigned int n_gs_subroutine = ((n_subroutine_permutation & (1 << 1)) != 0) ? 1 : 0;
			unsigned int n_tc_subroutine = ((n_subroutine_permutation & (1 << 2)) != 0) ? 1 : 0;
			unsigned int n_te_subroutine = ((n_subroutine_permutation & (1 << 3)) != 0) ? 1 : 0;
			unsigned int n_vs_subroutine = ((n_subroutine_permutation & (1 << 4)) != 0) ? 1 : 0;

			/* Configure subroutine uniforms */
			struct
			{
				glw::GLenum  stage;
				glw::GLuint  po_id;
				glw::GLuint* indices;
			} configurations[] = {
				{ GL_FRAGMENT_SHADER, fs_po_id, fs_subroutine_indices + n_fs_subroutine },
				{ GL_GEOMETRY_SHADER, gs_po_id, gs_subroutine_indices + n_gs_subroutine },
				{ GL_TESS_CONTROL_SHADER, tc_po_id, tc_subroutine_indices + n_tc_subroutine },
				{ GL_TESS_EVALUATION_SHADER, te_po_id, te_subroutine_indices + n_te_subroutine },
				{ GL_VERTEX_SHADER, vs_po_id, vs_subroutine_indices + n_vs_subroutine },
			};

			for (int i = 0; i < 5; ++i)
			{
				gl.activeShaderProgram(m_pipeline_id, configurations[i].po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveShaderProgram() call failed.");

				gl.uniformSubroutinesuiv(configurations[i].stage, 1 /* count */, configurations[i].indices);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() call failed.");
			}

			/* Render a full-screen quad with the pipeline */
			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

			gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

			/* Read color attachment's contents */
			gl.readPixels(0, /* x */
						  0, /* y */
						  m_to_width, m_to_height, GL_RGBA, GL_FLOAT, m_read_buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

			/* Verify the contents */
			verifyReadBuffer(n_fs_idx, n_fs_subroutine, n_gs_idx, n_gs_subroutine, n_tc_idx, n_tc_subroutine, n_te_idx,
							 n_te_subroutine, n_vs_idx, n_vs_subroutine);
		} /* for (all subroutine permutations) */
	}	 /* for (all program shader object permutations) */

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies the data that have been rendered using a pipeline object.
 *  Contents of the data depends on indices of the shaders, as well as
 *  on the subroutines that have been activated for particular iteration.
 *
 *  @param n_fs_id         Index of the fragment shader used for the iteration;
 *  @param n_fs_subroutine Index of the subroutine used in the fragment shader
 *                         for the iteration;
 *  @param n_gs_id         Index of the geometry shader used for the iteration;
 *  @param n_gs_subroutine Index of the subroutine used in the geometry shader
 *                         for the iteration;
 *  @param n_tc_id         Index of the tessellation control shader used for the iteration;
 *  @param n_tc_subroutine Index of the subroutine used in the tessellation control
 *                         shader for the iteration;
 *  @param n_te_id         Index of the tessellation evaluation shader used for the iteration;
 *  @param n_te_subroutine Index of the subroutine used in the tessellation evaluation
 *                         shader for the iteration;
 *  @param n_vs_id         Index of the vertex shader used for the iteration;
 *  @param n_vs_subroutine Index of the subroutine used in the vertex shader for
 *                         the iteration.
 */
void FunctionalTest13::verifyReadBuffer(unsigned int n_fs_id, unsigned int n_fs_subroutine, unsigned int n_gs_id,
										unsigned int n_gs_subroutine, unsigned int n_tc_id,
										unsigned int n_tc_subroutine, unsigned int n_te_id,
										unsigned int n_te_subroutine, unsigned int n_vs_id,
										unsigned int n_vs_subroutine)
{
	float expected_color[4] = { 0 };
	float fs_modifier[4]	= { 0 };
	float gs_modifier[4]	= { 0 };
	float tc_modifier[4]	= { 0 };
	float te_modifier[4]	= { 0 };
	float vs_modifier[4]	= { 0 };

	if (n_fs_subroutine == 0)
	{
		for (unsigned int n_component = 0; n_component < 4; ++n_component)
		{
			fs_modifier[n_component] = float(n_fs_id + n_component + 1) / 10.0f;
		}
	}
	else
	{
		for (unsigned int n_component = 0; n_component < 4; ++n_component)
		{
			fs_modifier[n_component] = float(n_fs_id + n_component + 1) / 20.0f;
		}
	}

	if (n_gs_subroutine == 0)
	{
		gs_modifier[3] = float(n_gs_id + 1) * 0.425f;
	}
	else
	{
		gs_modifier[3] = float(n_gs_id + 1) * 0.0425f;
	}

	if (n_tc_subroutine == 0)
	{
		tc_modifier[1] = float(n_tc_id + 1) * 0.25f;
	}
	else
	{
		tc_modifier[1] = float(n_tc_id + 1) * 0.025f;
	}

	if (n_te_subroutine == 0)
	{
		te_modifier[2] = float(n_te_id + 1) * 0.325f;
	}
	else
	{
		te_modifier[2] = float(n_te_id + 1) * 0.0325f;
	}

	if (n_vs_subroutine == 0)
	{
		vs_modifier[0] = float(n_vs_id + 1) * 0.125f;
	}
	else
	{
		vs_modifier[0] = float(n_vs_id + 1) * 0.0125f;
	}

	/* Determine the expected color */
	for (unsigned int n_component = 0; n_component < 4 /* rgba */; ++n_component)
	{
		expected_color[n_component] = fs_modifier[n_component] + gs_modifier[n_component] + tc_modifier[n_component] +
									  te_modifier[n_component] + vs_modifier[n_component];
	}

	/* Verify all read texels are valid */
	const float epsilon			= 1e-5f;
	bool		should_continue = true;

	for (unsigned int y = 0; y < m_to_height && should_continue; ++y)
	{
		const float* row_ptr = (const float*)m_read_buffer + y * m_to_width * 4; /* rgba */

		for (unsigned int x = 0; x < m_to_width && should_continue; ++x)
		{
			const float* texel_ptr = row_ptr + x * 4; /* rgba */

			if (de::abs(texel_ptr[0] - expected_color[0]) > epsilon ||
				de::abs(texel_ptr[1] - expected_color[1]) > epsilon ||
				de::abs(texel_ptr[2] - expected_color[2]) > epsilon ||
				de::abs(texel_ptr[3] - expected_color[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texel rendered at (" << x << ", " << y
								   << ") for "
									  "the following configuration: "
									  "n_fs_id:"
								   << n_fs_id << " n_fs_subroutine:" << n_fs_subroutine << " n_gs_id:" << n_gs_id
								   << " n_gs_subroutine:" << n_gs_subroutine << " n_tc_id:" << n_tc_id
								   << " n_tc_subroutine:" << n_tc_subroutine << " n_te_id:" << n_te_id
								   << " n_te_subroutine:" << n_te_subroutine << " n_vs_id:" << n_vs_id
								   << " n_vs_subroutine:" << n_vs_subroutine << "; expected:"
																				"("
								   << expected_color[0] << ", " << expected_color[1] << ", " << expected_color[2]
								   << ", " << expected_color[3] << "), found:"
																   "("
								   << texel_ptr[0] << ", " << texel_ptr[1] << ", " << texel_ptr[2] << ", "
								   << texel_ptr[3] << ")." << tcu::TestLog::EndMessage;

				m_has_test_passed = false;
				should_continue   = false;
			}
		} /* for (all columns) */
	}	 /* for (all rows) */
}

/** Constructor
 *
 * @param context CTS context
 **/
FunctionalTest14_15::FunctionalTest14_15(deqp::Context& context)
	: TestCase(context, "structure_parameters_program_binary", "Verify structures can be used as parameters")
	, m_uniform_location(0)
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalTest14_15::iterate()
{
	static const GLchar* vertex_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"struct data\n"
		"{\n"
		"    uint r;\n"
		"    uint g;\n"
		"    uint b;\n"
		"    uint a;\n"
		"};\n"
		"\n"
		"subroutine void routine_type_1(in data iparam, out data oparam);\n"
		"subroutine void routine_type_2(inout data arg);\n"
		"\n"
		"subroutine (routine_type_1) void invert(in data iparam, out data oparam)\n"
		"{\n"
		"    oparam.r = iparam.a;\n"
		"    oparam.g = iparam.b;\n"
		"    oparam.b = iparam.g;\n"
		"    oparam.a = iparam.r;\n"
		"}\n"
		"\n"
		"subroutine (routine_type_1) void increment(in data iparam, out data oparam)\n"
		"{\n"
		"    oparam.r = 1 + iparam.r;\n"
		"    oparam.g = 1 + iparam.g;\n"
		"    oparam.b = 1 + iparam.b;\n"
		"    oparam.a = 1 + iparam.a;\n"
		"}\n"
		"\n"
		"subroutine (routine_type_2) void div_by_2(inout data arg)\n"
		"{\n"
		"    arg.r = arg.r / 2;\n"
		"    arg.g = arg.g / 2;\n"
		"    arg.b = arg.b / 2;\n"
		"    arg.a = arg.a / 2;\n"
		"}\n"
		"\n"
		"subroutine (routine_type_2) void decrement(inout data arg)\n"
		"{\n"
		"    arg.r = arg.r - 1;\n"
		"    arg.g = arg.g - 1;\n"
		"    arg.b = arg.b - 1;\n"
		"    arg.a = arg.a - 1;\n"
		"}\n"
		"\n"
		"subroutine uniform routine_type_1 routine_1;\n"
		"subroutine uniform routine_type_2 routine_2;\n"
		"\n"
		"uniform uvec4 uni_input;\n"
		"\n"
		"out uvec4 out_routine_1;\n"
		"out uvec4 out_routine_2;\n"
		"\n"
		"\n"
		"void main()\n"
		"{\n"
		"    data routine_1_input;\n"
		"    data routine_1_output;\n"
		"    data routine_2_arg;\n"
		"\n"
		"    routine_1_input.r = uni_input.r;\n"
		"    routine_1_input.g = uni_input.g;\n"
		"    routine_1_input.b = uni_input.b;\n"
		"    routine_1_input.a = uni_input.a;\n"
		"\n"
		"    routine_2_arg.r = uni_input.r;\n"
		"    routine_2_arg.g = uni_input.g;\n"
		"    routine_2_arg.b = uni_input.b;\n"
		"    routine_2_arg.a = uni_input.a;\n"
		"\n"
		"    routine_1(routine_1_input, routine_1_output);\n"
		"    routine_2(routine_2_arg);\n"
		"\n"
		"    out_routine_1.r = routine_1_output.r;\n"
		"    out_routine_1.g = routine_1_output.g;\n"
		"    out_routine_1.b = routine_1_output.b;\n"
		"    out_routine_1.a = routine_1_output.a;\n"
		"\n"
		"    out_routine_2.r = routine_2_arg.r;\n"
		"    out_routine_2.g = routine_2_arg.g;\n"
		"    out_routine_2.b = routine_2_arg.b;\n"
		"    out_routine_2.a = routine_2_arg.a;\n"
		"}\n"
		"\n";

	static const GLchar* subroutine_names[][2] = { { "invert", "increment" }, { "div_by_2", "decrement" } };
	static const GLuint  n_subroutine_types	= sizeof(subroutine_names) / sizeof(subroutine_names[0]);

	static const GLchar* subroutine_uniform_names[] = { "routine_1", "routine_2" };
	static const GLuint  n_subroutine_uniform_names =
		sizeof(subroutine_uniform_names) / sizeof(subroutine_uniform_names[0]);

	static const GLchar* uniform_name	= "uni_input";
	static const GLchar* varying_names[] = { "out_routine_1", "out_routine_2" };

	static const GLuint n_varying_names				   = sizeof(varying_names) / sizeof(varying_names[0]);
	static const GLuint transform_feedback_buffer_size = n_varying_names * 4 * sizeof(GLuint);

	/* Test data */
	static const Utils::vec4<GLuint> uni_input[] = { Utils::vec4<GLuint>(8, 64, 4096, 16777216),
													 Utils::vec4<GLuint>(8, 64, 4096, 16777216) };

	static const Utils::vec4<GLuint> out_routine_1[] = { Utils::vec4<GLuint>(16777216, 4096, 64, 8),
														 Utils::vec4<GLuint>(9, 65, 4097, 16777217) };

	static const Utils::vec4<GLuint> out_routine_2[] = { Utils::vec4<GLuint>(4, 32, 2048, 8388608),
														 Utils::vec4<GLuint>(7, 63, 4095, 16777215) };

	static const GLuint n_test_cases = 2;

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* GL objects */
	Utils::program	 program(m_context);
	Utils::buffer	  transform_feedback_buffer(m_context);
	Utils::vertexArray vao(m_context);

	bool is_program_binary_supported = program.isProgramBinarySupported();

	/* Init GL objects */
	program.build(0 /* cs */, 0 /* fs */, 0 /* gs */, 0 /* tcs */, 0 /* test */, vertex_shader_code,
				  varying_names /* varying_names */, n_varying_names /* n_varyings */);

	/* Do not execute the test if GL_ARB_get_program_binary is not supported */
	if (true == is_program_binary_supported)
	{
		/* Get subroutine indices */
		for (GLuint type = 0; type < n_subroutine_types; ++type)
		{
			m_initial_subroutine_indices[type][0] =
				program.getSubroutineIndex(subroutine_names[type][0], GL_VERTEX_SHADER);

			m_initial_subroutine_indices[type][1] =
				program.getSubroutineIndex(subroutine_names[type][1], GL_VERTEX_SHADER);
		}

		/* Get subroutine uniform locations */
		for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
		{
			m_initial_subroutine_uniform_locations[uniform] =
				program.getSubroutineUniformLocation(subroutine_uniform_names[uniform], GL_VERTEX_SHADER);
		}

		/* Delete program and recreate it from binary */
		std::vector<GLubyte> program_binary;
		GLenum				 binary_format;

		program.getBinary(program_binary, binary_format);
		program.remove();
		program.createFromBinary(program_binary, binary_format);
	}

	program.use();

	vao.generate();
	vao.bind();

	transform_feedback_buffer.generate();
	transform_feedback_buffer.update(GL_TRANSFORM_FEEDBACK_BUFFER, transform_feedback_buffer_size, 0 /* data */,
									 GL_DYNAMIC_COPY);
	transform_feedback_buffer.bindRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0, transform_feedback_buffer_size);

	/* Get subroutine indices */
	for (GLuint type = 0; type < n_subroutine_types; ++type)
	{
		m_subroutine_indices[type][0] = program.getSubroutineIndex(subroutine_names[type][0], GL_VERTEX_SHADER);
		m_subroutine_indices[type][1] = program.getSubroutineIndex(subroutine_names[type][1], GL_VERTEX_SHADER);
	}

	/* Get subroutine uniform locations */
	for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
	{
		m_subroutine_uniform_locations[uniform] =
			program.getSubroutineUniformLocation(subroutine_uniform_names[uniform], GL_VERTEX_SHADER);
	}

	/* Get uniform locations */
	m_uniform_location = program.getUniformLocation(uniform_name);

	/* Test */
	bool result = true;

	/* Test program binary */
	if (true == is_program_binary_supported)
	{
		/* Test indices and locations */
		if (false == testIndicesAndLocations())
		{
			static const GLuint n_subroutines_per_type = 2;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Error. Subroutine indices or subroutine uniform location changed."
												<< tcu::TestLog::EndMessage;

			for (GLuint type = 0; type < n_subroutine_types; ++type)
			{
				for (GLuint i = 0; i < n_subroutines_per_type; ++i)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Subroutine: " << subroutine_names[type][i]
						<< " index: " << m_subroutine_indices[type][i]
						<< " initial index: " << m_initial_subroutine_indices[type][i] << tcu::TestLog::EndMessage;
				}
			}

			for (GLuint uniform = 0; uniform < n_subroutine_uniform_names; ++uniform)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Subroutine uniform: " << subroutine_uniform_names[uniform]
					<< " location: " << m_subroutine_uniform_locations[uniform]
					<< " initial location: " << m_initial_subroutine_uniform_locations[uniform]
					<< tcu::TestLog::EndMessage;
			}

			result = false;
		}

		/* Test draw with deafult set of subroutines */
		if (false == testDefaultSubroutineSet(uni_input[0], out_routine_1, out_routine_2))
		{
			result = false;
		}
	}

	for (GLuint i = 0; i < n_test_cases; ++i)
	{
		if (false == testDraw(i, uni_input[i], out_routine_1[i], out_routine_2[i]))
		{
			result = false;
		}
	}

	/* Set result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Execute draw call and verify results
 *
 * @param uni_input                 Input data
 * @param expected_routine_1_result Set of expected results of "routine_1"
 * @param expected_routine_2_result Set of expected results of "routine_2"
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest14_15::testDefaultSubroutineSet(const Utils::vec4<glw::GLuint>& uni_input,
												   const Utils::vec4<glw::GLuint>  expected_routine_1_result[2],
												   const Utils::vec4<glw::GLuint>  expected_routine_2_result[2]) const
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Set up input data uniforms */
	gl.uniform4ui(m_uniform_location, uni_input.m_x, uni_input.m_y, uni_input.m_z, uni_input.m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Capture results */
	GLuint* feedback_data = (GLuint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	Utils::vec4<GLuint> routine_1_result;
	Utils::vec4<GLuint> routine_2_result;

	routine_1_result.m_x = feedback_data[0 + 0];
	routine_1_result.m_y = feedback_data[0 + 1];
	routine_1_result.m_z = feedback_data[0 + 2];
	routine_1_result.m_w = feedback_data[0 + 3];

	routine_2_result.m_x = feedback_data[4 + 0];
	routine_2_result.m_y = feedback_data[4 + 1];
	routine_2_result.m_z = feedback_data[4 + 2];
	routine_2_result.m_w = feedback_data[4 + 3];

	/* Unmap buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Verifiy */
	result = result &&
			 ((routine_1_result == expected_routine_1_result[0]) || (routine_1_result == expected_routine_1_result[1]));

	result = result &&
			 ((routine_2_result == expected_routine_2_result[0]) || (routine_2_result == expected_routine_2_result[1]));

	/* Log error if any */
	if (false == result)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
											<< tcu::TestLog::EndMessage;

		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Routine_1, result: ";

		routine_1_result.log(message);

		message << "Routine_2, result: ";

		routine_2_result.log(message);

		message << tcu::TestLog::EndMessage;
	}

	/* Done */
	return result;
}

/** Execute draw call and verify results
 *
 * @param routine_configuration     Subroutine "type" ordinal
 * @param uni_input                 Input data
 * @param expected_routine_1_result Expected results of "routine_1"
 * @param expected_routine_2_result Expected results of "routine_2"
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest14_15::testDraw(glw::GLuint routine_configuration, const Utils::vec4<glw::GLuint>& uni_input,
								   const Utils::vec4<glw::GLuint>& expected_routine_1_result,
								   const Utils::vec4<glw::GLuint>& expected_routine_2_result) const
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;
	GLuint				  subroutine_indices[2];
	static const GLuint   n_subroutine_uniforms = sizeof(subroutine_indices) / sizeof(subroutine_indices[0]);

	/* Set up input data uniforms */
	gl.uniform4ui(m_uniform_location, uni_input.m_x, uni_input.m_y, uni_input.m_z, uni_input.m_w);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform4f");

	/* Prepare subroutine uniform data */
	for (GLuint i = 0; i < n_subroutine_uniforms; ++i)
	{
		const GLuint location = m_subroutine_uniform_locations[i];

		subroutine_indices[location] = m_subroutine_indices[i][routine_configuration];
	}

	/* Set up subroutine uniforms */
	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, n_subroutine_uniforms, &subroutine_indices[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Execute draw call with transform feedback */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BeginTransformFeedback");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "EndTransformFeedback");

	/* Capture results */
	GLuint* feedback_data = (GLuint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	Utils::vec4<GLuint> routine_1_result;
	Utils::vec4<GLuint> routine_2_result;

	routine_1_result.m_x = feedback_data[0 + 0];
	routine_1_result.m_y = feedback_data[0 + 1];
	routine_1_result.m_z = feedback_data[0 + 2];
	routine_1_result.m_w = feedback_data[0 + 3];

	routine_2_result.m_x = feedback_data[4 + 0];
	routine_2_result.m_y = feedback_data[4 + 1];
	routine_2_result.m_z = feedback_data[4 + 2];
	routine_2_result.m_w = feedback_data[4 + 3];

	/* Unmap buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Verifiy */
	result = result && (routine_1_result == expected_routine_1_result);
	result = result && (routine_2_result == expected_routine_2_result);

	/* Log error if any */
	if (false == result)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Error. Invalid result."
											<< tcu::TestLog::EndMessage;

		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Routine_1, result: ";

		routine_1_result.log(message);

		message << ", expected: ";

		expected_routine_1_result.log(message);

		message << "Routine_2, result: ";

		routine_2_result.log(message);

		message << ", expected: ";

		expected_routine_2_result.log(message);

		message << tcu::TestLog::EndMessage;
	}

	/* Done */
	return result;
}

/** Verify initial and current values of subroutine indices and subroutines uniform locations
 *
 * @return true if test pass, false otherwise
 **/
bool FunctionalTest14_15::testIndicesAndLocations() const
{
	static const GLuint n_subroutine_types = 2;
	bool				result			   = true;

	/* Verify subroutine indices */
	for (GLuint type = 0; type < n_subroutine_types; ++type)
	{
		result = result && (m_subroutine_indices[type][0] == m_initial_subroutine_indices[type][0]);
		result = result && (m_subroutine_indices[type][1] == m_initial_subroutine_indices[type][1]);
	}

	/* Verify subroutine uniform locations */
	for (GLuint uniform = 0; uniform < n_subroutine_types; ++uniform)
	{
		result = result && (m_subroutine_uniform_locations[uniform] == m_initial_subroutine_uniform_locations[uniform]);
	}

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest16::FunctionalTest16(deqp::Context& context)
	: TestCase(context, "subroutine_uniform_reset",
			   "Checks that when the active program for a shader stage is re-linke or "
			   "changed by a call to UseProgram, BindProgramPipeline, or UseProgramStages,"
			   " subroutine uniforms for that stage are reset to arbitrarily chosen default "
			   "functions with compatible subroutine types.")
	, m_are_pipeline_objects_supported(false)
	, m_has_test_passed(true)
{
	memset(m_fs_ids, 0, sizeof(m_fs_ids));
	memset(m_gs_ids, 0, sizeof(m_gs_ids));
	memset(m_po_ids, 0, sizeof(m_po_ids));
	memset(m_tc_ids, 0, sizeof(m_tc_ids));
	memset(m_te_ids, 0, sizeof(m_te_ids));
	memset(m_vs_ids, 0, sizeof(m_vs_ids));

	memset(m_fs_po_ids, 0, sizeof(m_fs_po_ids));
	memset(m_gs_po_ids, 0, sizeof(m_gs_po_ids));
	memset(m_pipeline_object_ids, 0, sizeof(m_pipeline_object_ids));
	memset(m_tc_po_ids, 0, sizeof(m_tc_po_ids));
	memset(m_te_po_ids, 0, sizeof(m_te_po_ids));
	memset(m_vs_po_ids, 0, sizeof(m_vs_po_ids));
}

/** Deinitializes all GL objects that may have been created during test execution. */
void FunctionalTest16::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int n_id = 0; n_id < 2; ++n_id)
	{
		if (m_fs_ids[n_id] != 0)
		{
			gl.deleteShader(m_fs_ids[n_id]);

			m_fs_ids[n_id] = 0;
		}

		if (m_fs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_fs_po_ids[n_id]);

			m_fs_po_ids[n_id] = 0;
		}

		if (m_gs_ids[n_id] != 0)
		{
			gl.deleteShader(m_gs_ids[n_id]);

			m_gs_ids[n_id] = 0;
		}

		if (m_gs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_gs_po_ids[n_id]);

			m_gs_po_ids[n_id] = 0;
		}

		if (m_pipeline_object_ids[n_id] != 0)
		{
			gl.deleteProgramPipelines(1 /* n */, m_pipeline_object_ids + n_id);
		}

		if (m_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_po_ids[n_id]);

			m_po_ids[n_id] = 0;
		}

		if (m_tc_ids[n_id] != 0)
		{
			gl.deleteShader(m_tc_ids[n_id]);

			m_tc_ids[n_id] = 0;
		}

		if (m_tc_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_tc_po_ids[n_id]);

			m_tc_po_ids[n_id] = 0;
		}

		if (m_te_ids[n_id] != 0)
		{
			gl.deleteShader(m_te_ids[n_id]);

			m_te_ids[n_id] = 0;
		}

		if (m_te_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_te_po_ids[n_id]);

			m_te_po_ids[n_id] = 0;
		}

		if (m_vs_ids[n_id] != 0)
		{
			gl.deleteShader(m_vs_ids[n_id]);

			m_vs_ids[n_id] = 0;
		}

		if (m_vs_po_ids[n_id] != 0)
		{
			gl.deleteProgram(m_vs_po_ids[n_id]);

			m_vs_po_ids[n_id] = 0;
		}
	} /* for (both IDs) */
}

/** Retrieves body of a shader that should be used for user-specified shader stage.
 *  This function returns slightly different implementations, depending on index of
 *  the program/pipeline object the shader will be used for.
 *
 *  @param shader_stage Stage the shader body is to be returned for.
 *  @param n_id         Index of the shader (as per description).
 *
 *  @return Requested string.
 **/
std::string FunctionalTest16::getShaderBody(const Utils::_shader_stage& shader_stage, const unsigned int& n_id) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	switch (shader_stage)
	{
	case Utils::SHADER_STAGE_VERTEX:
	{
		result_sstream << "out gl_PerVertex { vec4 gl_Position; } ;\n";
		break;
	}
	case Utils::SHADER_STAGE_GEOMETRY:
	{
		result_sstream << "layout(points)                   in;\n"
						  "layout(points, max_vertices = 1) out;\n";
		result_sstream << "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n";
		result_sstream << "out gl_PerVertex { vec4 gl_Position; } ;\n";
		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
	{
		result_sstream << "layout(vertices = 4) out;\n";
		result_sstream << "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n";
		result_sstream << "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n";
		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
	{
		result_sstream << "layout(quads) in;\n";
		result_sstream << "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n";
		result_sstream << "out gl_PerVertex { vec4 gl_Position; };\n";
		break;
	}

	default:
		break;
	} /* switch (shader_stage) */

	result_sstream << "\n"
					  "subroutine void subroutineType (inout vec4 result);\n"
					  "subroutine vec4 subroutineType2(in    vec4 data);\n"
					  "\n"
					  "subroutine(subroutineType) void function1(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4("
				   << (n_id + 1) << ", " << (n_id + 2) << ", " << (n_id + 3) << ", " << (n_id + 4)
				   << ");\n"
					  "}\n"
					  "subroutine(subroutineType) void function2(inout vec4 result)\n"
					  "{\n"
					  "    result += vec4("
				   << (n_id + 2) << ", " << (n_id + 3) << ", " << (n_id + 4) << ", " << (n_id + 5)
				   << ");\n"
					  "}\n"
					  "\n"
					  "subroutine(subroutineType2) vec4 function3(in vec4 data)\n"
					  "{\n"
					  "    return data * data;\n"
					  "}\n"
					  "subroutine(subroutineType2) vec4 function4(in vec4 data)\n"
					  "{\n"
					  "    return data + data;\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType  subroutine1;\n"
					  "subroutine uniform subroutineType  subroutine2;\n"
					  "subroutine uniform subroutineType2 subroutine3;\n"
					  "subroutine uniform subroutineType2 subroutine4;\n"
					  "\n";

	if (shader_stage == Utils::SHADER_STAGE_FRAGMENT)
	{
		result_sstream << "out vec4 result;\n";
	}

	result_sstream << "void main()\n"
					  "{\n";

	switch (shader_stage)
	{
	case Utils::SHADER_STAGE_FRAGMENT:
	{
		result_sstream << "    result = vec4(0);\n"
					   << "    subroutine1(result);\n"
						  "    subroutine2(result);\n"
						  "    result = subroutine3(result) + subroutine4(result);\n";

		break;
	}

	case Utils::SHADER_STAGE_GEOMETRY:
	{
		result_sstream << "    gl_Position = vec4(0);\n"
						  "    subroutine1(gl_Position);\n"
						  "    subroutine2(gl_Position);\n"
						  "    gl_Position = subroutine3(gl_Position) + subroutine4(gl_Position);\n"
						  "    EmitVertex();\n";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
	{
		result_sstream << "    gl_out[gl_InvocationID].gl_Position = vec4(0);\n"
						  "    subroutine1(gl_out[gl_InvocationID].gl_Position);\n"
						  "    subroutine2(gl_out[gl_InvocationID].gl_Position);\n"
						  "    gl_out[gl_InvocationID].gl_Position = subroutine3(gl_in[0].gl_Position) + "
						  "subroutine4(gl_in[0].gl_Position);\n";

		break;
	}

	case Utils::SHADER_STAGE_VERTEX:
	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
	{
		result_sstream << "    gl_Position = vec4(0);\n"
						  "    subroutine1(gl_Position);\n"
						  "    subroutine2(gl_Position);\n"
						  "    gl_Position = subroutine3(gl_Position) + subroutine4(gl_Position);\n";

		break;
	}

	default:
		break;
	} /* switch (shader_stage) */

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Initializes all objects required to run the test. */
void FunctionalTest16::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int n_id = 0; n_id < 2 /* test program/shader objects */; ++n_id)
	{
		const std::string fs_body = getShaderBody(Utils::SHADER_STAGE_FRAGMENT, n_id);
		const std::string gs_body = getShaderBody(Utils::SHADER_STAGE_GEOMETRY, n_id);
		const std::string tc_body = getShaderBody(Utils::SHADER_STAGE_TESSELLATION_CONTROL, n_id);
		const std::string te_body = getShaderBody(Utils::SHADER_STAGE_TESSELLATION_EVALUATION, n_id);
		const std::string vs_body = getShaderBody(Utils::SHADER_STAGE_VERTEX, n_id);

		if (!Utils::buildProgram(gl, vs_body, tc_body, te_body, gs_body, fs_body, DE_NULL, /* xfb_varyings */
								 DE_NULL,												   /* n_xfb_varyings */
								 m_vs_ids + n_id, m_tc_ids + n_id, m_te_ids + n_id, m_gs_ids + n_id, m_fs_ids + n_id,
								 m_po_ids + n_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Failed to build test program object, index:"
														   "["
							   << n_id << "]" << tcu::TestLog::EndMessage;

			TCU_FAIL("Failed to build a test program");
		}

		if (m_are_pipeline_objects_supported)
		{
			/* Initialize shader program objects */
			const char* fs_body_raw_ptr = fs_body.c_str();
			const char* gs_body_raw_ptr = gs_body.c_str();
			glw::GLint  link_status[5]  = { GL_FALSE };
			const char* tc_body_raw_ptr = tc_body.c_str();
			const char* te_body_raw_ptr = te_body.c_str();
			const char* vs_body_raw_ptr = vs_body.c_str();

			m_fs_po_ids[n_id] = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1 /* count */, &fs_body_raw_ptr);
			m_gs_po_ids[n_id] = gl.createShaderProgramv(GL_GEOMETRY_SHADER, 1 /* count */, &gs_body_raw_ptr);
			m_tc_po_ids[n_id] = gl.createShaderProgramv(GL_TESS_CONTROL_SHADER, 1 /* count */, &tc_body_raw_ptr);
			m_te_po_ids[n_id] = gl.createShaderProgramv(GL_TESS_EVALUATION_SHADER, 1 /* count */, &te_body_raw_ptr);
			m_vs_po_ids[n_id] = gl.createShaderProgramv(GL_VERTEX_SHADER, 1 /* count */, &vs_body_raw_ptr);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

			gl.getProgramiv(m_fs_po_ids[n_id], GL_LINK_STATUS, link_status + 0);
			gl.getProgramiv(m_gs_po_ids[n_id], GL_LINK_STATUS, link_status + 1);
			gl.getProgramiv(m_tc_po_ids[n_id], GL_LINK_STATUS, link_status + 2);
			gl.getProgramiv(m_te_po_ids[n_id], GL_LINK_STATUS, link_status + 3);
			gl.getProgramiv(m_vs_po_ids[n_id], GL_LINK_STATUS, link_status + 4);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			if (link_status[0] == GL_FALSE)
				TCU_FAIL("Fragment shader program failed to link");
			if (link_status[1] == GL_FALSE)
				TCU_FAIL("Geometry shader program failed to link");
			if (link_status[2] == GL_FALSE)
				TCU_FAIL("Tessellation control shader program failed to link");
			if (link_status[3] == GL_FALSE)
				TCU_FAIL("Tessellation evaluation shader program failed to link");
			if (link_status[4] == GL_FALSE)
				TCU_FAIL("Vertex shader program failed to link");

			/* Initialize pipeline program object */
			gl.genProgramPipelines(1 /* n */, m_pipeline_object_ids + n_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

			gl.useProgramStages(m_pipeline_object_ids[n_id], GL_FRAGMENT_SHADER_BIT, m_fs_po_ids[n_id]);
			gl.useProgramStages(m_pipeline_object_ids[n_id], GL_GEOMETRY_SHADER_BIT, m_gs_po_ids[n_id]);
			gl.useProgramStages(m_pipeline_object_ids[n_id], GL_TESS_CONTROL_SHADER_BIT, m_tc_po_ids[n_id]);
			gl.useProgramStages(m_pipeline_object_ids[n_id], GL_TESS_EVALUATION_SHADER_BIT, m_te_po_ids[n_id]);
			gl.useProgramStages(m_pipeline_object_ids[n_id], GL_VERTEX_SHADER_BIT, m_vs_po_ids[n_id]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");
		}

		/* Retrieve subroutine locations */
		struct _item
		{
			glw::GLuint	po_id;
			_shader_stage& stage;
			glw::GLuint	so_id;
			glw::GLenum	so_type;
		} items[] = {
			{ m_po_ids[n_id], m_po_descriptors[n_id].fragment, m_fs_ids[n_id], GL_FRAGMENT_SHADER },
			{ m_po_ids[n_id], m_po_descriptors[n_id].geometry, m_gs_ids[n_id], GL_GEOMETRY_SHADER },
			{ m_po_ids[n_id], m_po_descriptors[n_id].tess_control, m_tc_ids[n_id], GL_TESS_CONTROL_SHADER },
			{ m_po_ids[n_id], m_po_descriptors[n_id].tess_evaluation, m_te_ids[n_id], GL_TESS_EVALUATION_SHADER },
			{ m_po_ids[n_id], m_po_descriptors[n_id].vertex, m_vs_ids[n_id], GL_VERTEX_SHADER },

			{ m_fs_po_ids[n_id], m_fs_po_descriptors[n_id], m_fs_po_ids[n_id], GL_FRAGMENT_SHADER },
			{ m_gs_po_ids[n_id], m_gs_po_descriptors[n_id], m_gs_po_ids[n_id], GL_GEOMETRY_SHADER },
			{ m_tc_po_ids[n_id], m_tc_po_descriptors[n_id], m_tc_po_ids[n_id], GL_TESS_CONTROL_SHADER },
			{ m_te_po_ids[n_id], m_te_po_descriptors[n_id], m_te_po_ids[n_id], GL_TESS_EVALUATION_SHADER },
			{ m_vs_po_ids[n_id], m_vs_po_descriptors[n_id], m_vs_po_ids[n_id], GL_VERTEX_SHADER },
		};
		const unsigned int n_items = sizeof(items) / sizeof(items[0]);

		for (unsigned int n_item = 0; n_item < n_items; ++n_item)
		{
			_item& current_item = items[n_item];

			current_item.stage.function1_index =
				gl.getSubroutineIndex(current_item.po_id, current_item.so_type, "function1");
			current_item.stage.function2_index =
				gl.getSubroutineIndex(current_item.po_id, current_item.so_type, "function2");
			current_item.stage.function3_index =
				gl.getSubroutineIndex(current_item.po_id, current_item.so_type, "function3");
			current_item.stage.function4_index =
				gl.getSubroutineIndex(current_item.po_id, current_item.so_type, "function4");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() call(s) failed.");

			if (current_item.stage.function1_index == GL_INVALID_INDEX ||
				current_item.stage.function2_index == GL_INVALID_INDEX ||
				current_item.stage.function3_index == GL_INVALID_INDEX ||
				current_item.stage.function4_index == GL_INVALID_INDEX)
			{
				TCU_FAIL("Subroutine name was not recognized.");
			}

			current_item.stage.subroutine1_uniform_location =
				gl.getSubroutineUniformLocation(current_item.po_id, current_item.so_type, "subroutine1");
			current_item.stage.subroutine2_uniform_location =
				gl.getSubroutineUniformLocation(current_item.po_id, current_item.so_type, "subroutine2");
			current_item.stage.subroutine3_uniform_location =
				gl.getSubroutineUniformLocation(current_item.po_id, current_item.so_type, "subroutine3");
			current_item.stage.subroutine4_uniform_location =
				gl.getSubroutineUniformLocation(current_item.po_id, current_item.so_type, "subroutine4");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() call(s) failed.");

			if (current_item.stage.subroutine1_uniform_location == -1 ||
				current_item.stage.subroutine2_uniform_location == -1 ||
				current_item.stage.subroutine3_uniform_location == -1 ||
				current_item.stage.subroutine4_uniform_location == -1)
			{
				TCU_FAIL("Subroutine uniform name was not recognized.");
			}

			if (m_po_ids[n_id] == current_item.po_id)
			{
				gl.useProgram(current_item.po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
			}
			else
			{
				/* Temporarily bind the program pipeline. */
				gl.bindProgramPipeline(m_pipeline_object_ids[n_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");
			}

			gl.getUniformSubroutineuiv(current_item.so_type, current_item.stage.subroutine1_uniform_location,
									   &current_item.stage.default_subroutine1_value);
			gl.getUniformSubroutineuiv(current_item.so_type, current_item.stage.subroutine2_uniform_location,
									   &current_item.stage.default_subroutine2_value);
			gl.getUniformSubroutineuiv(current_item.so_type, current_item.stage.subroutine3_uniform_location,
									   &current_item.stage.default_subroutine3_value);
			gl.getUniformSubroutineuiv(current_item.so_type, current_item.stage.subroutine4_uniform_location,
									   &current_item.stage.default_subroutine4_value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformSubroutineuiv() call(s) failed.");

			current_item.stage.gl_stage = current_item.so_type;

			if (m_po_ids[n_id] != current_item.po_id)
			{
				/* Unbind the program pipeline object */
				gl.bindProgramPipeline(0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");
			}
		} /* for (all items) */

		/* Make sure the default subroutine choices are valid. */
		verifySubroutineUniformValues(
			TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT, /* makes the verification routine use program object descriptor */
			n_id, SUBROUTINE_UNIFORMS_SET_TO_VALID_VALUES);

		if (m_are_pipeline_objects_supported)
		{
			gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

			gl.bindProgramPipeline(m_pipeline_object_ids[n_id]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");
			{
				verifySubroutineUniformValues(
					TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_PIPELINE_OBJECT, /* makes the verification routine use pipeline object descriptor */
					n_id, SUBROUTINE_UNIFORMS_SET_TO_VALID_VALUES);
			}
			gl.bindProgramPipeline(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");
		}
	} /* for (both program descriptors) */
}

/** Retrieves IDs of shaders OR shader program objects, depending on which of the two
 *  the caller requests for.
 *
 *  @param retrieve_program_object_shader_ids true if the caller wishes to retrieve shader object IDs,
 *                                            false to return shader program IDs.
 *  @param n_id                               Index of the program/pipeline object the shaders
 *                                            are a part of.
 *  @param out_shader_stages                  Deref will be used to store exactly five IDs. Must not
 *                                            be NULL.
 **/
void FunctionalTest16::getShaderStages(bool retrieve_program_object_shader_ids, const unsigned int& n_id,
									   const _shader_stage** out_shader_stages) const
{
	if (retrieve_program_object_shader_ids)
	{
		out_shader_stages[0] = &m_po_descriptors[n_id].vertex;
		out_shader_stages[1] = &m_po_descriptors[n_id].tess_control;
		out_shader_stages[2] = &m_po_descriptors[n_id].tess_evaluation;
		out_shader_stages[3] = &m_po_descriptors[n_id].geometry;
		out_shader_stages[4] = &m_po_descriptors[n_id].fragment;
	}
	else
	{
		out_shader_stages[0] = m_vs_po_descriptors + n_id;
		out_shader_stages[1] = m_tc_po_descriptors + n_id;
		out_shader_stages[2] = m_te_po_descriptors + n_id;
		out_shader_stages[3] = m_gs_po_descriptors + n_id;
		out_shader_stages[4] = m_fs_po_descriptors + n_id;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest16::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	m_are_pipeline_objects_supported =
		m_context.getContextInfo().isExtensionSupported("GL_ARB_separate_shader_objects");

	/* Initialize GL objects required to run the test */
	initTest();

	/* Iterate over both pipelines/programs and verify that calling glUseProgram() /
	 * glBindProgramPipeline() / glUseProgramStages() resets subroutine uniform configuration.
	 */
	for (int test_case = static_cast<int>(TEST_CASE_FIRST); test_case != static_cast<int>(TEST_CASE_COUNT); ++test_case)
	{
		if (static_cast<_test_case>(test_case) != TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT &&
			!m_are_pipeline_objects_supported)
		{
			/* Current test case requires GL_ARB_separate_shader_objects support which is
			 * unavaiable on the platform that we're testing
			 */
			continue;
		}

		for (unsigned int n_object_id = 0; n_object_id < 2; /* pipeline/program objects allocated for the test */
			 ++n_object_id)
		{
			/* Verify that currently reported subroutine uniform values are equal to default values */
			if (test_case == TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT)
			{
				gl.useProgram(m_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");
			}
			else
			{
				gl.bindProgramPipeline(m_pipeline_object_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed");
			}

			verifySubroutineUniformValues(static_cast<_test_case>(test_case), n_object_id,
										  SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES);

			/* Re-configure subroutine uniforms so that they point to different subroutines than
			 * the default ones.
			 */
			const _shader_stage* stages[5 /* fs+gs+tc+te+vs */] = { DE_NULL };

			getShaderStages(static_cast<_test_case>(test_case) == TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT,
							n_object_id, stages);

			for (unsigned int n_stage = 0; n_stage < 5 /* fs+gs+tc+te+vs stages */; ++n_stage)
			{
				const _shader_stage& current_stage				 = *(stages[n_stage]);
				glw::GLuint			 subroutine_configuration[4] = { GL_INVALID_INDEX };

				subroutine_configuration[0] =
					(current_stage.default_subroutine1_value == current_stage.function1_index) ?
						current_stage.function2_index :
						current_stage.function1_index;
				subroutine_configuration[1] =
					(current_stage.default_subroutine2_value == current_stage.function1_index) ?
						current_stage.function2_index :
						current_stage.function1_index;
				subroutine_configuration[2] =
					(current_stage.default_subroutine3_value == current_stage.function3_index) ?
						current_stage.function4_index :
						current_stage.function3_index;
				subroutine_configuration[3] =
					(current_stage.default_subroutine4_value == current_stage.function3_index) ?
						current_stage.function4_index :
						current_stage.function3_index;

				gl.uniformSubroutinesuiv(current_stage.gl_stage, 4 /* count */, subroutine_configuration);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() call failed.");
			} /* for (all stages) */

			verifySubroutineUniformValues(static_cast<_test_case>(test_case), n_object_id,
										  SUBROUTINE_UNIFORMS_SET_TO_NONDEFAULT_VALUES);

			/* Execute test case-specific code */
			_shader_stage cached_shader_stage_data;
			bool		  stage_reset_status[Utils::SHADER_STAGE_COUNT] = { false, false, false, false, false };
			bool		  uses_stage_reset_status						= false;

			switch (test_case)
			{
			case TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT:
			{
				/* Switch to a different program object and then back to current PO.
				 * Subroutine uniforms should be back at their default settings, instead of
				 * the ones we've just set.
				 */
				gl.useProgram(m_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				gl.useProgram(m_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call(s) failed.");

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_PIPELINE_OBJECT:
			{
				/* Switch to a different pipeline object and then back to the current one.
				 * Subroutine uniforms should be back at their default settings, instead of
				 * the ones we've just set.
				 */
				gl.bindProgramPipeline(
					m_pipeline_object_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				gl.bindProgramPipeline(m_pipeline_object_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call(s) failed.");

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_FRAGMENT_STAGE:
			{
				/* Change the fragment shader stage to a different one.
				 *
				 * Note: We also need to update internal descriptor since the subroutine/uniform
				 *       locations may be different between the two programs.
				 */
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_FRAGMENT_SHADER_BIT,
									m_fs_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				cached_shader_stage_data		 = m_fs_po_descriptors[n_object_id];
				m_fs_po_descriptors[n_object_id] = m_fs_po_descriptors[(n_object_id + 1) % 2];

				stage_reset_status[Utils::SHADER_STAGE_FRAGMENT] = true;
				uses_stage_reset_status							 = true;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_GEOMETRY_STAGE:
			{
				/* Change the geometry shader stage to a different one.
				 *
				 * Note: We also need to update internal descriptor since the subroutine/uniform
				 *       locations may be different between the two programs.
				 */
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_GEOMETRY_SHADER_BIT,
									m_gs_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				cached_shader_stage_data		 = m_gs_po_descriptors[n_object_id];
				m_gs_po_descriptors[n_object_id] = m_gs_po_descriptors[(n_object_id + 1) % 2];

				stage_reset_status[Utils::SHADER_STAGE_GEOMETRY] = true;
				uses_stage_reset_status							 = true;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_CONTROL_STAGE:
			{
				/* Change the tessellation control shader stage to a different one.
				 *
				 * Note: We also need to update internal descriptor since the subroutine/uniform
				 *       locations may be different between the two programs.
				 */
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_TESS_CONTROL_SHADER_BIT,
									m_tc_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				cached_shader_stage_data		 = m_tc_po_descriptors[n_object_id];
				m_tc_po_descriptors[n_object_id] = m_tc_po_descriptors[(n_object_id + 1) % 2];

				stage_reset_status[Utils::SHADER_STAGE_TESSELLATION_CONTROL] = true;
				uses_stage_reset_status										 = true;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_EVALUATION_STAGE:
			{
				/* Change the tessellation evaluation shader stage to a different one.
				 *
				 * Note: We also need to update internal descriptor since the subroutine/uniform
				 *       locations may be different between the two programs.
				 */
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_TESS_EVALUATION_SHADER_BIT,
									m_te_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				cached_shader_stage_data		 = m_te_po_descriptors[n_object_id];
				m_te_po_descriptors[n_object_id] = m_te_po_descriptors[(n_object_id + 1) % 2];

				stage_reset_status[Utils::SHADER_STAGE_TESSELLATION_EVALUATION] = true;
				uses_stage_reset_status											= true;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_VERTEX_STAGE:
			{
				/* Change the vertex shader stage to a different one.
				 *
				 * Note: We also need to update internal descriptor since the subroutine/uniform
				 *       locations may be different between the two programs.
				 */
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_VERTEX_SHADER_BIT,
									m_vs_po_ids[(n_object_id + 1) % 2 /* objects allocated for the test */]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				cached_shader_stage_data		 = m_vs_po_descriptors[n_object_id];
				m_vs_po_descriptors[n_object_id] = m_vs_po_descriptors[(n_object_id + 1) % 2];

				stage_reset_status[Utils::SHADER_STAGE_VERTEX] = true;
				uses_stage_reset_status						   = true;

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized test case");
			}
			} /* switch (test_case) */

			/* Verify the subroutine uniform values are valid */
			if (!uses_stage_reset_status)
			{
				verifySubroutineUniformValues(static_cast<_test_case>(test_case), n_object_id,
											  SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES);
			}
			else
			{
				const _shader_stage* shader_stages[Utils::SHADER_STAGE_COUNT] = { DE_NULL };

				getShaderStages(static_cast<_test_case>(test_case) == TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT,
								n_object_id, shader_stages);

				for (unsigned int n_shader_stage = 0; n_shader_stage < Utils::SHADER_STAGE_COUNT; ++n_shader_stage)
				{
					const _shader_stage& current_shader_stage = *(shader_stages[n_shader_stage]);

					if (stage_reset_status[n_shader_stage])
					{
						verifySubroutineUniformValuesForShaderStage(current_shader_stage,
																	SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES);
					}
					else
					{
						verifySubroutineUniformValuesForShaderStage(current_shader_stage,
																	SUBROUTINE_UNIFORMS_SET_TO_NONDEFAULT_VALUES);
					}
				} /* for (all shader stages) */
			}

			/* Revert the changes some of the test cases appied */
			switch (test_case)
			{
			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_FRAGMENT_STAGE:
			{
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_FRAGMENT_SHADER_BIT,
									m_fs_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				m_fs_po_descriptors[n_object_id] = cached_shader_stage_data;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_GEOMETRY_STAGE:
			{
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_GEOMETRY_SHADER_BIT,
									m_gs_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				m_gs_po_descriptors[n_object_id] = cached_shader_stage_data;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_CONTROL_STAGE:
			{
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_TESS_CONTROL_SHADER_BIT,
									m_tc_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				m_tc_po_descriptors[n_object_id] = cached_shader_stage_data;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_TESS_EVALUATION_STAGE:
			{
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_TESS_EVALUATION_SHADER_BIT,
									m_te_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				m_te_po_descriptors[n_object_id] = cached_shader_stage_data;

				break;
			}

			case TEST_CASE_SWITCH_TO_DIFFERENT_PIPELINE_VERTEX_STAGE:
			{
				gl.useProgramStages(m_pipeline_object_ids[n_object_id], GL_VERTEX_SHADER_BIT, m_vs_po_ids[n_object_id]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

				m_vs_po_descriptors[n_object_id] = cached_shader_stage_data;

				break;
			}

			default:
				break;
			} /* switch (test_case) */

		} /* for (all program object descriptors) */

		/* Unbind the program object */
		gl.useProgram(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
	} /* for (all test cases) */

	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies the subroutine uniform values reported by GL implementation. Depending on the test case,
 *  it will either query program object stages or separate shader objects.
 *
 *  @param test_case    Test case the verification is to be performed for.
 *  @param n_id         Index of the program/pipeline object to use for the verification
 *  @param verification Verification method.
 */
void FunctionalTest16::verifySubroutineUniformValues(const _test_case& test_case, const unsigned int& n_id,
													 const _subroutine_uniform_value_verification& verification)
{
	const _shader_stage* stages[] = {
		DE_NULL, /* fragment shader     stage slot */
		DE_NULL, /* geometry shader     stage slot */
		DE_NULL, /* tess control shader stage slot */
		DE_NULL, /* tess eval shader    stage slot */
		DE_NULL  /* vertex shader       stage slot */
	};
	const unsigned int n_stages = sizeof(stages) / sizeof(stages[0]);

	/* Verify that currently reported subroutine uniform values are equal to default values */
	getShaderStages(test_case == TEST_CASE_SWITCH_TO_DIFFERENT_PROGRAM_OBJECT, n_id, stages);

	for (unsigned int n_stage = 0; n_stage < n_stages; ++n_stage)
	{
		const _shader_stage& current_stage = *(stages[n_stage]);

		verifySubroutineUniformValuesForShaderStage(current_stage, verification);
	} /* for (all items) */
}

/** Verifies the subroutine uniform values reported by GL implementation for user-specified
 *  shader stage. If the verification fails, m_has_test_passed will be set to false.
 *
 *  @param shader_stage Descriptor of a shader stage that should be used for the process.
 *  @param verification Type of verification that should be performed.
 *
 **/
void FunctionalTest16::verifySubroutineUniformValuesForShaderStage(
	const _shader_stage& shader_stage, const _subroutine_uniform_value_verification& verification)
{
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLuint			  result_values[4] = { 0 };

	gl.getUniformSubroutineuiv(shader_stage.gl_stage, shader_stage.subroutine1_uniform_location, result_values + 0);
	gl.getUniformSubroutineuiv(shader_stage.gl_stage, shader_stage.subroutine2_uniform_location, result_values + 1);
	gl.getUniformSubroutineuiv(shader_stage.gl_stage, shader_stage.subroutine3_uniform_location, result_values + 2);
	gl.getUniformSubroutineuiv(shader_stage.gl_stage, shader_stage.subroutine4_uniform_location, result_values + 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformSubroutineuiv() call(s) failed.");

	if (verification == SUBROUTINE_UNIFORMS_SET_TO_VALID_VALUES)
	{
		if (!((result_values[0] == (GLuint)shader_stage.subroutine1_uniform_location ||
			   result_values[0] == (GLuint)shader_stage.subroutine2_uniform_location) &&
			  (result_values[1] == (GLuint)shader_stage.subroutine1_uniform_location ||
			   result_values[1] == (GLuint)shader_stage.subroutine2_uniform_location) &&
			  (result_values[2] == (GLuint)shader_stage.subroutine3_uniform_location ||
			   result_values[2] == (GLuint)shader_stage.subroutine4_uniform_location) &&
			  (result_values[3] == (GLuint)shader_stage.subroutine3_uniform_location ||
			   result_values[3] == (GLuint)shader_stage.subroutine4_uniform_location)))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "SUBROUTINE_UNIFORMS_SET_TO_VALID_VALUES validation failed. "
														   "Shader stage:["
							   << Utils::getShaderStageStringFromGLEnum(shader_stage.gl_stage) << "], "
																								  "expected data:["
							   << shader_stage.subroutine1_uniform_location << " OR "
							   << shader_stage.subroutine2_uniform_location << " x 2, "
							   << shader_stage.subroutine3_uniform_location << " OR "
							   << shader_stage.subroutine4_uniform_location << " x 2], "
																			   "found data:["
							   << result_values[0] << ", " << result_values[1] << ", " << result_values[2] << ", "
							   << result_values[3] << "]." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}
	}
	else if (verification == SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES)
	{
		if (result_values[0] != shader_stage.default_subroutine1_value ||
			result_values[1] != shader_stage.default_subroutine2_value ||
			result_values[2] != shader_stage.default_subroutine3_value ||
			result_values[3] != shader_stage.default_subroutine4_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "SUBROUTINE_UNIFORMS_SET_TO_DEFAULT_VALUES validation failed. "
								  "Shader stage:["
							   << Utils::getShaderStageStringFromGLEnum(shader_stage.gl_stage) << "], "
																								  "expected data:["
							   << shader_stage.default_subroutine1_value << ", "
							   << shader_stage.default_subroutine2_value << ", "
							   << shader_stage.default_subroutine3_value << ", "
							   << shader_stage.default_subroutine4_value << "], "
																			"found data:["
							   << result_values[0] << ", " << result_values[1] << ", " << result_values[2] << ", "
							   << result_values[3] << "]." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}
	}
	else
	{
		DE_ASSERT(verification == SUBROUTINE_UNIFORMS_SET_TO_NONDEFAULT_VALUES);

		if (result_values[0] == shader_stage.default_subroutine1_value ||
			result_values[1] == shader_stage.default_subroutine2_value ||
			result_values[2] == shader_stage.default_subroutine3_value ||
			result_values[3] == shader_stage.default_subroutine4_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "SUBROUTINE_UNIFORMS_SET_TO_NONDEFAULT_VALUES validation failed. "
								  "Shader stage:["
							   << Utils::getShaderStageStringFromGLEnum(shader_stage.gl_stage) << "], "
																								  "expected data:!["
							   << shader_stage.default_subroutine1_value << ", "
							   << shader_stage.default_subroutine2_value << ", "
							   << shader_stage.default_subroutine3_value << ", "
							   << shader_stage.default_subroutine4_value << "], "
																			"found data:["
							   << result_values[0] << ", " << result_values[1] << ", " << result_values[2] << ", "
							   << result_values[3] << "]." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}
	}
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest17::FunctionalTest17(deqp::Context& context)
	: TestCase(context, "same_subroutine_and_subroutine_uniform_but_different_type_used_in_all_stages",
			   "Creates a program which uses the same subroutine and subroutine uniform "
			   "names for every stage (types of subroutines are different in each stage) "
			   "and then makes sure that such program compiles and works as expected.")
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_to_data(DE_NULL)
	, m_to_height(4) /* arbitrary value */
	, m_to_id(0)
	, m_to_width(4) /* arbitrary value */
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution. */
void FunctionalTest17::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_to_data != DE_NULL)
	{
		delete[] m_to_data;

		m_to_data = DE_NULL;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Restore original GL configuration */
	gl.patchParameteri(GL_PATCH_VERTICES, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");

	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed.");
}

/** Retrieves body of a fragment shader that should be used by the test program.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest17::getFragmentShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "in GS_DATA\n"
		   "{\n"
		   "    vec4 gs_data;\n"
		   "    vec4 tc_data;\n"
		   "    vec4 te_data;\n"
		   "    vec4 vs_data;\n"
		   "} gs;\n"
		   "\n"
		   "out vec4 result;\n"
		   "\n"
		   "subroutine void subroutineTypeFS(out vec4 result);\n"
		   "\n"
		   "subroutine(subroutineTypeFS) void subroutine1(out vec4 result)\n"
		   "{\n"
		   "    result = vec4(5, 6, 7, 8);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineTypeFS function;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    vec4 fs_data;\n"
		   "\n"
		   "    function(fs_data);\n"
		   "    result = gs.gs_data + gs.tc_data + gs.te_data + gs.vs_data + fs_data;\n"
		   "}\n";
}

/** Retrieves body of a geometry shader that should be used by the test program.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest17::getGeometryShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout(points)                           in;\n"
		   "layout(triangle_strip, max_vertices = 4) out;\n"
		   "\n"
		   "subroutine void subroutineTypeGS(out vec4 result);\n"
		   "\n"
		   "subroutine(subroutineTypeGS) void subroutine1(out vec4 result)\n"
		   "{\n"
		   "    result = vec4(4, 5, 6, 7);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineTypeGS function;\n"
		   "\n"
		   "in TE_DATA\n"
		   "{\n"
		   "    vec4 tc_data;\n"
		   "    vec4 te_data;\n"
		   "    vec4 vs_data;\n"
		   "} te[];\n"
		   "\n"
		   "out GS_DATA\n"
		   "{\n"
		   "    vec4 gs_data;\n"
		   "    vec4 tc_data;\n"
		   "    vec4 te_data;\n"
		   "    vec4 vs_data;\n"
		   "} result;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    function(result.gs_data);\n"
		   "    gl_Position    = vec4(1, -1, 0, 1);\n"
		   "    result.tc_data = te[0].tc_data;\n"
		   "    result.te_data = te[0].te_data;\n"
		   "    result.vs_data = te[0].vs_data;\n"
		   "    EmitVertex();\n"
		   "\n"
		   "    function(result.gs_data);\n"
		   "    gl_Position    = vec4(-1, -1, 0, 1);\n"
		   "    result.tc_data = te[0].tc_data;\n"
		   "    result.te_data = te[0].te_data;\n"
		   "    result.vs_data = te[0].vs_data;\n"
		   "    EmitVertex();\n"
		   "\n"
		   "    function(result.gs_data);\n"
		   "    gl_Position    = vec4(1, 1, 0, 1);\n"
		   "    result.tc_data = te[0].tc_data;\n"
		   "    result.te_data = te[0].te_data;\n"
		   "    result.vs_data = te[0].vs_data;\n"
		   "    EmitVertex();\n"
		   "\n"
		   "    function(result.gs_data);\n"
		   "    gl_Position    = vec4(-1, 1, 0, 1);\n"
		   "    result.tc_data = te[0].tc_data;\n"
		   "    result.te_data = te[0].te_data;\n"
		   "    result.vs_data = te[0].vs_data;\n"
		   "    EmitVertex();\n"
		   "    EndPrimitive();\n"
		   "}\n";
}

/** Retrieves body of a tessellation control shader that should be used by the test program.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest17::getTessellationControlShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout (vertices = 4) out;\n"
		   "\n"
		   "subroutine void subroutineTypeTC(out vec4 result);\n"
		   "\n"
		   "subroutine(subroutineTypeTC) void subroutine1(out vec4 result)\n"
		   "{\n"
		   "    result = vec4(2, 3, 4, 5);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineTypeTC function;\n"
		   "\n"
		   "in VS_DATA\n"
		   "{\n"
		   "    vec4 vs_data;\n"
		   "} vs[];\n"
		   "\n"
		   "out TC_DATA\n"
		   "{\n"
		   "    vec4 tc_data;\n"
		   "    vec4 vs_data;\n"
		   "} result[];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_TessLevelInner[0] = 1.0;\n"
		   "    gl_TessLevelInner[1] = 1.0;\n"
		   "    gl_TessLevelOuter[0] = 1.0;\n"
		   "    gl_TessLevelOuter[1] = 1.0;\n"
		   "    gl_TessLevelOuter[2] = 1.0;\n"
		   "    gl_TessLevelOuter[3] = 1.0;\n"
		   "\n"
		   "    function(result[gl_InvocationID].tc_data);\n"
		   "    result[gl_InvocationID].vs_data = vs[gl_InvocationID].vs_data;\n"
		   "}\n";
}

/** Retrieves body of a tessellation evaluation shader that should be used
 *  by the test program.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest17::getTessellationEvaluationShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout (quads, point_mode) in;\n"
		   "\n"
		   "subroutine void subroutineTypeTE(out vec4 result);\n"
		   "\n"
		   "subroutine(subroutineTypeTE) void subroutine1(out vec4 result)\n"
		   "{\n"
		   "    result = vec4(3, 4, 5, 6);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineTypeTE function;\n"
		   "\n"
		   "in TC_DATA\n"
		   "{\n"
		   "    vec4 tc_data;\n"
		   "    vec4 vs_data;\n"
		   "} tc[];\n"
		   "\n"
		   "out TE_DATA\n"
		   "{\n"
		   "    vec4 tc_data;\n"
		   "    vec4 te_data;\n"
		   "    vec4 vs_data;\n"
		   "} result;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    result.vs_data = tc[0].vs_data;\n"
		   "    result.tc_data = tc[0].tc_data;\n"
		   "    function(result.te_data);\n"
		   "}\n";
}

/** Retrieves body of a vertex shader that should be used by the test program.
 *
 *  @return Requested string.
 **/
std::string FunctionalTest17::getVertexShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "out VS_DATA\n"
		   "{\n"
		   "    vec4 vs_data;\n"
		   "} result;\n"
		   "\n"
		   "subroutine void subroutineTypeVS(out vec4 result);\n"
		   "\n"
		   "subroutine(subroutineTypeVS) void subroutine1(out vec4 result)\n"
		   "{\n"
		   "    result = vec4(1, 2, 3, 4);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform subroutineTypeVS function;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    function(result.vs_data);\n"
		   "}\n";
}

/** Initializes all buffers and GL objects required to run the test. */
void FunctionalTest17::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Configure GL_PATCH_VERTICES so that TC only takes a single patch vertex */
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");

	/* Generate & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set up test program object */
	std::string fs_body = getFragmentShaderBody();
	std::string gs_body = getGeometryShaderBody();
	std::string tc_body = getTessellationControlShaderBody();
	std::string te_body = getTessellationEvaluationShaderBody();
	std::string vs_body = getVertexShaderBody();

	if (!Utils::buildProgram(gl, vs_body, tc_body, te_body, gs_body, fs_body, DE_NULL, /* xfb_varyings */
							 DE_NULL,												   /* n_xfb_varyings */
							 &m_vs_id, &m_tc_id, &m_te_id, &m_gs_id, &m_fs_id, &m_po_id))
	{
		TCU_FAIL("Failed to link test program object");
	}

	/* Set up a texture object that will be used as a color attachment */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA32F, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up FBO */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Make sure glReadPixels() does not return misaligned data */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed.");

	/* Initialize a buffer that will be used to store rendered data */
	m_to_data = new float[m_to_width * m_to_height * 4 /* rgba */];
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest17::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	initTest();

	/* Use the test program to render a full-screen test quad */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.drawArrays(GL_PATCHES, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

	/* Read back the data that was rendered */
	gl.readPixels(0, /* x */
				  0, /* y */
				  m_to_width, m_to_height, GL_RGBA, GL_FLOAT, m_to_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReaDPixels() call failed.");

	/* Verify the data */
	verifyRenderedData();

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Verifies the data that have been rendered by the test program.
 *
 *  It is assumed the rendered data have already been copied to
 *  m_to_data.
 *
 *  If the rendered data is found to be invalid, m_has_test_passed
 *  will be set to false.
 **/
void FunctionalTest17::verifyRenderedData()
{
	const float epsilon			 = 1e-5f;
	const float expected_data[4] = { 15.0f, 20.0f, 25.0f, 30.0f };

	for (unsigned int y = 0; y < m_to_height && m_has_test_passed; ++y)
	{
		const float* row_ptr = m_to_data + y * 4 /* rgba */ * m_to_width;

		for (unsigned int x = 0; x < m_to_width && m_has_test_passed; ++x)
		{
			const float* pixel_ptr = row_ptr + 4 /* rgba */ * x;

			if (de::abs(pixel_ptr[0] - expected_data[0]) > epsilon ||
				de::abs(pixel_ptr[1] - expected_data[1]) > epsilon ||
				de::abs(pixel_ptr[2] - expected_data[2]) > epsilon ||
				de::abs(pixel_ptr[3] - expected_data[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texel found at (" << x << ", " << y
								   << "): "
									  "expected:("
								   << expected_data[0] << ", " << expected_data[1] << ", " << expected_data[2] << ", "
								   << expected_data[3] << "), found:(" << pixel_ptr[0] << ", " << pixel_ptr[1] << ", "
								   << pixel_ptr[2] << ", " << pixel_ptr[3] << ")." << tcu::TestLog::EndMessage;

				m_has_test_passed = false;
			}
		} /* for (all columns) */
	}	 /* for (all rows) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest18_19::FunctionalTest18_19(deqp::Context& context)
	: TestCase(context, "control_flow_and_returned_subroutine_values_used_as_subroutine_input",
			   "Makes sure that calling a subroutine with argument value returned by "
			   "another subroutine works correctly. Also checks that subroutine and "
			   "subroutine uniforms work as expected when used in connection with control "
			   "flow functions.")
	, m_has_test_passed(true)
	, m_n_points_to_draw(16) /* arbitrary value */
	, m_po_id(0)
	, m_po_subroutine_divide_by_two_location(GL_INVALID_INDEX)
	, m_po_subroutine_multiply_by_four_location(GL_INVALID_INDEX)
	, m_po_subroutine_returns_false_location(GL_INVALID_INDEX)
	, m_po_subroutine_returns_true_location(GL_INVALID_INDEX)
	, m_po_subroutine_uniform_bool_operator1(-1)
	, m_po_subroutine_uniform_bool_operator2(-1)
	, m_po_subroutine_uniform_vec4_processor1(-1)
	, m_po_subroutine_uniform_vec4_processor2(-1)
	, m_xfb_bo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** De-initializes all GL objects that may have been created during test execution */
void FunctionalTest18_19::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_xfb_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_xfb_bo_id);

		m_xfb_bo_id = 0;
	}
}

/** Executes a single test iteration using user-specified properties. If the
 *  iterations fails, m_has_test_passed is set to false.
 *
 *  @param bool_operator1_subroutine_location Location of a subroutine to be assigned to
 *                                            bool_operator1 subroutine uniform.
 *  @param bool_operator2_subroutine_location Location of a subroutine to be assigned to
 *                                            bool_operator2 subroutine uniform.
 *  @param vec4_operator1_subroutine_location Location of a subroutine to be assigned to
 *                                            vec4_operator1 subroutine uniform.
 *  @param vec4_operator2_subroutine_location Location of a subroutine to be assigned to
 *                                            vec4_operator2 subroutine uniform.
 &**/
void FunctionalTest18_19::executeTest(glw::GLuint bool_operator1_subroutine_location,
									  glw::GLuint bool_operator2_subroutine_location,
									  glw::GLuint vec4_operator1_subroutine_location,
									  glw::GLuint vec4_operator2_subroutine_location)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up subroutines */
	glw::GLuint subroutine_configuration[4 /* total number of subroutines */] = { 0 };

	subroutine_configuration[m_po_subroutine_uniform_bool_operator1]  = bool_operator1_subroutine_location;
	subroutine_configuration[m_po_subroutine_uniform_bool_operator2]  = bool_operator2_subroutine_location;
	subroutine_configuration[m_po_subroutine_uniform_vec4_processor1] = vec4_operator1_subroutine_location;
	subroutine_configuration[m_po_subroutine_uniform_vec4_processor2] = vec4_operator2_subroutine_location;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, 4 /* count */, subroutine_configuration);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() call failed");

	/* Draw test-specific number of points */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
	{
		gl.drawArrays(GL_POINTS, 0 /* first */, m_n_points_to_draw);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	/* Map the BO storage into process space */
	const glw::GLvoid* xfb_data_ptr = gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed.");

	verifyXFBData(xfb_data_ptr, bool_operator1_subroutine_location, bool_operator2_subroutine_location,
				  vec4_operator1_subroutine_location, vec4_operator2_subroutine_location);

	/* Unmap BO storage */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
}

/** Retrieves body of a vertex shader to be used by the test. */
std::string FunctionalTest18_19::getVertexShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "subroutine bool bool_processor();\n"
		   "subroutine vec4 vec4_processor(in vec4 iparam);\n"
		   "\n"
		   "subroutine(bool_processor) bool returnsFalse()\n"
		   "{\n"
		   "    return false;\n"
		   "}\n"
		   "\n"
		   "subroutine(bool_processor) bool returnsTrue()\n"
		   "{\n"
		   "    return true;\n"
		   "}\n"
		   "\n"
		   "subroutine(vec4_processor) vec4 divideByTwo(in vec4 iparam)\n"
		   "{\n"
		   "    return iparam * vec4(0.5);\n"
		   "}\n"
		   "\n"
		   "subroutine(vec4_processor) vec4 multiplyByFour(in vec4 iparam)\n"
		   "{\n"
		   "    return iparam * vec4(4.0);\n"
		   "}\n"
		   "\n"
		   "subroutine uniform bool_processor bool_operator1;\n"
		   "subroutine uniform bool_processor bool_operator2;\n"
		   "subroutine uniform vec4_processor vec4_operator1;\n"
		   "subroutine uniform vec4_processor vec4_operator2;\n"
		   "\n"
		   "out float result;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    if (bool_operator1() )\n"
		   "    {\n"
		   "        float value = float( (3 * gl_VertexID + 1) * 2);\n"
		   "\n"
		   "        while (bool_operator1() )\n"
		   "        {\n"
		   "            value /= float(gl_VertexID + 2);\n"
		   "\n"
		   "            if (value <= 1.0f) break;\n"
		   "        }\n"
		   "\n"
		   "        result = value;\n"
		   "    }\n"
		   "    else\n"
		   "    {\n"
		   "        vec4 value = vec4(gl_VertexID,     gl_VertexID + 1,\n"
		   "                          gl_VertexID + 2, gl_VertexID + 3);\n"
		   "\n"
		   "        switch (gl_VertexID % 2)\n"
		   "        {\n"
		   "            case 0:\n"
		   "            {\n"
		   "                for (int iteration = 0; iteration < gl_VertexID && bool_operator2(); ++iteration)\n"
		   "                {\n"
		   "                    value = vec4_operator2(vec4_operator1(value));\n"
		   "                }\n"
		   "\n"
		   "                break;\n"
		   "            }\n"
		   "\n"
		   "            case 1:\n"
		   "            {\n"
		   "                for (int iteration = 0; iteration < gl_VertexID * 2; ++iteration)\n"
		   "                {\n"
		   "                    value = vec4_operator1(vec4_operator2(value));\n"
		   "                }\n"
		   "\n"
		   "                break;\n"
		   "            }\n"
		   "        }\n"
		   "\n"
		   "        result = value.x + value.y + value.z + value.w;\n"
		   "\n"
		   "    }\n"
		   "}\n";
}

/** Initializes all GL objects required to run the test. */
void FunctionalTest18_19::initTest()
{
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	const char*			  varyings[1] = { "result" };
	std::string			  vs_body	 = getVertexShaderBody();
	const unsigned int	n_varyings  = sizeof(varyings) / sizeof(varyings[0]);

	if (!Utils::buildProgram(gl, vs_body, "",						  /* tc_body */
							 "",									  /* te_body */
							 "",									  /* gs_body */
							 "",									  /* fs_body */
							 varyings, n_varyings, &m_vs_id, DE_NULL, /* out_tc_id */
							 DE_NULL,								  /* out_te_id */
							 DE_NULL,								  /* out_gs_id */
							 DE_NULL,								  /* out_fs_id */
							 &m_po_id))
	{
		TCU_FAIL("Failed to build test program object");
	}

	/* Retrieve subroutine & subroutine uniform locations */
	m_po_subroutine_divide_by_two_location	= gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "divideByTwo");
	m_po_subroutine_multiply_by_four_location = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "multiplyByFour");
	m_po_subroutine_returns_false_location	= gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "returnsFalse");
	m_po_subroutine_returns_true_location	 = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "returnsTrue");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() call(s) failed");

	if (m_po_subroutine_divide_by_two_location == GL_INVALID_INDEX ||
		m_po_subroutine_multiply_by_four_location == GL_INVALID_INDEX ||
		m_po_subroutine_returns_false_location == GL_INVALID_INDEX ||
		m_po_subroutine_returns_true_location == GL_INVALID_INDEX)
	{
		TCU_FAIL("glGetSubroutineIndex() returned GL_INVALID_INDEX for a valid subroutine");
	}

	m_po_subroutine_uniform_bool_operator1 =
		gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "bool_operator1");
	m_po_subroutine_uniform_bool_operator2 =
		gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "bool_operator2");
	m_po_subroutine_uniform_vec4_processor1 =
		gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "vec4_operator1");
	m_po_subroutine_uniform_vec4_processor2 =
		gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "vec4_operator2");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() call(s) failed");

	if (m_po_subroutine_uniform_bool_operator1 == -1 || m_po_subroutine_uniform_bool_operator2 == -1 ||
		m_po_subroutine_uniform_vec4_processor1 == -1 || m_po_subroutine_uniform_vec4_processor2 == -1)
	{
		TCU_FAIL("glGetSubroutineUniformLocation() returned -1 for an active subroutine uniform");
	}

	/* Set up XFB BO */
	const unsigned int bo_size = static_cast<unsigned int>(sizeof(float) * m_n_points_to_draw);

	gl.genBuffers(1, &m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_xfb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL /* data */, GL_STATIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Set up a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest18_19::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Initialize all GL objects required to run the test */
	initTest();

	/* Iterate over all subroutine permutations */
	const glw::GLuint subroutine_bool_operators[] = { m_po_subroutine_returns_false_location,
													  m_po_subroutine_returns_true_location };
	const unsigned int n_subroutine_bool_operators =
		sizeof(subroutine_bool_operators) / sizeof(subroutine_bool_operators[0]);

	const glw::GLuint subroutine_vec4_operators[] = { m_po_subroutine_divide_by_two_location,
													  m_po_subroutine_multiply_by_four_location };
	const unsigned int n_subroutine_vec4_operators =
		sizeof(subroutine_vec4_operators) / sizeof(subroutine_vec4_operators[0]);

	for (unsigned int n_subroutine_uniform_bool_operator1 = 0;
		 n_subroutine_uniform_bool_operator1 < n_subroutine_bool_operators; ++n_subroutine_uniform_bool_operator1)
	{
		for (unsigned int n_subroutine_uniform_bool_operator2 = 0;
			 n_subroutine_uniform_bool_operator2 < n_subroutine_bool_operators; ++n_subroutine_uniform_bool_operator2)
		{
			for (unsigned int n_subroutine_uniform_vec4_operator1 = 0;
				 n_subroutine_uniform_vec4_operator1 < n_subroutine_vec4_operators;
				 ++n_subroutine_uniform_vec4_operator1)
			{
				for (unsigned int n_subroutine_uniform_vec4_operator2 = 0;
					 n_subroutine_uniform_vec4_operator2 < n_subroutine_vec4_operators;
					 ++n_subroutine_uniform_vec4_operator2)
				{
					executeTest(subroutine_bool_operators[n_subroutine_uniform_bool_operator1],
								subroutine_bool_operators[n_subroutine_uniform_bool_operator2],
								subroutine_vec4_operators[n_subroutine_uniform_vec4_operator1],
								subroutine_vec4_operators[n_subroutine_uniform_vec4_operator2]);
				} /* for (all subroutine vec4 operator subroutines used for processor2) */
			}	 /* for (all subroutine vec4 operator subroutines used for processor1) */
		}		  /* for (all subroutine bool operator subroutines used for operator2) */
	}			  /* for (all subroutine bool operator subroutines used for operator1) */

	/* All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Divides input argument by two. The result value is returned to the
 *  caller.
 *
 *  @param data Input value.
 *
 *  @return As per description.
 **/
tcu::Vec4 FunctionalTest18_19::vec4operator_div2(tcu::Vec4 data)
{
	return data * 0.5f;
}

/** Multiplies input argument by four. The result value is returned to the
 *  caller.
 *
 *  @param data Input value.
 *
 *  @return As per description.
 **/
tcu::Vec4 FunctionalTest18_19::vec4operator_mul4(tcu::Vec4 data)
{
	return data * 4.0f;
}

/** Verifies data XFBed out by the vertex shader. It is assumed the subroutines were configured
 *  as per passed arguments, prior to the draw call.
 *
 *  If the result data is found to be invalid, m_has_test_passed is set to false.
 *
 *  @param data                               XFBed data.
 *  @param bool_operator1_subroutine_location Location of a subroutine to be assigned to
 *                                            bool_operator1 subroutine uniform.
 *  @param bool_operator2_subroutine_location Location of a subroutine to be assigned to
 *                                            bool_operator2 subroutine uniform.
 *  @param vec4_operator1_subroutine_location Location of a subroutine to be assigned to
 *                                            vec4_operator1 subroutine uniform.
 *  @param vec4_operator2_subroutine_location Location of a subroutine to be assigned to
 *                                            vec4_operator2 subroutine uniform.
 */
void FunctionalTest18_19::verifyXFBData(const glw::GLvoid* data, glw::GLuint bool_operator1_subroutine_location,
										glw::GLuint bool_operator2_subroutine_location,
										glw::GLuint vec4_operator1_subroutine_location,
										glw::GLuint vec4_operator2_subroutine_location)
{
	bool				bool_operator1_result = false;
	bool				bool_operator2_result = false;
	const float			epsilon				  = 1e-5f;
	PFNVEC4OPERATORPROC pVec4Operator1		  = NULL;
	PFNVEC4OPERATORPROC pVec4Operator2		  = NULL;
	const glw::GLfloat* traveller_ptr		  = (const glw::GLfloat*)data;

	bool_operator1_result = (bool_operator1_subroutine_location == m_po_subroutine_returns_true_location);
	bool_operator2_result = (bool_operator2_subroutine_location == m_po_subroutine_returns_true_location);
	pVec4Operator1		  = (vec4_operator1_subroutine_location == m_po_subroutine_divide_by_two_location) ?
						 vec4operator_div2 :
						 vec4operator_mul4;
	pVec4Operator2 = (vec4_operator2_subroutine_location == m_po_subroutine_divide_by_two_location) ?
						 vec4operator_div2 :
						 vec4operator_mul4;

	for (unsigned int n_vertex = 0; n_vertex < m_n_points_to_draw; ++n_vertex)
	{
		float expected_value = 0.0f;

		if (bool_operator1_result)
		{
			float value = float((3 * n_vertex + 1) * 2);

			while (bool_operator1_result)
			{
				value /= float(n_vertex + 2);

				if (value <= 1.0f)
					break;
			}

			expected_value = value;
		}
		else
		{
			tcu::Vec4 value((float)n_vertex, (float)n_vertex + 1, (float)n_vertex + 2, (float)n_vertex + 3);

			switch (n_vertex % 2)
			{
			case 0:
			{
				for (unsigned int iteration = 0; iteration < n_vertex && bool_operator2_result; ++iteration)
				{
					value = pVec4Operator2(pVec4Operator1(value));
				}

				break;
			}

			case 1:
			{
				for (unsigned int iteration = 0; iteration < n_vertex * 2; ++iteration)
				{
					value = pVec4Operator1(pVec4Operator2(value));
				}

				break;
			}
			} /* switch (n_vertex % 2) */

			expected_value = value.x() + value.y() + value.z() + value.w();
		}

		if (de::abs(expected_value - *traveller_ptr) > epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "XFBed data was found to be invalid at index [" << n_vertex
							   << "]"
								  "for the following subroutine location configuration:"
								  " bool_operator1_subroutine_location:["
							   << bool_operator1_subroutine_location << "]"
																		" bool_operator2_subroutine_location:["
							   << bool_operator2_subroutine_location << "]"
																		" vec4_operator1_subroutine_location:["
							   << vec4_operator1_subroutine_location << "]"
																		" vec4_operator2_subroutine_location:["
							   << vec4_operator2_subroutine_location << "];"
																		" expected data:"
							   << expected_value << ", found:" << *traveller_ptr << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		++traveller_ptr;
	} /* for (all drawn points) */
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest1::NegativeTest1(deqp::Context& context)
	: TestCase(context, "subroutine_errors", "Verifies all GL_INVALID_OPERATION, GL_INVALID_VALUE, GL_INVALID ENUM "
											 "errors related to subroutine usage are properly generated.")
	, m_has_test_passed(true)
	, m_po_active_subroutine_uniform_locations(0)
	, m_po_active_subroutine_uniforms(0)
	, m_po_active_subroutines(0)
	, m_po_subroutine_uniform_function_index(-1)
	, m_po_subroutine_uniform_function2_index(-1)
	, m_po_subroutine_test1_index(GL_INVALID_INDEX)
	, m_po_subroutine_test2_index(GL_INVALID_INDEX)
	, m_po_subroutine_test3_index(GL_INVALID_INDEX)
	, m_po_not_linked_id(0)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during
 *  test execution.
 **/
void NegativeTest1::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_po_not_linked_id != 0)
	{
		gl.deleteProgram(m_po_not_linked_id);

		m_po_not_linked_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Initializes all GL objects required to run the test.  */
void NegativeTest1::initTest()
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();

	/* Create program objects */
	m_po_not_linked_id = gl.createProgram();
	m_po_id			   = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Create vertex shader object */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Set up vertex shader */
	const char* vs_body = "#version 400\n"
						  "\n"
						  "#extension GL_ARB_shader_subroutine : require\n"
						  "\n"
						  "subroutine void subroutineType (out ivec2 arg);\n"
						  "subroutine void subroutineType2(out ivec4 arg);\n"
						  "\n"
						  "subroutine(subroutineType) void test1(out ivec2 arg)\n"
						  "{\n"
						  "    arg = ivec2(1, 2);\n"
						  "}\n"
						  "subroutine(subroutineType) void test2(out ivec2 arg)\n"
						  "{\n"
						  "    arg = ivec2(3,4);\n"
						  "}\n"
						  "subroutine(subroutineType2) void test3(out ivec4 arg)\n"
						  "{\n"
						  "    arg = ivec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineType  function;\n"
						  "subroutine uniform subroutineType2 function2;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    ivec2 test;\n"
						  "    ivec4 test2;\n"
						  "\n"
						  "    function(test);\n"
						  "\n"
						  "    if (test.x > 2)\n"
						  "    {\n"
						  "        gl_Position = vec4(1);\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        function2(test2);\n"
						  "\n"
						  "        gl_Position = vec4(float(test2.x) );\n"
						  "    }\n"
						  "}\n";

	gl.shaderSource(m_vs_id, 1 /* count */, &vs_body, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Shader compilation failed");
	}

	/* Set up & link the test program object */
	glw::GLint link_status = GL_FALSE;

	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status == GL_FALSE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* Query test program object's properties */
	gl.getProgramStageiv(m_po_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS,
						 &m_po_active_subroutine_uniform_locations);
	gl.getProgramStageiv(m_po_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &m_po_active_subroutine_uniforms);
	gl.getProgramStageiv(m_po_id, GL_VERTEX_SHADER, GL_ACTIVE_SUBROUTINES, &m_po_active_subroutines);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramStageiv() call(s) failed.");

	if (m_po_active_subroutine_uniform_locations != 2)
	{
		TCU_FAIL("Invalid GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS value returned");
	}

	m_po_subroutine_test1_index = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "test1");
	m_po_subroutine_test2_index = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "test2");
	m_po_subroutine_test3_index = gl.getSubroutineIndex(m_po_id, GL_VERTEX_SHADER, "test3");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() call(s) failed.");

	if (m_po_subroutine_test1_index == GL_INVALID_INDEX || m_po_subroutine_test2_index == GL_INVALID_INDEX ||
		m_po_subroutine_test3_index == GL_INVALID_INDEX)
	{
		TCU_FAIL("Invalid subroutine index returned");
	}

	m_po_subroutine_uniform_function_index  = gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "function");
	m_po_subroutine_uniform_function2_index = gl.getSubroutineUniformLocation(m_po_id, GL_VERTEX_SHADER, "function2");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() call(s) failed.");

	if (m_po_subroutine_uniform_function_index == -1 || m_po_subroutine_uniform_function2_index == -1)
	{
		TCU_FAIL("Invalid subroutine uniform index returned");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest1::iterate()
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Initialize GL objects required to run the test */
	initTest();

	/* The error INVALID_OPERATION is generated by GetSubroutineUniformLocation
	 * if the program object identified by <program> has not been successfully
	 * linked.
	 */
	gl.getSubroutineUniformLocation(m_po_not_linked_id, GL_FRAGMENT_SHADER, "subroutine_uniform_name");

	error_code = gl.getError();

	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetSubroutineUniformLocation() does not generate GL_INVALID_OPERATION "
							  "error code when called for a non-linked program object."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_VALUE is generated by GetActiveSubroutineUniformiv or
	 * GetActiveSubroutineUniformName if <index> is greater than or equal to the
	 * value of ACTIVE_SUBROUTINE_UNIFORMS for the shader stage.
	 */
	glw::GLint temp_length = 0;
	glw::GLint temp_values = 0;

	gl.getActiveSubroutineUniformiv(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutine_uniforms,
									GL_NUM_COMPATIBLE_SUBROUTINES, &temp_values);
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.getActiveSubroutineUniformiv(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutine_uniforms + 1,
										GL_NUM_COMPATIBLE_SUBROUTINES, &temp_values);

		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetActiveSubroutineUniformiv() does not generate GL_INVALID_VALUE "
							  "when passed <index> argument that is greater than or equal to "
							  "the value of GL_ACTIVE_SUBROUTINE_UNIFORMS."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	gl.getActiveSubroutineUniformName(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutine_uniforms, 0, /* bufsize */
									  &temp_length, DE_NULL);										 /* name */
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.getActiveSubroutineUniformName(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutine_uniforms + 1,
										  0,					  /* bufsize */
										  &temp_length, DE_NULL); /* name */

		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetActiveSubroutineUniformName() does not generate GL_INVALID_VALUE "
							  "when passed <index> argument that is greater than or equal to "
							  "the value of GL_ACTIVE_SUBROUTINE_UNIFORMS."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_VALUE is generated by GetActiveSubroutineName if <index>
	 * is greater than or equal to the value of ACTIVE_SUBROUTINES for the shader
	 * stage.
	 */
	gl.getActiveSubroutineName(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutines, 0, /* bufsize */
							   &temp_length, DE_NULL);								  /* name */
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.getActiveSubroutineName(m_po_id, GL_VERTEX_SHADER, m_po_active_subroutines + 1, 0, /* bufsize */
								   &temp_length, DE_NULL);									  /* name */

		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetActiveSubroutineName() does not generate GL_INVALID_VALUE "
													   "when passed <index> argument that is greater than or equal to "
													   "the value of GL_ACTIVE_SUBROUTINES."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_VALUE is generated by UniformSubroutinesuiv if <count>
	 * is not equal to the value of ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS for the
	 * shader stage <shadertype>.
	 */
	glw::GLuint index = 0;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations - 1, &index);
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations + 1, &index);

		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glUniformSubroutinesiv() does not generate GL_INVALID_VALUE "
													   "when passed <count> argument that is not equal to the value of "
													   "GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_VALUE is generated by UniformSubroutinesuiv if any value
	 * in <indices> is greater than or equal to the value of ACTIVE_SUBROUTINES
	 * for the shader stage.
	 */
	glw::GLuint invalid_subroutine_indices[4] = { (GLuint)m_po_active_subroutines, (GLuint)m_po_active_subroutines,
												  (GLuint)m_po_active_subroutines + 1,
												  (GLuint)m_po_active_subroutines + 1 };

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations, /* count */
							 invalid_subroutine_indices + 0);
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations,
								 invalid_subroutine_indices + 2);

		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glUniformSubroutinesuiv() does not generate GL_INVALID_VALUE "
													   "when the value passed via <indices> argument is greater than "
													   "or equal to the value of GL_ACTIVE_SUBROUTINES."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_OPERATION is generated by UniformSubroutinesuiv() if any
	 * subroutine index in <indices> identifies a subroutine not associated with
	 * the type of the subroutine uniform variable assigned to the corresponding
	 * location.
	 */
	glw::GLuint invalid_subroutine_indices2[2] = { m_po_subroutine_test1_index, m_po_subroutine_test1_index };

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations, invalid_subroutine_indices2);
	error_code = gl.getError();

	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glUniformSubroutinesuiv() does not generate GL_INVALID_OPERATION "
							  "when the subroutine index passed via <indices> argument identifies"
							  "a subroutine not associated with the type of the subroutine uniform "
							  "assigned to the corresponding location."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_OPERATION is generated by UniformSubroutinesuiv if no
	 * program is active.
	 */
	glw::GLuint valid_subroutine_locations[2] = { 0 };

	valid_subroutine_locations[m_po_subroutine_uniform_function_index]  = m_po_subroutine_test1_index;
	valid_subroutine_locations[m_po_subroutine_uniform_function2_index] = m_po_subroutine_test3_index;

	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations, valid_subroutine_locations);
	error_code = gl.getError();

	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glUniformSubroutinesuiv() does not generate GL_INVALID_OPERATION "
							  "when called without an active program object."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_VALUE is generated by GetUniformSubroutineuiv if
	 * <location> is greater than or equal to the value of
	 * ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS for the shader stage.
	 */
	glw::GLuint temp_value = 0;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.getUniformSubroutineuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations, &temp_value);
	error_code = gl.getError();

	if (error_code == GL_INVALID_VALUE)
	{
		gl.getUniformSubroutineuiv(GL_VERTEX_SHADER, m_po_active_subroutine_uniform_locations + 1, &temp_value);
		error_code = gl.getError();
	}

	if (error_code != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "glGetUniformSubroutineuiv() does not generate GL_INVALID_VALUE "
							  "when called for location that is greater than or equal to the value "
							  "of GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS."
						   << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* The error INVALID_OPERATION is generated by GetUniformSubroutineuiv if no
	 * program is active for the shader stage identified by <shadertype>.
	 */
	const glw::GLenum undefined_shader_stages[] = { GL_FRAGMENT_SHADER, GL_GEOMETRY_SHADER, GL_TESS_CONTROL_SHADER,
													GL_TESS_EVALUATION_SHADER };
	const unsigned int n_undefined_shader_stages = sizeof(undefined_shader_stages) / sizeof(undefined_shader_stages[0]);

	for (unsigned int n_undefined_shader_stage = 0; n_undefined_shader_stage < n_undefined_shader_stages;
		 ++n_undefined_shader_stage)
	{
		glw::GLenum shader_stage = undefined_shader_stages[n_undefined_shader_stage];

		gl.getUniformSubroutineuiv(shader_stage, 0, /* location */
								   &temp_value);
		error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "glGetUniformSubroutineuiv() does not generate GL_INVALID_OPERATION "
								  "when called for a shader stage that is not defined for active "
								  "program object."
							   << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}
	} /* for (all undefined shader stages) */

	/* All done */
	if (m_has_test_passed)
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
 *  @param context Rendering context.
 *
 **/
NegativeTest2::NegativeTest2(deqp::Context& context)
	: TestCase(context, "subroutine_uniform_scope", "Verifies subroutine uniforms declared in shader stage A"
													"cannot be accessed from a different stage.")
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest2::deinit()
{
	deinitGLObjects();
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest2::deinitGLObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Builds an offending program object and tries to link it. We're either expecting
 *  a compile-time or link-time error here.
 *
 *  If the program object builds successfully, the test has failed.
 *
 *  @param referencing_stage Shader stage which defines a subroutine uniform that
 *                           should be called from fragment/geometry/tess control/
 *                           tess evaluation/vertex shader stages.
 *
 **/
void NegativeTest2::executeTestCase(const Utils::_shader_stage& referencing_stage)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const std::string fs_body = getFragmentShaderBody(referencing_stage);
	const std::string gs_body = getGeometryShaderBody(referencing_stage);
	const std::string tc_body = getTessellationControlShaderBody(referencing_stage);
	const std::string te_body = getTessellationEvaluationShaderBody(referencing_stage);
	const std::string vs_body = getVertexShaderBody(referencing_stage);

	if (Utils::buildProgram(gl, vs_body, tc_body, te_body, gs_body, fs_body, NULL, /* xfb_varyings */
							0,													   /* n_xfb_varyings */
							&m_vs_id, &m_tc_id, &m_te_id, &m_gs_id, &m_fs_id, &m_po_id))
	{
		/* Test program should not have built correctly ! */
		m_testCtx.getLog() << tcu::TestLog::Message << "In the following program, one of the stages references "
													   "a subroutine that is defined in another stage. This "
													   "is forbidden by the specification.\n"
													   "\n"
													   "Vertex shader:\n\n"
						   << vs_body.c_str() << "\n\nTessellation control shader:\n\n"
						   << tc_body.c_str() << "\n\nTessellation evaluation shader:\n\n"
						   << te_body.c_str() << "\n\nGeometry shader:\n\n"
						   << gs_body.c_str() << "\n\nFragment shader:\n\n"
						   << fs_body.c_str() << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	} /* if (test program was built successfully) */

	/* Release the shaders & the program object that buildProgram() created */
	deinitGLObjects();
}

/** Retrieves an offending fragment shader body.
 *
 *  @param referencing_stage Shader stage which defines the subroutine uniform that
 *                           will be called from fragment shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest2::getFragmentShaderBody(const Utils::_shader_stage& referencing_stage) const
{
	std::stringstream result;

	/* Form the pre-amble */
	result << "#version 400\n"
			  "\n"
			  "#extension GL_ARB_shader_subroutine : require\n"
			  "\n"
			  "subroutine void testSubroutineType(out vec4 test_argument);\n"
			  "\n"
			  /* Define a subroutine */
			  "subroutine(testSubroutineType) void fs_subroutine(out vec4 test_argument)\n"
			  "{\n"
			  "    test_argument = vec4(1, 0, 0, 0);\n"
			  "}\n"
			  "\n"
			  /* Define output variables */
			  "out vec4 result;\n"
			  "\n"
			  /* Define uniforms */
			  "subroutine uniform testSubroutineType test_fs_subroutine;\n"
			  "\n"
			  /* Define main() */
			  "void main()\n"
			  "{\n"
			  "    "
		   << getSubroutineUniformName(referencing_stage) << "(result);\n"
															 "}\n";

	return result.str();
}

/** Retrieves an offending geometry shader body.
 *
 *  @param referencing_stage Shader stage which defines the subroutine uniform that
 *                           will be called from geometry shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest2::getGeometryShaderBody(const Utils::_shader_stage& referencing_stage) const
{
	std::stringstream result;

	/* Form the pre-amble */
	result << "#version 400\n"
			  "\n"
			  "#extension GL_ARB_shader_subroutine : require\n"
			  "\n"
			  "subroutine void testSubroutineType(out vec4 test_argument);\n"
			  "\n"
			  "layout(points)                   in;\n"
			  "layout(points, max_vertices = 1) out;\n"
			  "\n"
			  /* Define a subroutine */
			  "subroutine(testSubroutineType) void gs_subroutine(out vec4 test_argument)\n"
			  "{\n"
			  "    test_argument = vec4(0, 1, 1, 1);\n"
			  "}\n"
			  "\n"
			  /* Define output variables */
			  "out vec4 result;\n"
			  "\n"
			  /* Define uniforms */
			  "subroutine uniform testSubroutineType test_gs_subroutine;\n"
			  "\n"
			  /* Define main() */
			  "void main()\n"
			  "{\n"
			  "    "
		   << getSubroutineUniformName(referencing_stage) << "(result);\n"
															 "}\n";

	return result.str();
}

/** Retrieves name of the subroutine uniform that is defined in user-specified
 *  shader stage.
 *
 *  @param stage Shader stage to retrieve the subroutine uniform name for.
 *
 *  @return As per description.
 **/
std::string NegativeTest2::getSubroutineUniformName(const Utils::_shader_stage& stage) const
{
	std::string result = "?";

	switch (stage)
	{
	case Utils::SHADER_STAGE_FRAGMENT:
	{
		result = "test_fs_subroutine";

		break;
	}

	case Utils::SHADER_STAGE_GEOMETRY:
	{
		result = "test_gs_subroutine";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
	{
		result = "test_tc_subroutine";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
	{
		result = "test_te_subroutine";

		break;
	}

	case Utils::SHADER_STAGE_VERTEX:
	{
		result = "test_vs_subroutine";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized shader stage requested");
	}
	} /* switch (stage) */

	return result;
}

/** Retrieves an offending tessellation control shader body.
 *
 *  @param referencing_stage Shader stage which defines the subroutine uniform that
 *                           will be called from tessellation control shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest2::getTessellationControlShaderBody(const Utils::_shader_stage& referencing_stage) const
{
	std::stringstream result;

	/* Form the pre-amble */
	result << "#version 400\n"
			  "\n"
			  "#extension GL_ARB_shader_subroutine : require\n"
			  "\n"
			  "layout(vertices = 4) out;\n"
			  "\n"
			  "subroutine void testSubroutineType(out vec4 test_argument);\n"
			  "\n"
			  /* Define a subroutine */
			  "subroutine(testSubroutineType) void tc_subroutine(out vec4 test_argument)\n"
			  "{\n"
			  "    test_argument = vec4(0, 0, 1, 0);\n"
			  "}\n"
			  "\n"
			  /* Define uniforms */
			  "subroutine uniform testSubroutineType test_tc_subroutine;\n"
			  "\n"
			  /* Define main() */
			  "void main()\n"
			  "{\n"
			  "    "
		   << getSubroutineUniformName(referencing_stage) << "(gl_out[gl_InvocationID].gl_Position);\n"
															 "}\n";

	return result.str();
}

/** Retrieves an offending tessellation evaluation shader body.
 *
 *  @param referencing_stage Shader stage which defines the subroutine uniform that
 *                           will be called from tessellation evaluation shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest2::getTessellationEvaluationShaderBody(const Utils::_shader_stage& referencing_stage) const
{
	std::stringstream result;

	/* Form the pre-amble */
	result << "#version 400\n"
			  "\n"
			  "#extension GL_ARB_shader_subroutine : require\n"
			  "\n"
			  "layout(quads) in;\n"
			  "\n"
			  "subroutine void testSubroutineType(out vec4 test_argument);\n"
			  "\n"
			  /* Define a subroutine */
			  "subroutine(testSubroutineType) void te_subroutine(out vec4 test_argument)\n"
			  "{\n"
			  "    test_argument = vec4(1, 1, 1, 1);\n"
			  "}\n"
			  "\n"
			  /* Define uniforms */
			  "subroutine uniform testSubroutineType test_te_subroutine;\n"
			  "\n"
			  /* Define main() */
			  "void main()\n"
			  "{\n"
			  "    "
		   << getSubroutineUniformName(referencing_stage) << "(gl_Position);\n"
															 "}\n";

	return result.str();
}

/** Retrieves an offending vertex shader body.
 *
 *  @param referencing_stage Shader stage which defines the subroutine uniform that
 *                           will be called from vertex shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest2::getVertexShaderBody(const Utils::_shader_stage& referencing_stage) const
{
	std::stringstream result;

	/* Form the pre-amble */
	result << "#version 400\n"
			  "\n"
			  "#extension GL_ARB_shader_subroutine : require\n"
			  "\n"
			  "subroutine void testSubroutineType(out vec4 test_argument);\n"
			  "\n"
			  /* Define a subroutine */
			  "subroutine(testSubroutineType) void vs_subroutine(out vec4 test_argument)\n"
			  "{\n"
			  "    test_argument = vec4(0, 1, 0, 0);\n"
			  "}\n"
			  "\n"
			  /* Define uniforms */
			  "subroutine uniform testSubroutineType test_vs_subroutine;\n"
			  "\n"
			  /* Define main() */
			  "void main()\n"
			  "{\n"
			  "    "
		   << getSubroutineUniformName(referencing_stage) << "(gl_Position);\n"
															 "}\n";

	return result.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest2::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages and execute the checks */
	for (int referencing_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 referencing_stage < static_cast<int>(Utils::SHADER_STAGE_COUNT); ++referencing_stage)
	{
		executeTestCase(static_cast<Utils::_shader_stage>(referencing_stage));
	} /* for (all test cases) */

	/* All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest3::NegativeTest3(deqp::Context& context)
	: TestCase(context, "missing_subroutine_keyword", "Verifies that subroutine keyword is necessary when declaring a "
													  "subroutine uniforn and a compilation error occurs without it.")
	, m_has_test_passed(true)
	, m_so_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest3::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_so_id != 0)
	{
		gl.deleteShader(m_so_id);

		m_so_id = 0;
	}
}

/** Verifies that broken shader (for user-specified shader stage) does not compile.
 *
 *  @param shader_stage Shader stage to use for the test.
 **/
void NegativeTest3::executeTest(const Utils::_shader_stage& shader_stage)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a new shader object */
	m_so_id = gl.createShader(Utils::getGLenumForShaderStage(shader_stage));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Assign body to the shader */
	std::string body;
	const char* body_raw_ptr = DE_NULL;

	switch (shader_stage)
	{
	case Utils::SHADER_STAGE_VERTEX:
		body = getVertexShaderBody();
		break;
	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
		body = getTessellationControlShaderBody();
		break;
	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
		body = getTessellationEvaluationShaderBody();
		break;
	case Utils::SHADER_STAGE_GEOMETRY:
		body = getGeometryShaderBody();
		break;
	case Utils::SHADER_STAGE_FRAGMENT:
		body = getFragmentShaderBody();
		break;

	default:
	{
		TCU_FAIL("Unrecognized shader stage requested");
	}
	} /* switch (shader_stage) */

	body_raw_ptr = body.c_str();

	gl.shaderSource(m_so_id, 1 /* count */, &body_raw_ptr, DE_NULL /* length */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Try to compile the shader */
	glw::GLint compile_status = 0;

	gl.compileShader(m_so_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_so_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status == GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "The following shader was expected to fail to compile but was "
													   "accepted by the compiler:\n"
													   "\n"
						   << body.c_str() << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}

	/* Good to release the shader at this point */
	gl.deleteShader(m_so_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed.");
}

/** Retrieves body of a broken fragment shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest3::getFragmentShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "subroutine void testSubroutineType(inout vec4 test);\n"
		   "\n"
		   "void testSubroutine1(inout vec4 test)\n"
		   "{\n"
		   "    test += vec4(3, 4, 5, 6);\n"
		   "}\n"
		   "\n"
		   "uniform testSubroutineType subroutineFunction;\n"
		   "out     vec4               result;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    vec4 test = vec4(2, 3, 4, 5);\n"
		   "\n"
		   "    subroutineFunction(test);\n"
		   "\n"
		   "    result = test;\n"
		   "}\n";
}

/** Retrieves body of a broken geometry shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest3::getGeometryShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout(points)                   in;\n"
		   "layout(points, max_vertices = 1) out;\n"
		   "\n"
		   "subroutine void testSubroutineType(inout vec4 test);\n"
		   "\n"
		   "void testSubroutine1(inout vec4 test)\n"
		   "{\n"
		   "    test += vec4(3, 4, 5, 6);\n"
		   "}\n"
		   "\n"
		   "uniform testSubroutineType subroutineFunction;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    vec4 test = vec4(2, 3, 4, 5);\n"
		   "\n"
		   "    subroutineFunction(test);\n"
		   "\n"
		   "    gl_Position = test;\n"
		   "    EmitVertex();\n"
		   "}\n";
}

/** Retrieves body of a broken tessellation control shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest3::getTessellationControlShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout(vertices=4) out;\n"
		   "\n"
		   "subroutine void testSubroutineType(inout vec4 test);\n"
		   "\n"
		   "void testSubroutine1(inout vec4 test)\n"
		   "{\n"
		   "    test += vec4(1, 2, 3, 4);\n"
		   "}\n"
		   "\n"
		   "uniform testSubroutineType subroutineFunction;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    vec4 test = vec4(0, 1, 2, 3);\n"
		   "\n"
		   "    subroutineFunction(test);\n"
		   "\n"
		   "    gl_out[gl_InvocationID].gl_Position = test;\n"
		   "}\n";
}

/** Retrieves body of a broken tessellation evaluation shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest3::getTessellationEvaluationShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "layout(quads) in;\n"
		   "\n"
		   "subroutine void testSubroutineType(inout vec4 test);\n"
		   "\n"
		   "void testSubroutine1(inout vec4 test)\n"
		   "{\n"
		   "    test += vec4(2, 3, 4, 5);\n"
		   "}\n"
		   "\n"
		   "uniform testSubroutineType subroutineFunction;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    vec4 test = vec4(1, 2, 3, 4);\n"
		   "\n"
		   "    subroutineFunction(test);\n"
		   "\n"
		   "    gl_Position = test;\n"
		   "}\n";
}

/** Retrieves body of a broken vertex shader.
 *
 *  @return Requested string.
 **/
std::string NegativeTest3::getVertexShaderBody() const
{
	return "#version 400\n"
		   "\n"
		   "#extension GL_ARB_shader_subroutine : require\n"
		   "\n"
		   "subroutine void testSubroutineType(inout vec4 test);\n"
		   "\n"
		   "void testSubroutine1(inout vec4 test)\n"
		   "{\n"
		   "    test += vec4(0, 1, 2, 3);\n"
		   "}\n"
		   "\n"
		   "uniform testSubroutineType subroutineFunction;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    subroutineFunction(gl_Position);\n"
		   "}\n";
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest3::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages */
	for (int shader_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 shader_stage < static_cast<int>(Utils::SHADER_STAGE_COUNT); ++shader_stage)
	{
		executeTest(static_cast<Utils::_shader_stage>(shader_stage));
	} /* for (all shader stages) */

	/* Done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest4::NegativeTest4(deqp::Context& context)
	: TestCase(context, "subroutines_incompatible_with_subroutine_type",
			   "Verifies that a compile-time error is generated when arguments and "
			   "return type do not match beween the function and each associated "
			   "subroutine type.")
	, m_has_test_passed(true)
	, m_so_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes GL objects that may have been created during test
 *  execution.
 **/
void NegativeTest4::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_so_id != 0)
	{
		gl.deleteShader(m_so_id);

		m_so_id = 0;
	}
}

/** Retrieves body of a shader of user-specified type that should be used
 *  for a single test iteration. The shader will define user-specified number
 *  of subroutine types, with the last type either defining an additional argument
 *  or using a different return type.
 *  A subroutine (claimed compatible with *all* subroutine types) will also be
 *  defined in the shader.
 *
 *  @param shader_stage       Shader stage to use for the query.
 *  @param n_subroutine_types Overall number of subroutine types that will be
 *                            declared & used in the shader. Please see description
 *                            for more details.
 *
 *  @return Requested string.
 **/
std::string NegativeTest4::getShaderBody(const Utils::_shader_stage& shader_stage,
										 const unsigned int& n_subroutine_types, const _test_case& test_case) const
{
	std::stringstream result_sstream;

	/* Form the pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	/* Inject stage-specific code */
	switch (shader_stage)
	{
	case Utils::SHADER_STAGE_GEOMETRY:
	{
		result_sstream << "layout (points) in;\n"
						  "layout (points, max_vertices = 1) out;\n"
						  "\n";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
	{
		result_sstream << "layout (vertices = 4) out;\n"
						  "\n";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
	{
		result_sstream << "layout (quads) in;\n"
						  "\n";

		break;
	}

	default:
		break;
	} /* switch (shader_stage) */

	/* Insert subroutine type declarations */
	for (unsigned int n_subroutine_type = 0; n_subroutine_type < n_subroutine_types - 1; ++n_subroutine_type)
	{
		result_sstream << "subroutine void subroutineType" << n_subroutine_type << "(inout vec3 argument);\n";
	} /* for (all subroutine types) */

	switch (test_case)
	{
	case TEST_CASE_INCOMPATIBLE_ARGUMENT_LIST:
	{
		result_sstream << "subroutine void subroutineType" << (n_subroutine_types - 1)
					   << "(inout vec3 argument, out vec4 argument2);\n";

		break;
	}

	case TEST_CASE_INCOMPATIBLE_RETURN_TYPE:
	{
		result_sstream << "subroutine int subroutineType" << (n_subroutine_types - 1) << "(inout vec3 argument);\n";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized test case");
	}
	} /* switch (test_case) */

	/* Insert subroutine declarations */
	result_sstream << "subroutine(";

	for (unsigned int n_subroutine_type = 0; n_subroutine_type < n_subroutine_types; ++n_subroutine_type)
	{
		result_sstream << "subroutineType" << n_subroutine_type;

		if (n_subroutine_type != (n_subroutine_types - 1))
		{
			result_sstream << ", ";
		}
	} /* for (all subroutine types) */

	result_sstream << ") void function(inout vec3 argument)\n"
					  "{\n"
					  "    argument = vec3(1, 2, 3);\n"
					  "}\n"
					  "\n";

	/* Insert remaining required stage-specific bits */
	switch (shader_stage)
	{
	case Utils::SHADER_STAGE_FRAGMENT:
	{
		result_sstream << "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = vec4(1, 2, 3, 4);\n"
						  "}\n";

		break;
	}

	case Utils::SHADER_STAGE_GEOMETRY:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1, 2, 3, 4);\n"
						  "    EmitVertex();\n"
						  "}\n";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_CONTROL:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    gl_TessLevelInner[0]                = 1;\n"
						  "    gl_TessLevelInner[1]                = 1;\n"
						  "    gl_TessLevelOuter[0]                = 1;\n"
						  "    gl_TessLevelOuter[1]                = 1;\n"
						  "    gl_TessLevelOuter[2]                = 1;\n"
						  "    gl_TessLevelOuter[3]                = 1;\n"
						  "    gl_out[gl_InvocationID].gl_Position = vec4(2, 3, 4, 5);\n"
						  "}\n";

		break;
	}

	case Utils::SHADER_STAGE_TESSELLATION_EVALUATION:
	case Utils::SHADER_STAGE_VERTEX:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1, 2, 3, 4);\n"
						  "}\n";

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized shader stage");
	}
	} /* switch (shader_stage) */

	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest4::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages.. */
	for (int shader_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 shader_stage != static_cast<int>(Utils::SHADER_STAGE_COUNT); ++shader_stage)
	{
		/* For each shader stage, we will be trying to compile a number of invalid shaders.
		 * Each shader defines N different subroutine types. (N-1) of them are compatible
		 * with a subroutine, but exactly 1 will be mismatched. The test passes if GLSL
		 * compiler correctly detects that all shaders we will be trying to compile are
		 * broken.
		 */
		const glw::GLenum shader_type = Utils::getGLenumForShaderStage(static_cast<Utils::_shader_stage>(shader_stage));

		for (unsigned int n_subroutine_types = 1; n_subroutine_types < 6; /* arbitrary number */
			 ++n_subroutine_types)
		{
			for (int test_case = static_cast<int>(TEST_CASE_FIRST); test_case != static_cast<int>(TEST_CASE_COUNT);
				 ++test_case)
			{
				std::string body;
				const char* body_raw_ptr   = NULL;
				glw::GLint  compile_status = GL_FALSE;

				body = getShaderBody(static_cast<Utils::_shader_stage>(shader_stage), n_subroutine_types,
									 static_cast<_test_case>(test_case));
				body_raw_ptr = body.c_str();

				/* Try to compile the shader */
				m_so_id = gl.createShader(shader_type);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

				gl.shaderSource(m_so_id, 1 /* count */, &body_raw_ptr, DE_NULL);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

				gl.compileShader(m_so_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

				gl.getShaderiv(m_so_id, GL_COMPILE_STATUS, &compile_status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

				if (compile_status == GL_TRUE)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "A malformed "
									   << Utils::getShaderStageString(static_cast<Utils::_shader_stage>(shader_stage))
									   << " compiled successfully "
										  "("
									   << n_subroutine_types << " subroutine types "
																"were defined)."
									   << tcu::TestLog::EndMessage;

					m_has_test_passed = false;
				}

				/* Release the object */
				gl.deleteShader(m_so_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed.");
			} /* for (all test cases) */
		}	 /* for (a number of different subroutine type declarations) */
	}		  /* for (all shader stages) */

	/* Done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest5::NegativeTest5(deqp::Context& context)
	: TestCase(context, "subroutine_uniform_wo_matching_subroutines",
			   "Verifies that a link- or compile-time error occurs when "
			   "trying to link a program with no subroutine for subroutine "
			   "uniform variable.")
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest5::deinit()
{
	deinitIteration();
}

/** Deinitializes all GL objects that may have been created during a single test
 *  iteration.
 ***/
void NegativeTest5::deinitIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration.
 *
 *  If the iteration fails, m_has_test_passed will be set to false.
 *
 *  @param shader_stage Shader stage, for which a subroutine uniform should be
 *                      declared in the shader without a matching subroutine.
 **/
void NegativeTest5::executeIteration(const Utils::_shader_stage& shader_stage)
{
	std::string fs_body = getFragmentShaderBody(shader_stage == Utils::SHADER_STAGE_FRAGMENT);
	std::string gs_body = getGeometryShaderBody(shader_stage == Utils::SHADER_STAGE_GEOMETRY);
	std::string tc_body = getTessellationControlShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_CONTROL);
	std::string te_body =
		getTessellationEvaluationShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_EVALUATION);
	std::string vs_body = getVertexShaderBody(shader_stage == Utils::SHADER_STAGE_VERTEX);

	if (Utils::buildProgram(m_context.getRenderContext().getFunctions(), vs_body, tc_body, te_body, gs_body, fs_body,
							DE_NULL, /* xfb_varyings */
							DE_NULL, /* n_xfb_varyings */
							&m_vs_id, &m_tc_id, &m_te_id, &m_gs_id, &m_fs_id, &m_po_id))
	{
		/* None of the test programs should ever build successfully */
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "A program object, consisting of the following shaders, has linked"
							  " correctly. One of the shaders defines a subroutine uniform but does "
							  "not implement any function that matches subroutine type of the uniform."
							  " This should have resulted in a compilation/link-time error.\n"
							  "\n"
							  "Vertex shader:\n"
							  "\n"
						   << vs_body << "\n"
										 "Tessellation control shader:\n"
										 "\n"
						   << tc_body << "\n"
										 "Tessellation evaluation shader:\n"
										 "\n"
						   << te_body << "\n"
										 "Geometry shader:\n"
										 "\n"
						   << gs_body << "\n"
										 "Fragment shader:\n"
										 "\n"
						   << fs_body << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}
}

/** Retrieves fragment shader body.
 *
 *  @param include_invalid_subroutine_uniform_declaration true if the shader should declare
 *                                                        a subroutine uniform without
 *                                                        a matching subroutine, false otherwise.
 *
 *  @return Requested string.
 **/
std::string NegativeTest5::getFragmentShaderBody(bool include_invalid_subroutine_uniform_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeFS(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeFS test_subroutineFS;\n";
	};

	result_sstream << "\n"
					  "out vec4 result;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "    test_subroutineFS(result);\n";
	}
	else
	{
		result_sstream << "    result = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves geometry shader body.
 *
 *  @param include_invalid_subroutine_uniform_declaration true if the shader should declare
 *                                                        a subroutine uniform without
 *                                                        a matching subroutine, false otherwise.
 *
 *  @return Requested string.
 **/
std::string NegativeTest5::getGeometryShaderBody(bool include_invalid_subroutine_uniform_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (points)                   in;\n"
					  "layout (points, max_vertices = 1) out;\n"
					  "\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeGS(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeGS test_subroutineGS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "    test_subroutineGS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "EmitVertex();\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation control shader body.
 *
 *  @param include_invalid_subroutine_uniform_declaration true if the shader should declare
 *                                                        a subroutine uniform without
 *                                                        a matching subroutine, false otherwise.
 *
 *  @return Requested string.
 **/
std::string NegativeTest5::getTessellationControlShaderBody(bool include_invalid_subroutine_uniform_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (vertices = 4) out;\n"
					  "\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTC(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTC test_subroutineTC;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "    test_subroutineTC(gl_out[gl_InvocationID].gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_out[gl_InvocationID].gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation evaluation body.
 *
 *  @param include_invalid_subroutine_uniform_declaration true if the shader should declare
 *                                                        a subroutine uniform without
 *                                                        a matching subroutine, false otherwise.
 *
 *  @return Requested string.
 **/
std::string NegativeTest5::getTessellationEvaluationShaderBody(
	bool include_invalid_subroutine_uniform_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (quads) in;\n"
					  "\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTE(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTE test_subroutineTE;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "    test_subroutineTE(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves vertex shader body.
 *
 *  @param include_invalid_subroutine_uniform_declaration true if the shader should declare
 *                                                        a subroutine uniform without
 *                                                        a matching subroutine, false otherwise.
 *
 *  @return Requested string.
 **/
std::string NegativeTest5::getVertexShaderBody(bool include_invalid_subroutine_uniform_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeVS(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeVS test_subroutineVS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_subroutine_uniform_declaration)
	{
		result_sstream << "    test_subroutineVS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest5::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages. Iteration-specific shader stage defines a subroutine type &
	 * a corresponding subroutine uniform, for which no compatible subroutines are available. All
	 * other shader stages are defined correctly.
	 */
	for (int shader_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 shader_stage < static_cast<int>(Utils::SHADER_STAGE_COUNT); ++shader_stage)
	{
		executeIteration(static_cast<Utils::_shader_stage>(shader_stage));
		deinitIteration();
	} /* for (all shader stages) */

	/* All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest6::NegativeTest6(deqp::Context& context)
	: TestCase(context, "two_duplicate_functions_one_being_a_subroutine",
			   "Verifies that a link- or compile-time error occurs if any shader in "
			   "a program object includes two functions with the same name and one "
			   "of which is associated with a subroutine type.")
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest6::deinit()
{
	deinitIteration();
}

/** Deinitializes all GL objects that may have been created during a single test
 *  iteration.
 ***/
void NegativeTest6::deinitIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration.
 *
 *  If the iteration fails, m_has_test_passed will be set to false.
 *
 *  @param shader_stage Shader stage, for which two duplicate functions
 *                      (one additionally marked as subroutine) should
 *                      be defined.
 **/
void NegativeTest6::executeIteration(const Utils::_shader_stage& shader_stage)
{
	std::string fs_body = getFragmentShaderBody(shader_stage == Utils::SHADER_STAGE_FRAGMENT);
	std::string gs_body = getGeometryShaderBody(shader_stage == Utils::SHADER_STAGE_GEOMETRY);
	std::string tc_body = getTessellationControlShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_CONTROL);
	std::string te_body =
		getTessellationEvaluationShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_EVALUATION);
	std::string vs_body = getVertexShaderBody(shader_stage == Utils::SHADER_STAGE_VERTEX);

	if (Utils::buildProgram(m_context.getRenderContext().getFunctions(), vs_body, tc_body, te_body, gs_body, fs_body,
							DE_NULL, /* xfb_varyings */
							DE_NULL, /* n_xfb_varyings */
							&m_vs_id, &m_tc_id, &m_te_id, &m_gs_id, &m_fs_id, &m_po_id))
	{
		/* None of the test programs should ever build successfully */
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "A program object, consisting of the following shaders, has linked"
							  " correctly. This is invalid, because one of the shaders defines two"
							  " functions with the same name, with an exception that one of the"
							  " functions is marked as a subroutine.\n"
							  "\n"
							  "Vertex shader:\n"
							  "\n"
						   << vs_body << "\n"
										 "Tessellation control shader:\n"
										 "\n"
						   << tc_body << "\n"
										 "Tessellation evaluation shader:\n"
										 "\n"
						   << te_body << "\n"
										 "Geometry shader:\n"
										 "\n"
						   << gs_body << "\n"
										 "Fragment shader:\n"
										 "\n"
						   << fs_body << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}
}

/** Retrieves fragment shader body.
 *
 *  @param include_invalid_declaration true if the shader should include duplicate function
 *                                     declaration.
 *
 *  @return Requested string.
 **/
std::string NegativeTest6::getFragmentShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeFS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeFS) void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(2, 3, 4, 5);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeFS test_subroutineFS;\n";
	};

	result_sstream << "\n"
					  "out vec4 result;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineFS(result);\n";
	}
	else
	{
		result_sstream << "    result = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves geometry shader body.
 *
 *  @param include_invalid_declaration true if the shader should include duplicate function
 *                                     declaration.
 *
 *  @return Requested string.
 **/
std::string NegativeTest6::getGeometryShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (points)                   in;\n"
					  "layout (points, max_vertices = 1) out;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeGS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeGS) void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(2, 3, 4, 5);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeGS test_subroutineGS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineGS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "EmitVertex();\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation control shader body.
 *
 *  @param include_invalid_declaration true if the shader should include duplicate function
 *                                     declaration.
 *
 *  @return Requested string.
 **/
std::string NegativeTest6::getTessellationControlShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (vertices = 4) out;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTC(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeTC) void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(2, 3, 4, 5);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTC test_subroutineTC;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineTC(gl_out[gl_InvocationID].gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_out[gl_InvocationID].gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation evaluation body.
 *
 *  @param include_invalid_declaration true if the shader should include duplicate function
 *                                     declaration.
 *
 *  @return Requested string.
 **/
std::string NegativeTest6::getTessellationEvaluationShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (quads) in;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTE(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeTE) void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(2, 3, 4, 5);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTE test_subroutineTE;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineTE(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves vertex shader body.
 *
 *  @param include_invalid_declaration true if the shader should include duplicate function
 *                                     declaration.
 *
 *  @return Requested string.
 **/
std::string NegativeTest6::getVertexShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeVS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeVS) void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(1, 2, 3, 4);\n"
						  "}\n"
						  "\n"
						  "void test_impl1(out vec4 test)\n"
						  "{\n"
						  "    test = vec4(2, 3, 4, 5);\n"
						  "}\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeVS test_subroutineVS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineVS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest6::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages. In each iteration, we will inject invalid
	 * duplicate function declarations to iteration-specific shader stage. All other
	 * shader stages will be assigned valid bodies. Test should fail if the program
	 * links successfully.
	 */
	for (int shader_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 shader_stage < static_cast<int>(Utils::SHADER_STAGE_COUNT); ++shader_stage)
	{
		executeIteration(static_cast<Utils::_shader_stage>(shader_stage));
		deinitIteration();
	} /* for (all shader stages) */

	/* All done */
	if (m_has_test_passed)
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
 * @param context CTS context
 **/
NegativeTest7::NegativeTest7(deqp::Context& context)
	: TestCase(context, "recursion", "Verify that it is not possible to build program with recursing subroutines")
	, m_program_id(0)
	, m_vertex_shader_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes all GL objects that may have been created during test execution
 *
 **/
void NegativeTest7::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 **/
tcu::TestNode::IterateResult NegativeTest7::iterate()
{
	static const GLchar* vertex_shader_with_static_recursion =
		"#version 400\n"
		"\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"subroutine vec4 routine_type(in vec4 data, in uint control);\n"
		"\n"
		"subroutine (routine_type) vec4 power_routine(in vec4 data, in uint control)\n"
		"{\n"
		"    if (0 != control)\n"
		"    {\n"
		"        return data * power_routine(data, control - 1);\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        return vec4(1, 1, 1, 1);\n"
		"    }\n"
		"}\n"
		"\n"
		"subroutine (routine_type) vec4 select_routine(in vec4 data, in uint control)\n"
		"{\n"
		"    if (0 == control)\n"
		"    {\n"
		"        return data.rrrr;\n"
		"    }\n"
		"    else if (1 == control)\n"
		"    {\n"
		"        return data.gggg;\n"
		"    }\n"
		"    else if (2 == control)\n"
		"    {\n"
		"        return data.bbbb;\n"
		"    }\n"
		"    else\n"
		"    {\n"
		"        return data.aaaa;\n"
		"    }\n"
		"}\n"
		"\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"uniform vec4 uni_value;\n"
		"uniform uint uni_control;\n"
		"\n"
		"out vec4 out_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_result = routine(uni_value, uni_control);\n"
		"}\n"
		"\n";

	static const GLchar* vertex_shader_with_dynamic_recursion =
		"#version 400\n"
		"\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"subroutine vec4 routine_type(in vec4 data);\n"
		"\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"subroutine (routine_type) vec4 div_by_2(in vec4 data)\n"
		"{\n"
		"    return data / 2;\n"
		"}\n"
		"\n"
		"subroutine (routine_type) vec4 div_routine_result_by_2(in vec4 data)\n"
		"{\n"
		"    return routine(data) / 2;\n"
		"}\n"
		"\n"
		"uniform vec4 uni_value;\n"
		"\n"
		"out vec4 out_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_result = routine(uni_value);\n"
		"}\n"
		"\n";

	static const GLchar* vertex_shader_with_subroutine_function_recursion =
		"#version 400\n"
		"\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"subroutine vec4 routine_type(in vec4 data);\n"
		"\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"vec4 function(in vec4 data)\n"
		"{\n"
		"    return routine(data) + vec4(0.5, 0.5, 0.5, 0.5);\n"
		"}\n"
		"\n"
		"subroutine (routine_type) vec4 routine_a(in vec4 data)\n"
		"{\n"
		"    return function(data) / 2;\n"
		"}\n"
		"\n"
		"subroutine (routine_type) vec4 routine_b(in vec4 data)\n"
		"{\n"
		"    return routine_a(data) * 2;\n"
		"}\n"
		"\n"
		"uniform vec4 uni_value;\n"
		"\n"
		"out vec4 out_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_result = routine(uni_value);\n"
		"}\n"
		"\n";

	bool result = true;

	if (false == test(vertex_shader_with_subroutine_function_recursion, "routine_a"))
	{
		result = false;
	}

	if (false == test(vertex_shader_with_dynamic_recursion, "div_routine_result_by_2"))
	{
		result = false;
	}

	if (false == test(vertex_shader_with_static_recursion, "power_routine"))
	{
		result = false;
	}

	/* Set result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Try to build program from vertex shader code.
 *
 * @param vertex_shader_code        Source code of vertex shader
 * @param name_of_recursive_routine Name of subroutine that should cause link failure due to recursion
 *
 * @return true build process failed, false otherwise
 **/
bool NegativeTest7::test(const GLchar* vertex_shader_code, const GLchar* name_of_recursive_routine)
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	bool				  result	   = true;
	static const GLchar*  varying_name = "out_result";

	/* Try to build program */
	if (true == Utils::buildProgram(gl, vertex_shader_code, "", "", "", "", &varying_name /* varying_names */,
									1 /* n_varyings */, &m_vertex_shader_id, 0, 0, 0, 0, &m_program_id))
	{
		/* Success is considered an error */

		Utils::program program(m_context);
		GLuint		   index = 0;

		program.build(0, 0, 0, 0, 0, vertex_shader_code, 0, 0);

		/* Verify that recursive subroutine is active */
		try
		{
			index = program.getSubroutineIndex(name_of_recursive_routine, GL_VERTEX_SHADER);
		}
		catch (const std::exception& exc)
		{
			/* Something wrong with shader or compilation */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "It is expected that subroutine: \n"
				<< name_of_recursive_routine
				<< " is considered active. This subroutine is potentially recursive and should cause link failure."
				<< tcu::TestLog::EndMessage;

			throw exc;
		}

		/* Subsoutine is active, however linking should fail */
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Error. Program with potentially recursive subroutine, "
			<< name_of_recursive_routine << ", which is active, index: " << index << ", has been built successfully.\n"
			<< vertex_shader_code << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Delete program and shader */
	deinit();

	/* Done */
	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
NegativeTest8::NegativeTest8(deqp::Context& context)
	: TestCase(context, "subroutine_wo_body", "Verifies that a compile- or link-time error occurs if a function "
											  "declared as a subroutine does not include a body.")
	, m_fs_id(0)
	, m_gs_id(0)
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes all GL objects that may have been created during test execution */
void NegativeTest8::deinit()
{
	deinitIteration();
}

/** Deinitializes all GL objects that may have been created during a single test
 *  iteration.
 ***/
void NegativeTest8::deinitIteration()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Executes a single test iteration.
 *
 *  If the iteration fails, m_has_test_passed will be set to false.
 *
 *  @param shader_stage Shader stage, for which two duplicate functions
 *                      (one additionally marked as subroutine) should
 *                      be defined.
 **/
void NegativeTest8::executeIteration(const Utils::_shader_stage& shader_stage)
{
	std::string fs_body = getFragmentShaderBody(shader_stage == Utils::SHADER_STAGE_FRAGMENT);
	std::string gs_body = getGeometryShaderBody(shader_stage == Utils::SHADER_STAGE_GEOMETRY);
	std::string tc_body = getTessellationControlShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_CONTROL);
	std::string te_body =
		getTessellationEvaluationShaderBody(shader_stage == Utils::SHADER_STAGE_TESSELLATION_EVALUATION);
	std::string vs_body = getVertexShaderBody(shader_stage == Utils::SHADER_STAGE_VERTEX);

	if (Utils::buildProgram(m_context.getRenderContext().getFunctions(), vs_body, tc_body, te_body, gs_body, fs_body,
							DE_NULL, /* xfb_varyings */
							DE_NULL, /* n_xfb_varyings */
							&m_vs_id, &m_tc_id, &m_te_id, &m_gs_id, &m_fs_id, &m_po_id))
	{
		/* None of the test programs should ever build successfully */
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "A program object consisting of FS+GS+TC+TE+VS stages has linked successfully, "
							  "even though one of the shaders only defines a subroutine that lacks any body."
							  "\n"
							  "Vertex shader:\n"
							  "\n"
						   << vs_body << "\n"
										 "Tessellation control shader:\n"
										 "\n"
						   << tc_body << "\n"
										 "Tessellation evaluation shader:\n"
										 "\n"
						   << te_body << "\n"
										 "Geometry shader:\n"
										 "\n"
						   << gs_body << "\n"
										 "Fragment shader:\n"
										 "\n"
						   << fs_body << tcu::TestLog::EndMessage;

		m_has_test_passed = false;
	}
}

/** Retrieves fragment shader body.
 *
 *  @param include_invalid_declaration true if a subroutine prototype should be included in
 *                                     the shader, false to skip it.
 *
 *  @return Requested string.
 **/
std::string NegativeTest8::getFragmentShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeFS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeFS) void test_impl1(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeFS test_subroutineFS;\n";
	};

	result_sstream << "\n"
					  "out vec4 result;\n"
					  "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineFS(result);\n";
	}
	else
	{
		result_sstream << "    result = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves geometry shader body.
 *
 *  @param include_invalid_declaration true if a subroutine prototype should be included in
 *                                     the shader, false to skip it.
 *
 *  @return Requested string.
 **/
std::string NegativeTest8::getGeometryShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (points)                   in;\n"
					  "layout (points, max_vertices = 1) out;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeGS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeGS) void test_impl1(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeGS test_subroutineGS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineGS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "EmitVertex();\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation control shader body.
 *
 *  @param include_invalid_declaration true if a subroutine prototype should be included in
 *                                     the shader, false to skip it.
 *
 *  @return Requested string.
 **/
std::string NegativeTest8::getTessellationControlShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (vertices = 4) out;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTC(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeTC) void test_impl1(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTC test_subroutineTC;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineTC(gl_out[gl_InvocationID].gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_out[gl_InvocationID].gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves tessellation evaluation body.
 *
 *  @param include_invalid_declaration true if a subroutine prototype should be included in
 *                                     the shader, false to skip it.
 *
 *  @return Requested string.
 **/
std::string NegativeTest8::getTessellationEvaluationShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (quads) in;\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeTE(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeTE) void test_impl1(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeTE test_subroutineTE;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineTE(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Retrieves vertex shader body.
 *
 *  @param include_invalid_declaration true if a subroutine prototype should be included in
 *                                     the shader, false to skip it.
 *
 *  @return Requested string.
 **/
std::string NegativeTest8::getVertexShaderBody(bool include_invalid_declaration) const
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n";

	if (include_invalid_declaration)
	{
		result_sstream << "subroutine void subroutineTestTypeVS(out vec4 test);\n"
						  "\n"
						  "subroutine(subroutineTestTypeVS) void test_impl1(out vec4 test);\n"
						  "\n"
						  "subroutine uniform subroutineTestTypeVS test_subroutineVS;\n";
	};

	result_sstream << "\n"
					  "void main()\n"
					  "{\n";

	if (include_invalid_declaration)
	{
		result_sstream << "    test_subroutineVS(gl_Position);\n";
	}
	else
	{
		result_sstream << "    gl_Position = vec4(0, 1, 2, 3);\n";
	}

	result_sstream << "}\n";

	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest8::iterate()
{
	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all shader stages. For each iteration, iteration-specific shader stage
	 * will feature an invalid subroutine definition. Other shader stages will be assigned
	 * valid bodies. The test fails if a program built of such shaders links successfully.
	 */
	for (int shader_stage = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 shader_stage < static_cast<int>(Utils::SHADER_STAGE_COUNT); ++shader_stage)
	{
		executeIteration(static_cast<Utils::_shader_stage>(shader_stage));
		deinitIteration();
	} /* for (all shader stages) */

	/* All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
NegativeTest9::NegativeTest9(deqp::Context& context)
	: TestCase(context, "subroutines_cannot_be_assigned_float_int_values_or_be_compared",
			   "Make sure it is not possible to assign float/int to subroutine "
			   "uniform and that subroutine uniform values cannot be compared.")
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes any GL objects that may have been created during
 *  test execution.
 **/
void NegativeTest9::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns a literal corresponding to user-specified test case enum.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest9::getTestCaseString(const _test_case& test_case)
{
	std::string result = "?";

	switch (test_case)
	{
	case TEST_CASE_INVALID_FLOAT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT:
		result = "TEST_CASE_INVALID_FLOAT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT";
		break;
	case TEST_CASE_INVALID_INT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT:
		result = "TEST_CASE_INVALID_INT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT";
		break;
	case TEST_CASE_INVALID_SUBROUTINE_UNIFORM_VALUE_COMPARISON:
		result = "TEST_CASE_INVALID_SUBROUTINE_UNIFORM_VALUE_COMPARISON";
		break;
	default:
		break;
	}

	return result;
}

/** Retrieves vertex shader body for user-specified test case.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest9::getVertexShader(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  /* Define a subroutine */
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test += vec4(0, 1, 2, 3);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	/* Include case-specific implementation */
	switch (test_case)
	{
	case TEST_CASE_INVALID_FLOAT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    function = 1.0f;\n"
						  "\n"
						  "    function(gl_Position);\n"
						  "}\n";

		break;
	}

	case TEST_CASE_INVALID_INT_TO_SUBROUTINE_UNIFORM_ASSIGNMENT:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    function = 1;\n"
						  "\n"
						  "    function(gl_Position);\n"
						  "}\n";

		break;
	}

	case TEST_CASE_INVALID_SUBROUTINE_UNIFORM_VALUE_COMPARISON:
	{
		result_sstream << "subroutine uniform subroutineType function2;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (function == function2)\n"
						  "    {\n"
						  "        function(gl_Position);\n"
						  "    }\n"
						  "    else\n"
						  "    {\n"
						  "        function2(gl_Position);\n"
						  "    }\n"
						  "}\n";

		break;
	}

	default:
		break;
	} /* switch (test_case) */

	/* Done */
	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest9::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all test cases */
	for (int test_case = static_cast<int>(TEST_CASE_FIRST); test_case != static_cast<int>(TEST_CASE_COUNT); ++test_case)
	{
		/* Try to build a program object using invalid vertex shader, specific to the
		 * iteration we're currently in */
		std::string vs_body = getVertexShader(static_cast<_test_case>(test_case));

		if (ShaderSubroutine::Utils::buildProgram(gl, vs_body, "",   /* tc_body */
												  "",				 /* te_body */
												  "",				 /* gs_body */
												  "",				 /* fs_body */
												  DE_NULL,			 /* xfb_varyings */
												  0,				 /* n_xfb_varyings */
												  &m_vs_id, DE_NULL, /* out_tc_id */
												  DE_NULL,			 /* out_te_id */
												  DE_NULL,			 /* out_gs_id */
												  DE_NULL,			 /* out_fs_id */
												  &m_po_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A program object was successfully built for ["
							   << getTestCaseString(static_cast<_test_case>(test_case))
							   << "] test case, even though it was invalid." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		/* Delete any objects that may have been created */
		deinit();
	} /* for (all test cases) */

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
NegativeTest10::NegativeTest10(deqp::Context& context)
	: TestCase(context, "function_overloading_forbidden_for_subroutines",
			   "Check that an overloaded function cannot be declared with subroutine and "
			   "a program will fail to compile or link if any shader or stage contains"
			   " two or more  functions with the same name if the name is associated with"
			   " a subroutine type.")
	, m_has_test_passed(true)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes any GL objects that may have been created during
 *  test execution.
 **/
void NegativeTest10::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Retrieves fragment shader that should be used for the purpose of the test.
 *  An overloaded version of a subroutine function is inserted if
 *  @param include_duplicate_function flag is set to true.
 *
 *  @param include_duplicate_function As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest10::getFragmentShader(bool include_duplicate_function)
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test = vec4(2, 3, 4, 5);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "out vec4 result;\n"
					  "\n";

	if (include_duplicate_function)
	{
		result_sstream << "void test_function(inout vec4 test)\n"
						  "{\n"
						  "    test = vec4(3, 4, 5, 6);\n"
						  "}\n"
						  "\n";
	}

	result_sstream << "void main()\n"
					  "{\n"
					  "    test_function(result);\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves geometry shader that should be used for the purpose of the test.
 *  An overloaded version of a subroutine function is inserted if
 *  @param include_duplicate_function flag is set to true.
 *
 *  @param include_duplicate_function As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest10::getGeometryShader(bool include_duplicate_function)
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (triangles)                        in;\n"
					  "layout (triangle_strip, max_vertices = 4) out;\n"
					  "\n"
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test = vec4(2, 3, 4, 5);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	if (include_duplicate_function)
	{
		result_sstream << "void test_function(inout vec4 test)\n"
						  "{\n"
						  "    test = vec4(3, 4, 5, 6);\n"
						  "}\n"
						  "\n";
	}

	result_sstream << "void main()\n"
					  "{\n"
					  "    function(gl_Position);\n"
					  "    EmitVertex();\n"
					  "    EndPrimitive();\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves tess control shader that should be used for the purpose of the test.
 *  An overloaded version of a subroutine function is inserted if
 *  @param include_duplicate_function flag is set to true.
 *
 *  @param include_duplicate_function As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest10::getTessellationControlShader(bool include_duplicate_function)
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (vertices = 4) out;\n"
					  "\n"
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test = vec4(2, 3, 4, 5);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	if (include_duplicate_function)
	{
		result_sstream << "void test_function(inout vec4 test)\n"
						  "{\n"
						  "    test = vec4(3, 4, 5, 6);\n"
						  "}\n"
						  "\n";
	}

	result_sstream << "void main()\n"
					  "{\n"
					  "    vec4 temp;\n"
					  "\n"
					  "    function(temp);\n"
					  "\n"
					  "    gl_out[gl_InvocationID].gl_Position = temp;\n"
					  "    gl_TessLevelInner[0]                = temp.x;\n"
					  "    gl_TessLevelInner[1]                = temp.y;\n"
					  "    gl_TessLevelOuter[0]                = temp.z;\n"
					  "    gl_TessLevelOuter[1]                = temp.w;\n"
					  "    gl_TessLevelOuter[2]                = temp.x;\n"
					  "    gl_TessLevelOuter[3]                = temp.y;\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves tess evaluation shader that should be used for the purpose of the test.
 *  An overloaded version of a subroutine function is inserted if
 *  @param include_duplicate_function flag is set to true.
 *
 *  @param include_duplicate_function As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest10::getTessellationEvaluationShader(bool include_duplicate_function)
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "layout (quads) in;\n"
					  "\n"
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test = vec4(2, 3, 4, 5);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	if (include_duplicate_function)
	{
		result_sstream << "void test_function(inout vec4 test)\n"
						  "{\n"
						  "    test = vec4(3, 4, 5, 6);\n"
						  "}\n"
						  "\n";
	}

	result_sstream << "void main()\n"
					  "{\n"
					  "    vec4 temp;\n"
					  "\n"
					  "    function(temp);\n"
					  "\n"
					  "    gl_Position = temp;\n"
					  "}\n";

	return result_sstream.str();
}

/** Retrieves vertex shader that should be used for the purpose of the test.
 *  An overloaded version of a subroutine function is inserted if
 *  @param include_duplicate_function flag is set to true.
 *
 *  @param include_duplicate_function As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest10::getVertexShader(bool include_duplicate_function)
{
	std::stringstream result_sstream;

	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test = vec4(2, 3, 4, 5);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	if (include_duplicate_function)
	{
		result_sstream << "void test_function(inout vec4 test)\n"
						  "{\n"
						  "    test = vec4(3, 4, 5, 6);\n"
						  "}\n"
						  "\n";
	}

	result_sstream << "void main()\n"
					  "{\n"
					  "    function(gl_Position);\n"
					  "}\n";

	return result_sstream.str();
}

/** Fills m_test_cases field with test case descriptors */
void NegativeTest10::initTestCases()
{
	/* For each test case, only one shader stage should define a function that
	 * has already been defined as a subroutine. */
	for (int offending_shader_stage_it = static_cast<int>(Utils::SHADER_STAGE_FIRST);
		 offending_shader_stage_it != static_cast<int>(Utils::SHADER_STAGE_COUNT); ++offending_shader_stage_it)
	{
		Utils::_shader_stage offending_shader_stage = static_cast<Utils::_shader_stage>(offending_shader_stage_it);
		/* Form the test case descriptor */
		std::stringstream name_sstream;
		_test_case		  test_case;

		name_sstream << "Broken shader stage:" << Utils::getShaderStageString(offending_shader_stage);

		test_case.fs_body = getFragmentShader(offending_shader_stage == Utils::SHADER_STAGE_FRAGMENT);
		test_case.gs_body = getGeometryShader(offending_shader_stage == Utils::SHADER_STAGE_GEOMETRY);
		test_case.name	= name_sstream.str();
		test_case.tc_body =
			getTessellationControlShader(offending_shader_stage == Utils::SHADER_STAGE_TESSELLATION_CONTROL);
		test_case.te_body =
			getTessellationEvaluationShader(offending_shader_stage == Utils::SHADER_STAGE_TESSELLATION_EVALUATION);
		test_case.vs_body = getVertexShader(offending_shader_stage == Utils::SHADER_STAGE_VERTEX);

		m_test_cases.push_back(test_case);
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest10::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Form test cases */
	initTestCases();

	/* Iterate over all test cases */
	for (_test_cases_const_iterator test_case_iterator = m_test_cases.begin(); test_case_iterator != m_test_cases.end();
		 ++test_case_iterator)
	{
		const _test_case& test_case = *test_case_iterator;

		/* Try to build the program object */
		if (ShaderSubroutine::Utils::buildProgram(gl, test_case.vs_body, test_case.tc_body, test_case.te_body,
												  test_case.gs_body, test_case.fs_body, DE_NULL, /* xfb_varyings */
												  0,											 /* n_xfb_varyings */
												  &m_vs_id, (test_case.tc_body.length() > 0) ? &m_tc_id : DE_NULL,
												  (test_case.te_body.length() > 0) ? &m_te_id : DE_NULL,
												  (test_case.gs_body.length() > 0) ? &m_gs_id : DE_NULL,
												  (test_case.fs_body.length() > 0) ? &m_fs_id : DE_NULL, &m_po_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A program object was successfully built for ["
							   << test_case.name << "] test case, even though it was invalid."
							   << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		/* Delete any objects that may have been created */
		deinit();
	} /* for (all test cases) */

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
NegativeTest11::NegativeTest11(deqp::Context& context)
	: TestCase(context, "subroutine_uniforms_used_for_sampling_atomic_image_functions",
			   "Tries to use subroutine uniforms in invalid way in sampling, "
			   "atomic and image functions. Verifies that compile- or link-time "
			   "error occurs.")
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes any GL objects that may have been created during
 *  test execution.
 **/
void NegativeTest11::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns a literal corresponding to user-specified test case enum.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest11::getTestCaseString(const _test_case& test_case)
{
	std::string result = "?";

	switch (test_case)
	{
	case TEST_CASE_INVALID_TEXTURE_SAMPLING_ATTEMPT:
		result = "TEST_CASE_INVALID_TEXTURE_SAMPLING_ATTEMPT";
		break;
	case TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT:
		result = "TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT";
		break;
	case TEST_CASE_INVALID_IMAGE_FUNCTION_USAGE_ATTEMPT:
		result = "TEST_CASE_INVALID_IMAGE_FUNCTION_USAGE_ATTEMPT";
		break;
	default:
		break;
	}

	return result;
}

/** Retrieves vertex shader body for user-specified test case.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest11::getVertexShader(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n";

	if (test_case == TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT)
	{
		result_sstream << "#extension GL_ARB_shader_atomic_counters : require\n";
	}

	result_sstream << "\n"
					  /* Define a subroutine */
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test += vec4(0, 1, 2, 3);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n"

					  /* Define main() body */
					  "void main()\n"
					  "{\n";

	/* Implement case-specific behavior */
	switch (test_case)
	{
	case TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT:
	{
		result_sstream << "if (atomicCounter(function) > 2)\n"
						  "{\n"
						  "    gl_Position = vec4(1);\n"
						  "}\n";

		break;
	}

	case TEST_CASE_INVALID_IMAGE_FUNCTION_USAGE_ATTEMPT:
	{
		result_sstream << "imageStore(function, vec2(0.0, 1.0), vec4(1.0) );\n";

		break;
	}

	case TEST_CASE_INVALID_TEXTURE_SAMPLING_ATTEMPT:
	{
		result_sstream << "gl_Position = texture(function, vec2(1.0) );\n";

		break;
	}

	default:
		break;
	} /* switch (test_case) */

	/* Close main() body */
	result_sstream << "}\n";

	/* Done */
	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest11::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all test cases */
	for (int test_case = static_cast<int>(TEST_CASE_FIRST); test_case != static_cast<int>(TEST_CASE_COUNT); ++test_case)
	{
		if (static_cast<_test_case>(test_case) == TEST_CASE_INVALID_ATOMIC_COUNTER_USAGE_ATTEMPT &&
			!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_atomic_counters"))
		{
			/* This iteration requires atomic counter support that this GL implementation
			 * is not capable of. Skip the iteration
			 */
			continue;
		}

		/* Try to build a program object using invalid vertex shader, specific to the
		 * iteration we're currently in */
		std::string vs_body = getVertexShader(static_cast<_test_case>(test_case));

		if (ShaderSubroutine::Utils::buildProgram(gl, vs_body, "",   /* tc_body */
												  "",				 /* te_body */
												  "",				 /* gs_body */
												  "",				 /* fs_body */
												  DE_NULL,			 /* xfb_varyings */
												  0,				 /* n_xfb_varyings */
												  &m_vs_id, DE_NULL, /* out_tc_id */
												  DE_NULL,			 /* out_te_id */
												  DE_NULL,			 /* out_gs_id */
												  DE_NULL,			 /* out_fs_id */
												  &m_po_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A program object was successfully built for ["
							   << getTestCaseString(static_cast<_test_case>(test_case))
							   << "] test case, even though it was invalid." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		/* Delete any objects that may have been created */
		deinit();
	} /* for (all test cases) */

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
NegativeTest12::NegativeTest12(deqp::Context& context)
	: TestCase(context, "subroutines_not_allowed_as_variables_constructors_and_argument_or_return_types",
			   "Verifies that it is not allowed to use subroutine type for "
			   "local/global variables, constructors or argument/return type.")
	, m_has_test_passed(true)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Deinitializes any GL objects that may have been created during
 *  test execution.
 **/
void NegativeTest12::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns a literal corresponding to user-specified test case enum.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest12::getTestCaseString(const _test_case& test_case)
{
	std::string result = "?";

	switch (test_case)
	{
	case TEST_CASE_INVALID_LOCAL_SUBROUTINE_VARIABLE:
		result = "TEST_CASE_INVALID_LOCAL_SUBROUTINE_VARIABLE";
		break;
	case TEST_CASE_INVALID_GLOBAL_SUBROUTINE_VARIABLE:
		result = "TEST_CASE_INVALID_GLOBAL_SUBROUTINE_VARIABLE";
		break;
	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR:
		result = "TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR";
		break;
	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_ARGUMENT:
		result = "TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_ARGUMENT";
		break;
	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_RETURN_TYPE:
		result = "TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_RETURN_TYPE";
		break;
	default:
		break;
	}

	return result;
}

/** Retrieves vertex shader body for user-specified test case.
 *
 *  @param test_case As per description.
 *
 *  @return Requested string.
 **/
std::string NegativeTest12::getVertexShader(const _test_case& test_case)
{
	std::stringstream result_sstream;

	/* Form pre-amble */
	result_sstream << "#version 400\n"
					  "\n"
					  "#extension GL_ARB_shader_subroutine : require\n"
					  "\n"
					  /* Define a subroutine */
					  "subroutine void subroutineType(inout vec4 test);\n"
					  "\n"
					  "subroutine(subroutineType) void test_function(inout vec4 test)\n"
					  "{\n"
					  "    test += vec4(0, 1, 2, 3);\n"
					  "}\n"
					  "\n"
					  "subroutine uniform subroutineType function;\n"
					  "\n";

	/* Include case-specific implementation */
	switch (test_case)
	{
	case TEST_CASE_INVALID_LOCAL_SUBROUTINE_VARIABLE:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    subroutine subroutineType function2;\n"
						  "    vec4                      result;\n"
						  "\n"
						  "    function2(result);\n"
						  "    gl_Position = result;\n"
						  "}\n";

		break;
	}

	case TEST_CASE_INVALID_GLOBAL_SUBROUTINE_VARIABLE:
	{
		result_sstream << "subroutine subroutineType function2;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    vec4 result;\n"
						  "\n"
						  "    function2(result);\n"
						  "    gl_Position = result;\n"
						  "}\n";

		break;
	}

	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR:
	{
		result_sstream << "void main()\n"
						  "{\n"
						  "    subroutineType(function);\n"
						  "}\n";

		break;
	}

	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_ARGUMENT:
	{
		result_sstream << "vec4 test_function(subroutineType argument)\n"
						  "{\n"
						  "    vec4 result = vec4(1, 2, 3, 4);\n"
						  "\n"
						  "    argument(result);\n"
						  "\n"
						  "    return result;\n"
						  "}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    test_function(function);\n"
						  "}\n";

		break;
	}

	case TEST_CASE_SUBROUTINE_USED_AS_CONSTRUCTOR_RETURN_TYPE:
	{
		result_sstream << "subroutineType test_function()\n"
						  "{\n"
						  "    return function;\n"
						  "}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    test_function()(gl_Position);\n"
						  "}\n";

		break;
	}

	default:
		break;
	} /* switch (test_case) */

	/* Done */
	return result_sstream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult NegativeTest12::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Iterate over all test cases */
	for (int test_case = static_cast<int>(TEST_CASE_FIRST); test_case != static_cast<int>(TEST_CASE_COUNT); ++test_case)
	{
		/* Try to build a program object using invalid vertex shader, specific to the
		 * iteration we're currently in */
		std::string vs_body = getVertexShader(static_cast<_test_case>(test_case));

		if (ShaderSubroutine::Utils::buildProgram(gl, vs_body, "",   /* tc_body */
												  "",				 /* te_body */
												  "",				 /* gs_body */
												  "",				 /* fs_body */
												  DE_NULL,			 /* xfb_varyings */
												  0,				 /* n_xfb_varyings */
												  &m_vs_id, DE_NULL, /* out_tc_id */
												  DE_NULL,			 /* out_te_id */
												  DE_NULL,			 /* out_gs_id */
												  DE_NULL,			 /* out_fs_id */
												  &m_po_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A program object was successfully built for ["
							   << getTestCaseString(static_cast<_test_case>(test_case))
							   << "] test case, even though it was invalid." << tcu::TestLog::EndMessage;

			m_has_test_passed = false;
		}

		/* Delete any objects that may have been created */
		deinit();
	} /* for (all test cases) */

	/** All done */
	if (m_has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} /* ShaderSubroutine */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
ShaderSubroutineTests::ShaderSubroutineTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_subroutine", "Verifies \"shader_subroutine\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void ShaderSubroutineTests::init(void)
{
	addChild(new ShaderSubroutine::APITest1(m_context));
	addChild(new ShaderSubroutine::APITest2(m_context));
	addChild(new ShaderSubroutine::FunctionalTest1_2(m_context));
	addChild(new ShaderSubroutine::FunctionalTest3_4(m_context));
	addChild(new ShaderSubroutine::FunctionalTest5(m_context));
	addChild(new ShaderSubroutine::FunctionalTest6(m_context));
	addChild(new ShaderSubroutine::FunctionalTest7_8(m_context));
	addChild(new ShaderSubroutine::FunctionalTest9(m_context));
	addChild(new ShaderSubroutine::FunctionalTest10(m_context));
	addChild(new ShaderSubroutine::FunctionalTest11(m_context));
	addChild(new ShaderSubroutine::FunctionalTest12(m_context));
	addChild(new ShaderSubroutine::FunctionalTest13(m_context));
	addChild(new ShaderSubroutine::FunctionalTest14_15(m_context));
	addChild(new ShaderSubroutine::FunctionalTest16(m_context));
	addChild(new ShaderSubroutine::FunctionalTest17(m_context));
	addChild(new ShaderSubroutine::FunctionalTest18_19(m_context));
	addChild(new ShaderSubroutine::NegativeTest1(m_context));
	addChild(new ShaderSubroutine::NegativeTest2(m_context));
	addChild(new ShaderSubroutine::NegativeTest3(m_context));
	addChild(new ShaderSubroutine::NegativeTest4(m_context));
	addChild(new ShaderSubroutine::NegativeTest5(m_context));
	addChild(new ShaderSubroutine::NegativeTest6(m_context));
	addChild(new ShaderSubroutine::NegativeTest7(m_context));
	addChild(new ShaderSubroutine::NegativeTest8(m_context));
	addChild(new ShaderSubroutine::NegativeTest9(m_context));
	addChild(new ShaderSubroutine::NegativeTest10(m_context));
	addChild(new ShaderSubroutine::NegativeTest11(m_context));
	addChild(new ShaderSubroutine::NegativeTest12(m_context));
}

} /* glcts namespace */
