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

#include "es31cShaderAtomicCountersTests.hpp"
#include "gluContextInfo.hpp"
#include "gluPixelTransfer.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuVector.hpp"
#include <assert.h>
#include <cstdarg>
#include <map>

namespace glcts
{
using namespace glw;
using tcu::Vec4;
using tcu::UVec4;

namespace
{

class SACSubcaseBase : public glcts::SubcaseBase
{
public:
	virtual std::string Title()
	{
		return NL "";
	}
	virtual std::string Purpose()
	{
		return NL "";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	virtual ~SACSubcaseBase()
	{
	}

	bool CheckProgram(GLuint program)
	{
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		GLint length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		if (length > 1)
		{
			std::vector<GLchar> log(length);
			glGetProgramInfoLog(program, length, NULL, &log[0]);
			m_context.getTestContext().getLog() << tcu::TestLog::Message << &log[0] << tcu::TestLog::EndMessage;
		}
		return status == GL_TRUE;
	}

	int getWindowWidth()
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		return renderTarget.getWidth();
	}

	int getWindowHeight()
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		return renderTarget.getHeight();
	}

	long ValidateReadBuffer(const Vec4& expected)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		int						 viewportW	= renderTarget.getWidth();
		int						 viewportH	= renderTarget.getHeight();
		tcu::Surface			 renderedFrame(viewportW, viewportH);
		tcu::Surface			 referenceFrame(viewportW, viewportH);

		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		for (int y = 0; y < viewportH; ++y)
		{
			for (int x = 0; x < viewportW; ++x)
			{
				referenceFrame.setPixel(
					x, y, tcu::RGBA(static_cast<int>(expected[0] * 255), static_cast<int>(expected[1] * 255),
									static_cast<int>(expected[2] * 255), static_cast<int>(expected[3] * 255)));
			}
		}
		tcu::TestLog& log = m_context.getTestContext().getLog();
		bool isOk = tcu::fuzzyCompare(log, "Result", "Image comparison result", referenceFrame, renderedFrame, 0.05f,
									  tcu::COMPARE_LOG_RESULT);
		return (isOk ? NO_ERROR : ERROR);
	}

	void LinkProgram(GLuint program)
	{
		glLinkProgram(program);
		GLsizei length;
		GLchar  log[1024];
		glGetProgramInfoLog(program, sizeof(log), &length, log);
		if (length > 1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
												<< log << tcu::TestLog::EndMessage;
		}
	}

	GLuint CreateProgram(const char* src_vs, const char* src_fs, bool link)
	{
		const GLuint p = glCreateProgram();

		if (src_vs)
		{
			GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_vs, NULL);
			glCompileShader(sh);
		}
		if (src_fs)
		{
			GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_fs, NULL);
			glCompileShader(sh);
		}
		if (link)
		{
			LinkProgram(p);
		}
		return p;
	}

	GLuint CreateShaderProgram(GLenum type, GLsizei count, const GLchar** strings)
	{
		GLuint program = glCreateShaderProgramv(type, count, strings);
		GLint  status  = GL_TRUE;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLsizei length;
			GLchar  log[1024];
			glGetProgramInfoLog(program, sizeof(log), &length, log);
			if (length > 1)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
													<< log << tcu::TestLog::EndMessage;
			}
		}
		return program;
	}

	void CreateQuad(GLuint* vao, GLuint* vbo, GLuint* ebo)
	{
		assert(vao && vbo);

		// interleaved data (vertex, color0 (green), color1 (blue), color2 (red))
		const float v[] = {
			-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f,
			0.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
			1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  0.0f,
		};
		glGenBuffers(1, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, *vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (ebo)
		{
			std::vector<GLushort> index_data(4);
			for (int i = 0; i < 4; ++i)
			{
				index_data[i] = static_cast<GLushort>(i);
			}
			glGenBuffers(1, ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 4, &index_data[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, vao);
		glBindVertexArray(*vao);
		glBindBuffer(GL_ARRAY_BUFFER, *vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 2));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 5));
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 8));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		if (ebo)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
		}
		glBindVertexArray(0);
	}

	void CreateTriangle(GLuint* vao, GLuint* vbo, GLuint* ebo)
	{
		assert(vao && vbo);

		// interleaved data (vertex, color0 (green), color1 (blue), color2 (red))
		const float v[] = {
			-1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 3.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f,  1.0f,  1.0f, 0.0f, 0.0f, -1.0f, 3.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f,
		};
		glGenBuffers(1, vbo);
		glBindBuffer(GL_ARRAY_BUFFER, *vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (ebo)
		{
			std::vector<GLushort> index_data(3);
			for (int i = 0; i < 3; ++i)
			{
				index_data[i] = static_cast<GLushort>(i);
			}
			glGenBuffers(1, ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 4, &index_data[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, vao);
		glBindVertexArray(*vao);
		glBindBuffer(GL_ARRAY_BUFFER, *vbo);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 11, 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 2));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 5));
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11, reinterpret_cast<void*>(sizeof(float) * 8));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		if (ebo)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
		}
		glBindVertexArray(0);
	}

	const char* GLenumToString(GLenum e)
	{
		switch (e)
		{
		case GL_ATOMIC_COUNTER_BUFFER_BINDING:
			return "GL_ATOMIC_COUNTER_BUFFER_BINDING";
		case GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS:
			return "GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS";
		case GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS:
			return "GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS";
		case GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS:
			return "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS";
		case GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS:
			return "GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS";

		case GL_MAX_VERTEX_ATOMIC_COUNTERS:
			return "GL_MAX_VERTEX_ATOMIC_COUNTERS";
		case GL_MAX_COMPUTE_ATOMIC_COUNTERS:
			return "GL_MAX_GEOMETRY_ATOMIC_COUNTERS";
		case GL_MAX_FRAGMENT_ATOMIC_COUNTERS:
			return "GL_MAX_FRAGMENT_ATOMIC_COUNTERS";
		case GL_MAX_COMBINED_ATOMIC_COUNTERS:
			return "GL_MAX_COMBINED_ATOMIC_COUNTERS";

		case GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE:
			return "GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE";
		case GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS:
			return "GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS";

		default:
			assert(0);
			break;
		}
		return NULL;
	}

	bool CheckMaxValue(GLenum e, GLint expected)
	{
		bool ok = true;

		GLint i;
		glGetIntegerv(e, &i);
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << GLenumToString(e) << " = " << i << tcu::TestLog::EndMessage;

		if (i < expected)
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << GLenumToString(e) << " state is incorrect (GetIntegerv, is: " << i
				<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLint64 i64;
		glGetInteger64v(e, &i64);
		if (i64 < static_cast<GLint64>(expected))
		{
			ok = false;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << GLenumToString(e)
												<< " state is incorrect (GetInteger64v, is: " << static_cast<GLint>(i64)
												<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLfloat f;
		glGetFloatv(e, &f);
		if (f < static_cast<GLfloat>(expected))
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << GLenumToString(e) << " state is incorrect (GetFloatv, is: " << f
				<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLboolean b;
		glGetBooleanv(e, &b);

		return ok;
	}

	bool CheckGetCommands(GLenum e, GLint expected)
	{
		bool ok = true;

		GLint i;
		glGetIntegerv(e, &i);
		if (i != expected)
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << GLenumToString(e) << " state is incorrect (GetIntegerv, is: " << i
				<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLint64 i64;
		glGetInteger64v(e, &i64);
		if (i64 != static_cast<GLint64>(expected))
		{
			ok = false;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << GLenumToString(e)
												<< " state is incorrect (GetInteger64v, is: " << static_cast<GLint>(i64)
												<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLfloat f;
		glGetFloatv(e, &f);
		if (f != static_cast<GLfloat>(expected))
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << GLenumToString(e) << " state is incorrect (GetFloatv, is: " << f
				<< ", expected: " << expected << ")" << tcu::TestLog::EndMessage;
		}

		GLboolean b;
		glGetBooleanv(e, &b);
		if (b != (expected ? GL_TRUE : GL_FALSE))
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << GLenumToString(e) << " state is incorrect (GetBooleanv, is: " << b
				<< ", expected: " << (expected ? GL_TRUE : GL_FALSE) << ")" << tcu::TestLog::EndMessage;
		}

		return ok;
	}

	bool CheckBufferBindingState(GLuint index, GLint binding, GLint64 start, GLint64 size)
	{
		bool ok = true;

		GLint i;
		glGetIntegeri_v(GL_ATOMIC_COUNTER_BUFFER_BINDING, index, &i);
		if (i != binding)
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GL_ATOMIC_COUNTER_BUFFER_BINDING state is incorrect (GetIntegeri_v, is: " << i
				<< ", expected: " << binding << ", index: " << index << tcu::TestLog::EndMessage;
		}

		GLint64 i64;
		glGetInteger64i_v(GL_ATOMIC_COUNTER_BUFFER_BINDING, index, &i64);
		if (i64 != static_cast<GLint64>(binding))
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_ATOMIC_COUNTER_BUFFER_BINDING state is incorrect (GetInteger64i_v, is: "
				<< static_cast<GLint>(i64) << ", expected: " << binding << ", index: " << index
				<< tcu::TestLog::EndMessage;
		}

		glGetInteger64i_v(GL_ATOMIC_COUNTER_BUFFER_START, index, &i64);
		if (i64 != start)
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GL_ATOMIC_COUNTER_BUFFER_START state is incorrect (GetInteger64i_v, is: " << static_cast<GLint>(i64)
				<< ", expected: " << static_cast<GLint>(start) << ", index: " << index << tcu::TestLog::EndMessage;
		}
		glGetInteger64i_v(GL_ATOMIC_COUNTER_BUFFER_SIZE, index, &i64);
		if (i64 != size && i64 != 0)
		{
			ok = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GL_ATOMIC_COUNTER_BUFFER_SIZE state is incorrect (GetInteger64i_v, is: " << static_cast<GLint>(i64)
				<< ", expected: (" << static_cast<GLint>(size) << " or 0), index: " << index
				<< tcu::TestLog::EndMessage;
		}

		return ok;
	}

	bool CheckUniform(GLuint prog, const GLchar* uniform_name, GLuint uniform_index, GLint uniform_type,
					  GLint uniform_size, GLint uniform_offset, GLint uniform_array_stride)
	{
		bool ok = true;

		GLuint index;
		glGetUniformIndices(prog, 1, &uniform_name, &index);
		if (index != uniform_index)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name
				<< ": Bad index returned by glGetUniformIndices." << tcu::TestLog::EndMessage;
			ok = false;
		}

		const GLsizei uniform_length = static_cast<GLsizei>(strlen(uniform_name));

		GLsizei length;
		GLint   size;
		GLenum  type;
		GLchar  name[32];

		glGetProgramResourceName(prog, GL_UNIFORM, uniform_index, sizeof(name), &length, name);
		if (length != uniform_length)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": Length is " << length << " should be "
				<< uniform_length << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniform(prog, uniform_index, sizeof(name), &length, &size, &type, name);
		if (strcmp(name, uniform_name))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": Bad name returned by glGetActiveUniform."
				<< tcu::TestLog::EndMessage;
			ok = false;
		}
		if (length != uniform_length)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": Length is " << length << " should be "
				<< uniform_length << tcu::TestLog::EndMessage;
			ok = false;
		}
		if (size != uniform_size)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name << ": Size is "
												<< size << " should be " << uniform_size << tcu::TestLog::EndMessage;
			ok = false;
		}
		if (type != static_cast<GLenum>(uniform_type))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name << ": Type is "
												<< type << " should be " << uniform_type << tcu::TestLog::EndMessage;
			ok = false;
		}

		GLint param;
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_TYPE, &param);
		if (param != uniform_type)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name << ": Type is "
												<< param << " should be " << uniform_type << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_SIZE, &param);
		if (param != uniform_size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_SIZE is " << param
				<< " should be " << uniform_size << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_NAME_LENGTH, &param);
		if (param != (uniform_length + 1))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_NAME_LENGTH is " << param
				<< " should be " << (uniform_length + 1) << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_BLOCK_INDEX, &param);
		if (param != -1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform: " << uniform_name
												<< ": GL_UNIFORM_BLOCK_INDEX should be -1." << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_OFFSET, &param);
		if (param != uniform_offset)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_OFFSET is " << param
				<< " should be " << uniform_offset << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_ARRAY_STRIDE, &param);
		if (param != uniform_array_stride)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_ARRAY_STRIDE is " << param
				<< " should be " << uniform_array_stride << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_MATRIX_STRIDE, &param);
		if (param != 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_MATRIX_STRIDE should be 0 is "
				<< param << tcu::TestLog::EndMessage;
			ok = false;
		}
		glGetActiveUniformsiv(prog, 1, &uniform_index, GL_UNIFORM_IS_ROW_MAJOR, &param);
		if (param != 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform: " << uniform_name << ": GL_UNIFORM_MATRIX_STRIDE should be 0 is "
				<< param << tcu::TestLog::EndMessage;
			ok = false;
		}

		return ok;
	}

	bool CheckCounterValues(GLuint size, GLuint* values, GLuint min_value)
	{
		std::sort(values, values + size);
		for (GLuint i = 0; i < size; ++i)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << values[i] << tcu::TestLog::EndMessage;
			if (values[i] != i + min_value)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Counter value is " << values[i]
													<< " should be " << i + min_value << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	bool CheckCounterValues(GLuint size, UVec4* data, GLuint min_value)
	{
		std::vector<GLuint> values(size);
		for (GLuint j = 0; j < size; ++j)
		{
			values[j] = data[j].x();
		}
		std::sort(values.begin(), values.end());
		for (GLuint i = 0; i < size; ++i)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << values[i] << tcu::TestLog::EndMessage;
			if (values[i] != i + min_value)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Counter value is " << values[i]
													<< " should be " << i + min_value << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	bool CheckFinalCounterValue(GLuint buffer, GLintptr offset, GLuint expected_value)
	{
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer);
		GLuint* value = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, offset, 4, GL_MAP_READ_BIT));
		if (value[0] != expected_value)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Counter value is " << value
												<< " should be " << expected_value << tcu::TestLog::EndMessage;
			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
			return false;
		}
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		return true;
	}
};

