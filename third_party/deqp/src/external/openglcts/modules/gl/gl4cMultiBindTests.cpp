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
 * \file  gl4cMultiBindTests.cpp
 * \brief Implements conformance tests for "Multi Bind" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cMultiBindTests.hpp"

#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <string>

#define DEBUG_ENBALE_MESSAGE_CALLBACK 0

#if DEBUG_ENBALE_MESSAGE_CALLBACK
#include <iomanip>
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

using namespace glw;

namespace gl4cts
{
namespace MultiBind
{

#if DEBUG_ENBALE_MESSAGE_CALLBACK
/** Debuging procedure. Logs parameters.
 *
 * @param source   As specified in GL spec.
 * @param type     As specified in GL spec.
 * @param id       As specified in GL spec.
 * @param severity As specified in GL spec.
 * @param ignored
 * @param message  As specified in GL spec.
 * @param info     Pointer to instance of deqp::Context used by test.
 */
void GLW_APIENTRY debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /* length */,
							 const GLchar* message, void* info)
{
	deqp::Context* ctx = (deqp::Context*)info;

	const GLchar* source_str   = "Unknown";
	const GLchar* type_str	 = "Unknown";
	const GLchar* severity_str = "Unknown";

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		source_str = "API";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		source_str = "APP";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		source_str = "OTR";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		source_str = "COM";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		source_str = "3RD";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		source_str = "WS";
		break;
	default:
		break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		type_str = "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_ERROR:
		type_str = "ERROR";
		break;
	case GL_DEBUG_TYPE_MARKER:
		type_str = "MARKER";
		break;
	case GL_DEBUG_TYPE_OTHER:
		type_str = "OTHER";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		type_str = "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		type_str = "POP_GROUP";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		type_str = "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		type_str = "PUSH_GROUP";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		type_str = "UNDEFINED_BEHAVIOR";
		break;
	default:
		break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		severity_str = "H";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		severity_str = "L";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		severity_str = "M";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		severity_str = "N";
		break;
	default:
		break;
	}

	ctx->getTestContext().getLog() << tcu::TestLog::Message << "DEBUG_INFO: " << std::setw(3) << source_str << "|"
								   << severity_str << "|" << std::setw(18) << type_str << "|" << std::setw(12) << id
								   << ": " << message << tcu::TestLog::EndMessage;
}

#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

/** Represents buffer instance
 * Provides basic buffer functionality
 **/
class Buffer
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Buffer();
	~Buffer();

	/* Init & Release */
	void Init(deqp::Context& context);

	void InitData(deqp::Context& context, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
				  const glw::GLvoid* data);

	void Release();

	/* Functionality */
	void Bind() const;
	void BindBase(glw::GLuint index) const;

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);

	static void BindBase(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index);

	static void Data(const glw::Functions& gl, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
					 const glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void SubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
						glw::GLvoid* data);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private enums */

	/* Private fields */
	deqp::Context* m_context;
	glw::GLenum	m_target;
};

/** Represents framebuffer
 * Provides basic functionality
 **/
class Framebuffer
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Framebuffer(deqp::Context& context);
	~Framebuffer();

	/* Init & Release */
	void Release();

	/* Public static routines */
	static void AttachTexture(const glw::Functions& gl, glw::GLenum target, glw::GLenum attachment,
							  glw::GLuint texture_id, glw::GLint level, glw::GLuint width, glw::GLuint height);

	static void Bind(const glw::Functions& gl, glw::GLenum target, glw::GLuint id);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents shader instance.
 * Provides basic functionality for shaders.
 **/
class Shader
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Shader(deqp::Context& context);
	~Shader();

	/* Init & Realese */
	void Init(glw::GLenum stage, const std::string& source);
	void Release();

	/* Public static routines */
	/* Functionality */
	static void Compile(const glw::Functions& gl, glw::GLuint id);

	static void Create(const glw::Functions& gl, glw::GLenum stage, glw::GLuint& out_id);

	static void Source(const glw::Functions& gl, glw::GLuint id, const std::string& source);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents program instance.
 * Provides basic functionality
 **/
class Program
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Program(deqp::Context& context);
	~Program();

	/* Init & Release */
	void Init(const std::string& compute_shader, const std::string& fragment_shader, const std::string& geometry_shader,
			  const std::string& tesselation_control_shader, const std::string& tesselation_evaluation_shader,
			  const std::string& vertex_shader);
	void Release();

	/* Functionality */
	void Use() const;

	/* Public static routines */
	/* Functionality */
	static void Attach(const glw::Functions& gl, glw::GLuint program_id, glw::GLuint shader_id);

	static void Create(const glw::Functions& gl, glw::GLuint& out_id);

	static void Link(const glw::Functions& gl, glw::GLuint id);

	static void Use(const glw::Functions& gl, glw::GLuint id);

	/* Public fields */
	glw::GLuint m_id;

	Shader m_compute;
	Shader m_fragment;
	Shader m_geometry;
	Shader m_tess_ctrl;
	Shader m_tess_eval;
	Shader m_vertex;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/** Represents texture instance
 **/
class Texture
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	Texture();
	~Texture();

	/* Init & Release */
	void Init(deqp::Context& context);

	void InitBuffer(deqp::Context& context, glw::GLenum internal_format, glw::GLuint buffer_id);

	void InitStorage(deqp::Context& context, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
					 glw::GLuint width, glw::GLuint height, glw::GLuint depth, bool allow_error = false);

	void Release();

	/* Public static routines */
	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);

	static void CompressedImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level,
								glw::GLenum internal_format, glw::GLuint width, glw::GLuint height, glw::GLuint depth,
								glw::GLsizei image_size, const glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void GetData(const glw::Functions& gl, glw::GLint level, glw::GLenum target, glw::GLenum format,
						glw::GLenum type, glw::GLvoid* out_data);

	static void GetLevelParameter(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum pname,
								  glw::GLint* param);

	static void Image(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum internal_format,
					  glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					  const glw::GLvoid* data);

	static void Storage(const glw::Functions& gl, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
						glw::GLuint width, glw::GLuint height, glw::GLuint depth, bool allow_error);

	static void SubImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLint x, glw::GLint y,
						 glw::GLint z, glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
						 glw::GLenum type, const glw::GLvoid* pixels);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context* m_context;
};

/* Buffer constants */
const GLuint Buffer::m_invalid_id = -1;

/** Constructor.
 *
 **/
Buffer::Buffer() : m_id(m_invalid_id), m_context(0), m_target(GL_ARRAY_BUFFER)
{
}

/** Destructor
 *
 **/
Buffer::~Buffer()
{
	Release();

	m_context = 0;
}

/** Initialize buffer instance
 *
 * @param context CTS context.
 **/
void Buffer::Init(deqp::Context& context)
{
	Release();

	m_context = &context;
}

/** Initialize buffer instance with some data
 *
 * @param context CTS context.
 * @param target Buffer target
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::InitData(deqp::Context& context, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
					  const glw::GLvoid* data)
{
	Init(context);

	m_target = target;

	const Functions& gl = m_context->getRenderContext().getFunctions();

	Generate(gl, m_id);
	Bind(gl, m_id, m_target);
	Data(gl, m_target, usage, size, data);
}

/** Release buffer instance
 *
 **/
void Buffer::Release()
{
	if (m_invalid_id != m_id)
	{
		const Functions& gl = m_context->getRenderContext().getFunctions();

		gl.deleteBuffers(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Binds buffer to its target
 *
 **/
void Buffer::Bind() const
{
	if (m_invalid_id == m_id)
	{
		return;
	}

	const Functions& gl = m_context->getRenderContext().getFunctions();

	Bind(gl, m_id, m_target);
}

/** Binds indexed buffer
 *
 * @param index <index> parameter
 **/
void Buffer::BindBase(glw::GLuint index) const
{
	if (m_invalid_id == m_id)
	{
		return;
	}

	const Functions& gl = m_context->getRenderContext().getFunctions();

	BindBase(gl, m_id, m_target, index);
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
Framebuffer::Framebuffer(deqp::Context& context) : m_id(m_invalid_id), m_context(context)
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
		const Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteFramebuffers(1, &m_id);
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
Program::Program(deqp::Context& context)
	: m_id(m_invalid_id)
	, m_compute(context)
	, m_fragment(context)
	, m_geometry(context)
	, m_tess_ctrl(context)
	, m_tess_eval(context)
	, m_vertex(context)
	, m_context(context)
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

	/* GL entry points */
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize shaders */
	m_compute.Init(GL_COMPUTE_SHADER, compute_shader);
	m_fragment.Init(GL_FRAGMENT_SHADER, fragment_shader);
	m_geometry.Init(GL_GEOMETRY_SHADER, geometry_shader);
	m_tess_ctrl.Init(GL_TESS_CONTROL_SHADER, tesselation_control_shader);
	m_tess_eval.Init(GL_TESS_EVALUATION_SHADER, tesselation_evaluation_shader);
	m_vertex.Init(GL_VERTEX_SHADER, vertex_shader);

	/* Create program, set up transform feedback and attach shaders */
	Create(gl, m_id);
	Attach(gl, m_id, m_compute.m_id);
	Attach(gl, m_id, m_fragment.m_id);
	Attach(gl, m_id, m_geometry.m_id);
	Attach(gl, m_id, m_tess_ctrl.m_id);
	Attach(gl, m_id, m_tess_eval.m_id);
	Attach(gl, m_id, m_vertex.m_id);

	/* Link program */
	Link(gl, m_id);
}

/** Release program instance
 *
 **/
void Program::Release()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_invalid_id != m_id)
	{
		Use(gl, m_invalid_id);

		gl.deleteProgram(m_id);
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
	const Functions& gl = m_context.getRenderContext().getFunctions();

	Use(gl, m_id);
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
Shader::Shader(deqp::Context& context) : m_id(m_invalid_id), m_context(context)
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

	/* Create, set source and compile */
	const Functions& gl = m_context.getRenderContext().getFunctions();

	Create(gl, stage, m_id);
	Source(gl, m_id, source);

	Compile(gl, m_id);
}

/** Release shader instance
 *
 **/
void Shader::Release()
{
	if (m_invalid_id != m_id)
	{
		const Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteShader(m_id);
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
 **/
Texture::Texture() : m_id(m_invalid_id), m_context(0)
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

/** Initialize texture instance
 *
 * @param context Test context
 **/
void Texture::Init(deqp::Context& context)
{
	Release();

	m_context = &context;
}

/** Initialize texture instance as texture buffer
 *
 * @param context         Test context
 * @param internal_format Internal format of texture
 * @param buufer_id       ID of buffer that will be used as storage
 **/
void Texture::InitBuffer(deqp::Context& context, glw::GLenum internal_format, glw::GLuint buffer_id)
{
	Init(context);

	const Functions& gl = m_context->getRenderContext().getFunctions();

	Generate(gl, m_id);
	Bind(gl, m_id, GL_TEXTURE_BUFFER);
	Buffer::Bind(gl, buffer_id, GL_TEXTURE_BUFFER);

	gl.texBuffer(GL_TEXTURE_BUFFER, internal_format, buffer_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "TexBuffer");
}

/** Initialize texture instance with storage
 *
 * @param context         Test context
 * @param target          Texture target
 * @param levels          Number of levels
 * @param internal_format Internal format of texture
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 **/
void Texture::InitStorage(deqp::Context& context, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
						  glw::GLuint width, glw::GLuint height, glw::GLuint depth, bool allow_error)
{
	Init(context);

	const Functions& gl = m_context->getRenderContext().getFunctions();

	Generate(gl, m_id);
	Bind(gl, m_id, target);
	Storage(gl, target, levels, internal_format, width, height, depth, allow_error);
}

/** Release texture instance
 *
 * @param context CTS context.
 **/
void Texture::Release()
{
	if (m_invalid_id != m_id)
	{
		const Functions& gl = m_context->getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
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
					  glw::GLuint width, glw::GLuint height, glw::GLuint depth, bool allow_error)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		gl.texStorage1D(target, levels, internal_format, width);
		if (!allow_error)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage1D");
		}
		break;
	case GL_TEXTURE_1D_ARRAY:
	case GL_TEXTURE_2D:
	case GL_TEXTURE_RECTANGLE:
	case GL_TEXTURE_CUBE_MAP:
		gl.texStorage2D(target, levels, internal_format, width, height);
		if (!allow_error)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2D");
		}
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		gl.texStorage2DMultisample(target, levels, internal_format, width, height, GL_FALSE);
		if (!allow_error)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage2DMultisample");
		}
		break;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		gl.texStorage3DMultisample(target, levels, internal_format, width, height, depth, GL_FALSE);
		if (!allow_error)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3DMultisample");
		}
		break;
	case GL_TEXTURE_3D:
	case GL_TEXTURE_2D_ARRAY:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.texStorage3D(target, levels, internal_format, width, height, depth);
		if (!allow_error)
		{
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexStorage3D");
		}
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
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		gl.texSubImage3D(target, level, x, y, z, width, height, depth, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage3D");
		break;
	default:
		TCU_FAIL("Invliad enum");
		break;
	}
}

