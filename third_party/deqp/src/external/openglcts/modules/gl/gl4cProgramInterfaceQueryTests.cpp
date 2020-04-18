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

#include "gl4cProgramInterfaceQueryTests.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include <cstdarg>
#include <map>
#include <set>

namespace gl4cts
{

using namespace glw;

namespace
{

class PIQBase : public deqp::SubcaseBase
{

public:
	virtual std::string PassCriteria()
	{
		return "All called functions return expected values.";
	}

	virtual std::string Purpose()
	{
		return "Verify that the set of tested functions glGetProgram* return\n"
			   "expected results when used to get data from program\n"
			   "made of " +
			   ShadersDesc() + "." + PurposeExt();
	}

	virtual std::string Method()
	{
		return "Create a program using " + ShadersDesc() +
			   "\n"
			   "then use set of tested functions to get an information about it and\n"
			   "verify that information with the expected data" +
			   Expectations();
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		return NO_ERROR;
	}

	virtual long Setup()
	{
		return NO_ERROR;
	}

	virtual ~PIQBase()
	{
	}

protected:
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

	GLuint CreateProgram(const char* src_vs, const char* src_tcs, const char* src_tes, const char* src_gs,
						 const char* src_fs, bool link)
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
		if (src_tcs)
		{
			GLuint sh = glCreateShader(GL_TESS_CONTROL_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_tcs, NULL);
			glCompileShader(sh);
		}
		if (src_tes)
		{
			GLuint sh = glCreateShader(GL_TESS_EVALUATION_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_tes, NULL);
			glCompileShader(sh);
		}
		if (src_gs)
		{
			GLuint sh = glCreateShader(GL_GEOMETRY_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_gs, NULL);
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

	virtual std::string ShadersDesc()
	{
		return "";
	}

	virtual std::string Expectations()
	{
		return ".";
	}

	virtual std::string PurposeExt()
	{
		return "";
	}

	virtual inline void ExpectError(GLenum expected, long& error)
	{
		if (error != NO_ERROR)
			return;
		GLenum tmp = glGetError();
		if (tmp == expected)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Found expected error" << tcu::TestLog::EndMessage;
			error = NO_ERROR; // Error is expected
		}
		else
		{
			error = ERROR;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << expected
												<< " error was expected, found: " << tmp << tcu::TestLog::EndMessage;
		}
	}

	virtual inline void VerifyGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, int expected,
													long& error)
	{
		GLint res;
		glGetProgramInterfaceiv(program, programInterface, pname, &res);
		if (res != expected)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res << ", expected "
												<< expected << tcu::TestLog::EndMessage;
			error = ERROR;
		}
	}

	virtual inline void VerifyGetProgramResourceIndex(GLuint program, GLenum programInterface, const std::string& name,
													  GLuint expected, long& error)
	{
		GLuint res = glGetProgramResourceIndex(program, programInterface, name.c_str());
		if (res != expected)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res << ", expected "
												<< expected << tcu::TestLog::EndMessage;
			error = ERROR;
		}
	}

	virtual inline void VerifyGetProgramResourceIndex(GLuint program, GLenum		 programInterface,
													  std::map<std::string, GLuint>& indices, const std::string& name,
													  long& error)
	{
		GLuint res = glGetProgramResourceIndex(program, programInterface, name.c_str());
		if (res == GL_INVALID_INDEX)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res
												<< ", expected number other than -1" << tcu::TestLog::EndMessage;
			error = ERROR;
			return;
		}
		std::map<std::string, GLuint>::iterator it = indices.begin();
		while (it != indices.end())
		{
			if (it->second == res)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "ERROR: Duplicated value found: " << res << tcu::TestLog::EndMessage;
				error = ERROR;
				return;
			}
			++it;
		}
		indices[name] = res;
	}

	virtual inline void VerifyGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index,
													 const std::string& expected, long& error)
	{
		GLchar  name[1024] = { '\0' };
		GLsizei len;
		glGetProgramResourceName(program, programInterface, index, 1024, &len, name);
		if (len <= 0 || len > 1023 || name[len - 1] == '\0')
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "ERROR: Length in glGetProgramResourceName should not count null terminator!"
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		else if (name != expected || name[len] != '\0')
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << name << ", expected "
												<< expected << tcu::TestLog::EndMessage;
			error = ERROR;
		}
	}

	virtual inline void VerifyGetProgramResourceLocation(GLuint program, GLenum programInterface,
														 const std::string& name, GLint expected, long& error)
	{
		GLint res = glGetProgramResourceLocation(program, programInterface, name.c_str());
		if (res != expected)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res << ", expected "
												<< expected << tcu::TestLog::EndMessage;
			error = ERROR;
		}
	}

	virtual inline void VerifyGetProgramResourceLocation(GLuint program, GLenum		   programInterface,
														 std::map<std::string, GLint>& locations,
														 const std::string& name, long& error)
	{
		GLint res = glGetProgramResourceLocation(program, programInterface, name.c_str());
		if (res < 0)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res
												<< ", expected not less than 0" << tcu::TestLog::EndMessage;
			error = ERROR;
			return;
		}
		std::map<std::string, GLint>::iterator it = locations.begin();
		while (it != locations.end())
		{
			if (it->second == res)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "ERROR: Duplicated value found: " << res << tcu::TestLog::EndMessage;
				error = ERROR;
				return;
			}
			++it;
		}
		locations[name] = res;
	}

	virtual inline void VerifyGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index,
												   GLsizei propCount, const GLenum props[], GLsizei expectedLength,
												   const GLint expected[], long& error)
	{
		const GLsizei bufSize = 1000;
		GLsizei		  length;
		GLint		  params[bufSize];
		glGetProgramResourceiv(program, programInterface, index, propCount, props, bufSize, &length, params);
		if (length != expectedLength || length <= 0)
		{
			error = ERROR;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: Got length " << length << ", expected " << expectedLength
				<< "\nCALL: glGetProgramResourceiv, with " << programInterface << ", " << index
				<< tcu::TestLog::EndMessage;
			return;
		}
		for (int i = 0; i < length; ++i)
		{
			if (params[i] != expected[i])
			{
				error = ERROR;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "ERROR: Got " << params[i] << ", expected " << expected[i]
					<< " at: " << i << "\nCALL: glGetProgramResourceiv, with " << programInterface << ", " << index
					<< tcu::TestLog::EndMessage;
			}
		}
	}

	virtual inline void VerifyGetProgramResourceLocationIndex(GLuint program, GLenum programInterface,
															  const std::string& name, GLint expected, long& error)
	{
		GLint res = glGetProgramResourceLocationIndex(program, programInterface, name.c_str());
		if (res != expected)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: Got " << res << ", expected "
												<< expected << "\nCALL: glGetProgramResourceLocationIndex, with "
												<< programInterface << ", " << name << tcu::TestLog::EndMessage;
			error = ERROR;
		}
	}

	virtual inline GLint GetProgramivRetValue(GLuint program, GLenum pname)
	{
		GLint ret;
		glGetProgramiv(program, pname, &ret);
		return ret;
	}

	static const GLenum interfaces[];
};

const GLenum PIQBase::interfaces[] = { GL_PROGRAM_INPUT,
									   GL_PROGRAM_OUTPUT,
									   GL_UNIFORM,
									   GL_UNIFORM_BLOCK,
									   GL_BUFFER_VARIABLE,
									   GL_SHADER_STORAGE_BLOCK,
									   GL_VERTEX_SUBROUTINE,
									   GL_TESS_CONTROL_SUBROUTINE,
									   GL_TESS_EVALUATION_SUBROUTINE,
									   GL_GEOMETRY_SUBROUTINE,
									   GL_FRAGMENT_SUBROUTINE,
									   GL_COMPUTE_SUBROUTINE,
									   GL_VERTEX_SUBROUTINE_UNIFORM,
									   GL_TESS_CONTROL_SUBROUTINE_UNIFORM,
									   GL_TESS_EVALUATION_SUBROUTINE_UNIFORM,
									   GL_GEOMETRY_SUBROUTINE_UNIFORM,
									   GL_FRAGMENT_SUBROUTINE_UNIFORM,
									   GL_COMPUTE_SUBROUTINE_UNIFORM,
									   GL_TRANSFORM_FEEDBACK_VARYING };

class NoShaders : public PIQBase
{

	virtual std::string Title()
	{
		return "No Shaders Test";
	}

	virtual std::string ShadersDesc()
	{
		return "no shaders";
	}

	virtual long Run()
	{
		const GLuint program = glCreateProgram();

		long error = NO_ERROR;
		int  size  = sizeof(PIQBase::interfaces) / sizeof(PIQBase::interfaces[0]);

		for (int i = 0; i < size; ++i)
		{
			VerifyGetProgramInterfaceiv(program, PIQBase::interfaces[i], GL_ACTIVE_RESOURCES, 0, error);
			if (PIQBase::interfaces[i] == GL_ATOMIC_COUNTER_BUFFER)
				continue;
			VerifyGetProgramInterfaceiv(program, PIQBase::interfaces[i], GL_MAX_NAME_LENGTH, 0, error);
		}
		VerifyGetProgramInterfaceiv(program, GL_ATOMIC_COUNTER_BUFFER, GL_MAX_NUM_ACTIVE_VARIABLES, 0, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, 0, error);
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, 0, error);
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 0,
									error);
		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 0,
									error);
		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 0,
									error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES,
									0, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 0,
									error);
		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 0, error);

		for (int i = 0; i < size; ++i)
		{
			if (PIQBase::interfaces[i] == GL_ATOMIC_COUNTER_BUFFER)
				continue;
			VerifyGetProgramResourceIndex(program, PIQBase::interfaces[i], "", GL_INVALID_INDEX, error);
		}

		// can't test GetProgramResourceLocation* here since program has to be linked
		// can't test GetProgramResourceiv, need valid index

		glDeleteProgram(program);
		return error;
	}
};

class SimpleShaders : public PIQBase
{

public:
	virtual std::string Title()
	{
		return "Simple Shaders Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = position;          \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1);  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 9, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 6, error);

		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "color", 0, error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "position", 0, error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, 0, "color", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, 0, "position", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH };
		GLint expected[] = { 9, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 11, props, 11, expected, error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected2[] = { 6, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, 0, 12, props2, 12, expected2, error);

		glDeleteProgram(program);
		return error;
	}
};

class InputTypes : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Input Types Test";
	}

	virtual std::string ShadersDesc()
	{
		return "vertex shader with different `in` types and a fallthrough fragment shader";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in mat4 a;                           \n"
			   "in ivec4 b;                          \n"
			   "in float c[2];                       \n"
			   "in mat2x3 d[2];                      \n"
			   "in uvec2 e;                          \n"
			   "in uint f;                           \n"
			   "in vec3 g[2];                        \n"
			   "in int h;                            \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "   vec4 pos;                                           \n"
			   "   pos.w = h + g[0].x + g[1].y + d[1][1].y;            \n"
			   "   pos.y = b.x * c[0] + c[1] + d[0][0].x;              \n"
			   "   pos.x = a[0].x + a[1].y + a[2].z + a[3].w;          \n"
			   "   pos.z = d[0][1].z + e.x * f + d[1][0].z;            \n"
			   "   gl_Position = pos;                                  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "a");
		glBindAttribLocation(program, 4, "b");
		glBindAttribLocation(program, 5, "c");
		glBindAttribLocation(program, 7, "d");
		glBindAttribLocation(program, 11, "e");
		glBindAttribLocation(program, 12, "f");
		glBindAttribLocation(program, 13, "g");
		glBindAttribLocation(program, 15, "h");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 8, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "c[0]", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "e", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "f", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "g", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "h", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["c[0]"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["d"], "d[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["e"], "e", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["f"], "f", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["g"], "g[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["h"], "h", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "a", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "b", 4, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c[0]", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c[1]", 6, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "d[0]", 7, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "d", 7, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "e", 11, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "f", 12, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g[0]", 13, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g", 13, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g[1]", 14, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "h", 15, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH };
		GLint expected[] = { 2, GL_FLOAT_MAT4, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["a"], 11, props, 11, expected, error);
		GLint expected2[] = { 2, GL_INT_VEC4, 1, 0, 0, 0, 0, 0, 1, 4, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["b"], 11, props, 11, expected2, error);
		GLint expected3[] = { 5, GL_FLOAT, 2, 0, 0, 0, 0, 0, 1, 5, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["c[0]"], 11, props, 11, expected3, error);
		GLint expected4[] = { 5, GL_FLOAT_MAT2x3, 2, 0, 0, 0, 0, 0, 1, 7, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["d"], 11, props, 11, expected4, error);
		GLint expected5[] = { 2, GL_UNSIGNED_INT_VEC2, 1, 0, 0, 0, 0, 0, 1, 11, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["e"], 11, props, 11, expected5, error);
		GLint expected6[] = { 2, GL_UNSIGNED_INT, 1, 0, 0, 0, 0, 0, 1, 12, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["f"], 11, props, 11, expected6, error);
		GLint expected7[] = { 5, GL_FLOAT_VEC3, 2, 0, 0, 0, 0, 0, 1, 13, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["g"], 11, props, 11, expected7, error);
		GLint expected8[] = { 2, GL_INT, 1, 0, 0, 0, 0, 0, 1, 15, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["h"], 11, props, 11, expected8, error);

		glDeleteProgram(program);
		return error;
	}
};

class OutputTypes : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Output Types Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader with different `out` types and a fallthrough vertex shader";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec3 a[2];                 \n"
			   "out uint b;                    \n"
			   "out float c[2];                \n"
			   "out int d[2];                  \n"
			   "out vec2 e;                    \n"
			   "void main() {                  \n"
			   "    c[1] = -0.6;               \n"
			   "    d[0] = 0;                  \n"
			   "    b = 12u;                   \n"
			   "    c[0] = 1.1;                \n"
			   "    e = vec2(0, 1);            \n"
			   "    d[1] = -19;                \n"
			   "    a[1] = vec3(0, 1, 0);      \n"
			   "    a[0] = vec3(0, 1, 0);      \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "a");
		glBindFragDataLocation(program, 2, "b");
		glBindFragDataLocation(program, 3, "c");
		glBindFragDataLocation(program, 5, "d");
		glBindFragDataLocation(program, 7, "e");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 5, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "c[0]", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "e", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["a"], "a[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["c[0]"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["d"], "d[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["e"], "e", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a[0]", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a[1]", 1, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "b", 2, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c[0]", 3, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c", 3, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c[1]", 4, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d[0]", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d[1]", 6, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "e", 7, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "a[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "a", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "b", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "c[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "c", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "d[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "d", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "e", 0, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH,
						   GL_LOCATION_INDEX };
		GLint expected[] = { 5, GL_FLOAT_VEC3, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["a"], 12, props, 12, expected, error);
		GLint expected2[] = { 2, GL_UNSIGNED_INT, 1, 0, 1, 0, 0, 0, 0, 2, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["b"], 12, props, 12, expected2, error);
		GLint expected3[] = { 5, GL_FLOAT, 2, 0, 1, 0, 0, 0, 0, 3, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["c[0]"], 12, props, 12, expected3, error);
		GLint expected4[] = { 5, GL_INT, 2, 0, 1, 0, 0, 0, 0, 5, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["d"], 12, props, 12, expected4, error);
		GLint expected5[] = { 2, GL_FLOAT_VEC2, 1, 0, 1, 0, 0, 0, 0, 7, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["e"], 12, props, 12, expected5, error);

		glDeleteProgram(program);
		return error;
	}
};

class OutputLocationIndex : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Output Location Index Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader with `out` location index different from 0 and a fallthrough vertex shader";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocationIndexed(program, 0, 1, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "color", 1, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH,
						   GL_LOCATION_INDEX };
		GLint expected[] = { 6, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, 0, 12, props, 12, expected, error);

		glDeleteProgram(program);
		return error;
	}
};

