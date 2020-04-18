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
 * \file  glcViewportArrayTests.cpp
 * \brief Implements conformance tests for "Viewport Array" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcViewportArrayTests.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

using namespace glw;

namespace glcts
{

namespace ViewportArray
{
/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::buffer::buffer(deqp::Context& context) : m_id(0), m_context(context), m_target(0)
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

/** Execute BindBuffer
 *
 **/
void Utils::buffer::bind() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");
}

/** Execute BindBufferRange
 *
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Utils::buffer::bindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBufferRange(m_target, index, m_id, offset, size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");
}

/** Execute GenBuffer
 *
 * @param target Target that will be used by this buffer
 **/
void Utils::buffer::generate(glw::GLenum target)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_target = target;

	gl.genBuffers(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");
}

/** Maps buffer content
 *
 * @param access Access rights for mapped region
 *
 * @return Mapped memory
 **/
void* Utils::buffer::map(GLenum access) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	void* result = gl.mapBuffer(m_target, access);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	return result;
}

/** Unmaps buffer
 *
 **/
void Utils::buffer::unmap() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	gl.unmapBuffer(m_target);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
}

/** Execute BufferData
 *
 * @param size   <size> parameter
 * @param data   <data> parameter
 * @param usage  <usage> parameter
 **/
void Utils::buffer::update(glw::GLsizeiptr size, glw::GLvoid* data, glw::GLenum usage)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(m_target, m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBuffer");

	gl.bufferData(m_target, size, data, usage);
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

	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture");

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

/** Specifies clear color
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

Utils::shaderCompilationException::shaderCompilationException(const glw::GLchar* source, const glw::GLchar* message)
	: m_shader_source(source), m_error_message(message)
{
	/* Nothing to be done */
}

const char* Utils::shaderCompilationException::what() const throw()
{
	return "Shader compilation failed";
}

Utils::programLinkageException::programLinkageException(const glw::GLchar* message) : m_error_message(message)
{
	/* Nothing to be done */
}

const char* Utils::programLinkageException::what() const throw()
{
	return "Program linking failed";
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

void Utils::program::compile(GLuint shader_id, const GLchar* source) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &source, 0 /* lengths */);
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

		throw shaderCompilationException(source, &message[0]);
	}
}

glw::GLint Utils::program::getAttribLocation(const glw::GLchar* name) const
{
	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint location = gl.getAttribLocation(m_program_object_id, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetAttribLocation");

	return location;
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

		throw programLinkageException(&message[0]);
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

void Utils::program::printShaderSource(const GLchar* source, tcu::MessageBuilder& log)
{
	GLuint line_number = 0;

	log << "Shader source.";

	log << "\nLine||Source\n";

	while (0 != source)
	{
		std::string   line;
		const GLchar* next_line = strchr(source, '\n');

		if (0 != next_line)
		{
			next_line += 1;
			line.assign(source, next_line - source);
		}
		else
		{
			line = source;
		}

		if (0 != *source)
		{
			log << std::setw(4) << line_number << "||" << line;
		}

		source = next_line;
		line_number += 1;
	}
}

/** Constructor.
 *
 * @param context CTS context.
 **/
Utils::texture::texture(deqp::Context& context)
	: m_id(0), m_width(0), m_height(0), m_depth(0), m_context(context), m_is_array(false)
{
	/* Nothing to done here */
}

/** Destructor
 *
 **/
Utils::texture::~texture()
{
	release();
}

/** Bind texture to GL_TEXTURE_2D
 *
 **/
void Utils::texture::bind() const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (false == m_is_array)
	{
		gl.bindTexture(GL_TEXTURE_2D, m_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	}
	else
	{
		gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
	}
}

/** Create 2d texture
 *
 * @param width           Width of texture
 * @param height          Height of texture
 * @param internal_format Internal format of texture
 **/
void Utils::texture::create(GLuint width, GLuint height, GLenum internal_format)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	release();

	m_width	= width;
	m_height   = height;
	m_depth	= 1;
	m_is_array = false;

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, internal_format, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
}

/** Create 2d texture array
 *
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param internal_format Internal format of texture
 **/
void Utils::texture::create(GLuint width, GLuint height, GLuint depth, GLenum internal_format)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	release();

	m_width	= width;
	m_height   = height;
	m_depth	= depth;
	m_is_array = true;

	gl.genTextures(1, &m_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	bind();

	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* levels */, internal_format, width, height, depth);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
}

/** Get contents of texture
 *
 * @param format   Format of image
 * @param type     Type of image
 * @param out_data Buffer for image
 **/
void Utils::texture::get(glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data) const
{
	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	bind();

	GLenum textarget = GL_TEXTURE_2D;

	if (true == m_is_array)
	{
		textarget = GL_TEXTURE_2D_ARRAY;
	}

	if (glu::isContextTypeGLCore(context_type))
	{
		gl.getTexImage(textarget, 0 /* level */, format, type, out_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));

		GLuint temp_fbo = 0;
		gl.genFramebuffers(1, &temp_fbo);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, temp_fbo);

		/* OpenGL ES only guarantees support for RGBA formats of each type.
		Since the tests are only expecting single-channel formats, we read them back
		in RGBA to a temporary buffer and then copy only the first component
		to the actual output buffer */
		GLenum read_format = format;
		switch (format)
		{
		case GL_RED:
			read_format = GL_RGBA;
			break;
		case GL_RED_INTEGER:
			read_format = GL_RGBA_INTEGER;
			break;
		default:
			TCU_FAIL("unexpected format");
		}
		/* we can get away just handling one type of data, as long as the components are the same size */
		if (type != GL_INT && type != GL_FLOAT)
		{
			TCU_FAIL("unexpected type");
		}
		std::vector<GLint> read_data;
		const GLuint	   layer_size = m_width * m_height * 4;
		read_data.resize(layer_size * m_depth);

		if (m_is_array)
		{
			for (GLuint layer = 0; layer < m_depth; ++layer)
			{
				gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_id, 0, layer);
				gl.readPixels(0, 0, m_width, m_height, read_format, type, &read_data[layer * layer_size]);
			}
		}
		else
		{
			gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textarget, m_id, 0);
			gl.readPixels(0, 0, m_width, m_height, read_format, type, &read_data[0]);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels");
		gl.deleteFramebuffers(1, &temp_fbo);

		/* copy the first channel from the readback buffer to the output buffer */
		GLint* out_data_int = (GLint*)out_data;
		for (GLuint elem = 0; elem < (m_width * m_height * m_depth); ++elem)
		{
			out_data_int[elem] = read_data[elem * 4];
		}
	}
}

/** Delete texture
 *
 **/
void Utils::texture::release()
{
	if (0 != m_id)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = 0;
	}
}

/** Update contents of texture
 *
 * @param width  Width of texture
 * @param height Height of texture
 * @param depth  Depth of texture
 * @param format Format of data
 * @param type   Type of data
 * @param data   Buffer with image
 **/