/* Gather info about buffer target */
struct bufferTargetInfo
{
	GLenum m_target;
	GLenum m_pname_alignment;
	GLenum m_pname_binding;
	GLenum m_pname_max;
	GLenum m_pname_max_size;
};

/* Gather info about texture target */
struct textureTargetInfo
{
	GLenum		  m_target;
	GLenum		  m_pname_binding;
	const GLchar* m_name;
};

/* Collects information about buffers */
static const bufferTargetInfo s_buffer_infos[] = {
	{ GL_ATOMIC_COUNTER_BUFFER, 0, GL_ATOMIC_COUNTER_BUFFER_BINDING, GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS,
	  GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE },
	{
		GL_TRANSFORM_FEEDBACK_BUFFER, 0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, GL_MAX_TRANSFORM_FEEDBACK_BUFFERS,
		GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
	},
	{ GL_UNIFORM_BUFFER, GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, GL_UNIFORM_BUFFER_BINDING, GL_MAX_UNIFORM_BUFFER_BINDINGS,
	  GL_MAX_UNIFORM_BLOCK_SIZE },
	{ GL_SHADER_STORAGE_BUFFER, GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, GL_SHADER_STORAGE_BUFFER_BINDING,
	  GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, GL_MAX_SHADER_STORAGE_BLOCK_SIZE },
};

static const size_t s_n_buffer_tragets = sizeof(s_buffer_infos) / sizeof(s_buffer_infos[0]);

/* Collects information about textures */
static const textureTargetInfo s_texture_infos[] = {
	{ GL_TEXTURE_1D, GL_TEXTURE_BINDING_1D, "1D" },
	{ GL_TEXTURE_1D_ARRAY, GL_TEXTURE_BINDING_1D_ARRAY, "1D_ARRAY" },
	{ GL_TEXTURE_2D, GL_TEXTURE_BINDING_2D, "2D" },
	{ GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BINDING_2D_ARRAY, "2D_ARRAY" },
	{ GL_TEXTURE_3D, GL_TEXTURE_BINDING_3D, "3D" },
	{ GL_TEXTURE_BUFFER, GL_TEXTURE_BINDING_BUFFER, "BUFFER" },
	{ GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BINDING_CUBE_MAP, "CUBE" },
	{ GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, "CUBE_ARRAY" },
	{ GL_TEXTURE_RECTANGLE, GL_TEXTURE_BINDING_RECTANGLE, "RECTANGLE" },
	{ GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BINDING_2D_MULTISAMPLE, "2D_MS" },
	{ GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY, "2D_MS_ARRAY" }
};

static const size_t s_n_texture_tragets = sizeof(s_texture_infos) / sizeof(s_texture_infos[0]);

/** Macro, verifies generated error, logs error message and throws failure
 *
 * @param expected_error Expected error value
 * @param error_message  Message logged if generated error is not the expected one
 **/
#define CHECK_ERROR(expected_error, error_message)                                                      \
	{                                                                                                   \
		GLenum generated_error = gl.getError();                                                         \
                                                                                                        \
		if (expected_error != generated_error)                                                          \
		{                                                                                               \
			m_context.getTestContext().getLog()                                                         \
				<< tcu::TestLog::Message << "File: " << __FILE__ << ", line: " << __LINE__              \
				<< ". Got wrong error: " << glu::getErrorStr(generated_error)                           \
				<< ", expected: " << glu::getErrorStr(expected_error) << ", message: " << error_message \
				<< tcu::TestLog::EndMessage;                                                            \
			TCU_FAIL("Invalid error generated");                                                        \
		}                                                                                               \
	}

/* Prototypes */
void replaceToken(const GLchar* token, size_t& search_position, const GLchar* text, std::string& string);

/** Checks binding
 *
 * @param context        Test contex
 * @param pname          Pname of binding
 * @param index          Index of binding
 * @param target_name    Name of target
 * @param expected_value Expected value of binding
 **/