class InputBuiltIn : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Input Built-ins Test";
	}

	virtual std::string ShadersDesc()
	{
		return "vertex shader using built-in variables and a fallthrough fragment shader";
	}

	virtual std::string Expectations()
	{
		return ".\n\n In this case we ask for information about built-in variables for the input interface.";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = (gl_VertexID + gl_InstanceID) * vec4(0.1);          \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 14, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "gl_VertexID", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "gl_InstanceID", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["gl_VertexID"], "gl_VertexID", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["gl_InstanceID"], "gl_InstanceID", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "gl_VertexID", -1, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "gl_InstanceID", -1, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH };
		GLint expected[] = { 12, GL_INT, 1, 0, 0, 0, 0, 0, 1, -1, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["gl_VertexID"], 11, props, 11, expected, error);
		GLint expected2[] = { 14, GL_INT, 1, 0, 0, 0, 0, 0, 1, -1, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["gl_InstanceID"], 11, props, 11, expected2,
								   error);

		glDeleteProgram(program);
		return error;
	}
};

class OutputBuiltIn : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Output Built-ins Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader using built-in variables and a fallthrough vertex shader";
	}

	virtual std::string Expectations()
	{
		return ".\n\n In this case we ask for information about built-in variables for the output interface.";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                               \n"
			   "void main(void)                            \n"
			   "{                                          \n"
			   "    gl_FragDepth = 0.1;                    \n"
			   "    gl_SampleMask[0] = 1;                  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), true);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 17, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "gl_FragDepth", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "gl_SampleMask[0]", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["gl_FragDepth"], "gl_FragDepth", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["gl_SampleMask[0]"], "gl_SampleMask[0]",
									 error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "gl_FragDepth", -1, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "gl_SampleMask", -1, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "gl_SampleMask[0]", -1, error);

		// spec is not clear what to require here
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "gl_FragDepth", -1, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "gl_SampleMask", -1, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "gl_SampleMask[0]", -1, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH,
						   GL_LOCATION_INDEX };
		GLint expected[] = { 13, GL_FLOAT, 1, 0, 1, 0, 0, 0, 0, -1, 0, -1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["gl_FragDepth"], 12, props, 12, expected, error);
		GLint expected2[] = { 17, GL_INT, 1, 0, 1, 0, 0, 0, 0, -1, 0, -1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["gl_SampleMask[0]"], 12, props, 12, expected2,
								   error);

		glDeleteProgram(program);
		return error;
	}
};

class InputLayout : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Input Layout Test";
	}

	virtual std::string ShadersDesc()
	{
		return "vertex shader with different `in` variables locations set through layout and a fallthrough fragment "
			   "shader";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "layout(location = 4) in ivec4 b;     \n"
			   "layout(location = 7) in mat2x3 d[2]; \n"
			   "layout(location = 5) in float c[2];  \n"
			   "layout(location = 12) in uint f;     \n"
			   "layout(location = 13) in vec3 g[2];  \n"
			   "layout(location = 0) in mat4 a;      \n"
			   "layout(location = 15) in int h;      \n"
			   "layout(location = 11) in uvec2 e;    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "   vec4 pos;                                           \n"
			   "   pos.w = h + g[0].x + g[1].y + d[1][1].y;            \n"
			   "   pos.y = b.x * c[0] + c[1] + d[0][0].x;              \n"
			   "   pos.x = a[0].x + a[1].y + a[2].z + a[3].w;          \n"
			   "   pos.z = d[0][1].z + e.x * f + d[1][0].z;            \n"
			   "   gl_Position = pos;                                  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 8, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "c[0]", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "e", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "f", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "g", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indices, "h", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["c[0]"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["d"], "d[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["e"], "e", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["f"], "f", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["g"], "g[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indices["h"], "h", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "a", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "b", 4, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c[0]", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "c[1]", 6, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "d[0]", 7, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "d", 7, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "e", 11, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "f", 12, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g[0]", 13, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g", 13, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "g[1]", 14, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "h", 15, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH };
		GLint expected[] = { 2, GL_FLOAT_MAT4, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["a"], 11, props, 11, expected, error);
		GLint expected2[] = { 2, GL_INT_VEC4, 1, 0, 0, 0, 0, 0, 1, 4, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["b"], 11, props, 11, expected2, error);
		GLint expected3[] = { 5, GL_FLOAT, 2, 0, 0, 0, 0, 0, 1, 5, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["c[0]"], 11, props, 11, expected3, error);
		GLint expected4[] = { 5, GL_FLOAT_MAT2x3, 2, 0, 0, 0, 0, 0, 1, 7, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["d"], 11, props, 11, expected4, error);
		GLint expected5[] = { 2, GL_UNSIGNED_INT_VEC2, 1, 0, 0, 0, 0, 0, 1, 11, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["e"], 11, props, 11, expected5, error);
		GLint expected6[] = { 2, GL_UNSIGNED_INT, 1, 0, 0, 0, 0, 0, 1, 12, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["f"], 11, props, 11, expected6, error);
		GLint expected7[] = { 5, GL_FLOAT_VEC3, 2, 0, 0, 0, 0, 0, 1, 13, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["g"], 11, props, 11, expected7, error);
		GLint expected8[] = { 2, GL_INT, 1, 0, 0, 0, 0, 0, 1, 15, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indices["h"], 11, props, 11, expected8, error);

		glDeleteProgram(program);
		return error;
	}
};

class OutputLayout : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Output Layout Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader with different `out` variables locations set through layout and a fallthrough vertex "
			   "shader";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "layout(location = 2) out uint b;                    \n"
			   "layout(location = 7) out vec2 e;                    \n"
			   "layout(location = 3) out float c[2];                \n"
			   "layout(location = 5) out int d[2];                  \n"
			   "layout(location = 0) out vec3 a[2];                 \n"
			   "void main() {                  \n"
			   "    c[1] = -0.6;               \n"
			   "    d[0] = 0;                  \n"
			   "    b = 12u;                   \n"
			   "    c[0] = 1.1;                \n"
			   "    e = vec2(0, 1);            \n"
			   "    d[1] = -19;                \n"
			   "    a[1] = vec3(0, 1, 0);      \n"
			   "    a[0] = vec3(0, 1, 0);      \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 5, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "c[0]", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indices, "e", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["a"], "a[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["c[0]"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["d"], "d[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indices["e"], "e", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a[0]", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "a[1]", 1, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "b", 2, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c[0]", 3, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c", 3, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "c[1]", 4, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d[0]", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d", 5, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "d[1]", 6, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "e", 7, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "a[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "a", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "b", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "c[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "c", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "d[0]", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "d", 0, error);
		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "e", 0, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH,
						   GL_LOCATION_INDEX };
		GLint expected[] = { 5, GL_FLOAT_VEC3, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["a"], 12, props, 12, expected, error);
		GLint expected2[] = { 2, GL_UNSIGNED_INT, 1, 0, 1, 0, 0, 0, 0, 2, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["b"], 12, props, 12, expected2, error);
		GLint expected3[] = { 5, GL_FLOAT, 2, 0, 1, 0, 0, 0, 0, 3, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["c[0]"], 12, props, 12, expected3, error);
		GLint expected4[] = { 5, GL_INT, 2, 0, 1, 0, 0, 0, 0, 5, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["d"], 12, props, 12, expected4, error);
		GLint expected5[] = { 2, GL_FLOAT_VEC2, 1, 0, 1, 0, 0, 0, 0, 7, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indices["e"], 12, props, 12, expected5, error);

		glDeleteProgram(program);
		return error;
	}
};

class OutputLayoutIndex : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Output Layout Index Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader with different `out` variables fragment color index\n"
			   "locations set through layout and a fallthrough vertex shader";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "layout(location = 0, index = 1) out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1);  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "color", 1, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH,
						   GL_LOCATION_INDEX };
		GLint expected[] = { 6, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, 0, 12, props, 12, expected, error);

		glDeleteProgram(program);
		return error;
	}
};

class UniformSimple : public PIQBase
{
	virtual std::string Title()
	{
		return "Uniform Simple Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with uniforms used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_UNIFORM as an interface param.";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "uniform vec4 repos;                  \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = position + repos;  \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "uniform vec4 recolor;          \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1) + recolor;  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
									GetProgramivRetValue(program, GL_ACTIVE_UNIFORMS), error);
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NAME_LENGTH, 8, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "repos", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "recolor", error);

		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["repos"], "repos", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["recolor"], "recolor", error);

		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "repos", glGetUniformLocation(program, "repos"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "recolor", glGetUniformLocation(program, "recolor"),
										 error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_OFFSET,
						   GL_BLOCK_INDEX,
						   GL_ARRAY_STRIDE,
						   GL_MATRIX_STRIDE,
						   GL_IS_ROW_MAJOR,
						   GL_ATOMIC_COUNTER_BUFFER_INDEX,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION };
		GLint expected[] = {
			6, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "repos")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["repos"], 16, props, 16, expected, error);

		GLint expected2[] = {
			8, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "recolor")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["recolor"], 16, props, 16, expected2, error);

		glDeleteProgram(program);
		return error;
	}
};

class UniformTypes : public PIQBase
{
	virtual std::string Title()
	{
		return "Uniform Types Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different uniform types used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_UNIFORM as an interface param.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "uniform vec4 a;                      \n"
			   "uniform ivec3 b;                     \n"
			   "uniform uvec2 c[3];                  \n"
			   "uniform mat2 g[8];                   \n"
			   "uniform mat3x2 i;                    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    float tmp;                       \n"
			   "    tmp =  g[0][1][1] + a.z + b.y + c[0].x;   \n"
			   "    tmp += g[1][1][1] + c[1].x;      \n"
			   "    tmp += g[2][1][1] + c[2].x;      \n"
			   "    for (int i = 3; i < 8; ++i)      \n"
			   "        tmp += g[i][1][1];           \n"
			   "    tmp = tmp + i[1][1];             \n"
			   "    gl_Position = position * tmp;    \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "struct U {                     \n"
			   "   bool a[3];                  \n"
			   "   vec4 b;                     \n"
			   "   mat3 c;                     \n"
			   "   float d[2];                 \n"
			   "};                             \n"
			   "struct UU {                    \n"
			   "   U a;                        \n"
			   "   U b[2];                     \n"
			   "   uvec2 c;                    \n"
			   "};                             \n"
			   "uniform mat4 d;                \n"
			   "uniform mat3 e;                \n"
			   "uniform float h;               \n"
			   "uniform int f;                 \n"
			   "uniform U j;                   \n"
			   "uniform UU k;                  \n"
			   "uniform UU l[3];               \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    float tmp;                 \n"
			   "    tmp = h + f + e[2][2];           \n"
			   "    tmp = tmp + d[0][0] + j.b.x;     \n"
			   "    tmp = tmp + k.b[0].c[0][0];      \n"
			   "    tmp = tmp + l[2].a.c[0][1];      \n"
			   "    tmp = tmp + l[2].b[1].d[0];      \n"
			   "    tmp = tmp + l[2].b[1].d[1];      \n"
			   "    tmp = tmp + l[0].c.x;            \n"
			   "    color = vec4(0, 1, 0, 1) * tmp;  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		// only active structure members
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
									GetProgramivRetValue(program, GL_ACTIVE_UNIFORMS), error);
		// l[2].b[1].d[0] and \0
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NAME_LENGTH, 15, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "b", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "e", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "f", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "g", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "h", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "i", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "j.b", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "k.b[0].c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "l[0].c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "l[2].b[1].d[0]", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "l[2].a.c", error);

		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["c"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["d"], "d", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["e"], "e", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["f"], "f", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["g"], "g[0]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["h"], "h", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["i"], "i", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["j.b"], "j.b", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["k.b[0].c"], "k.b[0].c", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["l[0].c"], "l[0].c", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["l[2].b[1].d[0]"], "l[2].b[1].d[0]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["l[2].a.c"], "l[2].a.c", error);

		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a", glGetUniformLocation(program, "a"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "b", glGetUniformLocation(program, "b"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "c", glGetUniformLocation(program, "c"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "d", glGetUniformLocation(program, "d"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "e", glGetUniformLocation(program, "e"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "f", glGetUniformLocation(program, "f"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "g", glGetUniformLocation(program, "g"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "h", glGetUniformLocation(program, "h"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "i", glGetUniformLocation(program, "i"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "j.b", glGetUniformLocation(program, "j.b"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "k.b[0].c", glGetUniformLocation(program, "k.b[0].c"),
										 error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "l[0].c", glGetUniformLocation(program, "l[0].c"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "l[2].b[1].d[0]",
										 glGetUniformLocation(program, "l[2].b[1].d[0]"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "l[2].a.c", glGetUniformLocation(program, "l[2].a.c"),
										 error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_OFFSET,
						   GL_BLOCK_INDEX,
						   GL_ARRAY_STRIDE,
						   GL_MATRIX_STRIDE,
						   GL_IS_ROW_MAJOR,
						   GL_ATOMIC_COUNTER_BUFFER_INDEX,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION };
		GLint expected[] = {
			2, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "a")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["a"], 16, props, 16, expected, error);
		GLint expected2[] = { 2,  GL_INT_VEC3, 1, -1, -1, -1, -1, 0,
							  -1, 0,		   0, 0,  0,  0,  1,  glGetUniformLocation(program, "b") };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["b"], 16, props, 16, expected2, error);
		GLint expected3[] = {
			5, GL_UNSIGNED_INT_VEC2, 3, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "c")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["c"], 16, props, 16, expected3, error);
		GLint expected4[] = {
			2, GL_FLOAT_MAT4, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "d")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["d"], 16, props, 16, expected4, error);
		GLint expected5[] = {
			2, GL_FLOAT_MAT3, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "e")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["e"], 16, props, 16, expected5, error);
		GLint expected6[] = {
			2, GL_INT, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "f")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["f"], 16, props, 16, expected6, error);
		GLint expected7[] = {
			5, GL_FLOAT_MAT2, 8, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "g")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["g"], 16, props, 16, expected7, error);
		GLint expected8[] = { 2,  GL_FLOAT, 1, -1, -1, -1, -1, 0,
							  -1, 0,		1, 0,  0,  0,  0,  glGetUniformLocation(program, "h") };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["h"], 16, props, 16, expected8, error);
		GLint expected9[] = {
			2, GL_FLOAT_MAT3x2, 1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "i")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["i"], 16, props, 16, expected9, error);
		GLint expected10[] = {
			4, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "j.b")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["j.b"], 16, props, 16, expected10, error);
		GLint expected11[] = {
			9, GL_FLOAT_MAT3, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "k.b[0].c")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["k.b[0].c"], 16, props, 16, expected11, error);
		GLint expected12[] = { 7,  GL_UNSIGNED_INT_VEC2,
							   1,  -1,
							   -1, -1,
							   -1, 0,
							   -1, 0,
							   1,  0,
							   0,  0,
							   0,  glGetUniformLocation(program, "l[0].c") };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["l[0].c"], 16, props, 16, expected12, error);
		GLint expected13[] = { 15, GL_FLOAT, 2, -1, -1, -1, -1, 0,
							   -1, 0,		 1, 0,  0,  0,  0,  glGetUniformLocation(program, "l[2].b[1].d[0]") };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["l[2].b[1].d[0]"], 16, props, 16, expected13, error);
		GLint expected14[] = {
			9, GL_FLOAT_MAT3, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 0, 0, 0, glGetUniformLocation(program, "l[2].a.c")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["l[2].a.c"], 16, props, 16, expected14, error);

		glDeleteProgram(program);
		return error;
	}
};

