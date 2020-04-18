/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  glcRobustBufferAccessBehaviorTests.cpp
 * \brief Implements conformance tests for "Robust Buffer Access Behavior" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcRobustBufferAccessBehaviorTests.hpp"

#include "deSharedPtr.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"

#include <cstring>
#include <string>

using namespace glw;

namespace glcts
{
namespace RobustBufferAccessBehavior
{
/* Buffer constants */
const GLuint Buffer::m_invalid_id = -1;

const GLenum Buffer::m_targets[Buffer::m_n_targets] = {
	GL_ARRAY_BUFFER,			  /*  0 */
	GL_ATOMIC_COUNTER_BUFFER,	 /*  1 */
	GL_COPY_READ_BUFFER,		  /*  2 */
	GL_COPY_WRITE_BUFFER,		  /*  3 */
	GL_DISPATCH_INDIRECT_BUFFER,  /*  4 */
	GL_DRAW_INDIRECT_BUFFER,	  /*  5 */
	GL_ELEMENT_ARRAY_BUFFER,	  /*  6 */
	GL_PIXEL_PACK_BUFFER,		  /*  7 */
	GL_PIXEL_UNPACK_BUFFER,		  /*  8 */
	GL_QUERY_BUFFER,			  /*  9 */
	GL_SHADER_STORAGE_BUFFER,	 /* 10 */
	GL_TRANSFORM_FEEDBACK_BUFFER, /* 11 */
	GL_UNIFORM_BUFFER,			  /* 12 */
};

/** Constructor.
 *
 * @param context CTS context.
 **/
Buffer::Buffer(const glw::Functions& gl) : m_id(m_invalid_id), m_gl(gl), m_target(GL_ARRAY_BUFFER)
{
}

/** Destructor
 *
 **/
Buffer::~Buffer()
{
	Release();
}

/** Initialize buffer instance
 *
 * @param target Buffer target
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::InitData(glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size, const glw::GLvoid* data)
{
	/* Delete previous buffer instance */
	Release();

	m_target = target;