void checkBinding(deqp::Context& context, GLenum pname, GLuint index, const std::string& target_name,
				  GLint expected_value)
{
	const Functions& gl = context.getRenderContext().getFunctions();

	GLint binding = -1;

	gl.getIntegeri_v(pname, index, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegeri_v");

	if (binding != expected_value)
	{
		context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid binding: " << binding
										  << ", expected: " << expected_value << ". Target: " << target_name
										  << " at index: " << index << tcu::TestLog::EndMessage;
		TCU_FAIL("Invalid binding");
	}
}

/** Checks bindings for given texture unit
 *
 * @param context        Test contex
 * @param pname          Binding pname of <expected_value>
 * @param index          Index of texture unit
 * @param expected_value Expected value of binding at <pname> target
 **/
void checkTextureBinding(deqp::Context& context, GLenum pname, GLuint index, GLint expected_value)
{
	const Functions& gl = context.getRenderContext().getFunctions();

	for (size_t i = 0; i < s_n_texture_tragets; ++i)
	{
		const GLenum  pname_binding = s_texture_infos[i].m_pname_binding;
		const GLchar* target_name   = s_texture_infos[i].m_name;

		GLint binding = -1;
		GLint value   = 0;

		gl.getIntegeri_v(pname_binding, index, &binding);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegeri_v");

		if (pname_binding == pname)
		{
			value = (GLint)expected_value;
		}

		if (binding != value)
		{
			context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid binding: " << binding
											  << ", expected: " << expected_value << ". Target: " << target_name
											  << " at index: " << index << tcu::TestLog::EndMessage;
			TCU_FAIL("Invalid binding");
		}
	}
}

/** Checks binding
 *
 * @param context        Test context
 * @param index          Index of binding
 * @param expected_value Expected value of binding
 **/
void checkVertexAttribBinding(deqp::Context& context, GLuint index, GLint expected_value)
{
	const Functions& gl = context.getRenderContext().getFunctions();

	GLint binding = -1;

	gl.getVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetVertexAttribiv");

	if (binding != expected_value)
	{
		context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid binding: " << binding
										  << ", expected: " << expected_value << ". Target: Vertex attribute"
										  << " at index: " << index << tcu::TestLog::EndMessage;
		TCU_FAIL("Invalid binding");
	}
}

/** Fills MS texture with specified value
 *
 * @param context        Test context
 * @param texture_id     Index of binding
 * @param value          Value for texture
 * @param is_array       Selects if array target should be used
 **/
void fillMSTexture(deqp::Context& context, GLuint texture_id, GLuint value, bool is_array)
{
	/* */
	static const GLchar* cs = "#version 430 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "layout (location = 0) writeonly uniform IMAGE uni_image;\n"
							  "\n"
							  "layout (location = 1) uniform uint uni_value;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    const POINT;\n"
							  "\n"
							  "    imageStore(uni_image, point, 0, uvec4(uni_value, 0, 0, 0));\n"
							  "}\n"
							  "\n";

	static const GLchar* array_image   = "uimage2DMSArray";
	static const GLchar* array_point   = "ivec3 point = ivec3(gl_WorkGroupID.x, gl_WorkGroupID.y, 0)";
	static const GLchar* regular_image = "uimage2DMS";
	static const GLchar* regular_point = "ivec2 point = ivec2(gl_WorkGroupID.x, gl_WorkGroupID.y)";

	/* */
	const Functions& gl		  = context.getRenderContext().getFunctions();
	const GLchar*	image	= (true == is_array) ? array_image : regular_image;
	const GLchar*	point	= (true == is_array) ? array_point : regular_point;
	size_t			 position = 0;
	std::string		 source   = cs;

	/* */
	replaceToken("IMAGE", position, image, source);
	replaceToken("POINT", position, point, source);

	/* */
	Program program(context);
	program.Init(source.c_str(), "", "", "", "", "");
	program.Use();

	/* */
	if (true == is_array)
	{
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_TRUE /* layered */, 0 /* layer */,
							GL_WRITE_ONLY, GL_R32UI);
	}
	else
	{
		gl.bindImageTexture(0 /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							GL_WRITE_ONLY, GL_R32UI);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTexture");

	gl.uniform1i(0 /* location */, 0 /* image unit*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");

	gl.uniform1ui(1 /* location */, value /* uni_value */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1ui");

	/* */
	gl.dispatchCompute(6, 6, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");
}

/** Get texture binding pname for given index
 *
 * @param index Index of texture target
 *
 * @return Pname
 **/
GLenum getBinding(GLuint index)
{
	if (index < s_n_texture_tragets)
	{
		return s_texture_infos[index].m_pname_binding;
	}
	else
	{
		return GL_TEXTURE_BINDING_2D;
	}
}

/** Get texture target for given index
 *
 * @param index Index of texture target
 *
 * @return Target
 **/
GLenum getTarget(GLuint index)
{
	if (index < s_n_texture_tragets)
	{
		return s_texture_infos[index].m_target;
	}
	else
	{
		return GL_TEXTURE_2D;
	}
}

/** Replace first occurance of <token> with <text> in <string> starting at <search_posistion>
 *
 * @param token           Token string
 * @param search_position Position at which find will start, it is updated to position at which replaced text ends
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceToken(const GLchar* token, size_t& search_position, const GLchar* text, std::string& string)
{
	const size_t text_length	= strlen(text);
	const size_t token_length   = strlen(token);
	const size_t token_position = string.find(token, search_position);

	string.replace(token_position, token_length, text, text_length);

	search_position = token_position + text_length;
}

/** Constructor
 *
 * @param context Test context
 **/
ErrorsBindBuffersTest::ErrorsBindBuffersTest(deqp::Context& context)
	: TestCase(context, "errors_bind_buffers", "Verifies that proper errors are generated by buffer binding routines")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ErrorsBindBuffersTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	/* - INVALID_ENUM when <target> is not valid; */
	{
		static const GLintptr buffer_size = 16;
		static const GLsizei  count		  = 1;
		static const GLuint   first		  = 0;
		static const GLintptr offset	  = 4;
		static const GLintptr size		  = buffer_size - offset;

		Buffer buffer;

		buffer.InitData(m_context, GL_ARRAY_BUFFER, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

		gl.bindBuffersBase(GL_ARRAY_BUFFER, first, count, &buffer.m_id);
		CHECK_ERROR(GL_INVALID_ENUM, "BindBuffersBase with invalid <target>");

		gl.bindBuffersRange(GL_ARRAY_BUFFER, first, count, &buffer.m_id, &offset, &size);
		CHECK_ERROR(GL_INVALID_ENUM, "BindBuffersRange with invalid <target>");
	}

	for (size_t i = 0; i < s_n_buffer_tragets; ++i)
	{
		static const GLsizei n_buffers = 4;

		const GLenum	   pname_alignment = s_buffer_infos[i].m_pname_alignment;
		const GLenum	   pname_max	   = s_buffer_infos[i].m_pname_max;
		const GLenum	   target		   = s_buffer_infos[i].m_target;
		const std::string& target_name	 = glu::getBufferTargetStr(target).toString();

		GLintptr buffer_size	  = 16;
		GLsizei  count			  = n_buffers;
		GLuint   first			  = 0;
		GLuint   invalid_id		  = 1; /* Start with 1, as 0 is not valid name */
		GLintptr offset			  = 4; /* ATOMIC and XFB require alignment of 4 */
		GLint	offset_alignment = 1;
		GLint	max_buffers	  = 0;
		GLintptr size			  = buffer_size - offset;
		size_t   validated_index  = n_buffers - 1;

		/* Get alignment */
		if (0 != pname_alignment)
		{
			gl.getIntegerv(pname_alignment, &offset_alignment);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

			buffer_size += offset_alignment;
			offset = offset_alignment;
			size   = buffer_size - offset;
		}

		/* Get max */
		gl.getIntegerv(pname_max, &max_buffers);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		/* Select count so <first + count> does not exceed max.
		 * Validated index shall be in the specified range.
		 */
		if (n_buffers > max_buffers)
		{
			count			= max_buffers;
			validated_index = max_buffers - 1;
		}

		/* Storage */
		Buffer   buffer[n_buffers];
		GLuint   buffer_ids[n_buffers];
		GLintptr offsets[n_buffers];
		GLintptr sizes[n_buffers];

		/* Prepare buffers */
		for (size_t j = 0; j < n_buffers; ++j)
		{
			buffer[j].InitData(m_context, target, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

			buffer_ids[j] = buffer[j].m_id;
			offsets[j]	= offset;
			sizes[j]	  = size;
		}

		/* - INVALID_OPERATION when <first> + <count> is greater than allowed limit; */
		{
			GLsizei t_count = n_buffers;
			GLuint  t_first = 0;

			/* Select first so <first + count> exceeds max, avoid negative first */
			if (n_buffers <= max_buffers)
			{
				t_first = max_buffers - n_buffers + 1;
			}
			else
			{
				t_count = max_buffers + 1;
				/* first = 0; */
			}

			/* Test */
			gl.bindBuffersBase(target, t_first, t_count, buffer_ids);
			CHECK_ERROR(GL_INVALID_OPERATION,
						"BindBuffersBase with invalid <first> + <count>, target: " << target_name);

			gl.bindBuffersRange(target, t_first, t_count, buffer_ids, offsets, sizes);
			CHECK_ERROR(GL_INVALID_OPERATION,
						"BindBuffersRange with invalid <first> + <count>, target: " << target_name);
		}

		/* - INVALID_OPERATION if any value in <buffers> is not zero or the name of
		 * existing buffer;
		 */
		{
			GLuint t_buffer_ids[n_buffers];

			memcpy(t_buffer_ids, buffer_ids, sizeof(buffer_ids));

			/* Find invalid id */
			while (1)
			{
				if (GL_TRUE != gl.isBuffer(invalid_id))
				{
					break;
				}

				invalid_id += 1;
			}

			/* Invalidate the entry */
			t_buffer_ids[validated_index] = invalid_id;

			/* Test */
			gl.bindBuffersBase(target, first, count, t_buffer_ids);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindBuffersBase with invalid buffer id, target: " << target_name);

			gl.bindBuffersRange(target, first, count, t_buffer_ids, offsets, sizes);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindBuffersRange with invalid buffer id, target: " << target_name);
		}

		/* - INVALID_VALUE if any value in <offsets> is less than zero; */
		{
			GLintptr t_offsets[n_buffers];
			GLintptr t_sizes[n_buffers];

			memcpy(t_offsets, offsets, sizeof(offsets));
			memcpy(t_sizes, sizes, sizeof(sizes));

			/* Invalidate the entry */
			t_offsets[validated_index] = -1;
			t_sizes[validated_index]   = -1;

			/* Test */
			gl.bindBuffersRange(target, first, count, buffer_ids, t_offsets, sizes);
			CHECK_ERROR(GL_INVALID_VALUE, "BindBuffersRange with negative offset, target: " << target_name);

			/* Test */
			gl.bindBuffersRange(target, first, count, buffer_ids, offsets, t_sizes);
			CHECK_ERROR(GL_INVALID_VALUE, "BindBuffersRange with negative size, target: " << target_name);
		}

		/* - INVALID_VALUE if any pair of <offsets> and <sizes> exceeds limits. */
		{
			GLintptr t_offsets[n_buffers];
			GLintptr t_sizes[n_buffers];

			memcpy(t_offsets, offsets, sizeof(offsets));
			memcpy(t_sizes, sizes, sizeof(sizes));

			/* Invalidate the entry */
			t_offsets[validated_index] -= 1;	 /* Not aligned by required value */
			t_sizes[validated_index] = size - 1; /* Not aligned by required value */

			/* Test */
			gl.bindBuffersRange(target, first, count, buffer_ids, t_offsets, sizes);
			CHECK_ERROR(GL_INVALID_VALUE, "BindBuffersRange with invalid <offset>, target: " << target_name);

			/* Test */
			if (GL_TRANSFORM_FEEDBACK_BUFFER == target)
			{
				gl.bindBuffersRange(target, first, count, buffer_ids, offsets, t_sizes);
				CHECK_ERROR(GL_INVALID_VALUE, "BindBuffersRange with invalid <size>, target: " << target_name);
			}
		}
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
ErrorsBindTexturesTest::ErrorsBindTexturesTest(deqp::Context& context)
	: TestCase(context, "errors_bind_textures", "Verifies that proper errors are generated by texture binding routines")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ErrorsBindTexturesTest::iterate()
{
	static const GLuint  depth		= 8;
	static const GLuint  height		= 8;
	static const GLsizei n_textures = 4;
	static const GLuint  width		= 8;

	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLsizei count			= n_textures;
	GLuint  first			= 0;
	GLuint  invalid_id		= 1; /* Start with 1, as 0 is not valid name */
	GLint   max_textures	= 0;
	size_t  validated_index = n_textures - 1;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Select count so <first + count> does not exceed max.
	 * Validated index shall be in the specified range.
	 */
	if (n_textures > max_textures)
	{
		count			= max_textures;
		validated_index = max_textures - 1;
	}

	/* Storage */
	Texture texture[n_textures];
	GLuint  texture_ids[n_textures];

	/* Prepare textures */
	texture[0].InitStorage(m_context, GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, width, height, depth);
	texture[1].InitStorage(m_context, GL_TEXTURE_2D_ARRAY, 1 /* levels */, GL_RGBA8, width, height, depth);
	texture[2].InitStorage(m_context, GL_TEXTURE_1D_ARRAY, 1 /* levels */, GL_RGBA8, width, height, depth);
	texture[3].InitStorage(m_context, GL_TEXTURE_3D, 1 /* levels */, GL_RGBA8, width, height, depth);

	for (size_t i = 0; i < n_textures; ++i)
	{
		texture_ids[i] = texture[i].m_id;
	}

	/* - INVALID_OPERATION when <first> + <count> exceed limits; */
	{
		GLsizei t_count = n_textures;
		GLuint  t_first = 0;

		/* Select first so <first + count> exceeds max, avoid negative first */
		if (n_textures <= max_textures)
		{
			t_first = max_textures - n_textures + 1;
		}
		else
		{
			t_count = max_textures + 1;
			/* first = 0; */
		}

		/* Test */
		gl.bindTextures(t_first, t_count, texture_ids);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindTextures with invalid <first> + <count>");
	}

	/* - INVALID_OPERATION if any value in <buffers> is not zero or the name of
	 * existing buffer;
	 */
	{
		GLuint t_texture_ids[n_textures];

		memcpy(t_texture_ids, texture_ids, sizeof(texture_ids));

		/* Find invalid id */
		while (1)
		{
			if (GL_TRUE != gl.isTexture(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		/* Invalidate the entry */
		t_texture_ids[validated_index] = invalid_id;

		/* Test */
		gl.bindTextures(first, count, t_texture_ids);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindTextures with invalid texture id");
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
ErrorsBindSamplersTest::ErrorsBindSamplersTest(deqp::Context& context)
	: TestCase(context, "errors_bind_samplers", "Verifies that proper errors are generated by sampler binding routines")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ErrorsBindSamplersTest::iterate()
{
	static const GLsizei n_samplers = 4;

	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLsizei count			= n_samplers;
	GLuint  first			= 0;
	GLuint  invalid_id		= 1; /* Start with 1, as 0 is not valid name */
	GLint   max_samplers	= 0;
	size_t  validated_index = n_samplers - 1;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_samplers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Select count so <first + count> does not exceed max.
	 * Validated index shall be in the specified range.
	 */
	if (n_samplers > max_samplers)
	{
		count			= max_samplers;
		validated_index = max_samplers - 1;
	}

	/* Storage */
	GLuint sampler_ids[n_samplers];

	/* Prepare samplers */
	gl.genSamplers(n_samplers, sampler_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenSamplers");

	try
	{
		/* - INVALID_OPERATION when <first> + <count> exceed limits; */
		{
			GLsizei t_count = n_samplers;
			GLuint  t_first = 0;

			/* Select first so <first + count> exceeds max, avoid negative first */
			if (n_samplers <= max_samplers)
			{
				t_first = max_samplers - n_samplers + 1;
			}
			else
			{
				t_count = max_samplers + 1;
				/* first = 0; */
			}

			/* Test */
			gl.bindSamplers(t_first, t_count, sampler_ids);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindSamplers with invalid <first> + <count>");
		}

		/* - INVALID_OPERATION if any value in <buffers> is not zero or the name of
		 * existing buffer;
		 */
		{
			GLuint t_sampler_ids[n_samplers];

			memcpy(t_sampler_ids, sampler_ids, sizeof(sampler_ids));

			/* Find invalid id */
			while (1)
			{
				if (GL_TRUE != gl.isTexture(invalid_id))
				{
					break;
				}

				invalid_id += 1;
			}

			/* Invalidate the entry */
			t_sampler_ids[validated_index] = invalid_id;

			/* Test */
			gl.bindTextures(first, count, t_sampler_ids);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindSamplers with invalid sampler id");
		}
	}
	catch (const std::exception&)
	{
		gl.deleteSamplers(n_samplers, sampler_ids);

		TCU_FAIL("Invalid error generated");
	}

	/* Delete samplers */
	gl.deleteSamplers(n_samplers, sampler_ids);

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
ErrorsBindImageTexturesTest::ErrorsBindImageTexturesTest(deqp::Context& context)
	: TestCase(context, "errors_bind_image_textures",
			   "Verifies that proper errors are generated by image binding routines")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ErrorsBindImageTexturesTest::iterate()
{
	static const GLuint  depth		= 8;
	static const GLuint  height		= 8;
	static const GLsizei n_textures = 4;
	static const GLuint  width		= 8;

	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLsizei count			= n_textures;
	GLuint  first			= 0;
	GLuint  invalid_id		= 1; /* Start with 1, as 0 is not valid name */
	GLint   max_textures	= 0;
	size_t  validated_index = n_textures - 1;

	/* Get max */
	gl.getIntegerv(GL_MAX_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Select count so <first + count> does not exceed max.
	 * Validated index shall be in the specified range.
	 */
	if (n_textures > max_textures)
	{
		count			= max_textures;
		validated_index = max_textures - 1;
	}

	/* Storage */
	Texture texture[n_textures];
	GLuint  texture_ids[n_textures];

	/* Prepare textures */
	texture[0].InitStorage(m_context, GL_TEXTURE_2D, 1, GL_RGBA8, width, height, depth);
	texture[1].InitStorage(m_context, GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, depth);
	texture[2].InitStorage(m_context, GL_TEXTURE_1D_ARRAY, 1, GL_RGBA8, width, height, depth);
	texture[3].InitStorage(m_context, GL_TEXTURE_3D, 1, GL_RGBA8, width, height, depth);

	for (size_t i = 0; i < n_textures; ++i)
	{
		texture_ids[i] = texture[i].m_id;
	}

	/* - INVALID_OPERATION when <first> + <count> exceed limits; */
	{
		GLsizei t_count = n_textures;
		GLuint  t_first = 0;

		/* Select first so <first + count> exceeds max, avoid negative first */
		if (n_textures <= max_textures)
		{
			t_first = max_textures - n_textures + 1;
		}
		else
		{
			t_count = max_textures + 1;
			/* first = 0; */
		}

		/* Test */
		gl.bindImageTextures(t_first, t_count, texture_ids);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindImageTextures with invalid <first> + <count>");
	}

	/* - INVALID_OPERATION if any value in <buffers> is not zero or the name of
	 * existing buffer;
	 */
	{
		GLuint t_texture_ids[n_textures];

		memcpy(t_texture_ids, texture_ids, sizeof(texture_ids));

		/* Find invalid id */
		while (1)
		{
			if (GL_TRUE != gl.isTexture(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		/* Invalidate the entry */
		t_texture_ids[validated_index] = invalid_id;

		/* Test */
		gl.bindImageTextures(first, count, t_texture_ids);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindImageTextures with invalid texture id");
	}

	/* - INVALID_OPERATION if any entry found in <textures> has invalid internal
	 * format at level 0;
	 */
	{
		GLuint t_texture_ids[n_textures];

		memcpy(t_texture_ids, texture_ids, sizeof(texture_ids));

		/* Prepare texture with invalid format */
		Texture t_texture;
		t_texture.Init(m_context);
		t_texture.Generate(gl, t_texture.m_id);
		t_texture.Bind(gl, t_texture.m_id, GL_TEXTURE_2D);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGB9_E5, width, 0);
		CHECK_ERROR(GL_INVALID_VALUE, "texStorage2D has height set to 0");

		/* Invalidate the entry */
		t_texture_ids[validated_index] = t_texture.m_id;

		/* Test */
		gl.bindImageTextures(first, count, t_texture_ids);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindImageTextures with invalid internal format");
	}

	/* - INVALID_VALUE when any entry in <textures> has any of dimensions equal
	 * to 0 at level 0.
	 */
	{
		GLuint t_texture_ids[n_textures];

		memcpy(t_texture_ids, texture_ids, sizeof(texture_ids));

		/* Prepare texture with invalid format */
		Texture t_texture;
		t_texture.InitStorage(m_context, GL_TEXTURE_2D, 1, GL_RGB9_E5, width, 0, depth, true);

		/* Invalidate the entry */
		t_texture_ids[validated_index] = t_texture.m_id;

		/* Test */
		gl.bindImageTextures(first, count, t_texture_ids);
		CHECK_ERROR(GL_INVALID_VALUE, "BindImageTextures with 2D texture that has height set to 0");
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
ErrorsBindVertexBuffersTest::ErrorsBindVertexBuffersTest(deqp::Context& context)
	: TestCase(context, "errors_bind_vertex_buffers",
			   "Verifies that proper errors are generated by vertex buffer binding routines")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ErrorsBindVertexBuffersTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	static const GLsizei n_buffers = 4;
	static const GLsizei stride	= 4;

	GLintptr buffer_size	 = 16;
	GLsizei  count			 = n_buffers;
	GLuint   first			 = 0;
	GLuint   invalid_id		 = 1; /* Start with 1, as 0 is not valid name */
	GLintptr offset			 = 4; /* ATOMIC and XFB require alignment of 4 */
	GLint	max_buffers	 = 0;
	size_t   validated_index = n_buffers - 1;

	/* Get max */
	gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Select count so <first + count> does not exceed max.
	 * Validated index shall be in the specified range.
	 */
	if (n_buffers > max_buffers)
	{
		count			= max_buffers;
		validated_index = max_buffers - 1;
	}

	/* Storage */
	Buffer   buffer[n_buffers];
	GLuint   buffer_ids[n_buffers];
	GLintptr offsets[n_buffers];
	GLsizei  strides[n_buffers];

	/* Prepare buffers */
	for (size_t j = 0; j < n_buffers; ++j)
	{
		buffer[j].InitData(m_context, GL_ARRAY_BUFFER, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

		buffer_ids[j] = buffer[j].m_id;
		offsets[j]	= offset;
		strides[j]	= stride;
	}

	/* Prepare VAO */
	GLuint vao = 0;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");
	try
	{
		gl.bindVertexArray(vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArrays");

		/* - INVALID_OPERATION when <first> + <count> exceeds limits; */
		{
			GLsizei t_count = n_buffers;
			GLuint  t_first = 0;

			/* Select first so <first + count> exceeds max, avoid negative first */
			if (n_buffers <= max_buffers)
			{
				t_first = max_buffers - n_buffers + 1;
			}
			else
			{
				t_count = max_buffers + 1;
				/* first = 0; */
			}

			/* Test */
			gl.bindVertexBuffers(t_first, t_count, buffer_ids, offsets, strides);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindVertexBuffers with invalid <first> + <count>");
		}

		/* - INVALID_OPERATION if any value in <buffers> is not zero or the name of
		 * existing buffer;
		 */
		{
			GLuint t_buffer_ids[n_buffers];

			memcpy(t_buffer_ids, buffer_ids, sizeof(buffer_ids));

			/* Find invalid id */
			while (1)
			{
				if (GL_TRUE != gl.isBuffer(invalid_id))
				{
					break;
				}

				invalid_id += 1;
			}

			/* Invalidate the entry */
			t_buffer_ids[validated_index] = invalid_id;

			/* Test */
			gl.bindVertexBuffers(first, count, t_buffer_ids, offsets, strides);
			CHECK_ERROR(GL_INVALID_OPERATION, "BindVertexBuffers with invalid buffer id");
		}

		/* - INVALID_VALUE if any value in <offsets> or <strides> is less than zero. */
		{
			GLintptr t_offsets[n_buffers];
			GLsizei  t_strides[n_buffers];

			memcpy(t_offsets, offsets, sizeof(offsets));
			memcpy(t_strides, strides, sizeof(strides));

			/* Invalidate the entry */
			t_offsets[validated_index] = -1;
			t_strides[validated_index] = -1;

			/* Test */
			gl.bindVertexBuffers(first, count, buffer_ids, t_offsets, strides);
			CHECK_ERROR(GL_INVALID_VALUE, "BindVertexBuffers with negative offset");

			gl.bindVertexBuffers(first, count, buffer_ids, offsets, t_strides);
			CHECK_ERROR(GL_INVALID_VALUE, "BindVertexBuffers with negative stride");
		}
	}
	catch (const std::exception&)
	{
		gl.deleteVertexArrays(1, &vao);
		TCU_FAIL("Unexpected error generated");
	}

	gl.deleteVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteVertexArrays");

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindBuffersBaseTest::FunctionalBindBuffersBaseTest(deqp::Context& context)
	: TestCase(context, "functional_bind_buffers_base", "Verifies that BindBuffersBase works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindBuffersBaseTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	for (size_t i = 0; i < s_n_buffer_tragets; ++i)
	{
		const GLenum	   pname_binding  = s_buffer_infos[i].m_pname_binding;
		const GLenum	   pname_max	  = s_buffer_infos[i].m_pname_max;
		const GLenum	   pname_max_size = s_buffer_infos[i].m_pname_max_size;
		const GLenum	   target		  = s_buffer_infos[i].m_target;
		const std::string& target_name	= glu::getBufferTargetStr(target).toString();

		GLint max_buffers = 0;
		GLint max_size	= 0;

		/* Get max */
		gl.getIntegerv(pname_max, &max_buffers);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		/* Get max size */
		gl.getIntegerv(pname_max_size, &max_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		GLintptr buffer_size = max_size / max_buffers;

		/* Storage */
		std::vector<Buffer> buffer;
		std::vector<GLuint> buffer_ids;

		buffer.resize(max_buffers);
		buffer_ids.resize(max_buffers);

		/* Prepare buffers */
		for (GLint j = 0; j < max_buffers; ++j)
		{
			buffer[j].InitData(m_context, target, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

			buffer_ids[j] = buffer[j].m_id;
		}

		/*
		 * - execute BindBufferBase to bind all buffers to tested target;
		 * - inspect if bindings were modified;
		 */
		gl.bindBuffersBase(target, 0 /* first */, max_buffers /* count */, &buffer_ids[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersBase");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/*
		 *
		 * - execute BindBufferBase for first half of bindings with NULL as <buffers>
		 * to unbind first half of bindings for tested target;
		 * - inspect if bindings were modified;
		 * - execute BindBufferBase for second half of bindings with NULL as <buffers>
		 * to unbind rest of bindings;
		 * - inspect if bindings were modified;
		 */
		GLint half_index = max_buffers / 2;
		gl.bindBuffersBase(target, 0 /* first */, half_index /* count */, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersBase");

		for (GLint j = 0; j < half_index; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, 0);
		}

		for (GLint j = half_index; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		gl.bindBuffersBase(target, half_index /* first */, max_buffers - half_index /* count */, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersBase");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, 0);
		}

		/*
		 * - change <buffers> so first entry is invalid;
		 * - execute BindBufferBase to bind all buffers to tested target; It is
		 * expected that INVALID_OPERATION will be generated;
		 * - inspect if all bindings but first were modified;
		 */

		/* Find invalid id */
		GLuint invalid_id = 1;
		while (1)
		{
			if (GL_TRUE != gl.isBuffer(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		buffer_ids[0] = invalid_id;

		gl.bindBuffersBase(target, 0 /* first */, max_buffers /* count */, &buffer_ids[0]);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindBufferBase with invalid buffer id");

		/* Update buffer_ids */
		buffer_ids[0] = 0; /* 0 means unbound */

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/*
		 * - bind any buffer to first binding;
		 * - execute BindBufferBase for 0 as <first>, 1 as <count> and <buffers> filled
		 * with zeros to unbind 1st binding for tested target;
		 * - inspect if bindings were modified;
		 */
		gl.bindBufferBase(target, 0, buffer[0].m_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferBase");
		checkBinding(m_context, pname_binding, 0, target_name, buffer[0].m_id);

		std::vector<GLuint> t_buffer_ids;
		t_buffer_ids.resize(max_buffers);

		gl.bindBuffersBase(target, 0 /* first */, 1 /* count */, &t_buffer_ids[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersBase");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/* - unbind all buffers. */
		gl.bindBuffersBase(target, 0 /* first */, max_buffers /* count */, 0);
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindBuffersRangeTest::FunctionalBindBuffersRangeTest(deqp::Context& context)
	: TestCase(context, "functional_bind_buffers_range", "Verifies that BindBuffersRange works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindBuffersRangeTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	for (size_t i = 0; i < s_n_buffer_tragets; ++i)
	{
		const GLenum	   pname_binding  = s_buffer_infos[i].m_pname_binding;
		const GLenum	   pname_max	  = s_buffer_infos[i].m_pname_max;
		const GLenum	   pname_max_size = s_buffer_infos[i].m_pname_max_size;
		const GLenum	   target		  = s_buffer_infos[i].m_target;
		const std::string& target_name	= glu::getBufferTargetStr(target).toString();

		GLint max_buffers = 0;
		GLint max_size	= 0;

		/* Get max */
		gl.getIntegerv(pname_max, &max_buffers);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		/* Get max size */
		gl.getIntegerv(pname_max_size, &max_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		GLintptr buffer_size = max_size / max_buffers;

		/* Storage */
		std::vector<Buffer>		buffer;
		std::vector<GLuint>		buffer_ids;
		std::vector<GLintptr>   offsets;
		std::vector<GLsizeiptr> sizes;

		buffer.resize(max_buffers);
		buffer_ids.resize(max_buffers);
		offsets.resize(max_buffers);
		sizes.resize(max_buffers);

		/* Prepare buffers */
		for (GLint j = 0; j < max_buffers; ++j)
		{
			buffer[j].InitData(m_context, target, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

			buffer_ids[j] = buffer[j].m_id;
			offsets[j]	= 0;
			sizes[j]	  = buffer_size;
		}

		/*
		 * - execute BindBufferBase to bind all buffers to tested target;
		 * - inspect if bindings were modified;
		 */
		gl.bindBuffersRange(target, 0 /* first */, max_buffers /* count */, &buffer_ids[0], &offsets[0], &sizes[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersRange");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/*
		 *
		 * - execute BindBufferBase for first half of bindings with NULL as <buffers>
		 * to unbind first half of bindings for tested target;
		 * - inspect if bindings were modified;
		 * - execute BindBufferBase for second half of bindings with NULL as <buffers>
		 * to unbind rest of bindings;
		 * - inspect if bindings were modified;
		 */
		GLint half_index = max_buffers / 2;
		gl.bindBuffersRange(target, 0 /* first */, half_index /* count */, 0, &offsets[0], &sizes[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersRange");

		for (GLint j = 0; j < half_index; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, 0);
		}

		for (GLint j = half_index; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		gl.bindBuffersRange(target, half_index /* first */, max_buffers - half_index /* count */, 0, &offsets[0],
							&sizes[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersRange");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, 0);
		}

		/*
		 * - change <buffers> so first entry is invalid;
		 * - execute BindBufferBase to bind all buffers to tested target; It is
		 * expected that INVALID_OPERATION will be generated;
		 * - inspect if all bindings but first were modified;
		 */

		/* Find invalid id */
		GLuint invalid_id = 1;
		while (1)
		{
			if (GL_TRUE != gl.isBuffer(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		buffer_ids[0] = invalid_id;

		gl.bindBuffersRange(target, 0 /* first */, max_buffers /* count */, &buffer_ids[0], &offsets[0], &sizes[0]);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindBuffersRange with invalid buffer id");

		/* Update buffer_ids */
		buffer_ids[0] = 0; /* 0 means unbound */

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/*
		 * - bind any buffer to first binding;
		 * - execute BindBufferBase for 0 as <first>, 1 as <count> and <buffers> filled
		 * with zeros to unbind 1st binding for tested target;
		 * - inspect if bindings were modified;
		 */
		gl.bindBufferBase(target, 0, buffer[0].m_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferBase");
		checkBinding(m_context, pname_binding, 0, target_name, buffer[0].m_id);

		std::vector<GLuint> t_buffer_ids;
		t_buffer_ids.resize(max_buffers);

		gl.bindBuffersRange(target, 0 /* first */, 1 /* count */, &t_buffer_ids[0], &offsets[0], &sizes[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersRange");

		for (GLint j = 0; j < max_buffers; ++j)
		{
			checkBinding(m_context, pname_binding, j, target_name, buffer_ids[j]);
		}

		/* - unbind all buffers. */
		gl.bindBuffersBase(target, 0 /* first */, max_buffers /* count */, 0);
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindTexturesTest::FunctionalBindTexturesTest(deqp::Context& context)
	: TestCase(context, "functional_bind_textures", "Verifies that BindTextures works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindTexturesTest::iterate()
{
	static const GLuint depth  = 6;
	static const GLuint height = 6;
	static const GLuint width  = 6;

	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLuint invalid_id   = 1; /* Start with 1, as 0 is not valid name */
	GLint  max_textures = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	Buffer				 buffer;
	std::vector<Texture> texture;
	std::vector<GLuint>  texture_ids;
	std::vector<GLuint>  t_texture_ids;

	texture.resize(max_textures);
	texture_ids.resize(max_textures);
	t_texture_ids.resize(max_textures);

	/* Prepare buffer */
	buffer.InitData(m_context, GL_TEXTURE_BUFFER, GL_DYNAMIC_COPY, 16 /* size */, 0 /* data */);

	/* Prepare textures */
	for (size_t i = 0; i < s_n_texture_tragets; ++i)
	{
		const GLenum target = s_texture_infos[i].m_target;

		if (GL_TEXTURE_BUFFER != target)
		{
			texture[i].InitStorage(m_context, target, 1, GL_RGBA8, width, height, depth);
		}
		else
		{
			texture[i].InitBuffer(m_context, GL_RGBA8, buffer.m_id);
		}

		/* Unbind */
		Texture::Bind(gl, 0, target);
	}

	for (GLint i = s_n_texture_tragets; i < max_textures; ++i)
	{
		texture[i].InitStorage(m_context, GL_TEXTURE_2D, 1, GL_RGBA8, width, height, depth);
	}

	/* Unbind */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	for (GLint i = 0; i < max_textures; ++i)
	{
		texture_ids[i] = texture[i].m_id;
	}

	/*
	 * - execute BindTextures to bind all textures;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * set;
	 */
	gl.bindTextures(0, max_textures, &texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	for (GLint i = 0; i < max_textures; ++i)
	{
		checkTextureBinding(m_context, getBinding(i), i, texture_ids[i]);
	}

	/*
	 * - execute BindTextures for the first half of units with <textures> filled
	 * with zeros, to unbind those units;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * unbound;
	 */
	GLint half_index = max_textures / 2;

	for (GLint i = 0; i < max_textures; ++i)
	{
		t_texture_ids[i] = 0;
	}

	gl.bindTextures(0, half_index, &t_texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	for (GLint i = 0; i < half_index; ++i)
	{
		checkTextureBinding(m_context, getBinding(i), i, 0);
	}

	for (GLint i = half_index; i < max_textures; ++i)
	{
		checkTextureBinding(m_context, getBinding(i), i, texture_ids[i]);
	}

	/*
	 * - execute BindTextures for the second half of units with NULL as<textures>,
	 * to unbind those units;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * unbound;
	 */
	gl.bindTextures(half_index, max_textures - half_index, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	for (GLint i = 0; i < max_textures; ++i)
	{
		checkTextureBinding(m_context, getBinding(i), i, 0);
	}

	/*
	 * - modify <textures> so first entry is invalid;
	 * - execute BindTextures to bind all textures; It is expected that
	 * INVALID_OPERATION will be generated;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * set;
	 */

	/* Find invalid id */
	while (1)
	{
		if (GL_TRUE != gl.isTexture(invalid_id))
		{
			break;
		}

		invalid_id += 1;
	}

	/* Set invalid id */
	texture_ids[0] = invalid_id;

	gl.bindTextures(0, max_textures, &texture_ids[0]);
	CHECK_ERROR(GL_INVALID_OPERATION, "BindTextures with invalid texture id");

	checkTextureBinding(m_context, getBinding(0), 0, 0);
	for (GLint i = 1; i < max_textures; ++i)
	{
		checkTextureBinding(m_context, getBinding(i), i, texture_ids[i]);
	}

	/* - unbind all textures. */
	gl.bindTextures(0, max_textures, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindSamplersTest::FunctionalBindSamplersTest(deqp::Context& context)
	: TestCase(context, "functional_bind_samplers", "Verifies that BindSamplers works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindSamplersTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLuint invalid_id   = 1; /* Start with 1, as 0 is not valid name */
	GLint  max_samplers = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_samplers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	std::vector<GLuint> sampler_ids;
	std::vector<GLuint> t_sampler_ids;

	sampler_ids.resize(max_samplers);
	t_sampler_ids.resize(max_samplers);

	for (GLint i = 0; i < max_samplers; ++i)
	{
		t_sampler_ids[i] = 0;
	}

	/* Prepare samplers */
	gl.genSamplers(max_samplers, &sampler_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenSamplers");

	try
	{
		/* - execute BindSamplers to bind all samplers;
		 * - inspect bindings to verify that proper samplers were set;
		 */
		gl.bindSamplers(0 /* first */, max_samplers /* count */, &sampler_ids[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindSamplers");

		for (GLint i = 0; i < max_samplers; ++i)
		{
			checkBinding(m_context, GL_SAMPLER_BINDING, i, "Sampler", sampler_ids[i]);
		}

		/* - execute BindSamplers for first half of bindings with <samplers> filled
		 * with zeros, to unbind those samplers;
		 * - inspect bindings to verify that proper samplers were unbound;
		 */
		GLint half_index = max_samplers / 2;

		gl.bindSamplers(0, half_index, &t_sampler_ids[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindSamplers");

		for (GLint i = 0; i < half_index; ++i)
		{
			checkBinding(m_context, GL_SAMPLER_BINDING, i, "Sampler", 0);
		}

		for (GLint i = half_index; i < max_samplers; ++i)
		{
			checkBinding(m_context, GL_SAMPLER_BINDING, i, "Sampler", sampler_ids[i]);
		}

		/* - execute BindSamplers for second half of bindings with NULL as <samplers>,
		 * to unbind those samplers;
		 * - inspect bindings to verify that proper samplers were unbound;
		 */
		gl.bindSamplers(half_index, max_samplers - half_index, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindSamplers");

		for (GLint i = 0; i < max_samplers; ++i)
		{
			checkBinding(m_context, GL_SAMPLER_BINDING, i, "Sampler", 0);
		}

		/* - modify <samplers> so first entry is invalid;
		 * - execute BindSamplers to bind all samplers; It is expected that
		 * INVALID_OPERATION will be generated;
		 * - inspect bindings to verify that proper samplers were set;
		 */

		/* Find invalid id */
		while (1)
		{
			if (GL_TRUE != gl.isSampler(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		/* Prepare ids */
		t_sampler_ids[0] = invalid_id;

		for (GLint i = 1; i < max_samplers; ++i)
		{
			t_sampler_ids[i] = sampler_ids[i];
		}

		/* Bind */
		gl.bindSamplers(0, max_samplers, &t_sampler_ids[0]);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindSamplers with invalid sampler id");

		/* Set 0 for invalid entry */
		t_sampler_ids[0] = 0;

		for (GLint i = 0; i < max_samplers; ++i)
		{
			checkBinding(m_context, GL_SAMPLER_BINDING, i, "Sampler", t_sampler_ids[i]);
		}

		/* - unbind all samplers. */
		gl.bindSamplers(0, max_samplers, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindSamplers");
	}
	catch (const std::exception&)
	{
		gl.deleteSamplers(max_samplers, &sampler_ids[0]);

		TCU_FAIL("Invalid error generated");
	}

	/* Delete samplers */
	gl.deleteSamplers(max_samplers, &sampler_ids[0]);

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindImageTexturesTest::FunctionalBindImageTexturesTest(deqp::Context& context)
	: TestCase(context, "functional_bind_image_textures", "Verifies that BindImageTextures works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindImageTexturesTest::iterate()
{
	static const GLuint depth  = 6;
	static const GLuint height = 6;
	static const GLuint width  = 6;

	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLuint invalid_id   = 1; /* Start with 1, as 0 is not valid name */
	GLint  max_textures = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	Buffer				 buffer;
	std::vector<Texture> texture;
	std::vector<GLuint>  texture_ids;
	std::vector<GLuint>  t_texture_ids;

	texture.resize(max_textures);
	texture_ids.resize(max_textures);
	t_texture_ids.resize(max_textures);

	/* Prepare buffer */
	buffer.InitData(m_context, GL_TEXTURE_BUFFER, GL_DYNAMIC_COPY, 16 /* size */, 0 /* data */);

	/* Prepare textures */
	for (GLint i = 0; i < (GLint)s_n_texture_tragets; ++i)
	{
		const GLenum target = s_texture_infos[i].m_target;

		if (i >= max_textures)
		{
			break;
		}

		if (GL_TEXTURE_BUFFER != target)
		{
			texture[i].InitStorage(m_context, target, 1, GL_RGBA8, width, height, depth);
		}
		else
		{
			texture[i].InitBuffer(m_context, GL_RGBA8, buffer.m_id);
		}

		/* Unbind */
		Texture::Bind(gl, 0, target);
	}

	for (GLint i = (GLint)s_n_texture_tragets; i < max_textures; ++i)
	{
		texture[i].InitStorage(m_context, GL_TEXTURE_2D, 1, GL_RGBA8, width, height, depth);
	}

	/* Unbind */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	for (GLint i = 0; i < max_textures; ++i)
	{
		texture_ids[i] = texture[i].m_id;
	}

	/*
	 * - execute BindImageTextures to bind all images;
	 * - inspect bindings to verify that proper images were set;
	 */
	gl.bindImageTextures(0, max_textures, &texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTextures");

	for (GLint i = 0; i < max_textures; ++i)
	{
		checkBinding(m_context, GL_IMAGE_BINDING_NAME, i, "Image unit", texture_ids[i]);
	}

	/*
	 * - execute BindTextures for the first half of units with <textures> filled
	 * with zeros, to unbind those units;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * unbound;
	 */
	GLint half_index = max_textures / 2;

	for (GLint i = 0; i < max_textures; ++i)
	{
		t_texture_ids[i] = 0;
	}

	gl.bindImageTextures(0, half_index, &t_texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTextures");

	for (GLint i = 0; i < half_index; ++i)
	{
		checkBinding(m_context, GL_IMAGE_BINDING_NAME, i, "Image unit", 0);
	}

	for (GLint i = half_index; i < max_textures; ++i)
	{
		checkBinding(m_context, GL_IMAGE_BINDING_NAME, i, "Image unit", texture_ids[i]);
	}

	/*
	 * - execute BindTextures for the second half of units with NULL as<textures>,
	 * to unbind those units;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * unbound;
	 */
	gl.bindImageTextures(half_index, max_textures - half_index, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTextures");

	for (GLint i = 0; i < max_textures; ++i)
	{
		checkBinding(m_context, GL_IMAGE_BINDING_NAME, i, "Image unit", 0);
	}

	/*
	 * - modify <textures> so first entry is invalid;
	 * - execute BindTextures to bind all textures; It is expected that
	 * INVALID_OPERATION will be generated;
	 * - inspect bindings of all texture units to verify that proper bindings were
	 * set;
	 */

	/* Find invalid id */
	while (1)
	{
		if (GL_TRUE != gl.isTexture(invalid_id))
		{
			break;
		}

		invalid_id += 1;
	}

	/* Set invalid id */
	texture_ids[0] = invalid_id;

	gl.bindImageTextures(0, max_textures, &texture_ids[0]);
	CHECK_ERROR(GL_INVALID_OPERATION, "BindImageTextures with invalid texture id");

	checkBinding(m_context, GL_IMAGE_BINDING_NAME, 0, "Image unit", 0);
	for (GLint i = 1; i < max_textures; ++i)
	{
		checkBinding(m_context, GL_IMAGE_BINDING_NAME, i, "Image unit", texture_ids[i]);
	}

	/* - unbind all textures. */
	gl.bindImageTextures(0, max_textures, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTextures");

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
FunctionalBindVertexBuffersTest::FunctionalBindVertexBuffersTest(deqp::Context& context)
	: TestCase(context, "functional_bind_vertex_buffers", "Verifies that BindVertexBuffers works as expected")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult FunctionalBindVertexBuffersTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	static const GLintptr buffer_size = 16;
	static const GLintptr offset	  = 4;
	static const GLsizei  stride	  = 4;

	GLuint invalid_id  = 1; /* Start with 1, as 0 is not valid name */
	GLint  max_buffers = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	std::vector<Buffer>   buffer;
	std::vector<GLuint>   buffer_ids;
	std::vector<GLintptr> offsets;
	std::vector<GLsizei>  strides;
	std::vector<GLuint>   t_buffer_ids;

	buffer.resize(max_buffers);
	buffer_ids.resize(max_buffers);
	offsets.resize(max_buffers);
	strides.resize(max_buffers);
	t_buffer_ids.resize(max_buffers);

	/* Prepare buffers */
	for (GLint i = 0; i < max_buffers; ++i)
	{
		buffer[i].InitData(m_context, GL_ARRAY_BUFFER, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

		buffer_ids[i]   = buffer[i].m_id;
		offsets[i]		= offset;
		strides[i]		= stride;
		t_buffer_ids[i] = 0;
	}

	GLuint vao = 0;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");
	try
	{
		gl.bindVertexArray(vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArrays");

		/* - execute BindVertexBuffers to bind all buffer;
		 * - inspect bindings to verify that proper buffers were set;
		 */
		gl.bindVertexBuffers(0, max_buffers, &buffer_ids[0], &offsets[0], &strides[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexBuffers");

		for (GLint i = 0; i < max_buffers; ++i)
		{
			checkVertexAttribBinding(m_context, i, buffer_ids[i]);
		}

		/* - execute BindVertexBuffers for first half of bindings with <buffers> filled
		 * with zeros, to unbind those buffers;
		 * - inspect bindings to verify that proper buffers were unbound;
		 */
		GLint half_index = max_buffers / 2;

		gl.bindVertexBuffers(0, half_index, &t_buffer_ids[0], &offsets[0], &strides[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexBuffers");

		for (GLint i = 0; i < half_index; ++i)
		{
			checkVertexAttribBinding(m_context, i, 0);
		}

		for (GLint i = half_index; i < max_buffers; ++i)
		{
			checkVertexAttribBinding(m_context, i, buffer_ids[i]);
		}

		/* - execute BindVertexBuffers for second half of bindings with NULL as
		 * <buffers>, to unbind those buffers;
		 * - inspect bindings to verify that proper buffers were unbound;
		 */
		gl.bindVertexBuffers(half_index, max_buffers - half_index, 0, &offsets[0], &strides[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexBuffers");

		for (GLint i = 0; i < max_buffers; ++i)
		{
			checkVertexAttribBinding(m_context, i, 0);
		}

		/* - modify <buffers> so first entry is invalid;
		 * - execute BindVertexBuffers to bind all buffers; It is expected that
		 * INVALID_OPERATION will be generated;
		 * - inspect bindings to verify that proper buffers were set;
		 */

		/* Find invalid id */
		while (1)
		{
			if (GL_TRUE != gl.isBuffer(invalid_id))
			{
				break;
			}

			invalid_id += 1;
		}

		buffer_ids[0] = invalid_id;
		gl.bindVertexBuffers(0, max_buffers, &buffer_ids[0], &offsets[0], &strides[0]);
		CHECK_ERROR(GL_INVALID_OPERATION, "BindVertexBuffers with invalid id");

		checkVertexAttribBinding(m_context, 0, 0);
		for (GLint i = 1; i < max_buffers; ++i)
		{
			checkVertexAttribBinding(m_context, i, buffer_ids[i]);
		}

		/* - unbind all buffers. */
		gl.bindVertexBuffers(0, max_buffers, 0, &offsets[0], &strides[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexBuffers");
	}
	catch (const std::exception&)
	{
		gl.deleteVertexArrays(1, &vao);

		TCU_FAIL("Unexpected error generated");
	}

	gl.deleteVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteVertexArrays");

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
DispatchBindBuffersBaseTest::DispatchBindBuffersBaseTest(deqp::Context& context)
	: TestCase(context, "dispatch_bind_buffers_base", "Tests BindBuffersBase with dispatch command")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DispatchBindBuffersBaseTest::iterate()
{
	static const GLchar* cs = "#version 440 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "UBO_LIST\n"
							  "layout (std140, binding = 0) buffer SSB {\n"
							  "    vec4 sum;\n"
							  "} ssb;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    ssb.sum = SUM_LIST;\n"
							  "}\n"
							  "\n";

	static const GLchar* ubo = "layout (std140, binding = XXX) uniform BXXX { vec4 data; } bXXX;";

	static const GLintptr buffer_size = 4 * sizeof(GLfloat);

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLint   max_buffers = 0;
	GLfloat sum[4]		= { 0.0f, 0.0f, 0.0f, 0.0f };

	/* Get max */
	gl.getIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &max_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* UBO */
	/* Storage */
	std::vector<Buffer> uni_buffer;
	std::vector<GLuint> uni_buffer_ids;

	uni_buffer.resize(max_buffers);
	uni_buffer_ids.resize(max_buffers);

	/* Prepare buffers */
	for (GLint i = 0; i < max_buffers; ++i)
	{
		const GLfloat data[4] = {
			(GLfloat)(i * 4 + 0), (GLfloat)(i * 4 + 1), (GLfloat)(i * 4 + 2), (GLfloat)(i * 4 + 3),
		};

		sum[0] += data[0];
		sum[1] += data[1];
		sum[2] += data[2];
		sum[3] += data[3];

		uni_buffer[i].InitData(m_context, GL_UNIFORM_BUFFER, GL_DYNAMIC_COPY, buffer_size, data);

		uni_buffer_ids[i] = uni_buffer[i].m_id;
	}

	gl.bindBuffersBase(GL_UNIFORM_BUFFER, 0 /* first */, max_buffers /* count */, &uni_buffer_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersBase");

	/* SSBO */
	Buffer ssb_buffer;
	ssb_buffer.InitData(m_context, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, buffer_size, 0 /* data */);

	ssb_buffer.BindBase(0);

	/* Prepare program */
	size_t		ubo_position = 0;
	size_t		sum_position = 0;
	std::string cs_source	= cs;
	for (GLint i = 0; i < max_buffers; ++i)
	{
		size_t ubo_start_position = ubo_position;
		size_t sum_start_position = sum_position;

		GLchar index[16];

		sprintf(index, "%d", i);

		/* Add entry to ubo list */
		replaceToken("UBO_LIST", ubo_position, "UBO\nUBO_LIST", cs_source);
		ubo_position = ubo_start_position;

		replaceToken("UBO", ubo_position, ubo, cs_source);
		ubo_position = ubo_start_position;

		replaceToken("XXX", ubo_position, index, cs_source);
		replaceToken("XXX", ubo_position, index, cs_source);
		replaceToken("XXX", ubo_position, index, cs_source);

		/* Add entry to sum list */
		replaceToken("SUM_LIST", sum_position, "bXXX.data + SUM_LIST", cs_source);
		sum_position = sum_start_position;

		replaceToken("XXX", sum_position, index, cs_source);
	}

	/* Remove token for lists */
	replaceToken(" + SUM_LIST", sum_position, "", cs_source);
	replaceToken("UBO_LIST", ubo_position, "", cs_source);

	Program program(m_context);
	program.Init(cs_source.c_str(), "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	program.Use();

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	GLfloat* result = (GLfloat*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 != memcmp(result, sum, 4 * sizeof(GLfloat)))
	{
		test_result = false;
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.getError(); /* Ignore error */

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

/** Constructor
 *
 * @param context Test context
 **/
DispatchBindBuffersRangeTest::DispatchBindBuffersRangeTest(deqp::Context& context)
	: TestCase(context, "dispatch_bind_buffers_range", "Tests BindBuffersRange with dispatch command")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DispatchBindBuffersRangeTest::iterate()
{
	static const GLchar* cs = "#version 440 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "layout (std140, binding = 0) uniform B0 { int data; } b0;"
							  "layout (std140, binding = 1) uniform B1 { int data; } b1;"
							  "layout (std140, binding = 2) uniform B2 { int data; } b2;"
							  "layout (std140, binding = 3) uniform B3 { int data; } b3;"
							  "\n"
							  "layout (std140, binding = 0) buffer SSB {\n"
							  "    int sum;\n"
							  "} ssb;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    //ssb.sum = b1.data;// + b1.data + b2.data + b3.data;\n"
							  "    ssb.sum = b0.data + b1.data + b2.data + b3.data;\n"
							  "}\n"
							  "\n";

	static const GLint  data[]	= { 0x00010001, 0x01000100 };
	static const size_t n_buffers = 4;
	static const GLint  sum		  = 0x02020202;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	/* UBO */
	GLint offset_alignment = 0;

	gl.getIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &offset_alignment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	Buffer				 uni_buffer;
	GLuint				 uni_buffer_ids[n_buffers];
	std::vector<GLubyte> uni_data;
	GLintptr			 uni_offsets[n_buffers];
	GLintptr			 uni_sizes[n_buffers];

	const size_t buffer_size = (n_buffers - 1) * offset_alignment + sizeof(GLint);
	uni_data.resize(buffer_size);

	for (size_t i = 0; i < buffer_size; ++i)
	{
		uni_data[i] = 0xaa;
	}

	for (size_t i = 0; i < n_buffers; ++i)
	{
		void*		dst = &uni_data[i * offset_alignment];
		const void* src = &data[(i % 2)];

		memcpy(dst, src, sizeof(GLint));
	}

	uni_buffer.InitData(m_context, GL_UNIFORM_BUFFER, GL_DYNAMIC_COPY, buffer_size, &uni_data[0]);

	for (size_t i = 0; i < n_buffers; ++i)
	{
		uni_buffer_ids[i] = uni_buffer.m_id;
		uni_offsets[i]	= i * offset_alignment;
		uni_sizes[i]	  = sizeof(GLint);
	}

	gl.bindBuffersRange(GL_UNIFORM_BUFFER, 0 /* first */, n_buffers /* count */, &uni_buffer_ids[0], &uni_offsets[0],
						&uni_sizes[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffersRange");

	/* SSBO */
	Buffer ssb_buffer;
	ssb_buffer.InitData(m_context, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(GLint), 0 /* data */);

	ssb_buffer.BindBase(0);

	/* Prepare program */
	Program program(m_context);
	program.Init(cs, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	program.Use();

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	GLint* result = (GLint*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 != memcmp(result, &sum, sizeof(sum)))
	{
		test_result = false;
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.getError(); /* Ignore error */

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

/** Constructor
 *
 * @param context Test context
 **/
DispatchBindTexturesTest::DispatchBindTexturesTest(deqp::Context& context)
	: TestCase(context, "dispatch_bind_textures", "Tests BindTextures with dispatch command")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DispatchBindTexturesTest::iterate()
{
	static const GLchar* cs = "#version 440 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "SAMPLER_LIST\n"
							  "layout (std140, binding = 0) buffer SSB {\n"
							  "    uint sum;\n"
							  "} ssb;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    uvec4 sum = SUM_LIST;\n"
							  "    ssb.sum = sum.r\n;"
							  "}\n"
							  "\n";

	static const GLchar* sampler = "layout (location = XXX) uniform SAMPLER sXXX;";

	static const GLchar* sampling[] = {
		"texture(sXXX, COORDS)", "texture(sXXX, COORDS)",		"texture(sXXX, COORDS)",	  "texture(sXXX, COORDS)",
		"texture(sXXX, COORDS)", "texelFetch(sXXX, COORDS)",	"texture(sXXX, COORDS)",	  "texture(sXXX, COORDS)",
		"texture(sXXX, COORDS)", "texelFetch(sXXX, COORDS, 0)", "texelFetch(sXXX, COORDS, 0)"
	};

	static const GLchar* samplers[] = { "usampler1D",	 "usampler1DArray", "usampler2D",		 "usampler2DArray",
										"usampler3D",	 "usamplerBuffer",  "usamplerCube",	 "usamplerCubeArray",
										"usampler2DRect", "usampler2DMS",	"usampler2DMSArray" };

	static const GLchar* coordinates[] = {
		"0.5f",
		"vec2(0.5f, 0.0f)",
		"vec2(0.5f, 0.5f)",
		"vec3(0.5f, 0.5f, 0.0f)",
		"vec3(0.5f, 0.5f, 0.5f)",
		"0",
		"vec3(0.5f, 0.5f, 0.5f)",
		"vec4(0.5f, 0.5f, 0.5f, 0.0f)",
		"vec2(0.5f, 0.5f)",
		"ivec2(0, 0)",
		"ivec3(0, 0, 0)",
	};

	static const GLuint depth  = 6;
	static const GLuint height = 6;
	static const GLuint width  = 6;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLint  max_textures		 = 0;
	GLint  max_image_samples = 0;
	GLuint sum				 = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Check if load/store from multisampled images is supported */
	gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Textures */
	/* Storage */
	std::vector<Texture> texture;
	std::vector<GLuint>  texture_ids;
	Buffer				 texture_buffer;

	texture.resize(max_textures);
	texture_ids.resize(max_textures);

	/* Prepare */
	for (GLint i = 0; i < max_textures; ++i)
	{
		GLenum target = getTarget(i);
		if (target >= GL_TEXTURE_2D_MULTISAMPLE && max_image_samples == 0)
			target = GL_TEXTURE_2D;

		GLuint data[width * height * depth];

		for (GLuint j = 0; j < width * height * depth; ++j)
		{
			data[j] = i;
		}

		sum += i;

		bool is_array = false;

		switch (target)
		{
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			is_array = true;
		/* Intended pass-through */

		case GL_TEXTURE_2D_MULTISAMPLE:
			texture[i].InitStorage(m_context, target, 1, GL_R32UI, width, height, depth);
			fillMSTexture(m_context, texture[i].m_id, i, is_array);
			break;

		case GL_TEXTURE_BUFFER:
			texture_buffer.InitData(m_context, GL_TEXTURE_BUFFER, GL_DYNAMIC_COPY, sizeof(data), data);
			texture[i].InitBuffer(m_context, GL_R32UI, texture_buffer.m_id);
			break;

		default:
			texture[i].InitStorage(m_context, target, 1, GL_R32UI, width, height, depth);
			Texture::Bind(gl, texture[i].m_id, target);
			Texture::SubImage(gl, target, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth,
							  GL_RED_INTEGER, GL_UNSIGNED_INT, &data);
			gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			break;
		}

		/* Clean */
		Texture::Bind(gl, 0, target);

		texture_ids[i] = texture[i].m_id;
	}

	gl.bindTextures(0 /* first */, max_textures /* count */, &texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	/* SSBO */
	Buffer ssb_buffer;
	ssb_buffer.InitData(m_context, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(GLuint), 0 /* data */);

	ssb_buffer.BindBase(0);

	/* Prepare program */
	size_t		sam_position	 = 0;
	size_t		sum_position	 = 0;
	std::string cs_source		 = cs;
	GLint		max_target_index = (GLint)(max_image_samples > 0 ? s_n_texture_tragets : s_n_texture_tragets - 2);
	for (GLint i = 0; i < max_textures; ++i)
	{
		size_t sam_start_position = sam_position;
		size_t sum_start_position = sum_position;

		GLchar index[16];

		sprintf(index, "%d", i);

		const GLchar* coords		= 0;
		const GLchar* sampler_type  = 0;
		const GLchar* sampling_code = 0;

		if (i < max_target_index)
		{
			coords		  = coordinates[i];
			sampler_type  = samplers[i];
			sampling_code = sampling[i];
		}
		else
		{
			coords		  = coordinates[2]; /* vec2(0.5f, 0.5f) */
			sampler_type  = samplers[2];	/* usampler2D */
			sampling_code = sampling[2];	/* texture(sXXX, COORDS) */
		}

		/* Add entry to ubo list */
		replaceToken("SAMPLER_LIST", sam_position, "SAMPLER\nSAMPLER_LIST", cs_source);
		sam_position = sam_start_position;

		replaceToken("SAMPLER", sam_position, sampler, cs_source);
		sam_position = sam_start_position;

		replaceToken("XXX", sam_position, index, cs_source);
		replaceToken("SAMPLER", sam_position, sampler_type, cs_source);
		replaceToken("XXX", sam_position, index, cs_source);

		/* Add entry to sum list */
		replaceToken("SUM_LIST", sum_position, "SAMPLING + SUM_LIST", cs_source);
		sum_position = sum_start_position;

		replaceToken("SAMPLING", sum_position, sampling_code, cs_source);
		sum_position = sum_start_position;

		replaceToken("XXX", sum_position, index, cs_source);
		replaceToken("COORDS", sum_position, coords, cs_source);
	}

	/* Remove token for lists */
	replaceToken(" + SUM_LIST", sum_position, "", cs_source);
	replaceToken("SAMPLER_LIST", sam_position, "", cs_source);

	Program program(m_context);
	program.Init(cs_source.c_str(), "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	program.Use();

	/* Set samplers */
	for (GLint i = 0; i < max_textures; ++i)
	{
		gl.uniform1i(i, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
	}

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	GLuint* result = (GLuint*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 != memcmp(result, &sum, sizeof(sum)))
	{
		test_result = false;
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.getError(); /* Ignore error */

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

/** Constructor
 *
 * @param context Test context
 **/
DispatchBindImageTexturesTest::DispatchBindImageTexturesTest(deqp::Context& context)
	: TestCase(context, "dispatch_bind_image_textures", "Tests BindImageTextures with dispatch command")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DispatchBindImageTexturesTest::iterate()
{
	static const GLchar* cs = "#version 440 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "IMAGE_LIST\n"
							  "layout (std140, binding = 0) buffer SSB {\n"
							  "    uint sum;\n"
							  "} ssb;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    uvec4 sum = SUM_LIST;\n"
							  "    ssb.sum = sum.r\n;"
							  "}\n"
							  "\n";

	static const GLchar* image = "layout (location = XXX, r32ui) readonly uniform IMAGE iXXX;";

	static const GLchar* loading[] = {
		"imageLoad(iXXX, COORDS)", "imageLoad(iXXX, COORDS)",	"imageLoad(iXXX, COORDS)",   "imageLoad(iXXX, COORDS)",
		"imageLoad(iXXX, COORDS)", "imageLoad(iXXX, COORDS)",	"imageLoad(iXXX, COORDS)",   "imageLoad(iXXX, COORDS)",
		"imageLoad(iXXX, COORDS)", "imageLoad(iXXX, COORDS, 0)", "imageLoad(iXXX, COORDS, 0)"
	};

	static const GLchar* images[] = { "uimage1D",	 "uimage1DArray", "uimage2D",		 "uimage2DArray",
									  "uimage3D",	 "uimageBuffer",  "uimageCube",	 "uimageCubeArray",
									  "uimage2DRect", "uimage2DMS",	"uimage2DMSArray" };

	static const GLchar* coordinates[] = {
		"0",
		"ivec2(0, 0)",
		"ivec2(0, 0)",
		"ivec3(0, 0, 0)",
		"ivec3(0, 0, 0)",
		"0",
		"ivec3(0, 0, 0)",
		"ivec3(0, 0, 0)",
		"ivec2(0, 0)",
		"ivec2(0, 0)",
		"ivec3(0, 0, 0)",
	};

	static const GLuint depth  = 6;
	static const GLuint height = 6;
	static const GLuint width  = 6;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLint  max_textures		 = 0;
	GLint  max_image_samples = 0;
	GLuint sum				 = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Check if load/store from multisampled images is supported */
	gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Textures */
	/* Storage */
	std::vector<Texture> texture;
	std::vector<GLuint>  texture_ids;
	Buffer				 texture_buffer;

	texture.resize(max_textures);
	texture_ids.resize(max_textures);

	/* Prepare */
	for (GLint i = 0; i < max_textures; ++i)
	{
		GLenum target = getTarget(i);
		if (target >= GL_TEXTURE_2D_MULTISAMPLE && max_image_samples == 0)
			target = GL_TEXTURE_2D;

		GLuint data[width * height * depth];

		for (GLuint j = 0; j < width * height * depth; ++j)
		{
			data[j] = i;
		}

		sum += i;

		bool is_array = false;

		switch (target)
		{
		case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
			is_array = true;
		/* Intended pass-through */

		case GL_TEXTURE_2D_MULTISAMPLE:
			texture[i].InitStorage(m_context, target, 1, GL_R32UI, width, height, depth);
			fillMSTexture(m_context, texture[i].m_id, i, is_array);
			break;

		case GL_TEXTURE_BUFFER:
			texture_buffer.InitData(m_context, GL_TEXTURE_BUFFER, GL_DYNAMIC_COPY, sizeof(data), data);
			texture[i].InitBuffer(m_context, GL_R32UI, texture_buffer.m_id);
			break;

		default:
			texture[i].InitStorage(m_context, target, 1, GL_R32UI, width, height, depth);
			Texture::Bind(gl, texture[i].m_id, target);
			Texture::SubImage(gl, target, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth,
							  GL_RED_INTEGER, GL_UNSIGNED_INT, &data);
			break;
		}

		/* Clean */
		Texture::Bind(gl, 0, target);

		texture_ids[i] = texture[i].m_id;
	}

	gl.bindImageTextures(0 /* first */, max_textures /* count */, &texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindImageTextures");

	/* SSBO */
	Buffer ssb_buffer;
	ssb_buffer.InitData(m_context, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(GLuint), 0 /* data */);

	ssb_buffer.BindBase(0);

	/* Prepare program */
	size_t		load_position	= 0;
	size_t		sum_position	 = 0;
	std::string cs_source		 = cs;
	GLint		max_target_index = (GLint)(max_image_samples > 0 ? s_n_texture_tragets : s_n_texture_tragets - 2);
	for (GLint i = 0; i < max_textures; ++i)
	{
		size_t load_start_position = load_position;
		size_t sum_start_position  = sum_position;

		GLchar index[16];

		sprintf(index, "%d", i);

		const GLchar* coords	   = 0;
		const GLchar* image_type   = 0;
		const GLchar* loading_code = 0;

		if (i < max_target_index)
		{
			coords		 = coordinates[i];
			image_type   = images[i];
			loading_code = loading[i];
		}
		else
		{
			coords		 = coordinates[2]; /* vec2(0.5f, 0.5f) */
			image_type   = images[2];	  /* usampler2D */
			loading_code = loading[2];	 /* texture(sXXX, COORDS) */
		}

		/* Add entry to ubo list */
		replaceToken("IMAGE_LIST", load_position, "IMAGE\nIMAGE_LIST", cs_source);
		load_position = load_start_position;

		replaceToken("IMAGE", load_position, image, cs_source);
		load_position = load_start_position;

		replaceToken("XXX", load_position, index, cs_source);
		replaceToken("IMAGE", load_position, image_type, cs_source);
		replaceToken("XXX", load_position, index, cs_source);

		/* Add entry to sum list */
		replaceToken("SUM_LIST", sum_position, "LOADING + SUM_LIST", cs_source);
		sum_position = sum_start_position;

		replaceToken("LOADING", sum_position, loading_code, cs_source);
		sum_position = sum_start_position;

		replaceToken("XXX", sum_position, index, cs_source);
		replaceToken("COORDS", sum_position, coords, cs_source);
	}

	/* Remove token for lists */
	replaceToken(" + SUM_LIST", sum_position, "", cs_source);
	replaceToken("IMAGE_LIST", load_position, "", cs_source);

	Program program(m_context);
	program.Init(cs_source.c_str(), "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	program.Use();

	/* Set images */
	for (GLint i = 0; i < max_textures; ++i)
	{
		gl.uniform1i(i, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
	}

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	gl.memoryBarrier(GL_ALL_BARRIER_BITS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

	GLuint* result = (GLuint*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 != memcmp(result, &sum, sizeof(sum)))
	{
		test_result = false;
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.getError(); /* Ignore error */

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

/** Constructor
 *
 * @param context Test context
 **/
DispatchBindSamplersTest::DispatchBindSamplersTest(deqp::Context& context)
	: TestCase(context, "dispatch_bind_samplers", "Tests BindSamplers with dispatch command")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DispatchBindSamplersTest::iterate()
{
	static const GLchar* cs = "#version 440 core\n"
							  "\n"
							  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							  "\n"
							  "SAMPLER_LIST\n"
							  "layout (std140, binding = 0) buffer SSB {\n"
							  "    uint sum;\n"
							  "} ssb;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    uvec4 sum = SUM_LIST;\n"
							  "    ssb.sum = sum.r\n;"
							  "}\n"
							  "\n";

	static const GLchar* sampler = "layout (location = XXX) uniform usampler2D sXXX;";

	static const GLchar* sampling = "texture(sXXX, vec2(1.5f, 0.5f))";

	static const GLuint depth  = 1;
	static const GLuint height = 8;
	static const GLuint width  = 8;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	GLint max_textures = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &max_textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Textures */
	/* Storage */
	std::vector<GLuint>  sampler_ids;
	std::vector<Texture> texture;
	std::vector<GLuint>  texture_ids;

	sampler_ids.resize(max_textures);
	texture.resize(max_textures);
	texture_ids.resize(max_textures);

	GLuint data[width * height * depth];

	for (GLuint j = 0; j < width * height; ++j)
	{
		data[j] = 0;
	}

	{
		const size_t last_line_offset		   = (height - 1) * width;
		const size_t last_pixel_in_line_offset = width - 1;

		for (GLuint j = 0; j < width; ++j)
		{
			data[j]					   = 1;
			data[j + last_line_offset] = 1;
		}

		for (GLuint j = 0; j < height; ++j)
		{
			const size_t line_offset = j * width;

			data[line_offset]							  = 1;
			data[line_offset + last_pixel_in_line_offset] = 1;
		}
	}

	/* Prepare */
	for (GLint i = 0; i < max_textures; ++i)
	{
		texture[i].InitStorage(m_context, GL_TEXTURE_2D, 1, GL_R32UI, width, height, depth);
		Texture::Bind(gl, texture[i].m_id, GL_TEXTURE_2D);
		Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, depth,
						  GL_RED_INTEGER, GL_UNSIGNED_INT, &data);

		texture_ids[i] = texture[i].m_id;
	}

	/* Clean */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	/* Execute the test */
	gl.bindTextures(0 /* first */, max_textures /* count */, &texture_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindTextures");

	/* SSBO */
	Buffer ssb_buffer;
	ssb_buffer.InitData(m_context, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, sizeof(GLuint), 0 /* data */);

	ssb_buffer.BindBase(0);

	/* Prepare program */
	size_t		sam_position = 0;
	size_t		sum_position = 0;
	std::string cs_source	= cs;

	for (GLint i = 0; i < max_textures; ++i)
	{
		size_t sam_start_position = sam_position;
		size_t sum_start_position = sum_position;

		GLchar index[16];

		sprintf(index, "%d", i);

		/* Add entry to ubo list */
		replaceToken("SAMPLER_LIST", sam_position, "SAMPLER\nSAMPLER_LIST", cs_source);
		sam_position = sam_start_position;

		replaceToken("SAMPLER", sam_position, sampler, cs_source);
		sam_position = sam_start_position;

		replaceToken("XXX", sam_position, index, cs_source);
		replaceToken("XXX", sam_position, index, cs_source);

		/* Add entry to sum list */
		replaceToken("SUM_LIST", sum_position, "SAMPLING + SUM_LIST", cs_source);
		sum_position = sum_start_position;

		replaceToken("SAMPLING", sum_position, sampling, cs_source);
		sum_position = sum_start_position;

		replaceToken("XXX", sum_position, index, cs_source);
	}

	/* Remove token for lists */
	replaceToken(" + SUM_LIST", sum_position, "", cs_source);
	replaceToken("SAMPLER_LIST", sam_position, "", cs_source);

	Program program(m_context);
	program.Init(cs_source.c_str(), "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	program.Use();

	/* Set texture units */
	for (GLint i = 0; i < max_textures; ++i)
	{
		gl.uniform1i(i, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform1i");
	}

	/* Prepare samplers */
	gl.genSamplers(max_textures, &sampler_ids[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenSamplers");

	try
	{
		gl.bindSamplers(0 /* first */, max_textures /* count */, &sampler_ids[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindSamplers");

		for (GLint i = 0; i < max_textures; ++i)
		{
			gl.samplerParameteri(sampler_ids[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			gl.samplerParameteri(sampler_ids[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			gl.samplerParameteri(sampler_ids[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			GLU_EXPECT_NO_ERROR(gl.getError(), "SamplerParameteri");
		}

		gl.dispatchCompute(1, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");
	}
	catch (const std::exception&)
	{
		gl.deleteSamplers(max_textures, &sampler_ids[0]);

		TCU_FAIL("Unexpected error generated");
	}

	/* Remove samplers */
	gl.deleteSamplers(max_textures, &sampler_ids[0]);

	/* Verify results */
	GLuint* result = (GLuint*)gl.mapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	if (0 != memcmp(result, &max_textures, sizeof(max_textures)))
	{
		test_result = false;
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	gl.getError(); /* Ignore error */

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

/** Constructor
 *
 * @param context Test context
 **/
DrawBindVertexBuffersTest::DrawBindVertexBuffersTest(deqp::Context& context)
	: TestCase(context, "draw_bind_vertex_buffers", "Tests BindVertexBuffers command with drawArrays")
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult DrawBindVertexBuffersTest::iterate()
{
	static const GLchar* vs = "#version 440 core\n"
							  "\n"
							  "ATTRIBUTE_LIST\n"
							  "\n"
							  "out vec4 vs_gs_sum;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    vs_gs_sum = SUM_LIST;\n"
							  "}\n"
							  "\n";

	static const GLchar* gs = "#version 440 core\n"
							  "\n"
							  "layout(points)                           in;\n"
							  "layout(triangle_strip, max_vertices = 4) out;\n"
							  "\n"
							  "in  vec4 vs_gs_sum[];\n"
							  "out vec4 gs_fs_sum;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gs_fs_sum   = vs_gs_sum[0];\n"
							  "    gl_Position = vec4(-1, -1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gs_fs_sum   = vs_gs_sum[0];\n"
							  "    gl_Position = vec4(-1, 1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gs_fs_sum   = vs_gs_sum[0];\n"
							  "    gl_Position = vec4(1, -1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gs_fs_sum   = vs_gs_sum[0];\n"
							  "    gl_Position = vec4(1, 1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "}\n"
							  "\n";

	static const GLchar* fs = "#version 440 core\n"
							  "\n"
							  "in  vec4 gs_fs_sum;\n"
							  "out vec4 fs_out;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    fs_out = gs_fs_sum;\n"
							  "}\n"
							  "\n";

	static const GLchar* attribute = "layout (location = XXX) in vec4 aXXX;";

	static const GLuint height = 8;
	static const GLuint width  = 8;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

#if DEBUG_ENBALE_MESSAGE_CALLBACK
	gl.debugMessageCallback(debug_proc, &m_context);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DebugMessageCallback");
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

	static const GLintptr attribute_size = 4 * sizeof(GLfloat);

	GLint  max_buffers = 0;
	GLuint vao		   = 0;

	/* Get max */
	gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	/* Storage */
	Buffer				  buffer;
	std::vector<GLuint>   buffer_ids;
	std::vector<GLfloat>  data;
	std::vector<GLintptr> offsets;
	std::vector<GLsizei>  strides;

	buffer_ids.resize(max_buffers);
	data.resize(max_buffers * 4);
	offsets.resize(max_buffers);
	strides.resize(max_buffers);

	/* Prepare data */
	const GLfloat value = 1.0f / (GLfloat)max_buffers;

	for (GLint i = 0; i < max_buffers; ++i)
	{
		data[i * 4 + 0] = value;
		data[i * 4 + 1] = value;
		data[i * 4 + 2] = value;
		data[i * 4 + 3] = value;
	}

	/* Prepare buffer */
	buffer.InitData(m_context, GL_ARRAY_BUFFER, GL_DYNAMIC_COPY, data.size() * sizeof(GLfloat), &data[0]);

	for (GLint i = 0; i < max_buffers; ++i)
	{
		buffer_ids[i] = buffer.m_id;
		offsets[i]	= i * attribute_size;
		strides[i]	= attribute_size;
	}

	/* Prepare FBO */
	Framebuffer framebuffer(m_context);
	Texture		texture;

	texture.InitStorage(m_context, GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, width, height, 1 /* depth */);

	/* */
	Framebuffer::Generate(gl, framebuffer.m_id);
	Framebuffer::Bind(gl, GL_DRAW_FRAMEBUFFER, framebuffer.m_id);
	Framebuffer::AttachTexture(gl, GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.m_id, 0 /* level */, width,
							   height);

	/* Prepare program */
	size_t		attr_position = 0;
	size_t		sum_position  = 0;
	std::string vs_source	 = vs;
	for (GLint i = 0; i < max_buffers; ++i)
	{
		size_t attr_start_position = attr_position;
		size_t sum_start_position  = sum_position;

		GLchar index[16];

		sprintf(index, "%d", i);

		/* Add entry to ubo list */
		replaceToken("ATTRIBUTE_LIST", attr_position, "ATTRIBUTE\nATTRIBUTE_LIST", vs_source);
		attr_position = attr_start_position;

		replaceToken("ATTRIBUTE", attr_position, attribute, vs_source);
		attr_position = attr_start_position;

		replaceToken("XXX", attr_position, index, vs_source);
		replaceToken("XXX", attr_position, index, vs_source);

		/* Add entry to sum list */
		replaceToken("SUM_LIST", sum_position, "aXXX + SUM_LIST", vs_source);
		sum_position = sum_start_position;

		replaceToken("XXX", sum_position, index, vs_source);
	}

	/* Remove token for lists */
	replaceToken(" + SUM_LIST", sum_position, "", vs_source);
	replaceToken("ATTRIBUTE_LIST", attr_position, "", vs_source);

	Program program(m_context);
	program.Init("" /* cs */, fs, gs, "" /* tcs */, "" /* tes */, vs_source.c_str());

	program.Use();

	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

	try
	{
		gl.bindVertexArray(vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArrays");

		for (GLint i = 0; i < max_buffers; ++i)
		{
			gl.enableVertexAttribArray(i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");
		}

		/* */
		gl.bindVertexBuffers(0, max_buffers, &buffer_ids[0], &offsets[0], &strides[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexBuffers");

		/* */
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

		for (GLint i = 0; i < max_buffers; ++i)
		{
			gl.disableVertexAttribArray(i);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DisableVertexAttribArray");
		}
	}
	catch (const std::exception&)
	{
		gl.deleteVertexArrays(1, &vao);

		TCU_FAIL("Unexpected error generated");
	}

	gl.deleteVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteVertexArrays");

	/* Verify results */
	GLuint pixels[width * height];
	for (GLuint i = 0; i < width * height; ++i)
	{
		pixels[i] = 0;
	}

	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);

	Texture::GetData(gl, 0 /* level */, GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	/* Unbind */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	/* Verify */
	for (GLuint i = 0; i < width * height; ++i)
	{
		if (0xffffffff != pixels[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid value: " << (GLuint)pixels[i]
												<< " at offset: " << i << tcu::TestLog::EndMessage;

			test_result = false;

			break;
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
} /* MultiBind */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
MultiBindTests::MultiBindTests(deqp::Context& context)
	: TestCaseGroup(context, "multi_bind", "Verifies \"multi bind\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a multi_bind test group.
 *
 **/
void MultiBindTests::init(void)
{
	addChild(new MultiBind::DispatchBindTexturesTest(m_context));

	addChild(new MultiBind::ErrorsBindBuffersTest(m_context));
	addChild(new MultiBind::ErrorsBindTexturesTest(m_context));
	addChild(new MultiBind::ErrorsBindSamplersTest(m_context));
	addChild(new MultiBind::ErrorsBindImageTexturesTest(m_context));
	addChild(new MultiBind::ErrorsBindVertexBuffersTest(m_context));
	addChild(new MultiBind::FunctionalBindBuffersBaseTest(m_context));
	addChild(new MultiBind::FunctionalBindBuffersRangeTest(m_context));
	addChild(new MultiBind::FunctionalBindTexturesTest(m_context));
	addChild(new MultiBind::FunctionalBindSamplersTest(m_context));
	addChild(new MultiBind::FunctionalBindImageTexturesTest(m_context));
	addChild(new MultiBind::FunctionalBindVertexBuffersTest(m_context));
	addChild(new MultiBind::DispatchBindBuffersBaseTest(m_context));
	addChild(new MultiBind::DispatchBindBuffersRangeTest(m_context));

	addChild(new MultiBind::DispatchBindImageTexturesTest(m_context));
	addChild(new MultiBind::DispatchBindSamplersTest(m_context));
	addChild(new MultiBind::DrawBindVertexBuffersTest(m_context));
}

} /* gl4cts namespace */