class UniformBlockTypes : public PIQBase
{
	virtual std::string Title()
	{
		return "Uniform Block Types Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different types of uniform blocks used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_UNIFORM_BLOCK as an interface param.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "uniform SimpleBlock {                \n"
			   "   mat3x2 a;                         \n"
			   "   mat4 b;                           \n"
			   "   vec4 c;                           \n"
			   "};                                   \n"
			   ""
			   "uniform NotSoSimpleBlockk {          \n"
			   "   ivec2 a[4];                       \n"
			   "   mat3 b[2];                        \n"
			   "   mat2 c;                           \n"
			   "} d;                                 \n"
			   ""
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    float tmp;                       \n"
			   "    tmp =  a[0][1] * b[1][2] * c.x;  \n"
			   "    tmp = tmp + d.a[2].y + d.b[0][1][1] + d.c[1][1];             \n"
			   "    gl_Position = position * tmp;    \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "struct U {                     \n"
			   "   bool a[3];                  \n"
			   "   vec4 b;                     \n"
			   "   mat3 c;                     \n"
			   "   float d[2];                 \n"
			   "};                             \n"
			   "struct UU {                    \n"
			   "   U a;                        \n"
			   "   U b[2];                     \n"
			   "   uvec2 c;                    \n"
			   "};                             \n"
			   ""
			   "uniform TrickyBlock {                            \n"
			   "   UU a[3];                                      \n"
			   "   mat4 b;                                       \n"
			   "   uint c;                                       \n"
			   "} e[2];                                          \n"
			   ""
			   "out vec4 color;                                \n"
			   "void main() {                                  \n"
			   "    float tmp;                                 \n"
			   "    tmp = e[0].a[2].b[0].d[1];                 \n"
			   "    color = vec4(0, 1, 0, 1) * tmp;            \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES,
									GetProgramivRetValue(program, GL_ACTIVE_UNIFORMS), error);
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, 4, error);
		VerifyGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, 18, error);

		std::map<std::string, GLuint> indicesUB;
		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, indicesUB, "SimpleBlock", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, indicesUB, "NotSoSimpleBlockk", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, indicesUB, "TrickyBlock", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, indicesUB, "TrickyBlock[1]", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "a", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "b", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "NotSoSimpleBlockk.a[0]", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "NotSoSimpleBlockk.c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "NotSoSimpleBlockk.b[0]", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "TrickyBlock.a[2].b[0].d", error);

		glUniformBlockBinding(program, indicesUB["SimpleBlock"], 0);
		glUniformBlockBinding(program, indicesUB["NotSoSimpleBlockk"], 2);
		glUniformBlockBinding(program, indicesUB["TrickyBlock"], 3);
		glUniformBlockBinding(program, indicesUB["TrickyBlock[1]"], 4);

		VerifyGetProgramResourceName(program, GL_UNIFORM_BLOCK, indicesUB["SimpleBlock"], "SimpleBlock", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM_BLOCK, indicesUB["NotSoSimpleBlockk"], "NotSoSimpleBlockk",
									 error);
		VerifyGetProgramResourceName(program, GL_UNIFORM_BLOCK, indicesUB["TrickyBlock"], "TrickyBlock[0]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM_BLOCK, indicesUB["TrickyBlock[1]"], "TrickyBlock[1]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["c"], "c", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["NotSoSimpleBlockk.a[0]"], "NotSoSimpleBlockk.a[0]",
									 error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["NotSoSimpleBlockk.c"], "NotSoSimpleBlockk.c",
									 error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["NotSoSimpleBlockk.b[0]"], "NotSoSimpleBlockk.b[0]",
									 error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["TrickyBlock.a[2].b[0].d"],
									 "TrickyBlock.a[2].b[0].d[0]", error);

		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "b", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "c", -1, error);

		GLenum props[] = {
			GL_NAME_LENGTH,
			GL_BUFFER_BINDING,
			GL_REFERENCED_BY_COMPUTE_SHADER,
			GL_REFERENCED_BY_FRAGMENT_SHADER,
			GL_REFERENCED_BY_GEOMETRY_SHADER,
			GL_REFERENCED_BY_TESS_CONTROL_SHADER,
			GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
			GL_REFERENCED_BY_VERTEX_SHADER,
			GL_BUFFER_DATA_SIZE,
		};
		GLint size;
		glGetActiveUniformBlockiv(program, indicesUB["SimpleBlock"], GL_UNIFORM_BLOCK_DATA_SIZE, &size);
		GLint expected[] = { 12, 0, 0, 0, 0, 0, 0, 1, size };
		VerifyGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["SimpleBlock"], 9, props, 9, expected, error);
		glGetActiveUniformBlockiv(program, indicesUB["NotSoSimpleBlockk"], GL_UNIFORM_BLOCK_DATA_SIZE, &size);
		GLint expected2[] = { 18, 2, 0, 0, 0, 0, 0, 1, size };
		VerifyGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["NotSoSimpleBlockk"], 9, props, 9, expected2,
								   error);
		glGetActiveUniformBlockiv(program, indicesUB["TrickyBlock"], GL_UNIFORM_BLOCK_DATA_SIZE, &size);
		GLint expected3[] = { 15, 3, 0, 1, 0, 0, 0, 0, size };
		VerifyGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["TrickyBlock"], 9, props, 9, expected3, error);
		GLint expected4[] = { 15, 4, 0, 0, 0, 0, 0, 0, size };
		VerifyGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["TrickyBlock[1]"], 9, props, 9, expected4,
								   error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_ARRAY_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_ATOMIC_COUNTER_BUFFER_INDEX,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION };
		GLint expected5[] = {
			2, GL_FLOAT_MAT3x2, 1, static_cast<GLint>(indicesUB["SimpleBlock"]), 0, 0, -1, 0, 0, 0, 0, 0, 1, -1
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 14, props2, 14, expected5, error);
		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_MATRIX_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_ATOMIC_COUNTER_BUFFER_INDEX,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION };
		GLint expected6[] = { 27, GL_FLOAT, 2, static_cast<GLint>(indicesUB["TrickyBlock"]), 0, 0, -1, 0, 1, 0, 0,
							  0,  0,		-1 };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indicesU["TrickyBlock.a[2].b[0].d"], 14, props3, 14, expected6,
								   error);

		GLenum			 prop	= GL_ACTIVE_VARIABLES;
		const GLsizei	bufSize = 1000;
		GLsizei			 length;
		GLint			 param[bufSize];
		std::set<GLuint> exp;
		exp.insert(indicesU["a"]);
		exp.insert(indicesU["b"]);
		exp.insert(indicesU["c"]);
		glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["SimpleBlock"], 1, &prop, bufSize, &length, param);
		for (int i = 0; i < length; ++i)
		{
			if (exp.find(param[i]) == exp.end())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Unexpected index found in active variables of SimpleBlock: " << param[i]
					<< "\nCall: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: GL_UNIFORM_BLOCK"
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
			else if (length != 3)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Call: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: GL_UNIFORM_BLOCK\n"
					<< "Expected length: 3, actual length: " << length << tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}
		std::set<GLuint> exp2;
		exp2.insert(indicesU["NotSoSimpleBlockk.a[0]"]);
		exp2.insert(indicesU["NotSoSimpleBlockk.b[0]"]);
		exp2.insert(indicesU["NotSoSimpleBlockk.c"]);
		glGetProgramResourceiv(program, GL_UNIFORM_BLOCK, indicesUB["NotSoSimpleBlockk"], 1, &prop, bufSize, &length,
							   param);
		for (int i = 0; i < length; ++i)
		{
			if (exp2.find(param[i]) == exp2.end())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Unexpected index found in active variables of NotSoSimpleBlockk: " << param[i]
					<< "\nCall: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: GL_UNIFORM_BLOCK"
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
			else if (length != 3)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Call: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: GL_UNIFORM_BLOCK"
					<< "\nExpected length: 3, actual length: " << length << tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}

		GLint res;
		glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, &res);
		if (res < 3)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Value of GL_MAX_NUM_ACTIVE_VARIABLES less than 3!"
				<< tcu::TestLog::EndMessage;
			glDeleteProgram(program);
			return ERROR;
		}

		glDeleteProgram(program);
		return error;
	}
};

class TransformFeedbackTypes : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Transform Feedback Varying Types";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different types of out variables used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_TRANSFORM_FEEDBACK_VARYING as an interface param.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "flat out ivec4 a;                    \n"
			   "out float b[2];                      \n"
			   "flat out uvec2 c;                    \n"
			   "flat out uint d;                     \n"
			   "out vec3 e[2];                       \n"
			   "flat out int f;                      \n"
			   ""
			   "void main(void)                      \n"
			   "{                                    \n"
			   "   vec4 pos;                         \n"
			   "   a = ivec4(1);                     \n"
			   "   b[0] = 1.1;                       \n"
			   "   b[1] = 1.1;                       \n"
			   "   c = uvec2(1u);                    \n"
			   "   d = 1u;                           \n"
			   "   e[0] = vec3(1.1);                 \n"
			   "   e[1] = vec3(1.1);                 \n"
			   "   f = 1;                            \n"
			   "   gl_Position = position;           \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		const char* varyings[6] = { "a", "b[0]", "b[1]", "c", "d", "e" };
		glTransformFeedbackVaryings(program, 6, varyings, GL_INTERLEAVED_ATTRIBS);
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, 6, error);
		VerifyGetProgramInterfaceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "a", error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "b[0]", error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "b[1]", error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "c", error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "d", error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, indices, "e", error);

		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["b[0]"], "b[0]", error);
		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["b[1]"], "b[1]", error);
		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["c"], "c", error);
		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["d"], "d", error);
		VerifyGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["e"], "e", error);

		GLenum props[]	= { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE };
		GLint  expected[] = { 2, GL_INT_VEC4, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["a"], 3, props, 3, expected, error);
		GLint expected2[] = { 5, GL_FLOAT, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["b[0]"], 3, props, 3, expected2,
								   error);
		GLint expected3[] = { 5, GL_FLOAT, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["b[1]"], 3, props, 3, expected3,
								   error);
		GLint expected4[] = { 2, GL_UNSIGNED_INT_VEC2, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["c"], 3, props, 3, expected4, error);
		GLint expected5[] = { 2, GL_UNSIGNED_INT, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["d"], 3, props, 3, expected5, error);
		GLint expected6[] = { 2, GL_FLOAT_VEC3, 2 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["e"], 3, props, 3, expected6, error);

		glDeleteProgram(program);
		return error;
	}
};

class AtomicCounterSimple : public SimpleShaders
{
public:
	virtual std::string Title()
	{
		return "Atomic Counter Buffer Simple Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with atomic counters used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_ATOMIC_COUNTER_BUFFER as an interface param.\n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   ""
			   "layout (binding = 1, offset = 0) uniform atomic_uint a;    \n"
			   "layout (binding = 2, offset = 0) uniform atomic_uint b;    \n"
			   "layout (binding = 2, offset = 4) uniform atomic_uint c;    \n"
			   "layout (binding = 5, offset = 0) uniform atomic_uint d[3]; \n"
			   "layout (binding = 5, offset = 12) uniform atomic_uint e;   \n"
			   ""
			   "void main() {                                                         \n"
			   "   uint x = atomicCounterIncrement(d[0]) + atomicCounterIncrement(a); \n"
			   "   uint y = atomicCounterIncrement(d[1]) + atomicCounterIncrement(b); \n"
			   "   uint z = atomicCounterIncrement(d[2]) + atomicCounterIncrement(c); \n"
			   "   uint w = atomicCounterIncrement(e);                                \n"
			   "   color = vec4(float(x), float(y), float(z), float(w));              \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_ATOMIC_COUNTER_BUFFER, GL_ACTIVE_RESOURCES, 3, error);
		VerifyGetProgramInterfaceiv(program, GL_ATOMIC_COUNTER_BUFFER, GL_MAX_NUM_ACTIVE_VARIABLES, 2, error);

		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "a", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "b", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "c", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "d", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "e", error);

		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["c"], "c", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["d"], "d[0]", error);
		VerifyGetProgramResourceName(program, GL_UNIFORM, indicesU["e"], "e", error);

		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "b", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "c", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "d", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "e", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "d[0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "d[1]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "d[2]", -1, error);

		GLenum		  prop	= GL_ATOMIC_COUNTER_BUFFER_INDEX;
		const GLsizei bufSize = 1000;
		GLsizei		  length;
		GLint		  res;
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 1, &prop, bufSize, &length, &res);

		GLenum props[] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES, GL_ACTIVE_VARIABLES };
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 1, &prop, bufSize, &length, &res);
		GLint expected[] = { 1, 4, 1, static_cast<GLint>(indicesU["a"]) };
		VerifyGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 4, props, 4, expected, error);

		GLenum props2[] = { GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES };
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["b"], 1, &prop, bufSize, &length, &res);
		GLint expected2[] = { 2, 8, 2 };
		VerifyGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 3, props2, 3, expected2, error);
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["c"], 1, &prop, bufSize, &length, &res);
		VerifyGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 3, props2, 3, expected2, error);

		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["d"], 1, &prop, bufSize, &length, &res);
		GLint expected3[] = { 5, 16, 2 };
		VerifyGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 3, props2, 3, expected3, error);
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["e"], 1, &prop, bufSize, &length, &res);
		VerifyGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 3, props2, 3, expected3, error);

		GLenum			 prop2 = GL_ACTIVE_VARIABLES;
		GLint			 param[bufSize];
		std::set<GLuint> exp;
		exp.insert(indicesU["b"]);
		exp.insert(indicesU["c"]);
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["b"], 1, &prop, bufSize, &length, &res);
		glGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 1, &prop2, bufSize, &length, param);
		for (int i = 0; i < length; ++i)
		{
			if (exp.find(param[i]) == exp.end() || length != 2)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Length: " << length
					<< "\nUnexpected index/length found in active variables of ATOMIC_COUNTER_BUFFER: " << param[i]
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "GL_ACTIVE_VARIABLES ok for 1st ATOMIC_COUNTER_BUFFER"
			<< tcu::TestLog::EndMessage;
		std::set<GLuint> exp2;
		GLint			 param2[bufSize];
		exp2.insert(indicesU["d"]);
		exp2.insert(indicesU["e"]);
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["d"], 1, &prop, bufSize, &length, &res);
		glGetProgramResourceiv(program, GL_ATOMIC_COUNTER_BUFFER, res, 1, &prop2, bufSize, &length, param2);
		for (int i = 0; i < length; ++i)
		{
			if (exp2.find(param2[i]) == exp2.end() || length != 2)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Length: " << length
					<< "\nUnexpected index/length found in active variables of ATOMIC_COUNTER_BUFFER: " << param2[i]
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}

		glDeleteProgram(program);
		return error;
	}
};

class SubroutinesBase : public SimpleShaders
{
protected:
	virtual std::string ShadersDesc()
	{
		return "fallthrough vertex, geometry, tesselation and fragment shaders with subroutines used";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 x() {           \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   "subroutine(a_t) vec4 y() {           \n"
			   "   return vec4(0);                   \n"
			   "}                                    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "   gl_Position = a();                \n"
			   "}";
	}

	virtual std::string TessControlShader()
	{
		return "#version 430                                                  \n"
			   "layout(vertices = 3) out;                                     \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 x() {           \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   ""
			   "void main() {                                                                   \n"
			   "   gl_out[gl_InvocationID].gl_Position = a();                                   \n"
			   "   gl_TessLevelInner[0] = 1.0;                                                  \n"
			   "   gl_TessLevelInner[1] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[0] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[1] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[2] = 1.0;                                                  \n"
			   "}";
	};

	virtual std::string TessEvalShader()
	{
		return "#version 430                                                  \n"
			   "layout(triangles, equal_spacing) in;                          \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 x() {           \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   ""
			   "void main() {                                \n"
			   "   vec4 p0 = gl_in[0].gl_Position;           \n"
			   "   vec4 p1 = gl_in[1].gl_Position;           \n"
			   "   vec4 p2 = gl_in[2].gl_Position;           \n"
			   "   vec3 p = gl_TessCoord.xyz;                \n"
			   "   gl_Position = a();                        \n"
			   "}";
	}

	virtual std::string GeometryShader()
	{
		return "#version 430                                                  \n"
			   "layout(triangles) in;                                         \n"
			   "layout(triangle_strip, max_vertices = 4) out;                 \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 x() {           \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   ""
			   "void main() {                              \n"
			   "   gl_Position = vec4(-1, 1, 0, 1);        \n"
			   "   EmitVertex();                           \n"
			   "   gl_Position = vec4(-1, -1, 0, 1);       \n"
			   "   EmitVertex();                           \n"
			   "   gl_Position = vec4(1, 1, 0, 1);         \n"
			   "   EmitVertex();                           \n"
			   "   gl_Position = a();                      \n"
			   "   EmitVertex();                           \n"
			   "   EndPrimitive();                         \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 x() {           \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   ""
			   "void main() {                             \n"
			   "   color = a();                           \n"
			   "}";
	}

	virtual void inline VerifyVS(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE, GL_ACTIVE_RESOURCES, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE, GL_MAX_NAME_LENGTH, 2, error);

		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 2, error);