	Generate(m_gl, m_id);
	Bind(m_gl, m_id, m_target);
	Data(m_gl, m_target, usage, size, data);
}

/** Release buffer instance
 *
 **/
void Buffer::Release()
{
	if (m_invalid_id != m_id)
	{
		m_gl.deleteBuffers(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Binds buffer to its target
 *
 **/
void Buffer::Bind() const
{
	Bind(m_gl, m_id, m_target);
}

/** Binds indexed buffer
 *
 * @param index <index> parameter
 **/
void Buffer::BindBase(glw::GLuint index) const
{
	BindBase(m_gl, m_id, m_target, index);
}

/** Bind buffer to given target
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param target Buffer target
 **/
void Buffer::Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target)
{
	gl.bindBuffer(target, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");
}

/** Binds indexed buffer
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param target Buffer target
 * @param index  <index> parameter
 **/
void Buffer::BindBase(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index)
{
	gl.bindBufferBase(target, index, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferBase");
}

/** Allocate memory for buffer and sends initial content
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::Data(const glw::Functions& gl, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
				  const glw::GLvoid* data)
{
	gl.bufferData(target, size, data, usage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");
}

/** Generate buffer
 *
 * @param gl     GL functions
 * @param out_id Id of buffer
 **/
void Buffer::Generate(const glw::Functions& gl, glw::GLuint& out_id)
{
	GLuint id = m_invalid_id;

	gl.genBuffers(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Got invalid id");
	}

	out_id = id;
}

/** Update range of buffer
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param offset Offset in buffer
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::SubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
					 glw::GLvoid* data)
{
	gl.bufferSubData(target, offset, size, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferSubData");
}

/* Framebuffer constants */
const GLuint Framebuffer::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
Framebuffer::Framebuffer(const glw::Functions& gl) : m_id(m_invalid_id), m_gl(gl)
{
	/* Nothing to done here */
}

/** Destructor
 *
 **/
Framebuffer::~Framebuffer()
{
	Release();
}

/** Release texture instance
 *
 **/
void Framebuffer::Release()
{
	if (m_invalid_id != m_id)
	{
		m_gl.deleteFramebuffers(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Attach texture to specified attachment
 *
 * @param gl         GL functions
 * @param target     Framebuffer target
 * @param attachment Attachment
 * @param texture_id Texture id
 * @param level      Level of mipmap
 * @param width      Texture width
 * @param height     Texture height
 **/
void Framebuffer::AttachTexture(const glw::Functions& gl, glw::GLenum target, glw::GLenum attachment,
								glw::GLuint texture_id, glw::GLint level, glw::GLuint width, glw::GLuint height)
{
	gl.framebufferTexture(target, attachment, texture_id, level);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture");

	gl.viewport(0 /* x */, 0 /* y */, width, height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}

/** Binds framebuffer to DRAW_FRAMEBUFFER
 *
 * @param gl     GL functions
 * @param target Framebuffer target
 * @param id     ID of framebuffer
 **/
void Framebuffer::Bind(const glw::Functions& gl, glw::GLenum target, glw::GLuint id)
{
	gl.bindFramebuffer(target, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
}

/** Generate framebuffer
 *
 **/
void Framebuffer::Generate(const glw::Functions& gl, glw::GLuint& out_id)
{
	GLuint id = m_invalid_id;

	gl.genFramebuffers(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Invalid id");
	}

	out_id = id;
}

/* Program constants */
const GLuint Program::m_invalid_id = 0;

/** Constructor.
 *
 * @param context CTS context.
 **/
Program::Program(const glw::Functions& gl)
	: m_id(m_invalid_id)
	, m_compute(gl)
	, m_fragment(gl)
	, m_geometry(gl)
	, m_tess_ctrl(gl)
	, m_tess_eval(gl)
	, m_vertex(gl)
	, m_gl(gl)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Program::~Program()
{
	Release();
}

/** Initialize program instance
 *
 * @param compute_shader                Compute shader source code
 * @param fragment_shader               Fragment shader source code
 * @param geometry_shader               Geometry shader source code
 * @param tesselation_control_shader    Tesselation control shader source code
 * @param tesselation_evaluation_shader Tesselation evaluation shader source code
 * @param vertex_shader                 Vertex shader source code
 **/
void Program::Init(const std::string& compute_shader, const std::string& fragment_shader,
				   const std::string& geometry_shader, const std::string& tesselation_control_shader,
				   const std::string& tesselation_evaluation_shader, const std::string& vertex_shader)
{
	/* Delete previous program */
	Release();

	/* Initialize shaders */
	m_compute.Init(GL_COMPUTE_SHADER, compute_shader);
	m_fragment.Init(GL_FRAGMENT_SHADER, fragment_shader);
	m_geometry.Init(GL_GEOMETRY_SHADER, geometry_shader);
	m_tess_ctrl.Init(GL_TESS_CONTROL_SHADER, tesselation_control_shader);
	m_tess_eval.Init(GL_TESS_EVALUATION_SHADER, tesselation_evaluation_shader);
	m_vertex.Init(GL_VERTEX_SHADER, vertex_shader);

	/* Create program, set up transform feedback and attach shaders */
	Create(m_gl, m_id);
	Attach(m_gl, m_id, m_compute.m_id);
	Attach(m_gl, m_id, m_fragment.m_id);
	Attach(m_gl, m_id, m_geometry.m_id);
	Attach(m_gl, m_id, m_tess_ctrl.m_id);
	Attach(m_gl, m_id, m_tess_eval.m_id);
	Attach(m_gl, m_id, m_vertex.m_id);

	/* Link program */
	Link(m_gl, m_id);
}

/** Release program instance
 *
 **/
void Program::Release()
{
	if (m_invalid_id != m_id)
	{
		Use(m_gl, m_invalid_id);

		m_gl.deleteProgram(m_id);
		m_id = m_invalid_id;
	}

	m_compute.Release();
	m_fragment.Release();
	m_geometry.Release();
	m_tess_ctrl.Release();
	m_tess_eval.Release();
	m_vertex.Release();
}

/** Set program as active
 *
 **/
void Program::Use() const
{
	Use(m_gl, m_id);
}

/** Attach shader to program
 *
 * @param gl         GL functions
 * @param program_id Id of program
 * @param shader_id  Id of shader
 **/
void Program::Attach(const glw::Functions& gl, glw::GLuint program_id, glw::GLuint shader_id)
{
	/* Sanity checks */
	if ((m_invalid_id == program_id) || (Shader::m_invalid_id == shader_id))
	{
		return;
	}

	gl.attachShader(program_id, shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
}

/** Create program instance
 *
 * @param gl     GL functions
 * @param out_id Id of program
 **/
void Program::Create(const glw::Functions& gl, glw::GLuint& out_id)
{
	const GLuint id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Failed to create program");
	}

	out_id = id;
}

/** Link program
 *
 * @param gl GL functions
 * @param id Id of program
 **/
void Program::Link(const glw::Functions& gl, glw::GLuint id)
{
	GLint status = GL_FALSE;

	gl.linkProgram(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

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

		TCU_FAIL(message.c_str());
	}
}

/** Use program
 *
 * @param gl GL functions
 * @param id Id of program
 **/
void Program::Use(const glw::Functions& gl, glw::GLuint id)
{
	gl.useProgram(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");
}

/* Shader's constants */
const GLuint Shader::m_invalid_id = 0;

/** Constructor.
 *
 * @param context CTS context.
 **/
Shader::Shader(const glw::Functions& gl) : m_id(m_invalid_id), m_gl(gl)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
Shader::~Shader()
{
	Release();
}

/** Initialize shader instance
 *
 * @param stage  Shader stage
 * @param source Source code
 **/
void Shader::Init(glw::GLenum stage, const std::string& source)
{
	if (true == source.empty())
	{
		/* No source == no shader */
		return;
	}

	/* Delete any previous shader */
	Release();

	Create(m_gl, stage, m_id);
	Source(m_gl, m_id, source);

	Compile(m_gl, m_id);
}

/** Release shader instance
 *
 **/
void Shader::Release()
{
	if (m_invalid_id != m_id)
	{
		m_gl.deleteShader(m_id);
		m_id = m_invalid_id;
	}
}

/** Compile shader
 *
 * @param gl GL functions
 * @param id Shader id
 **/
void Shader::Compile(const glw::Functions& gl, glw::GLuint id)
{
	GLint status = GL_FALSE;

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

		TCU_FAIL(message.c_str());
	}
}

/** Create shader
 *
 * @param gl     GL functions
 * @param stage  Shader stage
 * @param out_id Shader id
 **/
void Shader::Create(const glw::Functions& gl, glw::GLenum stage, glw::GLuint& out_id)
{
	const GLuint id = gl.createShader(stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Failed to create shader");
	}

	out_id = id;
}

/** Set shader's source code
 *
 * @param gl     GL functions
 * @param id     Shader id
 * @param source Shader source code
 **/
void Shader::Source(const glw::Functions& gl, glw::GLuint id, const std::string& source)
{
	const GLchar* code = source.c_str();

	gl.shaderSource(id, 1 /* count */, &code, 0 /* lengths */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");
}

/* Texture static fields */
const GLuint Texture::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
Texture::Texture(const glw::Functions& gl) : m_id(m_invalid_id), m_gl(gl)
{
	/* Nothing to done here */
}

/** Destructor
 *
 **/
Texture::~Texture()
{
	Release();
}

/** Release texture instance
 *
 **/
void Texture::Release()
{
	if (m_invalid_id != m_id)
	{
		m_gl.deleteTextures(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Bind texture to target
 *
 * @param gl       GL functions
 * @param id       Id of texture
 * @param tex_type Type of texture
 **/
void Texture::Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target)
{
	gl.bindTexture(target, id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");
}

/** Set contents of compressed texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param internal_format Format of data
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param image_size      Size of data
 * @param data            Buffer with image data
 **/
void Texture::CompressedImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level,
							  glw::GLenum internal_format, glw::GLuint width, glw::GLuint height, glw::GLuint depth,
							  glw::GLsizei image_size, const glw::GLvoid* data)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.compressedTexImage1D(target, level, internal_format, width, 0 /* border */, image_size, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CompressedTexImage1D");
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.compressedTexImage2D(target, level, internal_format, width, height, 0 /* border */, image_size, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CompressedTexImage2D");
		break;
	case GL_TEXTURE_CUBE_MAP:
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		gl.compressedTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, internal_format, width, height, 0 /* border */,
								image_size, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CompressedTexImage2D");
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		gl.compressedTexImage3D(target, level, internal_format, width, height, depth, 0 /* border */, image_size, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CompressedTexImage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Generate texture instance
 *
 * @param gl     GL functions
 * @param out_id Id of texture
 **/
void Texture::Generate(const glw::Functions& gl, glw::GLuint& out_id)
{
	GLuint id = m_invalid_id;

	gl.genTextures(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Invalid id");
	}

	out_id = id;
}

/** Get texture data
 *
 * @param gl       GL functions
 * @param target   Texture target
 * @param format   Format of data
 * @param type     Type of data
 * @param out_data Buffer for data
 **/
void Texture::GetData(const glw::Functions& gl, glw::GLint level, glw::GLenum target, glw::GLenum format,
					  glw::GLenum type, glw::GLvoid* out_data)
{
	gl.getTexImage(target, level, format, type, out_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");
}

/** Get texture data
 *
 * @param gl       GL functions
 * @param id       Texture id
 * @param level    Mipmap level
 * @param width    Texture width
 * @param height   Texture height
 * @param format   Format of data
 * @param type     Type of data
 * @param out_data Buffer for data
 **/
void Texture::GetData(const glw::Functions& gl, glw::GLuint id, glw::GLint level, glw::GLuint width, glw::GLuint height,
					  glw::GLenum format, glw::GLenum type, glw::GLvoid* out_data)
{
	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, level);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	gl.readPixels(0, 0, width, height, format, type, out_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels");

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
}

/** Generate texture instance
 *
 * @param gl     GL functions
 * @param target Texture target
 * @param level  Mipmap level
 * @param pname  Parameter to query
 * @param param  Result of query
 **/
void Texture::GetLevelParameter(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum pname,
								glw::GLint* param)
{
	gl.getTexLevelParameteriv(target, level, pname, param);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexLevelParameteriv");
}

/** Set contents of texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param internal_format Format of data
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param data            Buffer with image data
 **/
void Texture::Image(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum internal_format,
					glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					const glw::GLvoid* data)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texImage1D(target, level, internal_format, width, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage1D");
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texImage2D(target, level, internal_format, width, height, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");
		break;
	case GL_TEXTURE_CUBE_MAP:
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		gl.texImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, internal_format, width, height, 0 /* border */, format,
					  type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		gl.texImage3D(target, level, internal_format, width, height, depth, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Allocate storage for texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param levels          Number of levels
 * @param internal_format Internal format of texture
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 **/
void Texture::Storage(const glw::Functions& gl, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
					  glw::GLuint width, glw::GLuint height, glw::GLuint depth)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texStorage1D(target, levels, internal_format, width);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
	case GL_TEXTURE_CUBE_MAP:
		gl.texStorage2D(target, levels, internal_format, width, height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.texStorage2DMultisample(target, levels, internal_format, width, height, GL_FALSE);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2DMultisample");
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		gl.texStorage3D(target, levels, internal_format, width, height, depth);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/** Set contents of texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param x               X offset
 * @param y               Y offset
 * @param z               Z offset
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param pixels          Buffer with image data
 **/
void Texture::SubImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLint x, glw::GLint y,
					   glw::GLint z, glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
					   glw::GLenum type, const glw::GLvoid* pixels)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texSubImage1D(target, level, x, width, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage1D");
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
		gl.texSubImage2D(target, level, x, y, width, height, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");
		break;
	case GL_TEXTURE_CUBE_MAP:
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, x, y, width, height, format, type, pixels);
		gl.texSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, x, y, width, height, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
		gl.texSubImage3D(target, level, x, y, z, width, height, depth, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/* VertexArray constants */
const GLuint VertexArray::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
VertexArray::VertexArray(const glw::Functions& gl) : m_id(m_invalid_id), m_gl(gl)
{
}

/** Destructor
 *
 **/
VertexArray::~VertexArray()
{
	Release();
}

/** Release vertex array object instance
 *
 **/
void VertexArray::Release()
{
	if (m_invalid_id != m_id)
	{
		Bind(m_gl, 0);

		m_gl.deleteVertexArrays(1, &m_id);

		m_id = m_invalid_id;
	}
}

/** Binds Vertex array object
 *
 * @param gl GL functions
 * @param id ID of vertex array object
 **/
void VertexArray::Bind(const glw::Functions& gl, glw::GLuint id)
{
	gl.bindVertexArray(id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArray");
}

/** Generates Vertex array object
 *
 * @param gl     GL functions
 * @param out_id ID of vertex array object
 **/
void VertexArray::Generate(const glw::Functions& gl, glw::GLuint& out_id)
{
	GLuint id = m_invalid_id;

	gl.genVertexArrays(1, &id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	if (m_invalid_id == id)
	{
		TCU_FAIL("Invalid id");
	}

	out_id = id;
}

template <typename TYPE>
void initPixels(std::vector<TYPE>& pixels, GLuint n_pixels, GLuint n_channels)
{
	if (n_channels == 1)
	{
		for (GLuint i = 0; i < n_pixels; ++i)
			pixels[i] = static_cast<TYPE>(i);
	}
	else if (n_channels == 2)
	{
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			GLuint idx		= i * 2;
			pixels[idx]		= static_cast<TYPE>(i);
			pixels[idx + 1] = pixels[idx];
		}
	}
	else if (n_channels == 4)
	{
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			GLuint idx		= i * 4;
			pixels[idx]		= static_cast<TYPE>(i);
			pixels[idx + 1] = pixels[idx];
			pixels[idx + 2] = pixels[idx];
			pixels[idx + 3] = pixels[idx];
		}
	}
	else
		TCU_FAIL("Unsuported number of channels");
}

RobustnessBase::RobustnessBase(tcu::TestContext& testCtx, const char* name, const char* description,
							   glu::ApiType apiType)
	: tcu::TestCase(testCtx, name, description), m_api_type(apiType), m_has_khr_robust_buffer_access(false)
{
}

glu::RenderContext* RobustnessBase::createRobustContext(glu::ResetNotificationStrategy reset)
{
	// Create test context to verify if required extensions are available
	{
		deqp::Context			context(m_testCtx, glu::ContextType(m_api_type));
		const glu::ContextInfo& contextInfo  = context.getContextInfo();
		glu::ContextType		context_type = context.getRenderContext().getType();
		if (!contextInfo.isExtensionSupported("GL_KHR_robustness") &&
			!contextSupports(context_type, glu::ApiType::es(3, 2)))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_robustness extension not supported");
			return NULL;
		}

		m_has_khr_robust_buffer_access = contextInfo.isExtensionSupported("GL_KHR_robust_buffer_access_behavior") ||
										 contextInfo.isExtensionSupported("GL_ARB_robust_buffer_access_behavior") ||
										 contextSupports(context_type, glu::ApiType::core(4, 5));
		if (!m_has_khr_robust_buffer_access && !contextSupports(context_type, glu::ApiType::core(4, 3)))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED,
									"robust_buffer_access_behavior extension not supported");
			return NULL;
		}

		glu::GLSLVersion glslVersion   = glu::getContextTypeGLSLVersion(context_type);
		m_specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(glslVersion);
		m_context_is_es				   = glu::isContextTypeES(context_type);
	}

	glu::RenderConfig		renderCfg(glu::ContextType(m_api_type, glu::CONTEXT_ROBUST));
	const tcu::CommandLine& commandLine = m_testCtx.getCommandLine();
	glu::parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() == tcu::SURFACETYPE_WINDOW)
		renderCfg.resetNotificationStrategy = reset;
	else
		throw tcu::NotSupportedError("Test not supported in non-windowed context");

	/* Try to create core/es robusness context */
	return createRenderContext(m_testCtx.getPlatform(), commandLine, renderCfg);
}

/** Constructor
 *
 * @param testCtx Test context
 **/
VertexBufferObjectsTest::VertexBufferObjectsTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: RobustnessBase(testCtx, "vertex_buffer_objects", "Verifies that out-of-bound reads from VB result in zero",
					 apiType)
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult VertexBufferObjectsTest::iterate()
{
	de::SharedPtr<glu::RenderContext> robustContext(createRobustContext());
	if (!robustContext.get())
		return STOP;

	static const GLuint invalid_elements[] = {
		9, 1, 12, 10, 2, 3, 11, 3, 4, 12, 4, 5, 13, 5, 6, 14, 6, 7, 15, 7, 8, 16, 8, 1,
	};

	static const GLuint valid_elements[] = {
		0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 1,
	};

	static const GLfloat vertices[] = {
		0.0f,  0.0f,  0.0f, /* 0 */
		-1.0f, 0.0f,  0.0f, /* 1 */
		-1.0f, 1.0f,  0.0f, /* 2 */
		0.0f,  1.0f,  0.0f, /* 3 */
		1.0f,  1.0f,  0.0f, /* 4 */
		1.0f,  0.0f,  0.0f, /* 5 */
		1.0f,  -1.0f, 0.0f, /* 6 */
		0.0f,  -1.0f, 0.0f, /* 7 */
		-1.0f, -1.0f, 0.0f, /* 8 */
	};

	static const GLuint height	 = 8;
	static const GLuint n_vertices = 24;
	static const GLuint width	  = 8;

	/* GL entry points */
	const Functions& gl = robustContext->getFunctions();

	/* Test case objects */
	Framebuffer framebuffer(gl);
	Program		program(gl);
	Texture		texture(gl);
	Buffer		elements_buffer(gl);
	Buffer		vertices_buffer(gl);
	VertexArray vao(gl);

	/* Vertex array */
	VertexArray::Generate(gl, vao.m_id);
	VertexArray::Bind(gl, vao.m_id);

	/* Buffers initialization */
	elements_buffer.InitData(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, sizeof(valid_elements), valid_elements);
	vertices_buffer.InitData(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, sizeof(vertices), vertices);

	/* Texture initialization */
	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);
	Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R8UI, width, height, 0);
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	/* Framebuffer initialization*/
	Framebuffer::Generate(gl, framebuffer.m_id);
	Framebuffer::Bind(gl, GL_DRAW_FRAMEBUFFER, framebuffer.m_id);
	Framebuffer::AttachTexture(gl, GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.m_id, 0 /* level */, width,
							   height);

	/* Shaders initialization */
	program.Init("" /* cs */, getFragmentShader(), "" /* gs */, "" /* tcs */, "" /* tes */, getVertexShader());
	Program::Use(gl, program.m_id);

	/* Vertex buffer initialization */
	vertices_buffer.Bind();
	gl.bindVertexBuffer(0 /* bindindex = location */, vertices_buffer.m_id, 0 /* offset */, 12 /* stride */);
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, NULL);
	gl.enableVertexAttribArray(0 /* location */);

	/* Binding elements/indices buffer */
	elements_buffer.Bind();

	cleanTexture(gl, texture.m_id);

	gl.drawElements(GL_TRIANGLES, n_vertices, GL_UNSIGNED_INT, 0 /* indices */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements");

	if (false == verifyValidResults(gl, texture.m_id))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid result for valid input" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return tcu::TestNode::STOP;
	}

	/* Generate invalid data sets */
	const GLuint invalid_elements_offsets[] = {
		0,				 // close fetch
		4 * 1024,		 // near fetch (4K of the end of the object)
		1024 * 1024,	 // medium fetch (1MB past the end of the object)
		10 * 1024 * 1024 // high fetch (10MB beyond the end of the object)
	};
	const GLuint invalid_buffers_count = DE_LENGTH_OF_ARRAY(invalid_elements_offsets);
	const GLuint item_count			   = DE_LENGTH_OF_ARRAY(invalid_elements);
	GLuint		 invalid_elements_set[invalid_buffers_count][item_count];
	for (GLuint buffer_index = 0; buffer_index < invalid_buffers_count; ++buffer_index)
	{
		for (GLuint item_index = 0; item_index < item_count; ++item_index)
			invalid_elements_set[buffer_index][item_index] =
				invalid_elements[item_index] + invalid_elements_offsets[buffer_index];
	}

	for (GLuint buffer_index = 0; buffer_index < invalid_buffers_count; ++buffer_index)
	{
		/* Create elements/indices buffer */
		elements_buffer.InitData(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, sizeof(invalid_elements_set[buffer_index]),
								 invalid_elements_set[buffer_index]);
		elements_buffer.Bind();

		cleanTexture(gl, texture.m_id);

		gl.drawElements(GL_TRIANGLES, n_vertices, GL_UNSIGNED_INT, 0 /* indices */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements");

		if (false == verifyInvalidResults(gl, texture.m_id))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid result for invalid input"
							   << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return tcu::TestNode::STOP;
		}
	}

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return tcu::TestNode::STOP;
}

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string VertexBufferObjectsTest::getFragmentShader()
{
	const char* source = "${VERSION}\n"
						 "layout (location = 0) out lowp uvec4 out_fs_color;\n"
						 "void main()\n"
						 "{\n"
						 "    out_fs_color = uvec4(1, 255, 255, 255);\n"
						 "}\n";
	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string VertexBufferObjectsTest::getVertexShader()
{
	const char* source = "${VERSION}\n"
						 "layout (location = 0) in vec4 in_vs_position;\n"
						 "void main()\n"
						 "{\n"
						 "    gl_Position = in_vs_position;\n"
						 "}\n";

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Fill texture with value 128
 *
 * @param texture_id Id of texture
 **/
void VertexBufferObjectsTest::cleanTexture(const Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height = 8;
	static const GLuint width  = 8;

	GLubyte pixels[width * height];
	for (GLuint i = 0; i < width * height; ++i)
	{
		pixels[i] = 128;
	}

	Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

	Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level  */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, 0 /* depth */,
					  GL_RED_INTEGER, GL_UNSIGNED_BYTE, pixels);

	/* Unbind */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);
}

/** Verifies that texutre is not filled with 1
 *
 * @param texture_id Id of texture
 *
 * @return false when image is filled with 1, true otherwise
 **/
bool VertexBufferObjectsTest::verifyInvalidResults(const Functions& gl, glw::GLuint texture_id)
{
	// In OpenGL ES there is undefined out-of-bound behavior - no verification
	if (m_context_is_es)
		return true;
	return !verifyResults(gl, texture_id);
}

/** Verifies that texutre is filled with 1
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with 1, false otherwise
 **/
bool VertexBufferObjectsTest::verifyValidResults(const Functions& gl, glw::GLuint texture_id)
{
	return verifyResults(gl, texture_id);
}

/** Verifies that texutre is filled with 1
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with 1, false otherwise
 **/
bool VertexBufferObjectsTest::verifyResults(const Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height = 8;
	static const GLuint width  = 8;
	GLuint				pixel_size	 = 4 * sizeof(GLuint);
	GLuint				expected_value = 1;

	std::vector<GLubyte> pixels(width * height * pixel_size, 0);
	Texture::Bind(gl, texture_id, GL_TEXTURE_2D);
	Texture::GetData(gl, texture_id, 0 /* level */, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	/* Verify */
	for (GLuint i = 0; i < pixels.size(); i += pixel_size)
	{
		if (expected_value != pixels[i])
			return false;
	}

	return true;
}

/** Constructor
 *
 * @param testCtx Test context
 **/
TexelFetchTest::TexelFetchTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: RobustnessBase(testCtx, "texel_fetch", "Verifies that out-of-bound fetches from texture result in zero", apiType)
	, m_test_case(R8)
{
	/* Nothing to be done */
}

/** Constructor
 *
 * @param testCtx Test context
 * @param name Test name
 * @param description Test description
 * @param apiType Api type
 **/
TexelFetchTest::TexelFetchTest(tcu::TestContext& testCtx, const char* name, const char* description,
							   glu::ApiType apiType)
	: RobustnessBase(testCtx, name, description, apiType), m_test_case(R8)
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult TexelFetchTest::iterate()
{
	de::SharedPtr<glu::RenderContext> robustContext(createRobustContext());
	if (!robustContext.get())
		return STOP;

	/* Constants */
	static const GLuint height = 16;
	static const GLuint width  = 16;

	/* GL entry points */
	const Functions& gl = robustContext->getFunctions();

	/* Test result indicator */
	bool test_result = true;

	GLuint invalid_fetch_offsets[] = {
		16,   // near fetch
		512,  // medium fetch
		1008, // high fetch
	};
	GLuint fetch_offsets_count = sizeof(invalid_fetch_offsets) / sizeof(GLuint);
	glu::ContextType contextType		 = robustContext->getType();

	/* Iterate over all cases */
	for (; m_test_case < LAST; m_test_case = (TEST_CASES)((GLuint)m_test_case + 1))
	{
		GLint  level		  = 0;
		GLenum texture_target = GL_TEXTURE_2D;

		if (R32UI_MULTISAMPLE == m_test_case || RG8_SNORM == m_test_case)
		{
			// 1. RG8_SNORM case:
			// Skip RG8_SNORM format case.
			// RG8_SNORM is not required to be used as a render target
			// OpenGL 4.5 Core Spec, Page 197
			//
			// 2. R32UI_MULTISAMPLE case
			// Skip test in multi sample case
			// texelFetch with invalid lod plane results undefined value
			// OpenGL 4.5 Core Spec, around page 377
			m_test_case = (TEST_CASES)((GLuint)m_test_case + 1);
			continue;
		}

		/* */
		Texture		destination_texture(gl);
		Framebuffer framebuffer(gl);
		Texture		source_texture(gl);
		Program		program(gl);
		VertexArray vao(gl);

		/* Prepare VAO */
		VertexArray::Generate(gl, vao.m_id);
		VertexArray::Bind(gl, vao.m_id);

		/* Prepare textures */
		Texture::Generate(gl, destination_texture.m_id);
		Texture::Generate(gl, source_texture.m_id);

		if (R32UI_MULTISAMPLE == m_test_case)
		{
			GLint max_integer_samples;
			gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &max_integer_samples);
			GLint max_image_samples;
			gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
			if (max_integer_samples < 4 || max_image_samples < 4)
			{
				/* prepareTexture() hard-codes 4 samples (n_levels) for
				 * R32UI_MULTISAMPLE case. This value exceeds the required
				 * min-max value (1 in OpenGL ES 3.2) and is not supported
				 * by all implementations.
				 *
				 * Also, the test uses a compute shader with images
				 * to upload the texture so max_image_samples >= 4
				 * is also required.
				 */
				m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << getTestCaseName() << " not supported"
								   << tcu::TestLog::EndMessage;

				continue;
			}
		}

		prepareTexture(gl, false, destination_texture.m_id);
		prepareTexture(gl, true, source_texture.m_id);

		/* Select FBO settings */
		if (R32UI_MIPMAP == m_test_case)
		{
			level = 1;
		}
		else if (R32UI_MULTISAMPLE == m_test_case)
		{
			texture_target = GL_TEXTURE_2D_MULTISAMPLE;
		}

		/* Prepare FBO */
		Framebuffer::Generate(gl, framebuffer.m_id);
		Framebuffer::Bind(gl, GL_DRAW_FRAMEBUFFER, framebuffer.m_id);
		Framebuffer::AttachTexture(gl, GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, destination_texture.m_id, level,
								   width, height);

		/* Prepare valid program */
		program.Init("" /* cs */, getFragmentShader(contextType, true), getGeometryShader(), "" /* tcs */, "" /* tes */,
					 getVertexShader());

		/* Test valid case */
		/* Set program */
		Program::Use(gl, program.m_id);

		/* Set texture */
		gl.activeTexture(GL_TEXTURE0); /* location = 0 */
		GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");
		Texture::Bind(gl, source_texture.m_id, texture_target);
		gl.uniform1i(0 /* location */, 0 /* texture unit */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		/* Check if setup is supported */
		GLenum fbo_status = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CheckFramebufferStatus");
		if (GL_FRAMEBUFFER_COMPLETE != fbo_status)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << getTestCaseName() << " not supported"
							   << tcu::TestLog::EndMessage;

			continue;
		}

		/* Enable multisampling */
		if (R32UI_MULTISAMPLE == m_test_case)
		{
			gl.enable(GL_MULTISAMPLE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Enable");
		}

		/* Draw */
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		{
			/* Get error from draw */
			GLenum error = gl.getError();

			/* Disable multisampling */
			if (R32UI_MULTISAMPLE == m_test_case)
			{
				gl.disable(GL_MULTISAMPLE);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Disable");
			}

			/* Handle error from draw */
			GLU_EXPECT_NO_ERROR(error, "DrawArrays");
		}

		/* Verification */
		if (false == verifyValidResults(gl, destination_texture.m_id))
		{
			test_result = false;
		}

		/* Test invalid cases */
		for (GLuint index = 0; index < fetch_offsets_count; ++index)
		{
			/* Prepare invalid program */
			program.Init("" /* cs */, getFragmentShader(contextType, false, invalid_fetch_offsets[index]),
						 getGeometryShader(), "" /* tcs */, "" /* tes */, getVertexShader());
			Program::Use(gl, program.m_id);
			Framebuffer::Bind(gl, GL_DRAW_FRAMEBUFFER, framebuffer.m_id);

			/* Set texture */
			gl.activeTexture(GL_TEXTURE0); /* location = 0 */
			GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveTexture");
			Texture::Bind(gl, source_texture.m_id, texture_target);
			gl.uniform1i(0 /* location */, 0 /* texture unit */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

			/* Draw */
			gl.clear(GL_COLOR_BUFFER_BIT);
			gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

			/* Verification */
			if (false == verifyInvalidResults(gl, destination_texture.m_id))
			{
				test_result = false;
				m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << getTestCaseName() << " failed for "
								   << invalid_fetch_offsets[index] << " offset" << tcu::TestLog::EndMessage;
			}
		}
	}

	/* Set result */
	if (true == test_result)
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

/** Prepares source code for fragment shader
 *
 * @param is_case_valid Selects if valid or invalid case is tested
 *
 * @return string with prepared code
 **/
std::string TexelFetchTest::getFragmentShader(const glu::ContextType&, bool is_case_valid, GLuint fetch_offset)
{
	const GLchar* source = "${VERSION}\n"
						   "in lowp vec2 gs_fs_tex_coord;\n"
						   "layout (location = 0) out lowp ${TYPE} out_fs_color;\n"
						   "layout (location = 0) uniform lowp ${SAMPLER} uni_texture;\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "  ivec2 point  = ivec2(gs_fs_tex_coord * 16.0 + float(${OFFSET}));\n"
						   "  out_fs_color = texelFetch(uni_texture, point, ${PLANE});\n"
						   "}\n";

	m_specializationMap["PLANE"]   = "0";
	m_specializationMap["SAMPLER"] = "sampler2D";
	m_specializationMap["TYPE"]	= "vec4";

	if (R32UI_MIPMAP == m_test_case)
	{
		m_specializationMap["PLANE"]   = "1";
		m_specializationMap["SAMPLER"] = "usampler2D";
		m_specializationMap["TYPE"]	= "uvec4";

		if (false == is_case_valid)
		{
			fetch_offset = 0;
			m_specializationMap["PLANE"] = "2";
		}
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		m_specializationMap["PLANE"]   = "9";
		m_specializationMap["SAMPLER"] = "usampler2DMS";
		m_specializationMap["TYPE"]	= "uvec4";

		if (false == is_case_valid)
		{
			fetch_offset = 0;
			m_specializationMap["PLANE"] = "gl_SampleID";
		}
	}

	std::stringstream offset;
	offset << fetch_offset;
	m_specializationMap["OFFSET"] = offset.str();

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string TexelFetchTest::getGeometryShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "layout(points)                           in;\n"
								  "layout(triangle_strip, max_vertices = 4) out;\n"
								  "\n"
								  "out vec2 gs_fs_tex_coord;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gs_fs_tex_coord = vec2(0, 0);\n"
								  "    gl_Position     = vec4(-1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "\n"
								  "    gs_fs_tex_coord = vec2(0, 1);\n"
								  "    gl_Position     = vec4(-1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "\n"
								  "    gs_fs_tex_coord = vec2(1, 0);\n"
								  "    gl_Position     = vec4(1, -1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "\n"
								  "    gs_fs_tex_coord = vec2(1, 1);\n"
								  "    gl_Position     = vec4(1, 1, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n";

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string TexelFetchTest::getVertexShader()
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gl_Position = vec4(0, 0, 0, 1);\n"
								  "}\n";
	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Returns name of current test case
 *
 * @return Name of test case
 **/
const glw::GLchar* TexelFetchTest::getTestCaseName() const
{
	const GLchar* name = "";

	switch (m_test_case)
	{
	case R8:
		name = "Sampling GL_R8 texture";
		break;
	case RG8_SNORM:
		name = "Sampling GL_RG8_SNORM  texture";
		break;
	case RGBA32F:
		name = "Sampling GL_RGBA32F  texture";
		break;
	case R32UI_MIPMAP:
		name = "Sampling mipmap of GL_32UI texture";
		break;
	case R32UI_MULTISAMPLE:
		name = "Sampling GL_32UI multisampled texture";
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	return name;
}

/** Prepare a texture
 *
 * @param is_source  Selects if texutre will be used as source or destination
 * @param texture_id Id of texutre
 **/
void TexelFetchTest::prepareTexture(const Functions& gl, bool is_source, glw::GLuint texture_id)
{
	/* Image size */
	static const GLuint image_height = 16;
	static const GLuint image_width  = 16;

	/* Texture storage parameters */
	GLuint  height			= image_height;
	GLenum  internal_format = 0;
	GLsizei n_levels		= 1;
	GLenum  target			= GL_TEXTURE_2D;
	GLuint  width			= image_width;

	/* Prepare texture storage parameters */
	switch (m_test_case)
	{
	case R8:
		internal_format = GL_R8;
		break;
	case RG8_SNORM:
		internal_format = GL_RG8_SNORM;
		break;
	case RGBA32F:
		internal_format = GL_RGBA32F;
		break;
	case R32UI_MIPMAP:
		height			= 2 * image_height;
		internal_format = GL_R32UI;
		n_levels		= 2;
		width			= 2 * image_width;
		break;
	case R32UI_MULTISAMPLE:
		internal_format = GL_R32UI;
		n_levels		= 4;
		target			= GL_TEXTURE_2D_MULTISAMPLE;
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	/* Prepare storage */
	Texture::Bind(gl, texture_id, target);
	Texture::Storage(gl, target, n_levels, internal_format, width, height, 0);

	/* Set samplers to NEAREST/NEAREST if required */
	if (R32UI_MULTISAMPLE != m_test_case)
	{
		gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	/* Destination image can be left empty */
	if (false == is_source)
	{
		Texture::Bind(gl, 0, target);
		return;
	}

	/* Prepare texture */
	if (R8 == m_test_case)
	{
		GLubyte source_pixels[image_width * image_height];
		for (GLuint i = 0; i < image_width * image_height; ++i)
		{
			source_pixels[i] = static_cast<GLubyte>(i);
		}

		Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height,
						  0 /* depth */, GL_RED, GL_UNSIGNED_BYTE, source_pixels);
	}
	else if (RG8_SNORM == m_test_case)
	{
		static const GLuint n_components = 2;

		GLbyte source_pixels[image_width * image_height * n_components];
		for (GLuint i = 0; i < image_width * image_height; ++i)
		{
			source_pixels[i * n_components + 0] = static_cast<GLubyte>((i % 16) - 8);
			source_pixels[i * n_components + 1] = static_cast<GLubyte>((i / 16) - 8);
		}

		Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height,
						  0 /* depth */, GL_RG, GL_BYTE, source_pixels);
	}
	else if (RGBA32F == m_test_case)
	{
		static const GLuint n_components = 4;

		GLfloat source_pixels[image_width * image_height * n_components];
		for (GLuint i = 0; i < image_width * image_height; ++i)
		{
			source_pixels[i * n_components + 0] = (GLfloat)(i % 16) / 16.0f;
			source_pixels[i * n_components + 1] = (GLfloat)(i / 16) / 16.0f;
			source_pixels[i * n_components + 2] = (GLfloat)i / 256.0f;
			source_pixels[i * n_components + 3] = 1.0f;
		}

		Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height,
						  0 /* depth */, GL_RGBA, GL_FLOAT, source_pixels);
	}
	else if (R32UI_MIPMAP == m_test_case)
	{
		GLuint source_pixels[image_width * image_height];
		for (GLuint i = 0; i < image_width * image_height; ++i)
		{
			source_pixels[i] = i;
		}

		Texture::SubImage(gl, GL_TEXTURE_2D, 1 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, image_width, image_height,
						  0 /* depth */, GL_RED_INTEGER, GL_UNSIGNED_INT, source_pixels);
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		/* Compute shader */
		static const GLchar* source =
			"${VERSION}\n"
			"\n"
			"layout (local_size_x = ${LOCAL_SIZE}, local_size_y = ${LOCAL_SIZE}, local_size_z = 1) in;\n"
			"layout (${QUALIFIER​S}) writeonly uniform highp uimage2DMS uni_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    const ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);\n"
			"    const uint  index = gl_WorkGroupID.y * 16U + gl_WorkGroupID.x;\n"
			"\n"
			"    imageStore(uni_image, point, 0, uvec4(index + 0U, 0, 0, 0));\n"
			"    imageStore(uni_image, point, 1, uvec4(index + 1U, 0, 0, 0));\n"
			"    imageStore(uni_image, point, 2, uvec4(index + 2U, 0, 0, 0));\n"
			"    imageStore(uni_image, point, 3, uvec4(index + 3U, 0, 0, 0));\n"
			"}\n"
			"\n";

		if (m_context_is_es)
		{
			m_specializationMap["LOCAL_SIZE"]	= "16";
			m_specializationMap["QUALIFIER​S"] = "binding = 0, r32ui";
		}
		else
		{
			m_specializationMap["LOCAL_SIZE"]	= "1";
			m_specializationMap["QUALIFIER​S"] = "location = 0";
		}

		Program		program(gl);
		std::string cs = tcu::StringTemplate(source).specialize(m_specializationMap);
		program.Init(cs, "", "", "", "", "");
		program.Use();

		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		if (!m_context_is_es)
		{
			gl.uniform1i(0 /* location */, 0 /* image unit*/);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
		}

		gl.dispatchCompute(16, 16, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");
	}

	Texture::Bind(gl, 0, target);
}

/** Verifies that texutre is filled with 0 or with (0, 0, 0, x),
 *  where x may be 0, 1 or the biggest representable integer value.
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with 0, 1 or biggest represetable integer number, false otherwise
 **/
bool TexelFetchTest::verifyInvalidResults(const Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height   = 16;
	static const GLuint width	= 16;
	static const GLuint n_pixels = height * width;

	// OpenGL ES has undefined out-of-bound behavior - no verification
	if (m_context_is_es)
		return true;

	bool result = true;

	if (R8 == m_test_case)
	{
		static const GLuint n_channels = 4;

		std::vector<GLubyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLubyte expected_red = 0;
			const GLubyte drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLuint)drawn_red
								   << ". Expected value: " << (GLuint)expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RG8_SNORM == m_test_case)
	{
		static const GLuint n_channels = 4;

		std::vector<GLbyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA, GL_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLbyte expected_red   = 0;
			const GLbyte expected_green = 0;
			const GLbyte drawn_red		= pixels[i * n_channels + 0];
			const GLbyte drawn_green	= pixels[i * n_channels + 1];

			if ((expected_red != drawn_red) || (expected_green != drawn_green))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLint)drawn_red << ", "
								   << (GLint)drawn_green << ". Expected value: " << (GLint)expected_red << ", "
								   << (GLint)expected_green << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RGBA32F == m_test_case)
	{
		static const GLuint n_channels = 4;

		std::vector<GLfloat> pixels(n_pixels * n_channels);
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint  idx   = i * n_channels;
			const GLfloat value = static_cast<GLfloat>(i) / n_pixels;
			pixels[idx + 0]		= value;
			pixels[idx + 1]		= value;
			pixels[idx + 2]		= value;
			pixels[idx + 3]		= value;
		}

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLfloat expected_red   = 0.0f;
			const GLfloat expected_green = 0.0f;
			const GLfloat expected_blue  = 0.0f;
			const GLfloat expected_alpha_0 =
				0.0f; /* OpenGL 4.5 (and ES) specifies two possiblities (0 or 1) for alpha channel (Chapter 11.1.3.12). */
			const GLfloat expected_alpha_1 = 1.0f;
			const GLfloat drawn_red		   = pixels[i * n_channels + 0];
			const GLfloat drawn_green	  = pixels[i * n_channels + 1];
			const GLfloat drawn_blue	   = pixels[i * n_channels + 2];
			const GLfloat drawn_alpha	  = pixels[i * n_channels + 3];

			const GLfloat precision = 0.0009765625; /* (1.f / 1024.f) */

			if ((de::abs(expected_red - drawn_red) > precision) ||
				(de::abs(expected_green - drawn_green) > precision) ||
				(de::abs(expected_blue - drawn_blue) > precision) ||
				((de::abs(expected_alpha_0 - drawn_alpha) > precision) &&
				 (de::abs(expected_alpha_1 - drawn_alpha) > precision)))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red << ", " << drawn_green
								   << ", " << drawn_blue << ", " << drawn_alpha << ". Expected value: " << expected_red
								   << ", " << expected_green << ", " << expected_blue << ", " << expected_alpha_0
								   << " or " << expected_alpha_1 << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MIPMAP == m_test_case)
	{
		static const GLuint n_channels = 4;

		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, texture_id, 1 /* level */, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 0;
			const GLuint drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		static const GLuint n_channels = 4;

		/* Compute shader */
		static const GLchar* source =
			"${VERSION}\n"
			"\n"
			"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
			"\n"
			"layout (location = 1)        writeonly uniform uimage2D   uni_destination_image;\n"
			"layout (location = 0, r32ui) readonly  uniform uimage2DMS uni_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    const ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);\n"
			"\n"
			"    const uvec4 color_0 = imageLoad(uni_source_image, point, 0);\n"
			"    const uvec4 color_1 = imageLoad(uni_source_image, point, 1);\n"
			"    const uvec4 color_2 = imageLoad(uni_source_image, point, 2);\n"
			"    const uvec4 color_3 = imageLoad(uni_source_image, point, 3);\n"
			"\n"
			"    if (any(equal(uvec4(color_0.r, color_1.r, color_2.r, color_3.r), uvec4(0))))\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(1, 1, 1, 1));\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(0, 0, 0, 0));\n"
			"    }\n"
			"}\n"
			"\n";

		Program program(gl);
		Texture destination_texture(gl);

		Texture::Generate(gl, destination_texture.m_id);
		Texture::Bind(gl, destination_texture.m_id, GL_TEXTURE_2D);
		Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R32UI, width, height, 0 /* depth */);

		std::string cs = tcu::StringTemplate(source).specialize(m_specializationMap);
		program.Init(cs, "", "", "", "", "");
		program.Use();
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_READ_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");
		gl.bindImageTexture(1 /* unit */, destination_texture.m_id, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		gl.uniform1i(0 /* location */, 0 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.uniform1i(1 /* location */, 1 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.dispatchCompute(16, 16, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		/* Pixels buffer initialization */
		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 1;
			const GLuint drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}

	return result;
}

/** Verifies that texutre is filled with increasing values
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with increasing values, false otherwise
 **/
bool TexelFetchTest::verifyValidResults(const Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height   = 16;
	static const GLuint width	= 16;
	static const GLuint n_pixels = height * width;

	bool result = true;

	if (R8 == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLubyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, texture_id, 0 /* level */, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLubyte expected_red = static_cast<GLubyte>(i);
			const GLubyte drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLuint)drawn_red
								   << ". Expected value: " << (GLuint)expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RG8_SNORM == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLbyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, texture_id, 0 /* level */, width, height, GL_RGBA, GL_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLbyte expected_red   = static_cast<GLubyte>((i % 16) - 8);
			const GLbyte expected_green = static_cast<GLubyte>((i / 16) - 8);
			const GLbyte drawn_red		= pixels[i * n_channels + 0];
			const GLbyte drawn_green	= pixels[i * n_channels + 1];

			if ((expected_red != drawn_red) || (expected_green != drawn_green))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLint)drawn_red << ", "
								   << (GLint)drawn_green << ". Expected value: " << (GLint)expected_red << ", "
								   << (GLint)expected_green << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RGBA32F == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLfloat> pixels(n_pixels * n_channels);
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint  idx   = i * n_channels;
			const GLfloat value = static_cast<GLfloat>(i) / n_pixels;
			pixels[idx + 0]		= value;
			pixels[idx + 1]		= value;
			pixels[idx + 2]		= value;
			pixels[idx + 3]		= value;
		}

		Texture::GetData(gl, texture_id, 0 /* level */, width, height, GL_RGBA, GL_FLOAT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLfloat expected_red   = (GLfloat)(i % 16) / 16.0f;
			const GLfloat expected_green = (GLfloat)(i / 16) / 16.0f;
			const GLfloat expected_blue  = (GLfloat)i / 256.0f;
			const GLfloat expected_alpha = 1.0f;
			const GLuint  idx			 = i * n_channels;
			const GLfloat drawn_red		 = pixels[idx + 0];
			const GLfloat drawn_green	= pixels[idx + 1];
			const GLfloat drawn_blue	 = pixels[idx + 2];
			const GLfloat drawn_alpha	= pixels[idx + 3];

			if ((expected_red != drawn_red) || (expected_green != drawn_green) || (expected_blue != drawn_blue) ||
				(expected_alpha != drawn_alpha))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red << ", " << drawn_green
								   << ", " << drawn_blue << ", " << drawn_alpha << ". Expected value: " << expected_red
								   << ", " << expected_green << ", " << expected_blue << ", " << expected_alpha
								   << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MIPMAP == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLuint> pixels(n_pixels * n_channels, 0);

		Texture::GetData(gl, texture_id, 1 /* level */, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = i;
			const GLuint drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		static const GLuint n_channels = 4;

		/* Compute shader */
		static const GLchar* source =
			"${VERSION}\n"
			"\n"
			"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
			"\n"
			"layout (location = 1, r32ui) writeonly uniform uimage2D   uni_destination_image;\n"
			"layout (location = 0, r32ui) readonly  uniform uimage2DMS uni_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    const ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);\n"
			"    const uint  index = gl_WorkGroupID.y * 16U + gl_WorkGroupID.x;\n"
			"\n"
			"    const uvec4 color_0 = imageLoad(uni_source_image, point, 0);\n"
			"    const uvec4 color_1 = imageLoad(uni_source_image, point, 1);\n"
			"    const uvec4 color_2 = imageLoad(uni_source_image, point, 2);\n"
			"    const uvec4 color_3 = imageLoad(uni_source_image, point, 3);\n"
			"\n"
			"    if (any(equal(uvec4(color_0.r, color_1.r, color_2.r, color_3.r), uvec4(index + 3U))))\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(1U));\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(0U));\n"
			"    }\n"
			"}\n"
			"\n";

		Program program(gl);
		Texture destination_texture(gl);

		Texture::Generate(gl, destination_texture.m_id);
		Texture::Bind(gl, destination_texture.m_id, GL_TEXTURE_2D);
		Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R32UI, width, height, 0 /* depth */);

		std::string cs = tcu::StringTemplate(source).specialize(m_specializationMap);
		program.Init(cs, "", "", "", "", "");
		program.Use();
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_READ_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");
		gl.bindImageTexture(1 /* unit */, destination_texture.m_id, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		if (!m_context_is_es)
		{
			gl.uniform1i(0 /* location */, 0 /* image unit*/);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

			gl.uniform1i(1 /* location */, 1 /* image unit*/);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
		}

		gl.dispatchCompute(16, 16, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		/* Pixels buffer initialization */
		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, destination_texture.m_id, 0 /* level */, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
						 &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 1;
			const GLuint drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}

	return result;
}

/** Constructor
 *
 * @param testCtx Test context
 * @param apiType Api type
 **/
ImageLoadStoreTest::ImageLoadStoreTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: TexelFetchTest(testCtx, "image_load_store", "Verifies that out-of-bound to image result in zero or is discarded",
					 apiType)
{
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ImageLoadStoreTest::iterate()
{
	de::SharedPtr<glu::RenderContext> robustContext(createRobustContext());
	if (!robustContext.get())
		return STOP;

	/* Constants */
	static const GLuint height = 16;
	static const GLuint width  = 16;

	/* GL entry points */
	const Functions& gl = robustContext->getFunctions();

	struct FetchingOffset
	{
		GLuint coord_offset;
		GLuint sample_offset;
	};
	const FetchingOffset fetching_offsets[] = {
		{ 16, 4 }, { 512, 4 }, { 1024, 8 }, { 2048, 8 },
	};

	/* For ES start from RGBA32F as R8, R32UI_MULTISAMPLE and R8_SNORM are not supported */
	if (m_context_is_es)
		m_test_case = RGBA32F;

	/* Test result indicator */
	bool test_result = true;

	/* Iterate over all cases */
	for (; m_test_case < LAST; m_test_case = (TEST_CASES)((GLuint)m_test_case + 1))
	{
		/* Test case result indicator */
		bool case_result = true;

		if (R32UI_MULTISAMPLE == m_test_case)
		{
			// Skip invalid program test in multi sample case
			// texelFetch with invalid lod plane results undefined value
			// OpenGL 4.5 Core Spec, around page 377
			continue;
		}

		/* Test case objects */
		Texture destination_texture(gl);
		Texture source_texture(gl);
		Program program(gl);

		/* Prepare textures */
		Texture::Generate(gl, destination_texture.m_id);
		Texture::Generate(gl, source_texture.m_id);

		if (R32UI_MULTISAMPLE == m_test_case)
		{
			GLint max_integer_samples;
			gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &max_integer_samples);
			GLint max_image_samples;
			gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
			if (max_integer_samples < 4 || max_image_samples < 4)
			{
				/* prepareTexture() hard-codes 4 samples (n_levels) for
				 * R32UI_MULTISAMPLE case. This value exceeds the required
				 * min-max value (1 in OpenGL ES 3.2) and is not supported
				 * by all implementations.
				 *
				 * Also, the test uses a compute shader with images
				 * to upload the texture so max_image_samples >= 4
				 * is also required.
				 */
				m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << getTestCaseName() << " not supported"
								   << tcu::TestLog::EndMessage;
				continue;
			}
		}

		prepareTexture(gl, false, destination_texture.m_id);
		prepareTexture(gl, true, source_texture.m_id);

		/* Test invalid source cases */
		for (GLuint i = 0; i < DE_LENGTH_OF_ARRAY(fetching_offsets); ++i)
		{
			const FetchingOffset& fo = fetching_offsets[i];
			const std::string&	cs = getComputeShader(SOURCE_INVALID, fo.coord_offset, fo.sample_offset);
			program.Init(cs, "", "", "", "", "");
			program.Use();

			/* Set texture */
			setTextures(gl, destination_texture.m_id, source_texture.m_id);

			/* Dispatch */
			gl.dispatchCompute(width, height, 1 /* depth */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

			/* Verification */
			if (false == verifyInvalidResults(gl, destination_texture.m_id))
			{
				case_result = false;
			}
		}

		/* Test valid case */
		program.Init(getComputeShader(VALID), "", "", "", "", "");
		program.Use();

		/* Set texture */
		setTextures(gl, destination_texture.m_id, source_texture.m_id);

		/* Dispatch */
		gl.dispatchCompute(width, height, 1 /* depth */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		/* Verification */
		if (false == verifyValidResults(gl, destination_texture.m_id))
		{
			case_result = false;
		}

		/* Test invalid destination cases */
		for (GLuint i = 0; i < DE_LENGTH_OF_ARRAY(fetching_offsets); ++i)
		{
			const FetchingOffset& fo = fetching_offsets[i];
			const std::string&	cs = getComputeShader(DESTINATION_INVALID, fo.coord_offset, fo.sample_offset);
			program.Init(cs, "", "", "", "", "");
			program.Use();

			/* Set texture */
			setTextures(gl, destination_texture.m_id, source_texture.m_id);

			/* Dispatch */
			gl.dispatchCompute(width, height, 1 /* depth */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

			/* Verification */
			if (false == verifyValidResults(gl, destination_texture.m_id))
			{
				case_result = false;
			}
		}

		/* Set test result */
		if (false == case_result)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << getTestCaseName() << " failed"
							   << tcu::TestLog::EndMessage;

			test_result = false;
		}
	}

	/* Set result */
	if (true == test_result)
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

/** Prepare shader for current test case
 *
 * @param version Specify which version should be prepared
 *
 * @return Source
 **/
std::string ImageLoadStoreTest::getComputeShader(VERSION version, GLuint coord_offset, GLuint sample_offset)
{
	static const GLchar* source =
		"${VERSION}\n"
		"\n"
		"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"layout (${QUALIFIER} = 1, ${FORMAT}) writeonly uniform highp ${IMAGE} uni_destination_image;\n"
		"layout (${QUALIFIER} = 0, ${FORMAT}) readonly  uniform highp ${IMAGE} uni_source_image;\n"
		"\n"
		"void main()\n"
		"{\n"
		"  ivec2 point_destination = ivec2(gl_WorkGroupID.xy) + ivec2(${DST_COORD_OFFSET}U);\n"
		"  ivec2 point_source      = ivec2(gl_WorkGroupID.xy) + ivec2(${SRC_COORD_OFFSET}U);\n"
		"\n"
		"${COPY}"
		"}\n";

	static const GLchar* copy_multisampled =
		"  ${TYPE} color_0 = imageLoad(uni_source_image, point_source, 0 + ${SRC_SAMPLE_OFFSET});\n"
		"  ${TYPE} color_1 = imageLoad(uni_source_image, point_source, 1 + ${SRC_SAMPLE_OFFSET});\n"
		"  ${TYPE} color_2 = imageLoad(uni_source_image, point_source, 2 + ${SRC_SAMPLE_OFFSET});\n"
		"  ${TYPE} color_3 = imageLoad(uni_source_image, point_source, 3 + ${SRC_SAMPLE_OFFSET});\n"
		"  imageStore(uni_destination_image, point_destination, 0 + ${DST_SAMPLE_OFFSET}, color_0);\n"
		"  imageStore(uni_destination_image, point_destination, 1 + ${DST_SAMPLE_OFFSET}, color_1);\n"
		"  imageStore(uni_destination_image, point_destination, 2 + ${DST_SAMPLE_OFFSET}, color_2);\n"
		"  imageStore(uni_destination_image, point_destination, 3 + ${DST_SAMPLE_OFFSET}, color_3);\n";

	static const GLchar* copy_regular = "  ${TYPE} color = imageLoad(uni_source_image, point_source);\n"
										"  imageStore(uni_destination_image, point_destination, color);\n";

	std::string src_coord_offset_str("0");
	std::string dst_coord_offset_str("0");
	std::string src_sample_offset_str("0");
	std::string dst_sample_offset_str("0");

	std::stringstream coord_offset_stream;
	coord_offset_stream << coord_offset;
	std::stringstream sample_offset_stream;
	sample_offset_stream << sample_offset;

	m_specializationMap["QUALIFIER"] = m_context_is_es ? "binding" : "location";
	m_specializationMap["IMAGE"]	 = "image2D";
	m_specializationMap["TYPE"]		 = "vec4";
	switch (m_test_case)
	{
	case R8:
		m_specializationMap["FORMAT"] = "r8";
		break;
	case RG8_SNORM:
		m_specializationMap["FORMAT"] = "rg8_snorm";
		break;
	case RGBA32F:
		m_specializationMap["FORMAT"] = "rgba32f";
		break;
	case R32UI_MIPMAP:
		m_specializationMap["FORMAT"] = "r32ui";
		m_specializationMap["IMAGE"]  = "uimage2D";
		m_specializationMap["TYPE"]   = "uvec4";
		break;
	case R32UI_MULTISAMPLE:
		m_specializationMap["FORMAT"] = "r32ui";
		m_specializationMap["IMAGE"]  = "uimage2DMS";
		m_specializationMap["TYPE"]   = "uvec4";
		break;
	default:
		TCU_FAIL("Invalid enum");
	};

	m_specializationMap["SRC_COORD_OFFSET"]  = "0";
	m_specializationMap["SRC_SAMPLE_OFFSET"] = "0";
	m_specializationMap["DST_COORD_OFFSET"]  = "0";
	m_specializationMap["DST_SAMPLE_OFFSET"] = "0";

	if (version == SOURCE_INVALID)
	{
		m_specializationMap["SRC_COORD_OFFSET"]  = coord_offset_stream.str();
		m_specializationMap["SRC_SAMPLE_OFFSET"] = sample_offset_stream.str();
	}
	else if (version == DESTINATION_INVALID)
	{
		m_specializationMap["DST_COORD_OFFSET"]  = coord_offset_stream.str();
		m_specializationMap["DST_SAMPLE_OFFSET"] = sample_offset_stream.str();
	}

	const GLchar* copy			= (m_test_case == R32UI_MULTISAMPLE) ? copy_multisampled : copy_regular;
	m_specializationMap["COPY"] = tcu::StringTemplate(copy).specialize(m_specializationMap);

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Set textures as images
 *
 * @param id_destination Id of texture used as destination
 * @param id_source      Id of texture used as source
 **/
void ImageLoadStoreTest::setTextures(const Functions& gl, glw::GLuint id_destination, glw::GLuint id_source)
{
	GLenum format = 0;
	GLint  level  = 0;

	switch (m_test_case)
	{
	case R8:
		format = GL_R8;
		break;
	case RG8_SNORM:
		format = GL_RG8_SNORM;
		break;
	case RGBA32F:
		format = GL_RGBA32F;
		break;
	case R32UI_MIPMAP:
		format = GL_R32UI;
		level  = 1;
		break;
	case R32UI_MULTISAMPLE:
		format = GL_R32UI;
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	gl.bindImageTexture(0 /* unit */, id_source, level, GL_FALSE /* layered */, 0 /* layer */, GL_READ_ONLY, format);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	gl.bindImageTexture(1 /* unit */, id_destination, level, GL_FALSE /* layered */, 0 /* layer */, GL_WRITE_ONLY,
						format);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	if (!m_context_is_es)
	{
		gl.uniform1i(0 /* location */, 0 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.uniform1i(1 /* location */, 1 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
	}
}

/** Verifies that texutre is filled with 0
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with 0, false otherwise
 **/
bool ImageLoadStoreTest::verifyInvalidResults(const Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height   = 16;
	static const GLuint width	= 16;
	static const GLuint n_pixels = height * width;

	// OpenGL ES has undefined out-of-bound behavior - no verification
	if (m_context_is_es)
		return true;

	gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	bool result = true;

	if (R8 == m_test_case)
	{
		static const GLuint n_channels = 1;

		std::vector<GLubyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLubyte expected_red = 0;
			const GLubyte drawn_red	= pixels[i];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLuint)drawn_red
								   << ". Expected value: " << (GLuint)expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RG8_SNORM == m_test_case)
	{
		static const GLuint n_channels = 2;

		std::vector<GLbyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RG, GL_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLbyte expected_red   = 0;
			const GLbyte expected_green = 0;
			const GLbyte drawn_red		= pixels[i * n_channels + 0];
			const GLbyte drawn_green	= pixels[i * n_channels + 1];

			if ((expected_red != drawn_red) || (expected_green != drawn_green))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLint)drawn_red << ", "
								   << (GLint)drawn_green << ". Expected value: " << (GLint)expected_red << ", "
								   << (GLint)expected_green << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RGBA32F == m_test_case)
	{
		static const GLuint n_channels = 4;

		std::vector<GLfloat> pixels(n_pixels * n_channels);
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			GLuint  idx		= i * n_channels;
			GLfloat value   = static_cast<GLfloat>(i) / n_pixels;
			pixels[idx + 0] = value;
			pixels[idx + 1] = value;
			pixels[idx + 2] = value;
			pixels[idx + 3] = value;
		}

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLfloat expected_red   = 0.0f;
			const GLfloat expected_green = 0.0f;
			const GLfloat expected_blue  = 0.0f;
			const GLfloat expected_alpha = 0.0f;
			const GLuint  idx			 = i * n_channels;
			const GLfloat drawn_red		 = pixels[idx + 0];
			const GLfloat drawn_green	= pixels[idx + 1];
			const GLfloat drawn_blue	 = pixels[idx + 2];
			const GLfloat drawn_alpha	= pixels[idx + 3];

			if ((expected_red != drawn_red) || (expected_green != drawn_green) || (expected_blue != drawn_blue) ||
				(expected_alpha != drawn_alpha))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red << ", " << drawn_green
								   << ", " << drawn_blue << ", " << drawn_alpha << ". Expected value: " << expected_red
								   << ", " << expected_green << ", " << expected_blue << ", " << expected_alpha
								   << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MIPMAP == m_test_case)
	{
		static const GLuint n_channels = 1;

		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		Texture::GetData(gl, 1 /* level */, GL_TEXTURE_2D, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 0;
			const GLuint drawn_red	= pixels[i];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		static const GLuint n_channels = 1;

		/* Compute shader */
		static const GLchar* cs = "${VERSION}\n"
								  "\n"
								  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
								  "\n"
								  "layout (location = 1)        writeonly uniform uimage2D   uni_destination_image;\n"
								  "layout (location = 0, r32ui) readonly  uniform uimage2DMS uni_source_image;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    const ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);\n"
								  "\n"
								  "    const uvec4 color_0 = imageLoad(uni_source_image, point, 0);\n"
								  "    const uvec4 color_1 = imageLoad(uni_source_image, point, 1);\n"
								  "    const uvec4 color_2 = imageLoad(uni_source_image, point, 2);\n"
								  "    const uvec4 color_3 = imageLoad(uni_source_image, point, 3);\n"
								  "\n"
								  "    if (any(equal(uvec4(color_0.r, color_1.r, color_2.r, color_3.r), uvec4(0))))\n"
								  "    {\n"
								  "        imageStore(uni_destination_image, point, uvec4(1, 1, 1, 1));\n"
								  "    }\n"
								  "    else\n"
								  "    {\n"
								  "        imageStore(uni_destination_image, point, uvec4(0, 0, 0, 0));\n"
								  "    }\n"
								  "}\n"
								  "\n";

		Program program(gl);
		Texture destination_texture(gl);

		Texture::Generate(gl, destination_texture.m_id);
		Texture::Bind(gl, destination_texture.m_id, GL_TEXTURE_2D);
		Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R32UI, width, height, 0 /* depth */);

		program.Init(cs, "", "", "", "", "");
		program.Use();
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_READ_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");
		gl.bindImageTexture(1 /* unit */, destination_texture.m_id, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		gl.uniform1i(0 /* location */, 0 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.uniform1i(1 /* location */, 1 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.dispatchCompute(16, 16, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		/* Pixels buffer initialization */
		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 1;
			const GLuint drawn_red	= pixels[i];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}

	return result;
}

/** Verifies that texutre is filled with increasing values
 *
 * @param texture_id Id of texture
 *
 * @return true when image is filled with increasing values, false otherwise
 **/
bool ImageLoadStoreTest::verifyValidResults(const glw::Functions& gl, glw::GLuint texture_id)
{
	static const GLuint height   = 16;
	static const GLuint width	= 16;
	static const GLuint n_pixels = height * width;

	gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	bool result = true;

	if (R8 == m_test_case)
	{
		static const GLuint n_channels = 1;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLubyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLubyte expected_red = static_cast<GLubyte>(i);
			const GLubyte drawn_red	= pixels[i];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLuint)drawn_red
								   << ". Expected value: " << (GLuint)expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RG8_SNORM == m_test_case)
	{
		static const GLuint n_channels = 2;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLbyte> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RG, GL_BYTE, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLbyte expected_red   = static_cast<GLubyte>((i % 16) - 8);
			const GLbyte expected_green = static_cast<GLubyte>((i / 16) - 8);
			const GLbyte drawn_red		= pixels[i * n_channels + 0];
			const GLbyte drawn_green	= pixels[i * n_channels + 1];

			if ((expected_red != drawn_red) || (expected_green != drawn_green))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << (GLint)drawn_red << ", "
								   << (GLint)drawn_green << ". Expected value: " << (GLint)expected_red << ", "
								   << (GLint)expected_green << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (RGBA32F == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLfloat> pixels(n_pixels * n_channels);
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			GLfloat value			   = static_cast<GLfloat>(i) / n_pixels;
			pixels[i * n_channels + 0] = value;
			pixels[i * n_channels + 1] = value;
			pixels[i * n_channels + 2] = value;
			pixels[i * n_channels + 3] = value;
		}

		Texture::GetData(gl, texture_id, 0 /* level */, width, height, GL_RGBA, GL_FLOAT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLfloat expected_red   = (GLfloat)(i % 16) / 16.0f;
			const GLfloat expected_green = (GLfloat)(i / 16) / 16.0f;
			const GLfloat expected_blue  = (GLfloat)i / 256.0f;
			const GLfloat expected_alpha = 1.0f;
			const GLuint  idx			 = i * n_channels;
			const GLfloat drawn_red		 = pixels[idx + 0];
			const GLfloat drawn_green	= pixels[idx + 1];
			const GLfloat drawn_blue	 = pixels[idx + 2];
			const GLfloat drawn_alpha	= pixels[idx + 3];

			if ((expected_red != drawn_red) || (expected_green != drawn_green) || (expected_blue != drawn_blue) ||
				(expected_alpha != drawn_alpha))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red << ", " << drawn_green
								   << ", " << drawn_blue << ", " << drawn_alpha << ". Expected value: " << expected_red
								   << ", " << expected_green << ", " << expected_blue << ", " << expected_alpha
								   << ". At offset: " << i << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MIPMAP == m_test_case)
	{
		static const GLuint n_channels = 4;

		Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, texture_id, 1 /* level */, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = i;
			const GLuint drawn_red	= pixels[i * n_channels];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}
	else if (R32UI_MULTISAMPLE == m_test_case)
	{
		static const GLuint n_channels = 1;

		/* Compute shader */
		static const GLchar* cs =
			"${VERSION}\n"
			"\n"
			"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
			"\n"
			"layout (location = 1)        writeonly uniform uimage2D   uni_destination_image;\n"
			"layout (location = 0, r32ui) readonly  uniform uimage2DMS uni_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    const ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y);\n"
			"    const uint  index = gl_WorkGroupID.y * 16 + gl_WorkGroupID.x;\n"
			"\n"
			"    const uvec4 color_0 = imageLoad(uni_source_image, point, 0);\n"
			"    const uvec4 color_1 = imageLoad(uni_source_image, point, 1);\n"
			"    const uvec4 color_2 = imageLoad(uni_source_image, point, 2);\n"
			"    const uvec4 color_3 = imageLoad(uni_source_image, point, 3);\n"
			"\n"
			"    if (any(equal(uvec4(color_0.r, color_1.r, color_2.r, color_3.r), uvec4(index + 3))))\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(1, 1, 1, 1));\n"
			"    }\n"
			"    else\n"
			"    {\n"
			"        imageStore(uni_destination_image, point, uvec4(0, 0, 0, 0));\n"
			"    }\n"
			"}\n"
			"\n";

		Program program(gl);
		Texture destination_texture(gl);

		Texture::Generate(gl, destination_texture.m_id);
		Texture::Bind(gl, destination_texture.m_id, GL_TEXTURE_2D);
		Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R32UI, width, height, 0 /* depth */);

		program.Init(cs, "", "", "", "", "");
		program.Use();
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_READ_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");
		gl.bindImageTexture(1 /* unit */, destination_texture.m_id, 0 /* level */, GL_FALSE /* layered */,
							0 /* layer */, GL_WRITE_ONLY, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

		gl.uniform1i(0 /* location */, 0 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.uniform1i(1 /* location */, 1 /* image unit*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

		gl.dispatchCompute(16, 16, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		/* Pixels buffer initialization */
		std::vector<GLuint> pixels(n_pixels * n_channels);
		initPixels(pixels, n_pixels, n_channels);

		Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixels[0]);

		/* Unbind */
		Texture::Bind(gl, 0, GL_TEXTURE_2D);

		/* Verify */
		for (GLuint i = 0; i < n_pixels; ++i)
		{
			const GLuint expected_red = 1;
			const GLuint drawn_red	= pixels[i];

			if (expected_red != drawn_red)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value: " << drawn_red
								   << ". Expected value: " << expected_red << " at offset: " << i
								   << tcu::TestLog::EndMessage;

				result = false;
				break;
			}
		}
	}

	return result;
}

/* StorageBufferTest constants */
const GLfloat StorageBufferTest::m_destination_data[4]	= { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat StorageBufferTest::m_source_data[4]		= { 2.0f, 3.0f, 4.0f, 5.0f };

/** Constructor
 *
 * @param testCtx Test context
 * @param apiType Api type
 **/
StorageBufferTest::StorageBufferTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: RobustnessBase(testCtx, "storage_buffer", "Verifies that out-of-bound access to SSBO is discared or resutls in 0",
					 apiType)
	, m_test_case(VALID)
{
	/* Nothing to be done here */
}


/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult StorageBufferTest::iterate()
{
	de::SharedPtr<glu::RenderContext> robustContext(createRobustContext());
	if (!robustContext.get())
		return STOP;

	/* GL entry points */
	const Functions& gl = robustContext->getFunctions();

	/* Test result indicator */
	bool test_result = true;

	GLuint test_offsets[] = {
		16,				 // close fetch
		4 * 1024,		 // near fetch (4K of the end of the object)
		1024 * 1024,	 // medium fetch (1MB past the end of the object)
		10 * 1024 * 1024 // high fetch (10MB beyond the end of the object)
	};

	/* Iterate over all cases */
	while (LAST != m_test_case)
	{
		/* Test case objects */
		Buffer  destination_buffer(gl);
		Buffer  source_buffer(gl);
		Program program(gl);

		/* Buffers initialization */
		destination_buffer.InitData(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(m_destination_data),
									m_destination_data);
		source_buffer.InitData(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(m_source_data), m_source_data);

		destination_buffer.BindBase(0);
		source_buffer.BindBase(1);

		for (GLuint i = 0; i < DE_LENGTH_OF_ARRAY(test_offsets); ++i)
		{
			/* Initialize shader */
			const std::string& cs = getComputeShader(test_offsets[i]);
			program.Init(cs, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);
			program.Use();

			/* Dispatch compute */
			gl.dispatchCompute(1 /* x */, 1 /* y */, 1 /* z */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

			/* Set memory barrier */
			gl.memoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

			/* Verify results */
			destination_buffer.Bind();
			GLfloat* buffer_data =
				(GLfloat*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(m_destination_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MapBufferRange");

			test_result &= verifyResults(buffer_data);

			gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
		}

		/* Increment */
		m_test_case = (VERSION)((GLuint)m_test_case + 1);
	}

	/* Set result */
	if (true == test_result)
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

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string StorageBufferTest::getComputeShader(GLuint offset)
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "layout (local_size_x = 4, local_size_y = 1, local_size_z = 1) in;\n"
								  "\n"
								  "layout (binding = 1, std430) buffer Source {\n"
								  "    float data[];\n"
								  "} source;\n"
								  "\n"
								  "layout (binding = 0, std430) buffer Destination {\n"
								  "    float data[];\n"
								  "} destination;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    uint index_destination = gl_LocalInvocationID.x + ${DST_OFFSET}U;\n"
								  "    uint index_source      = gl_LocalInvocationID.x + ${SRC_OFFSET}U;\n"
								  "\n"
								  "    destination.data[index_destination] = source.data[index_source];\n"
								  "}\n"
								  "\n";

	std::stringstream offset_stream;
	offset_stream << offset;

	m_specializationMap["DST_OFFSET"] = "0";
	m_specializationMap["SRC_OFFSET"] = "0";
	if (m_test_case == SOURCE_INVALID)
		m_specializationMap["SRC_OFFSET"] = offset_stream.str();
	else if (m_test_case == DESTINATION_INVALID)
		m_specializationMap["DST_OFFSET"] = offset_stream.str();

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Verify test case results
 *
 * @param buffer_data Buffer data to verify
 *
 * @return true if buffer_data is as expected, false othrewise
 **/
bool StorageBufferTest::verifyResults(GLfloat* buffer_data)
{
	/* KHR_robust_buffer_access_behavior (and also GL 4.5 and later) states
	 * which values can be expected when reading or writing outside of a
	 * buffer's range. If supported, we will compare results against those
	 * expectations.
	 *
	 * Otherwise, we will attempt to match results against previously observed
	 * and valid behavior.
	 */
	static const GLfloat expected_data_valid[4]				   = { 2.0f, 3.0f, 4.0f, 5.0f };
	static const GLfloat expected_data_invalid_source[4]	   = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const GLfloat expected_data_invalid_destination[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };

	/* OpenGL ES has undefined out-of-bound behavior - verify only valid result*/
	if (m_context_is_es && (m_test_case != VALID))
		return true;

	/* Prepare expected data const for proper case*/
	const GLchar*  name				   = 0;
	bool		   check_expected_data = false;
	const GLfloat* expected_data	   = 0;
	switch (m_test_case)
	{
	case VALID:
		name				= "valid indices";
		check_expected_data	= true;
		expected_data		= expected_data_valid;
		break;
	case SOURCE_INVALID:
		name				= "invalid source indices";
		if (m_has_khr_robust_buffer_access)
		{
			for (int b = 0; b < 4; b++)
			{
				/* Each out-of-range read can either be 0 or any value within
				 * the source buffer.
				 * */
				bool valid = false;
				if (buffer_data[b] == 0.0f)
				{
					valid = true;
				}
				else
				{
					for (int c = 0; c < 4 && !valid; c++)
					{
						if (buffer_data[b] == m_source_data[c])
						{
							valid = true;
						}
					}
				}
				if (!valid)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << name << " failed"
									   << tcu::TestLog::EndMessage;
				}
			}
		}
		else
		{
			check_expected_data	= true;
			expected_data		= expected_data_invalid_source;
		}
		break;
	case DESTINATION_INVALID:
		name				= "invalid destination indices";
		if (m_has_khr_robust_buffer_access)
		{
			for (int b = 0; b < 4; b++)
			{
				bool valid = false;
				/* Each out-of-range write can either be discarded (in which
				 * case it would have the original destination value) or it
				 * could write any value within the buffer (so we need to check
				 * against each possible source value).
				 */
				if (buffer_data[b] == m_destination_data[b])
				{
					valid = true;
				}
				else
				{
					for (int c = 0; c < 4 && !valid; c++)
					{
						if (buffer_data[b] == m_source_data[c])
						{
							valid = true;
						}
					}
				}
				if (!valid)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << name << " failed"
									   << tcu::TestLog::EndMessage;
				}
			}
		}
		else
		{
			check_expected_data	= true;
			expected_data		= expected_data_invalid_destination;
		}
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	if (check_expected_data)
	{
		/* Verify buffer data */
		int size = static_cast<int>(sizeof(GLfloat) * 4);
		if (0 != memcmp(expected_data, buffer_data, size))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << name << " failed"
							   << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
UniformBufferTest::UniformBufferTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: RobustnessBase(testCtx, "uniform_buffer", "Verifies that out-of-bound access to UBO resutls in 0", apiType)
	, m_test_case(VALID)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult UniformBufferTest::iterate()
{
	de::SharedPtr<glu::RenderContext> robustContext(createRobustContext());
	if (!robustContext.get())
		return STOP;

	static const GLfloat destination_data[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	/* The source buffer is packed std140 so we need vec4s */
	static const GLfloat source_data[16] = {
		2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f,
	};

	GLuint test_offsets[] = {
		16,				 // close fetch
		4 * 1024,		 // near fetch (4K of the end of the object)
		1024 * 1024,	 // medium fetch (1MB past the end of the object)
		10 * 1024 * 1024 // high fetch (10MB beyond the end of the object)
	};

	/* GL entry points */
	const Functions& gl = robustContext->getFunctions();

	/* Test result indicator */
	bool test_result = true;

	/* Iterate over all cases */
	while (LAST != m_test_case)
	{
		/* Test case objects */
		Buffer  destination_buffer(gl);
		Buffer  source_buffer(gl);
		Program program(gl);

		/* Buffers initialization */
		destination_buffer.InitData(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(destination_data),
									destination_data);
		source_buffer.InitData(GL_UNIFORM_BUFFER, GL_DYNAMIC_COPY, sizeof(source_data), source_data);

		destination_buffer.BindBase(0);
		source_buffer.BindBase(0);

		for (GLuint i = 0; i < DE_LENGTH_OF_ARRAY(test_offsets); ++i)
		{
			/* Initialize shader */
			const std::string& cs = getComputeShader(test_offsets[i]);
			program.Init(cs, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);
			program.Use();

			/* Dispatch compute */
			gl.dispatchCompute(1 /* x */, 1 /* y */, 1 /* z */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

			/* Set memory barrier */
			gl.memoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

			/* Verify results */
			destination_buffer.Bind();
			GLfloat* buffer_data =
				(GLfloat*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(destination_data), GL_MAP_READ_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MapBufferRange");

			test_result &= verifyResults(buffer_data);

			gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
		}

		/* Increment */
		m_test_case = (VERSION)((GLuint)m_test_case + 1);
	}

	/* Set result */
	if (true == test_result)
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

/** Prepare shader for current test case
 *
 * @return Source
 **/
std::string UniformBufferTest::getComputeShader(GLuint offset)
{
	static const GLchar* source = "${VERSION}\n"
								  "\n"
								  "layout (local_size_x = 4, local_size_y = 1, local_size_z = 1) in;\n"
								  "\n"
								  "layout (binding = 0, std140) uniform Source {\n"
								  "    float data[16];\n"
								  "} source;\n"
								  "\n"
								  "layout (binding = 0, std430) buffer Destination {\n"
								  "    float data[];\n"
								  "} destination;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    uint index_destination = gl_LocalInvocationID.x;\n"
								  "    uint index_source      = gl_LocalInvocationID.x + ${OFFSET}U;\n"
								  "\n"
								  "    destination.data[index_destination] = source.data[index_source];\n"
								  "}\n"
								  "\n";

	m_specializationMap["OFFSET"] = "0";
	if (m_test_case == SOURCE_INVALID)
	{
		std::stringstream offset_stream;
		offset_stream << offset;
		m_specializationMap["OFFSET"] = offset_stream.str();
	}

	return tcu::StringTemplate(source).specialize(m_specializationMap);
}

/** Verify test case results
 *
 * @param buffer_data Buffer data to verify
 *
 * @return true if buffer_data is as expected, false othrewise
 **/
bool UniformBufferTest::verifyResults(GLfloat* buffer_data)
{
	static const GLfloat expected_data_valid[4]			 = { 2.0f, 3.0f, 4.0f, 5.0f };
	static const GLfloat expected_data_invalid_source[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	int size = static_cast<int>(sizeof(GLfloat) * 4);

	/* Prepare expected data const for proper case*/
	const GLfloat* expected_data = 0;
	const GLchar*  name			 = 0;
	switch (m_test_case)
	{
	case VALID:
		expected_data = expected_data_valid;
		name		  = "valid indices";
		break;
	case SOURCE_INVALID:
		expected_data = expected_data_invalid_source;
		name		  = "invalid source indices";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	/* Verify buffer data */
	if (0 != memcmp(expected_data, buffer_data, size))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test case: " << name << " failed" << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}
} /* RobustBufferAccessBehavior */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
RobustBufferAccessBehaviorTests::RobustBufferAccessBehaviorTests(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCaseGroup(testCtx, "robust_buffer_access_behavior",
						 "Verifies \"robust buffer access behavior\" functionality")
	, m_ApiType(apiType)
{
	/* Left blank on purpose */
}

/** Initializes a multi_bind test group.
 *
 **/
void RobustBufferAccessBehaviorTests::init(void)
{
	addChild(new RobustBufferAccessBehavior::VertexBufferObjectsTest(m_testCtx, m_ApiType));
	addChild(new RobustBufferAccessBehavior::TexelFetchTest(m_testCtx, m_ApiType));
	addChild(new RobustBufferAccessBehavior::ImageLoadStoreTest(m_testCtx, m_ApiType));
	addChild(new RobustBufferAccessBehavior::StorageBufferTest(m_testCtx, m_ApiType));
	addChild(new RobustBufferAccessBehavior::UniformBufferTest(m_testCtx, m_ApiType));
}

} /* glcts */