class Buffer : public glcts::GLWrapper
{
public:
	Buffer()
		: size_(0)
		, usage_(GL_STATIC_DRAW)
		, access_(GL_WRITE_ONLY)
		, access_flags_(0)
		, mapped_(GL_FALSE)
		, map_pointer_(NULL)
		, map_offset_(0)
		, map_length_(0)
	{
		glGenBuffers(1, &name_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, name_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	}
	~Buffer()
	{
		glDeleteBuffers(1, &name_);
	}
	GLuint name() const
	{
		return name_;
	}
	long Verify()
	{
		GLint   i;
		GLint64 i64;

		glGetBufferParameteri64v(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_SIZE, &i64);
		if (i64 != size_)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BUFFER_SIZE is " << static_cast<GLint>(i64) << " should be "
				<< static_cast<GLint>(size_) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetBufferParameteriv(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_USAGE, &i);
		if (i != static_cast<GLint>(usage_))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "BUFFER_USAGE is " << i << " should be "
												<< usage_ << tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (this->m_context.getContextInfo().isExtensionSupported("GL_OES_mapbuffer"))
		{
			glGetBufferParameteriv(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_ACCESS, &i);
			if (i != static_cast<GLint>(access_))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "BUFFER_ACCESS is " << i
													<< " should be " << access_ << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		else
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_OES_mapbuffer not supported, skipping GL_BUFFER_ACCESS enum"
				<< tcu::TestLog::EndMessage;
		}
		glGetBufferParameteriv(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_ACCESS_FLAGS, &i);
		if (i != access_flags_)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "BUFFER_ACCESS_FLAGS is " << i
												<< " should be " << access_flags_ << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetBufferParameteriv(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_MAPPED, &i);
		if (i != mapped_)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "BUFFER_MAPPED is " << i << " should be "
												<< mapped_ << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetBufferParameteri64v(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_MAP_OFFSET, &i64);
		if (i64 != map_offset_)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BUFFER_MAP_OFFSET is " << static_cast<GLint>(i64) << " should be "
				<< static_cast<GLint>(map_offset_) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetBufferParameteri64v(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_MAP_LENGTH, &i64);
		if (i64 != map_length_)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BUFFER_MAP_LENGTH is " << static_cast<GLint>(i64) << " should be "
				<< static_cast<GLint>(map_length_) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		void* ptr;
		glGetBufferPointerv(GL_ATOMIC_COUNTER_BUFFER, GL_BUFFER_MAP_POINTER, &ptr);
		if (ptr != map_pointer_)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BUFFER_MAP_POINTER is " << reinterpret_cast<long>(static_cast<int*>(ptr))
				<< " should be " << reinterpret_cast<long>(static_cast<int*>(map_pointer_)) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
	void Data(GLsizeiptr size, const void* data, GLenum usage)
	{
		size_  = size;
		usage_ = usage;
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, size, data, usage);
	}
	void* MapRange(GLintptr offset, GLsizeiptr length, GLbitfield access)
	{
		assert(mapped_ == GL_FALSE);

		map_pointer_ = glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, offset, length, access);
		if (map_pointer_)
		{
			map_offset_   = offset;
			map_length_   = length;
			access_flags_ = access;
			if (access & GL_MAP_READ_BIT)
				access_ = GL_READ_ONLY;
			else if (access & GL_MAP_WRITE_BIT)
				access_ = GL_WRITE_ONLY;
			mapped_		= GL_TRUE;
		}
		return map_pointer_;
	}
	GLboolean Unmap()
	{
		assert(mapped_ == GL_TRUE);

		if (glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER))
		{
			map_offset_   = 0;
			map_length_   = 0;
			map_pointer_  = 0;
			mapped_		  = GL_FALSE;
			access_		  = GL_WRITE_ONLY;
			access_flags_ = 0;
			return GL_TRUE;
		}
		return GL_FALSE;
	}

private:
	GLuint	name_;
	GLint64   size_;
	GLenum	usage_;
	GLenum	access_;
	GLint	 access_flags_;
	GLboolean mapped_;
	void*	 map_pointer_;
	GLint64   map_offset_;
	GLint64   map_length_;
};
}

class BasicUsageCS : public SACSubcaseBase
{
public:
	virtual std::string Title()
	{
		return NL "Atomic Counters usage in the Compute Shader stage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters work as expected in the Compute Shader stage." NL
				  "In particular make sure that values returned by GLSL built-in functions" NL
				  "atomicCounterIncrement and atomicCounterDecrement are unique in every shader invocation." NL
				  "Also make sure that the final values in atomic counter buffer objects are as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint prog_;
	GLuint m_buffer;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		prog_			= 0;
		m_buffer		= 0;
		return NO_ERROR;
	}

	GLuint CreateComputeProgram(const std::string& cs)
	{
		const GLuint p = glCreateProgram();

		const char* const kGLSLVer = "#version 310 es\n";

		if (!cs.empty())
		{
			const GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, cs.c_str() };
			glShaderSource(sh, 2, src, NULL);
			glCompileShader(sh);
		}