		std::map<std::string, GLuint> indicesS;
		VerifyGetProgramResourceIndex(program, GL_VERTEX_SUBROUTINE, indicesS, "x", error);
		VerifyGetProgramResourceIndex(program, GL_VERTEX_SUBROUTINE, indicesS, "y", error);
		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(program, GL_VERTEX_SUBROUTINE_UNIFORM, indicesU, "a", error);

		VerifyGetProgramResourceName(program, GL_VERTEX_SUBROUTINE, indicesS["x"], "x", error);
		VerifyGetProgramResourceName(program, GL_VERTEX_SUBROUTINE, indicesS["y"], "y", error);
		VerifyGetProgramResourceName(program, GL_VERTEX_SUBROUTINE_UNIFORM, indicesU["a"], "a", error);

		VerifyGetProgramResourceLocation(program, GL_VERTEX_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_VERTEX_SHADER, "a"), error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 2 };
		VerifyGetProgramResourceiv(program, GL_VERTEX_SUBROUTINE, indicesS["x"], 1, propsS, 1, expectedS, error);
		VerifyGetProgramResourceiv(program, GL_VERTEX_SUBROUTINE, indicesS["y"], 1, propsS, 1, expectedS, error);

		GLenum propsU[]	= { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION };
		GLint  expectedU[] = { 2, 1, 2, glGetSubroutineUniformLocation(program, GL_VERTEX_SHADER, "a") };
		VerifyGetProgramResourceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, indicesU["a"], 4, propsU, 4, expectedU,
								   error);

		GLenum			 prop	= GL_COMPATIBLE_SUBROUTINES;
		const GLsizei	bufSize = 1000;
		GLint			 param[bufSize];
		GLsizei			 length;
		std::set<GLuint> exp;
		exp.insert(indicesS["x"]);
		exp.insert(indicesS["y"]);
		glGetProgramResourceiv(program, GL_VERTEX_SUBROUTINE_UNIFORM, indicesU["a"], 1, &prop, bufSize, &length, param);
		for (int i = 0; i < length; ++i)
		{
			if (exp.find(param[i]) == exp.end() || length != 2)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Length: " << length
					<< "\nUnexpected index/length found in active variables of GL_VERTEX_SUBROUTINE_UNIFORM: "
					<< param[i] << tcu::TestLog::EndMessage;
				error = ERROR;
			}
		}
	}

	virtual void inline VerifyTCS(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE, GL_MAX_NAME_LENGTH, 2, error);

		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 1,
									error);

		std::map<std::string, GLuint> indicesTS;
		VerifyGetProgramResourceIndex(program, GL_TESS_CONTROL_SUBROUTINE, indicesTS, "x", error);
		std::map<std::string, GLuint> indicesTU;
		VerifyGetProgramResourceIndex(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, indicesTU, "a", error);

		VerifyGetProgramResourceName(program, GL_TESS_CONTROL_SUBROUTINE, indicesTS["x"], "x", error);
		VerifyGetProgramResourceName(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, indicesTU["a"], "a", error);

		VerifyGetProgramResourceLocation(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_TESS_CONTROL_SHADER, "a"), error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 2 };
		VerifyGetProgramResourceiv(program, GL_TESS_CONTROL_SUBROUTINE, static_cast<GLint>(indicesTS["x"]), 1, propsS,
								   1, expectedS, error);

		GLenum propsU[] = { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION,
							GL_COMPATIBLE_SUBROUTINES };
		GLint expectedU[] = { 2, 1, 1, glGetSubroutineUniformLocation(program, GL_TESS_CONTROL_SHADER, "a"),
							  static_cast<GLint>(indicesTS["x"]) };
		VerifyGetProgramResourceiv(program, GL_TESS_CONTROL_SUBROUTINE_UNIFORM, indicesTU["a"], 5, propsU, 5, expectedU,
								   error);
	}
};

class SubroutinesVertex : public SubroutinesBase
{
	virtual std::string Title()
	{
		return "Vertex Shader Subroutines Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *VERTEX_SUBROUTINE* as an interface params.\n";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), TessControlShader().c_str(), TessEvalShader().c_str(),
									   GeometryShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);
		long error = NO_ERROR;

		VerifyVS(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class SubroutinesTessControl : public SubroutinesBase
{
	virtual std::string Title()
	{
		return "Tess Control Subroutines Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *TESS_CONTROL_SUBROUTINE* as an interface params.\n";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), TessControlShader().c_str(), TessEvalShader().c_str(),
									   GeometryShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);
		long error = NO_ERROR;

		VerifyTCS(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class SubroutinesTessEval : public SubroutinesBase
{
	virtual std::string Title()
	{
		return "Tess Evaluation Subroutines Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *TESS_EVALUATION_SUBROUTINE* as an interface params.\n";
	}

	virtual void inline VerifyTES(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE, GL_MAX_NAME_LENGTH, 2, error);

		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES,
									1, error);

		std::map<std::string, GLuint> indicesTS;
		VerifyGetProgramResourceIndex(program, GL_TESS_EVALUATION_SUBROUTINE, indicesTS, "x", error);
		std::map<std::string, GLuint> indicesTU;
		VerifyGetProgramResourceIndex(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, indicesTU, "a", error);

		VerifyGetProgramResourceName(program, GL_TESS_EVALUATION_SUBROUTINE, indicesTS["x"], "x", error);
		VerifyGetProgramResourceName(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, indicesTU["a"], "a", error);

		VerifyGetProgramResourceLocation(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_TESS_EVALUATION_SHADER, "a"),
										 error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 2 };
		VerifyGetProgramResourceiv(program, GL_TESS_EVALUATION_SUBROUTINE, indicesTS["x"], 1, propsS, 1, expectedS,
								   error);

		GLenum propsU[] = { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION,
							GL_COMPATIBLE_SUBROUTINES };
		GLint expectedU[] = { 2, 1, 1, glGetSubroutineUniformLocation(program, GL_TESS_EVALUATION_SHADER, "a"),
							  static_cast<GLint>(indicesTS["x"]) };
		VerifyGetProgramResourceiv(program, GL_TESS_EVALUATION_SUBROUTINE_UNIFORM, indicesTU["a"], 5, propsU, 5,
								   expectedU, error);
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), TessControlShader().c_str(), TessEvalShader().c_str(),
									   GeometryShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);
		long error = NO_ERROR;

		VerifyTES(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class SubroutinesGeometry : public SubroutinesBase
{
	virtual std::string Title()
	{
		return "Geometry Shader Subroutines Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *GEOMETRY_SUBROUTINE* as an interface params.\n";
	}

	virtual void inline VerifyGEO(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE, GL_MAX_NAME_LENGTH, 2, error);

		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 1,
									error);

		std::map<std::string, GLuint> indicesTS;
		VerifyGetProgramResourceIndex(program, GL_GEOMETRY_SUBROUTINE, indicesTS, "x", error);
		std::map<std::string, GLuint> indicesTU;
		VerifyGetProgramResourceIndex(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, indicesTU, "a", error);

		VerifyGetProgramResourceName(program, GL_GEOMETRY_SUBROUTINE, indicesTS["x"], "x", error);
		VerifyGetProgramResourceName(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, indicesTU["a"], "a", error);

		VerifyGetProgramResourceLocation(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_GEOMETRY_SHADER, "a"), error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 2 };
		VerifyGetProgramResourceiv(program, GL_GEOMETRY_SUBROUTINE, indicesTS["x"], 1, propsS, 1, expectedS, error);

		GLenum propsU[] = { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION,
							GL_COMPATIBLE_SUBROUTINES };
		GLint expectedU[] = { 2, 1, 1, glGetSubroutineUniformLocation(program, GL_GEOMETRY_SHADER, "a"),
							  static_cast<GLint>(indicesTS["x"]) };
		VerifyGetProgramResourceiv(program, GL_GEOMETRY_SUBROUTINE_UNIFORM, indicesTU["a"], 5, propsU, 5, expectedU,
								   error);
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), TessControlShader().c_str(), TessEvalShader().c_str(),
									   GeometryShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);
		long error = NO_ERROR;

		VerifyGEO(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class SubroutinesFragment : public SubroutinesBase
{
	virtual std::string Title()
	{
		return "Fragment Shader Subroutines Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *FRAGMENT_SUBROUTINE* as an interface params.\n";
	}

	virtual void inline VerifyFS(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE, GL_MAX_NAME_LENGTH, 2, error);

		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 1,
									error);

		std::map<std::string, GLuint> indicesTS;
		VerifyGetProgramResourceIndex(program, GL_FRAGMENT_SUBROUTINE, indicesTS, "x", error);
		std::map<std::string, GLuint> indicesTU;
		VerifyGetProgramResourceIndex(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, indicesTU, "a", error);

		VerifyGetProgramResourceName(program, GL_FRAGMENT_SUBROUTINE, indicesTS["x"], "x", error);
		VerifyGetProgramResourceName(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, indicesTU["a"], "a", error);

		VerifyGetProgramResourceLocation(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_FRAGMENT_SHADER, "a"), error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 2 };
		VerifyGetProgramResourceiv(program, GL_FRAGMENT_SUBROUTINE, indicesTS["x"], 1, propsS, 1, expectedS, error);

		GLenum propsU[] = { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION,
							GL_COMPATIBLE_SUBROUTINES };
		GLint expectedU[] = { 2, 1, 1, glGetSubroutineUniformLocation(program, GL_FRAGMENT_SHADER, "a"),
							  static_cast<GLint>(indicesTS["x"]) };
		VerifyGetProgramResourceiv(program, GL_FRAGMENT_SUBROUTINE_UNIFORM, indicesTU["a"], 5, propsU, 5, expectedU,
								   error);
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), TessControlShader().c_str(), TessEvalShader().c_str(),
									   GeometryShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);
		long error = NO_ERROR;

		VerifyFS(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class SoubroutinesCompute : public PIQBase
{
	virtual std::string Title()
	{
		return "Compute Shader Subroutines Test";
	}

	virtual std::string ShadersDesc()
	{
		return "compute shader with subroutines used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using *COMPUTE_SUBROUTINE* as an interface params.\n";
	}

	virtual std::string ComputeShader()
	{
		return "layout(local_size_x = 1, local_size_y = 1) in; \n"

			   "layout(std430) buffer Output {                 \n"
			   "  vec4 data;                                   \n"
			   "} g_out;                                       \n"
			   ""
			   "subroutine vec4 a_t();               \n"
			   "subroutine uniform a_t a;            \n"
			   "subroutine(a_t) vec4 ax() {          \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   "subroutine(a_t) vec4 ay() {          \n"
			   "   return vec4(0);                   \n"
			   "}                                    \n"
			   ""
			   "subroutine vec4 b_t();               \n"
			   "subroutine uniform b_t b[3];         \n"
			   "subroutine(b_t) vec4 bx() {          \n"
			   "   return vec4(1);                   \n"
			   "}                                    \n"
			   "subroutine(b_t) vec4 by() {          \n"
			   "   return vec4(0);                   \n"
			   "}                                    \n"
			   "subroutine(b_t) vec4 bz() {          \n"
			   "   return vec4(-1);                   \n"
			   "}                                    \n"
			   ""
			   "void main() {                                    \n"
			   "   g_out.data = a() + b[0]() + b[1]() + b[2]();  \n"
			   "}";
	}

	GLuint CreateComputeProgram(const std::string& cs)
	{
		const GLuint p = glCreateProgram();

		const char* const kGLSLVer = "#version 430 core\n";

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

	virtual void inline VerifyCompute(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE, GL_ACTIVE_RESOURCES, 5, error);
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE, GL_MAX_NAME_LENGTH, 3, error);

		GLint res;
		glGetProgramStageiv(program, GL_COMPUTE_SHADER, GL_ACTIVE_SUBROUTINE_UNIFORMS, &res);
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, GL_ACTIVE_RESOURCES, res, error);
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, GL_MAX_NAME_LENGTH, 5, error);
		VerifyGetProgramInterfaceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, 3,
									error);

		std::map<std::string, GLuint> indicesTS;
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE, indicesTS, "ax", error);
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE, indicesTS, "ay", error);
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE, indicesTS, "bx", error);
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE, indicesTS, "by", error);
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE, indicesTS, "bz", error);
		std::map<std::string, GLuint> indicesTU;
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU, "a", error);
		VerifyGetProgramResourceIndex(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU, "b[0]", error);

		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE, indicesTS["ax"], "ax", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE, indicesTS["ay"], "ay", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE, indicesTS["bx"], "bx", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE, indicesTS["by"], "by", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE, indicesTS["bz"], "bz", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU["b[0]"], "b[0]", error);

		VerifyGetProgramResourceLocation(program, GL_COMPUTE_SUBROUTINE_UNIFORM, "a",
										 glGetSubroutineUniformLocation(program, GL_COMPUTE_SHADER, "a"), error);
		VerifyGetProgramResourceLocation(program, GL_COMPUTE_SUBROUTINE_UNIFORM, "b",
										 glGetSubroutineUniformLocation(program, GL_COMPUTE_SHADER, "b"), error);

		GLenum propsS[]	= { GL_NAME_LENGTH };
		GLint  expectedS[] = { 3 };
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE, indicesTS["ax"], 1, propsS, 1, expectedS, error);
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE, indicesTS["ay"], 1, propsS, 1, expectedS, error);
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE, indicesTS["bx"], 1, propsS, 1, expectedS, error);
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE, indicesTS["by"], 1, propsS, 1, expectedS, error);
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE, indicesTS["bz"], 1, propsS, 1, expectedS, error);

		GLenum propsU[]	= { GL_NAME_LENGTH, GL_ARRAY_SIZE, GL_NUM_COMPATIBLE_SUBROUTINES, GL_LOCATION };
		GLint  expectedU[] = { 2, 1, 2, glGetSubroutineUniformLocation(program, GL_COMPUTE_SHADER, "a") };
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU["a"], 4, propsU, 4, expectedU,
								   error);
		GLint expectedU2[] = { 5, 3, 3, glGetSubroutineUniformLocation(program, GL_COMPUTE_SHADER, "b") };
		VerifyGetProgramResourceiv(program, GL_COMPUTE_SUBROUTINE_UNIFORM, indicesTU["b[0]"], 4, propsU, 4, expectedU2,
								   error);
	}

	virtual long Run()
	{
		GLuint program = CreateComputeProgram(ComputeShader());
		glLinkProgram(program);
		if (!CheckProgram(program))
		{
			glDeleteProgram(program);
			return ERROR;
		}
		glUseProgram(program);

		long error = NO_ERROR;

		VerifyCompute(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class InvalidValueTest : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Invalid Value Test";
	}

	virtual std::string PassCriteria()
	{
		return "GL_INVALID_VALUE error is generated after every function call.";
	}

	virtual std::string Purpose()
	{
		return "Verify that wrong use of functions generates GL_INVALID_VALUE as described in spec.";
	}

	virtual std::string Method()
	{
		return "Call functions with invalid values and check if GL_INVALID_VALUE was generated.";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		GLint   res = 0;
		GLsizei len = 0;
		GLchar  name[100] = { '\0' };
		GLenum  props[1]  = { GL_NAME_LENGTH };

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 1: <program> not a name of shader/program object"
			<< tcu::TestLog::EndMessage;
		glGetProgramInterfaceiv(1337u, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &res);
		ExpectError(GL_INVALID_VALUE, error);
		glGetProgramResourceIndex(31337u, GL_PROGRAM_INPUT, "pie");
		ExpectError(GL_INVALID_VALUE, error);
		glGetProgramResourceName(1337u, GL_PROGRAM_INPUT, 0, 1024, &len, name);
		ExpectError(GL_INVALID_VALUE, error);
		glGetProgramResourceiv(1337u, GL_PROGRAM_INPUT, 0, 1, props, 1024, &len, &res);
		ExpectError(GL_INVALID_VALUE, error);
		glGetProgramResourceLocation(1337u, GL_PROGRAM_INPUT, "pie");
		ExpectError(GL_INVALID_VALUE, error);
		glGetProgramResourceLocationIndex(1337u, GL_PROGRAM_OUTPUT, "pie");
		ExpectError(GL_INVALID_VALUE, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 1 finished" << tcu::TestLog::EndMessage;

		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Case 2: <index> is greater than the number of the active resources in GetProgramResourceName"
			<< tcu::TestLog::EndMessage;
		glGetProgramResourceName(program, GL_PROGRAM_INPUT, 3000, 1024, &len, name);
		ExpectError(GL_INVALID_VALUE, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 2 finished" << tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 3: <propCount> is zero in GetProgramResourceiv"
			<< tcu::TestLog::EndMessage;
		glGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 0, props, 1024, &len, &res);
		ExpectError(GL_INVALID_VALUE, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 3 finished" << tcu::TestLog::EndMessage;

		std::string str = "position";
		glGetProgramResourceName(program, GL_PROGRAM_INPUT, 0, -100, NULL, const_cast<char*>(str.c_str()));
		ExpectError(GL_INVALID_VALUE, error);
		GLenum prop = GL_NAME_LENGTH;
		glGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 1, &prop, -100, &len, &res);
		ExpectError(GL_INVALID_VALUE, error);

		glDeleteProgram(program);
		return error;
	}
};

class InvalidEnumTest : public AtomicCounterSimple
{
	virtual std::string Title()
	{
		return "Invalid Enum Test";
	}

	virtual std::string PassCriteria()
	{
		return "GL_INVALID_ENUM error is generated after every function call.";
	}

	virtual std::string Purpose()
	{
		return "Verify that wrong use of functions generates GL_INVALID_ENUM as described in spec.";
	}

	virtual std::string Method()
	{
		return "Call functions with invalid enums and check if GL_INVALID_ENUM was generated.";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		GLint   res = 0;
		GLsizei len = 0;
		GLchar  name[100] = { '\0' };
		GLenum  props[1]  = { GL_TEXTURE_1D };

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 1: <programInterface> is ATOMIC_COUNTER_BUFFER "
			<< "in GetProgramResourceIndex or GetProgramResourceName" << tcu::TestLog::EndMessage;
		glGetProgramResourceIndex(program, GL_ATOMIC_COUNTER_BUFFER, name);
		ExpectError(GL_INVALID_ENUM, error);
		glGetProgramResourceName(program, GL_ATOMIC_COUNTER_BUFFER, 0, 1024, &len, name);
		ExpectError(GL_INVALID_ENUM, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 1 finished" << tcu::TestLog::EndMessage;

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 2: <props> is not a property name supported by "
			<< "the command GetProgramResourceiv" << tcu::TestLog::EndMessage;
		glGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 1, props, 1024, &len, &res);
		ExpectError(GL_INVALID_ENUM, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 2 finished" << tcu::TestLog::EndMessage;

		glGetProgramResourceLocation(program, GL_ATOMIC_COUNTER_BUFFER, "position");
		ExpectError(GL_INVALID_ENUM, error);

		glDeleteProgram(program);
		return error;
	}
};

class InvalidOperationTest : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Invalid Operation Test";
	}

	virtual std::string PassCriteria()
	{
		return "GL_INVALID_OPERATION error is generated after every function call.";
	}

	virtual std::string Purpose()
	{
		return "Verify that wrong use of functions generates GL_INVALID_OPERATION as described in spec.";
	}

	virtual std::string Method()
	{
		return "Perform invalid operation and check if GL_INVALID_OPERATION was generated.";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		GLuint program  = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		GLuint program2 = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		const GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
		GLint		 res = 0;
		GLsizei		 len = 0;
		GLchar		 name[100] = { '\0' };
		GLenum		 props[1]  = { GL_OFFSET };

		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 1: <program> is the name of a shader object" << tcu::TestLog::EndMessage;
		glGetProgramInterfaceiv(sh, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &res);
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceIndex(sh, GL_PROGRAM_INPUT, "pie");
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceName(sh, GL_PROGRAM_INPUT, 0, 1024, &len, name);
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceiv(sh, GL_PROGRAM_INPUT, 0, 1, props, 1024, &len, &res);
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceLocation(sh, GL_PROGRAM_INPUT, "pie");
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceLocationIndex(sh, GL_PROGRAM_OUTPUT, "pie");
		ExpectError(GL_INVALID_OPERATION, error);
		glDeleteShader(sh);
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 1 finished\n"
			<< "Case 2: <pname> is not supported in GetProgramInterfacei" << tcu::TestLog::EndMessage;

		glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NUM_ACTIVE_VARIABLES, &res);
		ExpectError(GL_INVALID_OPERATION, error);
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 2 finished\n"
			<< "Case 3: <props> is not supported in GetProgramResourceiv" << tcu::TestLog::EndMessage;

		glGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 1, props, 1024, &len, &res);
		ExpectError(GL_INVALID_OPERATION, error);
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Case 3 finished\n"
			<< "Case 4: <program> has not been linked in GetProgramResourceLocation/GetProgramResourceLocationIndex"
			<< tcu::TestLog::EndMessage;

		glGetProgramResourceLocation(program2, GL_PROGRAM_INPUT, "pie");
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramResourceLocationIndex(program2, GL_PROGRAM_OUTPUT, "pie");
		ExpectError(GL_INVALID_OPERATION, error);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Case 4 finished\n" << tcu::TestLog::EndMessage;

		glDeleteProgram(program);
		glDeleteProgram(program2);
		return error;
	}
};

