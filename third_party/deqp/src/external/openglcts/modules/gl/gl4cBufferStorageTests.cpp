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
 * \file  gl4cBufferStorageTests.cpp
 * \brief Implements conformance tests for "Buffer storage" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cBufferStorageTests.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageIO.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"

#include <iomanip>
#include <string>

using namespace glw;

namespace gl4cts
{
namespace BufferStorage
{
/* Enums */

/* Represents how functionality is supported */
enum FUNCTIONALITY_SUPPORT
{
	FUNCTIONALITY_SUPPORT_NONE = 0,
	FUNCTIONALITY_SUPPORT_EXTENSION,
	FUNCTIONALITY_SUPPORT_CORE,
	FUNCTIONALITY_SUPPORT_NOT_DETERMINED,
};

/* Prototypes of functions  */
FUNCTIONALITY_SUPPORT getDirectStateAccessSupport(deqp::Context& context);
bool isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor);

/* Classes */

/** Represents buffer instance
 * Provides basic buffer functionality
 **/
class Buffer
{
public:
	// I don't quite understand how the old code *ever* worked...
	// This is uglyish hack to make it actually compile on any sane
	// compiler, and not crash.
	struct MoveMapOwner
	{
		MoveMapOwner(Buffer* buffer_, glw::GLvoid* data_) : buffer(buffer_), data(data_)
		{
		}

		Buffer*		 buffer;
		glw::GLvoid* data;
	};

	/* Public classes */
	class MapOwner
	{
		friend class Buffer;

	public:
		MapOwner(MapOwner& map_owner);
		MapOwner(const MoveMapOwner& moveOwner);
		~MapOwner();

		glw::GLvoid* m_data;

	private:
		MapOwner(Buffer& buffer, glw::GLvoid* data);

		Buffer* m_buffer;
	};

	/* Public methods */
	/* Ctr & Dtr */
	Buffer(deqp::Context& context);
	~Buffer();

	/* Init & Release */
	void InitData(glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size, const glw::GLvoid* data);

	void InitStorage(glw::GLenum target, glw::GLenum flags, glw::GLsizeiptr size, const glw::GLvoid* data);

	void Release();

	/* Functionality */
	void Bind() const;
	void BindBase(glw::GLuint index) const;

	void BindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size) const;

	MoveMapOwner MapRange(glw::GLintptr offset, glw::GLsizeiptr length, glw::GLenum access);

	void UnMap();

	/* Public static routines */
	/* Extensions */
	static void LoadExtDirectStateAccess(deqp::Context& context);

	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);

	static void BindBase(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index);

	static void BindRange(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index,
						  glw::GLintptr offset, glw::GLsizeiptr size);

	static void Data(const glw::Functions& gl, glw::GLenum target, glw::GLenum usage, glw::GLsizeiptr size,
					 const glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void GetNamedParameter(const glw::Functions& gl, glw::GLuint buffer, glw::GLenum pname, glw::GLint* data);

	static void GetParameter(const glw::Functions& gl, glw::GLenum target, glw::GLenum value, glw::GLint* data);

	static void GetSubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
						   glw::GLvoid* data);

	static void* Map(const glw::Functions& gl, glw::GLenum target, glw::GLenum access);

	static void* MapRange(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr length,
						  glw::GLenum access);

	static void Storage(const glw::Functions& gl, glw::GLenum target, glw::GLenum flags, glw::GLsizeiptr size,
						const glw::GLvoid* data);

	static void SubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
						glw::GLvoid* data);

	static void UnMap(const glw::Functions& gl, glw::GLenum target);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;
	static const glw::GLuint m_n_targets = 13;
	static const glw::GLenum m_targets[m_n_targets];

private:
	/* Private enums */

	/* Private fields */
	deqp::Context& m_context;
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
							  glw::GLuint texture_id, glw::GLuint width, glw::GLuint height);

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
	/* Public types */
	/* Public methods */
	/* Ctr & Dtr */
	Texture(deqp::Context& context);
	~Texture();

	/* Init & Release */
	void Release();

	/* Public static routines */
	/* Extensions */
	static void LoadExtDirectStateAccess(deqp::Context& context);

	/* Functionality */
	static void Bind(const glw::Functions& gl, glw::GLuint id, glw::GLenum target);

	static void CompressedImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level,
								glw::GLenum internal_format, glw::GLuint width, glw::GLuint height, glw::GLuint depth,
								glw::GLsizei image_size, const glw::GLvoid* data);

	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	static void GetData(const glw::Functions& gl, glw::GLenum target, glw::GLenum format, glw::GLenum type,
						glw::GLvoid* out_data);

	static void GetLevelParameter(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum pname,
								  glw::GLint* param);

	static void Image(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLenum internal_format,
					  glw::GLuint width, glw::GLuint height, glw::GLuint depth, glw::GLenum format, glw::GLenum type,
					  const glw::GLvoid* data);

	static void Storage(const glw::Functions& gl, glw::GLenum target, glw::GLsizei levels, glw::GLenum internal_format,
						glw::GLuint width, glw::GLuint height, glw::GLuint depth);

	static void SubImage(const glw::Functions& gl, glw::GLenum target, glw::GLint level, glw::GLint x, glw::GLint y,
						 glw::GLint z, glw::GLsizei width, glw::GLsizei height, glw::GLsizei depth, glw::GLenum format,
						 glw::GLenum type, const glw::GLvoid* pixels);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;

	/* Private static fields */
	static FUNCTIONALITY_SUPPORT m_direct_state_access_support;
};

/** Represents Vertex array object
 * Provides basic functionality
 **/
class VertexArray
{
public:
	/* Public methods */
	/* Ctr & Dtr */
	VertexArray(deqp::Context& Context);
	~VertexArray();

	/* Init & Release */
	void Release();

	/* Public static methods */
	static void Bind(const glw::Functions& gl, glw::GLuint id);
	static void Generate(const glw::Functions& gl, glw::GLuint& out_id);

	/* Public fields */
	glw::GLuint m_id;

	/* Public constants */
	static const glw::GLuint m_invalid_id;

private:
	/* Private fields */
	deqp::Context& m_context;
};

/* Global variables */
static FUNCTIONALITY_SUPPORT m_direct_state_access_support = FUNCTIONALITY_SUPPORT_NOT_DETERMINED;

/* Implementations of functions */
/** Get support for direct state access
 *
 * @param context CTS context
 **/
FUNCTIONALITY_SUPPORT getDirectStateAccessSupport(deqp::Context& context)
{
	if (FUNCTIONALITY_SUPPORT_NOT_DETERMINED == m_direct_state_access_support)
	{
		const Functions& gl = context.getRenderContext().getFunctions();

		if (true == isGLVersionAtLeast(gl, 4, 5))
		{
			m_direct_state_access_support = FUNCTIONALITY_SUPPORT_CORE;
		}
		else
		{
			bool is_supported = context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

			if (true == is_supported)
			{
				m_direct_state_access_support = FUNCTIONALITY_SUPPORT_EXTENSION;
			}
			else
			{
				m_direct_state_access_support = FUNCTIONALITY_SUPPORT_NONE;
			}
		}
	}

	return m_direct_state_access_support;
}

/** Check if GL context meets version requirements
 *
 * @param gl             Functions
 * @param required_major Minimum required MAJOR_VERSION
 * @param required_minor Minimum required MINOR_VERSION
 *
 * @return true if GL context version is at least as requested, false otherwise
 **/
bool isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor)
{
	glw::GLint major = 0;
	glw::GLint minor = 0;

	gl.getIntegerv(GL_MAJOR_VERSION, &major);
	gl.getIntegerv(GL_MINOR_VERSION, &minor);

	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (major > required_major)
	{
		/* Major is higher than required one */
		return true;
	}
	else if (major == required_major)
	{
		if (minor >= required_minor)
		{
			/* Major is equal to required one */
			/* Minor is higher than or equal to required one */
			return true;
		}
		else
		{
			/* Major is equal to required one */
			/* Minor is lower than required one */
			return false;
		}
	}
	else
	{
		/* Major is lower than required one */
		return false;
	}
}

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
Buffer::Buffer(deqp::Context& context) : m_id(m_invalid_id), m_context(context), m_target(GL_ARRAY_BUFFER)
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

	const Functions& gl = m_context.getRenderContext().getFunctions();

	Generate(gl, m_id);
	Bind(gl, m_id, m_target);
	Data(gl, m_target, usage, size, data);
}

/** Initialize buffer instance
 *
 * @param target Buffer target
 * @param usage  Buffer usage enum
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::InitStorage(glw::GLenum target, glw::GLenum flags, glw::GLsizeiptr size, const glw::GLvoid* data)
{
	/* Delete previous buffer instance */
	Release();

	m_target = target;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	Generate(gl, m_id);
	Bind(gl, m_id, m_target);
	Storage(gl, m_target, flags, size, data);
}

/** Release buffer instance
 *
 **/
void Buffer::Release()
{
	if (m_invalid_id != m_id)
	{
		const Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteBuffers(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Binds buffer to its target
 *
 **/
void Buffer::Bind() const
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	Bind(gl, m_id, m_target);
}

/** Binds indexed buffer
 *
 * @param index <index> parameter
 **/
void Buffer::BindBase(glw::GLuint index) const
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	BindBase(gl, m_id, m_target, index);
}

/** Binds range of buffer
 *
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Buffer::BindRange(glw::GLuint index, glw::GLintptr offset, glw::GLsizeiptr size) const
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	BindRange(gl, m_id, m_target, index, offset, size);
}

/** Maps contents of buffer into CPU space
 *
 * @param access Requested access
 *
 * @return Pointer to memory region available for CPU
 **/
Buffer::MoveMapOwner Buffer::MapRange(glw::GLintptr offset, glw::GLsizeiptr length, glw::GLenum access)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	Bind(gl, m_id, m_target);

	void* data = MapRange(gl, m_target, offset, length, access);

	MoveMapOwner map(this, data);

	return map;
}