void Utils::texture::update(glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format,
							glw::GLenum type, glw::GLvoid* data)
{
	static const GLuint level = 0;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bind();

	if (false == m_is_array)
	{
		gl.texSubImage2D(GL_TEXTURE_2D, level, 0 /* x */, 0 /* y */, width, height, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
	}
	else
	{
		gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth, format,
						 type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
	}
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

/** Constructor
 *
 * @param context          Test context
 **/
APIErrors::APIErrors(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "api_errors", "Test verifies error generated by API")
{
	/* Nothing to be done here */
}

template <typename T>
void APIErrors::depthRangeArrayHelper(Utils::DepthFuncWrapper& depthFunc, GLint max_viewports, bool& test_result, T*)
{
	std::vector<T> data;
	data.resize(max_viewports * 2 /* near + far */);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		data[i * 2]		= (T)0.0;
		data[i * 2 + 1] = (T)1.0;
	}

	depthFunc.depthRangeArray(0, max_viewports - 1, &data[0]);
	checkGLError(GL_NO_ERROR, "depthRangeArray, correct parameters", test_result);

	depthFunc.depthRangeArray(max_viewports, 1, &data[0]);
	checkGLError(GL_INVALID_VALUE, "depthRangeArray, <first> == GL_MAX_VIEWPORTS", test_result);

	depthFunc.depthRangeArray(1, max_viewports - 1, &data[0]);
	checkGLError(GL_NO_ERROR, "depthRangeArray, <first> + <count> == GL_MAX_VIEWPORTS", test_result);

	depthFunc.depthRangeArray(1, max_viewports, &data[0]);
	checkGLError(GL_INVALID_VALUE, "depthRangeArray, <first> + <count> > GL_MAX_VIEWPORTS", test_result);
}

template <typename T>
void APIErrors::depthRangeIndexedHelper(Utils::DepthFuncWrapper& depthFunc, GLint max_viewports, bool& test_result, T*)
{
	depthFunc.depthRangeIndexed(0 /* index */, (T)0.0, (T)1.0);
	checkGLError(GL_NO_ERROR, "depthRangeIndexed, <index> == 0", test_result);

	depthFunc.depthRangeIndexed(max_viewports - 1 /* index */, (T)0.0, (T)1.0);
	checkGLError(GL_NO_ERROR, "depthRangeIndexed, <index> == GL_MAX_VIEWPORTS - 1", test_result);

	depthFunc.depthRangeIndexed(max_viewports /* index */, (T)0.0, (T)1.0);
	checkGLError(GL_INVALID_VALUE, "depthRangeIndexed, <index> == GL_MAX_VIEWPORTS", test_result);

	depthFunc.depthRangeIndexed(max_viewports + 1 /* index */, (T)0.0, (T)1.0);
	checkGLError(GL_INVALID_VALUE, "depthRangeIndexed, <index> > GL_MAX_VIEWPORTS", test_result);
}

template <typename T>
void APIErrors::getDepthHelper(Utils::DepthFuncWrapper& depthFunc, GLint max_viewports, bool& test_result, T*)
{
	T data[4];

	depthFunc.getDepthi_v(GL_DEPTH_RANGE, max_viewports - 1, data);
	checkGLError(GL_NO_ERROR, "getDouble/Floati_v, <index> == GL_MAX_VIEWPORTS - 1", test_result);

	depthFunc.getDepthi_v(GL_DEPTH_RANGE, max_viewports, data);
	checkGLError(GL_INVALID_VALUE, "getDouble/Floati_v, <index> == GL_MAX_VIEWPORTS", test_result);
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult APIErrors::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL entry points */
	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();
	Utils::DepthFuncWrapper depthFunc(m_context);

	/* Test result */
	bool test_result = true;

	GLint max_viewports = 0;
	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/*
	 *   * DepthRangeArrayv generates INVALID_VALUE when <first> + <count> is greater
	 *   than or equal to the value of MAX_VIEWPORTS;
	 */
	if (glu::isContextTypeGLCore(context_type))
	{
		depthRangeArrayHelper<GLdouble>(depthFunc, max_viewports, test_result);
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		depthRangeArrayHelper<GLfloat>(depthFunc, max_viewports, test_result);
	}

	/*
	 *   * DepthRangeIndexed generates INVALID_VALUE when <index> is greater than or
	 *   equal to the value of MAX_VIEWPORTS;
	 */
	if (glu::isContextTypeGLCore(context_type))
	{
		depthRangeIndexedHelper<GLdouble>(depthFunc, max_viewports, test_result);
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		depthRangeIndexedHelper<GLfloat>(depthFunc, max_viewports, test_result);
	}

	/*
	 *   * ViewportArrayv generates INVALID_VALUE when <first> + <count> is greater
	 *   than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		std::vector<GLfloat> data;
		data.resize(max_viewports * 4 /* x + y + w + h */);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			data[i * 4 + 0] = 0.0f;
			data[i * 4 + 1] = 0.0f;
			data[i * 4 + 2] = 1.0f;
			data[i * 4 + 3] = 1.0f;
		}

		gl.viewportArrayv(0, max_viewports - 1, &data[0]);
		checkGLError(GL_NO_ERROR, "viewportArrayv, correct parameters", test_result);

		gl.viewportArrayv(max_viewports, 1, &data[0]);
		checkGLError(GL_INVALID_VALUE, "viewportArrayv, <first> == GL_MAX_VIEWPORTS", test_result);

		gl.viewportArrayv(1, max_viewports - 1, &data[0]);
		checkGLError(GL_NO_ERROR, "viewportArrayv, <first> + <count> == GL_MAX_VIEWPORTS", test_result);

		gl.viewportArrayv(1, max_viewports, &data[0]);
		checkGLError(GL_INVALID_VALUE, "viewportArrayv, <first> + <count> > GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * ViewportIndexedf and ViewportIndexedfv generate INVALID_VALUE when <index>
	 *   is greater than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		GLfloat data[4 /* x + y + w + h */];

		data[0] = 0.0f;
		data[1] = 0.0f;
		data[2] = 1.0f;
		data[3] = 1.0f;

		gl.viewportIndexedf(0 /* index */, 0.0f, 0.0f, 1.0f, 1.0f);
		checkGLError(GL_NO_ERROR, "viewportIndexedf, <index> == 0", test_result);

		gl.viewportIndexedf(max_viewports - 1 /* index */, 0.0f, 0.0f, 1.0f, 1.0f);
		checkGLError(GL_NO_ERROR, "viewportIndexedf, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.viewportIndexedf(max_viewports /* index */, 0.0f, 0.0f, 1.0f, 1.0f);
		checkGLError(GL_INVALID_VALUE, "viewportIndexedf, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.viewportIndexedf(max_viewports + 1 /* index */, 0.0f, 0.0f, 1.0f, 1.0f);
		checkGLError(GL_INVALID_VALUE, "viewportIndexedf, <index> > GL_MAX_VIEWPORTS", test_result);

		gl.viewportIndexedfv(0 /* index */, data);
		checkGLError(GL_NO_ERROR, "viewportIndexedfv, <index> == 0", test_result);

		gl.viewportIndexedfv(max_viewports - 1 /* index */, data);
		checkGLError(GL_NO_ERROR, "viewportIndexedfv, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.viewportIndexedfv(max_viewports /* index */, data);
		checkGLError(GL_INVALID_VALUE, "viewportIndexedfv, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.viewportIndexedfv(max_viewports + 1 /* index */, data);
		checkGLError(GL_INVALID_VALUE, "viewportIndexedfv, <index> > GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * ViewportArrayv, Viewport, ViewportIndexedf and ViewportIndexedfv generate
	 *   INVALID_VALUE when <w> or <h> values are negative;
	 */
	{
		gl.viewport(0, 0, -1, 1);
		checkGLError(GL_INVALID_VALUE, "viewport, negative width", test_result);

		gl.viewport(0, 0, 1, -1);
		checkGLError(GL_INVALID_VALUE, "viewport, negative height", test_result);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			std::vector<GLfloat> data;
			data.resize(max_viewports * 4 /* x + y + w + h */);

			for (GLint j = 0; j < max_viewports; ++j)
			{
				data[j * 4 + 0] = 0.0f;
				data[j * 4 + 1] = 0.0f;
				data[j * 4 + 2] = 1.0f;
				data[j * 4 + 3] = 1.0f;
			}

			/* Set width to -1 */
			data[i * 4 + 2] = -1.0f;

			gl.viewportArrayv(0, max_viewports, &data[0]);
			checkGLError(GL_INVALID_VALUE, "viewportArrayv, negative width", test_result);

			gl.viewportIndexedf(i /* index */, 0.0f, 0.0f, -1.0f, 1.0f);
			checkGLError(GL_INVALID_VALUE, "viewportIndexedf, negative width", test_result);

			gl.viewportIndexedfv(i /* index */, &data[i * 4]);
			checkGLError(GL_INVALID_VALUE, "viewportIndexedfv, negative width", test_result);

			/* Set width to 1 and height to -1*/
			data[i * 4 + 2] = 1.0f;
			data[i * 4 + 3] = -1.0f;

			gl.viewportArrayv(0, max_viewports, &data[0]);
			checkGLError(GL_INVALID_VALUE, "viewportArrayv, negative height", test_result);

			gl.viewportIndexedf(i /* index */, 0.0f, 0.0f, 1.0f, -1.0f);
			checkGLError(GL_INVALID_VALUE, "viewportIndexedf, negative height", test_result);

			gl.viewportIndexedfv(i /* index */, &data[i * 4]);
			checkGLError(GL_INVALID_VALUE, "viewportIndexedfv, negative height", test_result);
		}
	}

	/*
	 *   * ScissorArrayv generates INVALID_VALUE when <first> + <count> is greater
	 *   than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		std::vector<GLint> data;
		data.resize(max_viewports * 4 /* x + y + w + h */);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			data[i * 4 + 0] = 0;
			data[i * 4 + 1] = 0;
			data[i * 4 + 2] = 1;
			data[i * 4 + 3] = 1;
		}

		gl.scissorArrayv(0, max_viewports - 1, &data[0]);
		checkGLError(GL_NO_ERROR, "scissorArrayv, correct parameters", test_result);

		gl.scissorArrayv(max_viewports, 1, &data[0]);
		checkGLError(GL_INVALID_VALUE, "scissorArrayv, <first> == GL_MAX_VIEWPORTS", test_result);

		gl.scissorArrayv(1, max_viewports - 1, &data[0]);
		checkGLError(GL_NO_ERROR, "scissorArrayv, <first> + <count> == GL_MAX_VIEWPORTS", test_result);

		gl.scissorArrayv(1, max_viewports, &data[0]);
		checkGLError(GL_INVALID_VALUE, "scissorArrayv, <first> + <count> > GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * ScissorIndexed and ScissorIndexedv generate INVALID_VALUE when <index> is
	 *   greater than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		GLint data[4 /* x + y + w + h */];

		data[0] = 0;
		data[1] = 0;
		data[2] = 1;
		data[3] = 1;

		gl.scissorIndexed(0 /* index */, 0, 0, 1, 1);
		checkGLError(GL_NO_ERROR, "scissorIndexed, <index> == 0", test_result);

		gl.scissorIndexed(max_viewports - 1 /* index */, 0, 0, 1, 1);
		checkGLError(GL_NO_ERROR, "scissorIndexed, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.scissorIndexed(max_viewports /* index */, 0, 0, 1, 1);
		checkGLError(GL_INVALID_VALUE, "scissorIndexed, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.scissorIndexed(max_viewports + 1 /* index */, 0, 0, 1, 1);
		checkGLError(GL_INVALID_VALUE, "scissorIndexed, <index> > GL_MAX_VIEWPORTS", test_result);

		gl.scissorIndexedv(0 /* index */, data);
		checkGLError(GL_NO_ERROR, "scissorIndexedv, <index> == 0", test_result);

		gl.scissorIndexedv(max_viewports - 1 /* index */, data);
		checkGLError(GL_NO_ERROR, "scissorIndexedv, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.scissorIndexedv(max_viewports /* index */, data);
		checkGLError(GL_INVALID_VALUE, "scissorIndexedv, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.scissorIndexedv(max_viewports + 1 /* index */, data);
		checkGLError(GL_INVALID_VALUE, "scissorIndexedv, <index> > GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * ScissorArrayv, ScissorIndexed, ScissorIndexedv and Scissor generate
	 *   INVALID_VALUE when <width> or <height> values are negative;
	 */
	{
		gl.scissor(0, 0, -1, 1);
		checkGLError(GL_INVALID_VALUE, "scissor, negative width", test_result);

		gl.scissor(0, 0, 1, -1);
		checkGLError(GL_INVALID_VALUE, "scissor, negative height", test_result);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			std::vector<GLint> data;
			data.resize(max_viewports * 4 /* x + y + w + h */);

			for (GLint j = 0; j < max_viewports; ++j)
			{
				data[j * 4 + 0] = 0;
				data[j * 4 + 1] = 0;
				data[j * 4 + 2] = 1;
				data[j * 4 + 3] = 1;
			}

			/* Set width to -1 */
			data[i * 4 + 2] = -1;

			gl.scissorArrayv(0, max_viewports, &data[0]);
			checkGLError(GL_INVALID_VALUE, "scissorArrayv, negative width", test_result);

			gl.scissorIndexed(i /* index */, 0, 0, -1, 1);
			checkGLError(GL_INVALID_VALUE, "scissorIndexed, negative width", test_result);

			gl.scissorIndexedv(i /* index */, &data[i * 4]);
			checkGLError(GL_INVALID_VALUE, "scissorIndexedv, negative width", test_result);

			/* Set width to 1 and height to -1*/
			data[i * 4 + 2] = 1;
			data[i * 4 + 3] = -1;

			gl.scissorArrayv(0, max_viewports, &data[0]);
			checkGLError(GL_INVALID_VALUE, "scissorArrayv, negative height", test_result);

			gl.scissorIndexed(i /* index */, 0, 0, 1, -1);
			checkGLError(GL_INVALID_VALUE, "scissorIndexed, negative height", test_result);

			gl.scissorIndexedv(i /* index */, &data[i * 4]);
			checkGLError(GL_INVALID_VALUE, "scissorIndexedv, negative height", test_result);
		}
	}

	/*
	 *   * Disablei, Enablei and IsEnabledi generate INVALID_VALUE when <cap> is
	 *   SCISSOR_TEST and <index> is greater than or equal to the
	 *   value of MAX_VIEWPORTS;
	 */
	{
		gl.disablei(GL_SCISSOR_TEST, max_viewports - 1);
		checkGLError(GL_NO_ERROR, "disablei, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.disablei(GL_SCISSOR_TEST, max_viewports);
		checkGLError(GL_INVALID_VALUE, "disablei, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.enablei(GL_SCISSOR_TEST, max_viewports - 1);
		checkGLError(GL_NO_ERROR, "enablei, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.enablei(GL_SCISSOR_TEST, max_viewports);
		checkGLError(GL_INVALID_VALUE, "enablei, <index> == GL_MAX_VIEWPORTS", test_result);

		gl.isEnabledi(GL_SCISSOR_TEST, max_viewports - 1);
		checkGLError(GL_NO_ERROR, "isEnabledi, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.isEnabledi(GL_SCISSOR_TEST, max_viewports);
		checkGLError(GL_INVALID_VALUE, "isEnabledi, <index> == GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * GetIntegeri_v generates INVALID_VALUE when <target> is SCISSOR_BOX and
	 *   <index> is greater than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		GLint data[4];

		gl.getIntegeri_v(GL_SCISSOR_BOX, max_viewports - 1, data);
		checkGLError(GL_NO_ERROR, "getIntegeri_v, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.getIntegeri_v(GL_SCISSOR_BOX, max_viewports, data);
		checkGLError(GL_INVALID_VALUE, "getIntegeri_v, <index> == GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * GetFloati_v generates INVALID_VALUE when <target> is VIEWPORT and <index>
	 *   is greater than or equal to the value of MAX_VIEWPORTS;
	 */
	{
		GLfloat data[4];

		gl.getFloati_v(GL_VIEWPORT, max_viewports - 1, data);
		checkGLError(GL_NO_ERROR, "getFloati_v, <index> == GL_MAX_VIEWPORTS - 1", test_result);

		gl.getFloati_v(GL_VIEWPORT, max_viewports, data);
		checkGLError(GL_INVALID_VALUE, "getFloati_v, <index> == GL_MAX_VIEWPORTS", test_result);
	}

	/*
	 *   * GetDoublei_v generates INVALID_VALUE when <target> is DEPTH_RANGE and
	 *   <index> is greater than or equal to the value of MAX_VIEWPORTS;
	 */
	if (glu::isContextTypeGLCore(context_type))
	{
		getDepthHelper<GLdouble>(depthFunc, max_viewports, test_result);
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		getDepthHelper<GLfloat>(depthFunc, max_viewports, test_result);
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

/** Check if glGetError returns expected error
 *
 * @param expected_error Expected error code
 * @param description    Description of test case
 * @param out_result     Set to false if the current error is not equal to expected one
 **/
void APIErrors::checkGLError(GLenum expected_error, const GLchar* description, bool& out_result)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLenum error = gl.getError();

	if (expected_error != error)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case fail. Description: " << description
											<< " Invalid error: " << glu::getErrorStr(error)
											<< " expected: " << glu::getErrorStr(expected_error)
											<< tcu::TestLog::EndMessage;

		out_result = false;
	}
}

/** Constructor
 *
 * @param context          Test context
 **/
Queries::Queries(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "queries", "Test verifies initial state of API")
{
	/* Nothing to be done here */
}

template <typename T>
void Queries::depthRangeInitialValuesHelper(Utils::DepthFuncWrapper& depthFunc, GLint max_viewports, bool& test_result,
											T*)
{
	std::vector<T> data;
	data.resize(max_viewports * 2 /* near + far */);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		depthFunc.getDepthi_v(GL_DEPTH_RANGE, i, &data[i * 2]);
		GLU_EXPECT_NO_ERROR(depthFunc.getFunctions().getError(), "getDouble/Floati_v");
	}

	for (GLint i = 0; i < max_viewports; ++i)
	{
		GLint near = (GLint)data[2 * i + 0];
		GLint far  = (GLint)data[2 * i + 1];

		if ((0.0 != near) || (1.0 != far))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid initial depth range [" << i
												<< "]: " << near << " : " << far << " expected: 0.0 : 1.0"
												<< tcu::TestLog::EndMessage;

			test_result = false;
			break;
		}
	}
}
/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult Queries::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL entry points */
	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();
	Utils::DepthFuncWrapper depthFunc(m_context);

	/* Test result */
	bool test_result = true;

	GLint   layer_provoking_vertex	= 0;
	GLint   max_viewports			  = 0;
	GLfloat max_renderbuffer_size	 = 0.0f;
	GLfloat max_viewport_dims[2]	  = { 0.0f, 0.0f };
	GLfloat viewport_bounds_range[2]  = { 0.0, 0.0f };
	GLint   viewport_provoking_vertex = 0;
	GLint   viewport_subpixel_bits	= -1;

	gl.getIntegerv(GL_LAYER_PROVOKING_VERTEX, &layer_provoking_vertex);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getFloatv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getFloatv");

	gl.getFloatv(GL_MAX_VIEWPORT_DIMS, max_viewport_dims);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetFloatv");

	gl.getFloatv(GL_VIEWPORT_BOUNDS_RANGE, viewport_bounds_range);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetFloatv");

	gl.getIntegerv(GL_VIEWPORT_INDEX_PROVOKING_VERTEX, &viewport_provoking_vertex);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getIntegerv(GL_VIEWPORT_SUBPIXEL_BITS, &viewport_subpixel_bits);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	const GLint window_width  = m_context.getRenderContext().getRenderTarget().getWidth();
	const GLint window_height = m_context.getRenderContext().getRenderTarget().getHeight();

	/*
	 *   * Initial dimensions of VIEWPORT returned by GetFloati_v match dimensions of
	 *   the window into which GL is rendering;
	 */
	{
		std::vector<GLfloat> data;
		data.resize(max_viewports * 4 /* x + y + w+ h */);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			gl.getFloati_v(GL_VIEWPORT, i, &data[i * 4]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetFloati_v");
		}

		for (GLint i = 0; i < max_viewports; ++i)
		{
			GLint viewport_width  = (GLint)data[4 * i + 2];
			GLint viewport_height = (GLint)data[4 * i + 3];

			if ((window_width != viewport_width) || (window_height != viewport_height))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid initial viewport [" << i
													<< "] dimennsions: " << viewport_width << " x " << viewport_height
													<< " expected: " << window_width << " x " << window_height
													<< tcu::TestLog::EndMessage;

				test_result = false;
				break;
			}
		}
	}

	/*
	 *   * Initial values of DEPTH_RANGE returned by GetDoublei_v are [0, 1];
	 */
	if (glu::isContextTypeGLCore(context_type))
	{
		depthRangeInitialValuesHelper<GLdouble>(depthFunc, max_viewports, test_result);
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		depthRangeInitialValuesHelper<GLfloat>(depthFunc, max_viewports, test_result);
	}

	/*
	 *   * Initial state of SCISSOR_TEST returned by IsEnabledi is FALSE;
	 */
	{
		for (GLint i = 0; i < max_viewports; ++i)
		{
			if (GL_FALSE != gl.isEnabledi(GL_SCISSOR_TEST, i))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Scissor test is enabled at " << i
													<< ". Expected disabled." << tcu::TestLog::EndMessage;

				test_result = false;
				break;
			}
		}
	}

	/*
	 *   * Initial dimensions of SCISSOR_BOX returned by GetIntegeri_v are either
	 *   zeros or match dimensions of the window into which GL is rendering;
	 */
	{
		std::vector<GLint> data;
		data.resize(max_viewports * 4 /* x + y + w+ h */);

		for (GLint i = 0; i < max_viewports; ++i)
		{
			gl.getIntegeri_v(GL_SCISSOR_BOX, i, &data[i * 4]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegeri_v");
		}

		for (GLint i = 0; i < max_viewports; ++i)
		{
			GLint scissor_width  = data[4 * i + 2];
			GLint scissor_height = data[4 * i + 3];

			if ((window_width != scissor_width) || (window_height != scissor_height))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid initial scissor box [" << i
													<< "] dimennsions: " << scissor_width << " x " << scissor_height
													<< " expected: " << window_width << " x " << window_height
													<< tcu::TestLog::EndMessage;

				test_result = false;
				break;
			}
		}
	}

	/*
	 *   * Dimensions of MAX_VIEWPORT_DIMS returned by GetFloati_v are at least
	 *   as big as supported dimensions of render buffers, see MAX_RENDERBUFFER_SIZE;
	 */
	{
		if ((max_viewport_dims[0] < max_renderbuffer_size) || (max_viewport_dims[1] < max_renderbuffer_size))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid MAX_VIEWPORT_DIMS: " << max_viewport_dims[0] << " x "
				<< max_viewport_dims[1] << " expected: " << max_renderbuffer_size << " x " << max_renderbuffer_size
				<< tcu::TestLog::EndMessage;

			test_result = false;
		}
	}

	/*
	 *   * Value of MAX_VIEWPORTS returned by GetIntegeri_v is at least 16;
	 */
	{
		if (16 > max_viewports)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid MAX_VIEWPORTS: " << max_viewports
												<< " expected at least 16." << tcu::TestLog::EndMessage;

			test_result = false;
		}
	}

	/*
	 *   * Value of VIEWPORT_SUBPIXEL_BITS returned by GetIntegeri_v is at least 0;
	 */
	{
		if (0 > viewport_subpixel_bits)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Invalid VIEWPORT_SUBPIXEL_BITS: " << viewport_subpixel_bits
												<< " expected at least 0." << tcu::TestLog::EndMessage;

			test_result = false;
		}
	}

	/*
	 *   * Values of VIEWPORT_BOUNDS_RANGE returned by GetFloatv are
	 *   at least [-32768, 32767];
	 */
	{
		if ((-32768.0f < viewport_bounds_range[0]) || (32767.0f > viewport_bounds_range[1]))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid VIEWPORT_BOUNDS_RANGE: " << viewport_bounds_range[0] << " : "
				<< viewport_bounds_range[1] << " expected at least: -32768.0f : 32767.0f" << tcu::TestLog::EndMessage;

			test_result = false;
		}
	}

	/*
	 *   * Values of LAYER_PROVOKING_VERTEX and VIEWPORT_INDEX_PROVOKING_VERTEX
	 *   returned by GetIntegerv are located in the following set
	 *   { FIRST_VERTEX_CONVENTION, LAST_VERTEX_CONVENTION, PROVOKING_VERTEX,
	 *   UNDEFINED_VERTEX };
	 */
	{
		switch (layer_provoking_vertex)
		{
		case GL_FIRST_VERTEX_CONVENTION:
		case GL_LAST_VERTEX_CONVENTION:
		case GL_PROVOKING_VERTEX:
		case GL_UNDEFINED_VERTEX:
			break;
		default:
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Invalid LAYER_PROVOKING_VERTEX: " << layer_provoking_vertex
												<< tcu::TestLog::EndMessage;

			test_result = false;
		}

		switch (viewport_provoking_vertex)
		{
		case GL_FIRST_VERTEX_CONVENTION:
		case GL_LAST_VERTEX_CONVENTION:
		case GL_PROVOKING_VERTEX:
		case GL_UNDEFINED_VERTEX:
			break;
		default:
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Invalid LAYER_PROVOKING_VERTEX: " << layer_provoking_vertex
												<< tcu::TestLog::EndMessage;

			test_result = false;
		}
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

/* Constants used by ViewportAPI */
const GLuint ViewportAPI::m_n_elements = 4;

/** Constructor
 *
 * @param context          Test context
 **/
ViewportAPI::ViewportAPI(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "viewport_api", "Test verifies that \viewport api\" works as expected")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult ViewportAPI::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool test_result = true;

	GLint max_viewports = 0;

	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	std::vector<GLfloat> scissor_box_data_a;
	std::vector<GLfloat> scissor_box_data_b;

	scissor_box_data_a.resize(max_viewports * m_n_elements);
	scissor_box_data_b.resize(max_viewports * m_n_elements);

	/*
	 *   - get initial dimensions of VIEWPORT for all MAX_VIEWPORTS indices;
	 *   - change location and dimensions of all indices at once with
	 *   ViewportArrayv;
	 *   - get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
	 */
	getViewports(max_viewports, scissor_box_data_a);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] += 0.125f;
		scissor_box_data_a[i * m_n_elements + 1] += 0.125f;
		scissor_box_data_a[i * m_n_elements + 2] -= 0.125f;
		scissor_box_data_a[i * m_n_elements + 3] -= 0.125f;
	}

	gl.viewportArrayv(0, max_viewports, &scissor_box_data_a[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewportArrayv");

	getViewports(max_viewports, scissor_box_data_b);
	compareViewports(scissor_box_data_a, scissor_box_data_b, "viewportArrayv", test_result);

	/*
	 *   - for each index:
	 *     * modify with ViewportIndexedf,
	 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_b[i * m_n_elements + 0] = 0.25f;
		scissor_box_data_b[i * m_n_elements + 1] = 0.25f;
		scissor_box_data_b[i * m_n_elements + 2] = 0.75f;
		scissor_box_data_b[i * m_n_elements + 3] = 0.75f;

		gl.viewportIndexedf(i, 0.25f, 0.25f, 0.75f, 0.75f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "viewportIndexedf");

		getViewports(max_viewports, scissor_box_data_a);
		compareViewports(scissor_box_data_a, scissor_box_data_b, "viewportIndexedf", test_result);
	}

	/*
	 *   - for each index:
	 *     * modify with ViewportIndexedfv,
	 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] = 0.375f;
		scissor_box_data_a[i * m_n_elements + 1] = 0.375f;
		scissor_box_data_a[i * m_n_elements + 2] = 0.625f;
		scissor_box_data_a[i * m_n_elements + 3] = 0.625f;

		gl.viewportIndexedfv(i, &scissor_box_data_a[i * m_n_elements]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "viewportIndexedfv");

		getViewports(max_viewports, scissor_box_data_b);
		compareViewports(scissor_box_data_a, scissor_box_data_b, "viewportIndexedfv", test_result);
	}

	/*
	 *   - for each index:
	 *     * modify all indices before and after current one with ViewportArrayv,
	 *     * get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		const GLfloat value = (0 == i % 2) ? 1.0f : 0.25f;

		for (GLint j = 0; j < i; ++j)
		{
			scissor_box_data_b[j * m_n_elements + 0] = value;
			scissor_box_data_b[j * m_n_elements + 1] = value;
			scissor_box_data_b[j * m_n_elements + 2] = value;
			scissor_box_data_b[j * m_n_elements + 3] = value;
		}

		for (GLint j = i + 1; j < max_viewports; ++j)
		{
			scissor_box_data_b[j * m_n_elements + 0] = value;
			scissor_box_data_b[j * m_n_elements + 1] = value;
			scissor_box_data_b[j * m_n_elements + 2] = value;
			scissor_box_data_b[j * m_n_elements + 3] = value;
		}

		gl.viewportArrayv(0, max_viewports, &scissor_box_data_b[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "viewportArrayv");

		getViewports(max_viewports, scissor_box_data_a);
		compareViewports(scissor_box_data_a, scissor_box_data_b, "viewportArrayv", test_result);
	}

	/*
	 *   - change location and dimensions of all indices at once with Viewport;
	 *   - get VIEWPORT for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] = 0.0f;
		scissor_box_data_a[i * m_n_elements + 1] = 0.0f;
		scissor_box_data_a[i * m_n_elements + 2] = 1.0f;
		scissor_box_data_a[i * m_n_elements + 3] = 1.0f;
	}

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");

	getViewports(max_viewports, scissor_box_data_b);
	compareViewports(scissor_box_data_a, scissor_box_data_b, "viewport", test_result);

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

/** Compare two sets of viewport data (simple vector comparison)
 *
 * @param left        Left set
 * @param right       Right set
 * @param description Test case description
 * @param out_result  Set to false if sets are different, not modified otherwise
 **/
void ViewportAPI::compareViewports(std::vector<GLfloat>& left, std::vector<GLfloat>& right, const GLchar* description,
								   bool& out_result)
{
	for (size_t i = 0; i < left.size(); ++i)
	{
		if (left[i] != right[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case: " << description
												<< " Invalid values [" << i << "] " << left[i] << " " << right[i]
												<< tcu::TestLog::EndMessage;

			out_result = false;
		}
	}
}

/** Get position of all viewports
 *
 * @param max_viewports Number of viewports to capture, MAX_VIEWPORTS
 * @param data          Memory buffer prepared for captured data
 **/
void ViewportAPI::getViewports(GLint max_viewports, std::vector<GLfloat>& out_data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (GLint i = 0; i < max_viewports; ++i)
	{
		gl.getFloati_v(GL_VIEWPORT, i, &out_data[i * 4]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getFloati_v");
	}
}

/* Constants used by ScissorAPI */
const GLuint ScissorAPI::m_n_elements = 4;

/** Constructor
 *
 * @param context          Test context
 **/
ScissorAPI::ScissorAPI(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "scissor_api", "Test verifies that \"scissor api\" works as expected")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult ScissorAPI::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool test_result = true;

	GLint max_viewports = 0;

	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	std::vector<GLint> scissor_box_data_a;
	std::vector<GLint> scissor_box_data_b;

	scissor_box_data_a.resize(max_viewports * m_n_elements);
	scissor_box_data_b.resize(max_viewports * m_n_elements);

	/*
	 *   - get initial dimensions of SCISSOR_BOX for all MAX_VIEWPORTS indices;
	 *   - change location and dimensions of all indices at once with
	 *   ScissorArrayv;
	 *   - get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
	 */
	getScissorBoxes(max_viewports, scissor_box_data_a);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] += 1;
		scissor_box_data_a[i * m_n_elements + 1] += 1;
		scissor_box_data_a[i * m_n_elements + 2] -= 1;
		scissor_box_data_a[i * m_n_elements + 3] -= 1;
	}

	gl.scissorArrayv(0, max_viewports, &scissor_box_data_a[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "scissorArrayv");

	getScissorBoxes(max_viewports, scissor_box_data_b);
	compareScissorBoxes(scissor_box_data_a, scissor_box_data_b, "scissorArrayv", test_result);

	/*
	 *   - for each index:
	 *     * modify with ScissorIndexed,
	 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_b[i * m_n_elements + 0] = 4;
		scissor_box_data_b[i * m_n_elements + 1] = 4;
		scissor_box_data_b[i * m_n_elements + 2] = 8;
		scissor_box_data_b[i * m_n_elements + 3] = 8;

		gl.scissorIndexed(i, 4, 4, 8, 8);
		GLU_EXPECT_NO_ERROR(gl.getError(), "scissorIndexed");

		getScissorBoxes(max_viewports, scissor_box_data_a);
		compareScissorBoxes(scissor_box_data_a, scissor_box_data_b, "scissorIndexed", test_result);
	}

	/*
	 *   - for each index:
	 *     * modify with ScissorIndexedv,
	 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] = 8;
		scissor_box_data_a[i * m_n_elements + 1] = 8;
		scissor_box_data_a[i * m_n_elements + 2] = 12;
		scissor_box_data_a[i * m_n_elements + 3] = 12;

		gl.scissorIndexedv(i, &scissor_box_data_a[i * m_n_elements]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "scissorIndexedv");

		getScissorBoxes(max_viewports, scissor_box_data_b);
		compareScissorBoxes(scissor_box_data_a, scissor_box_data_b, "scissorIndexedv", test_result);
	}

	/*
	 *   - for each index:
	 *     * modify all indices before and after current one with ScissorArrayv,
	 *     * get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		const GLint value = (0 == i % 2) ? 1 : 4;

		for (GLint j = 0; j < i; ++j)
		{
			scissor_box_data_b[j * m_n_elements + 0] = value;
			scissor_box_data_b[j * m_n_elements + 1] = value;
			scissor_box_data_b[j * m_n_elements + 2] = value;
			scissor_box_data_b[j * m_n_elements + 3] = value;
		}

		for (GLint j = i + 1; j < max_viewports; ++j)
		{
			scissor_box_data_b[j * m_n_elements + 0] = value;
			scissor_box_data_b[j * m_n_elements + 1] = value;
			scissor_box_data_b[j * m_n_elements + 2] = value;
			scissor_box_data_b[j * m_n_elements + 3] = value;
		}

		gl.scissorArrayv(0, max_viewports, &scissor_box_data_b[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "scissorArrayv");

		getScissorBoxes(max_viewports, scissor_box_data_a);
		compareScissorBoxes(scissor_box_data_a, scissor_box_data_b, "scissorArrayv", test_result);
	}

	/*
	 *   - change location and dimensions of all indices at once with Scissor;
	 *   - get SCISSOR_BOX for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_box_data_a[i * m_n_elements + 0] = 0;
		scissor_box_data_a[i * m_n_elements + 1] = 0;
		scissor_box_data_a[i * m_n_elements + 2] = 1;
		scissor_box_data_a[i * m_n_elements + 3] = 1;
	}

	gl.scissor(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "scissor");

	getScissorBoxes(max_viewports, scissor_box_data_b);
	compareScissorBoxes(scissor_box_data_a, scissor_box_data_b, "scissor", test_result);

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

/** Compare two sets of scissor box data (simple vector comparison)
 *
 * @param left        Left set
 * @param right       Right set
 * @param description Test case description
 * @param out_result  Set to false if sets are different, not modified otherwise
 **/
void ScissorAPI::compareScissorBoxes(std::vector<GLint>& left, std::vector<GLint>& right, const GLchar* description,
									 bool& out_result)
{
	for (size_t i = 0; i < left.size(); ++i)
	{
		if (left[i] != right[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case: " << description
												<< " Invalid values [" << i << "] " << left[i] << " " << right[i]
												<< tcu::TestLog::EndMessage;

			out_result = false;
		}
	}
}

/** Get position of all scissor boxes
 *
 * @param max_viewports Number of scissor boxes to capture, MAX_VIEWPORTS
 * @param data          Memory buffer prepared for captured data
 **/
void ScissorAPI::getScissorBoxes(GLint max_viewports, std::vector<GLint>& out_data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (GLint i = 0; i < max_viewports; ++i)
	{
		gl.getIntegeri_v(GL_SCISSOR_BOX, i, &out_data[i * 4]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegeri_v");
	}
}

/* Constants used by DepthRangeAPI */
const GLuint DepthRangeAPI::m_n_elements = 2 /* near + far */;

/** Constructor
 *
 * @param context          Test context
 **/
DepthRangeAPI::DepthRangeAPI(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "depth_range_api", "Test verifies that \"depth range api\" works as expected")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult DepthRangeAPI::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	bool					test_result;
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	if (glu::isContextTypeGLCore(context_type))
	{
		test_result = iterateHelper<GLdouble>();
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		test_result = iterateHelper<GLfloat>();
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

template <typename T>
bool DepthRangeAPI::iterateHelper(T*)
{
	/* GL entry points */
	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();
	Utils::DepthFuncWrapper depthFunc(m_context);

	bool test_result = true;

	GLint max_viewports = 0;

	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	std::vector<T> depth_range_data_a;
	std::vector<T> depth_range_data_b;

	depth_range_data_a.resize(max_viewports * m_n_elements);
	depth_range_data_b.resize(max_viewports * m_n_elements);

	/*
	 *   - get initial values of DEPTH_RANGE for all MAX_VIEWPORTS indices;
	 *   - change values of all indices at once with DepthRangeArrayv;
	 *   - get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
	 */
	getDepthRanges(depthFunc, max_viewports, depth_range_data_a);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		depth_range_data_a[i * m_n_elements + 0] += 0.125;
		depth_range_data_a[i * m_n_elements + 1] -= 0.125;
	}

	depthFunc.depthRangeArray(0, max_viewports, &depth_range_data_a[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "depthRangeArray");

	getDepthRanges(depthFunc, max_viewports, depth_range_data_b);
	compareDepthRanges(depth_range_data_a, depth_range_data_b, "depthRangeArray", test_result);

	/*
	 *   - for each index:
	 *     * modify with DepthRangeIndexed,
	 *     * get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		depth_range_data_b[i * m_n_elements + 0] = 0.25;
		depth_range_data_b[i * m_n_elements + 1] = 0.75;

		depthFunc.depthRangeIndexed(i, (T)0.25, (T)0.75);
		GLU_EXPECT_NO_ERROR(gl.getError(), "depthRangeIndexed");

		getDepthRanges(depthFunc, max_viewports, depth_range_data_a);
		compareDepthRanges(depth_range_data_a, depth_range_data_b, "depthRangeIndexed", test_result);
	}

	/*
	 *   - for each index:
	 *     * modify all indices before and after current one with DepthRangeArrayv,
	 *     * get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		const T value = (0 == i % 2) ? T(1.0) : T(0.25);

		for (GLint j = 0; j < i; ++j)
		{
			depth_range_data_b[j * m_n_elements + 0] = value;
			depth_range_data_b[j * m_n_elements + 1] = value;
		}

		for (GLint j = i + 1; j < max_viewports; ++j)
		{
			depth_range_data_b[j * m_n_elements + 0] = value;
			depth_range_data_b[j * m_n_elements + 1] = value;
		}

		depthFunc.depthRangeArray(0, max_viewports, &depth_range_data_b[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "depthRangeArray");

		getDepthRanges(depthFunc, max_viewports, depth_range_data_a);
		compareDepthRanges(depth_range_data_a, depth_range_data_b, "depthRangeArray", test_result);
	}

	/*
	 *   - change values of all indices at once with DepthRange;
	 *   - get DEPTH_RANGE for all MAX_VIEWPORTS indices and verify results;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		depth_range_data_a[i * m_n_elements + 0] = 0.0f;
		depth_range_data_a[i * m_n_elements + 1] = 1.0f;
	}

	depthFunc.depthRange((T)0.0, (T)1.0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "depthRange");

	getDepthRanges(depthFunc, max_viewports, depth_range_data_b);
	compareDepthRanges(depth_range_data_a, depth_range_data_b, "depthRange", test_result);

	return test_result;
}

/** Compare two sets of depth range data (simple vector comparison)
 *
 * @param left        Left set
 * @param right       Right set
 * @param description Test case description
 * @param out_result  Set to false if sets are different, not modified otherwise
 **/
template <typename T>
void DepthRangeAPI::compareDepthRanges(std::vector<T>& left, std::vector<T>& right, const GLchar* description,
									   bool& out_result)
{
	for (size_t i = 0; i < left.size(); ++i)
	{
		if (left[i] != right[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case: " << description
												<< " Invalid values [" << i << "] " << left[i] << " " << right[i]
												<< tcu::TestLog::EndMessage;
			out_result = false;
		}
	}
}

/** Get all depth ranges
 *
 * @param max_viewports Number of viewports to capture, MAX_VIEWPORTS
 * @param data          Memory buffer prepared for captured data
 **/
template <typename T>
void DepthRangeAPI::getDepthRanges(Utils::DepthFuncWrapper& depthFunc, GLint max_viewports, std::vector<T>& out_data)
{
	for (GLint i = 0; i < max_viewports; ++i)
	{
		depthFunc.getDepthi_v(GL_DEPTH_RANGE, i, &out_data[i * m_n_elements]);
		GLU_EXPECT_NO_ERROR(depthFunc.getFunctions().getError(), "getDouble/Floati_v");
	}
}

/** Constructor
 *
 * @param context          Test context
 **/
ScissorTestStateAPI::ScissorTestStateAPI(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseBase(context, extParams, "scissor_test_state_api",
				   "Test verifies that \"enable/disable api\" works as expected for scissor test")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult ScissorTestStateAPI::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test result */
	bool test_result = true;

	GLint max_viewports = 0;

	gl.getIntegerv(GL_MAX_VIEWPORTS, &max_viewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	std::vector<GLboolean> scissor_test_states_a;
	std::vector<GLboolean> scissor_test_states_b;

	scissor_test_states_a.resize(max_viewports);
	scissor_test_states_b.resize(max_viewports);

	/*
	 *   - get initial state of SCISSOR_TEST for all MAX_VIEWPORTS indices;
	 *   - for each index:
	 *     * toggle SCISSOR_TEST,
	 *     * get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
	 *   - for each index:
	 *     * toggle SCISSOR_TEST,
	 *     * get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
	 */
	getScissorTestStates(max_viewports, scissor_test_states_a);

	for (GLint i = 0; i < max_viewports; ++i)
	{
		if (GL_FALSE == scissor_test_states_a[i])
		{
			gl.enablei(GL_SCISSOR_TEST, i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Enablei");

			scissor_test_states_a[i] = GL_TRUE;
		}
		else
		{
			gl.disablei(GL_SCISSOR_TEST, i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Disablei");

			scissor_test_states_a[i] = GL_FALSE;
		}

		getScissorTestStates(max_viewports, scissor_test_states_b);
		compareScissorTestStates(scissor_test_states_a, scissor_test_states_b, "1st toggle", test_result);
	}

	for (GLint i = 0; i < max_viewports; ++i)
	{
		if (GL_FALSE == scissor_test_states_a[i])
		{
			gl.enablei(GL_SCISSOR_TEST, i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Enablei");

			scissor_test_states_a[i] = GL_TRUE;
		}
		else
		{
			gl.disablei(GL_SCISSOR_TEST, i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Disablei");

			scissor_test_states_a[i] = GL_FALSE;
		}

		getScissorTestStates(max_viewports, scissor_test_states_b);
		compareScissorTestStates(scissor_test_states_a, scissor_test_states_b, "2nd toggle", test_result);
	}

	/*
	 *   - enable SCISSOR_TEST for all indices at once with Enable;
	 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_test_states_a[i] = GL_TRUE;
	}

	gl.enable(GL_SCISSOR_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Enable");

	getScissorTestStates(max_viewports, scissor_test_states_b);
	compareScissorTestStates(scissor_test_states_a, scissor_test_states_b, "1st enable all", test_result);

	/*
	 *   - disable SCISSOR_TEST for all indices at once with Disable;
	 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_test_states_a[i] = GL_FALSE;
	}

	gl.disable(GL_SCISSOR_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");

	getScissorTestStates(max_viewports, scissor_test_states_b);
	compareScissorTestStates(scissor_test_states_a, scissor_test_states_b, "Disable all", test_result);

	/*
	 *   - enable SCISSOR_TEST for all indices at once with Enable;
	 *   - get state of SCISSOR_TEST for all MAX_VIEWPORTS indices and verify;
	 */
	for (GLint i = 0; i < max_viewports; ++i)
	{
		scissor_test_states_a[i] = GL_TRUE;
	}

	gl.enable(GL_SCISSOR_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Enable");

	getScissorTestStates(max_viewports, scissor_test_states_b);
	compareScissorTestStates(scissor_test_states_a, scissor_test_states_b, "2nd enable all", test_result);

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

/** Compare two sets of depth range data (simple vector comparison)
 *
 * @param left        Left set
 * @param right       Right set
 * @param description Test case description
 * @param out_result  Set to false if sets are different, not modified otherwise
 **/
void ScissorTestStateAPI::compareScissorTestStates(std::vector<GLboolean>& left, std::vector<GLboolean>& right,
												   const GLchar* description, bool& out_result)
{
	for (size_t i = 0; i < left.size(); ++i)
	{
		if (left[i] != right[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Test case: " << description
												<< " Invalid values [" << i << "] " << left[i] << " " << right[i]
												<< tcu::TestLog::EndMessage;

			out_result = false;
		}
	}
}

/** Get all depth ranges
 *
 * @param max_viewports Number of viewports to capture, MAX_VIEWPORTS
 * @param data          Memory buffer prepared for captured data
 **/
void ScissorTestStateAPI::getScissorTestStates(GLint max_viewports, std::vector<GLboolean>& out_data)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (GLint i = 0; i < max_viewports; ++i)
	{
		out_data[i] = gl.isEnabledi(GL_SCISSOR_TEST, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "isEnabledi");
	}
}

/* Constants used by DrawTestBase */
const GLuint DrawTestBase::m_depth		  = 16;
const GLuint DrawTestBase::m_height		  = 128;
const GLuint DrawTestBase::m_width		  = 128;
const GLuint DrawTestBase::m_r32f_height  = 2;
const GLuint DrawTestBase::m_r32f_width   = 16;
const GLuint DrawTestBase::m_r32ix4_depth = 4;

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
DrawTestBase::DrawTestBase(deqp::Context& context, const glcts::ExtParameters& extParams, const GLchar* test_name,
						   const GLchar* test_description)
	: TestCaseBase(context, extParams, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult DrawTestBase::iterate()
{
	if (!m_is_viewport_array_supported)
	{
		throw tcu::NotSupportedError(VIEWPORT_ARRAY_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	/* Test result */
	bool test_result = true;

	/* Get type of test */
	const TEST_TYPE test_type = getTestType();

	GLuint n_draw_calls = getDrawCallsNumber();
	GLuint n_iterations = 0;
	switch (test_type)
	{
	case VIEWPORT:
	case SCISSOR:
		n_iterations = 3;
		break;
	case DEPTHRANGE:
	case PROVOKING:
		n_iterations = 2;
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	/* Get shader sources and specialize them */
	const std::string& frag = getFragmentShader();
	const std::string& geom = getGeometryShader();
	const std::string& vert = getVertexShader();

	const GLchar* frag_template = frag.c_str();
	const GLchar* geom_template = geom.c_str();
	const GLchar* vert_template = vert.c_str();

	std::string fragment = specializeShader(1, &frag_template);
	std::string geometry = specializeShader(1, &geom_template);
	std::string vertex   = specializeShader(1, &vert_template);

	/* Prepare program */
	Utils::program program(m_context);

	try
	{
		program.build(0 /* compute */, fragment.c_str(), geometry.c_str(), 0 /* tess ctrl */, 0 /* tess eval */,
					  vertex.c_str(), 0 /* varying names */, 0 /* n_varyings */);
	}
	catch (Utils::shaderCompilationException& exc)
	{
		/* Something wrong with compilation, test case failed */
		tcu::MessageBuilder message = m_context.getTestContext().getLog() << tcu::TestLog::Message;

		message << "Shader compilation failed. Error message: " << exc.m_error_message;

		Utils::program::printShaderSource(exc.m_shader_source.c_str(), message);

		message << tcu::TestLog::EndMessage;

		TCU_FAIL("Shader compilation failed");
	}
	catch (Utils::programLinkageException& exc)
	{
		/* Something wrong with linking, test case failed */
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Program linking failed. Error message: " << exc.m_error_message
											<< tcu::TestLog::EndMessage;
		TCU_FAIL("Program linking failed");
	}

	program.use();

	/* Prepare VAO */
	Utils::vertexArray vao(m_context);
	vao.generate();
	vao.bind();

	/* For each iteration from test type */
	for (GLuint i = 0; i < n_iterations; ++i)
	{
		/* Prepare textures */
		Utils::texture texture_0(m_context);
		Utils::texture texture_1(m_context);

		prepareTextures(texture_0, texture_1);

		/* Prepare framebuffer */
		Utils::framebuffer framebuffer(m_context);
		framebuffer.generate();
		setupFramebuffer(framebuffer, texture_0, texture_1);
		framebuffer.bind();

		/* Set up viewports */
		setupViewports(test_type, i);

		if (false == isClearTest())
		{
			/* For each draw call */
			for (GLuint draw_call = 0; draw_call < n_draw_calls; ++draw_call)
			{
				prepareUniforms(program, draw_call);

				bool	is_clear;
				GLfloat depth_value;

				getClearSettings(is_clear, draw_call, depth_value);

				if (true == is_clear)
				{
					if (glu::isContextTypeGLCore(context_type))
					{
						gl.clearDepth((GLdouble)depth_value);
					}
					else
					{
						gl.clearDepthf(depth_value);
					}
					GLU_EXPECT_NO_ERROR(gl.getError(), "ClearDepth");

					gl.clear(GL_DEPTH_BUFFER_BIT);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
				}

				gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
				GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

				bool result = checkResults(texture_0, texture_1, draw_call);

				if (false == result)
				{
					test_result = false;
					goto end;
				}
			}
		}
		else
		{
			gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");

			bool result = checkResults(texture_0, texture_1, 0);

			if (false == result)
			{
				test_result = false;
				goto end;
			}
		}
	}

end:
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

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0 Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool DrawTestBase::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */, GLuint /*draw_call_index */)
{
	bool  check_result = true;
	GLint index		   = 0;

	std::vector<GLint> texture_data;
	texture_data.resize(m_width * m_height);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 0; x < 4; ++x)
		{
			bool result = checkRegionR32I(x, y, index, &texture_data[0]);

			if (false == result)
			{
				check_result = false;
				goto end;
			}

			index += 1;
		}
	}

end:
	return check_result;
}

/** Get settings of clear operation
 *
 * @param clear_depth_before_draw Selects if clear depth should be executed before draw.
 * @param ignored
 * @param ignored
 **/
void DrawTestBase::getClearSettings(bool& clear_depth_before_draw, GLuint /* iteration_index */,
									GLfloat& /* depth_value */)
{
	clear_depth_before_draw = false;
}

/** Get number of draw call to be executed during test
 *
 * @return 1
 **/
GLuint DrawTestBase::getDrawCallsNumber()
{
	return 1;
}

/** Get test type
 *
 * @return VIEWPORT
 **/
DrawTestBase::TEST_TYPE DrawTestBase::getTestType()
{
	return VIEWPORT;
}

/** Selects if test should do draw or clear operation
 *
 * @return false - draw operation
 **/
bool DrawTestBase::isClearTest()
{
	return false;
}

/** Prepare textures used as framebuffer's attachments for current draw call
 *
 * @param texture_0 R32I texture
 * @param ignored
 **/
void DrawTestBase::prepareTextures(Utils::texture& texture_0, Utils::texture& /* texture_1 */)
{
	prepareTextureR32I(texture_0);
}

/** Prepare uniforms for given draw call
 *
 * @param ignored
 * @param ignored
 **/
void DrawTestBase::prepareUniforms(Utils::program& /* program */, GLuint /* draw_call_index */)
{
	/* empty */
}

/** Attach textures to framebuffer
 *
 * @param framebuffer Framebuffer instance
 * @param texture_0   Texture attached as color 0
 * @param ignored
 **/
void DrawTestBase::setupFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& texture_0,
									Utils::texture& /* texture_1 */)
{
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, texture_0.m_id, m_width, m_height);
}

/** Check if region specified with <x and <y> is filled with expected value.
 * Note: there is assumption that there are 4x4 regions
 *
 * @param x              X coordinate of region
 * @param y              Y coordinate of region
 * @param expected_value Expected value
 * @param data           Texture data (not region, but whole texture)
 *
 * @return True if region is filled with <expected_value>, false otherwise
 **/
bool DrawTestBase::checkRegionR32I(GLuint x, GLuint y, GLint expected_value, GLint* data)
{
	static GLuint width  = m_width / 4;
	static GLuint height = m_height / 4;

	return checkRegionR32I(x, y, width, height, expected_value, data);
}

/** Check if region specified with <x and <y> is filled with expected value.
 * Note: there is assumption that there are 4x4 regions
 *
 * @param x              X coordinate of region
 * @param y              Y coordinate of region
 * @param width          Width of region
 * @param height         Height of region
 * @param expected_value Expected value
 * @param data           Texture data (not region, but whole texture)
 *
 * @return True if region is filled with <expected_value>, false otherwise
 **/
bool DrawTestBase::checkRegionR32I(GLuint x, GLuint y, GLuint width, GLuint height, GLint expected_value, GLint* data)
{
	bool result = true;

	const GLuint offset = (y * height * m_width) + (x * width);

	for (GLuint line = 0; line < height; ++line)
	{
		const GLuint line_offset = offset + line * m_width;

		for (GLuint texel = 0; texel < width; ++texel)
		{
			const GLuint texel_offset = line_offset + texel;

			const GLint value = data[texel_offset];

			if (expected_value != value)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid result. Region (" << x << "x"
													<< y << "). Expected: " << expected_value << " got " << value
													<< tcu::TestLog::EndMessage;

				result = false;
				goto end;
			}
		}
	}

end:
	return result;
}

/** Return boiler-plate vertex shader
 *
 * @return Source code of vertex shader
 **/
std::string DrawTestBase::getVertexShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    /* empty */;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Set up viewports
 *
 * @param type            Type of test
 * @param iteration_index Index of iteration for given test type
 **/
void DrawTestBase::setupViewports(TEST_TYPE type, GLuint iteration_index)
{
	switch (type)
	{
	case VIEWPORT:
	{
		VIEWPORT_METHOD method;
		switch (iteration_index)
		{
		case 0:
		case 1:
		case 2:
			method = (VIEWPORT_METHOD)iteration_index;
			break;
		default:
			TCU_FAIL("Invalid value");
		}
		setup4x4Viewport(method);
	}
	break;
	case SCISSOR:
	{
		SCISSOR_METHOD method;
		switch (iteration_index)
		{
		case 0:
		case 1:
		case 2:
			method = (SCISSOR_METHOD)iteration_index;
			break;
		default:
			TCU_FAIL("Invalid value");
		}
		setup4x4Scissor(method, false /* set_zeros */);
	}
	break;
	case DEPTHRANGE:
	{
		DEPTH_RANGE_METHOD method;
		switch (iteration_index)
		{
		case 0:
		case 1:
			method = (DEPTH_RANGE_METHOD)iteration_index;
			break;
		default:
			TCU_FAIL("Invalid value");
		}
		setup16x2Depths(method);
	}
	break;
	case PROVOKING:
	{
		PROVOKING_VERTEX provoking;
		switch (iteration_index)
		{
		case 0:
		case 1:
			provoking = (PROVOKING_VERTEX)iteration_index;
			break;
		default:
			TCU_FAIL("Invalid value");
		}
		setup2x2Viewport(provoking);
	}
	break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Prepare R32I texture filled with value -1
 *
 * @param texture Texture instance
 **/
void DrawTestBase::prepareTextureR32I(Utils::texture& texture)
{
	static const GLuint size = m_width * m_height;
	GLint				data[size];

	for (GLuint i = 0; i < size; ++i)
	{
		data[i] = -1;
	}

	texture.create(m_width, m_height, GL_R32I);
	texture.update(m_width, m_height, 0 /* depth */, GL_RED_INTEGER, GL_INT, data);
}

/** Prepare R32I array texture filled with value -1, 4 layers
 *
 * @param texture Texture instance
 **/
void DrawTestBase::prepareTextureR32Ix4(Utils::texture& texture)
{
	static const GLuint size = m_width * m_height * m_r32ix4_depth;

	std::vector<GLint> data;
	data.resize(size);

	for (GLuint i = 0; i < size; ++i)
	{
		data[i] = -1;
	}

	texture.create(m_width, m_height, m_r32ix4_depth, GL_R32I);
	texture.update(m_width, m_height, m_r32ix4_depth, GL_RED_INTEGER, GL_INT, &data[0]);
}

/** Prepare R32I array texture filled with value -1
 *
 * @param texture Texture instance
 **/
void DrawTestBase::prepareTextureArrayR32I(Utils::texture& texture)
{
	static const GLuint size = m_width * m_height * m_depth;

	std::vector<GLint> data;
	data.resize(size);

	for (GLuint i = 0; i < size; ++i)
	{
		data[i] = -1;
	}

	texture.create(m_width, m_height, m_depth, GL_R32I);
	texture.update(m_width, m_height, m_depth, GL_RED_INTEGER, GL_INT, &data[0]);
}

/** Prepare R32F texture filled with value -1
 *
 * @param texture Texture instance
 **/
void DrawTestBase::prepareTextureR32F(Utils::texture& texture)
{
	static const GLuint size = m_r32f_width * m_r32f_height;
	GLfloat				data[size];

	for (GLuint i = 0; i < size; ++i)
	{
		data[i] = -1.0f;
	}

	texture.create(m_r32f_width, m_r32f_height, GL_R32F);
	texture.update(m_r32f_width, m_r32f_height, 0 /* depth */, GL_RED, GL_FLOAT, data);
}

/** Prepare D32F texture filled with value -1
 *
 * @param texture Texture instance
 **/
void DrawTestBase::prepareTextureD32F(Utils::texture& texture)
{
	static const GLuint size = m_width * m_height;
	GLfloat				data[size];

	for (GLuint i = 0; i < size; ++i)
	{
		data[i] = -1.0f;
	}

	texture.create(m_width, m_height, GL_DEPTH_COMPONENT32F);
	texture.update(m_width, m_height, 0 /* depth */, GL_DEPTH_COMPONENT, GL_FLOAT, data);
}

/** Set up 16 viewports and depth ranges horizontally
 *
 * @param method Method used to set depth ranges
 **/
void DrawTestBase::setup16x2Depths(DEPTH_RANGE_METHOD method)
{
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	if (glu::isContextTypeGLCore(context_type))
	{
		setup16x2DepthsHelper<GLdouble>(method);
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		setup16x2DepthsHelper<GLfloat>(method);
	}
}

template <typename T>
void DrawTestBase::setup16x2DepthsHelper(DEPTH_RANGE_METHOD method, T*)
{
	static const T step = 1.0 / 16.0;

	const glw::Functions&   gl = m_context.getRenderContext().getFunctions();
	Utils::DepthFuncWrapper depthFunc(m_context);

	T		depth_data[16 * 2];
	GLfloat viewport_data[16 * 4];

	for (GLuint i = 0; i < 16; ++i)
	{
		const T near = step * (T)i;

		depth_data[i * 2 + 0] = near;
		depth_data[i * 2 + 1] = T(1.0) - near;

		viewport_data[i * 4 + 0] = (GLfloat)i;
		viewport_data[i * 4 + 1] = 0.0f;
		viewport_data[i * 4 + 2] = 1.0f;
		viewport_data[i * 4 + 3] = 2.0f;
	}

	gl.viewportArrayv(0 /* first */, 16 /* count */, viewport_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportArrayv");

	switch (method)
	{
	case DEPTHRANGEINDEXED:
		for (GLuint i = 0; i < 16; ++i)
		{
			depthFunc.depthRangeIndexed(i, depth_data[i * 2 + 0], depth_data[i * 2 + 1]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DepthRangeIndexed");
		}
		break;

	case DEPTHRANGEARRAYV:
		depthFunc.depthRangeArray(0 /* first */, 16 /* count */, depth_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DepthRangeArray");
		break;

	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Set up 4x4 scissor boxes with enabled test
 *
 * @param method    Method used to set scissor boxes
 * @param set_zeros Select if width and height should be 0 or image_dim / 4
 **/
void DrawTestBase::setup4x4Scissor(SCISSOR_METHOD method, bool set_zeros)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (GLuint i = 0; i < 16; ++i)
	{
		gl.enablei(GL_SCISSOR_TEST, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Enablei");
	}

	GLint index = 0;
	GLint data[16 * 4 /* 4x4 * (x + y + w + h) */];

	GLint width  = m_width / 4;
	GLint height = m_height / 4;

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 0; x < 4; ++x)
		{
			data[index * 4 + 0] = x * width;
			data[index * 4 + 1] = y * height;
			if (false == set_zeros)
			{
				data[index * 4 + 2] = width;
				data[index * 4 + 3] = height;
			}
			else
			{
				data[index * 4 + 2] = 0;
				data[index * 4 + 3] = 0;
			}

			index += 1;
		}
	}

	switch (method)
	{
	case SCISSORARRAYV:
		gl.scissorArrayv(0 /* first */, 16 /*count */, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ScissorArrayv");
		break;
	case SCISSORINDEXEDF:
		for (GLuint i = 0; i < 16; ++i)
		{
			const GLint x = data[i * 4 + 0];
			const GLint y = data[i * 4 + 1];
			const GLint w = data[i * 4 + 2];
			const GLint h = data[i * 4 + 3];

			gl.scissorIndexed(i, x, y, w, h);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ScissorIndexed");
		}
		break;
	case SCISSORINDEXEDF_V:
		for (GLuint i = 0; i < 16; ++i)
		{
			gl.scissorIndexedv(i, &data[i * 4]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ScissorIndexedv");
		}
		break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Set up 4x4 viewports
 *
 * @param method Method used to set viewports
 **/
void DrawTestBase::setup4x4Viewport(VIEWPORT_METHOD method)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint   index = 0;
	GLfloat data[16 * 4 /* 4x4 * (x + y + w + h) */];

	GLfloat width  = (GLfloat)(m_width / 4);
	GLfloat height = (GLfloat)(m_height / 4);

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 0; x < 4; ++x)
		{
			data[index * 4 + 0] = (GLfloat)((GLfloat)x * width);
			data[index * 4 + 1] = (GLfloat)((GLfloat)y * height);
			data[index * 4 + 2] = width;
			data[index * 4 + 3] = height;

			index += 1;
		}
	}

	switch (method)
	{
	case VIEWPORTARRAYV:
		gl.viewportArrayv(0 /* first */, 16 /*count */, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportArrayv");
		break;
	case VIEWPORTINDEXEDF:
		for (GLuint i = 0; i < 16; ++i)
		{
			const GLfloat x = data[i * 4 + 0];
			const GLfloat y = data[i * 4 + 1];
			const GLfloat w = data[i * 4 + 2];
			const GLfloat h = data[i * 4 + 3];

			gl.viewportIndexedf(i, x, y, w, h);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedf");
		}
		break;
	case VIEWPORTINDEXEDF_V:
		for (GLuint i = 0; i < 16; ++i)
		{
			gl.viewportIndexedfv(i, &data[i * 4]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedfv");
		}
		break;
	default:
		TCU_FAIL("Invalid enum");
	}
}

/** Set up 4x4 viewports
 *
 * @param method Method used to set viewports
 **/
void DrawTestBase::setup2x2Viewport(PROVOKING_VERTEX provoking)
{
	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	GLint   index = 0;
	GLfloat data[4 * 4 /* 4x4 * (x + y + w + h) */];

	GLfloat width  = (GLfloat)(m_width / 2);
	GLfloat height = (GLfloat)(m_height / 2);

	for (GLuint y = 0; y < 2; ++y)
	{
		for (GLuint x = 0; x < 2; ++x)
		{
			data[index * 4 + 0] = (GLfloat)((GLfloat)x * width);
			data[index * 4 + 1] = (GLfloat)((GLfloat)y * height);
			data[index * 4 + 2] = width;
			data[index * 4 + 3] = height;

			index += 1;
		}
	}

	gl.viewportArrayv(0 /* first */, 4 /*count */, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportArrayv");

	if (glu::isContextTypeGLCore(context_type))
	{
		GLenum mode = 0;
		switch (provoking)
		{
		case FIRST:
			mode = GL_FIRST_VERTEX_CONVENTION;
			break;
		case LAST:
			mode = GL_LAST_VERTEX_CONVENTION;
			break;
		default:
			TCU_FAIL("Invalid enum");
		}

		gl.provokingVertex(mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ProvokingVertex");
	}
	else
	{
		/* can't control the provoking vertex in ES yet - it stays as LAST */
		DE_ASSERT(glu::isContextTypeES(context_type));
		DE_UNREF(provoking);
	}
}

/** Constructor
 *
 * @param context          Test context
 **/
DrawToSingleLayerWithMultipleViewports::DrawToSingleLayerWithMultipleViewports(deqp::Context&			   context,
																			   const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "draw_to_single_layer_with_multiple_viewports",
				   "Test verifies that multiple viewports can be used to draw to single layer")
{
	/* Nothing to be done here */
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DrawToSingleLayerWithMultipleViewports::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DrawToSingleLayerWithMultipleViewports::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 16)         in;\n"
								  "layout(triangle_strip, max_vertices = 4) out;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Constructor
 *
 * @param context          Test context
 **/
DynamicViewportIndex::DynamicViewportIndex(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "dynamic_viewport_index",
				   "Test verifies that gl_ViewportIndex can be assigned with dynamic value")
{
	/* Nothing to be done here */
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DynamicViewportIndex::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DynamicViewportIndex::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 1)          in;\n"
								  "layout(triangle_strip, max_vertices = 4) out;\n"
								  "\n"
								  "uniform int uni_index;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gs_fs_color      = uni_index;\n"
								  "    gl_ViewportIndex = uni_index;\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = uni_index;\n"
								  "    gl_ViewportIndex = uni_index;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = uni_index;\n"
								  "    gl_ViewportIndex = uni_index;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = uni_index;\n"
								  "    gl_ViewportIndex = uni_index;\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0       Verified texture
 * @param ignored
 * @param draw_call_index Draw call that was executed
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool DynamicViewportIndex::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
										GLuint			draw_call_index)
{
	bool   check_result = true;
	GLuint index		= 0;

	std::vector<GLint> texture_data;
	texture_data.resize(m_width * m_height);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 0; x < 4; ++x)
		{
			GLint expected_value = -1;
			if (index <= draw_call_index)
			{
				expected_value = index;
			}

			bool result = checkRegionR32I(x, y, expected_value, &texture_data[0]);

			if (false == result)
			{
				check_result = false;
				goto end;
			}

			index += 1;
		}
	}

end:
	return check_result;
}

/** Get number of draw call to be executed during test
 *
 * @return 16
 **/
GLuint DynamicViewportIndex::getDrawCallsNumber()
{
	return 16;
}

/** Prepare uniforms for given draw call
 *
 * @param program         Program object
 * @param draw_call_index Index of draw call to be executed
 **/
void DynamicViewportIndex::prepareUniforms(Utils::program& program, GLuint draw_call_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint location = program.getUniformLocation("uni_index");

	gl.uniform1i(location, (GLint)draw_call_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
}

/** Constructor
 *
 * @param context          Test context
 **/
DrawMulitpleViewportsWithSingleInvocation::DrawMulitpleViewportsWithSingleInvocation(
	deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "draw_mulitple_viewports_with_single_invocation",
				   "Test verifies that single invocation can output to multiple viewports")
{
	/* Nothing to be done here */
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DrawMulitpleViewportsWithSingleInvocation::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DrawMulitpleViewportsWithSingleInvocation::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 1)           in;\n"
								  "layout(triangle_strip, max_vertices = 64) out;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "void routine(int index)\n"
								  "{\n"
								  "    gs_fs_color      = index;\n"
								  "    gl_ViewportIndex = index;\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = index;\n"
								  "    gl_ViewportIndex = index;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = index;\n"
								  "    gl_ViewportIndex = index;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = index;\n"
								  "    gl_ViewportIndex = index;\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "}\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    for(int i = 0; i < 16; ++i)\n"
								  "    {\n"
								  "        routine(i);\n"
								  "    }\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Constructor
 *
 * @param context          Test context
 **/
ViewportIndexSubroutine::ViewportIndexSubroutine(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "viewport_index_subroutine",
				   "Test verifies subroutines can be used to output data to specific viewport")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::CONTINUE after executing test case, tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult ViewportIndexSubroutine::iterate()
{
	/* this exists solely to check for subroutine support, which is not supported in ES.
	   The real work is done in DrawTestBase::iterate() */
	const glu::ContextType& context_type = m_context.getRenderContext().getType();
	if (!glu::isContextTypeGLCore(context_type))
	{
		throw tcu::NotSupportedError("Subroutines not supported", "", __FILE__, __LINE__);
	}

	return DrawTestBase::iterate();
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string ViewportIndexSubroutine::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string ViewportIndexSubroutine::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 1)          in;\n"
								  "layout(triangle_strip, max_vertices = 4) out;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "subroutine void indexSetter(void);\n"
								  "\n"
								  "subroutine(indexSetter) void four()\n"
								  "{\n"
								  "    gs_fs_color      = 4;\n"
								  "    gl_ViewportIndex = 4;\n"
								  "}\n"
								  "\n"
								  "subroutine(indexSetter) void five()\n"
								  "{\n"
								  "    gs_fs_color      = 5;\n"
								  "    gl_ViewportIndex = 5;\n"
								  "}\n"
								  "\n"
								  "subroutine uniform indexSetter routine;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    routine();\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    routine();\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    routine();\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    routine();\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Check if R32I texture is filled with two halves, left is 4, right is either -1 or 5
 *
 * @param texture_0       Verified texture
 * @param ignored
 * @param draw_call_index Draw call that was executed
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool ViewportIndexSubroutine::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
										   GLuint		   draw_call_index)
{
	bool check_result = true;

	std::vector<GLint> texture_data;
	texture_data.resize(m_width * m_height);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	/* Left is 4 and right is -1, or left is 4 and right is 5 */
	GLint expected_left  = 4;
	GLint expected_right = (1 == draw_call_index) ? 5 : -1;

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 0; x < 2; ++x)
		{
			bool result = checkRegionR32I(x, y, expected_left, &texture_data[0]);

			if (false == result)
			{
				check_result = false;
				goto end;
			}
		}
	}

	for (GLuint y = 0; y < 4; ++y)
	{
		for (GLuint x = 2; x < 4; ++x)
		{
			bool result = checkRegionR32I(x, y, expected_right, &texture_data[0]);

			if (false == result)
			{
				check_result = false;
				goto end;
			}
		}
	}

end:
	return check_result;
}

/** Get number of draw call to be executed during test
 *
 * @return 2
 **/
GLuint ViewportIndexSubroutine::getDrawCallsNumber()
{
	return 2;
}

/** Prepare uniforms for given draw call
 *
 * @param program         Program object
 * @param draw_call_index Index of draw call to be executed
 **/
void ViewportIndexSubroutine::prepareUniforms(Utils::program& program, GLuint draw_call_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const GLchar* subroutine_name = (0 == draw_call_index) ? "four" : "five";

	GLint  location = program.getSubroutineUniformLocation("routine", GL_GEOMETRY_SHADER);
	GLuint index	= program.getSubroutineIndex(subroutine_name, GL_GEOMETRY_SHADER);

	if (0 != location)
	{
		TCU_FAIL("Something wrong, subroutine uniform location is not 0. Mistake in geometry shader?");
	}

	gl.uniformSubroutinesuiv(GL_GEOMETRY_SHADER, 1, &index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");
}

/** Set 4th viewport on left half and 5 on right half of framebuffer. Rest span over whole image.
 *
 * @param ignored
 * @param iteration_index Index of iteration, used to select "viewport" method
 **/
void ViewportIndexSubroutine::setupViewports(TEST_TYPE /* type */, glw::GLuint iteration_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLfloat data[2 * 4] = { 0.0f, 0.0f, 64.0f, 128.0f, 64.0f, 0.0f, 64.0f, 128.0f };

	gl.viewport(0, 0, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

	switch (iteration_index)
	{
	case 0:

		gl.viewportArrayv(4, 2, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportArrayv");

		break;

	case 1:

		gl.viewportIndexedf(4, data[0], data[1], data[2], data[3]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedf");

		gl.viewportIndexedf(5, data[4], data[5], data[6], data[7]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedf");

		break;

	case 2:

		gl.viewportIndexedfv(4, &data[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedfv");

		gl.viewportIndexedfv(5, &data[4]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportIndexedfv");

		break;

	default:
		TCU_FAIL("Invalid value");
	}
}

/** Constructor
 *
 * @param context Test context
 **/
DrawMultipleLayers::DrawMultipleLayers(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "draw_multiple_layers",
				   "Test verifies that single viewport affects multiple layers in the same way")
{
	/* Nothing to be done here */
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
DrawMultipleLayers::DrawMultipleLayers(deqp::Context& context, const glcts::ExtParameters& extParams,
									   const GLchar* test_name, const GLchar* test_description)
	: DrawTestBase(context, extParams, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DrawMultipleLayers::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DrawMultipleLayers::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 16)         in;\n"
								  "layout(triangle_strip, max_vertices = 4) out;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Layer         = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Layer         = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Layer         = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = gl_InvocationID;\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Layer         = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0       Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool DrawMultipleLayers::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
									  GLuint /* draw_call_index */)
{
	static const GLuint layer_size = m_width * m_height;

	bool check_result = true;

	std::vector<GLint> texture_data;
	texture_data.resize(layer_size * m_depth);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	/* 16 layers, only region corresponding with layer index should be modified */
	for (GLuint layer = 0; layer < m_depth; ++layer)
	{
		GLuint index = 0;

		for (GLuint y = 0; y < 4; ++y)
		{
			for (GLuint x = 0; x < 4; ++x)
			{
				GLint* layer_data = &texture_data[layer * layer_size];

				GLint expected_value = -1;
				if (index == layer)
				{
					expected_value = index;
				}

				bool result = checkRegionR32I(x, y, expected_value, layer_data);

				if (false == result)
				{
					check_result = false;
					goto end;
				}

				index += 1;
			}
		}
	}

end:
	return check_result;
}

/** Prepare textures used as framebuffer's attachments for current draw call
 *
 * @param texture_0 R32I texture
 * @param ignored
 **/
void DrawMultipleLayers::prepareTextures(Utils::texture& texture_0, Utils::texture& /* texture_1 */)
{
	prepareTextureArrayR32I(texture_0);
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
Scissor::Scissor(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawMultipleLayers(context, extParams, "scissor", "Test verifies that scissor test is applied as expected")
{
	/* Nothing to be done here */
}

/** Get test type
 *
 * @return SCISSOR
 **/
DrawTestBase::TEST_TYPE Scissor::getTestType()
{
	return SCISSOR;
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
ScissorZeroDimension::ScissorZeroDimension(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawMultipleLayers(context, extParams, "scissor_zero_dimension",
						 "Test verifies that scissor test discard all fragments when width and height is set to zero")
{
	/* Nothing to be done here */
}

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0       Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool ScissorZeroDimension::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
										GLuint /* draw_call_index */)
{
	static const GLuint layer_size = m_width * m_height;

	bool check_result = true;

	std::vector<GLint> texture_data;
	texture_data.resize(layer_size * m_depth);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	/* 16 layers, all regions were not modified */
	for (GLuint layer = 0; layer < m_depth; ++layer)
	{
		for (GLuint y = 0; y < 4; ++y)
		{
			for (GLuint x = 0; x < 4; ++x)
			{
				GLint* layer_data = &texture_data[layer * layer_size];

				GLint expected_value = -1;

				bool result = checkRegionR32I(x, y, expected_value, layer_data);

				if (false == result)
				{
					check_result = false;
					goto end;
				}
			}
		}
	}

end:
	return check_result;
}

/** Get test type
 *
 * @return SCISSOR
 **/
DrawTestBase::TEST_TYPE ScissorZeroDimension::getTestType()
{
	return SCISSOR;
}

/** Set up viewports
 *
 * @param Ignored
 * @param iteration_index Index of iteration for given test type
 **/
void ScissorZeroDimension::setupViewports(TEST_TYPE /* type */, GLuint iteration_index)
{
	SCISSOR_METHOD method;
	switch (iteration_index)
	{
	case 0:
	case 1:
	case 2:
		method = (SCISSOR_METHOD)iteration_index;
		break;
	default:
		TCU_FAIL("Invalid value");
	}

	setup4x4Scissor(method, true /* set_zeros */);
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
ScissorClear::ScissorClear(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawMultipleLayers(context, extParams, "scissor_clear",
						 "Test verifies that Clear is affected only by settings of scissor test in first viewport")
{
	/* Nothing to be done here */
}

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0 Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool ScissorClear::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */, GLuint /*draw_call_index */)
{
	static const GLuint layer_size = m_width * m_height;

	bool check_result = true;

	std::vector<GLint> texture_data;
	texture_data.resize(layer_size * m_depth);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	/* 16 layers, only region corresponding with scissor box 0 should be modified */
	for (GLuint layer = 0; layer < m_depth; ++layer)
	{
		for (GLuint y = 0; y < 4; ++y)
		{
			for (GLuint x = 0; x < 4; ++x)
			{
				GLint* layer_data = &texture_data[layer * layer_size];

				GLint expected_value = -1;
				if ((0 == x) && (0 == y))
				{
					expected_value = 0;
				}

				bool result = checkRegionR32I(x, y, expected_value, layer_data);

				if (false == result)
				{
					check_result = false;
					goto end;
				}
			}
		}
	}

end:
	return check_result;
}

/** Get test type
 *
 * @return SCISSOR
 **/
DrawTestBase::TEST_TYPE ScissorClear::getTestType()
{
	return SCISSOR;
}

/** Selects if test should do draw or clear operation
 *
 * @return true - clear operation
 **/
bool ScissorClear::isClearTest()
{
	return true;
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
DepthRange::DepthRange(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "depth_range", "Test verifies that depth range is applied as expected")
{
	/* Nothing to be done here */
}

/** Check if R32F texture is filled with two rows, top with decreasing values, bottom with incresing values
 *
 * @param texture_0 Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool DepthRange::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */, GLuint /*draw_call_index */)
{
	static const GLfloat step = 1.0f / 16.0f;

	bool check_result = true;

	std::vector<GLfloat> texture_data;
	texture_data.resize(m_r32f_width * m_r32f_height);
	texture_0.get(GL_RED, GL_FLOAT, &texture_data[0]);

	GLfloat depth_data[16 * 2];

	for (GLuint i = 0; i < 16; ++i)
	{
		const GLfloat near = step * (GLfloat)i;

		depth_data[i * 2 + 0] = near;
		depth_data[i * 2 + 1] = 1.0f - near;
	}

	for (GLuint i = 0; i < 16; ++i)
	{
		const GLfloat expected_near = depth_data[i * 2 + 0];
		const GLfloat expected_far  = depth_data[i * 2 + 1];

		/* Bottom row should contain near values, top one should contain far values */
		const GLfloat near = texture_data[i];
		const GLfloat far  = texture_data[i + 16];

		if ((expected_near != near) || (expected_far != far))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid values at " << i << " expected ["
												<< expected_near << ", " << expected_far << "] got [" << near << ", "
												<< far << "]" << tcu::TestLog::EndMessage;

			check_result = false;
			break;
		}
	}

	return check_result;
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DepthRange::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "#ifdef GL_ES\n"
								  "precision highp float;\n"
								  "#endif\n"
								  "out float fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gl_FragCoord.z;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DepthRange::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 16)         in;\n"
								  "layout(triangle_strip, max_vertices = 8) out;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    const float top_z    = 1.0;\n"
								  "    const float bottom_z = -1.0;\n"
								  "\n"
								  "    /* Bottom */\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, -1, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 0, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, -1, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 0, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "\n"
								  "    /* Top */\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 0, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 1, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 0, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 1, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get test type
 *
 * @return DEPTHRANGE
 **/
DrawTestBase::TEST_TYPE DepthRange::getTestType()
{
	return DEPTHRANGE;
}

/** Prepare textures used as framebuffer's attachments for current draw call
 *
 * @param texture_0 R32F texture
 * @param ignored
 **/
void DepthRange::prepareTextures(Utils::texture& texture_0, Utils::texture& /* texture_1 */)
{
	prepareTextureR32F(texture_0);
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
DepthRangeDepthTest::DepthRangeDepthTest(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "depth_range_depth_test",
				   "Test verifies that depth test work as expected with multiple viewports")
{
	/* Nothing to be done here */
}

/** Check if R32F texture is filled with two rows of values less than expected depth
 *
 * @param texture_0       Verified texture
 * @param ignored
 * @param draw_call_index Index of draw call
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool DepthRangeDepthTest::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
									   GLuint		   draw_call_index)
{
	static const GLfloat step = 1.0f / 16.0f;

	const GLfloat depth_value = step * (GLfloat)draw_call_index;

	bool check_result = true;

	std::vector<GLfloat> texture_data;
	texture_data.resize(m_r32f_width * m_r32f_height);
	texture_0.get(GL_RED, GL_FLOAT, &texture_data[0]);

	for (GLuint i = 0; i < 16; ++i)
	{
		/* Bottom row should contain near values, top one should contain far values */
		const GLfloat near = texture_data[i];
		const GLfloat far  = texture_data[i + 16];

		if ((depth_value <= near) || (depth_value <= far))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid values at " << i << " depth value "
												<< depth_value << " got [" << near << ", " << far << "]"
												<< tcu::TestLog::EndMessage;

			check_result = false;
			break;
		}
	}

	return check_result;
}

/** Get settings of clear operation
 *
 * @param clear_depth_before_draw Selects if clear should be executed before draw.
 * @param iteration_index         Index of draw call
 * @param depth_value             Value that will be used to clear depth buffer
 **/
void DepthRangeDepthTest::getClearSettings(bool& clear_depth_before_draw, GLuint iteration_index, GLfloat& depth_value)
{
	static const GLfloat step = 1.0 / 16.0;

	clear_depth_before_draw = true;

	depth_value = step * (GLfloat)iteration_index;
}

/** Get number of draw call to be executed during test
 *
 * @return 18
 **/
GLuint DepthRangeDepthTest::getDrawCallsNumber()
{
	return 18;
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string DepthRangeDepthTest::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "#ifdef GL_ES\n"
								  "precision highp float;\n"
								  "#endif\n"
								  "out float fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gl_FragCoord.z;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string DepthRangeDepthTest::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 16)         in;\n"
								  "layout(triangle_strip, max_vertices = 8) out;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    const float top_z    = 1.0;\n"
								  "    const float bottom_z = -1.0;\n"
								  "\n"
								  "    /* Bottom */\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, -1, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 0, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, -1, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 0, bottom_z, 1);\n"
								  "    EmitVertex();\n"
								  "\n"
								  "    /* Top */\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 0, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(-1, 1, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 0, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    gl_ViewportIndex = gl_InvocationID;\n"
								  "    gl_Position  = vec4(1, 1, top_z, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get test type
 *
 * @return DEPTHRANGE
 **/
DrawTestBase::TEST_TYPE DepthRangeDepthTest::getTestType()
{
	return DEPTHRANGE;
}

/** Prepare textures used as framebuffer's attachments for current draw call
 *
 * @param texture_0 R32F texture
 * @param texture_1 D32F texture
 **/
void DepthRangeDepthTest::prepareTextures(Utils::texture& texture_0, Utils::texture& texture_1)
{
	prepareTextureR32F(texture_0);
	prepareTextureD32F(texture_1);
}

/** Attach textures to framebuffer
 *
 * @param framebuffer Framebuffer instance
 * @param texture_0   Texture attached as color 0
 * @param texture_1   Texture attached as depth
 **/
void DepthRangeDepthTest::setupFramebuffer(Utils::framebuffer& framebuffer, Utils::texture& texture_0,
										   Utils::texture& texture_1)
{
	framebuffer.attachTexture(GL_COLOR_ATTACHMENT0, texture_0.m_id, m_width, m_height);
	framebuffer.attachTexture(GL_DEPTH_ATTACHMENT, texture_1.m_id, m_width, m_height);
}

/** Set up viewports
 *
 * @param ignored
 * @param iteration_index Index of iteration for given test type
 **/
void DepthRangeDepthTest::setupViewports(TEST_TYPE /* type */, GLuint iteration_index)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	DEPTH_RANGE_METHOD method;
	switch (iteration_index)
	{
	case 0:
	case 1:
		method = (DEPTH_RANGE_METHOD)iteration_index;
		break;
	default:
		TCU_FAIL("Invalid value");
	}
	setup16x2Depths(method);

	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Enable");
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
ProvokingVertex::ProvokingVertex(deqp::Context& context, const glcts::ExtParameters& extParams)
	: DrawTestBase(context, extParams, "provoking_vertex", "Test verifies that provoking vertex work as expected")
{
	/* Nothing to be done here */
}

/** Check if R32I texture is filled with 4x4 regions of increasing values <0:15>
 *
 * @param texture_0 Verified texture
 * @param ignored
 * @param ignored
 *
 * @return True if texture_0 is filled with expected pattern
 **/
bool ProvokingVertex::checkResults(Utils::texture& texture_0, Utils::texture& /* texture_1 */,
								   GLuint /*draw_call_index */)
{
	static const GLuint layer_size = m_width * m_height;

	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextType& context_type = m_context.getRenderContext().getType();

	GLint layer_mode	= 0;
	GLint viewport_mode = 0;
	gl.getIntegerv(GL_LAYER_PROVOKING_VERTEX, &layer_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
	gl.getIntegerv(GL_VIEWPORT_INDEX_PROVOKING_VERTEX, &viewport_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if ((GL_UNDEFINED_VERTEX == layer_mode) || (GL_UNDEFINED_VERTEX == viewport_mode))
	{
		/* Results are undefined, therefore it does not make sense to verify them */
		return true;
	}

	bool  check_result = true;
	GLint provoking	= 0;

	std::vector<GLint> texture_data;
	texture_data.resize(layer_size * m_r32ix4_depth);
	texture_0.get(GL_RED_INTEGER, GL_INT, &texture_data[0]);

	if (glu::isContextTypeGLCore(context_type))
	{
		gl.getIntegerv(GL_PROVOKING_VERTEX, &provoking);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");
	}
	else
	{
		DE_ASSERT(glu::isContextTypeES(context_type));
		/* ES doesn't have provoking vertex control, so it's always LAST */
		provoking = GL_LAST_VERTEX_CONVENTION;
	}

	GLuint expected_layer	= 0;
	GLint  expected_viewport = 0;

	/* Mode is 1st, or mode is provoking and provoking is 1st */
	if ((GL_FIRST_VERTEX_CONVENTION == layer_mode) ||
		((GL_PROVOKING_VERTEX == layer_mode) && (GL_FIRST_VERTEX_CONVENTION == provoking)))
	{
		expected_layer = 0;
	}
	else
	{
		expected_layer = 2;
	}

	if ((GL_FIRST_VERTEX_CONVENTION == viewport_mode) ||
		((GL_PROVOKING_VERTEX == viewport_mode) && (GL_FIRST_VERTEX_CONVENTION == provoking)))
	{
		expected_viewport = 0;
	}
	else
	{
		expected_viewport = 2;
	}

	for (GLuint layer = 0; layer < m_r32ix4_depth; ++layer)
	{
		GLint* layer_data = &texture_data[layer * layer_size];
		GLint  viewport   = 0;

		for (GLuint y = 0; y < 2; ++y)
		{
			for (GLuint x = 0; x < 2; ++x)
			{
				/* If layer and viewport are expected ones, than result shall be 1, otherwise -1. */
				const GLint expected_value = ((expected_viewport == viewport) && (expected_layer == layer)) ? 1 : -1;

				bool result = checkRegionR32I(x, y, m_width / 2, m_height / 2, expected_value, layer_data);

				if (false == result)
				{
					check_result = false;
					goto end;
				}

				viewport += 1;
			}
		}
	}

end:
	return check_result;
}

/** Get string with fragment shader source code
 *
 * @return Fragment shader source
 **/
std::string ProvokingVertex::getFragmentShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "flat in  int gs_fs_color;\n"
								  "     out int fs_out_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fs_out_color = gs_fs_color;\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get string with geometry shader source code
 *
 * @return Geometry shader source
 **/
std::string ProvokingVertex::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "${GEOMETRY_SHADER_ENABLE}\n"
								  "${VIEWPORT_ARRAY_ENABLE}\n"
								  "\n"
								  "layout(points, invocations = 1)          in;\n"
								  "layout(triangle_strip, max_vertices = 6) out;\n"
								  "\n"
								  "flat out int gs_fs_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    /* Left-bottom half */\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 0;\n"
								  "    gl_Layer         = 0;\n"
								  "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 1;\n"
								  "    gl_Layer         = 1;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 2;\n"
								  "    gl_Layer         = 2;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "\n"
								  "    /* Right-top half */\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 0;\n"
								  "    gl_Layer         = 0;\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 1;\n"
								  "    gl_Layer         = 1;\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    gs_fs_color      = 1;\n"
								  "    gl_ViewportIndex = 2;\n"
								  "    gl_Layer         = 2;\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "    EndPrimitive();\n"
								  "}\n"
								  "\n";

	std::string result = source;

	return result;
}

/** Get test type
 *
 * @return PROVOKING
 **/
DrawTestBase::TEST_TYPE ProvokingVertex::getTestType()
{
	return PROVOKING;
}

/** Prepare textures used as framebuffer's attachments for current draw call
 *
 * @param texture_0 R32I texture
 * @param ignored
 **/
void ProvokingVertex::prepareTextures(Utils::texture& texture_0, Utils::texture& /* texture_1 */)
{
	prepareTextureR32Ix4(texture_0);
}

} /* ViewportArray namespace */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
ViewportArrayTests::ViewportArrayTests(deqp::Context& context, const glcts::ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "viewport_array", "Verifies \"viewport_array\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void ViewportArrayTests::init(void)
{
	addChild(new ViewportArray::APIErrors(m_context, m_extParams));
	addChild(new ViewportArray::Queries(m_context, m_extParams));
	addChild(new ViewportArray::ViewportAPI(m_context, m_extParams));
	addChild(new ViewportArray::ScissorAPI(m_context, m_extParams));
	addChild(new ViewportArray::DepthRangeAPI(m_context, m_extParams));
	addChild(new ViewportArray::ScissorTestStateAPI(m_context, m_extParams));
	addChild(new ViewportArray::DrawToSingleLayerWithMultipleViewports(m_context, m_extParams));
	addChild(new ViewportArray::DynamicViewportIndex(m_context, m_extParams));
	addChild(new ViewportArray::DrawMulitpleViewportsWithSingleInvocation(m_context, m_extParams));
	addChild(new ViewportArray::ViewportIndexSubroutine(m_context, m_extParams));
	addChild(new ViewportArray::DrawMultipleLayers(m_context, m_extParams));
	addChild(new ViewportArray::Scissor(m_context, m_extParams));
	addChild(new ViewportArray::ScissorZeroDimension(m_context, m_extParams));
	addChild(new ViewportArray::ScissorClear(m_context, m_extParams));
	addChild(new ViewportArray::DepthRange(m_context, m_extParams));
	addChild(new ViewportArray::DepthRangeDepthTest(m_context, m_extParams));
	addChild(new ViewportArray::ProvokingVertex(m_context, m_extParams));
}

} /* glcts namespace */