class ShaderStorageBlock : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Shader Storage Block Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different types of storage blocks used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_BUFFER_VARIABLE and GL_SHADER_STORAGE_BLOCK as an interface "
			   "params.\n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   ""
			   "struct U {                     \n"
			   "   bool a[3];                  \n"
			   "   mediump vec4 b;             \n"
			   "   mediump mat3 c;             \n"
			   "   mediump float d[2];         \n"
			   "};                             \n"
			   ""
			   "struct UU {                    \n"
			   "   U a;                        \n"
			   "   U b[2];                     \n"
			   "   uvec2 c;                    \n"
			   "};                             \n"
			   ""
			   "layout(binding=4) buffer TrickyBuffer {          \n"
			   "   UU a[3];                                      \n"
			   "   mediump mat4 b;                               \n"
			   "   uint c;                                       \n"
			   "} e[2];                                          \n"
			   ""
			   "layout(binding = 0) buffer SimpleBuffer {        \n"
			   "   mediump mat3x2 a;                             \n"
			   "   mediump mat4 b;                               \n"
			   "   mediump vec4 c;                               \n"
			   "};                                               \n"
			   ""
			   "layout(binding = 1) buffer NotSoSimpleBuffer {   \n"
			   "   ivec2 a[4];                                   \n"
			   "   mediump mat3 b[2];                            \n"
			   "   mediump mat2 c;                               \n"
			   "} d;                                             \n"
			   ""
			   "out mediump vec4 color;                          \n"
			   ""
			   "void main() {                                    \n"
			   "    mediump float tmp;                           \n"
			   "    mediump float tmp2;                          \n"
			   "    tmp = e[0].a[0].b[0].d[0] * float(e[1].c);   \n"
			   "    tmp2 = a[0][0] * b[0][0] * c.x;                                \n"
			   "    tmp2 = tmp2 + float(d.a[0].y) + d.b[0][0][0] + d.c[0][0];      \n"
			   "    color = vec4(0, 1, 0, 1) * tmp * tmp2;                         \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		GLint res;
		VerifyGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, 28, error);
		glGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &res);
		if (res < 7)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Error on: glGetProgramInterfaceiv, if: GL_BUFFER_VARIABLE, param: GL_ACTIVE_RESOURCES\n"
				<< "Expected value greater or equal to 7, got " << res << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, 4, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, 18, error);

		std::map<std::string, GLuint> indicesSSB;
		std::map<std::string, GLuint> indicesBV;
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "SimpleBuffer", error);
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "NotSoSimpleBuffer", error);
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "TrickyBuffer", error);
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "TrickyBuffer[1]", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "a", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "b", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "c", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "NotSoSimpleBuffer.a[0]", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "NotSoSimpleBuffer.c", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "NotSoSimpleBuffer.b[0]", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "TrickyBuffer.a[0].b[0].d", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "TrickyBuffer.b", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "TrickyBuffer.c", error);

		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["SimpleBuffer"], "SimpleBuffer",
									 error);
		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["NotSoSimpleBuffer"],
									 "NotSoSimpleBuffer", error);
		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["TrickyBuffer"], "TrickyBuffer[0]",
									 error);
		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["TrickyBuffer[1]"], "TrickyBuffer[1]",
									 error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["c"], "c", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["NotSoSimpleBuffer.a[0]"],
									 "NotSoSimpleBuffer.a[0]", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["NotSoSimpleBuffer.c"],
									 "NotSoSimpleBuffer.c", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["NotSoSimpleBuffer.b[0]"],
									 "NotSoSimpleBuffer.b[0]", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["TrickyBuffer.a[0].b[0].d"],
									 "TrickyBuffer.a[0].b[0].d[0]", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["TrickyBuffer.b"], "TrickyBuffer.b", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["TrickyBuffer.c"], "TrickyBuffer.c", error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_BUFFER_BINDING,
						   GL_NUM_ACTIVE_VARIABLES,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER };
		GLint expected[] = { 13, 0, 3, 0, 1, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["SimpleBuffer"], 9, props, 9, expected,
								   error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_BUFFER_BINDING,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER };
		GLint expected2[] = { 18, 1, 0, 1, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["NotSoSimpleBuffer"], 8, props2, 8,
								   expected2, error);
		GLint expected3[] = { 16, 4, 0, 1, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["TrickyBuffer"], 8, props2, 8,
								   expected3, error);
		GLint expected4[] = { 16, 5, 0, 1, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["TrickyBuffer[1]"], 8, props2, 8,
								   expected4, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_ARRAY_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_TOP_LEVEL_ARRAY_SIZE,
							GL_TOP_LEVEL_ARRAY_STRIDE };
		GLint expected5[] = {
			2, GL_FLOAT_MAT3x2, 1, static_cast<GLint>(indicesSSB["SimpleBuffer"]), 0, 0, 0, 1, 0, 0, 0, 0, 1, 0
		};
		VerifyGetProgramResourceiv(program, GL_BUFFER_VARIABLE, indicesBV["a"], 14, props3, 14, expected5, error);

		GLenum props4[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_MATRIX_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_TOP_LEVEL_ARRAY_SIZE };
		GLint expected6[] = {
			28, GL_FLOAT, 2, static_cast<GLint>(indicesSSB["TrickyBuffer"]), 0, 0, 0, 1, 0, 0, 0, 0, 3
		};
		VerifyGetProgramResourceiv(program, GL_BUFFER_VARIABLE, indicesBV["TrickyBuffer.a[0].b[0].d"], 13, props4, 13,
								   expected6, error);

		GLenum			 prop	= GL_ACTIVE_VARIABLES;
		const GLsizei	bufSize = 1000;
		GLsizei			 length;
		GLint			 param[bufSize];
		std::set<GLuint> exp;
		exp.insert(indicesBV["a"]);
		exp.insert(indicesBV["b"]);
		exp.insert(indicesBV["c"]);
		glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["SimpleBuffer"], 1, &prop, bufSize, &length,
							   param);
		for (int i = 0; i < length; ++i)
		{
			if (exp.find(param[i]) == exp.end())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Unexpected index found in active variables of SimpleBuffer: " << param[i]
					<< "\nCall: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: "
					   "GL_SHADER_STORAGE_BLOCK"
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
			else if (length != 3)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Call: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES "
												"interface: GL_SHADER_STORAGE_BLOCK\n"
					<< "Expected length: 3, actual length: " << length << tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}
		std::set<GLuint> exp2;
		exp2.insert(indicesBV["NotSoSimpleBuffer.a[0]"]);
		exp2.insert(indicesBV["NotSoSimpleBuffer.b[0]"]);
		exp2.insert(indicesBV["NotSoSimpleBuffer.c"]);
		glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["NotSoSimpleBuffer"], 1, &prop, bufSize,
							   &length, param);
		for (int i = 0; i < length; ++i)
		{
			if (exp2.find(param[i]) == exp2.end())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Unexpected index found in active variables of NotSoSimpleBuffer: " << param[i]
					<< "\nCall: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES interface: "
					   "GL_SHADER_STORAGE_BLOCK"
					<< tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
			else if (length != 3)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Call: glGetProgramResourceiv, property: GL_ACTIVE_VARIABLES "
												"interface: GL_SHADER_STORAGE_BLOCK\n"
					<< "Expected length: 3, actual length: " << length << tcu::TestLog::EndMessage;
				glDeleteProgram(program);
				return ERROR;
			}
		}

		glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, &res);
		if (res < 3)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Value of GL_MAX_NUM_ACTIVE_VARIABLES less than 3!\n"
				<< "Call: glGetProgramInterfaceiv, interface: GL_SHADER_STORAGE_BLOCK" << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glDeleteProgram(program);
		return error;
	}
};

class UniformBlockArray : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Uniform Block Array Test";
	}

	virtual std::string ShadersDesc()
	{
		return "verify BLOCK_INDEX property when an interface block is declared as an array of block instances";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_BLOCK_INDEX as an interface param.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                    \n"
			   "void main(void)                 \n"
			   "{                               \n"
			   "    gl_Position = vec4(1.0);    \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "uniform TestBlock {            \n"
			   "   mediump vec4 color;         \n"
			   "} blockInstance[4];            \n"
			   ""
			   "out mediump vec4 color;                                      \n"
			   "void main() {                                                \n"
			   "    color = blockInstance[2].color + blockInstance[3].color; \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		LinkProgram(program);

		long error = NO_ERROR;

		std::map<std::string, GLuint> indicesUB;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM_BLOCK, indicesUB, "TestBlock", error);

		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "TestBlock.color", error);

		GLenum props[]	= { GL_BLOCK_INDEX };
		GLint  expected[] = { static_cast<GLint>(indicesUB["TestBlock"]) };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indicesU["TestBlock.color"], 1, props, 1, expected, error);

		glDeleteProgram(program);
		return error;
	}
};