/** Unmaps contents of buffer
 *
 **/
void Buffer::UnMap()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	UnMap(gl, m_target);
}

/** Loads entry points for direct state access extension
 *
 * @param context CTS context
 **/
void Buffer::LoadExtDirectStateAccess(deqp::Context& context)
{
	FUNCTIONALITY_SUPPORT support = getDirectStateAccessSupport(context);

	switch (support)
	{
	case FUNCTIONALITY_SUPPORT_NONE:
		/* Nothing to be done */
		break;
	case FUNCTIONALITY_SUPPORT_CORE:
	case FUNCTIONALITY_SUPPORT_EXTENSION:
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}
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

/** Binds buffer range
 *
 * @param gl     GL functions
 * @param id     Id of buffer
 * @param target Buffer target
 * @param index  <index> parameter
 * @param offset <offset> parameter
 * @param size   <size> parameter
 **/
void Buffer::BindRange(const glw::Functions& gl, glw::GLuint id, glw::GLenum target, glw::GLuint index,
					   glw::GLintptr offset, glw::GLsizeiptr size)
{
	gl.bindBufferRange(target, index, id, offset, size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferRange");
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

/** Query parameter of named buffer
 *
 * @param gl     GL functions
 * @param buffer Buffer name
 * @param pname  Parameter name
 * @param data   Storage for queried results
 **/
void Buffer::GetNamedParameter(const glw::Functions& gl, glw::GLuint buffer, glw::GLenum pname, glw::GLint* data)
{
	gl.getNamedBufferParameteriv(buffer, pname, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetNameBufferParameteriv");
}

/** Query parameter of bound buffer
 *
 * @param gl     GL functions
 * @param Target Buffer target
 * @param pname  Parameter name
 * @param data   Storage for queried results
 **/
void Buffer::GetParameter(const glw::Functions& gl, glw::GLenum target, glw::GLenum value, glw::GLint* data)
{
	gl.getBufferParameteriv(target, value, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetBufferParameteriv");
}

/** Get contents of buffer's region
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param offset Offset in buffer
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::GetSubData(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr size,
						glw::GLvoid* data)
{
	gl.getBufferSubData(target, offset, size, data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetBufferSubData");
}

/** Maps buffer content
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param access Access rights for mapped region
 *
 * @return Mapped memory
 **/
void* Buffer::Map(const glw::Functions& gl, glw::GLenum target, glw::GLenum access)
{
	void* result = gl.mapBuffer(target, access);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBuffer");

	return result;
}

/** Maps buffer content
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param access Access rights for mapped region
 *
 * @return Mapped memory
 **/
void* Buffer::MapRange(const glw::Functions& gl, glw::GLenum target, glw::GLintptr offset, glw::GLsizeiptr length,
					   glw::GLenum access)
{
	void* result = gl.mapBufferRange(target, offset, length, access);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBufferRange");

	return result;
}

/** Allocate immutable memory for buffer and sends initial content
 *
 * @param gl     GL functions
 * @param target Buffer target
 * @param flags  Buffer flags
 * @param size   <size> parameter
 * @param data   <data> parameter
 **/
void Buffer::Storage(const glw::Functions& gl, glw::GLenum target, glw::GLenum flags, glw::GLsizeiptr size,
					 const glw::GLvoid* data)
{
	gl.bufferStorage(target, size, data, flags);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BufferStorage");
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

/** Unmaps buffer
 *
 * @param gl     GL functions
 * @param target Buffer target
 **/
void Buffer::UnMap(const glw::Functions& gl, glw::GLenum target)
{
	gl.unmapBuffer(target);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");
}

/** Constructor
 * Takes ownership of mapped region
 *
 * @param buffer Mapped buffer
 * @param data   Mapped data
 **/
Buffer::MapOwner::MapOwner(Buffer& buffer, glw::GLvoid* data) : m_data(data), m_buffer(&buffer)
{
	/* Nothing to be done */
}

Buffer::MapOwner::MapOwner(const Buffer::MoveMapOwner& moveOwner) : m_data(moveOwner.data), m_buffer(moveOwner.buffer)
{
}

/** Move constructor
 * Transfer ownership of mapped region.
 *
 * @param map_owner Map owner
 **/
Buffer::MapOwner::MapOwner(MapOwner& map_owner) : m_data(map_owner.m_data), m_buffer(map_owner.m_buffer)
{
	map_owner.m_data   = 0;
	map_owner.m_buffer = 0;
}

/** Destructor
 * Unmaps buffer
 **/
Buffer::MapOwner::~MapOwner()
{
	m_data = 0;
	if (0 != m_buffer)
	{
		m_buffer->Bind();
		m_buffer->UnMap();
		m_buffer = 0;
	}
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
 * @param width      Texture width
 * @param height     Texture height
 **/
void Framebuffer::AttachTexture(const glw::Functions& gl, glw::GLenum target, glw::GLenum attachment,
								glw::GLuint texture_id, glw::GLuint width, glw::GLuint height)
{
	gl.framebufferTexture(target, attachment, texture_id, 0 /* level */);
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

FUNCTIONALITY_SUPPORT Texture::m_direct_state_access_support = FUNCTIONALITY_SUPPORT_NOT_DETERMINED;

/* Texture constants */
const GLuint Texture::m_invalid_id = -1;

/** Constructor.
 *
 * @param context CTS context.
 **/
Texture::Texture(deqp::Context& context) : m_id(m_invalid_id), m_context(context)
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
		const Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteTextures(1, &m_id);
		m_id = m_invalid_id;
	}
}

/** Loads entry points for direct state access extension
 *
 * @param context CTS context
 **/
void Texture::LoadExtDirectStateAccess(deqp::Context& context)
{
	FUNCTIONALITY_SUPPORT support = getDirectStateAccessSupport(context);

	switch (support)
	{
	case FUNCTIONALITY_SUPPORT_NONE:
		/* Nothing to be done */
		break;
	case FUNCTIONALITY_SUPPORT_CORE:
	case FUNCTIONALITY_SUPPORT_EXTENSION:
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
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
void Texture::GetData(const glw::Functions& gl, glw::GLenum target, glw::GLenum format, glw::GLenum type,
					  glw::GLvoid* out_data)
{
	gl.getTexImage(target, 0 /* level */, format, type, out_data);
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
VertexArray::VertexArray(deqp::Context& context) : m_id(m_invalid_id), m_context(context)
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
		const Functions& gl = m_context.getRenderContext().getFunctions();

		Bind(gl, 0);

		gl.deleteVertexArrays(1, &m_id);

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

/** Constructor
 *
 * @param context Test context
 **/
ErrorsTest::ErrorsTest(deqp::Context& context)
	: TestCase(context, "errors", "Test if errors are generated as specified")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult ErrorsTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	Buffer::LoadExtDirectStateAccess(m_context);

	// No GL45 or GL_ARB_direct_state_access support
	if (m_direct_state_access_support == FUNCTIONALITY_SUPPORT_NONE)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Unsupported");
		return tcu::TestNode::STOP;
	}

	/*
	 * - INVALID_OPERATION is generated by BufferStorage when 0 is bound to
	 * <target>; Check all targets;
	 */
	for (GLuint i = 0; i < Buffer::m_n_targets; ++i)
	{
		const GLenum target  = Buffer::m_targets[i];
		std::string  message = "BufferStorage was executed for id 0, target: ";

		message.append(glu::getBufferTargetStr(target).toString().c_str());

		Buffer::Bind(gl, 0 /* id */, target);
		gl.bufferStorage(target, 0 /* size */, 0 /* data */, GL_DYNAMIC_STORAGE_BIT /* flags */);

		verifyError(GL_INVALID_OPERATION, message.c_str(), test_result);
	}

	/*
	 * - INVLIAD_OPERATION is generated by BufferStorage, NamedBufferStorage and
	 * BufferData if buffer already have immutable store;
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		buffer.InitStorage(GL_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT, data_size, data);

		/* NamedBufferStorage */
		if (0 != gl.namedBufferStorage)
		{
			gl.namedBufferStorage(buffer.m_id, data_size, data, GL_DYNAMIC_STORAGE_BIT);
			verifyError(GL_INVALID_OPERATION, "NamedBufferStorage was executed for id with immutable storage",
						test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.bufferStorage(GL_ARRAY_BUFFER, data_size, data, GL_DYNAMIC_STORAGE_BIT);
		verifyError(GL_INVALID_OPERATION, "BufferStorage was executed for target with immutable storage", test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
	}

	/*
	 * - INVALID_VALUE is generated by BufferStorage and NamedBufferStorage when
	 * <size> is less or equal to zero;
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		gl.createBuffers(1, &buffer.m_id);

		/* NamedBufferStorage */
		if (0 != gl.namedBufferStorage)
		{
			gl.namedBufferStorage(buffer.m_id, 0 /* size */, data, GL_DYNAMIC_STORAGE_BIT);
			verifyError(GL_INVALID_VALUE, "NamedBufferStorage was executed with size == 0", test_result);

			gl.namedBufferStorage(buffer.m_id, -16 /* size */, data, GL_DYNAMIC_STORAGE_BIT);
			verifyError(GL_INVALID_VALUE, "NamedBufferStorage was executed with size == -16", test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.bufferStorage(GL_ARRAY_BUFFER, 0 /* size */, data, GL_DYNAMIC_STORAGE_BIT);
		verifyError(GL_INVALID_VALUE, "BufferStorage was executed with size == 0", test_result);

		gl.bufferStorage(GL_ARRAY_BUFFER, -16 /* size */, data, GL_DYNAMIC_STORAGE_BIT);
		verifyError(GL_INVALID_VALUE, "BufferStorage was executed with size == -16", test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
	}

	/*
	 * - INVLAID_VALUE is generated by BufferStorage and NamedBufferStorage when
	 * <flags> contains MAP_PERSISTENT_BIT and neither MAP_READ_BIT nor
	 * MAP_WRITE_BIT;
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		gl.createBuffers(1, &buffer.m_id);

		/* NamedBufferStorage */
		if (0 != gl.namedBufferStorage)
		{
			gl.namedBufferStorage(buffer.m_id, data_size, data, GL_MAP_PERSISTENT_BIT);
			verifyError(GL_INVALID_VALUE, "NamedBufferStorage was executed with flags == GL_MAP_PERSISTENT_BIT",
						test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.bufferStorage(GL_ARRAY_BUFFER, data_size, data, GL_MAP_PERSISTENT_BIT);
		verifyError(GL_INVALID_VALUE, "BufferStorage was executed with flags == GL_MAP_PERSISTENT_BIT", test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
	}

	/*
	 * - INVALID_VALUE is generated by BufferStorage and NamedBufferStorage when
	 * <flags> contains MAP_COHERENT_BIT and no MAP_PERSISTENT_BIT;
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		gl.createBuffers(1, &buffer.m_id);

		/* NamedBufferStorage */
		if (0 != gl.namedBufferStorage)
		{
			gl.namedBufferStorage(buffer.m_id, data_size, data, GL_MAP_COHERENT_BIT);
			verifyError(GL_INVALID_VALUE, "NamedBufferStorage was executed with flags == GL_MAP_COHERENT_BIT",
						test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.bufferStorage(GL_ARRAY_BUFFER, data_size, data, GL_MAP_COHERENT_BIT);
		verifyError(GL_INVALID_VALUE, "BufferStorage was executed with flags == GL_MAP_COHERENT_BIT", test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
	}

	/*
	 * - INVALID_OPERATION is generated by MapBufferRange if any of:
	 *   * MAP_COHERENT_BIT,
	 *   * MAP_PERSISTENT_BIT,
	 *   * MAP_READ_BIT,
	 *   * MAP_WRITE_BIT
	 * is included in <access> and not in buffer's storage flags;
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		buffer.InitStorage(GL_ARRAY_BUFFER, GL_DYNAMIC_STORAGE_BIT, data_size, data);

		/* MapNamedBufferRange */
		if (0 != gl.mapNamedBufferRange)
		{
			gl.mapNamedBufferRange(buffer.m_id, 0 /* offset */, data_size, GL_MAP_READ_BIT);
			verifyError(GL_INVALID_OPERATION, "MapNamedBufferRange was executed with access == GL_MAP_READ_BIT, "
											  "storage flags == GL_DYNAMIC_STORAGE_BIT",
						test_result);

			gl.mapNamedBufferRange(buffer.m_id, 0 /* offset */, data_size, GL_MAP_WRITE_BIT);
			verifyError(GL_INVALID_OPERATION, "MapNamedBufferRange was executed with access == GL_MAP_WRITE_BIT, "
											  "storage flags == GL_DYNAMIC_STORAGE_BIT",
						test_result);

			gl.mapNamedBufferRange(buffer.m_id, 0 /* offset */, data_size, GL_MAP_PERSISTENT_BIT);
			verifyError(GL_INVALID_OPERATION, "MapNamedBufferRange was executed with access == GL_MAP_PERSISTENT_BIT, "
											  "storage flags == GL_DYNAMIC_STORAGE_BIT",
						test_result);

			gl.mapNamedBufferRange(buffer.m_id, 0 /* offset */, data_size, GL_MAP_COHERENT_BIT);
			verifyError(GL_INVALID_OPERATION, "MapNamedBufferRange was executed with access == GL_MAP_COHERENT_BIT, "
											  "storage flags == GL_DYNAMIC_STORAGE_BIT",
						test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.mapBufferRange(GL_ARRAY_BUFFER, 0 /* offset */, data_size, GL_MAP_READ_BIT);
		verifyError(
			GL_INVALID_OPERATION,
			"MapBufferRange was executed with access == GL_MAP_READ_BIT, storage flags == GL_DYNAMIC_STORAGE_BIT",
			test_result);

		gl.mapBufferRange(GL_ARRAY_BUFFER, 0 /* offset */, data_size, GL_MAP_WRITE_BIT);
		verifyError(
			GL_INVALID_OPERATION,
			"MapBufferRange was executed with access == GL_MAP_WRITE_BIT, storage flags == GL_DYNAMIC_STORAGE_BIT",
			test_result);

		gl.mapBufferRange(GL_ARRAY_BUFFER, 0 /* offset */, data_size, GL_MAP_PERSISTENT_BIT);
		verifyError(
			GL_INVALID_OPERATION,
			"MapBufferRange was executed with access == GL_MAP_PERSISTENT_BIT, storage flags == GL_DYNAMIC_STORAGE_BIT",
			test_result);

		gl.mapBufferRange(GL_ARRAY_BUFFER, 0 /* offset */, data_size, GL_MAP_COHERENT_BIT);
		verifyError(
			GL_INVALID_OPERATION,
			"MapBufferRange was executed with access == GL_MAP_COHERENT_BIT, storage flags == GL_DYNAMIC_STORAGE_BIT",
			test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
	}

	/*
	 * - INVALID_OPERATION is generated by BufferSubData and NamedBufferSubData
	 * when buffer has immutable store but its flags does not include
	 * DYNAMIC_STORAGE.
	 */
	{
		static const GLsizeiptr data_size = 32;
		static GLubyte			data[data_size];

		Buffer buffer(m_context);
		buffer.InitStorage(GL_ARRAY_BUFFER, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT, data_size,
						   data);

		/* NamedBufferSubData */
		if (0 != gl.namedBufferSubData)
		{
			gl.namedBufferSubData(buffer.m_id, 0 /* offset */, data_size, data);
			verifyError(GL_INVALID_OPERATION,
						"NamedBufferSubData was executed for storage without GL_DYNAMIC_STORAGE_BIT", test_result);
		}

		/* BufferStorage */
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		gl.bufferSubData(GL_ARRAY_BUFFER, 0 /* offset */, data_size, data);
		verifyError(GL_INVALID_OPERATION, "BufferSubData was executed for storage without GL_DYNAMIC_STORAGE_BIT",
					test_result);

		Buffer::Bind(gl, 0, GL_ARRAY_BUFFER);
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

/** Verifies that expected error was generated
 *
 * @param expected_error  Expected error
 * @param error_message   Message that will be logged in case of wrong error
 * @param out_test_result Set to false if worng error was generated, not modified otherwise
 **/
void ErrorsTest::verifyError(glw::GLenum expected_error, const glw::GLchar* error_message, bool& out_test_result)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLenum error = gl.getError();

	if (error != expected_error)
	{
		out_test_result = false;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Got invalid error: " << glu::getErrorName(error)
			<< ", expected: " << glu::getErrorName(expected_error) << ". Message: " << error_message
			<< tcu::TestLog::EndMessage;
	}
}

/** Constructor
 *
 * @param context Test context
 **/
GetBufferParameterTest::GetBufferParameterTest(deqp::Context& context)
	: TestCase(context, "get_buffer_parameter", "Test queries for parameters of buffers")
{
	static const GLenum s_mapping_bits[] = { 0, GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT };
	static const GLuint s_n_mapping_bits = sizeof(s_mapping_bits) / sizeof(s_mapping_bits[0]);

	GLenum flags = 0;

	for (GLuint dynamic = 0; dynamic < 2; ++dynamic)
	{
		flags = (0 == dynamic) ? 0 : GL_DYNAMIC_STORAGE_BIT;

		for (GLuint client = 0; client < 2; ++client)
		{
			flags |= (0 == client) ? 0 : GL_CLIENT_STORAGE_BIT;

			/* No "map" bits */
			if (0 != flags)
			{
				m_test_cases.push_back(testCase(flags, 0));
			}

			for (GLuint flag_idx = 0; flag_idx < s_n_mapping_bits; ++flag_idx)
			{
				const GLenum flag_mapping_bits  = s_mapping_bits[flag_idx];
				const GLenum flags_with_mapping = flags | flag_mapping_bits;

				for (GLuint access_idx = 0; access_idx <= flag_idx; ++access_idx)
				{
					const GLenum access = s_mapping_bits[access_idx];

					m_test_cases.push_back(testCase(flags_with_mapping | GL_MAP_READ_BIT, access | GL_MAP_READ_BIT));
					m_test_cases.push_back(testCase(flags_with_mapping | GL_MAP_WRITE_BIT, access | GL_MAP_WRITE_BIT));
					m_test_cases.push_back(
						testCase(flags_with_mapping | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, access | GL_MAP_READ_BIT));
					m_test_cases.push_back(
						testCase(flags_with_mapping | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, access | GL_MAP_WRITE_BIT));
					m_test_cases.push_back(testCase(flags_with_mapping | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
													access | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT));
				}
			}
		}
	}
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult GetBufferParameterTest::iterate()
{
	static const GLsizeiptr data_size = 32;
	static GLubyte			data[data_size];

	Buffer::LoadExtDirectStateAccess(m_context);

	// No GL45 or GL_ARB_direct_state_access support
	if (m_direct_state_access_support == FUNCTIONALITY_SUPPORT_NONE)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Unsupported");
		return tcu::TestNode::STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	for (GLuint i = 0; i < m_test_cases.size(); ++i)
	{
		const testCase& test_case = m_test_cases[i];
		const GLenum	access	= test_case.m_access;
		const GLenum	flags	 = test_case.m_flags;

		GLint queried_flags		= -1;
		GLint queried_immutable = -1;
		GLint queried_size		= -1;

		Buffer buffer(m_context);

		buffer.InitStorage(GL_ARRAY_BUFFER, flags, data_size, data);
		Buffer::Bind(gl, buffer.m_id, GL_ARRAY_BUFFER);

		if (0 != gl.getNamedBufferParameteriv)
		{
			Buffer::GetNamedParameter(gl, buffer.m_id, GL_BUFFER_STORAGE_FLAGS, &queried_flags);
			Buffer::GetNamedParameter(gl, buffer.m_id, GL_BUFFER_IMMUTABLE_STORAGE, &queried_immutable);
			Buffer::GetNamedParameter(gl, buffer.m_id, GL_BUFFER_SIZE, &queried_size);

			if (queried_flags != (GLint)flags)
			{
				test_result = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GetNamedBufferParameteriv reported invalid state of GL_BUFFER_STORAGE_FLAGS: " << queried_flags
					<< " expected: " << flags << tcu::TestLog::EndMessage;
			}
		}

		if (queried_flags != (GLint)flags)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetNamedBufferParameteriv reported invalid state of GL_BUFFER_STORAGE_FLAGS: " << queried_flags
				<< " expected: " << flags << tcu::TestLog::EndMessage;
		}

		if (queried_immutable != GL_TRUE)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetNamedBufferParameteriv reported invalid state of GL_BUFFER_IMMUTABLE_STORAGE: "
				<< queried_immutable << " expected: " << GL_TRUE << tcu::TestLog::EndMessage;
		}

		if (queried_size != data_size)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetNamedBufferParameteriv reported invalid state of GL_BUFFER_SIZE: " << queried_size
				<< " expected: " << data_size << tcu::TestLog::EndMessage;
		}

		queried_flags	 = -1;
		queried_immutable = -1;
		queried_size	  = -1;

		Buffer::GetParameter(gl, GL_ARRAY_BUFFER, GL_BUFFER_STORAGE_FLAGS, &queried_flags);
		Buffer::GetParameter(gl, GL_ARRAY_BUFFER, GL_BUFFER_IMMUTABLE_STORAGE, &queried_immutable);
		Buffer::GetParameter(gl, GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &queried_size);

		if (queried_flags != (GLint)flags)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetBufferParameteriv reported invalid state of GL_BUFFER_STORAGE_FLAGS: " << queried_flags
				<< " expected: " << flags << tcu::TestLog::EndMessage;
		}

		if (queried_immutable != GL_TRUE)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetBufferParameteriv reported invalid state of GL_BUFFER_IMMUTABLE_STORAGE: " << queried_immutable
				<< " expected: " << GL_TRUE << tcu::TestLog::EndMessage;
		}

		if (queried_size != data_size)
		{
			test_result = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GetBufferParameteriv reported invalid state of GL_BUFFER_SIZE: " << queried_size
				<< " expected: " << data_size << tcu::TestLog::EndMessage;
		}

		if (0 != access)
		{
			GLint queried_access = -1;

			Buffer::MapOwner tmp(buffer.MapRange(0 /* offset */, data_size, access));

			if (0 != gl.getNamedBufferParameteriv)
			{
				Buffer::GetNamedParameter(gl, buffer.m_id, GL_BUFFER_ACCESS_FLAGS, &queried_access);
			}

			if (queried_access != (GLint)access)
			{
				test_result = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GetNamedBufferParameteriv reported invalid state of GL_BUFFER_ACCESS_FLAGS: " << queried_access
					<< " expected: " << access << tcu::TestLog::EndMessage;
			}

			queried_access = -1;

			Buffer::GetParameter(gl, GL_ARRAY_BUFFER, GL_BUFFER_ACCESS_FLAGS, &queried_access);

			if (queried_access != (GLint)access)
			{
				test_result = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GetBufferParameteriv reported invalid state of GL_BUFFER_ACCESS_FLAGS: " << queried_access
					<< " expected: " << access << tcu::TestLog::EndMessage;
			}
		}

		Buffer::Bind(gl, 0 /* id */, GL_ARRAY_BUFFER);
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

/** Constructor
 *
 * @param context Test context
 **/
GetBufferParameterTest::testCase::testCase(glw::GLenum flags, glw::GLenum access) : m_flags(flags), m_access(access)
{
}

/** Constructor
 *
 * @param context Test context
 **/
DynamicStorageTest::DynamicStorageTest(deqp::Context& context)
	: TestCase(context, "dynamic_storage", "Test if DYNAMIC_STORAGE_BIT is respected")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult DynamicStorageTest::iterate()
{
	static const size_t data_size = 64;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/*
	 * - prepare 64 bytes immutable buffer filled with value 1; Bind the buffer to
	 * COPY_READ_BUFFER;
	 * - prepare 64 bytes immutable buffer filled with value 2; Do not set
	 * DYNAMIC_STORAGE_BIT for <flags>; Bind the buffer to COPY_WRITE_BUFFER;
	 * - execute BufferSubData to update COPY_WRITE_BUFFER buffer with 64 bytes
	 * filled with value 3; INVLIAD_OPERATION error should be generated;
	 * - inspect contents of buffer to verify it is filled with 2;
	 * - execute CopyBufferSubData to transfer data from COPY_READ_BUFFER to
	 * COPY_WRITE_BUFFER; No error should be generated;
	 * - inspect contents of buffer to verify it is filled with 1;
	 */
	{
		/* Prepare buffers */
		GLubyte read_data[data_size];
		GLubyte temp_data[data_size];
		GLubyte update_data[data_size];
		GLubyte write_data[data_size];

		for (size_t i = 0; i < data_size; ++i)
		{
			read_data[i]   = 1;
			temp_data[i]   = 0;
			update_data[i] = 3;
			write_data[i]  = 2;
		}

		Buffer read_buffer(m_context);
		Buffer write_buffer(m_context);

		read_buffer.InitStorage(GL_COPY_READ_BUFFER, 0 /* flags */, data_size, read_data);
		write_buffer.InitStorage(GL_COPY_WRITE_BUFFER, 0 /* flags */, data_size, write_data);

		/* Check bufferSubData */
		write_buffer.Bind();
		gl.bufferSubData(GL_COPY_WRITE_BUFFER, 0 /* offset */, data_size, update_data);

		GLenum error = gl.getError();
		if (GL_INVALID_OPERATION != error)
		{
			test_result = false;

			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid error was generated. BufferSubData was executed on store without "
											"DYNAMIC_STORAGE_BIT. Expected INVALID_OPERATION, got: "
				<< glu::getErrorStr(error).toString().c_str() << tcu::TestLog::EndMessage;
		}

		Buffer::GetSubData(gl, GL_COPY_WRITE_BUFFER, 0 /* offset */, data_size, temp_data);

		if (0 != memcmp(temp_data, write_data, data_size))
		{
			test_result = false;

			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BufferSubData modified contents of store without DYNAMIC_STORAGE_BIT."
				<< tcu::TestLog::EndMessage;
		}

		/* Check copyBufferSubData */
		read_buffer.Bind();
		gl.copyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0 /* readOffset */, 0 /* writeOffset */,
							 data_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "CopyBufferSubData");

		Buffer::GetSubData(gl, GL_COPY_WRITE_BUFFER, 0 /* offset */, data_size, temp_data);

		if (0 != memcmp(temp_data, read_data, data_size))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "CopyBufferSubData stored invalid contents in write target buffer."
												<< tcu::TestLog::EndMessage;
		}
	}

	/*
	 * - delete buffer and create new one; This time <flags> should contain
	 * DYNAMIC_STORAGE_BIT; Bind the buffer to COPY_WRITE_BUFFER;
	 * - execute BufferSubData to update COPY_WRITE_BUFFER buffer with 64 bytes
	 * filled with value 3; No error should be generated;
	 * - inspect contents of buffer to verify it is filled with 3;
	 */
	{
		/* Prepare buffers */
		GLubyte temp_data[data_size];
		GLubyte update_data[data_size];
		GLubyte write_data[data_size];

		for (size_t i = 0; i < data_size; ++i)
		{
			temp_data[i]   = 0;
			update_data[i] = 3;
			write_data[i]  = 2;
		}

		Buffer write_buffer(m_context);

		write_buffer.InitStorage(GL_COPY_WRITE_BUFFER, GL_DYNAMIC_STORAGE_BIT, data_size, write_data);

		/* Check bufferSubData */
		write_buffer.Bind();
		gl.bufferSubData(GL_COPY_WRITE_BUFFER, 0 /* offset */, data_size, update_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferSubData");

		Buffer::GetSubData(gl, GL_COPY_WRITE_BUFFER, 0 /* offset */, data_size, temp_data);

		if (0 != memcmp(temp_data, update_data, data_size))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "BufferSubData stored invalid contents in write target buffer."
												<< tcu::TestLog::EndMessage;
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

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentBufferSubDataTest::MapPersistentBufferSubDataTest(deqp::Context& context)
	: TestCase(context, "map_persistent_buffer_sub_data", "Test sub buffer operations against mapped buffer")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentBufferSubDataTest::iterate()
{
	static const size_t		data_size			 = 64;
	static const GLintptr   mapped_region_offset = 16;
	static const GLsizeiptr mapped_region_size   = 16;
	static const testCase   test_cases[]		 = {
		{ 0, 16, false },  /* before mapped region */
		{ 32, 16, false }, /* after mapped region  */
		{ 8, 16, true },   /* at the beginning     */
		{ 24, 16, true },  /* at the end           */
		{ 12, 8, true },   /* in the middle        */
	};
	static const size_t n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Storage for data */
	GLubyte incrementing_data[data_size];

	/* Prepare data */
	for (size_t i = 0; i < data_size; ++i)
	{
		incrementing_data[i] = (glw::GLubyte)i;
	}

	/* Load DSA */
	Buffer::LoadExtDirectStateAccess(m_context);

	// No GL45 or GL_ARB_direct_state_access support
	if (m_direct_state_access_support == FUNCTIONALITY_SUPPORT_NONE)
	{
		m_context.getTestContext().setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Unsupported");
		return tcu::TestNode::STOP;
	}

	/* Prepare buffer */
	Buffer buffer(m_context);
	buffer.InitStorage(GL_ARRAY_BUFFER,
					   GL_DYNAMIC_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, data_size,
					   0 /* data */);
	buffer.Bind();

	/*
	 * - execute tested operation, to update whole buffer with incrementing values
	 * starting from 0; No error should be generated;
	 */
	gl.bufferSubData(GL_ARRAY_BUFFER, 0 /* offset */, data_size, incrementing_data);
	GLenum error = gl.getError();

	if (GL_NO_ERROR != error)
	{
		test_result = false;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "BufferSubData generated unexpected error: "
											<< glu::getErrorStr(error).toString().c_str() << tcu::TestLog::EndMessage;
	}

	if (0 != gl.namedBufferSubData)
	{
		gl.namedBufferSubData(buffer.m_id, 0 /* offset */, data_size, incrementing_data);
		error = gl.getError();
	}

	gl.namedBufferSubData(buffer.m_id, 0 /* offset */, data_size, incrementing_data);
	error = gl.getError();

	if (GL_NO_ERROR != error)
	{
		test_result = false;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "NamedBufferSubData generated unexpected error: " << glu::getErrorStr(error).toString().c_str()
			<< tcu::TestLog::EndMessage;
	}

	/*
	 * - map buffer contents with MapBufferRange; <access> should contain
	 * MAP_PERSISTENT_BIT, MAP_READ_BIT and MAP_WRITE_BIT; Provide 16 as <offset>
	 * and <size>;
	 * - mapped region should contain values from 16 to 31;
	 * - execute tested operation, to update portions of buffer specified below;
	 * No error should be generated;
	 */
	{
		const Buffer::MapOwner map(buffer.MapRange(mapped_region_offset, mapped_region_size,
												   GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		if (0 != memcmp(map.m_data, incrementing_data + mapped_region_offset, mapped_region_size))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Mapped region contains unexpected data"
												<< tcu::TestLog::EndMessage;
		}

		for (size_t i = 0; i < n_test_cases; ++i)
		{
			const GLintptr   offset = test_cases[i].m_offset;
			const GLsizeiptr size   = test_cases[i].m_size;

			gl.bufferSubData(GL_ARRAY_BUFFER, offset, size, incrementing_data);
			error = gl.getError();

			if (GL_NO_ERROR != error)
			{
				test_result = false;

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "BufferSubData generated unexpected error: " << glu::getErrorStr(error).toString().c_str()
					<< tcu::TestLog::EndMessage;
			}

			if (0 != gl.namedBufferSubData)
			{
				gl.namedBufferSubData(buffer.m_id, offset, size, incrementing_data);
				error = gl.getError();
			}

			if (GL_NO_ERROR != error)
			{
				test_result = false;

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "NamedBufferSubData generated unexpected error: " << glu::getErrorStr(error).toString().c_str()
					<< tcu::TestLog::EndMessage;
			}
		}
	}

	/*
	 * - unmap buffer;
	 * - map buffer contents again, this time do not provide MAP_PERSISTENT_BIT;
	 * - execute tested operation to update regions specified below; It is expected
	 * that INVALID_OPERATION will be generated for cases that cross mapped region;
	 * No error should be generated for other cases.
	 */
	{
		Buffer::MapOwner tmp(
			buffer.MapRange(mapped_region_offset, mapped_region_size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		for (size_t i = 0; i < n_test_cases; ++i)
		{
			const GLintptr   offset			   = test_cases[i].m_offset;
			const GLsizeiptr size			   = test_cases[i].m_size;
			const bool		 is_error_expected = test_cases[i].m_cross_mapped_region;
			const GLenum	 expected_error	= (true == is_error_expected) ? GL_INVALID_OPERATION : GL_NO_ERROR;

			gl.bufferSubData(GL_ARRAY_BUFFER, offset, size, incrementing_data);
			error = gl.getError();

			if (expected_error != error)
			{
				test_result = false;

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "BufferSubData generated wrong error: " << glu::getErrorStr(error).toString().c_str()
					<< ", expected: " << glu::getErrorStr(expected_error).toString().c_str()
					<< ". Mapped region: offset: " << mapped_region_offset << ", size: " << mapped_region_size
					<< ". Operation region: offset: " << offset << ", size: " << size << tcu::TestLog::EndMessage;
			}

			if (0 != gl.namedBufferSubData)
			{
				gl.namedBufferSubData(buffer.m_id, offset, size, incrementing_data);
				error = gl.getError();
			}

			if (expected_error != error)
			{
				test_result = false;

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "NamedBufferSubData generated wrong error: " << glu::getErrorStr(error).toString().c_str()
					<< ", expected: " << glu::getErrorStr(expected_error).toString().c_str()
					<< ". Mapped region: offset: " << mapped_region_offset << ", size: " << mapped_region_size
					<< ". Operation region: offset: " << offset << ", size: " << size << tcu::TestLog::EndMessage;
			}
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

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentTextureTest::MapPersistentTextureTest(deqp::Context& context)
	: TestCase(context, "map_persistent_texture", "Test texture operations against mapped buffer")
	, m_compressed_image_size(0)
	, m_compressed_internal_format(0)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentTextureTest::iterate()
{
	static const size_t data_size = 256;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Storage for data */
	GLubyte data[data_size];

	/* Prepare data */
	for (size_t i = 0; i < data_size; ++i)
	{
		data[i] = (glw::GLubyte)i;
	}

	/* Load DSA */
	Buffer::LoadExtDirectStateAccess(m_context);
	Texture::LoadExtDirectStateAccess(m_context);

	/* Get info about compressed image */
	getCompressedInfo();

	/* Prepare buffer */
	Buffer buffer(m_context);
	buffer.InitStorage(GL_PIXEL_UNPACK_BUFFER, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, data_size,
					   data);
	Buffer::Bind(gl, 0 /* id */, GL_PIXEL_UNPACK_BUFFER);

	/*
	 * - prepare texture in a way that is relevant for tested operation;
	 * - execute tested operation, no error should be generated;
	 * - delete texture and prepare next one;
	 */
	for (GLuint i = 0; i < TESTED_OPERATION_MAX; ++i)
	{
		const TESTED_OPERATION operation = (TESTED_OPERATION)i;

		bool result = verifyTestedOperation(operation, buffer, GL_NO_ERROR);

		if (false == result)
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Buffer bound to PIXEL_UNPACK_BUFFER is not mapped"
												<< tcu::TestLog::EndMessage;
		}
	}

	/*
	 * - map buffer contents with MapBufferRange, <access> should contain
	 * MAP_PERSISTENT_BIT, MAP_READ_BIT and MAP_WRITE_BIT;
	 * - execute tested operation, no error should be generated;
	 */
	for (GLuint i = 0; i < TESTED_OPERATION_MAX; ++i)
	{
		const TESTED_OPERATION operation = (TESTED_OPERATION)i;

		{
			Buffer::MapOwner tmp(
				buffer.MapRange(0 /* offset */, data_size, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
		}
		Buffer::Bind(gl, 0 /* id */, GL_PIXEL_UNPACK_BUFFER);

		bool result = verifyTestedOperation(operation, buffer, GL_NO_ERROR);

		if (false == result)
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Buffer bound to PIXEL_UNPACK_BUFFER is persistently mapped"
												<< tcu::TestLog::EndMessage;
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

/** Return name of operation
 *
 * @param operation Operation which name will be returned
 *
 * @return Name of operation or 0 in case of invalid enum
 **/
const char* MapPersistentTextureTest::getOperationName(TESTED_OPERATION operation)
{
	const char* name = 0;

	switch (operation)
	{
	case OP_COMPRESSED_TEX_IMAGE:
		name = "CompressedTexImage";
		break;
	case OP_COMPRESSED_TEX_SUB_IMAGE:
		name = "CompressedTexSubImage";
		break;
	case OP_COMPRESSED_TEXTURE_SUB_IMAGE:
		name = "CompressedTextureSubImage";
		break;
	case OP_TEX_IMAGE:
		name = "TexImage";
		break;
	case OP_TEX_SUB_IMAGE:
		name = "TexSubImage";
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	return name;
}

/** Check format and size of compressed image
 *
 **/
void MapPersistentTextureTest::getCompressedInfo()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	/* Texture creation */
	Texture texture(m_context);
	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);
	Texture::Image(gl, GL_TEXTURE_2D, 0, GL_COMPRESSED_RED_RGTC1, 8, 8, 1, GL_RED, GL_UNSIGNED_BYTE,
				   0); // glspec 4.5 pg 216

	/* Queries */
	Texture::GetLevelParameter(gl, GL_TEXTURE_2D, 0 /* level */, GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
							   &m_compressed_image_size);
	Texture::GetLevelParameter(gl, GL_TEXTURE_2D, 0 /* level */, GL_TEXTURE_INTERNAL_FORMAT,
							   &m_compressed_internal_format);
}

/** Verifies results of tested operation
 *
 * @param operation      Operation to be tested
 * @param buffer         Buffer that will be used as GL_PIXEL_UNPACK_BUFFER
 * @param expected_error Expected error
 *
 * @return false in case of any error, true otherwise
 **/
bool MapPersistentTextureTest::verifyTestedOperation(TESTED_OPERATION operation, Buffer& buffer,
													 glw::GLenum expected_error)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLenum  error = GL_NO_ERROR;
	Texture texture(m_context);

	/* Prepare texture */
	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);

	switch (operation)
	{
	case OP_COMPRESSED_TEX_IMAGE:
	case OP_TEX_IMAGE:
		break;
	case OP_COMPRESSED_TEX_SUB_IMAGE:
	case OP_COMPRESSED_TEXTURE_SUB_IMAGE:
		Texture::CompressedImage(gl, GL_TEXTURE_2D, 0 /* level */, m_compressed_internal_format, 8 /* width */,
								 8 /* height */, 0 /* depth */, m_compressed_image_size /* imageSize */,
								 0 /* empty image */);
		break;
	case OP_TEX_SUB_IMAGE:
		Texture::Image(gl, GL_TEXTURE_2D, 0 /* level */, GL_R8, 8 /* width */, 8 /* height */, 0 /* depth */, GL_RED,
					   GL_UNSIGNED_BYTE, 0 /* empty image */);
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	/* Bind buffer to PIXEL_UNPACK */
	Buffer::Bind(gl, buffer.m_id, GL_PIXEL_UNPACK_BUFFER);

	/* Execute operation */
	switch (operation)
	{
	case OP_COMPRESSED_TEX_IMAGE:
		gl.compressedTexImage2D(GL_TEXTURE_2D, 0 /* level */, m_compressed_internal_format, 8 /* width */,
								8 /* height */, 0 /* border */, m_compressed_image_size /* imageSize */,
								0 /* offset to pixel unpack buffer */);
		error = gl.getError();
		break;
	case OP_COMPRESSED_TEX_SUB_IMAGE:
		gl.compressedTexSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 8 /* width */, 8 /* height */,
								   m_compressed_internal_format, m_compressed_image_size,
								   0 /* offset to pixel unpack buffer */);
		error = gl.getError();
		break;
	case OP_COMPRESSED_TEXTURE_SUB_IMAGE:
		if (0 != gl.compressedTextureSubImage2D)
		{
			gl.compressedTextureSubImage2D(texture.m_id, 0 /* level */, 0 /* x */, 0 /* y */, 8 /* width */,
										   8 /* height */, m_compressed_internal_format, m_compressed_image_size,
										   0 /* offset to pixel unpack buffer */);
			error = gl.getError();
		}
		else
		{
			/* Not supported, ignore */
			error = expected_error;
		}
		break;
	case OP_TEX_IMAGE:
		gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, GL_R8, 8 /* width */, 8 /* height */, 0 /* border */, GL_RED,
					  GL_UNSIGNED_BYTE, 0 /* offset to pixel unpack buffer */);
		error = gl.getError();
		break;
	case OP_TEX_SUB_IMAGE:
		gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 8 /* width */, 8 /* height */, GL_RED,
						 GL_UNSIGNED_BYTE, 0 /* offset to pixel unpack buffer */);
		error = gl.getError();
		break;
	default:
		TCU_FAIL("Invalid enum");
	}

	/* Unbind buffer */
	Buffer::Bind(gl, 0 /* id */, GL_PIXEL_UNPACK_BUFFER);

	/* Check result */
	if (expected_error != error)
	{
		result = false;

		m_context.getTestContext().getLog() << tcu::TestLog::Message << getOperationName(operation)
											<< " generated wrong error: " << glu::getErrorStr(error).toString().c_str()
											<< ", expected: " << glu::getErrorStr(expected_error).toString().c_str()
											<< tcu::TestLog::EndMessage;
	}

	/* Done */
	return result;
}

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentReadPixelsTest::MapPersistentReadPixelsTest(deqp::Context& context)
	: TestCase(context, "map_persistent_read_pixels", "Test read pixels operation against mapped buffer")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentReadPixelsTest::iterate()
{
	static const GLuint height	= 8;
	static const GLuint width	 = 8;
	static const size_t data_size = width * height;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Prepare data */
	GLubyte initial_texture_data[data_size];
	GLubyte updated_texture_data[data_size];

	for (size_t i = 0; i < data_size; ++i)
	{
		initial_texture_data[i] = (glw::GLubyte)i;
		updated_texture_data[i] = (glw::GLubyte)(data_size - i);
	}

	/* Prepare GL objects */
	Buffer		buffer(m_context);
	Framebuffer framebuffer(m_context);
	Texture		texture(m_context);

	buffer.InitStorage(GL_PIXEL_PACK_BUFFER, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT, data_size,
					   0 /* data */);

	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);
	Texture::Storage(gl, GL_TEXTURE_2D, 1 /* levels */, GL_R8UI, width, height, 0 /* depth */);
	Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, 0 /* depth */,
					  GL_RED_INTEGER, GL_UNSIGNED_BYTE, initial_texture_data);

	Framebuffer::Generate(gl, framebuffer.m_id);
	Framebuffer::Bind(gl, GL_READ_FRAMEBUFFER, framebuffer.m_id);
	Framebuffer::AttachTexture(gl, GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.m_id, width, height);

	/*
	 * - execute ReadPixels to transfer texture contents to buffer, no error should
	 * be generated;
	 */
	buffer.Bind();
	gl.readPixels(0 /* x */, 0 /* y */, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
				  0 /* offset in PIXEL_PACK_BUFFER */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels to not mapped PIXEL_PACK buffer");

	/*
	 * - update contents of texture with different image;
	 * - map buffer contents with MapBufferRange, <access> should contain
	 * MAP_PERSISTENT_BIT, MAP_READ_BIT and MAP_WRITE_BIT;
	 * - execute ReadPixels to transfer texture contents to buffer, no error should
	 * be generated;
	 * - execute MemoryBarrier with CLIENT_MAPPED_BUFFER_BARRIER_BIT and Finish;
	 * - inspect contents of mapped buffer, to verify that latest data transfer was
	 * successful;
	 * - unmap buffer
	 */
	{
		Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, 0 /* z */, width, height,
						  0 /* depth */, GL_RED_INTEGER, GL_UNSIGNED_BYTE, updated_texture_data);

		const Buffer::MapOwner map(
			buffer.MapRange(0 /* offset */, data_size, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		buffer.Bind();
		gl.readPixels(0 /* x */, 0 /* y */, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
					  0 /* offset in PIXEL_PACK_BUFFER */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels to persistently mapped PIXEL_PACK buffer");

		gl.memoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Finish");

		if (0 != memcmp(updated_texture_data, map.m_data, data_size))
		{
			test_result = false;

			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Wrong contents of persistently mapped PIXEL_PACK buffer after ReadPixels"
				<< tcu::TestLog::EndMessage;
		}
	}

	/*
	 * - map buffer contents again, this time do not provide MAP_PERSISTENT_BIT;
	 * - execute ReadPixels to transfer texture contents to buffer,
	 * INVALID_OPERATION error should be generated.
	 */
	{
		Buffer::MapOwner tmp(buffer.MapRange(0 /* offset */, data_size, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		buffer.Bind();
		gl.readPixels(0 /* x */, 0 /* y */, width, height, GL_RED_INTEGER, GL_UNSIGNED_BYTE,
					  0 /* offset in PIXEL_PACK_BUFFER */);
		GLenum error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			test_result = false;

			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Wrong error was generated by ReadPixels. Expected INVALID_OPERATION as "
											"PIXEL_PACK buffer is mapped. Got: "
				<< glu::getErrorStr(error).toString().c_str() << tcu::TestLog::EndMessage;
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

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentDispatchTest::MapPersistentDispatchTest(deqp::Context& context)
	: TestCase(context, "map_persistent_dispatch", "test dispatch operation against mapped buffer")
{
	/* Nothing to be done here */
}

/** Constructor
 *
 * @param context          Test context
 * @param test_name        Test name
 * @param test_description Test description
 **/
MapPersistentDispatchTest::MapPersistentDispatchTest(deqp::Context& context, const GLchar* test_name,
													 const GLchar* test_description)
	: TestCase(context, test_name, test_description)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentDispatchTest::iterate()
{
	static const GLchar* compute_shader = "#version 430 core\n"
										  "\n"
										  "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										  "\n"
										  "layout (binding = 0, std430) buffer DestinationData {\n"
										  "    uint values[];\n"
										  "} destination;\n"
										  "\n"
										  "layout (binding = 1, std430) buffer SourceData {\n"
										  "    uint values[];\n"
										  "} source;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    uint index = gl_GlobalInvocationID.x;\n"
										  "    uint sum   = 0u;\n"
										  "\n"
										  "    for (uint i = 0u; i <= index; ++i)\n"
										  "    {\n"
										  "        sum += source.values[i];\n"
										  "    }\n"
										  "\n"
										  "    destination.values[index] = sum;\n"
										  "}\n"
										  "\n";
	static const GLuint data_size			= 16;
	static const GLuint destination_binding = 0;
	static const GLuint source_binding		= 1;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Prepare data */
	GLuint destination_data[data_size];
	GLuint modified_source_data[data_size];
	GLuint modified_sum_data[data_size];
	GLuint source_data[data_size];
	GLuint sum_data[data_size];

	GLuint modified_sum = 0;
	GLuint sum			= 0;

	for (GLuint i = 0; i < data_size; ++i)
	{
		destination_data[i]		= 0;
		modified_source_data[i] = data_size - i;
		source_data[i]			= i;

		modified_sum += modified_source_data[i];
		sum += source_data[i];

		modified_sum_data[i] = modified_sum;
		sum_data[i]			 = sum;
	}

	/* Prepare buffers */
	Buffer destination(m_context);
	Buffer source(m_context);

	destination.InitStorage(GL_SHADER_STORAGE_BUFFER, GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
							data_size * sizeof(GLuint), destination_data);

	source.InitStorage(GL_SHADER_STORAGE_BUFFER, GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
					   data_size * sizeof(GLuint), source_data);

	/* Prepare program */
	Program program(m_context);
	program.Init(compute_shader, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	/*
	 * - bind buffers to SHADER_STORAGE_BUFFER;
	 * - use MapBufferRange to map both buffers; <access> shall be set as follows:
	 *   * MAP_COHERENT_BIT and MAP_PERSISTENT_BIT flags set for both
	 *   * MAP_WRITE_BIT flag shall be set for source;
	 *   * MAP_READ_BIT flag shall be set for destination;
	 * - dispatch program for 16x1x1 groups;
	 * - modify contents of source buffer via mapped memory;
	 * - execute Finish;
	 * - inspect contents of destination buffer via mapped memory; It is expected
	 * that it will contain results based on original content of source buffer;
	 * - dispatch program for 16x1x1 groups;
	 * - execute Finish;
	 * - inspect contents of destination buffer via mapped memory; It is expected
	 * that it will contain results based on modified content of source buffer.
	 */
	{
		/* Set program */
		Program::Use(gl, program.m_id);

		/* Map buffers */
		destination.Bind();
		const Buffer::MapOwner destination_map(destination.MapRange(
			0 /* offset */, data_size * sizeof(GLuint), GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT));

		source.Bind();
		const Buffer::MapOwner source_map(
			source.MapRange(0 /* offset */, data_size * sizeof(GLuint),
							GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT));

		/* Clear binding point */
		Buffer::Bind(gl, 0, GL_SHADER_STORAGE_BUFFER);

		/* Bind buffers */
		Buffer::BindBase(gl, destination.m_id, GL_SHADER_STORAGE_BUFFER, destination_binding);
		Buffer::BindBase(gl, source.m_id, GL_SHADER_STORAGE_BUFFER, source_binding);

		/* Execute program for 16x1x1 groups */
		gl.dispatchCompute(16, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute with persistently mapped buffers");

		/* Make sure that program executed */
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Finish");

		if (0 != memcmp(destination_map.m_data, sum_data, data_size * sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of mapped region does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
		}

		/* Modify source buffer via mapped area */
		memcpy(source_map.m_data, modified_source_data, data_size * sizeof(GLuint));

		/* Execute program for 16x1x1 groups */
		gl.dispatchCompute(16, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute with persistently mapped buffers");

		/* Make sure that program executed */
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Finish");

		if (0 != memcmp(destination_map.m_data, modified_sum_data, data_size * sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of mapped region does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
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

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentFlushTest::MapPersistentFlushTest(deqp::Context& context)
	: TestCase(context, "map_persistent_flush", "Test mapped buffer against flushing")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentFlushTest::iterate()
{
	static const GLchar* compute_shader = "#version 430 core\n"
										  "\n"
										  "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										  "\n"
										  "layout (binding = 0, std430) buffer DestinationData {\n"
										  "    uint values[];\n"
										  "} destination;\n"
										  "\n"
										  "layout (binding = 1, std430) buffer SourceData {\n"
										  "    uint values[];\n"
										  "} source;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    uint index = gl_GlobalInvocationID.x;\n"
										  "    uint sum   = 0u;\n"
										  "\n"
										  "    for (uint i = 0u; i <= index; ++i)\n"
										  "    {\n"
										  "        sum += source.values[i];\n"
										  "    }\n"
										  "\n"
										  "    destination.values[index] = sum;\n"
										  "}\n"
										  "\n";
	static const GLuint data_size			= 16;
	static const GLuint destination_binding = 0;
	static const GLuint source_binding		= 1;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Prepare data */
	GLuint destination_data[data_size];
	GLuint modified_source_data[data_size];
	GLuint modified_sum_data[data_size];
	GLuint source_data[data_size];
	GLuint sum_data[data_size];

	GLuint modified_sum = 0;
	GLuint sum			= 0;

	for (GLuint i = 0; i < data_size; ++i)
	{
		destination_data[i]		= 0;
		modified_source_data[i] = data_size - i;
		source_data[i]			= i;

		modified_sum += modified_source_data[i];
		sum += source_data[i];

		modified_sum_data[i] = modified_sum;
		sum_data[i]			 = sum;
	}

	/* Prepare buffers */
	Buffer destination(m_context);
	Buffer source(m_context);

	destination.InitStorage(GL_SHADER_STORAGE_BUFFER, GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
							data_size * sizeof(GLuint), destination_data);

	source.InitStorage(GL_SHADER_STORAGE_BUFFER, GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
					   data_size * sizeof(GLuint), source_data);

	/* Prepare program */
	Program program(m_context);
	program.Init(compute_shader, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);

	/*
	 * - bind buffers to SHADER_STORAGE_BUFFER;
	 * - use MapBufferRange to map both buffers; <access> shall be set as follows:
	 *   * MAP_COHERENT_BIT and MAP_PERSISTENT_BIT flags set for both
	 *   * MAP_WRITE_BIT flag shall be set for source;
	 *   * MAP_READ_BIT flag shall be set for destination;
	 * - dispatch program for 16x1x1 groups;
	 * - modify contents of source buffer via mapped memory;
	 * - execute Finish;
	 * - inspect contents of destination buffer via mapped memory; It is expected
	 * that it will contain results based on original content of source buffer;
	 * - dispatch program for 16x1x1 groups;
	 * - execute Finish;
	 * - inspect contents of destination buffer via mapped memory; It is expected
	 * that it will contain results based on modified content of source buffer.
	 */
	{
		/* Set program */
		Program::Use(gl, program.m_id);

		/* Map buffers */
		destination.Bind();
		const Buffer::MapOwner destination_map(destination.MapRange(
			0 /* offset */, data_size * sizeof(GLuint), GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT));

		source.Bind();
		const Buffer::MapOwner source_map(
			source.MapRange(0 /* offset */, data_size * sizeof(GLuint),
							GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT));

		/* Clear binding point */
		Buffer::Bind(gl, 0, GL_SHADER_STORAGE_BUFFER);

		/* Bind buffers */
		Buffer::BindBase(gl, destination.m_id, GL_SHADER_STORAGE_BUFFER, destination_binding);
		Buffer::BindBase(gl, source.m_id, GL_SHADER_STORAGE_BUFFER, source_binding);

		/* Execute program for 16x1x1 groups */
		gl.dispatchCompute(16, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute with persistently mapped buffers");

		/* Make sure that program executed */
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Finish");

		if (0 != memcmp(destination_map.m_data, sum_data, data_size * sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of mapped region does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
		}

		/* Modify source buffer via mapped area */
		memcpy(source_map.m_data, modified_source_data, data_size * sizeof(GLuint));

		/*
		 * - apply FlushMappedBufferRange to ensure that modifications of source buffer
		 * are visible to server.
		 */
		source.Bind();
		gl.flushMappedBufferRange(GL_SHADER_STORAGE_BUFFER, 0 /* offset */, data_size * sizeof(GLuint));
		GLU_EXPECT_NO_ERROR(gl.getError(), "FlushMappedBufferRange");

		/* Clear binding point */
		Buffer::Bind(gl, 0, GL_SHADER_STORAGE_BUFFER);

		/* Execute program for 16x1x1 groups */
		gl.dispatchCompute(16, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute with persistently mapped buffers");

		/* Make sure that program executed */
		gl.finish();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Finish");

		if (0 != memcmp(destination_map.m_data, modified_sum_data, data_size * sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of mapped region does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
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

/** Constructor
 *
 * @param context Test context
 **/
MapPersistentDrawTest::MapPersistentDrawTest(deqp::Context& context)
	: TestCase(context, "map_persistent_draw", "Test draw operation against mapped buffer")
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP otherwise
 **/
tcu::TestNode::IterateResult MapPersistentDrawTest::iterate()
{
	/*
	 *   * fragment shader should pass value of "gs_fs_color" varying to red
	 *   channel of output color;
	 */
	static const GLchar* fragment_shader = "#version 440 core\n"
										   "\n"
										   "in  float gs_fs_color;\n"
										   "out vec4  fs_out_color;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   "    fs_out_color = vec4(gs_fs_color, 0, 0, 1);\n"
										   "}\n"
										   "\n";

	/*
	 *   * geometry shader should:
	 *     - define single uniform buffer array "rectangles" with unspecified size;
	 *     Rectangles should have two vec2 fields: position and size;
	 *     - define single atomic_uint "atom_color";
	 *     - increment "atom_color" once per execution;
	 *     - output a quad that is placed at rectangles[vs_gs_index].position and
	 *     has size equal rectangles[vs_gs_index].size;
	 *     - define output float varying "gs_fs_color" equal to "atom_color" / 255;
	 */
	static const GLchar* geometry_shader =
		"#version 440 core\n"
		"\n"
		"layout(points)                           in;\n"
		"layout(triangle_strip, max_vertices = 4) out;\n"
		"\n"
		"struct Rectangle {\n"
		"    vec2 position;\n"
		"    vec2 size;\n"
		"};\n"
		"\n"
		"layout (std140, binding = 0) uniform Rectangles {\n"
		"    Rectangle rectangle[2];\n"
		"} rectangles;\n"
		"\n"
		"layout (binding = 0) uniform atomic_uint atom_color;\n"
		"\n"
		"in  uint  vs_gs_index[];\n"
		"out float gs_fs_color;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    const uint  atom_color_value = atomicCounterIncrement(atom_color);\n"
		"    //const uint  atom_color_value = vs_gs_index[0];\n"
		"    const float color            = float(atom_color_value) / 255.0;\n"
		"    //const float color            = rectangles.rectangle[1].size.x;\n"
		"\n"
		"    const float left   = rectangles.rectangle[vs_gs_index[0]].position.x;\n"
		"    const float bottom = rectangles.rectangle[vs_gs_index[0]].position.y;\n"
		"    const float right  = rectangles.rectangle[vs_gs_index[0]].size.x + left;\n"
		"    const float top    = rectangles.rectangle[vs_gs_index[0]].size.y + bottom;\n"
		"\n"
		"    //const float left   = rectangles.rectangle[0].position.x;\n"
		"    //const float bottom = rectangles.rectangle[0].position.y;\n"
		"    //const float right  = rectangles.rectangle[0].size.x + left;\n"
		"    //const float top    = rectangles.rectangle[0].size.y + bottom;\n"
		"\n"
		"    gs_fs_color = color;\n"
		"    gl_Position  = vec4(left, bottom, 0, 1);\n"
		"    EmitVertex();\n"
		"\n"
		"    gs_fs_color = color;\n"
		"    gl_Position  = vec4(left, top, 0, 1);\n"
		"    EmitVertex();\n"
		"\n"
		"    gs_fs_color = color;\n"
		"    gl_Position  = vec4(right, bottom, 0, 1);\n"
		"    EmitVertex();\n"
		"\n"
		"    gs_fs_color = color;\n"
		"    gl_Position  = vec4(right, top, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n"
		"\n";

	/*
	 *   * vertex shader should output single varying "vs_gs_index" of type uint,
	 *   equal to gl_VertexID;
	 */
	static const GLchar* vertex_shader = "#version 440 core\n"
										 "\n"
										 "out uint vs_gs_index;\n"
										 "\n"
										 "void main()\n"
										 "{\n"
										 "    vs_gs_index = gl_VertexID;\n"
										 "}\n"
										 "\n";

	static const GLuint atom_binding		 = 0;
	static const size_t atom_data_size		 = 1 * sizeof(GLuint);
	static const GLuint expected_atom_first  = 3;
	static const GLuint expected_atom_second = 7;
	static const GLuint expected_pixel		 = 0xff000003;
	static const GLuint height				 = 16;
	static const GLuint n_rectangles		 = 2;
	static const GLuint pixel_size			 = 4 * sizeof(GLubyte);
	static const GLuint rectangles_binding   = 0;
	static const size_t rectangle_size		 = 2 * 2 * sizeof(GLfloat); /* 2 * vec2 */
	static const size_t rectangles_data_size = n_rectangles * rectangle_size;
	static const GLuint width				 = 16;
	static const GLuint line_size			 = width * pixel_size;
	static const GLuint pixel_offset		 = 8 * line_size + 7 * pixel_size;
	static const size_t texture_data_size	= height * line_size;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	/* Prepare data */
	GLuint  atom_first_data[1];
	GLuint  atom_second_data[1];
	GLubyte rectangles_first_data[rectangles_data_size];
	GLubyte rectangles_second_data[rectangles_data_size];
	GLubyte texture_data[texture_data_size];

	atom_first_data[0]  = 1;
	atom_second_data[0] = 5;

	{
		GLfloat* ptr = (GLfloat*)rectangles_first_data;

		/* First.position*/
		ptr[0] = -0.5f;
		ptr[1] = -0.5f;

		/* First.size*/
		ptr[2] = 1.0f;
		ptr[3] = 1.0f;

		/* Second.position*/
		ptr[4 + 0] = -0.75f;
		ptr[4 + 1] = -0.75f;

		/* Second.size*/
		ptr[4 + 2] = 1.5f;
		ptr[4 + 3] = 1.5f;
	}

	{
		GLfloat* ptr = (GLfloat*)rectangles_second_data;

		/* First.position*/
		ptr[0] = -1.0f;
		ptr[1] = -1.0f;

		/* First.size*/
		ptr[2] = 0.5f;
		ptr[3] = 0.5f;

		/* Second.position*/
		ptr[4 + 0] = 0.5f;
		ptr[4 + 1] = 0.5f;

		/* Second.size*/
		ptr[4 + 2] = 0.5f;
		ptr[4 + 3] = 0.5f;
	}

	/* Prepare buffers */
	Buffer atom(m_context);
	Buffer rectangles(m_context);

	atom.InitStorage(GL_ATOMIC_COUNTER_BUFFER, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
					 atom_data_size, 0);

	rectangles.InitStorage(GL_UNIFORM_BUFFER, GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT, rectangles_data_size, 0);

	/* Prepare framebuffer */
	Framebuffer framebuffer(m_context);
	Texture		texture(m_context);

	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);
	Texture::Storage(gl, GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, width, height, 0 /* depth */);

	Framebuffer::Generate(gl, framebuffer.m_id);
	Framebuffer::Bind(gl, GL_DRAW_FRAMEBUFFER, framebuffer.m_id);
	Framebuffer::AttachTexture(gl, GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.m_id, width, height);

	/* Prepare VAO */
	VertexArray vao(m_context);

	VertexArray::Generate(gl, vao.m_id);
	VertexArray::Bind(gl, vao.m_id);

	/* Prepare program */
	Program program(m_context);
	program.Init("" /* cs */, fragment_shader, geometry_shader, "" /* tcs */, "" /* tes */, vertex_shader);
	Program::Use(gl, program.m_id);

	/*
	 * - make persistent mapping of both buffers for reads and writes;
	 * - modify "rectangles" buffer via mapped memory with the following two sets
	 *   * position [-0.5,-0.5], size [1.0,1.0],
	 *   * position [-0.25,-0.25], size [1.5,1.5];
	 * - modify "atom_color" buffer via mapped memory to value 1;
	 * - execute MemoryBarrier for CLIENT_MAPPED_BUFFER_BARRIER_BIT;
	 * - enable blending with functions ONE for both source and destination;
	 * - execute DrawArrays for two vertices;
	 * - execute MemoryBarrier for ALL_BARRIER_BITS and Finish;
	 * - inspect contents of:
	 *   * texture - to verify that pixel at 8,8 is filled with RGBA8(3,0,0,0),
	 *   * "atom_color" - to verify that it is equal to 3;
	 * - modify "rectangles" buffer via mapped memory with the following two sets
	 *   * position [-1.0,-1.0], size [0.5,0.5],
	 *   * position [0.5,0.5], size [0.5,0.5];
	 * - modify "atom_color" buffer via mapped memory to value 5;
	 * - execute MemoryBarrier for CLIENT_MAPPED_BUFFER_BARRIER_BIT;
	 * - execute DrawArrays for two vertices;
	 * - execute MemoryBarrier for ALL_BARRIER_BITS and Finish;
	 * - inspect contents of:
	 *   * texture - to verify that pixel at 8,8 is filled with RGBA8(3,0,0,0),
	 *   * "atom_color" - to verify that it is equal to 7;
	 *
	 *  Additionally: change MemoryBarrier to FlushMapped*BufferRange if context supports OpenGL 4.5 Core Profile.
	 */
	{
		/* Choose specification */
		const bool is_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));

		/* Map buffers */
		atom.Bind();
		const Buffer::MapOwner atom_map(atom.MapRange(0 /* offset */, atom_data_size,
													  GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT |
														  (is_gl_45 ? GL_MAP_FLUSH_EXPLICIT_BIT : 0)));

		rectangles.Bind();
		const Buffer::MapOwner rectangles_map(
			rectangles.MapRange(0 /* offset */, rectangles_data_size,
								GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | (is_gl_45 ? GL_MAP_FLUSH_EXPLICIT_BIT : 0)));

		/* Clear binding points */
		Buffer::Bind(gl, 0, GL_ATOMIC_COUNTER_BUFFER);
		Buffer::Bind(gl, 0, GL_UNIFORM_BUFFER);

		/* Bind buffers */
		Buffer::BindBase(gl, atom.m_id, GL_ATOMIC_COUNTER_BUFFER, atom_binding);
		Buffer::BindBase(gl, rectangles.m_id, GL_UNIFORM_BUFFER, rectangles_binding);

		/* Set up blending */
		gl.enable(GL_BLEND);
		gl.blendFunc(GL_ONE, GL_ONE);

		/* Modify buffers */
		memcpy(atom_map.m_data, atom_first_data, atom_data_size);
		memcpy(rectangles_map.m_data, rectangles_first_data, rectangles_data_size);

		/* Execute barrier or flush content. */
		if (is_gl_45)
		{
			gl.flushMappedBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, atom_data_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFlushMappedBufferRange");

			gl.flushMappedBufferRange(GL_UNIFORM_BUFFER, 0, rectangles_data_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFlushMappedBufferRange");
		}
		else
		{
			gl.memoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");
		}

		/* Clear drawbuffer */
		GLint clear_color[4] = { 0, 0, 0, 0 };
		gl.clearBufferiv(GL_COLOR, 0, clear_color);

		/* Execute program for 2 vertices */
		gl.drawArrays(GL_POINTS, 0, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays with persistently mapped buffers");

		/* Execute barrier */
		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

		/* Inspect texture */
		Texture::GetData(gl, GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
		if (0 != memcmp(texture_data + pixel_offset, &expected_pixel, pixel_size))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of framebuffer does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
			tcu::ConstPixelBufferAccess img(
				tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), width, height,
				1 /* depth */, texture_data);
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Image("Framebuffer", "Framebuffer contents using initial buffer data", img);
		}

		/* Inspect atom */
		if (0 != memcmp(atom_map.m_data, &expected_atom_first, sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of ATOMIC_COUNTER buffer are invalid."
												<< tcu::TestLog::EndMessage;
		}

		/* Modify buffers */
		memcpy(atom_map.m_data, atom_second_data, atom_data_size);
		memcpy(rectangles_map.m_data, rectangles_second_data, rectangles_data_size);

		/* Execute barrier or flush content. */
		if (is_gl_45)
		{
			gl.flushMappedBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, atom_data_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFlushMappedBufferRange");

			gl.flushMappedBufferRange(GL_UNIFORM_BUFFER, 0, rectangles_data_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFlushMappedBufferRange");
		}
		else
		{
			gl.memoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");
		}

		/* Execute program for 2 vertices */
		gl.drawArrays(GL_POINTS, 0, 2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays with persistently mapped buffers");

		/* Execute barrier */
		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier");

		/* Inspect texture */
		Texture::GetData(gl, GL_TEXTURE_2D, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
		if (0 != memcmp(texture_data + pixel_offset, &expected_pixel, pixel_size))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of framebuffer does not correspond with expected results"
												<< tcu::TestLog::EndMessage;
			tcu::ConstPixelBufferAccess img(
				tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), width, height,
				1 /* depth */, texture_data);
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Image("Framebuffer", "Framebuffer contents using updated buffer data", img);
		}

		/* Inspect atom */
		if (0 != memcmp(atom_map.m_data, &expected_atom_second, sizeof(GLuint)))
		{
			test_result = false;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Contents of ATOMIC_COUNTER buffer are invalid."
												<< tcu::TestLog::EndMessage;
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
} /* BufferStorage */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
BufferStorageTests::BufferStorageTests(deqp::Context& context)
	: TestCaseGroup(context, "buffer_storage", "Verifies \"buffer storage\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void BufferStorageTests::init(void)
{
	addChild(new BufferStorage::ErrorsTest(m_context));
	addChild(new BufferStorage::GetBufferParameterTest(m_context));
	addChild(new BufferStorage::DynamicStorageTest(m_context));
	addChild(new BufferStorage::MapPersistentBufferSubDataTest(m_context));
	addChild(new BufferStorage::MapPersistentTextureTest(m_context));
	addChild(new BufferStorage::MapPersistentReadPixelsTest(m_context));
	addChild(new BufferStorage::MapPersistentDispatchTest(m_context));
	addChild(new BufferStorage::MapPersistentFlushTest(m_context));
	addChild(new BufferStorage::MapPersistentDrawTest(m_context));
}
} /* gl4cts namespace */