		return p;
	}

	bool CheckProgram(GLuint program, bool* compile_error = NULL)
	{
		GLint compile_status = GL_TRUE;
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);

		if (status == GL_FALSE)
		{
			GLint attached_shaders;
			glGetProgramiv(program, GL_ATTACHED_SHADERS, &attached_shaders);

			if (attached_shaders > 0)
			{
				std::vector<GLuint> shaders(attached_shaders);
				glGetAttachedShaders(program, attached_shaders, NULL, &shaders[0]);

				for (GLint i = 0; i < attached_shaders; ++i)
				{
					GLenum type;
					glGetShaderiv(shaders[i], GL_SHADER_TYPE, reinterpret_cast<GLint*>(&type));
					switch (type)
					{
					case GL_VERTEX_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Vertex Shader ***" << tcu::TestLog::EndMessage;
						break;
					case GL_TESS_CONTROL_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Tessellation Control Shader ***"
							<< tcu::TestLog::EndMessage;
						break;
					case GL_TESS_EVALUATION_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Tessellation Evaluation Shader ***"
							<< tcu::TestLog::EndMessage;
						break;
					case GL_GEOMETRY_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Geometry Shader ***" << tcu::TestLog::EndMessage;
						break;
					case GL_FRAGMENT_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Fragment Shader ***" << tcu::TestLog::EndMessage;
						break;
					case GL_COMPUTE_SHADER:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Compute Shader ***" << tcu::TestLog::EndMessage;
						break;
					default:
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "*** Unknown Shader ***" << tcu::TestLog::EndMessage;
						break;
					}

					GLint res;
					glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &res);
					if (res != GL_TRUE)
						compile_status = res;

					// shader source
					GLint length;
					glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &length);
					if (length > 0)
					{
						std::vector<GLchar> source(length);
						glGetShaderSource(shaders[i], length, NULL, &source[0]);
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << &source[0] << tcu::TestLog::EndMessage;
					}

					// shader info log
					glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &length);
					if (length > 0)
					{
						std::vector<GLchar> log(length);
						glGetShaderInfoLog(shaders[i], length, NULL, &log[0]);
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << &log[0] << tcu::TestLog::EndMessage;
					}
				}
			}

			// program info log
			GLint length;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
			if (length > 0)
			{
				std::vector<GLchar> log(length);
				glGetProgramInfoLog(program, length, NULL, &log[0]);
				m_context.getTestContext().getLog() << tcu::TestLog::Message << &log[0] << tcu::TestLog::EndMessage;
			}
		}

		if (compile_error)
			*compile_error = (compile_status == GL_TRUE ? false : true);
		if (compile_status != GL_TRUE)
			return false;
		return status == GL_TRUE ? true : false;
	}

	virtual long Run()
	{
		// create program
		const char* const glsl_cs = NL
			"layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			"layout(binding = 0, offset = 0) uniform atomic_uint ac_counter_inc;" NL
			"layout(binding = 0, offset = 4) uniform atomic_uint ac_counter_dec;" NL "layout(std430) buffer Output {" NL
			"  mediump uint data_inc[256];" NL "  mediump uint data_dec[256];" NL "} g_out;" NL "void main() {" NL
			"  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			"  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter_inc);" NL
			"  g_out.data_dec[offset] = atomicCounterDecrement(ac_counter_dec);" NL "}";
		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr++ = 0;
		*ptr++ = 256;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 512 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(prog_);
		glDispatchCompute(4, 1, 1);

		long	error = NO_ERROR;
		GLuint* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data = static_cast<GLuint*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 512 * sizeof(GLuint), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		std::sort(data, data + 512);
		for (int i = 0; i < 512; i += 2)
		{
			if (data[i] != data[i + 1])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Pair of values should be equal, got: " << data[i] << ", "
					<< data[i + 1] << tcu::TestLog::EndMessage;
				error = ERROR;
			}
			if (i < 510 && data[i] == data[i + 2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Too many same values found: " << data[i] << ", index: " << i
					<< tcu::TestLog::EndMessage;
				error = ERROR;
			}
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &m_buffer);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class BasicBufferOperations : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counter Buffer - basic operations";
	}
	virtual std::string Purpose()
	{
		return NL
			"Verify that basic buffer operations work as expected with new buffer target." NL
			"Tested commands: BindBuffer, BufferData, BufferSubData, MapBuffer, MapBufferRange, UnmapBuffer and" NL
			"GetBufferSubData.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint buffer_;

	virtual long Setup()
	{
		buffer_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		glGenBuffers(1, &buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8 * 4, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		GLuint* ptr = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8 * 4, GL_MAP_WRITE_BIT));
		if (ptr == NULL)
		{
			return ERROR;
		}
		for (GLuint i = 0; i < 8; ++i)
			ptr[i]	= i;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		long	res = NO_ERROR;
		GLuint* data;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		data = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 32, GL_MAP_READ_BIT));
		if (data == NULL)
		{
			return ERROR;
		}
		for (GLuint i = 0; i < 8; ++i)
		{
			if (data[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "data[" << i << "] is: " << data[i]
													<< " should be: " << i << tcu::TestLog::EndMessage;
				res = ERROR;
			}
		}
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		if (res != NO_ERROR)
			return res;

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		ptr = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 32, GL_MAP_WRITE_BIT));
		if (ptr == NULL)
		{
			return ERROR;
		}
		for (GLuint i = 0; i < 8; ++i)
			ptr[i]	= i * 2;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		data = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 32, GL_MAP_READ_BIT));
		if (data == NULL)
		{
			return ERROR;
		}
		for (GLuint i = 0; i < 8; ++i)
		{
			if (data[i] != i * 2)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "data[" << i << "] is: " << data[i]
													<< " should be: " << i * 2 << tcu::TestLog::EndMessage;
				res = ERROR;
			}
		}
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		GLuint data2[8];
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);
		for (GLuint i = 0; i < 8; ++i)
			data2[i]  = i * 3;
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 32, data2);
		for (GLuint i = 0; i < 8; ++i)
			data2[i]  = 0;
		data		  = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 32, GL_MAP_READ_BIT));
		for (GLuint i = 0; i < 8; ++i)
		{
			if (data[i] != i * 3)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "data[" << i << "] is: " << data[i]
													<< " should be: " << i * 3 << tcu::TestLog::EndMessage;
				res = ERROR;
			}
		}
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		return res;
	}
	virtual long Cleanup()
	{
		glDeleteBuffers(1, &buffer_);
		return NO_ERROR;
	}
};

class BasicBufferState : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counter Buffer - state";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that setting and getting buffer state works as expected for new buffer target.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	virtual long Run()
	{
		Buffer buffer;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer.name());

		if (buffer.Verify() != NO_ERROR)
			return ERROR;

		buffer.Data(100, NULL, GL_DYNAMIC_COPY);
		if (buffer.Verify() != NO_ERROR)
			return ERROR;

		buffer.MapRange(10, 50, GL_MAP_WRITE_BIT);
		if (buffer.Verify() != NO_ERROR)
			return ERROR;
		buffer.Unmap();
		if (buffer.Verify() != NO_ERROR)
			return ERROR;

		return NO_ERROR;
	}
};

class BasicBufferBind : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counter Buffer - binding";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that binding buffer objects to ATOMIC_COUNTER_BUFFER (indexed) target" NL
				  "works as expected. In particualr make sure that binding with BindBufferBase and BindBufferRange" NL
				  "also bind to generic binding point and deleting buffer that is currently bound unbinds it. Tested" NL
				  "commands: BindBuffer, BindBufferBase and BindBufferRange.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint buffer_;

	virtual long Setup()
	{
		buffer_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		GLint bindings;
		glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &bindings);
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "MAX_ATOMIC_COUNTER_BUFFER_BINDINGS: " << bindings << tcu::TestLog::EndMessage;

		if (!CheckGetCommands(GL_ATOMIC_COUNTER_BUFFER_BINDING, 0))
			return ERROR;
		for (GLint index = 0; index < bindings; ++index)
		{
			if (!CheckBufferBindingState(static_cast<GLuint>(index), 0, 0, 0))
				return ERROR;
		}

		glGenBuffers(1, &buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_);

		if (!CheckGetCommands(GL_ATOMIC_COUNTER_BUFFER_BINDING, static_cast<GLint>(buffer_)))
			return ERROR;
		for (GLint index = 0; index < bindings; ++index)
		{
			if (!CheckBufferBindingState(static_cast<GLuint>(index), 0, 0, 0))
				return ERROR;
		}

		long res = NO_ERROR;

		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 1000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffer_);
		if (!CheckBufferBindingState(0, static_cast<GLint>(buffer_), 0, 1000))
			res = ERROR;
		if (!CheckGetCommands(GL_ATOMIC_COUNTER_BUFFER_BINDING, static_cast<GLint>(buffer_)))
			res = ERROR;

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, static_cast<GLuint>(bindings / 2), buffer_);
		if (!CheckBufferBindingState(static_cast<GLuint>(bindings / 2), static_cast<GLint>(buffer_), 0, 1000))
			res = ERROR;

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, static_cast<GLuint>(bindings - 1), buffer_);
		if (!CheckBufferBindingState(static_cast<GLuint>(bindings - 1), static_cast<GLint>(buffer_), 0, 1000))
			res = ERROR;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, buffer_, 8, 32);
		if (!CheckBufferBindingState(0, static_cast<GLint>(buffer_), 8, 32))
			res = ERROR;
		if (!CheckGetCommands(GL_ATOMIC_COUNTER_BUFFER_BINDING, static_cast<GLint>(buffer_)))
			res = ERROR;

		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, static_cast<GLuint>(bindings / 2), buffer_, 512, 100);
		if (!CheckBufferBindingState(static_cast<GLuint>(bindings / 2), static_cast<GLint>(buffer_), 512, 100))
			res = ERROR;

		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, static_cast<GLuint>(bindings - 1), buffer_, 12, 128);
		if (!CheckBufferBindingState(static_cast<GLuint>(bindings - 1), static_cast<GLint>(buffer_), 12, 128))
			res = ERROR;

		glDeleteBuffers(1, &buffer_);
		buffer_ = 0;

		GLint i;
		glGetIntegerv(GL_ATOMIC_COUNTER_BUFFER_BINDING, &i);
		if (i != 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Generic binding point should be 0 after deleting bound buffer object."
				<< tcu::TestLog::EndMessage;
			res = ERROR;
		}
		for (GLint index = 0; index < bindings; ++index)
		{
			glGetIntegeri_v(GL_ATOMIC_COUNTER_BUFFER_BINDING, static_cast<GLuint>(index), &i);
			if (i != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Binding point " << index
					<< " should be 0 after deleting bound buffer object." << tcu::TestLog::EndMessage;
				res = ERROR;
			}
		}

		return res;
	}
	virtual long Cleanup()
	{
		glDeleteBuffers(1, &buffer_);
		return NO_ERROR;
	}
};

class BasicProgramMax : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Program - max values";
	}
	virtual std::string Purpose()
	{
		return NL "Verify all max values which deal with atomic counter buffers.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	virtual long Run()
	{
		if (!CheckMaxValue(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, 1))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, 32))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, 1))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_COMBINED_ATOMIC_COUNTERS, 8))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, 0))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_VERTEX_ATOMIC_COUNTERS, 0))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, 1))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_COMPUTE_ATOMIC_COUNTERS, 8))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, 0))
			return ERROR;
		if (!CheckMaxValue(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, 0))
			return ERROR;
		return NO_ERROR;
	}
};