class TransformFeedbackBuiltin : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Transform Feedback Built-in Variables";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different types of out variables used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_TRANSFORM_FEEDBACK_VARYING as an interface param with"
			   " built-in variables (gl_NextBuffer, gl_SkipComponents) used.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "out ivec4 a;                         \n"
			   "out uvec2 c;                         \n"
			   "out uint d;                          \n"
			   "out int f;                           \n"
			   "out uint e;                          \n"
			   "out int g;                           \n"
			   ""
			   "void main(void)                      \n"
			   "{                                    \n"
			   "   vec4 pos;                         \n"
			   "   a = ivec4(1);                     \n"
			   "   c = uvec2(1u);                    \n"
			   "   d = 1u;                           \n"
			   "   f = 1;                            \n"
			   "   e = 1u;                           \n"
			   "   g = 1;                            \n"
			   "   gl_Position = position;           \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		const char* varyings[11] = { "a", "gl_NextBuffer",		"c", "gl_SkipComponents1", "d", "gl_SkipComponents2",
									 "f", "gl_SkipComponents3", "e", "gl_SkipComponents4", "g" };
		glTransformFeedbackVaryings(program, 11, varyings, GL_INTERLEAVED_ATTRIBS);
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, GL_ACTIVE_RESOURCES, 11, error);
		VerifyGetProgramInterfaceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, GL_MAX_NAME_LENGTH, 19, error);

		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, "gl_NextBuffer", GL_INVALID_INDEX, error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents1", GL_INVALID_INDEX,
									  error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents2", GL_INVALID_INDEX,
									  error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents3", GL_INVALID_INDEX,
									  error);
		VerifyGetProgramResourceIndex(program, GL_TRANSFORM_FEEDBACK_VARYING, "gl_SkipComponents4", GL_INVALID_INDEX,
									  error);

		std::map<std::string, GLuint> indices;
		for (int i = 0; i < 11; i++)
		{
			GLsizei length;
			GLchar  name[1024] = { '\0' };
			glGetProgramResourceName(program, GL_TRANSFORM_FEEDBACK_VARYING, i, 1024, &length, name);
			indices[name] = i;
		}

		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Indices of builtins:\n"
											<< indices["gl_NextBuffer"] << ", " << indices["gl_SkipComponents1"] << ", "
											<< indices["gl_SkipComponents2"] << ", " << indices["gl_SkipComponents3"]
											<< ", " << indices["gl_SkipComponents4"] << tcu::TestLog::EndMessage;

		GLenum props[]	= { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE };
		GLint  expected[] = { 14, GL_NONE, 0 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["gl_NextBuffer"], 3, props, 3,
								   expected, error);
		GLint expected2[] = { 19, GL_NONE, 1 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["gl_SkipComponents1"], 3, props, 3,
								   expected2, error);
		GLint expected3[] = { 19, GL_NONE, 2 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["gl_SkipComponents2"], 3, props, 3,
								   expected3, error);
		GLint expected4[] = { 19, GL_NONE, 3 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["gl_SkipComponents3"], 3, props, 3,
								   expected4, error);
		GLint expected5[] = { 19, GL_NONE, 4 };
		VerifyGetProgramResourceiv(program, GL_TRANSFORM_FEEDBACK_VARYING, indices["gl_SkipComponents4"], 3, props, 3,
								   expected5, error);

		glDeleteProgram(program);
		return error;
	}
};

class NullLength : public SimpleShaders
{

	virtual std::string Title()
	{
		return "NULL Length Test";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that GetProgramResourceName with null length doesn't return length (doesn't "
			   "crash).\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = position;          \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1);  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		GLchar name[1024] = { '\0' };
		GLuint index	  = glGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "color");
		GLenum prop		  = GL_ARRAY_SIZE;
		GLint  res;
		glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, 0, 1024, NULL, name);
		glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, index, 1, &prop, 1, NULL, &res);

		std::string expected = "color";
		if (name != expected)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Expected name: " << expected
												<< ", got: " << name << tcu::TestLog::EndMessage;
			glDeleteProgram(program);
			return ERROR;
		}
		else if (res != 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected array_size: 1, got: " << res << tcu::TestLog::EndMessage;
			glDeleteProgram(program);
			return ERROR;
		}

		glDeleteProgram(program);
		return NO_ERROR;
	}
};

class ArraysOfArrays : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Arrays Of Arrays Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with multi dimensional uniform array used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that feature works correctly with arrays_of_arrays extension.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "uniform vec4 a[3][4][5];             \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = position;          \n"
			   "    for (int i = 0; i < 5; ++i)      \n"
			   "        gl_Position += a[2][1][i];   \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1);  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NAME_LENGTH, 11, error);

		std::map<std::string, GLuint> indices;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indices, "a[2][1]", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, "a[2][1][0]", indices["a[2][1]"], error);

		VerifyGetProgramResourceName(program, GL_UNIFORM, indices["a[2][1]"], "a[2][1][0]", error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_OFFSET,
						   GL_BLOCK_INDEX,
						   GL_ARRAY_STRIDE,
						   GL_MATRIX_STRIDE,
						   GL_IS_ROW_MAJOR,
						   GL_ATOMIC_COUNTER_BUFFER_INDEX,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION };
		GLint expected[] = {
			11, GL_FLOAT_VEC4, 5, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(program, "a[2][1]")
		};
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indices["a[2][1]"], 16, props, 16, expected, error);

		glDeleteProgram(program);
		return error;
	}
};

class TopLevelArray : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Top Level Array Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with multi dimensional array used inside storage block";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that feature works correctly when querying for GL_TOP_LEVEL_ARRAY_SIZE\n"
			   " and GL_TOP_LEVEL_ARRAY_STRIDE extension.\n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "buffer Block {                       \n"
			   "   vec4 a[5][4][3];                  \n"
			   "};                                   \n"
			   "out vec4 color;                             \n"
			   "void main() {                               \n"
			   "    color = vec4(0, 1, 0, 1) + a[0][0][0];  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, 11, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, 6, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, 1, error);

		std::map<std::string, GLuint> indicesSSB;
		std::map<std::string, GLuint> indicesBV;
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "a[0][0]", error);
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "Block", error);

		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["a[0][0]"], "a[0][0][0]", error);
		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["Block"], "Block", error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_IS_ROW_MAJOR,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_TOP_LEVEL_ARRAY_SIZE };
		GLint expected5[] = { 11, GL_FLOAT_VEC4, 3, static_cast<GLint>(indicesSSB["Block"]), 0, 0, 1, 0, 0, 0, 0, 5 };
		VerifyGetProgramResourceiv(program, GL_BUFFER_VARIABLE, indicesBV["a[0][0]"], 12, props3, 12, expected5, error);

		GLenum  prop = GL_TOP_LEVEL_ARRAY_STRIDE;
		GLsizei len;
		GLint   res;
		glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, indicesBV["a[0][0]"], 1, &prop, 1024, &len, &res);
		if (res <= 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Call: glGetProgramResourceiv, interface: GL_BUFFER_VARIABLE, param: GL_TOP_LEVEL_ARRAY_STRIDE\n"
				<< "Expected value greater than 0, got: " << res << tcu::TestLog::EndMessage;
			glDeleteProgram(program);
			return ERROR;
		}

		glDeleteProgram(program);
		return error;
	}
};

class SeparateProgramsVertex : public SimpleShaders
{
public:
	virtual std::string Title()
	{
		return "Separate Program Vertex Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "vertex shader as separate shader object";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that feature works correctly when using separate_shader_objects "
			   "functionality.\n";
	}

	virtual GLuint CreateShaderProgram(GLenum type, GLsizei count, const GLchar** strings)
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