class BasicProgramQuery : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "Program - atomic counters queries";
	}
	virtual std::string Purpose()
	{
		return NL "Get all the information from the program object about atomic counters." NL
				  "Verify that all informations are correct. Tested commands:" NL
				  "GetProgramiv and GetUniform* with new enums.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_, m_buffer;
	GLuint prog_;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		m_buffer		= 0;
		prog_			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{

		// create program
		const char* glsl_cs =
			NL "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;" NL "layout(std430) buffer Output {" NL
			   "  mediump vec4 data;" NL "} g_out;" NL
			   "layout(binding = 0, offset = 0)  uniform atomic_uint ac_counter0;" NL
			   "layout(binding = 0, offset = 4)  uniform atomic_uint ac_counter1;" NL
			   "layout(binding = 0)              uniform atomic_uint ac_counter2;" NL
			   "layout(binding = 0)              uniform atomic_uint ac_counter67[2];" NL
			   "layout(binding = 0)              uniform atomic_uint ac_counter3;" NL
			   "layout(binding = 0)              uniform atomic_uint ac_counter4;" NL
			   "layout(binding = 0)              uniform atomic_uint ac_counter5;" NL "void main() {" NL
			   "  mediump uint c = 0u;" NL "  c += atomicCounterIncrement(ac_counter0);" NL
			   "  c += atomicCounterIncrement(ac_counter1);" NL "  c += atomicCounterIncrement(ac_counter2);" NL
			   "  c += atomicCounterIncrement(ac_counter3);" NL "  c += atomicCounterIncrement(ac_counter4);" NL
			   "  c += atomicCounterIncrement(ac_counter5);" NL "  c += atomicCounterIncrement(ac_counter67[0]);" NL
			   "  c += atomicCounterIncrement(ac_counter67[1]);" NL
			   "  if (c > 10u) g_out.data = vec4(0.0, 1.0, 0.0, 1.0);" NL
			   "  else g_out.data = vec4(1.0, float(c), 0.0, 1.0);" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;
		glUseProgram(prog_);

		// get active buffers
		GLuint active_buffers;
		glGetProgramiv(prog_, GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, reinterpret_cast<GLint*>(&active_buffers));
		if (active_buffers != 1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_ACTIVE_ATOMIC_COUNTER_BUFFERS is "
												<< active_buffers << " should be 1." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		// get active uniforms
		std::map<std::string, GLuint> uniforms_name_index;
		GLuint active_uniforms;
		glGetProgramiv(prog_, GL_ACTIVE_UNIFORMS, reinterpret_cast<GLint*>(&active_uniforms));
		if (active_uniforms != 7)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_ACTIVE_UNIFORMS is " << active_uniforms
												<< " should be 8." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		for (GLuint index = 0; index < active_uniforms; ++index)
		{
			GLchar name[32];
			glGetProgramResourceName(prog_, GL_UNIFORM, index, sizeof(name), NULL, name);
			uniforms_name_index.insert(std::make_pair(name, index));
		}

		if (!CheckUniform(prog_, "ac_counter0", uniforms_name_index["ac_counter0"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  0, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter1", uniforms_name_index["ac_counter1"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  4, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter2", uniforms_name_index["ac_counter2"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  8, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter3", uniforms_name_index["ac_counter3"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  20, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter4", uniforms_name_index["ac_counter4"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  24, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter5", uniforms_name_index["ac_counter5"], GL_UNSIGNED_INT_ATOMIC_COUNTER, 1,
						  28, 0))
			return ERROR;
		if (!CheckUniform(prog_, "ac_counter67[0]", uniforms_name_index["ac_counter67[0]"],
						  GL_UNSIGNED_INT_ATOMIC_COUNTER, 2, 12, 4))
			return ERROR;

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		const unsigned int data[8] = { 20, 20, 20, 20, 20, 20, 20, 20 };
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDispatchCompute(1, 1, 1);

		long  error = NO_ERROR;
		Vec4* data_out;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
		data_out = static_cast<Vec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Vec4), GL_MAP_READ_BIT));
		if (data_out[0].x() != 0.0 || data_out[0].y() != 1.0 || data_out[0].z() != 0.0 || data_out[0].w() != 1.0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected vec4(0, 1, 0, 1) in the buffer, got: " << data_out[0].x() << " "
				<< data_out[0].y() << " " << data_out[0].z() << " " << data_out[0].w() << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &m_buffer);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class BasicUsageSimple : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "Simple Use Case";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that simple usage of atomic counters work as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint storage_buffer_;
	GLuint prog_;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		storage_buffer_ = 0;
		prog_			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* glsl_cs =
			NL "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;" NL "layout(std430) buffer Output {" NL
			   "  mediump vec4 color;" NL "} g_out;" NL
			   "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter;" NL "void main() {" NL
			   "  mediump uint c = atomicCounterIncrement(ac_counter);" NL
			   "  mediump float r = float(c / 40u) / 255.0;" NL "  g_out.color = vec4(r, 0.0, 0.0, 1.0);" NL "}";
		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// clear counter buffer (set to 0)
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr = 0;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// create shader storage buffer
		glGenBuffers(1, &storage_buffer_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage_buffer_);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 16, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(prog_);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, storage_buffer_);
		glDispatchCompute(1, 1, 1);

		if (glGetError() != GL_NO_ERROR)
		{
			return ERROR;
		}
		else
		{
			return NO_ERROR;
		}
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &storage_buffer_);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class BasicUsageFS : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counters usage in the Fragment Shader stage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters work as expected in the Fragment Shader stage." NL
				  "In particular make sure that values returned by GLSL built-in functions" NL
				  "atomicCounterIncrement and atomicCounterDecrement are unique in every shader invocation." NL
				  "Also make sure that the final values in atomic counter buffer objects are as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint vao_, vbo_;
	GLuint prog_;
	GLuint fbo_, rt_[2];

	virtual long Setup()
	{
		counter_buffer_ = 0;
		vao_ = vbo_ = 0;
		prog_		= 0;
		fbo_ = rt_[0] = rt_[1] = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p2);
		if (p1 < 1 || p2 < 2)
		{
			OutputNotSupported(
				"GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS or GL_MAX_FRAGMENT_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* src_vs = "#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "void main() {" NL
							 "  gl_Position = i_vertex;" NL "}";

		const char* src_fs = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[2];" NL
							 "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter_inc;" NL
							 "layout(binding = 0, offset = 4) uniform atomic_uint ac_counter_dec;" NL "void main() {" NL
							 "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter_inc));" NL
							 "  o_color[1] = uvec4(atomicCounterDecrement(ac_counter_dec));" NL "}";
		prog_ = CreateProgram(src_vs, src_fs, true);

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create render targets
		const int s = 8;
		glGenTextures(2, rt_);

		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, rt_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, s, s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// create fbo
		glGenFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt_[1], 0);
		const GLenum draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, draw_buffers);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// create geometry
		CreateQuad(&vao_, &vbo_, NULL);

		// init counter buffer
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr++ = 0;
		*ptr++ = 80;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// draw
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glViewport(0, 0, s, s);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glUseProgram(prog_);
		glBindVertexArray(vao_);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// validate
		UVec4 data[s * s];
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		if (!CheckCounterValues(s * s, data, 0))
			return ERROR;

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		if (!CheckCounterValues(s * s, data, 16))
			return ERROR;

		if (!CheckFinalCounterValue(counter_buffer_, 0, 64))
			return ERROR;
		if (!CheckFinalCounterValue(counter_buffer_, 4, 16))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteFramebuffers(1, &fbo_);
		glDeleteTextures(2, rt_);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteBuffers(1, &vbo_);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class BasicUsageVS : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counters usage in the Vertex Shader stage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters work as expected in the Vertex Shader stage." NL
				  "In particular make sure that values returned by GLSL built-in functions" NL
				  "atomicCounterIncrement and atomicCounterDecrement are unique in every shader invocation." NL
				  "Also make sure that the final values in atomic counter buffer objects are as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_[2];
	GLuint xfb_buffer_[2];
	GLuint array_buffer_;
	GLuint vao_;
	GLuint prog_;

	virtual long Setup()
	{
		counter_buffer_[0] = counter_buffer_[1] = 0;
		xfb_buffer_[0] = xfb_buffer_[1] = 0;
		array_buffer_					= 0;
		vao_							= 0;
		prog_							= 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &p2);
		if (p1 < 2 || p2 < 2)
		{
			OutputNotSupported(
				"GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS or GL_MAX_VERTEX_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* src_vs =
			"#version 310 es" NL "layout(location = 0) in uint i_zero;" NL "flat out uint o_atomic_inc;" NL
			"flat out uint o_atomic_dec;" NL "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter_inc;" NL
			"layout(binding = 1, offset = 0) uniform atomic_uint ac_counter_dec;" NL "void main() {" NL
			"  o_atomic_inc = i_zero + atomicCounterIncrement(ac_counter_inc);" NL
			"  o_atomic_dec = i_zero + atomicCounterDecrement(ac_counter_dec);" NL "}";

		const char* src_fs = "#version 310 es                \n"
							 "out mediump vec4 color;        \n"
							 "void main() {                  \n"
							 "    color = vec4(0, 1, 0, 1);  \n"
							 "}";

		prog_				   = CreateProgram(src_vs, src_fs, false);
		const char* xfb_var[2] = { "o_atomic_inc", "o_atomic_dec" };
		glTransformFeedbackVaryings(prog_, 2, xfb_var, GL_SEPARATE_ATTRIBS);
		LinkProgram(prog_);

		// create array buffer
		const unsigned int array_buffer_data[32] = { 0 };
		glGenBuffers(1, &array_buffer_);
		glBindBuffer(GL_ARRAY_BUFFER, array_buffer_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(array_buffer_data), array_buffer_data, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// create atomic counter buffers
		glGenBuffers(2, counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_[0]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_[1]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create transform feedback buffers
		glGenBuffers(2, xfb_buffer_);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[0]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000, NULL, GL_STREAM_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[1]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000, NULL, GL_STREAM_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		// init counter buffers
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_[0]);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr = 7;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_[1]);
		ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr = 77;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// create vertex array object
		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, array_buffer_);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		// draw
		glEnable(GL_RASTERIZER_DISCARD);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_[0]);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, counter_buffer_[1]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_buffer_[0]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, xfb_buffer_[1]);
		glUseProgram(prog_);
		glBindVertexArray(vao_);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, 32);
		glEndTransformFeedback();
		glDisable(GL_RASTERIZER_DISCARD);

		// validate
		GLuint* data;
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[0]);
		// CheckCounterValues will sort in place, so map buffer for both read and write
		data = static_cast<GLuint*>(
			glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 32 * 4, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
		if (!CheckCounterValues(32, data, 7))
			return ERROR;
		glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[1]);
		data = static_cast<GLuint*>(
			glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 32 * 4, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
		if (!CheckCounterValues(32, data, 45))
			return ERROR;
		glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		if (!CheckFinalCounterValue(counter_buffer_[0], 0, 39))
			return ERROR;
		if (!CheckFinalCounterValue(counter_buffer_[1], 0, 45))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteBuffers(2, counter_buffer_);
		glDeleteBuffers(2, xfb_buffer_);
		glDeleteBuffers(1, &array_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class AdvancedUsageMultiStage : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Same atomic counter accessed from multiple shader stages";
	}
	virtual std::string Purpose()
	{
		return NL "Same atomic counter is incremented (decremented) from two shader stages (VS and FS)." NL
				  "Verify that this scenario works as expected. In particular ensure that all generated values are "
				  "unique and" NL "final value in atomic counter buffer objects are as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint xfb_buffer_[2];
	GLuint vao_, vbo_;
	GLuint prog_;
	GLuint fbo_, rt_[2];

	virtual long Setup()
	{
		counter_buffer_ = 0;
		xfb_buffer_[0] = xfb_buffer_[1] = 0;
		vao_ = vbo_ = 0;
		prog_		= 0;
		fbo_ = rt_[0] = rt_[1] = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2, p3, p4;
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &p2);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p3);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p4);
		if (p1 < 8 || p2 < 2 || p3 < 8 || p4 < 2)
		{
			OutputNotSupported("GL_MAX_FRAGMENT/VERTEX_ATOMIC_COUNTER_BUFFERS or"
							   "GL_MAX_FRAGMENT/VERTEX_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* src_vs =
			"#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "flat out uint o_atomic_inc;" NL
			"flat out uint o_atomic_dec;" NL "layout(binding = 1, offset = 16) uniform atomic_uint ac_counter_inc;" NL
			"layout(binding = 7, offset = 128) uniform atomic_uint ac_counter_dec;" NL "void main() {" NL
			"  gl_Position = i_vertex;" NL "  o_atomic_inc = atomicCounterIncrement(ac_counter_inc);" NL
			"  o_atomic_dec = atomicCounterDecrement(ac_counter_dec);" NL "}";
		const char* src_fs = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[2];" NL
							 "layout(binding = 1, offset = 16) uniform atomic_uint ac_counter_inc;" NL
							 "layout(binding = 7, offset = 128) uniform atomic_uint ac_counter_dec;" NL
							 "void main() {" NL "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter_inc));" NL
							 "  o_color[1] = uvec4(atomicCounterDecrement(ac_counter_dec));" NL "}";
		prog_				   = CreateProgram(src_vs, src_fs, false);
		const char* xfb_var[2] = { "o_atomic_inc", "o_atomic_dec" };
		glTransformFeedbackVaryings(prog_, 2, xfb_var, GL_SEPARATE_ATTRIBS);
		LinkProgram(prog_);

		// create atomic counter buffer
		std::vector<GLuint> init_data(256, 100);
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, (GLsizeiptr)(init_data.size() * sizeof(GLuint)), &init_data[0],
					 GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create transform feedback buffers
		glGenBuffers(2, xfb_buffer_);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[0]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000, NULL, GL_STREAM_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[1]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000, NULL, GL_STREAM_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		// create render targets
		const int s = 8;
		glGenTextures(2, rt_);
		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, rt_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, s, s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// create fbo
		glGenFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt_[1], 0);
		const GLenum draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, draw_buffers);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// create geometry
		CreateTriangle(&vao_, &vbo_, NULL);

		// draw
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, 1, counter_buffer_, 16, 32);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 7, counter_buffer_);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_buffer_[0]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, xfb_buffer_[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glViewport(0, 0, s, s);
		glUseProgram(prog_);
		glBindVertexArray(vao_);
		glBeginTransformFeedback(GL_TRIANGLES);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glEndTransformFeedback();

		// validate
		UVec4 data[s * s + 3];
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[0]);
		GLuint* data2;
		data2 = static_cast<GLuint*>(
			glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 3 * sizeof(GLuint), GL_MAP_READ_BIT));
		data[s * s]		= UVec4(data2[0]);
		data[s * s + 1] = UVec4(data2[1]);
		data[s * s + 2] = UVec4(data2[2]);
		if (!CheckCounterValues(s * s + 3, data, 100))
			return ERROR;
		glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_[1]);
		data2 = static_cast<GLuint*>(
			glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 3 * sizeof(GLuint), GL_MAP_READ_BIT));
		data[s * s]		= UVec4(data2[0]);
		data[s * s + 1] = UVec4(data2[1]);
		data[s * s + 2] = UVec4(data2[2]);
		if (!CheckCounterValues(s * s + 3, data, 33))
			return ERROR;
		glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		if (!CheckFinalCounterValue(counter_buffer_, 32, 167))
			return ERROR;
		if (!CheckFinalCounterValue(counter_buffer_, 128, 33))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteFramebuffers(1, &fbo_);
		glDeleteTextures(2, rt_);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(2, xfb_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteBuffers(1, &vbo_);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class AdvancedUsageDrawUpdateDraw : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Update via Draw Call and update via MapBufferRange";
	}
	virtual std::string Purpose()
	{
		return NL "1. Create atomic counter buffers and init them with start values." NL
				  "2. Increment (decrement) buffer values in the shader." NL
				  "3. Map buffers with MapBufferRange command. Increment (decrement) buffer values manually." NL
				  "4. Unmap buffers with UnmapBuffer command." NL
				  "5. Again increment (decrement) buffer values in the shader." NL
				  "Verify that this scenario works as expected and final values in the buffer objects are correct.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint vao_, vbo_;
	GLuint prog_, prog2_;
	GLuint fbo_, rt_[2];

	virtual long Setup()
	{
		counter_buffer_ = 0;
		vao_ = vbo_ = 0;
		prog_		= 0;
		prog2_		= 0;
		fbo_ = rt_[0] = rt_[1] = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p2);
		if (p1 < 1 || p2 < 2)
		{
			OutputNotSupported(
				"GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS or GL_MAX_FRAGMENT_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* src_vs = "#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "void main() {" NL
							 "  gl_Position = i_vertex;" NL "}";
		const char* src_fs = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[2];" NL
							 "layout(binding = 0) uniform atomic_uint ac_counter[2];" NL "void main() {" NL
							 "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter[0]));" NL
							 "  o_color[1] = uvec4(atomicCounterDecrement(ac_counter[1]));" NL "}";
		const char* src_fs2 = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[2];" NL
							  "layout(binding = 0) uniform atomic_uint ac_counter[2];" NL "void main() {" NL
							  "  o_color[0] = uvec4(atomicCounter(ac_counter[0]));" NL
							  "  o_color[1] = uvec4(atomicCounter(ac_counter[1]));" NL "}";
		prog_  = CreateProgram(src_vs, src_fs, true);
		prog2_ = CreateProgram(src_vs, src_fs2, true);

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create render targets
		const int s = 8;
		glGenTextures(2, rt_);

		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, rt_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, s, s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// create fbo
		glGenFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt_[1], 0);
		const GLenum draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, draw_buffers);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// create geometry
		CreateQuad(&vao_, &vbo_, NULL);

		// init counter buffer
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr++ = 256;
		*ptr++ = 256;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// draw
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glViewport(0, 0, s, s);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glUseProgram(prog_);
		glBindVertexArray(vao_);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// update counter buffer
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT));
		*ptr++ += 512;
		*ptr++ += 1024;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// draw
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

		// draw
		glUseProgram(prog2_);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// validate
		UVec4 data[s * s];
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		for (int i = 0; i < s * s; ++i)
		{
			if (data[i].x() != 896)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Counter value is " << data[i].x()
													<< " should be 896." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		for (int i = 0; i < s * s; ++i)
		{
			if (data[i].x() != 1152)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Counter value is " << data[i].x()
													<< " should be 896." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		if (!CheckFinalCounterValue(counter_buffer_, 0, 896))
			return ERROR;
		if (!CheckFinalCounterValue(counter_buffer_, 4, 1152))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteFramebuffers(1, &fbo_);
		glDeleteTextures(2, rt_);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteBuffers(1, &vbo_);
		glDeleteProgram(prog_);
		glDeleteProgram(prog2_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class AdvancedUsageManyCounters : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "Large atomic counters array indexed with uniforms";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that large atomic counters array works as expected when indexed with dynamically uniform "
				  "expressions." NL
				  "Built-ins tested: atomicCounterIncrement, atomicCounterDecrement and atomicCounter.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_, m_ssbo;
	GLuint prog_;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		m_ssbo			= 0;
		prog_			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		// create program
		const char* glsl_cs = NL
			"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;" NL "layout(std430) buffer Output {" NL
			"  mediump uvec4 data1[64];" NL "  mediump uvec4 data2[64];" NL "  mediump uvec4 data3[64];" NL
			"  mediump uvec4 data4[64];" NL "  mediump uvec4 data5[64];" NL "  mediump uvec4 data6[64];" NL
			"  mediump uvec4 data7[64];" NL "  mediump uvec4 data8[64];" NL "} g_out;" NL
			"uniform mediump int u_active_counters[8];" NL "layout(binding = 0) uniform atomic_uint ac_counter[8];" NL
			"void main() {" NL "  mediump uint offset = 8u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			"  g_out.data1[offset] = uvec4(atomicCounterIncrement(ac_counter[u_active_counters[0]]));" NL
			"  g_out.data2[offset] = uvec4(atomicCounterDecrement(ac_counter[u_active_counters[1]]));" NL
			"  g_out.data3[offset] = uvec4(atomicCounter(ac_counter[u_active_counters[2]]));" NL
			"  g_out.data4[offset] = uvec4(atomicCounterIncrement(ac_counter[u_active_counters[3]]));" NL
			"  g_out.data5[offset] = uvec4(atomicCounterDecrement(ac_counter[u_active_counters[4]]));" NL
			"  g_out.data6[offset] = uvec4(atomicCounter(ac_counter[u_active_counters[5]]));" NL
			"  g_out.data7[offset] = uvec4(atomicCounterIncrement(ac_counter[u_active_counters[6]]));" NL
			"  g_out.data8[offset] = uvec4(atomicCounterIncrement(ac_counter[u_active_counters[7]]));" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;

		// create atomic counter buffer
		std::vector<GLuint> init_data(1024, 1000);
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 1024 * sizeof(GLuint), &init_data[0], GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glGenBuffers(1, &m_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * 64 * sizeof(UVec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// set uniforms
		glUseProgram(prog_);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[0]"), 0);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[1]"), 1);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[2]"), 2);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[3]"), 3);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[4]"), 4);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[5]"), 5);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[6]"), 6);
		glUniform1i(glGetUniformLocation(prog_, "u_active_counters[7]"), 7);

		// dispatch
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
		glDispatchCompute(8, 8, 1);

		// validate
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
		UVec4* data;
		long   error = NO_ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 0, 1064))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4), 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000 - 64))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 1 * sizeof(GLuint), 1000 - 64))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 2, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		for (int i = 0; i < 8 * 8; ++i)
			if (data[i].x() != 1000)
				error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 2 * sizeof(GLuint), 1000))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 3, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 3 * sizeof(GLuint), 1064))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 4, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000 - 64))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 4 * sizeof(GLuint), 1000 - 64))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 5, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		for (int i = 0; i < 8 * 8; ++i)
			if (data[i].x() != 1000)
				error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 5 * sizeof(GLuint), 1000))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 6, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 6 * sizeof(GLuint), 1064))
			error = ERROR;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64 * sizeof(UVec4) * 7, 64 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(8 * 8, data, 1000))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!CheckFinalCounterValue(counter_buffer_, 7 * sizeof(GLuint), 1064))
			error = ERROR;

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class AdvancedUsageSwitchPrograms : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Switching several program objects with different atomic counters with different bindings";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that each program upadate atomic counter buffer object in appropriate binding point.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_[8];
	GLuint xfb_buffer_;
	GLuint vao_, vbo_;
	GLuint prog_[8];
	GLuint fbo_, rt_;

	std::string GenVSSrc(int binding, int offset)
	{
		std::ostringstream os;
		os << "#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "flat out uvec4 o_atomic_value;" NL
			  "layout(binding = "
		   << binding << ", offset = " << offset
		   << ") uniform atomic_uint ac_counter_vs;" NL "void main() {" NL "  gl_Position = i_vertex;" NL
			  "  o_atomic_value = uvec4(atomicCounterIncrement(ac_counter_vs));" NL "}";
		return os.str();
	}
	std::string GenFSSrc(int binding, int offset)
	{
		std::ostringstream os;
		os << "#version 310 es" NL "layout(location = 0) out uvec4 o_color;" NL "layout(binding = " << binding
		   << ", offset = " << offset << ") uniform atomic_uint ac_counter_fs;" NL "void main() {" NL
										 "  o_color = uvec4(atomicCounterIncrement(ac_counter_fs));" NL "}";
		return os.str();
	}
	virtual long Setup()
	{
		memset(counter_buffer_, 0, sizeof(counter_buffer_));
		xfb_buffer_ = 0;
		vao_ = vbo_ = 0;
		memset(prog_, 0, sizeof(prog_));
		fbo_ = rt_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2, p3, p4;
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &p2);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p3);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p4);
		if (p1 < 8 || p2 < 1 || p3 < 8 || p4 < 1)
		{
			OutputNotSupported("GL_MAX_*_ATOMIC_COUNTER_BUFFERS or GL_MAX_*_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create programs
		for (int i = 0; i < 8; ++i)
		{
			std::string vs_str  = GenVSSrc(i, i * 8);
			std::string fs_str  = GenFSSrc(7 - i, 128 + i * 16);
			const char* src_vs  = vs_str.c_str();
			const char* src_fs  = fs_str.c_str();
			prog_[i]			= CreateProgram(src_vs, src_fs, false);
			const char* xfb_var = "o_atomic_value";
			glTransformFeedbackVaryings(prog_[i], 1, &xfb_var, GL_SEPARATE_ATTRIBS);
			LinkProgram(prog_[i]);
		}

		// create atomic counter buffers
		glGenBuffers(8, counter_buffer_);
		for (int i = 0; i < 8; ++i)
		{
			std::vector<GLuint> init_data(256);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_[i]);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, (GLsizeiptr)(init_data.size() * sizeof(GLuint)), &init_data[0],
						 GL_DYNAMIC_COPY);
		}
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create transform feedback buffer
		glGenBuffers(1, &xfb_buffer_);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buffer_);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1000, NULL, GL_STREAM_COPY);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		// create render target
		const int s = 8;
		glGenTextures(1, &rt_);
		glBindTexture(GL_TEXTURE_2D, rt_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, s, s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		// create fbo
		glGenFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// create geometry
		CreateTriangle(&vao_, &vbo_, NULL);

		// draw
		for (GLuint i = 0; i < 8; ++i)
		{
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, i, counter_buffer_[i]);
		}
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, xfb_buffer_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glViewport(0, 0, s, s);
		glBindVertexArray(vao_);

		for (int i = 0; i < 8; ++i)
		{
			glUseProgram(prog_[i]);
			glBeginTransformFeedback(GL_TRIANGLES);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glEndTransformFeedback();
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

			if (!CheckFinalCounterValue(counter_buffer_[i], i * 8, 3))
				return ERROR;
			if (!CheckFinalCounterValue(counter_buffer_[7 - i], 128 + i * 16, 64))
				return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteFramebuffers(1, &fbo_);
		glDeleteTextures(1, &rt_);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteBuffers(8, counter_buffer_);
		glDeleteBuffers(1, &xfb_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteBuffers(1, &vbo_);
		for (int i = 0; i < 8; ++i)
			glDeleteProgram(prog_[i]);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class AdvancedUsageUBO : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "Atomic Counters used to access Uniform Buffer Objects";
	}
	virtual std::string Purpose()
	{
		return NL "Atomic counters are used to access UBOs. In that way each shader invocation can access UBO at "
				  "unique offset." NL
				  "This scenario is a base for some practical algorithms. Verify that it works as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_, m_ssbo;
	GLuint uniform_buffer_;
	GLuint prog_;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		uniform_buffer_ = 0;
		m_ssbo			= 0;
		prog_			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		// create program
		const char* glsl_cs =
			NL "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;" NL "layout(std430) buffer Output {" NL
			   "  mediump uvec4 color[256];" NL "} g_out;" NL
			   "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter;" NL "layout(std140) uniform Data {" NL
			   "  mediump uint index[256];" NL "} ub_data;" NL "void main() {" NL
			   "  mediump uint offset = 16u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.color[offset] = uvec4(ub_data.index[atomicCounterIncrement(ac_counter)]);" NL "}";
		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;
		glUniformBlockBinding(prog_, glGetUniformBlockIndex(prog_, "Data"), 1);

		// create atomic counter buffer
		const unsigned int z = 0;
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(z), &z, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create uniform buffer
		std::vector<UVec4> init_data(256);
		for (GLuint i	= 0; i < 256; ++i)
			init_data[i] = UVec4(i);
		glGenBuffers(1, &uniform_buffer_);
		glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer_);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)(sizeof(UVec4) * init_data.size()), &init_data[0], GL_DYNAMIC_COPY);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glGenBuffers(1, &m_ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(UVec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// draw
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, uniform_buffer_);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
		glUseProgram(prog_);
		glDispatchCompute(16, 16, 1);

		// validate
		UVec4* data;
		long   error = NO_ERROR;
		glMemoryBarrier(GL_UNIFORM_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		data = static_cast<UVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 256 * sizeof(UVec4), GL_MAP_READ_BIT));
		if (!CheckCounterValues(16 * 16, data, 0))
			error = ERROR;
		if (!CheckFinalCounterValue(counter_buffer_, 0, 256))
			error = ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &uniform_buffer_);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class NegativeAPI : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "NegativeAPI";
	}
	virtual std::string Purpose()
	{
		return NL "Verify errors reported by BindBuffer* commands.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint buffer;

	virtual long Setup()
	{
		return NO_ERROR;
	}
	virtual long Run()
	{
		long  error = NO_ERROR;
		GLint res;
		glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &res);
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, res, buffer);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindBufferBase should generate INVALID_VALUE when"
											" index is greater than or equal GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS."
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, res, buffer, 0, 4);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindBufferRange should generate INVALID_VALUE when"
											" index is greater than or equal GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS."
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, res - 1, buffer, 3, 4);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindBufferRange should generate INVALID_VALUE when"
											" <offset> is not a multiple of four"
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		return error;
	}
	virtual long Cleanup()
	{
		glDeleteBuffers(1, &buffer);
		return NO_ERROR;
	}
};

class NegativeGLSL : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that two different atomic counter uniforms with same binding cannot share same offset value.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs = NL
			"layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			"layout(binding = 0, offset = 4) uniform atomic_uint ac_counter_inc;" NL
			"layout(binding = 0, offset = 4) uniform atomic_uint ac_counter_dec;" NL "layout(std430) buffer Output {" NL
			"  mediump uint data_inc[256];" NL "  mediump uint data_dec[256];" NL "} g_out;" NL "void main() {" NL
			"  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			"  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter_inc);" NL
			"  g_out.data_dec[offset] = atomicCounterDecrement(ac_counter_dec);" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (CheckProgram(prog_))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Link should fail because ac_counter0 and ac_counter2 uses same binding and same offset."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class AdvancedManyDrawCalls : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "Atomic Counters usage in multiple draw calls";
	}
	virtual std::string Purpose()
	{
		return NL "Verify atomic counters behaviour across multiple draw calls.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint vao_, vbo_;
	GLuint prog_;
	GLuint fbo_, rt_[2];

	virtual long Setup()
	{
		counter_buffer_ = 0;
		vao_ = vbo_ = 0;
		prog_		= 0;
		fbo_ = rt_[0] = rt_[1] = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p2);
		if (p1 < 1 || p2 < 2)
		{
			OutputNotSupported(
				"GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS or GL_MAX_FRAGMENT_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* src_vs = "#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "void main() {" NL
							 "  gl_Position = i_vertex;" NL "}";
		const char* src_fs = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[2];" NL
							 "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter_inc;" NL
							 "layout(binding = 0, offset = 4) uniform atomic_uint ac_counter_dec;" NL "void main() {" NL
							 "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter_inc));" NL
							 "  o_color[1] = uvec4(atomicCounterDecrement(ac_counter_dec));" NL "}";
		prog_ = CreateProgram(src_vs, src_fs, true);

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		// create render targets
		const int s = 8;
		glGenTextures(2, rt_);

		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, rt_[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, s, s, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// create fbo
		glGenFramebuffers(1, &fbo_);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt_[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, rt_[1], 0);
		const GLenum draw_buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, draw_buffers);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// create geometry
		CreateQuad(&vao_, &vbo_, NULL);

		// init counter buffer
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr++ = 0;
		*ptr++ = 256;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		// draw
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		glViewport(0, 0, s, s);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glUseProgram(prog_);
		glBindVertexArray(vao_);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// validate
		UVec4 data[s * s];
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		if (!CheckCounterValues(s * s, data, s * s * 3))
			return ERROR;

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		glReadPixels(0, 0, s, s, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);
		if (!CheckCounterValues(s * s, data, 0))
			return ERROR;

		if (!CheckFinalCounterValue(counter_buffer_, 0, 256))
			return ERROR;
		if (!CheckFinalCounterValue(counter_buffer_, 4, 0))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteFramebuffers(1, &fbo_);
		glDeleteTextures(2, rt_);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteVertexArrays(1, &vao_);
		glDeleteBuffers(1, &vbo_);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class NegativeSSBO : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters cannot be declared in the buffer block.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		const char* const glsl_cs =
			NL "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			   "layout(binding = 0, offset = 0) uniform atomic_uint ac_counter_dec;" NL
			   "layout(std430) buffer Output {" NL "  mediump uint data_inc[256];" NL "  mediump uint data_dec[256];" NL
			   "  layout(binding = 0, offset = 16) uniform atomic_uint ac_counter0;" NL "} g_out;" NL "void main() {" NL
			   "  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter0);" NL
			   "  g_out.data_dec[offset] = atomicCounterDecrement(ac_counter_dec);" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (CheckProgram(prog_))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Link should fail because atomic counters cannot be declared in the buffer block."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class NegativeUBO : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters cannot be declared in uniform block.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL "uniform Block {" NL
			   "  uniform atomic_uint ac_counter;" NL "};" NL "layout(std430) buffer Output {" NL
			   "  mediump uint data_inc[256];" NL "} g_out;" NL "void main() {" NL
			   "  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter);" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (CheckProgram(prog_))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Link should fail because atomic counters cannot be declared in the uniform block."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class BasicUsageNoOffset : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "Atomic Counters usage in the Compute Shader stage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomic counters work as expected in the Compute Shader stage when decalred with no "
				  "offset qualifier." NL "In particular make sure that values returned by GLSL built-in functions" NL
				  "atomicCounterIncrement and atomicCounterDecrement are unique in every shader invocation." NL
				  "Also make sure that the final values in atomic counter buffer objects are as expected.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint counter_buffer_;
	GLuint prog_;
	GLuint m_buffer;

	virtual long Setup()
	{
		counter_buffer_ = 0;
		prog_			= 0;
		m_buffer		= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		// create program
		const char* const glsl_cs =
			NL "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			   "layout(binding = 0) uniform atomic_uint ac_counter_inc;" NL
			   "layout(binding = 0) uniform atomic_uint ac_counter_dec;" NL "layout(std430) buffer Output {" NL
			   "  mediump uint data_inc[256];" NL "  mediump uint data_dec[256];" NL "} g_out;" NL "void main() {" NL
			   "  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter_inc);" NL
			   "  g_out.data_dec[offset] = atomicCounterDecrement(ac_counter_dec);" NL "}";
		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;

		// create atomic counter buffer
		glGenBuffers(1, &counter_buffer_);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer_);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 8, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer_);
		unsigned int* ptr = static_cast<unsigned int*>(
			glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 8, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
		*ptr++ = 0;
		*ptr++ = 256;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 512 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(prog_);
		glDispatchCompute(4, 1, 1);

		long	error = NO_ERROR;
		GLuint* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data = static_cast<GLuint*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 512 * sizeof(GLuint), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));

		std::sort(data, data + 512);
		for (int i = 0; i < 512; i += 2)
		{
			if (data[i] != data[i + 1])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Pair of values should be equal, got: " << data[i] << ", "
					<< data[i + 1] << tcu::TestLog::EndMessage;
				error = ERROR;
			}
			if (i < 510 && data[i] == data[i + 2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Too many same values found: " << data[i] << ", index: " << i
					<< tcu::TestLog::EndMessage;
				error = ERROR;
			}
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &counter_buffer_);
		glDeleteBuffers(1, &m_buffer);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

class NegativeUniform : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomicCounterIncrement/atomicCounterDecrement "
				  "cannot be used on normal uniform.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p2);
		if (p1 < 1 || p2 < 1)
		{
			OutputNotSupported(
				"GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS or GL_MAX_FRAGMENT_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		// create program
		const char* glsl_vs = "#version 310 es" NL "layout(location = 0) in vec4 i_vertex;" NL "void main() {" NL
							  "  gl_Position = i_vertex;" NL "}";

		const char* glsl_fs1 =
			"#version 310 es" NL "layout(location = 0) out uvec4 o_color[4];" NL "uniform uint ac_counter0;" NL
			"void main() {" NL "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter0));" NL "}";

		prog_ = glCreateProgram();

		GLuint sh = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(prog_, sh);
		glShaderSource(sh, 1, &glsl_vs, NULL);
		glCompileShader(sh);
		GLint status_comp;
		glGetShaderiv(sh, GL_COMPILE_STATUS, &status_comp);
		if (status_comp != GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Unexpected error during vertex shader compilation."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glDeleteShader(sh);

		sh = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(prog_, sh);
		glShaderSource(sh, 1, &glsl_fs1, NULL);
		glCompileShader(sh);
		glGetShaderiv(sh, GL_COMPILE_STATUS, &status_comp);
		glDeleteShader(sh);

		GLint status;
		glLinkProgram(prog_);
		glGetProgramiv(prog_, GL_LINK_STATUS, &status);
		if (status_comp == GL_TRUE && status == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected error during fragment shader compilation or linking."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class NegativeArray : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that atomicCounterIncrement/atomicCounterDecrement "
				  "cannot be used on array of atomic counters.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		const char* const glsl_cs =
			NL "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			   "layout(binding = 0) uniform atomic_uint ac_counter[3];" NL "layout(std430) buffer Output {" NL
			   "  mediump uint data_inc[256];" NL "} g_out;" NL "void main() {" NL
			   "  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.data_inc[offset] = atomicCounterIncrement(ac_counter);" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (CheckProgram(prog_))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Link should fail because atomicCounterIncrement cannot be used on array of atomic counters."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class NegativeArithmetic : public BasicUsageCS
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that standard arithmetic operations \n"
				  "cannot be performed on atomic counters.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint prog_;

	virtual long Setup()
	{
		prog_ = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{

		const char* const glsl_cs =
			NL "layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;" NL
			   "layout(binding = 0) uniform atomic_uint ac_counter;" NL "layout(std430) buffer Output {" NL
			   "  mediump uint data_inc[256];" NL "} g_out;" NL "void main() {" NL
			   "  mediump uint offset = 32u * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;" NL
			   "  g_out.data_inc[offset] = ac_counter + 1;" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (CheckProgram(prog_))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Link should fail because atomic counters cannot be incremented by standard arithmetic operations."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgram(prog_);
		return NO_ERROR;
	}
};

class NegativeUnsizedArray : public SACSubcaseBase
{
	virtual std::string Title()
	{
		return NL "GLSL errors";
	}

	virtual std::string Purpose()
	{
		return NL "Verify that it is compile-time error to declare an unsized array of atomic_uint..";
	}

	virtual std::string Method()
	{
		return NL "";
	}

	virtual std::string PassCriteria()
	{
		return NL "";
	}

	virtual long Run()
	{
		const char* glsl_fs1 = "#version 310 es" NL "layout(location = 0) out uvec4 o_color[4];"
							   "  layout(binding = 0, offset = 4) uniform atomic_uint ac_counter[];" NL
							   "void main() {" NL "  o_color[0] = uvec4(atomicCounterIncrement(ac_counter[0]));" NL "}";

		GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(sh, 1, &glsl_fs1, NULL);
		glCompileShader(sh);
		GLint status_comp;
		glGetShaderiv(sh, GL_COMPILE_STATUS, &status_comp);
		glDeleteShader(sh);

		if (status_comp == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected error during fragment shader compilation."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
};

class AdvancedManyDrawCalls2 : public SACSubcaseBase
{

	GLuint m_acbo, m_ssbo;
	GLuint m_vao;
	GLuint m_ppo, m_vsp, m_fsp;

	virtual long Setup()
	{
		glGenBuffers(1, &m_acbo);
		glGenBuffers(1, &m_ssbo);
		glGenVertexArrays(1, &m_vao);
		glGenProgramPipelines(1, &m_ppo);
		m_vsp = m_fsp = 0;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_acbo);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteProgramPipelines(1, &m_ppo);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		return NO_ERROR;
	}

	virtual long Run()
	{

		GLint p1, p2;
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &p1);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &p2);
		if (p1 < 1 || p2 < 1)
		{
			OutputNotSupported(
				"GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS or GL_MAX_FRAGMENT_ATOMIC_COUNTERS are less than required");
			return NOT_SUPPORTED;
		}

		const char* const glsl_vs = "#version 310 es" NL "void main() {" NL "#ifdef GL_ES" NL
									"  gl_PointSize = 1.0f;" NL "#endif" NL "  gl_Position = vec4(0, 0, 0, 1);" NL "}";
		const char* const glsl_fs =
			"#version 310 es" NL "layout(binding = 0) uniform atomic_uint g_counter;" NL
			"layout(std430, binding = 0) buffer Output {" NL "  uint g_output[];" NL "};" NL "void main() {" NL
			"  uint c = atomicCounterIncrement(g_counter);" NL "  g_output[c] = c;" NL "}";

		m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
		if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
			return ERROR;

		glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_acbo);
		{
			GLuint data = 0;
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, &data, GL_DYNAMIC_COPY);
		}

		{
			std::vector<GLuint> data(1000, 0xffff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(data.size() * 4), &data[0], GL_DYNAMIC_READ);
		}

		// draw
		glViewport(0, 0, 1, 1);
		glBindProgramPipeline(m_ppo);
		glBindVertexArray(m_vao);
		for (int i = 0; i < 100; ++i)
		{
			glDrawArrays(GL_POINTS, 0, 1);
		}

		glViewport(0, 0, getWindowWidth(), getWindowHeight());

		long status = NO_ERROR;

		{
			GLuint* data;

			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_acbo);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			data = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 4, GL_MAP_READ_BIT));
			if (data[0] != 100)
			{
				status = ERROR;
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "AC buffer content is " << data[0]
													<< ", sholud be 100." << tcu::TestLog::EndMessage;
			}
			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			data = static_cast<GLuint*>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 100 * 4, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
			std::sort(data, data + 100);
			for (GLuint i = 0; i < 100; ++i)
			{
				if (data[i] != i)
				{
					status = ERROR;
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "data[" << i << " is " << data[i]
														<< ", sholud be " << i << tcu::TestLog::EndMessage;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		return status;
	}
};

class AdvancedUsageMultipleComputeDispatches : public SACSubcaseBase
{
	GLuint m_acbo, m_ssbo;
	GLuint m_ppo, m_csp;

	virtual long Setup()
	{
		glGenBuffers(1, &m_acbo);
		glGenBuffers(1, &m_ssbo);
		glGenProgramPipelines(1, &m_ppo);
		m_csp = 0;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_acbo);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteProgramPipelines(1, &m_ppo);
		glDeleteProgram(m_csp);
		return NO_ERROR;
	}

	virtual long Run()
	{
		// create program
		const char* const glsl_cs =
			"#version 310 es" NL "layout(local_size_x = 1) in;" NL
			"layout(binding = 0) uniform atomic_uint g_counter;" NL "layout(std430, binding = 0) buffer Output {" NL
			"  mediump uint g_output[];" NL "};" NL "void main() {" NL
			"  mediump uint c = atomicCounterIncrement(g_counter);" NL "  g_output[c] = c;" NL "}";

		m_csp = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &glsl_cs);
		if (!CheckProgram(m_csp))
			return ERROR;
		glUseProgramStages(m_ppo, GL_COMPUTE_SHADER_BIT, m_csp);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_acbo);
		{
			GLuint data = 0;
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, &data, GL_DYNAMIC_COPY);
		}

		{
			std::vector<GLuint> data(1000, 0xffff);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(data.size() * 4), &data[0], GL_DYNAMIC_READ);
		}

		glBindProgramPipeline(m_ppo);
		for (int i = 0; i < 100; ++i)
		{
			glDispatchCompute(1, 1, 1);
		}

		long status = NO_ERROR;

		{
			GLuint* data;

			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_acbo);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			data = static_cast<GLuint*>(glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, 4, GL_MAP_READ_BIT));
			if (data[0] != 100)
			{
				status = ERROR;
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "AC buffer content is " << data[0]
													<< ", sholud be 100" << tcu::TestLog::EndMessage;
			}
			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			data = static_cast<GLuint*>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 100 * 4, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
			std::sort(data, data + 100);
			for (GLuint i = 0; i < 100; ++i)
			{
				if (data[i] != i)
				{
					status = ERROR;
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "data[" << i << "] is " << data[i]
														<< ", sholud be " << i << tcu::TestLog::EndMessage;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

		return status;
	}
};