	virtual long Run()
	{
		long error = NO_ERROR;

		const char* srcVS = "#version 430 core                          \n"
							"layout(location = 0) in vec4 in_vertex;    \n"
							""
							"out Color {                                \n"
							"  float r, g, b;                           \n"
							"  vec4 iLikePie;                           \n"
							"} vs_color;                                \n"
							"out gl_PerVertex {                         \n"
							"  vec4 gl_Position;                        \n"
							"};                                         \n"
							""
							"uniform float u;                           \n"
							"uniform vec4 v;                            \n"
							""
							"void main() {                              \n"
							"  gl_Position = in_vertex;                 \n"
							"  vs_color.r = u;                          \n"
							"  vs_color.g = 0.0;                        \n"
							"  vs_color.b = 0.0;                        \n"
							"  vs_color.iLikePie = v;                   \n"
							"}";

		const GLuint vs = CreateShaderProgram(GL_VERTEX_SHADER, 1, &srcVS);

		VerifyGetProgramInterfaceiv(vs, GL_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(vs, GL_UNIFORM, GL_ACTIVE_RESOURCES, 2, error);
		VerifyGetProgramInterfaceiv(vs, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 10, error);
		VerifyGetProgramInterfaceiv(vs, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(vs, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 15, error);
		VerifyGetProgramInterfaceiv(vs, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 5, error);

		std::map<std::string, GLuint> indicesU;
		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		VerifyGetProgramResourceIndex(vs, GL_UNIFORM, indicesU, "u", error);
		VerifyGetProgramResourceIndex(vs, GL_UNIFORM, indicesU, "v", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_INPUT, indicesI, "in_vertex", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_OUTPUT, indicesO, "Color.r", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_OUTPUT, indicesO, "Color.g", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_OUTPUT, indicesO, "Color.b", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_OUTPUT, indicesO, "Color.iLikePie", error);
		VerifyGetProgramResourceIndex(vs, GL_PROGRAM_OUTPUT, indicesO, "gl_Position", error);

		VerifyGetProgramResourceName(vs, GL_UNIFORM, indicesU["u"], "u", error);
		VerifyGetProgramResourceName(vs, GL_UNIFORM, indicesU["v"], "v", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_INPUT, indicesI["in_vertex"], "in_vertex", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_OUTPUT, indicesO["Color.r"], "Color.r", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_OUTPUT, indicesO["Color.g"], "Color.g", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_OUTPUT, indicesO["Color.b"], "Color.b", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_OUTPUT, indicesO["Color.iLikePie"], "Color.iLikePie", error);
		VerifyGetProgramResourceName(vs, GL_PROGRAM_OUTPUT, indicesO["gl_Position"], "gl_Position", error);

		VerifyGetProgramResourceLocation(vs, GL_UNIFORM, "u", glGetUniformLocation(vs, "u"), error);
		VerifyGetProgramResourceLocation(vs, GL_UNIFORM, "v", glGetUniformLocation(vs, "v"), error);
		VerifyGetProgramResourceLocation(vs, GL_PROGRAM_INPUT, "in_vertex", 0, error);

		VerifyGetProgramResourceLocationIndex(vs, GL_PROGRAM_OUTPUT, "Color.r", -1, error);
		VerifyGetProgramResourceLocationIndex(vs, GL_PROGRAM_OUTPUT, "Color.g", -1, error);
		VerifyGetProgramResourceLocationIndex(vs, GL_PROGRAM_OUTPUT, "Color.b", -1, error);
		VerifyGetProgramResourceLocationIndex(vs, GL_PROGRAM_OUTPUT, "Color.iLikePie", -1, error);
		VerifyGetProgramResourceLocationIndex(vs, GL_PROGRAM_OUTPUT, "gl_Position", -1, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_OFFSET,
						   GL_BLOCK_INDEX,
						   GL_ARRAY_STRIDE,
						   GL_MATRIX_STRIDE,
						   GL_IS_ROW_MAJOR,
						   GL_ATOMIC_COUNTER_BUFFER_INDEX,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION };
		GLint expected[] = {
			2, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 1, glGetUniformLocation(vs, "v")
		};
		VerifyGetProgramResourceiv(vs, GL_UNIFORM, indicesU["v"], 16, props, 16, expected, error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION,
							GL_IS_PER_PATCH };
		GLint expected2[] = { 10, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(vs, GL_PROGRAM_INPUT, indicesI["in_vertex"], 11, props2, 11, expected2, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected3[] = { 15, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 0, 1, 0, -1 };
		VerifyGetProgramResourceiv(vs, GL_PROGRAM_OUTPUT, indicesO["Color.iLikePie"], 11, props3, 11, expected3, error);

		glDeleteProgram(vs);
		return error;
	}
};

class SeparateProgramsTessControl : public SeparateProgramsVertex
{

	virtual std::string Title()
	{
		return "Separate Program Tess Control Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "tess control shader as separate shader object";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that feature works correctly when using separate_shader_objects "
			   "functionality.\n";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		const char* srcTCS = "#version 430                                                  \n"
							 "layout(vertices = 3) out;                                     \n"
							 "layout(location = 0) patch out vec4 data;                     \n"
							 "out gl_PerVertex {                                            \n"
							 "   vec4 gl_Position;                                          \n"
							 "   float gl_PointSize;                                        \n"
							 "   float gl_ClipDistance[];                                   \n"
							 "} gl_out[];                                                   \n"
							 ""
							 "in Color {                                 \n"
							 "  float r, g, b;                           \n"
							 "  vec4 iLikePie;                           \n"
							 "} vs_color[];                              \n"
							 ""
							 "void main() {                                                                   \n"
							 "   data = vec4(1);                                                              \n"
							 "   gl_out[gl_InvocationID].gl_Position =                                        \n"
							 "           vec4(vs_color[0].r, vs_color[0].g, vs_color[0].b, vs_color[0].iLikePie.x + "
							 "float(gl_InvocationID));       \n"
							 "   gl_TessLevelInner[0] = 1.0;                                                  \n"
							 "   gl_TessLevelInner[1] = 1.0;                                                  \n"
							 "   gl_TessLevelOuter[0] = 1.0;                                                  \n"
							 "   gl_TessLevelOuter[1] = 1.0;                                                  \n"
							 "   gl_TessLevelOuter[2] = 1.0;                                                  \n"
							 "}";

		const GLuint tcs = CreateShaderProgram(GL_TESS_CONTROL_SHADER, 1, &srcTCS);

		// gl_InvocationID should be included
		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 16, error);
		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 5, error);

		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_OUTPUT, indicesO, "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_OUTPUT, indicesO, "data", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "Color.r", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "Color.g", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "Color.b", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "Color.iLikePie", error);

		VerifyGetProgramResourceName(tcs, GL_PROGRAM_OUTPUT, indicesO["gl_PerVertex.gl_Position"],
									 "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_OUTPUT, indicesO["data"], "data", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["Color.r"], "Color.r", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["Color.g"], "Color.g", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["Color.b"], "Color.b", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["Color.iLikePie"], "Color.iLikePie", error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH };
		GLint expected2[] = { 15, GL_FLOAT_VEC4, 1, 0, 0, 0, 1, 0, 0, 0 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_INPUT, indicesI["Color.iLikePie"], 10, props2, 10, expected2, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH,
							GL_LOCATION,
							GL_LOCATION_INDEX };
		GLint expected3[] = { 5, GL_FLOAT_VEC4, 1, 0, 0, 0, 1, 0, 0, 1, 0, -1 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_OUTPUT, indicesO["data"], 12, props3, 12, expected3, error);

		glDeleteProgram(tcs);
		return error;
	}
};

class SeparateProgramsTessEval : public SeparateProgramsVertex
{

	virtual std::string Title()
	{
		return "Separate Program Tess Eval Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "tess eval shader as separate shader object";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		const char* srcTCS = "#version 430                                                  \n"
							 "layout(quads, equal_spacing, ccw) in;                         \n"
							 "out gl_PerVertex {                                            \n"
							 "   vec4 gl_Position;                                          \n"
							 "   float gl_PointSize;                                        \n"
							 "   float gl_ClipDistance[];                                   \n"
							 "};                                                            \n"
							 ""
							 "in gl_PerVertex {                   \n"
							 "   vec4 gl_Position;                \n"
							 "   float gl_PointSize;              \n"
							 "   float gl_ClipDistance[];         \n"
							 "} gl_in[];                          \n"
							 ""
							 "void main() {                                \n"
							 "   vec4 p0 = gl_in[0].gl_Position;           \n"
							 "   vec4 p1 = gl_in[1].gl_Position;           \n"
							 "   vec4 p2 = gl_in[2].gl_Position;           \n"
							 "   gl_Position = vec4(p0.x, p1.y, p2.z, p0.x);                 \n"
							 "}";

		const GLuint tcs = CreateShaderProgram(GL_TESS_EVALUATION_SHADER, 1, &srcTCS);

		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_OUTPUT, indicesO, "gl_Position", error);

		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["gl_PerVertex.gl_Position"],
									 "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_OUTPUT, indicesO["gl_Position"], "gl_Position", error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH };
		GLint expected2[] = { 25, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_INPUT, indicesI["gl_PerVertex.gl_Position"], 10, props2, 10,
								   expected2, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected3[] = { 12, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 1, 0, 0, -1 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_OUTPUT, indicesO["gl_Position"], 11, props3, 11, expected3, error);

		glDeleteProgram(tcs);
		return error;
	}
};

class SeparateProgramsGeometry : public SeparateProgramsVertex
{

	virtual std::string Title()
	{
		return "Separate Program Geometry Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "geometry shader as separate shader object";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		const char* srcTCS = "#version 430                                                  \n"
							 "layout(triangles) in;                                         \n"
							 "layout(triangle_strip, max_vertices = 4) out;                 \n"
							 ""
							 "out gl_PerVertex {                                            \n"
							 "   vec4 gl_Position;                                          \n"
							 "   float gl_PointSize;                                        \n"
							 "   float gl_ClipDistance[];                                   \n"
							 "};                                                            \n"
							 ""
							 "in gl_PerVertex {                                             \n"
							 "   vec4 gl_Position;                                          \n"
							 "   float gl_PointSize;                                        \n"
							 "   float gl_ClipDistance[];                                   \n"
							 "} gl_in[];                                                    \n"
							 ""
							 "void main() {                              \n"
							 "   gl_Position = vec4(-1, 1, 0, 1);        \n"
							 "   EmitVertex();                           \n"
							 "   gl_Position = vec4(-1, -1, 0, 1);       \n"
							 "   EmitVertex();                           \n"
							 "   gl_Position = vec4(1, 1, 0, 1);         \n"
							 "   EmitVertex();                           \n"
							 "   gl_Position = gl_in[0].gl_Position;     \n"
							 "   EmitVertex();                           \n"
							 "   EndPrimitive();                         \n"
							 "}";

		const GLuint tcs = CreateShaderProgram(GL_GEOMETRY_SHADER, 1, &srcTCS);

		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_OUTPUT, indicesO, "gl_Position", error);

		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["gl_PerVertex.gl_Position"],
									 "gl_PerVertex.gl_Position", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_OUTPUT, indicesO["gl_Position"], "gl_Position", error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH };
		GLint expected2[] = { 25, GL_FLOAT_VEC4, 1, 0, 0, 1, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_INPUT, indicesI["gl_PerVertex.gl_Position"], 10, props2, 10,
								   expected2, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected3[] = { 12, GL_FLOAT_VEC4, 1, 0, 0, 1, 0, 0, 0, 0, -1 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_OUTPUT, indicesO["gl_Position"], 11, props3, 11, expected3, error);

		glDeleteProgram(tcs);
		return error;
	}
};

class SeparateProgramsFragment : public SeparateProgramsVertex
{

	virtual std::string Title()
	{
		return "Separate Program Fragment Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment shader as separate shader object";
	}

	virtual long Run()
	{
		long error = NO_ERROR;

		const char* srcTCS = "#version 430                                     \n"
							 "out vec4 fs_color;                               \n"
							 ""
							 "layout(location = 1) uniform vec4 x;             \n"
							 ""
							 "layout(binding = 0) buffer SimpleBuffer {        \n"
							 "   vec4 a;                                       \n"
							 "};                                               \n"
							 ""
							 "in vec4 vs_color;                                \n"
							 "void main() {                                    \n"
							 "   fs_color = vs_color + x + a;                  \n"
							 "}";

		const GLuint tcs = CreateShaderProgram(GL_FRAGMENT_SHADER, 1, &srcTCS);

		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 9, error);
		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 9, error);
		VerifyGetProgramInterfaceiv(tcs, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(tcs, GL_UNIFORM, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(tcs, GL_UNIFORM, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(tcs, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, 2, error);
		VerifyGetProgramInterfaceiv(tcs, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(tcs, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(tcs, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, 13, error);
		VerifyGetProgramInterfaceiv(tcs, GL_SHADER_STORAGE_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, 1, error);

		std::map<std::string, GLuint> indicesSSB;
		std::map<std::string, GLuint> indicesBV;
		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_INPUT, indicesI, "vs_color", error);
		VerifyGetProgramResourceIndex(tcs, GL_PROGRAM_OUTPUT, indicesO, "fs_color", error);
		VerifyGetProgramResourceIndex(tcs, GL_UNIFORM, indicesU, "x", error);
		VerifyGetProgramResourceIndex(tcs, GL_SHADER_STORAGE_BLOCK, indicesSSB, "SimpleBuffer", error);
		VerifyGetProgramResourceIndex(tcs, GL_BUFFER_VARIABLE, indicesBV, "a", error);

		VerifyGetProgramResourceName(tcs, GL_SHADER_STORAGE_BLOCK, indicesSSB["SimpleBuffer"], "SimpleBuffer", error);
		VerifyGetProgramResourceName(tcs, GL_BUFFER_VARIABLE, indicesBV["a"], "a", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_INPUT, indicesI["vs_color"], "vs_color", error);
		VerifyGetProgramResourceName(tcs, GL_PROGRAM_OUTPUT, indicesO["fs_color"], "fs_color", error);
		VerifyGetProgramResourceName(tcs, GL_UNIFORM, indicesU["x"], "x", error);

		VerifyGetProgramResourceLocation(tcs, GL_UNIFORM, "x", 1, error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH };
		GLint expected2[] = { 9, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_INPUT, indicesI["vs_color"], 10, props2, 10, expected2, error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected3[] = { 9, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(tcs, GL_PROGRAM_OUTPUT, indicesO["fs_color"], 11, props3, 11, expected3, error);

		GLenum props5[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_OFFSET,
							GL_BLOCK_INDEX,
							GL_ARRAY_STRIDE,
							GL_MATRIX_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_ATOMIC_COUNTER_BUFFER_INDEX,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION };
		GLint expected5[] = { 2, GL_FLOAT_VEC4, 1, -1, -1, -1, -1, 0, -1, 0, 1, 0, 1 };
		VerifyGetProgramResourceiv(tcs, GL_UNIFORM, indicesU["x"], 13, props5, 13, expected5, error);

		GLenum props6[] = { GL_NAME_LENGTH,
							GL_BUFFER_BINDING,
							GL_NUM_ACTIVE_VARIABLES,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_ACTIVE_VARIABLES };
		GLint expected6[] = { 13, 0, 1, 0, 1, 0, static_cast<GLint>(indicesBV["a"]) };
		VerifyGetProgramResourceiv(tcs, GL_SHADER_STORAGE_BLOCK, indicesSSB["SimpleBuffer"], 7, props6, 7, expected6,
								   error);

		GLenum props7[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_ARRAY_STRIDE,
							GL_IS_ROW_MAJOR,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_TOP_LEVEL_ARRAY_SIZE,
							GL_TOP_LEVEL_ARRAY_STRIDE };
		GLint expected7[] = {
			2, GL_FLOAT_VEC4, 1, static_cast<GLint>(indicesSSB["SimpleBuffer"]), 0, 0, 0, 1, 0, 1, 0
		};
		VerifyGetProgramResourceiv(tcs, GL_BUFFER_VARIABLE, indicesBV["a"], 11, props7, 11, expected7, error);

		glDeleteProgram(tcs);
		return error;
	}
};

class UniformBlockAdvanced : public SimpleShaders
{
	virtual std::string Title()
	{
		return "Uniform Block Advanced Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment and vertex shaders with different types of uniform blocks used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify calls using GL_UNIFORM_BLOCK as an interface param and\n"
			   "verify results of querying offset, strides and row order.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "layout(row_major) uniform SimpleBlock {   \n"
			   "   mat4 a;                                \n"
			   "   vec4 b[10];                            \n"
			   "};                                        \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    float tmp;                       \n"
			   "    tmp = a[0][0] + b[0].x;          \n"
			   "    gl_Position = position * tmp;    \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		std::map<std::string, GLuint> indicesU;
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "a", error);
		VerifyGetProgramResourceIndex(program, GL_UNIFORM, indicesU, "b", error);

		GLenum props[]	= { GL_IS_ROW_MAJOR };
		GLint  expected[] = { 1 };
		VerifyGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 1, props, 1, expected, error);

		GLenum  prop = GL_MATRIX_STRIDE;
		GLsizei len;
		GLint   res;
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 1, &prop, 1024, &len, &res);
		if (res < 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "ERROR: glGetProgramResourceiv, interface GL_UNIFORM, prop GL_MATRIX_STRIDE\n"
				<< "Expected value greater than 0, got " << res << tcu::TestLog::EndMessage;
		}
		prop = GL_OFFSET;
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["a"], 1, &prop, 1024, &len, &res);
		if (res < 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: glGetProgramResourceiv, interface GL_UNIFORM, prop GL_OFFSET\n"
				<< "Expected value not less than 0, got " << res << tcu::TestLog::EndMessage;
		}
		prop = GL_ARRAY_STRIDE;
		glGetProgramResourceiv(program, GL_UNIFORM, indicesU["b"], 1, &prop, 1024, &len, &res);
		if (res < 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "ERROR: glGetProgramResourceiv, interface GL_UNIFORM, prop GL_ARRAY_STRIDE\n"
				<< "Expected value greater than 0, got " << res << tcu::TestLog::EndMessage;
		}

		glDeleteProgram(program);
		return error;
	}
};

class ArrayNames : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Array Names Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment shader and a vertex shader with array of vec4 uniform used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that GetProgramResourceLocation match "
			   "name strings correctly.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "uniform vec4 a[2];           \n"
			   ""
			   "void main(void)                            \n"
			   "{                                          \n"
			   "    gl_Position = position + a[0] + a[1];  \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a", glGetUniformLocation(program, "a"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[0]", glGetUniformLocation(program, "a"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[1]", glGetUniformLocation(program, "a[1]"), error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[2]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[0 + 0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[0+0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[ 0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[0 ]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[\n0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[\t0]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[01]", -1, error);
		VerifyGetProgramResourceLocation(program, GL_UNIFORM, "a[00]", -1, error);

		glDeleteProgram(program);
		return error;
	}
};

class BuffLength : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Buff Length Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fallthrough fragment shader and vertex with uniform of vec4 type used";
	}

	virtual std::string PurposeExt()
	{
		return "\n\n Purpose is to verify that bufsize of GetProgramResourceName and "
			   "GetProgramResourceiv is respected.\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   ""
			   "uniform vec4 someLongName;           \n"
			   ""
			   "void main(void)                            \n"
			   "{                                          \n"
			   "    gl_Position = position + someLongName; \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		LinkProgram(program);

		long error = NO_ERROR;

		GLuint  index = glGetProgramResourceIndex(program, GL_UNIFORM, "someLongName");
		GLsizei length;
		GLchar  buff[3] = { 'a', 'b', 'c' };
		glGetProgramResourceName(program, GL_UNIFORM, index, 0, NULL, NULL);
		glGetProgramResourceName(program, GL_UNIFORM, index, 0, NULL, buff);
		if (buff[0] != 'a' || buff[1] != 'b' || buff[2] != 'c')
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: buff has changed" << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glGetProgramResourceName(program, GL_UNIFORM, index, 2, &length, buff);
		if (buff[0] != 's' || buff[1] != '\0' || buff[2] != 'c')
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: buff different then expected" << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		if (length != 1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: incorrect length, expected 1, got "
												<< length << tcu::TestLog::EndMessage;
			error = ERROR;
		}

		GLint  params[3] = { 1, 2, 3 };
		GLenum props[]   = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_OFFSET,
						   GL_BLOCK_INDEX,
						   GL_ARRAY_STRIDE,
						   GL_MATRIX_STRIDE,
						   GL_IS_ROW_MAJOR,
						   GL_ATOMIC_COUNTER_BUFFER_INDEX,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION };
		glGetProgramResourceiv(program, GL_UNIFORM, index, 13, props, 0, NULL, NULL);
		glGetProgramResourceiv(program, GL_UNIFORM, index, 13, props, 0, NULL, params);
		if (params[0] != 1 || params[1] != 2 || params[2] != 3)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: params has changed" << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glGetProgramResourceiv(program, GL_UNIFORM, index, 13, props, 2, &length, params);
		if (params[0] != 13 || params[1] != GL_FLOAT_VEC4 || params[2] != 3)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ERROR: params has incorrect values" << tcu::TestLog::EndMessage;
			error = ERROR;
		}
		if (length != 2)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ERROR: incorrect length, expected 2, got "
												<< length << tcu::TestLog::EndMessage;
			error = ERROR;
		}

		glDeleteProgram(program);
		return error;
	}
};

class NoLocations : public SimpleShaders
{

	virtual std::string Title()
	{
		return "No Locations Test";
	}

	virtual std::string ShadersDesc()
	{
		return "fragment and vertex shaders with no locations set";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                         \n"
			   "in vec4 a;                           \n"
			   "in vec4 b;                           \n"
			   "in vec4 c[1];                        \n"
			   "in vec4 d;                           \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = a + b + c[0] + d;  \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 430                   \n"
			   "out vec4 a;                    \n"
			   "out vec4 b;                    \n"
			   "out vec4 c;                    \n"
			   "out vec4 d[1];                 \n"
			   "void main() {                  \n"
			   "    a = vec4(0, 1, 0, 1);      \n"
			   "    b = vec4(0, 1, 0, 1);      \n"
			   "    c = vec4(0, 1, 0, 1);      \n"
			   "    d[0] = vec4(0, 1, 0, 1);   \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glLinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 4, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 5, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 4, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 5, error);

		std::map<std::string, GLuint> indicesI;
		std::map<std::string, GLuint> indicesO;
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indicesI, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indicesI, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indicesI, "c", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, indicesI, "d", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indicesO, "a", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indicesO, "b", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indicesO, "c", error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, indicesO, "d[0]", error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indicesI["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indicesI["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indicesI["c"], "c[0]", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, indicesI["d"], "d", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indicesO["a"], "a", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indicesO["b"], "b", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indicesO["c"], "c", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, indicesO["d[0]"], "d[0]", error);

		std::map<std::string, GLint> locationsI;
		std::map<std::string, GLint> locationsO;
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, locationsI, "a", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, locationsI, "b", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, locationsI, "c", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, locationsI, "d", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, locationsO, "a", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, locationsO, "b", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, locationsO, "c", error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, locationsO, "d[0]", error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER };
		GLint expected[] = { 2, GL_FLOAT_VEC4, 1, 0, 0, 1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indicesI["a"], 6, props, 6, expected, error);
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indicesI["b"], 6, props, 6, expected, error);
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indicesI["d"], 6, props, 6, expected, error);
		GLint expected2[] = { 5, GL_FLOAT_VEC4, 1, 0, 0, 1 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, indicesI["c"], 6, props, 6, expected2, error);
		GLint expected3[] = { 2, GL_FLOAT_VEC4, 1, 0, 1, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indicesO["a"], 6, props, 6, expected3, error);
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indicesO["b"], 6, props, 6, expected3, error);
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indicesO["c"], 6, props, 6, expected3, error);
		GLint expected4[] = { 5, GL_FLOAT_VEC4, 1, 0, 1, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, indicesO["d[0]"], 6, props, 6, expected4, error);

		glDeleteProgram(program);
		return error;
	}
};

class ComputeShaderTest : public PIQBase
{
	virtual std::string Title()
	{
		return "Compute Shader Test";
	}

	virtual std::string ShadersDesc()
	{
		return "compute shader";
	}

	virtual std::string ComputeShader()
	{
		return "layout(local_size_x = 1, local_size_y = 1) in; \n"
			   "layout(std430) buffer Output {                 \n"
			   "   vec4 data[];                                \n"
			   "} g_out;                                       \n"
			   ""
			   "void main() {                                   \n"
			   "   g_out.data[0] = vec4(1.0, 2.0, 3.0, 4.0);    \n"
			   "   g_out.data[100] = vec4(1.0, 2.0, 3.0, 4.0);  \n"
			   "}";
	}