class BasicGLSLBuiltIn : public BasicUsageCS
{
public:
	virtual std::string Title()
	{
		return NL "gl_Max* Check";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that gl_Max*Counters and gl_Max*Bindings exist in glsl and their values are no lower" NL
				  "than minimum required by the spec and are no different from their GL_MAX_* counterparts.";
	}

	GLuint prog_;
	GLuint m_buffer;

	virtual long Setup()
	{
		prog_	= 0;
		m_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		// create program
		const char* const glsl_cs = NL
			"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;" NL "layout(std430) buffer Output {" NL
			"  mediump uint data;" NL "} g_out;" NL "uniform mediump int m_vac;" NL "uniform mediump int m_fac;" NL
			"uniform mediump int m_csac;" NL "uniform mediump int m_cac;" NL "uniform mediump int m_abuf;" NL
			"void main() {" NL "  mediump uint res = 1u;" NL
			"  if (gl_MaxVertexAtomicCounters < 0 || gl_MaxVertexAtomicCounters != m_vac)" NL "     res = res * 2u;" NL
			"  if (gl_MaxFragmentAtomicCounters < 0 || gl_MaxFragmentAtomicCounters != m_fac)" NL
			"     res = res * 3u;" NL
			"  if (gl_MaxComputeAtomicCounters < 8 || gl_MaxComputeAtomicCounters != m_csac)" NL
			"     res = res * 5u;" NL
			"  if (gl_MaxCombinedAtomicCounters < 8 || gl_MaxCombinedAtomicCounters != m_cac)" NL
			"     res = res * 7u;" NL
			"  if (gl_MaxAtomicCounterBindings < 1 || gl_MaxAtomicCounterBindings != m_abuf)" NL
			"     res = res * 11u;" NL "  g_out.data = res;" NL "}";

		prog_ = CreateComputeProgram(glsl_cs);
		glLinkProgram(prog_);
		if (!CheckProgram(prog_))
			return ERROR;
		glUseProgram(prog_);

		int m_vac;
		int m_fac;
		int m_csac;
		int m_cac;
		int m_abuf;
		glGetIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &m_vac);
		glUniform1i(glGetUniformLocation(prog_, "m_vac"), m_vac);
		glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, &m_fac);
		glUniform1i(glGetUniformLocation(prog_, "m_fac"), m_fac);
		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &m_csac);
		glUniform1i(glGetUniformLocation(prog_, "m_csac"), m_csac);
		glGetIntegerv(GL_MAX_COMBINED_ATOMIC_COUNTERS, &m_cac);
		glUniform1i(glGetUniformLocation(prog_, "m_cac"), m_cac);
		glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &m_abuf);
		glUniform1i(glGetUniformLocation(prog_, "m_abuf"), m_abuf);

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDispatchCompute(1, 1, 1);

		long	error = NO_ERROR;
		GLuint* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data = static_cast<GLuint*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
		if (data[0] != 1u)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 1, got: " << data[0] << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return error;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_buffer);
		glDeleteProgram(prog_);
		glUseProgram(0);
		return NO_ERROR;
	}
};