	GLuint CreateComputeProgram(const std::string& cs)
	{
		const GLuint p = glCreateProgram();

		const char* const kGLSLVer = "#version 430  core \n";

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

	virtual void inline VerifyCompute(GLuint program, long& error)
	{
		VerifyGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, 15, error);
		VerifyGetProgramInterfaceiv(program, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, 7, error);
		VerifyGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, 1, error);

		std::map<std::string, GLuint> indicesSSB;
		std::map<std::string, GLuint> indicesBV;
		VerifyGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, indicesSSB, "Output", error);
		VerifyGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, indicesBV, "Output.data", error);

		VerifyGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["Output"], "Output", error);
		VerifyGetProgramResourceName(program, GL_BUFFER_VARIABLE, indicesBV["Outputa.data"], "Output.data[0]", error);

		GLenum props3[] = { GL_NAME_LENGTH,
							GL_BUFFER_BINDING,
							GL_NUM_ACTIVE_VARIABLES,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_ACTIVE_VARIABLES };
		GLint expected3[] = { 7, 0, 1, 1, 0, 0, static_cast<GLint>(indicesBV["Outputa.data"]) };
		VerifyGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, indicesSSB["Output"], 7, props3, 7, expected3,
								   error);

		GLenum props4[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_BLOCK_INDEX,
							GL_IS_ROW_MAJOR,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_TOP_LEVEL_ARRAY_SIZE };
		GLint expected4[] = { 15, GL_FLOAT_VEC4, 0, static_cast<GLint>(indicesSSB["Output"]), 0, 1, 0, 0, 1 };
		VerifyGetProgramResourceiv(program, GL_BUFFER_VARIABLE, indicesBV["Outputa.data"], 9, props4, 9, expected4,
								   error);
	}

	virtual long Run()
	{
		GLuint program = CreateComputeProgram(ComputeShader());
		glLinkProgram(program);
		if (!CheckProgram(program))
		{
			glDeleteProgram(program);
			return ERROR;
		}
		glUseProgram(program);

		long error = NO_ERROR;

		VerifyCompute(program, error);

		glDeleteProgram(program);
		return error;
	}
};

class QueryNotUsed : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Query Not Used Test";
	}

	virtual std::string PassCriteria()
	{
		return "Data from queries matches the not used program.";
	}

	virtual std::string Purpose()
	{
		return "Verify that program parameter works correctly and proper program is queried when different program is "
			   "used.";
	}

	virtual std::string Method()
	{
		return "Create 2 programs, use one of them and query the other, verify the results.";
	}

	virtual std::string VertexShader2()
	{
		return "#version 430                         \n"
			   "in vec4 p;                           \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = p;                 \n"
			   "}";
	}

	virtual std::string FragmentShader2()
	{
		return "#version 430                   \n"
			   "out vec4 c;                    \n"
			   "void main() {                  \n"
			   "    c = vec4(0, 1, 0, 1);      \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		GLuint program2 = CreateProgram(VertexShader2().c_str(), FragmentShader2().c_str(), false);
		glBindAttribLocation(program, 0, "p");
		glBindFragDataLocation(program, 0, "c");
		LinkProgram(program2);
		glUseProgram(program2);

		long error = NO_ERROR;

		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, 9, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, 1, error);
		VerifyGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, 6, error);

		VerifyGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "color", 0, error);
		VerifyGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "position", 0, error);

		VerifyGetProgramResourceName(program, GL_PROGRAM_OUTPUT, 0, "color", error);
		VerifyGetProgramResourceName(program, GL_PROGRAM_INPUT, 0, "position", error);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position", 0, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		VerifyGetProgramResourceLocationIndex(program, GL_PROGRAM_OUTPUT, "color", 0, error);

		GLenum props[] = { GL_NAME_LENGTH,
						   GL_TYPE,
						   GL_ARRAY_SIZE,
						   GL_REFERENCED_BY_COMPUTE_SHADER,
						   GL_REFERENCED_BY_FRAGMENT_SHADER,
						   GL_REFERENCED_BY_GEOMETRY_SHADER,
						   GL_REFERENCED_BY_TESS_CONTROL_SHADER,
						   GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
						   GL_REFERENCED_BY_VERTEX_SHADER,
						   GL_LOCATION,
						   GL_IS_PER_PATCH };
		GLint expected[] = { 9, GL_FLOAT_VEC4, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_INPUT, 0, 11, props, 11, expected, error);

		GLenum props2[] = { GL_NAME_LENGTH,
							GL_TYPE,
							GL_ARRAY_SIZE,
							GL_REFERENCED_BY_COMPUTE_SHADER,
							GL_REFERENCED_BY_FRAGMENT_SHADER,
							GL_REFERENCED_BY_GEOMETRY_SHADER,
							GL_REFERENCED_BY_TESS_CONTROL_SHADER,
							GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
							GL_REFERENCED_BY_VERTEX_SHADER,
							GL_LOCATION,
							GL_IS_PER_PATCH,
							GL_LOCATION_INDEX };
		GLint expected2[] = { 6, GL_FLOAT_VEC4, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
		VerifyGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, 0, 12, props2, 12, expected2, error);

		glDeleteProgram(program);
		glDeleteProgram(program2);
		return error;
	}
};

class RelinkFailure : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Relink Failure Test";
	}

	virtual std::string PassCriteria()
	{
		return "INVALID_OPERATION is generated when asking for locations after failed link.";
	}

	virtual std::string Purpose()
	{
		return "Verify that queries behave correctly after failed relink of a program.";
	}

	virtual std::string Method()
	{
		return "Create a program, use it, relink with failure and then verify that INVALID_OPERATION is returned when "
			   "asking for locations.";
	}

	virtual std::string VertexShader()
	{
		return "#version 430                                  \n"
			   "in vec4 position;                             \n"
			   "in vec3 pos;                                  \n"
			   "void main(void)                               \n"
			   "{                                             \n"
			   "    gl_Position = position + vec4(pos, 1);    \n"
			   "}";
	}

	virtual long Run()
	{
		GLuint program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str(), false);
		glBindAttribLocation(program, 0, "position");
		glBindAttribLocation(program, 1, "pos");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "pos", 1, error);
		glUseProgram(program);

		tcu::Vec4 v[4] = { tcu::Vec4(-1, 1, 0, 1), tcu::Vec4(-1, -1, 0, 1), tcu::Vec4(1, 1, 0, 1),
						   tcu::Vec4(1, -1, 0, 1) };
		GLuint vao, vbuf;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4), 0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisableVertexAttribArray(0);
		glDeleteVertexArrays(1, &vao);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &vbuf);

		glBindAttribLocation(program, 0, "pos");
		glBindAttribLocation(program, 0, "position");
		const char* varyings[2] = { "q", "z" };
		glTransformFeedbackVaryings(program, 2, varyings, GL_INTERLEAVED_ATTRIBS);
		LinkProgram(program);

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position", -1, error);
		ExpectError(GL_INVALID_OPERATION, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "pos", -1, error);
		ExpectError(GL_INVALID_OPERATION, error);
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", -1, error);
		ExpectError(GL_INVALID_OPERATION, error);

		glDeleteProgram(program);
		return error;
	}
};

class LinkFailure : public SimpleShaders
{

	virtual std::string Title()
	{
		return "Link Failure Test";
	}

	virtual std::string PassCriteria()
	{
		return "INVALID_OPERATION is generated when asking for locations after failed link.";
	}

	virtual std::string Purpose()
	{
		return "Verify that queries behave correctly after failed relink of a program with changed sources.";
	}

	virtual std::string Method()
	{
		return "Create a program, use it, relink with failure using different sources and then \n"
			   "verify that INVALID_OPERATION is returned when asking for locations.";
	}

	virtual const char* VertexShader_prop()
	{
		return "#version 430                         \n"
			   "in vec4 posit;                       \n"
			   "in vec4 p;                           \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = p + posit;         \n"
			   "}";
	}

	virtual const char* FragmentShader_prop()
	{
		return "#version 430                   \n"
			   "out vec4 color;                \n"
			   "void main() {                  \n"
			   "    color = vec4(0, 1, 0, 1);  \n"
			   "}";
	}

	virtual const char* VertexShader_fail()
	{
		return "#version 430                         \n"
			   "in vec4 position;                    \n"
			   "void main(void)                      \n"
			   "{                                    \n"
			   "    gl_Position = position;          \n"
			   "}";
	}

	virtual long Run()
	{
		const GLuint program = glCreateProgram();
		const char*  src_vs  = VertexShader_prop();
		const char*  src_fs  = FragmentShader_prop();
		const char*  src_vsh = VertexShader_fail();

		GLuint sh1 = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(program, sh1);
		glDeleteShader(sh1);
		glShaderSource(sh1, 1, &src_vs, NULL);
		glCompileShader(sh1);

		GLuint sh2 = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(program, sh2);
		glDeleteShader(sh2);
		glShaderSource(sh2, 1, &src_fs, NULL);
		glCompileShader(sh2);

		glBindAttribLocation(program, 0, "p");
		glBindAttribLocation(program, 1, "posit");
		glBindFragDataLocation(program, 0, "color");
		LinkProgram(program);

		long error = NO_ERROR;

		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "posit", 1, error);
		glUseProgram(program);

		tcu::Vec4 v[4] = { tcu::Vec4(-1, 1, 0, 1), tcu::Vec4(-1, -1, 0, 1), tcu::Vec4(1, 1, 0, 1),
						   tcu::Vec4(1, -1, 0, 1) };
		GLuint vao, vbuf;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(tcu::Vec4), 0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisableVertexAttribArray(0);
		glDeleteVertexArrays(1, &vao);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &vbuf);

		glDetachShader(program, sh1);
		GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(program, vsh);
		glDeleteShader(vsh);
		glShaderSource(vsh, 1, &src_vsh, NULL);
		glCompileShader(vsh);
		const char* varyings[2] = { "q", "z" };
		glTransformFeedbackVaryings(program, 2, varyings, GL_INTERLEAVED_ATTRIBS);
		LinkProgram(program);

		GLint res;
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position", -1, error);
		ExpectError(GL_INVALID_OPERATION, error);
		glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &res);
		if (res != 0 && res != 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Error, expected 0 or 1 active resources, got: " << res
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, &res);
		if (res != 0 && res != 9)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Error, expected 1 or 9 GL_MAX_NAME_LENGTH, got: " << res
				<< tcu::TestLog::EndMessage;
			error = ERROR;
		}
		VerifyGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "color", -1, error);
		ExpectError(GL_INVALID_OPERATION, error);

		glDeleteProgram(program);
		return error;
	}
};

} // anonymous namespace

ProgramInterfaceQueryTests::ProgramInterfaceQueryTests(deqp::Context& context)
	: TestCaseGroup(context, "program_interface_query", "")
{
}

ProgramInterfaceQueryTests::~ProgramInterfaceQueryTests(void)
{
}

void ProgramInterfaceQueryTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "empty-shaders", TestSubcase::Create<NoShaders>));
	addChild(new TestSubcase(m_context, "simple-shaders", TestSubcase::Create<SimpleShaders>));
	addChild(new TestSubcase(m_context, "input-types", TestSubcase::Create<InputTypes>));
	addChild(new TestSubcase(m_context, "input-built-in", TestSubcase::Create<InputBuiltIn>));
	addChild(new TestSubcase(m_context, "input-layout", TestSubcase::Create<InputLayout>));
	addChild(new TestSubcase(m_context, "output-types", TestSubcase::Create<OutputTypes>));
	addChild(new TestSubcase(m_context, "output-location-index", TestSubcase::Create<OutputLocationIndex>));
	addChild(new TestSubcase(m_context, "output-built-in", TestSubcase::Create<OutputBuiltIn>));
	addChild(new TestSubcase(m_context, "output-layout", TestSubcase::Create<OutputLayout>));
	addChild(new TestSubcase(m_context, "output-layout-index", TestSubcase::Create<OutputLayoutIndex>));
	addChild(new TestSubcase(m_context, "uniform-simple", TestSubcase::Create<UniformSimple>));
	addChild(new TestSubcase(m_context, "uniform-types", TestSubcase::Create<UniformTypes>));
	addChild(new TestSubcase(m_context, "uniform-block-types", TestSubcase::Create<UniformBlockTypes>));
	addChild(new TestSubcase(m_context, "transform-feedback-types", TestSubcase::Create<TransformFeedbackTypes>));
	addChild(new TestSubcase(m_context, "atomic-counters", TestSubcase::Create<AtomicCounterSimple>));
	addChild(new TestSubcase(m_context, "subroutines-vertex", TestSubcase::Create<SubroutinesVertex>));
	addChild(new TestSubcase(m_context, "subroutines-tess-control", TestSubcase::Create<SubroutinesTessControl>));
	addChild(new TestSubcase(m_context, "subroutines-tess-eval", TestSubcase::Create<SubroutinesTessEval>));
	addChild(new TestSubcase(m_context, "subroutines-geometry", TestSubcase::Create<SubroutinesGeometry>));
	addChild(new TestSubcase(m_context, "subroutines-fragment", TestSubcase::Create<SubroutinesFragment>));
	addChild(new TestSubcase(m_context, "subroutines-compute", TestSubcase::Create<SoubroutinesCompute>));
	addChild(new TestSubcase(m_context, "ssb-types", TestSubcase::Create<ShaderStorageBlock>));
	addChild(new TestSubcase(m_context, "transform-feedback-built-in", TestSubcase::Create<TransformFeedbackBuiltin>));
	addChild(new TestSubcase(m_context, "null-length", TestSubcase::Create<NullLength>));
	addChild(new TestSubcase(m_context, "arrays-of-arrays", TestSubcase::Create<ArraysOfArrays>));
	addChild(new TestSubcase(m_context, "top-level-array", TestSubcase::Create<TopLevelArray>));
	addChild(new TestSubcase(m_context, "separate-programs-vertex", TestSubcase::Create<SeparateProgramsVertex>));
	addChild(
		new TestSubcase(m_context, "separate-programs-tess-control", TestSubcase::Create<SeparateProgramsTessControl>));
	addChild(new TestSubcase(m_context, "separate-programs-tess-eval", TestSubcase::Create<SeparateProgramsTessEval>));
	addChild(new TestSubcase(m_context, "separate-programs-geometry", TestSubcase::Create<SeparateProgramsGeometry>));
	addChild(new TestSubcase(m_context, "separate-programs-fragment", TestSubcase::Create<SeparateProgramsFragment>));
	addChild(new TestSubcase(m_context, "uniform-block", TestSubcase::Create<UniformBlockAdvanced>));
	addChild(new TestSubcase(m_context, "uniform-block-array", TestSubcase::Create<UniformBlockArray>));
	addChild(new TestSubcase(m_context, "array-names", TestSubcase::Create<ArrayNames>));
	addChild(new TestSubcase(m_context, "buff-length", TestSubcase::Create<BuffLength>));
	addChild(new TestSubcase(m_context, "no-locations", TestSubcase::Create<NoLocations>));
	addChild(new TestSubcase(m_context, "query-not-used", TestSubcase::Create<QueryNotUsed>));
	addChild(new TestSubcase(m_context, "relink-failure", TestSubcase::Create<RelinkFailure>));
	addChild(new TestSubcase(m_context, "link-failure", TestSubcase::Create<LinkFailure>));
	addChild(new TestSubcase(m_context, "compute-shader", TestSubcase::Create<ComputeShaderTest>));
	addChild(new TestSubcase(m_context, "invalid-value", TestSubcase::Create<InvalidValueTest>));
	addChild(new TestSubcase(m_context, "invalid-operation", TestSubcase::Create<InvalidOperationTest>));
	addChild(new TestSubcase(m_context, "invalid-enum", TestSubcase::Create<InvalidEnumTest>));
}

} // gl4cts namespace