ShaderAtomicCountersTests::ShaderAtomicCountersTests(glcts::Context& context)
	: TestCaseGroup(context, "shader_atomic_counters", "")
{
}

ShaderAtomicCountersTests::~ShaderAtomicCountersTests(void)
{
}

void ShaderAtomicCountersTests::init()
{
	using namespace glcts;
	addChild(new TestSubcase(m_context, "basic-buffer-operations", TestSubcase::Create<BasicBufferOperations>));
	addChild(new TestSubcase(m_context, "basic-buffer-state", TestSubcase::Create<BasicBufferState>));
	addChild(new TestSubcase(m_context, "basic-buffer-bind", TestSubcase::Create<BasicBufferBind>));
	addChild(new TestSubcase(m_context, "basic-program-max", TestSubcase::Create<BasicProgramMax>));
	addChild(new TestSubcase(m_context, "basic-program-query", TestSubcase::Create<BasicProgramQuery>));
	addChild(new TestSubcase(m_context, "basic-usage-simple", TestSubcase::Create<BasicUsageSimple>));
	addChild(new TestSubcase(m_context, "basic-usage-no-offset", TestSubcase::Create<BasicUsageNoOffset>));
	addChild(new TestSubcase(m_context, "basic-usage-fs", TestSubcase::Create<BasicUsageFS>));
	addChild(new TestSubcase(m_context, "basic-usage-vs", TestSubcase::Create<BasicUsageVS>));
	addChild(new TestSubcase(m_context, "basic-usage-cs", TestSubcase::Create<BasicUsageCS>));
	addChild(new TestSubcase(m_context, "basic-glsl-built-in", TestSubcase::Create<BasicGLSLBuiltIn>));
	addChild(new TestSubcase(m_context, "advanced-usage-multi-stage", TestSubcase::Create<AdvancedUsageMultiStage>));
	addChild(new TestSubcase(m_context, "advanced-usage-draw-update-draw",
							 TestSubcase::Create<AdvancedUsageDrawUpdateDraw>));
	addChild(
		new TestSubcase(m_context, "advanced-usage-many-counters", TestSubcase::Create<AdvancedUsageManyCounters>));
	addChild(
		new TestSubcase(m_context, "advanced-usage-switch-programs", TestSubcase::Create<AdvancedUsageSwitchPrograms>));
	addChild(new TestSubcase(m_context, "advanced-usage-ubo", TestSubcase::Create<AdvancedUsageUBO>));
	addChild(new TestSubcase(m_context, "advanced-usage-many-draw-calls", TestSubcase::Create<AdvancedManyDrawCalls>));
	addChild(
		new TestSubcase(m_context, "advanced-usage-many-draw-calls2", TestSubcase::Create<AdvancedManyDrawCalls2>));
	addChild(new TestSubcase(m_context, "advanced-usage-many-dispatches",
							 TestSubcase::Create<AdvancedUsageMultipleComputeDispatches>));
	addChild(new TestSubcase(m_context, "negative-api", TestSubcase::Create<NegativeAPI>));
	addChild(new TestSubcase(m_context, "negative-glsl", TestSubcase::Create<NegativeGLSL>));
	addChild(new TestSubcase(m_context, "negative-ssbo", TestSubcase::Create<NegativeSSBO>));
	addChild(new TestSubcase(m_context, "negative-ubo", TestSubcase::Create<NegativeUBO>));
	addChild(new TestSubcase(m_context, "negative-uniform", TestSubcase::Create<NegativeUniform>));
	addChild(new TestSubcase(m_context, "negative-array", TestSubcase::Create<NegativeArray>));
	addChild(new TestSubcase(m_context, "negative-arithmetic", TestSubcase::Create<NegativeArithmetic>));
	addChild(new TestSubcase(m_context, "negative-unsized-array", TestSubcase::Create<NegativeUnsizedArray>));
}
}
