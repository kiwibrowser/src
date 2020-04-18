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

#include "gl4cComputeShaderTests.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include "tcuMatrixUtil.hpp"
#include "tcuRenderTarget.hpp"
#include <cmath>
#include <cstdarg>
#include <sstream>

namespace gl4cts
{

using namespace glw;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::UVec4;
using tcu::UVec3;
using tcu::Mat4;

namespace
{

typedef Vec3  vec2;
typedef Vec3  vec3;
typedef Vec4  vec4;
typedef UVec3 uvec3;
typedef UVec4 uvec4;
typedef Mat4  mat4;

const char* const kGLSLVer = "#version 430 core\n";

class ComputeShaderBase : public deqp::SubcaseBase
{

public:
	virtual ~ComputeShaderBase()
	{
	}

	ComputeShaderBase()
		: renderTarget(m_context.getRenderContext().getRenderTarget()), pixelFormat(renderTarget.getPixelFormat())
	{
		float epsilon_zero = 1.f / (1 << 13);
		if (pixelFormat.redBits != 0 && pixelFormat.greenBits != 0 && pixelFormat.blueBits != 0 &&
			pixelFormat.alphaBits != 0)
		{
			g_color_eps = vec4(1.f / ((float)(1 << pixelFormat.redBits) - 1.0f),
							   1.f / ((float)(1 << pixelFormat.greenBits) - 1.0f),
							   1.f / ((float)(1 << pixelFormat.blueBits) - 1.0f),
							   1.f / ((float)(1 << pixelFormat.alphaBits) - 1.0f)) +
						  vec4(epsilon_zero);
		}
		else if (pixelFormat.redBits != 0 && pixelFormat.greenBits != 0 && pixelFormat.blueBits != 0)
		{
			g_color_eps = vec4(1.f / ((float)(1 << pixelFormat.redBits) - 1.0f),
							   1.f / ((float)(1 << pixelFormat.greenBits) - 1.0f),
							   1.f / ((float)(1 << pixelFormat.blueBits) - 1.0f), 1.f) +
						  vec4(epsilon_zero);
		}
		else
		{
			g_color_eps = vec4(epsilon_zero);
		}
	}

	const tcu::RenderTarget& renderTarget;
	const tcu::PixelFormat&  pixelFormat;
	vec4					 g_color_eps;

	uvec3 IndexTo3DCoord(GLuint idx, GLuint max_x, GLuint max_y)
	{
		const GLuint x = idx % max_x;
		idx /= max_x;
		const GLuint y = idx % max_y;
		idx /= max_y;
		const GLuint z = idx;
		return uvec3(x, y, z);
	}

	bool CheckProgram(GLuint program, bool* compile_error = NULL)
	{
		GLint compile_status = GL_TRUE;
		GLint status		 = GL_TRUE;
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

					GLint length;
					glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &length);
					if (length > 0)
					{
						std::vector<GLchar> source(length);
						glGetShaderSource(shaders[i], length, NULL, &source[0]);
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << &source[0] << tcu::TestLog::EndMessage;
					}

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

	GLuint CreateComputeProgram(const std::string& cs)
	{
		const GLuint p = glCreateProgram();

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

	GLuint CreateProgram(const std::string& vs, const std::string& fs)
	{
		const GLuint p = glCreateProgram();

		if (!vs.empty())
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, vs.c_str() };
			glShaderSource(sh, 2, src, NULL);
			glCompileShader(sh);
		}
		if (!fs.empty())
		{
			const GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, fs.c_str() };
			glShaderSource(sh, 2, src, NULL);
			glCompileShader(sh);
		}

		return p;
	}

	GLuint BuildShaderProgram(GLenum type, const std::string& source)
	{
		const char* const src[2] = { kGLSLVer, source.c_str() };
		return glCreateShaderProgramv(type, 2, src);
	}

	GLfloat distance(GLfloat p0, GLfloat p1)
	{
		return de::abs(p0 - p1);
	}

	inline bool ColorEqual(const vec4& c0, const vec4& c1, const vec4& epsilon)
	{
		if (distance(c0.x(), c1.x()) > epsilon.x())
			return false;
		if (distance(c0.y(), c1.y()) > epsilon.y())
			return false;
		if (distance(c0.z(), c1.z()) > epsilon.z())
			return false;
		if (distance(c0.w(), c1.w()) > epsilon.w())
			return false;
		return true;
	}

	inline bool ColorEqual(const vec3& c0, const vec3& c1, const vec4& epsilon)
	{
		if (distance(c0.x(), c1.x()) > epsilon.x())
			return false;
		if (distance(c0.y(), c1.y()) > epsilon.y())
			return false;
		if (distance(c0.z(), c1.z()) > epsilon.z())
			return false;
		return true;
	}

	bool ValidateReadBuffer(int x, int y, int w, int h, const vec4& expected)
	{
		std::vector<vec4> display(w * h);
		glReadPixels(x, y, w, h, GL_RGBA, GL_FLOAT, &display[0]);

		for (int j = 0; j < h; ++j)
		{
			for (int i = 0; i < w; ++i)
			{
				if (!ColorEqual(display[j * w + i], expected, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Color at (" << (x + i) << ", " << (y + j) << ") is ["
						<< display[j * w + i].x() << ", " << display[j * w + i].y() << ", " << display[j * w + i].z()
						<< ", " << display[j * w + i].w() << "] should be [" << expected.x() << ", " << expected.y()
						<< ", " << expected.z() << ", " << expected.w() << "]." << tcu::TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}

	bool ValidateReadBufferCenteredQuad(int width, int height, const vec3& expected)
	{
		bool			  result = true;
		std::vector<vec3> fb(width * height);
		glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, &fb[0]);

		int startx = int(((float)width * 0.1f) + 1);
		int starty = int(((float)height * 0.1f) + 1);
		int endx   = int((float)width - 2 * (((float)width * 0.1f) + 1) - 1);
		int endy   = int((float)height - 2 * (((float)height * 0.1f) + 1) - 1);

		for (int y = starty; y < endy; ++y)
		{
			for (int x = startx; x < endx; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], expected, g_color_eps))
				{
					return false;
				}
			}
		}

		if (!ColorEqual(fb[2 * width + 2], vec3(0), g_color_eps))
		{
			result = false;
		}
		if (!ColorEqual(fb[2 * width + (width - 3)], vec3(0), g_color_eps))
		{
			result = false;
		}
		if (!ColorEqual(fb[(height - 3) * width + (width - 3)], vec3(0), g_color_eps))
		{
			result = false;
		}
		if (!ColorEqual(fb[(height - 3) * width + 2], vec3(0), g_color_eps))
		{
			result = false;
		}

		return result;
	}

	int getWindowWidth()
	{
		return renderTarget.getWidth();
	}

	int getWindowHeight()
	{
		return renderTarget.getHeight();
	}

	bool ValidateWindow4Quads(const vec3& lb, const vec3& rb, const vec3& rt, const vec3& lt)
	{
		int				  width  = 100;
		int				  height = 100;
		std::vector<vec3> fb(width * height);
		glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, &fb[0]);

		bool status = true;

		// left-bottom quad
		for (int y = 10; y < height / 2 - 10; ++y)
		{
			for (int x = 10; x < width / 2 - 10; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], lb, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "First bad color (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		// right-bottom quad
		for (int y = 10; y < height / 2 - 10; ++y)
		{
			for (int x = width / 2 + 10; x < width - 10; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], rb, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		// right-top quad
		for (int y = height / 2 + 10; y < height - 10; ++y)
		{
			for (int x = width / 2 + 10; x < width - 10; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], rt, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		// left-top quad
		for (int y = height / 2 + 10; y < height - 10; ++y)
		{
			for (int x = 10; x < width / 2 - 10; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], lt, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		// middle horizontal line should be black
		for (int y = height / 2 - 2; y < height / 2 + 2; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], vec3(0), g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		// middle vertical line should be black
		for (int y = 0; y < height; ++y)
		{
			for (int x = width / 2 - 2; x < width / 2 + 2; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], vec3(0), g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx].x() << " "
						<< fb[idx].y() << " " << fb[idx].z() << tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}

		return status;
	}

	bool IsEqual(vec4 a, vec4 b)
	{
		return (a.x() == b.x()) && (a.y() == b.y()) && (a.z() == b.z()) && (a.w() == b.w());
	}

	bool IsEqual(uvec4 a, uvec4 b)
	{
		return (a.x() == b.x()) && (a.y() == b.y()) && (a.z() == b.z()) && (a.w() == b.w());
	}
};

class SimpleCompute : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Simplest possible Compute Shader";
	}

	virtual std::string Purpose()
	{
		return "1. Verify that CS can be created, compiled and linked.\n"
			   "2. Verify that local work size can be queried with GetProgramiv command.\n"
			   "3. Verify that CS can be dispatched with DispatchCompute command.\n"
			   "4. Verify that CS can write to SSBO.";
	}

	virtual std::string Method()
	{
		return "Create and dispatch CS. Verify SSBO content.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_buffer;

	virtual long Setup()
	{

		const char* const glsl_cs =
			NL "layout(local_size_x = 1, local_size_y = 1) in;" NL "layout(std430) buffer Output {" NL "  vec4 data;" NL
			   "} g_out;" NL "void main() {" NL "  g_out.data = vec4(1.0, 2.0, 3.0, 4.0);" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		GLint v[3];
		glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, v);
		if (v[0] != 1 || v[1] != 1 || v[2] != 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Got " << v[0] << ", " << v[1] << ", " << v[2]
				<< ", expected: 1, 1, 1 in GL_COMPUTE_WORK_GROUP_SIZE check" << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);

		vec4* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data	   = static_cast<vec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4), GL_MAP_READ_BIT));
		long error = NO_ERROR;
		if (!IsEqual(data[0], vec4(1.0f, 2.0f, 3.0f, 4.0f)))
		{
			error = ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return error;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

class BasicOneWorkGroup : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "One work group with various local sizes";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that declared local work size has correct effect." NL
				  "2. Verify that the number of shader invocations is correct." NL
				  "3. Verify that the built-in variables: gl_WorkGroupSize, gl_WorkGroupID, gl_GlobalInvocationID," NL
				  "    gl_LocalInvocationID and gl_LocalInvocationIndex has correct values." NL
				  "4. Verify that DispatchCompute and DispatchComputeIndirect commands work as expected.";
	}

	virtual std::string Method()
	{
		return NL "1. Create several CS with various local sizes." NL
				  "2. Dispatch each CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Verify SSBO content.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	std::string GenSource(int x, int y, int z, GLuint binding)
	{
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << x << ", local_size_y = " << y << ", local_size_z = " << z
		   << ") in;" NL "layout(std430, binding = " << binding
		   << ") buffer Output {" NL "  uvec4 local_id[];" NL "} g_out;" NL "void main() {" NL
			  "  if (gl_WorkGroupSize == uvec3("
		   << x << ", " << y << ", " << z
		   << ") && gl_WorkGroupID == uvec3(0) &&" NL "      gl_GlobalInvocationID == gl_LocalInvocationID) {" NL
			  "    g_out.local_id[gl_LocalInvocationIndex] = uvec4(gl_LocalInvocationID, 0);" NL "  } else {" NL
			  "    g_out.local_id[gl_LocalInvocationIndex] = uvec4(0xffff);" NL "  }" NL "}";
		return ss.str();
	}

	bool RunIteration(int local_size_x, int local_size_y, int local_size_z, GLuint binding, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size_x, local_size_y, local_size_z, binding));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		GLint v[3];
		glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, v);
		if (v[0] != local_size_x || v[1] != local_size_y || v[2] != local_size_z)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_COMPUTE_LOCAL_WORK_SIZE is (" << v[0] << " " << v[1] << " " << v[2]
				<< ") should be (" << local_size_x << " " << local_size_y << " " << local_size_z << ")"
				<< tcu::TestLog::EndMessage;
			return false;
		}

		const int kSize = local_size_x * local_size_y * local_size_z;

		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uvec4) * kSize, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			const GLuint num_groups[3] = { 1, 1, 1 };
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), num_groups, GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(1, 1, 1);
		}

		uvec4* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		data =
			static_cast<uvec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * sizeof(uvec4), GL_MAP_READ_BIT));

		bool ret = true;

		for (int z = 0; z < local_size_z; ++z)
		{
			for (int y = 0; y < local_size_y; ++y)
			{
				for (int x = 0; x < local_size_x; ++x)
				{
					const int index = z * local_size_x * local_size_y + y * local_size_x + x;
					if (!IsEqual(data[index], uvec4(x, y, z, 0)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Invalid data at offset " << index << tcu::TestLog::EndMessage;
						ret = false;
					}
				}
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return ret;
	}

	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!RunIteration(16, 1, 1, 0, true))
			return ERROR;
		if (!RunIteration(8, 8, 1, 1, false))
			return ERROR;
		if (!RunIteration(4, 4, 4, 2, true))
			return ERROR;
		if (!RunIteration(1, 2, 3, 3, false))
			return ERROR;
		if (!RunIteration(1024, 1, 1, 3, true))
			return ERROR;
		if (!RunIteration(16, 8, 8, 3, false))
			return ERROR;
		if (!RunIteration(32, 1, 32, 7, true))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicResourceUBO : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Compute Shader resources - UBOs";
	}

	virtual std::string Purpose()
	{
		return "Verify that CS is able to read data from UBOs and write it to SSBO.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which uses array of UBOs." NL
				  "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Read data from each UBO and write it to SSBO." NL "4. Verify SSBO content." NL
				  "5. Repeat for different buffer and CS work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_uniform_buffer[12];
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z() << ") in;" NL "const uvec3 kGlobalSize = uvec3(" << global_size.x()
		   << ", " << global_size.y() << ", " << global_size.z()
		   << ");" NL "layout(std140) uniform InputBuffer {" NL "  vec4 data["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "} g_in_buffer[12];" NL "layout(std430) buffer OutputBuffer {" NL "  vec4 data0["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data1["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data2["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data3["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data4["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data5["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data6["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data7["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data8["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data9["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data10["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data11["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "} g_out_buffer;" NL "void main() {" NL "  const uint global_index = gl_GlobalInvocationID.x +" NL
			  "                            gl_GlobalInvocationID.y * kGlobalSize.x +" NL
			  "                            gl_GlobalInvocationID.z * kGlobalSize.x * kGlobalSize.y;" NL
			  "  g_out_buffer.data0[global_index] = g_in_buffer[0].data[global_index];" NL
			  "  g_out_buffer.data1[global_index] = g_in_buffer[1].data[global_index];" NL
			  "  g_out_buffer.data2[global_index] = g_in_buffer[2].data[global_index];" NL
			  "  g_out_buffer.data3[global_index] = g_in_buffer[3].data[global_index];" NL
			  "  g_out_buffer.data4[global_index] = g_in_buffer[4].data[global_index];" NL
			  "  g_out_buffer.data5[global_index] = g_in_buffer[5].data[global_index];" NL
			  "  g_out_buffer.data6[global_index] = g_in_buffer[6].data[global_index];" NL
			  "  g_out_buffer.data7[global_index] = g_in_buffer[7].data[global_index];" NL
			  "  g_out_buffer.data8[global_index] = g_in_buffer[8].data[global_index];" NL
			  "  g_out_buffer.data9[global_index] = g_in_buffer[9].data[global_index];" NL
			  "  g_out_buffer.data10[global_index] = g_in_buffer[10].data[global_index];" NL
			  "  g_out_buffer.data11[global_index] = g_in_buffer[11].data[global_index];" NL "}";
		return ss.str();
	}

	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		for (GLuint i = 0; i < 12; ++i)
		{
			char name[32];
			sprintf(name, "InputBuffer[%u]", i);
			const GLuint index = glGetUniformBlockIndex(m_program, name);
			glUniformBlockBinding(m_program, index, i);
			GLint p = 0;
			glGetActiveUniformBlockiv(m_program, index, GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER, &p);
			if (p == GL_FALSE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER should be TRUE."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}

		const GLuint kBufferSize =
			local_size.x() * num_groups.x() * local_size.y() * num_groups.y() * local_size.z() * num_groups.z();

		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * kBufferSize * 12, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		if (m_uniform_buffer[0] == 0)
			glGenBuffers(12, m_uniform_buffer);
		for (GLuint i = 0; i < 12; ++i)
		{
			std::vector<vec4> data(kBufferSize);
			for (GLuint j = 0; j < kBufferSize; ++j)
			{
				data[j] = vec4(static_cast<float>(i * kBufferSize + j));
			}
			glBindBufferBase(GL_UNIFORM_BUFFER, i, m_uniform_buffer[i]);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(vec4) * kBufferSize, &data[0], GL_DYNAMIC_DRAW);
		}
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}

		std::vector<vec4> data(kBufferSize * 12);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * kBufferSize * 12, &data[0]);

		for (GLuint z = 0; z < local_size.z() * num_groups.z(); ++z)
		{
			for (GLuint y = 0; y < local_size.y() * num_groups.y(); ++y)
			{
				for (GLuint x = 0; x < local_size.x() * num_groups.x(); ++x)
				{
					const GLuint index = z * local_size.x() * num_groups.x() * local_size.y() * num_groups.y() +
										 y * local_size.x() * num_groups.x() + x;
					for (int i = 0; i < 1; ++i)
					{
						if (!IsEqual(data[index * 12 + i], vec4(static_cast<float>(index * 12 + i))))
						{
							m_context.getTestContext().getLog() << tcu::TestLog::Message << "Incorrect data at offset "
																<< index * 12 + i << "." << tcu::TestLog::EndMessage;
							return false;
						}
					}
				}
			}
		}
		return true;
	}

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		memset(m_uniform_buffer, 0, sizeof(m_uniform_buffer));
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!RunIteration(uvec3(64, 1, 1), uvec3(8, 1, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(2, 2, 2), uvec3(2, 2, 2), true))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 2), uvec3(2, 4, 1), false))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(12, m_uniform_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicResourceTexture : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return NL "Compute Shader resources - Textures";
	}

	virtual std::string Purpose()
	{
		return NL "Verify that texture access works correctly in CS.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which uses all sampler types (sampler1D, sampler2D, sampler3D, sampler2DRect," NL
				  "    sampler1DArray, sampler2DArray, samplerBuffer, sampler2DMS, sampler2DMSArray)." NL
				  "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Sample each texture and write sampled value to SSBO." NL "4. Verify SSBO content." NL
				  "5. Repeat for different texture and CS work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_texture[9];
	GLuint m_texture_buffer;
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z() << ") in;" NL "const uvec3 kGlobalSize = uvec3(" << global_size.x()
		   << ", " << global_size.y() << ", " << global_size.z()
		   << ");" NL "uniform sampler1D g_sampler0;" NL "uniform sampler2D g_sampler1;" NL
			  "uniform sampler3D g_sampler2;" NL "uniform sampler2DRect g_sampler3;" NL
			  "uniform sampler1DArray g_sampler4;" NL "uniform sampler2DArray g_sampler5;" NL
			  "uniform samplerBuffer g_sampler6;" NL "uniform sampler2DMS g_sampler7;" NL
			  "uniform sampler2DMSArray g_sampler8;" NL "layout(std430) buffer OutputBuffer {" NL "  vec4 data0["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data1["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data2["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data3["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data4["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data5["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data6["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data7["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  vec4 data8["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "} g_out_buffer;" NL "void main() {" NL "  const uint global_index = gl_GlobalInvocationID.x +" NL
			  "                            gl_GlobalInvocationID.y * kGlobalSize.x +" NL
			  "                            gl_GlobalInvocationID.z * kGlobalSize.x * kGlobalSize.y;" NL
			  "  g_out_buffer.data0[global_index] = texelFetch(g_sampler0, int(gl_GlobalInvocationID), 0);" NL
			  "  g_out_buffer.data1[global_index] = texture(g_sampler1, vec2(gl_GlobalInvocationID) / "
			  "vec2(kGlobalSize));" NL "  g_out_buffer.data2[global_index] = textureProj(g_sampler2, "
			  "vec4(vec3(gl_GlobalInvocationID) / vec3(kGlobalSize), 1.0));" NL
			  "  g_out_buffer.data3[global_index] = textureProjOffset(g_sampler3, vec3(vec2(gl_GlobalInvocationID), "
			  "1.0), ivec2(0));" NL "  g_out_buffer.data4[global_index] = textureLodOffset(g_sampler4, "
			  "vec2(gl_GlobalInvocationID.x / kGlobalSize.x, gl_GlobalInvocationID.y), 0.0, "
			  "0);" NL "  g_out_buffer.data5[global_index] = texelFetchOffset(g_sampler5, "
			  "ivec3(gl_GlobalInvocationID), 0, ivec2(0));" NL
			  "  g_out_buffer.data6[global_index] = texelFetch(g_sampler6, int(global_index));" NL
			  "  g_out_buffer.data7[global_index] = texelFetch(g_sampler7, ivec2(gl_GlobalInvocationID), 1);" NL
			  "  g_out_buffer.data8[global_index] = texelFetch(g_sampler8, ivec3(gl_GlobalInvocationID), 2);" NL "}";
		return ss.str();
	}

	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		glUseProgram(m_program);
		for (int i = 0; i < 9; ++i)
		{
			char name[32];
			sprintf(name, "g_sampler%d", i);
			glUniform1i(glGetUniformLocation(m_program, name), i);
		}
		glUseProgram(0);

		const GLuint kBufferSize =
			local_size.x() * num_groups.x() * local_size.y() * num_groups.y() * local_size.z() * num_groups.z();
		const GLint kWidth  = static_cast<GLint>(local_size.x() * num_groups.x());
		const GLint kHeight = static_cast<GLint>(local_size.y() * num_groups.y());
		const GLint kDepth  = static_cast<GLint>(local_size.z() * num_groups.z());

		std::vector<vec4> buffer_data(kBufferSize * 9);
		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * kBufferSize * 9, &buffer_data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		std::vector<vec4> texture_data(kBufferSize, vec4(123.0f));
		if (m_texture[0] == 0)
			glGenTextures(9, m_texture);
		if (m_texture_buffer == 0)
			glGenBuffers(1, &m_texture_buffer);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_1D, m_texture[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, kWidth, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kWidth, kHeight, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, m_texture[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, kWidth, kHeight, kDepth, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_RECTANGLE, m_texture[3]);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA32F, kWidth, kHeight, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_1D_ARRAY, m_texture[4]);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_RGBA32F, kWidth, kHeight, 0, GL_RGBA, GL_FLOAT, &texture_data[0]);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture[5]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, kWidth, kHeight, kDepth, 0, GL_RGBA, GL_FLOAT,
					 &texture_data[0]);

		glActiveTexture(GL_TEXTURE6);
		glBindBuffer(GL_TEXTURE_BUFFER, m_texture_buffer);
		glBufferData(GL_TEXTURE_BUFFER, kBufferSize * sizeof(vec4), &texture_data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture[6]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_texture_buffer);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture[7]);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA32F, kWidth, kHeight, GL_FALSE);

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_texture[8]);
		glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 4, GL_RGBA32F, kWidth, kHeight, kDepth, GL_FALSE);

		// clear MS textures
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture[7], 0);
		glClearBufferfv(GL_COLOR, 0, &vec4(123.0f)[0]);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture[8], 0);
		glClearBufferfv(GL_COLOR, 0, &vec4(123.0f)[0]);
		glDeleteFramebuffers(1, &fbo);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * kBufferSize * 9, &buffer_data[0]);
		for (GLuint index = 0; index < kBufferSize * 9; ++index)
		{
			if (!IsEqual(buffer_data[index], vec4(123.0f)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Incorrect data at index " << index << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		memset(m_texture, 0, sizeof(m_texture));
		m_texture_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!RunIteration(uvec3(4, 4, 4), uvec3(8, 1, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 2), uvec3(2, 4, 1), true))
			return ERROR;
		if (!RunIteration(uvec3(2, 2, 2), uvec3(2, 2, 2), false))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glActiveTexture(GL_TEXTURE0);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteTextures(9, m_texture);
		glDeleteBuffers(1, &m_texture_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicResourceImage : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return NL "Compute Shader resources - Images";
	}

	virtual std::string Purpose()
	{
		return NL "Verify that reading/writing GPU memory via image variables work as expected.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which uses two image2D variables to read and write underlying GPU memory." NL
				  "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Verify memory content." NL "4. Repeat for different texture and CS work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_draw_program;
	GLuint m_texture[2];
	GLuint m_dispatch_buffer;
	GLuint m_vertex_array;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z()
		   << ") in;" NL "layout(rgba32f) coherent uniform image2D g_image1;" NL
			  "layout(rgba32f) uniform image2D g_image2;" NL "const uvec3 kGlobalSize = uvec3("
		   << global_size.x() << ", " << global_size.y() << ", " << global_size.z()
		   << ");" NL "void main() {" NL
			  "  if (gl_GlobalInvocationID.x >= kGlobalSize.x || gl_GlobalInvocationID.y >= kGlobalSize.y) return;" NL
			  "  vec4 color = vec4(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y) / 255.0;" NL
			  "  imageStore(g_image1, ivec2(gl_GlobalInvocationID), color);" NL
			  "  vec4 c = imageLoad(g_image1, ivec2(gl_GlobalInvocationID));" NL
			  "  imageStore(g_image2, ivec2(gl_GlobalInvocationID), c);" NL "}";
		return ss.str();
	}

	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_image1"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image2"), 1);
		glUseProgram(0);

		const GLint  kWidth  = static_cast<GLint>(local_size.x() * num_groups.x());
		const GLint  kHeight = static_cast<GLint>(local_size.y() * num_groups.y());
		const GLint  kDepth  = static_cast<GLint>(local_size.z() * num_groups.z());
		const GLuint kSize   = kWidth * kHeight * kDepth;

		std::vector<vec4> data(kSize);
		if (m_texture[0] == 0)
			glGenTextures(2, m_texture);

		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, m_texture[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kWidth, kHeight, 0, GL_RGBA, GL_FLOAT, &data[0]);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glClear(GL_COLOR_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_vertex_array);
		glViewport(0, 0, kWidth, kHeight);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

		std::vector<vec4> display(kWidth * kHeight);
		glReadPixels(0, 0, kWidth, kHeight, GL_RGBA, GL_FLOAT, &display[0]);

		for (int y = 0; y < kHeight; ++y)
		{
			for (int x = 0; x < kWidth; ++x)
			{
				if (y >= getWindowHeight() || x >= getWindowWidth())
				{
					continue;
				}
				const vec4 c = vec4(float(y + x) / 255.0f);
				if (!ColorEqual(display[y * kWidth + x], c, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Got " << display[y * kWidth + x].x() << ", "
						<< display[y * kWidth + x].y() << ", " << display[y * kWidth + x].z() << ", "
						<< display[y * kWidth + x].w() << ", expected " << c.x() << ", " << c.y() << ", " << c.z()
						<< ", " << c.w() << " at " << x << ", " << y << tcu::TestLog::EndMessage;
					return false;
				}
			}
		}

		return true;
	}

	virtual long Setup()
	{
		m_program = 0;
		memset(m_texture, 0, sizeof(m_texture));
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{

		const char* const glsl_vs =
			NL "out StageData {" NL "  vec2 texcoord;" NL "} vs_out;" NL
			   "const vec2 g_quad[] = vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));" NL "void main() {" NL
			   "  gl_Position = vec4(g_quad[gl_VertexID], 0, 1);" NL
			   "  vs_out.texcoord = 0.5 + 0.5 * g_quad[gl_VertexID];" NL "}";

		const char* glsl_fs =
			NL "in StageData {" NL "  vec2 texcoord;" NL "} fs_in;" NL "layout(location = 0) out vec4 o_color;" NL
			   "uniform sampler2D g_image1;" NL "uniform sampler2D g_image2;" NL "void main() {" NL
			   "  vec4 c1 = texture(g_image1, fs_in.texcoord);" NL "  vec4 c2 = texture(g_image2, fs_in.texcoord);" NL
			   "  if (c1 == c2) o_color = c1;" NL "  else o_color = vec4(1, 0, 0, 1);" NL "}";

		m_draw_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_draw_program);
		if (!CheckProgram(m_draw_program))
			return ERROR;

		glUseProgram(m_draw_program);
		glUniform1i(glGetUniformLocation(m_draw_program, "g_image1"), 0);
		glUniform1i(glGetUniformLocation(m_draw_program, "g_image2"), 1);
		glUseProgram(0);

		glGenVertexArrays(1, &m_vertex_array);

		if (!pixelFormat.alphaBits)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Test requires default framebuffer alpha bits" << tcu::TestLog::EndMessage;
			return NO_ERROR;
		}

		if (!RunIteration(uvec3(8, 16, 1), uvec3(8, 4, 1), true))
			return ERROR;
		if (!RunIteration(uvec3(4, 32, 1), uvec3(16, 2, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(16, 4, 1), uvec3(4, 16, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(8, 8, 1), uvec3(8, 8, 1), true))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteTextures(2, m_texture);
		glDeleteBuffers(1, &m_dispatch_buffer);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		return NO_ERROR;
	}
};

class BasicResourceAtomicCounter : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Compute Shader resources - Atomic Counters";
	}

	virtual std::string Purpose()
	{
		return NL
			"1. Verify that Atomic Counters work as expected in CS." NL
			"2. Verify that built-in functions: atomicCounterIncrement and atomicCounterDecrement work correctly." NL
			"3. Verify that GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER is accepted by" NL
			"    GetActiveAtomicCounterBufferiv command.";
	}

	virtual std::string Method()
	{
		return NL
			"1. Create CS which uses two atomic_uint variables." NL
			"2. In CS write values returned by atomicCounterIncrement and atomicCounterDecrement functions to SSBO." NL
			"3. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL "4. Verify SSBO content." NL
			"5. Repeat for different buffer and CS work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_counter_buffer[2];
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z()
		   << ") in;" NL "layout(std430, binding = 0) buffer Output {" NL "  uint inc_data["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uint dec_data["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "};" NL "layout(binding = 0, offset = 0) uniform atomic_uint g_inc_counter;" NL
			  "layout(binding = 1, offset = 0) uniform atomic_uint g_dec_counter;" NL "void main() {" NL
			  "  const uint index = atomicCounterIncrement(g_inc_counter);" NL "  inc_data[index] = index;" NL
			  "  dec_data[index] = atomicCounterDecrement(g_dec_counter);" NL "}";
		return ss.str();
	}

	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		GLint p[2] = { 0 };
		glGetActiveAtomicCounterBufferiv(m_program, 0, GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER, &p[0]);
		glGetActiveAtomicCounterBufferiv(m_program, 1, GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER, &p[1]);

		if (p[0] == GL_FALSE || p[1] == GL_FALSE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER should be TRUE."
				<< tcu::TestLog::EndMessage;
			return false;
		}

		const GLint  kWidth  = static_cast<GLint>(local_size.x() * num_groups.x());
		const GLint  kHeight = static_cast<GLint>(local_size.y() * num_groups.y());
		const GLint  kDepth  = static_cast<GLint>(local_size.z() * num_groups.z());
		const GLuint kSize   = kWidth * kHeight * kDepth;

		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * kSize * 2, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		if (m_counter_buffer[0] == 0)
			glGenBuffers(2, m_counter_buffer);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_counter_buffer[0]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STREAM_DRAW);
		*static_cast<GLuint*>(glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY)) = 0;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, m_counter_buffer[1]);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STREAM_DRAW);
		*static_cast<GLuint*>(glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY)) = kSize;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}

		std::vector<GLuint> data(kSize);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * kSize, &data[0]);

		for (GLuint i = 0; i < kSize; ++i)
		{
			if (data[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Value at index " << i << " is "
													<< data[i] << " should be " << i << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		GLuint value;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counter_buffer[0]);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &value);
		if (value != kSize)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Final atomic counter value (buffer 0) is "
												<< value << " should be " << kSize << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counter_buffer[1]);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &value);
		if (value != 0)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Final atomic counter value (buffer 1) is "
												<< value << " should be 0." << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		memset(m_counter_buffer, 0, sizeof(m_counter_buffer));
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!RunIteration(uvec3(4, 3, 2), uvec3(2, 3, 4), false))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 1), uvec3(1, 1, 1), true))
			return ERROR;
		if (!RunIteration(uvec3(1, 6, 1), uvec3(1, 1, 8), false))
			return ERROR;
		if (!RunIteration(uvec3(4, 1, 2), uvec3(10, 3, 4), true))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_counter_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class BasicResourceSubroutine : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Compute Shader resources - Subroutines";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that subroutines work as expected in CS." NL
				  "2. Verify that subroutines array can be indexed with gl_WorkGroupID built-in variable." NL
				  "3. Verify that atomicCounterIncrement, imageLoad and texelFetch functions" NL
				  "    work as expected when called in CS from subroutine.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which uses array of subroutines." NL
				  "2. In CS index subroutine array with gl_WorkGroupID built-in variable." NL
				  "3. In each subroutine load data from SSBO0 and write it to SSBO1." NL
				  "3. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "4. Verify SSBO1 content." NL "5. Repeat for different buffer and CS work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_atomic_counter_buffer;
	GLuint m_storage_buffer[2];
	GLuint m_buffer[2];
	GLuint m_texture_buffer[2];

	virtual long Setup()
	{
		m_program				= 0;
		m_atomic_counter_buffer = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		memset(m_buffer, 0, sizeof(m_buffer));
		memset(m_texture_buffer, 0, sizeof(m_texture_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 16) in;" NL "layout(binding = 1, std430) buffer Input {" NL "  uvec4 data[16];" NL
			   "} g_input;" NL "layout(std430, binding = 0) buffer Output {" NL "  uvec4 g_output[64];" NL "};" NL
			   "subroutine void ComputeType();" NL "subroutine uniform ComputeType Compute[4];" NL
			   "layout(binding = 0, offset = 0) uniform atomic_uint g_atomic_counter;" NL
			   "layout(rgba32ui) readonly uniform uimageBuffer g_image_buffer;" NL
			   "uniform usamplerBuffer g_sampler_buffer;" NL "subroutine(ComputeType)" NL "void Compute0() {" NL
			   "  const uint index = atomicCounterIncrement(g_atomic_counter);" NL
			   "  g_output[index] = uvec4(index);" NL "}" NL "subroutine(ComputeType)" NL "void Compute1() {" NL
			   "  g_output[gl_GlobalInvocationID.x] = g_input.data[gl_LocalInvocationIndex];" NL "}" NL
			   "subroutine(ComputeType)" NL "void Compute2() {" NL
			   "  g_output[gl_GlobalInvocationID.x] = imageLoad(g_image_buffer, int(gl_LocalInvocationIndex));" NL
			   "}" NL "subroutine(ComputeType)" NL "void Compute3() {" NL
			   "  g_output[gl_GlobalInvocationID.x] = texelFetch(g_sampler_buffer, int(gl_LocalInvocationIndex));" NL
			   "}" NL "void main() {" NL "  Compute[gl_WorkGroupID.x]();" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);
		/* output buffer */
		{
			std::vector<uvec4> data(64, uvec4(0xffff));
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uvec4) * 64, &data[0], GL_DYNAMIC_DRAW);
		}
		/* input buffer */
		{
			std::vector<uvec4> data(16);
			for (GLuint i = 0; i < 16; ++i)
				data[i]   = uvec4(i + 16);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uvec4) * 16, &data[0], GL_DYNAMIC_DRAW);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenBuffers(1, &m_atomic_counter_buffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomic_counter_buffer);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STREAM_DRAW);
		*static_cast<GLuint*>(glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY)) = 0;
		glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

		glGenBuffers(2, m_buffer);
		/* image buffer */
		{
			std::vector<uvec4> data(16);
			for (GLuint i = 0; i < 16; ++i)
				data[i]   = uvec4(i + 32);
			glBindBuffer(GL_TEXTURE_BUFFER, m_buffer[0]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(uvec4) * 16, &data[0], GL_STATIC_DRAW);
		}
		/* texture buffer */
		{
			std::vector<uvec4> data(16);
			for (GLuint i = 0; i < 16; ++i)
				data[i]   = uvec4(i + 48);
			glBindBuffer(GL_TEXTURE_BUFFER, m_buffer[1]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(uvec4) * 16, &data[0], GL_STATIC_DRAW);
		}
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(2, m_texture_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture_buffer[0]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, m_buffer[0]);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture_buffer[1]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, m_buffer[1]);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		const GLuint index_compute0 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Compute0");
		const GLuint index_compute1 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Compute1");
		const GLuint index_compute2 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Compute2");
		const GLuint index_compute3 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Compute3");
		const GLint  loc_compute0   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "Compute[0]");
		const GLint  loc_compute1   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "Compute[1]");
		const GLint  loc_compute2   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "Compute[2]");
		const GLint  loc_compute3   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "Compute[3]");

		// bind resources
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_atomic_counter_buffer);
		glBindImageTexture(0, m_texture_buffer[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32UI);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture_buffer[1]);

		glUseProgram(m_program);

		// setup subroutines
		GLuint indices[4];
		indices[loc_compute0] = index_compute0;
		indices[loc_compute1] = index_compute1;
		indices[loc_compute2] = index_compute2;
		indices[loc_compute3] = index_compute3;
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 4, indices);

		glDispatchCompute(4, 1, 1);

		std::vector<uvec4> data(64);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uvec4) * 64, &data[0]);

		for (GLuint i = 0; i < 64; ++i)
		{
			if (!IsEqual(data[i], uvec4(i)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Invalid value at index " << i << "." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		GLuint value;
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomic_counter_buffer);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &value);
		if (value != 16)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Final atomic counter value is " << value
												<< " should be 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_atomic_counter_buffer);
		glDeleteBuffers(2, m_storage_buffer);
		glDeleteBuffers(2, m_buffer);
		glDeleteTextures(2, m_texture_buffer);
		return NO_ERROR;
	}
};

class BasicResourceUniform : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Compute Shader resources - Uniforms";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that all types of uniform variables work as expected in CS." NL
				  "2. Verify that uniform variables can be updated with Uniform* and ProgramUniform* commands." NL
				  "3. Verify that re-linking CS program works as expected.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which uses all (single precision and integer) types of uniform variables." NL
				  "2. Update uniform variables with ProgramUniform* commands." NL
				  "3. Verify that uniform variables were updated correctly." NL "4. Re-link CS program." NL
				  "5. Update uniform variables with Uniform* commands." NL
				  "6. Verify that uniform variables were updated correctly.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs = NL
			"layout(local_size_x = 1) in;" NL "buffer Result {" NL "  int g_result;" NL "};" NL "uniform float g_0;" NL
			"uniform vec2 g_1;" NL "uniform vec3 g_2;" NL "uniform vec4 g_3;" NL "uniform mat2 g_4;" NL
			"uniform mat2x3 g_5;" NL "uniform mat2x4 g_6;" NL "uniform mat3x2 g_7;" NL "uniform mat3 g_8;" NL
			"uniform mat3x4 g_9;" NL "uniform mat4x2 g_10;" NL "uniform mat4x3 g_11;" NL "uniform mat4 g_12;" NL
			"uniform int g_13;" NL "uniform ivec2 g_14;" NL "uniform ivec3 g_15;" NL "uniform ivec4 g_16;" NL
			"uniform uint g_17;" NL "uniform uvec2 g_18;" NL "uniform uvec3 g_19;" NL "uniform uvec4 g_20;" NL NL
			"void main() {" NL "  g_result = 1;" NL NL "  if (g_0 != 1.0) g_result = 0;" NL
			"  if (g_1 != vec2(2.0, 3.0)) g_result = 0;" NL "  if (g_2 != vec3(4.0, 5.0, 6.0)) g_result = 0;" NL
			"  if (g_3 != vec4(7.0, 8.0, 9.0, 10.0)) g_result = 0;" NL NL
			"  if (g_4 != mat2(11.0, 12.0, 13.0, 14.0)) g_result = 0;" NL
			"  if (g_5 != mat2x3(15.0, 16.0, 17.0, 18.0, 19.0, 20.0)) g_result = 0;" NL
			"  if (g_6 != mat2x4(21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0)) g_result = 0;" NL NL
			"  if (g_7 != mat3x2(29.0, 30.0, 31.0, 32.0, 33.0, 34.0)) g_result = 0;" NL
			"  if (g_8 != mat3(35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0)) g_result = 0;" NL
			"  if (g_9 != mat3x4(44.0, 45.0, 46.0, 47.0, 48.0, 49.0, 50.0, 51.0, 52.0, 53.0, 54.0, 55.0)) g_result = "
			"0;" NL NL "  if (g_10 != mat4x2(56.0, 57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0)) g_result = 0;" NL
			"  if (g_11 != mat4x3(63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0, 70.0, 71.0, 27.0, 73, 74.0)) g_result = "
			"0;" NL "  if (g_12 != mat4(75.0, 76.0, 77.0, 78.0, 79.0, 80.0, 81.0, 82.0, 83.0, 84.0, 85.0, 86.0, 87.0, "
			"88.0, 89.0, 90.0)) g_result = 0;" NL NL "  if (g_13 != 91) g_result = 0;" NL
			"  if (g_14 != ivec2(92, 93)) g_result = 0;" NL "  if (g_15 != ivec3(94, 95, 96)) g_result = 0;" NL
			"  if (g_16 != ivec4(97, 98, 99, 100)) g_result = 0;" NL NL "  if (g_17 != 101u) g_result = 0;" NL
			"  if (g_18 != uvec2(102u, 103u)) g_result = 0;" NL
			"  if (g_19 != uvec3(104u, 105u, 106u)) g_result = 0;" NL
			"  if (g_20 != uvec4(107u, 108u, 109u, 110u)) g_result = 0;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		/* create buffer */
		{
			const int data = 123;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}

		glProgramUniform1f(m_program, glGetUniformLocation(m_program, "g_0"), 1.0f);
		glProgramUniform2f(m_program, glGetUniformLocation(m_program, "g_1"), 2.0f, 3.0f);
		glProgramUniform3f(m_program, glGetUniformLocation(m_program, "g_2"), 4.0f, 5.0f, 6.0f);
		glProgramUniform4f(m_program, glGetUniformLocation(m_program, "g_3"), 7.0f, 8.0f, 9.0f, 10.0f);

		/* mat2 */
		{
			const GLfloat value[4] = { 11.0f, 12.0f, 13.0f, 14.0f };
			glProgramUniformMatrix2fv(m_program, glGetUniformLocation(m_program, "g_4"), 1, GL_FALSE, value);
		}
		/* mat2x3 */
		{
			const GLfloat value[6] = { 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f };
			glProgramUniformMatrix2x3fv(m_program, glGetUniformLocation(m_program, "g_5"), 1, GL_FALSE, value);
		}
		/* mat2x4 */
		{
			const GLfloat value[8] = { 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f };
			glProgramUniformMatrix2x4fv(m_program, glGetUniformLocation(m_program, "g_6"), 1, GL_FALSE, value);
		}

		/* mat3x2 */
		{
			const GLfloat value[6] = { 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f };
			glProgramUniformMatrix3x2fv(m_program, glGetUniformLocation(m_program, "g_7"), 1, GL_FALSE, value);
		}
		/* mat3 */
		{
			const GLfloat value[9] = { 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f };
			glProgramUniformMatrix3fv(m_program, glGetUniformLocation(m_program, "g_8"), 1, GL_FALSE, value);
		}
		/* mat3x4 */
		{
			const GLfloat value[12] = { 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f,
										50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f };
			glProgramUniformMatrix3x4fv(m_program, glGetUniformLocation(m_program, "g_9"), 1, GL_FALSE, value);
		}

		/* mat4x2 */
		{
			const GLfloat value[8] = { 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f };
			glProgramUniformMatrix4x2fv(m_program, glGetUniformLocation(m_program, "g_10"), 1, GL_FALSE, value);
		}
		/* mat4x3 */
		{
			const GLfloat value[12] = {
				63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 27.0f, 73, 74.0f
			};
			glProgramUniformMatrix4x3fv(m_program, glGetUniformLocation(m_program, "g_11"), 1, GL_FALSE, value);
		}
		/* mat4 */
		{
			const GLfloat value[16] = { 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f,
										83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f };
			glProgramUniformMatrix4fv(m_program, glGetUniformLocation(m_program, "g_12"), 1, GL_FALSE, value);
		}

		glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_13"), 91);
		glProgramUniform2i(m_program, glGetUniformLocation(m_program, "g_14"), 92, 93);
		glProgramUniform3i(m_program, glGetUniformLocation(m_program, "g_15"), 94, 95, 96);
		glProgramUniform4i(m_program, glGetUniformLocation(m_program, "g_16"), 97, 98, 99, 100);

		glProgramUniform1ui(m_program, glGetUniformLocation(m_program, "g_17"), 101);
		glProgramUniform2ui(m_program, glGetUniformLocation(m_program, "g_18"), 102, 103);
		glProgramUniform3ui(m_program, glGetUniformLocation(m_program, "g_19"), 104, 105, 106);
		glProgramUniform4ui(m_program, glGetUniformLocation(m_program, "g_20"), 107, 108, 109, 110);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		// re-link program (all uniforms will be set to zero)
		glLinkProgram(m_program);

		{
			const int data = 123;
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
		}

		glUniform1f(glGetUniformLocation(m_program, "g_0"), 1.0f);
		glUniform2f(glGetUniformLocation(m_program, "g_1"), 2.0f, 3.0f);
		glUniform3f(glGetUniformLocation(m_program, "g_2"), 4.0f, 5.0f, 6.0f);
		glUniform4f(glGetUniformLocation(m_program, "g_3"), 7.0f, 8.0f, 9.0f, 10.0f);

		/* mat2 */
		{
			const GLfloat value[4] = { 11.0f, 12.0f, 13.0f, 14.0f };
			glUniformMatrix2fv(glGetUniformLocation(m_program, "g_4"), 1, GL_FALSE, value);
		}
		/* mat2x3 */
		{
			const GLfloat value[6] = { 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f };
			glUniformMatrix2x3fv(glGetUniformLocation(m_program, "g_5"), 1, GL_FALSE, value);
		}
		/* mat2x4 */
		{
			const GLfloat value[8] = { 21.0f, 22.0f, 23.0f, 24.0f, 25.0f, 26.0f, 27.0f, 28.0f };
			glUniformMatrix2x4fv(glGetUniformLocation(m_program, "g_6"), 1, GL_FALSE, value);
		}

		/* mat3x2 */
		{
			const GLfloat value[6] = { 29.0f, 30.0f, 31.0f, 32.0f, 33.0f, 34.0f };
			glUniformMatrix3x2fv(glGetUniformLocation(m_program, "g_7"), 1, GL_FALSE, value);
		}
		/* mat3 */
		{
			const GLfloat value[9] = { 35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f };
			glUniformMatrix3fv(glGetUniformLocation(m_program, "g_8"), 1, GL_FALSE, value);
		}
		/* mat3x4 */
		{
			const GLfloat value[12] = { 44.0f, 45.0f, 46.0f, 47.0f, 48.0f, 49.0f,
										50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f };
			glUniformMatrix3x4fv(glGetUniformLocation(m_program, "g_9"), 1, GL_FALSE, value);
		}

		/* mat4x2 */
		{
			const GLfloat value[8] = { 56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f };
			glUniformMatrix4x2fv(glGetUniformLocation(m_program, "g_10"), 1, GL_FALSE, value);
		}
		/* mat4x3 */
		{
			const GLfloat value[12] = {
				63.0f, 64.0f, 65.0f, 66.0f, 67.0f, 68.0f, 69.0f, 70.0f, 71.0f, 27.0f, 73, 74.0f
			};
			glUniformMatrix4x3fv(glGetUniformLocation(m_program, "g_11"), 1, GL_FALSE, value);
		}
		/* mat4 */
		{
			const GLfloat value[16] = { 75.0f, 76.0f, 77.0f, 78.0f, 79.0f, 80.0f, 81.0f, 82.0f,
										83.0f, 84.0f, 85.0f, 86.0f, 87.0f, 88.0f, 89.0f, 90.0f };
			glUniformMatrix4fv(glGetUniformLocation(m_program, "g_12"), 1, GL_FALSE, value);
		}

		glUniform1i(glGetUniformLocation(m_program, "g_13"), 91);
		glUniform2i(glGetUniformLocation(m_program, "g_14"), 92, 93);
		glUniform3i(glGetUniformLocation(m_program, "g_15"), 94, 95, 96);
		glUniform4i(glGetUniformLocation(m_program, "g_16"), 97, 98, 99, 100);

		glUniform1ui(glGetUniformLocation(m_program, "g_17"), 101);
		glUniform2ui(glGetUniformLocation(m_program, "g_18"), 102, 103);
		glUniform3ui(glGetUniformLocation(m_program, "g_19"), 104, 105, 106);
		glUniform4ui(glGetUniformLocation(m_program, "g_20"), 107, 108, 109, 110);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class BasicBuiltinVariables : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "CS built-in variables";
	}

	virtual std::string Purpose()
	{
		return NL "Verify that all (gl_WorkGroupSize, gl_WorkGroupID, gl_LocalInvocationID," NL
				  "gl_GlobalInvocationID, gl_NumWorkGroups, gl_WorkGroupSize)" NL
				  "CS built-in variables has correct values.";
	}

	virtual std::string Method()
	{
		return NL "1. Create CS which writes all built-in variables to SSBO." NL
				  "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Verify SSBO content." NL "4. Repeat for several different local and global work sizes.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z() << ") in;" NL "const uvec3 kGlobalSize = uvec3(" << global_size.x()
		   << ", " << global_size.y() << ", " << global_size.z()
		   << ");" NL "layout(std430) buffer OutputBuffer {" NL "  uvec4 num_work_groups["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uvec4 work_group_size["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uvec4 work_group_id["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uvec4 local_invocation_id["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uvec4 global_invocation_id["
		   << global_size.x() * global_size.y() * global_size.z() << "];" NL "  uvec4 local_invocation_index["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "} g_out_buffer;" NL "void main() {" NL
			  "  if ((gl_WorkGroupSize * gl_WorkGroupID + gl_LocalInvocationID) != gl_GlobalInvocationID) return;" NL
			  "  const uint global_index = gl_GlobalInvocationID.x +" NL
			  "                            gl_GlobalInvocationID.y * kGlobalSize.x +" NL
			  "                            gl_GlobalInvocationID.z * kGlobalSize.x * kGlobalSize.y;" NL
			  "  g_out_buffer.num_work_groups[global_index] = uvec4(gl_NumWorkGroups, 0);" NL
			  "  g_out_buffer.work_group_size[global_index] = uvec4(gl_WorkGroupSize, 0);" NL
			  "  g_out_buffer.work_group_id[global_index] = uvec4(gl_WorkGroupID, 0);" NL
			  "  g_out_buffer.local_invocation_id[global_index] = uvec4(gl_LocalInvocationID, 0);" NL
			  "  g_out_buffer.global_invocation_id[global_index] = uvec4(gl_GlobalInvocationID, 0);" NL
			  "  g_out_buffer.local_invocation_index[global_index] = uvec4(gl_LocalInvocationIndex);" NL "}";
		return ss.str();
	}

	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		const GLuint kBufferSize =
			local_size.x() * num_groups.x() * local_size.y() * num_groups.y() * local_size.z() * num_groups.z();

		std::vector<uvec4> data(kBufferSize * 6);
		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uvec4) * kBufferSize * 6, &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uvec4) * kBufferSize * 6, &data[0]);

		// gl_NumWorkGroups
		for (GLuint index = 0; index < kBufferSize; ++index)
		{
			if (!IsEqual(data[index], uvec4(num_groups.x(), num_groups.y(), num_groups.z(), 0)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "gl_NumWorkGroups: Invalid data at index " << index << "."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
		// gl_WorkGroupSize
		for (GLuint index = kBufferSize; index < 2 * kBufferSize; ++index)
		{
			if (!IsEqual(data[index], uvec4(local_size.x(), local_size.y(), local_size.z(), 0)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "gl_WorkGroupSize: Invalid data at index " << index << "."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
		// gl_WorkGroupID
		for (GLuint index = 2 * kBufferSize; index < 3 * kBufferSize; ++index)
		{
			uvec3 expected = IndexTo3DCoord(index - 2 * kBufferSize, local_size.x() * num_groups.x(),
											local_size.y() * num_groups.y());
			expected.x() /= local_size.x();
			expected.y() /= local_size.y();
			expected.z() /= local_size.z();
			if (!IsEqual(data[index], uvec4(expected.x(), expected.y(), expected.z(), 0)))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "gl_WorkGroupID: Invalid data at index "
													<< index << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		// gl_LocalInvocationID
		for (GLuint index = 3 * kBufferSize; index < 4 * kBufferSize; ++index)
		{
			uvec3 expected = IndexTo3DCoord(index - 3 * kBufferSize, local_size.x() * num_groups.x(),
											local_size.y() * num_groups.y());
			expected.x() %= local_size.x();
			expected.y() %= local_size.y();
			expected.z() %= local_size.z();
			if (!IsEqual(data[index], uvec4(expected.x(), expected.y(), expected.z(), 0)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "gl_LocalInvocationID: Invalid data at index " << index << "."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
		// gl_GlobalInvocationID
		for (GLuint index = 4 * kBufferSize; index < 5 * kBufferSize; ++index)
		{
			uvec3 expected = IndexTo3DCoord(index - 4 * kBufferSize, local_size.x() * num_groups.x(),
											local_size.y() * num_groups.y());
			if (!IsEqual(data[index], uvec4(expected.x(), expected.y(), expected.z(), 0)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "gl_GlobalInvocationID: Invalid data at index " << index << "."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
		// gl_LocalInvocationIndex
		for (GLuint index = 5 * kBufferSize; index < 6 * kBufferSize; ++index)
		{
			uvec3 coord = IndexTo3DCoord(index - 5 * kBufferSize, local_size.x() * num_groups.x(),
										 local_size.y() * num_groups.y());
			const GLuint expected = (coord.x() % local_size.x()) + (coord.y() % local_size.y()) * local_size.x() +
									(coord.z() % local_size.z()) * local_size.x() * local_size.y();
			if (!IsEqual(data[index], uvec4(expected)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "gl_LocalInvocationIndex: Invalid data at index " << index << "."
					<< tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!RunIteration(uvec3(64, 1, 1), uvec3(8, 1, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 64), uvec3(1, 5, 2), true))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 4), uvec3(2, 2, 2), false))
			return ERROR;
		if (!RunIteration(uvec3(3, 2, 1), uvec3(1, 2, 3), true))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 2), uvec3(2, 4, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 7), uvec3(2, 1, 4), true))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicMax : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return NL "CS max values";
	}

	virtual std::string Purpose()
	{
		return NL "Verify (on the API and GLSL side) that all GL_MAX_COMPUTE_* values are not less than" NL
				  "required by the OpenGL specification.";
	}

	virtual std::string Method()
	{
		return NL "1. Use all API commands to query all GL_MAX_COMPUTE_* values. Verify that they are correct." NL
				  "2. Verify all gl_MaxCompute* constants in the GLSL.";
	}

	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_buffer;

	bool CheckIndexed(GLenum target, const GLint* min_values)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		for (GLuint c = 0; c < 3; c++)
		{
			glGetIntegeri_v(target, c, &i);
			if (i < min_values[c])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Is " << i << " should be at least "
													<< min_values[c] << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		for (GLuint c = 0; c < 3; c++)
		{
			glGetInteger64i_v(target, c, &i64);
			if (i64 < static_cast<GLint64>(min_values[c]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Is " << static_cast<GLint>(i64) << " should be at least "
					<< min_values[c] << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		for (GLuint c = 0; c < 3; c++)
		{
			glGetFloati_v(target, c, &f);
			if (f < static_cast<GLfloat>(min_values[c]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Is " << static_cast<GLint>(f) << " should be at least "
					<< min_values[c] << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		for (GLuint c = 0; c < 3; c++)
		{
			glGetDoublei_v(target, c, &d);
			if (d < static_cast<GLdouble>(min_values[c]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Is " << static_cast<GLint>(d) << " should be at least "
					<< min_values[c] << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		for (GLuint c = 0; c < 3; c++)
		{
			glGetBooleani_v(target, c, &b);
			if (b == GL_FALSE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Is GL_FALSE should be at least GL_TRUE." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}

	bool Check(GLenum target, const GLint min_value)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		glGetIntegerv(target, &i);
		if (i < min_value)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Is " << i << " should be at least "
												<< min_value << "." << tcu::TestLog::EndMessage;
			return false;
		}
		glGetInteger64v(target, &i64);
		if (static_cast<GLint>(i64) < min_value)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Is " << static_cast<GLint>(i64) << " should be at least " << min_value
				<< "." << tcu::TestLog::EndMessage;
			return false;
		}
		glGetFloatv(target, &f);
		if (f < static_cast<GLfloat>(min_value))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Is " << static_cast<GLint>(f) << " should be at least " << min_value << "."
				<< tcu::TestLog::EndMessage;
			return false;
		}
		glGetDoublev(target, &d);
		if (d < static_cast<GLdouble>(min_value))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Is " << static_cast<GLint>(d) << " should be at least " << min_value << "."
				<< tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleanv(target, &b);
		if (b != (min_value ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Is " << b << " should be "
												<< (min_value ? GL_TRUE : GL_FALSE) << "." << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}

	virtual long Setup()
	{
		m_program = 0;
		m_buffer  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const GLint work_group_count[3] = { 65535, 65535, 65535 };
		if (!CheckIndexed(GL_MAX_COMPUTE_WORK_GROUP_COUNT, work_group_count))
			return ERROR;

		const GLint work_group_size[3] = { 1024, 1024, 64 };
		if (!CheckIndexed(GL_MAX_COMPUTE_WORK_GROUP_SIZE, work_group_size))
			return ERROR;

		if (!Check(GL_MAX_COMPUTE_UNIFORM_BLOCKS, 12))
			return ERROR;
		if (!Check(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, 16))
			return ERROR;
		if (!Check(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, 8))
			return ERROR;
		if (!Check(GL_MAX_COMPUTE_ATOMIC_COUNTERS, 8))
			return ERROR;
		if (!Check(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, 32768))
			return ERROR;

		if (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)))
		{
			if (!Check(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, 1024))
				return ERROR;
		}
		else
		{
			if (!Check(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, 512))
				return ERROR;
		}

		if (!Check(GL_MAX_COMPUTE_IMAGE_UNIFORMS, 8))
			return ERROR;
		if (!Check(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, 512))
			return ERROR;

		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL "  int g_output;" NL "};" NL
			   "uniform ivec3 MaxComputeWorkGroupCount;" NL "uniform ivec3 MaxComputeWorkGroupSize;" NL
			   "uniform int MaxComputeUniformComponents;" NL "uniform int MaxComputeTextureImageUnits;" NL
			   "uniform int MaxComputeImageUniforms;" NL "uniform int MaxComputeAtomicCounters;" NL
			   "uniform int MaxComputeAtomicCounterBuffers;" NL "void main() {" NL "  g_output = 1;" NL
			   "  if (MaxComputeWorkGroupCount != gl_MaxComputeWorkGroupCount) g_output = 0;" NL
			   "  if (MaxComputeWorkGroupSize != gl_MaxComputeWorkGroupSize) g_output = 0;" NL
			   "  if (MaxComputeUniformComponents != gl_MaxComputeUniformComponents) g_output = 0;" NL
			   "  if (MaxComputeTextureImageUnits != gl_MaxComputeTextureImageUnits) g_output = 0;" NL
			   "  if (MaxComputeImageUniforms != gl_MaxComputeImageUniforms) g_output = 0;" NL
			   "  if (MaxComputeAtomicCounters != gl_MaxComputeAtomicCounters) g_output = 0;" NL
			   "  if (MaxComputeAtomicCounterBuffers != gl_MaxComputeAtomicCounterBuffers) g_output = 0;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;
		glUseProgram(m_program);

		GLint p[3];
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &p[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &p[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &p[2]);
		glUniform3i(glGetUniformLocation(m_program, "MaxComputeWorkGroupCount"), p[0], p[1], p[2]);

		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &p[0]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &p[1]);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &p[2]);
		glUniform3iv(glGetUniformLocation(m_program, "MaxComputeWorkGroupSize"), 1, p);

		glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, p);
		glUniform1i(glGetUniformLocation(m_program, "MaxComputeUniformComponents"), p[0]);

		glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, p);
		glUniform1iv(glGetUniformLocation(m_program, "MaxComputeTextureImageUnits"), 1, p);

		glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, p);
		glUniform1i(glGetUniformLocation(m_program, "MaxComputeImageUniforms"), p[0]);

		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, p);
		glUniform1i(glGetUniformLocation(m_program, "MaxComputeAtomicCounters"), p[0]);

		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, p);
		glUniform1i(glGetUniformLocation(m_program, "MaxComputeAtomicCounterBuffers"), p[0]);

		GLint data = 0xffff;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint), &data, GL_DYNAMIC_DRAW);

		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint), &data);

		return data == 1 ? NO_ERROR : ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

class BasicBuildMonolithic : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Building CS monolithic program";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that building monolithic CS program works as expected." NL
				  "2. Verify that program consisting from 3 compilation units links as expected." NL
				  "3. Verify that CS consisting from 2 strings compiles as expected.";
	}

	virtual std::string Method()
	{
		return NL "1. Create, compile and link CS using CreateShader, CompileShader and LinkProgram commands." NL
				  "2. Dispatch and verify CS program.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	virtual long Run()
	{
		const char* const cs1[2] = { "#version 430 core",

									 NL "layout(local_size_x = 1) in;" NL "void Run();" NL "void main() {" NL
										"  Run();" NL "}" };

		const char* const cs2 =
			"#version 430 core" NL "layout(binding = 0, std430) buffer Output {" NL "  vec4 g_output;" NL "};" NL
			"vec4 CalculateOutput();" NL "void Run() {" NL "  g_output = CalculateOutput();" NL "}";

		const char* const cs3 =
			"#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(binding = 0, std430) buffer Output {" NL
			"  vec4 g_output;" NL "};" NL "vec4 CalculateOutput() {" NL "  g_output = vec4(0);" NL
			"  return vec4(1, 2, 3, 4);" NL "}";

		const GLuint sh1 = glCreateShader(GL_COMPUTE_SHADER);

		GLint type;
		glGetShaderiv(sh1, GL_SHADER_TYPE, &type);
		if (static_cast<GLenum>(type) != GL_COMPUTE_SHADER)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "SHADER_TYPE should be COMPUTE_SHADER." << tcu::TestLog::EndMessage;
			glDeleteShader(sh1);
			return false;
		}

		glShaderSource(sh1, 2, cs1, NULL);
		glCompileShader(sh1);

		const GLuint sh2 = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(sh2, 1, &cs2, NULL);
		glCompileShader(sh2);

		const GLuint sh3 = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(sh3, 1, &cs3, NULL);
		glCompileShader(sh3);

		const GLuint p = glCreateProgram();
		glAttachShader(p, sh1);
		glAttachShader(p, sh2);
		glAttachShader(p, sh3);
		glLinkProgram(p);

		glDeleteShader(sh1);
		glDeleteShader(sh2);
		glDeleteShader(sh3);

		bool res = CheckProgram(p);

		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4), &vec4(0.0f)[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(p);
		glDispatchCompute(1, 1, 1);

		vec4 data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4), &data[0]);
		if (!IsEqual(data, vec4(1.0f, 2.0f, 3.0f, 4.0f)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid value!" << tcu::TestLog::EndMessage;
			res = false;
		}

		glDeleteBuffers(1, &buffer);
		glUseProgram(0);
		glDeleteProgram(p);

		return res == true ? NO_ERROR : ERROR;
	}
};

class BasicBuildSeparable : public ComputeShaderBase
{

	virtual std::string Title()
	{
		return "Building CS separable program";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that building separable CS program works as expected." NL
				  "2. Verify that program consisting from 4 strings works as expected.";
	}

	virtual std::string Method()
	{
		return NL "1. Create, compile and link CS using CreateShaderProgramv command." NL
				  "2. Dispatch and verify CS program.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	virtual long Run()
	{
		const char* const cs[4] = {
			"#version 430 core",

			NL "layout(local_size_x = 1) in;" NL "void Run();" NL "void main() {" NL "  Run();" NL "}",
			NL "layout(binding = 0, std430) buffer Output {" NL "  vec4 g_output;" NL "};" NL
			   "vec4 CalculateOutput();" NL "void Run() {" NL "  g_output = CalculateOutput();" NL "}",
			NL "vec4 CalculateOutput() {" NL "  g_output = vec4(0);" NL "  return vec4(1, 2, 3, 4);" NL "}"
		};

		const GLuint p   = glCreateShaderProgramv(GL_COMPUTE_SHADER, 4, cs);
		bool		 res = CheckProgram(p);

		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4), &vec4(0.0f)[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(p);
		glDispatchCompute(1, 1, 1);

		vec4 data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4), &data[0]);
		if (!IsEqual(data, vec4(1.0f, 2.0f, 3.0f, 4.0f)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid value!" << tcu::TestLog::EndMessage;
			res = false;
		}

		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4), &vec4(0.0f)[0]);

		GLuint pipeline;
		glGenProgramPipelines(1, &pipeline);
		glUseProgramStages(pipeline, GL_COMPUTE_SHADER_BIT, p);

		glUseProgram(0);
		glBindProgramPipeline(pipeline);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4), &data[0]);
		if (!IsEqual(data, vec4(1.0f, 2.0f, 3.0f, 4.0f)))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Invalid value!" << tcu::TestLog::EndMessage;
			res = false;
		}

		glDeleteProgramPipelines(1, &pipeline);
		glDeleteBuffers(1, &buffer);
		glDeleteProgram(p);

		return res == true ? NO_ERROR : ERROR;
	}
};

class BasicSharedSimple : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return "Shared Memory - simple usage";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that shared array of uints works as expected." NL
				  "2. Verify that shared memory written by one invocation is observable by other invocations" NL
				  "    when groupMemoryBarrier() and barrier() built-in functions are used.";
	}

	virtual std::string Method()
	{
		return NL "1. Create and dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "2. Verify results written by CS to SSBO." NL
				  "3. Repeat for several different number of work groups.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	bool RunIteration(const GLuint num_groups, bool dispatch_indirect)
	{
		const GLuint kBufferSize = 256 * num_groups;

		std::vector<GLuint> data(kBufferSize, 0xffff);
		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * kBufferSize, &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			const GLuint groups[3] = { num_groups, 1, 1 };
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(groups), groups, GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups, 1, 1);
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * kBufferSize, &data[0]);
		for (GLuint i = 0; i < kBufferSize; ++i)
		{
			if (data[i] != 1)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at index " << i << " is "
													<< data[i] << " should be 1." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 256) in;" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
			   "shared uint g_shared_data[256];" NL "void main() {" NL
			   "  g_shared_data[gl_LocalInvocationID.x] = gl_LocalInvocationIndex;" NL
			   "  groupMemoryBarrier();" // flush memory stores
			NL "  barrier();"			 // wait for all stores to finish
			NL "  g_output[gl_GlobalInvocationID.x] = 1;" NL "  if (gl_LocalInvocationIndex < 255) {" NL
			   "    const uint res = g_shared_data[gl_LocalInvocationID.x + "
			   "1];" // load data from shared memory filled by other thread
			NL "    if (res != (gl_LocalInvocationIndex + 1)) {" NL "      g_output[gl_GlobalInvocationID.x] = 0;" NL
			   "    }" NL "  }" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		if (!RunIteration(1, false))
			return ERROR;
		if (!RunIteration(8, true))
			return ERROR;
		if (!RunIteration(13, false))
			return ERROR;
		if (!RunIteration(7, true))
			return ERROR;
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicSharedStruct : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return "Shared Memory - arrays and structers";
	}

	virtual std::string Purpose()
	{
		return NL "1. Verify that vectors, matrices, structers and arrays of those can be used" NL
				  "    as a shared memory." NL
				  "2. Verify that shared memory can be indexed with constant values, built-in" NL
				  "    variables and dynamic expressions." NL
				  "3. Verify that memoryBarrierAtomicCounter(), memoryBarrierImage(), memoryBarrier()," NL
				  "     memoryBarrierBuffer() and memoryBarrierShared() built-in functions are accepted" NL
				  "     by the GLSL compiler.";
	}

	virtual std::string Method()
	{
		return NL "1. Create and dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "2. Verify results written by CS to SSBO.";
	}

	virtual std::string PassCriteria()
	{
		return "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	bool RunIteration(bool dispatch_indirect)
	{
		const GLuint kBufferSize = 256;

		std::vector<vec4> data(kBufferSize);
		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * kBufferSize, &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			const GLuint groups[3] = { 1, 1, 1 };
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(groups), groups, GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(1, 1, 1);
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(vec4) * kBufferSize, &data[0]);
		for (GLuint i = 0; i < kBufferSize; ++i)
		{
			if (!IsEqual(data[i], vec4(static_cast<float>(i))))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Invalid data at index " << i << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs = NL
			"layout(local_size_x = 128) in;" NL "layout(std430) buffer Output {" NL "  vec4 g_output[256];" NL "};" NL
			"struct SubData {" NL "  mat2x4 data;" NL "};" NL "struct Data {" NL "  uint index;" NL "  vec3 data0;" NL
			"  SubData data1;" NL "};" NL "shared Data g_shared_data[256];" NL "shared int g_shared_buf[2];" NL
			"void main() {" NL "  if (gl_LocalInvocationID.x == 0) {" NL "    g_shared_buf[1] = 1;" NL
			"    g_shared_buf[1 + gl_LocalInvocationID.x] = 0;" NL "    g_shared_buf[0] = 128;" NL
			"    g_output[0] = vec4(g_shared_buf[1]);" NL "    g_output[128] = vec4(g_shared_buf[0]);" NL
			"    memoryBarrierBuffer();" // note: this call is not needed here, just check if compiler accepts it
			NL "  } else {" NL "    const uint index = gl_LocalInvocationIndex;" NL
			"    g_shared_data[index].index = index;" NL "    g_shared_data[index + 128].index = index + 128;" NL
			"    g_shared_data[index].data1.data = mat2x4(0.0);" NL
			"    g_shared_data[index + 128].data1.data = mat2x4(0.0);" NL
			"    g_output[index] = vec4(g_shared_data[index].index);" // load data from shared memory
			NL "    g_output[index + 128] = vec4(g_shared_data[index + 128].index);" NL
			"    memoryBarrierShared();" // note: this call is not needed here, just check if compiler accepts it
			NL "  }" NL "  memoryBarrierAtomicCounter();" NL "  memoryBarrierImage();" NL
			"  memoryBarrier();" // note: these calls are not needed here, just check if compiler accepts them
			NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		if (!RunIteration(false))
			return ERROR;
		if (!RunIteration(true))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicDispatchIndirect : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "DispatchComputeIndirect command";
	}

	virtual std::string Purpose()
	{
		return NL
			"1. Verify that DispatchComputeIndirect command works as described in the OpenGL specification." NL
			"2. Verify that <offset> parameter is correctly applied." NL
			"3. Verify that updating dispatch buffer with different methods (BufferData, BufferSubData, MapBuffer)" NL
			"    just before DispatchComputeIndirect call works as expected." NL
			"4. Verify that GL_DISPATCH_INDIRECT_BUFFER_BINDING binding point is set correctly.";
	}

	virtual std::string Method()
	{
		return NL
			"1. Create CS and dispatch indirect buffer." NL "2. Dispatch CS with DispatchComputeIndirect command." NL
			"3. Update dispatch indirect buffer." NL
			"4. Repeat several times updating dispatch buffer with different methods and changing <offset> parameter.";
	}

	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer[2];

	bool RunIteration(GLintptr offset, GLuint buffer_size)
	{
		std::vector<GLuint> data(buffer_size);
		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * buffer_size, &data[0], GL_DYNAMIC_DRAW);

		glDispatchComputeIndirect(offset);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * buffer_size, &data[0]);
		for (GLuint i = 0; i < buffer_size; ++i)
		{
			if (data[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at index " << i << " is "
													<< data[i] << " should be " << i << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}

	bool CheckBinding(GLuint expected)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		glGetIntegerv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &i);
		if (static_cast<GLuint>(i) != expected)
		{
			return false;
		}
		glGetInteger64v(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &i64);
		if (static_cast<GLuint>(i64) != expected)
		{
			return false;
		}
		glGetFloatv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &f);
		if (static_cast<GLuint>(f) != expected)
		{
			return false;
		}
		glGetDoublev(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &d);
		if (static_cast<GLuint>(d) != expected)
		{
			return false;
		}
		glGetBooleanv(GL_DISPATCH_INDIRECT_BUFFER_BINDING, &b);
		if (b != (expected != 0 ? GL_TRUE : GL_FALSE))
		{
			return false;
		}

		return true;
	}

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		memset(m_dispatch_buffer, 0, sizeof(m_dispatch_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
			   "uniform uvec3 g_global_size;" NL "void main() {" NL
			   "  const uint global_index = gl_GlobalInvocationID.x +" NL
			   "                            gl_GlobalInvocationID.y * g_global_size.x +" NL
			   "                            gl_GlobalInvocationID.z * g_global_size.x * g_global_size.y;" NL
			   "  if (gl_NumWorkGroups != g_global_size) {" NL "    g_output[global_index] = 0xffff;" NL
			   "    return;" NL "  }" NL "  g_output[global_index] = global_index;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		if (!CheckBinding(0))
			return ERROR;

		glGenBuffers(2, m_dispatch_buffer);

		const GLuint data[]  = { 1, 2, 3, 4, 5, 6, 7, 8 };
		const GLuint data2[] = { 3, 1, 4, 4 };

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer[0]);
		glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data), data, GL_STREAM_DRAW);
		if (!CheckBinding(m_dispatch_buffer[0]))
			return ERROR;

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer[1]);
		glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data2), data2, GL_STREAM_READ);
		if (!CheckBinding(m_dispatch_buffer[1]))
			return ERROR;

		glUseProgram(m_program);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer[0]);

		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 1, 2, 3);
		if (!RunIteration(0, 6))
			return ERROR;

		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 2, 3, 4);
		if (!RunIteration(4, 24))
			return ERROR;

		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 4, 5, 6);
		if (!RunIteration(12, 120))
			return ERROR;

		glBufferSubData(GL_DISPATCH_INDIRECT_BUFFER, 20, 12, data);
		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 1, 2, 3);
		if (!RunIteration(20, 6))
			return ERROR;

		GLuint* ptr = static_cast<GLuint*>(glMapBuffer(GL_DISPATCH_INDIRECT_BUFFER, GL_WRITE_ONLY));
		*ptr++		= 4;
		*ptr++		= 4;
		*ptr++		= 4;
		glUnmapBuffer(GL_DISPATCH_INDIRECT_BUFFER);

		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 4, 4, 4);
		if (!RunIteration(0, 64))
			return ERROR;

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer[1]);

		glUniform3ui(glGetUniformLocation(m_program, "g_global_size"), 1, 4, 4);
		if (!RunIteration(4, 16))
			return ERROR;

		glDeleteBuffers(2, m_dispatch_buffer);
		memset(m_dispatch_buffer, 0, sizeof(m_dispatch_buffer));

		if (!CheckBinding(0))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(2, m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicSSOComputePipeline : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Separable CS Programs - Compute and non-compute stages (1)";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that compute and non-compute stages can be attached to one pipeline object." NL
				  "2. Verify that DrawArrays and ComputeDispatch commands works as expected in this case.";
	}
	virtual std::string Method()
	{
		return NL "1. Create VS, FS and CS. Attach all created stages to one pipeline object." NL
				  "2. Bind pipeline object." NL "3. Invoke compute stage with DispatchCompute commmand." NL
				  "4. Issue MemoryBarrier command." NL
				  "5. Issue DrawArrays command which uses data written by the compute stage." NL "6. Verify result.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_vsp, m_fsp, m_csp;
	GLuint m_storage_buffer;
	GLuint m_vertex_array;
	GLuint m_pipeline;

	virtual long Setup()
	{
		m_vsp = m_fsp = m_csp = 0;
		m_storage_buffer	  = 0;
		m_vertex_array		  = 0;
		m_pipeline			  = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "layout(std430) buffer Output {" NL "  vec4 g_output[4];" NL "};" NL
			   "void main() {" NL "  const vec2 quad[4] = { vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1) };" NL
			   "  g_output[gl_GlobalInvocationID.x] = vec4(quad[gl_GlobalInvocationID.x], 0, 1);" NL "}";

		m_csp = CreateComputeProgram(glsl_cs);
		glProgramParameteri(m_csp, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glLinkProgram(m_csp);
		if (!CheckProgram(m_csp))
			return ERROR;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL
			   "void main() {" NL "  gl_Position = i_position;" NL "}";
		m_vsp = BuildShaderProgram(GL_VERTEX_SHADER, glsl_vs);
		if (!CheckProgram(m_vsp))
			return ERROR;

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL "  o_color = vec4(0, 1, 0, 1);" NL "}";
		m_fsp = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs);
		if (!CheckProgram(m_fsp))
			return ERROR;

		glGenProgramPipelines(1, &m_pipeline);
		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_fsp);
		glUseProgramStages(m_pipeline, GL_COMPUTE_SHADER_BIT, m_csp);

		glGenBuffers(1, &m_storage_buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_storage_buffer);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glBindProgramPipeline(m_pipeline);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glDispatchCompute(1, 1, 1);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vertex_array);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteProgram(m_csp);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteProgramPipelines(1, &m_pipeline);
		return NO_ERROR;
	}
};

class BasicSSOCase2 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Separable CS Programs - Compute and non-compute stages (2)";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that data computed by the compute stage is visible to non-compute stage after "
				  "MemoryBarrier command." NL "2. Verify that ProgramParameteri(program, GL_PROGRAM_SEPARABLE, "
				  "GL_TRUE) command works correctly for CS." NL
				  "3. Verify that gl_WorkGroupSize built-in variable is a contant and can be used as an array size.";
	}
	virtual std::string Method()
	{
		return NL "1. Create VS, FS and CS. Attach all created stages to one pipeline object." NL
				  "2. Bind pipeline object." NL "3. Invoke compute stage with DispatchCompute commmand." NL
				  "4. Issue MemoryBarrier command." NL
				  "5. Issue DrawArrays command which uses data written to the buffer object by the compute stage." NL
				  "6. Verify result.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program_ab;
	GLuint m_program_c;
	GLuint m_pipeline;
	GLuint m_storage_buffer;
	GLuint m_vao;

	virtual long Setup()
	{
		m_program_ab	 = 0;
		m_program_c		 = 0;
		m_pipeline		 = 0;
		m_storage_buffer = 0;
		m_vao			 = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		GLint res;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &res);
		if (res <= 0)
		{
			OutputNotSupported("GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS <= 0");
			return NO_ERROR;
		}

		const char* const glsl_a =
			"#version 430 core" NL "layout(binding = 1, std430) buffer Input {" NL "  vec2 g_input[4];" NL "};" NL
			"out StageData {" NL "  vec3 color;" NL "} g_vs_out;" NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL
			"};" NL "void main() {" NL "  gl_Position = vec4(g_input[gl_VertexID], 0, 1);" NL
			"  g_vs_out.color = vec3(0, 1, 0);" NL "}";

		const char* const glsl_b =
			"#version 430 core" NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL
			"layout(location = 0) out vec4 g_color;" NL "void main() {" NL "  g_color = vec4(g_fs_in.color, 1);" NL "}";

		const char* const glsl_c =
			"#version 430 core" NL "layout(local_size_x = 4) in;" NL "layout(binding = 1, std430) buffer Output {" NL
			"  vec2 g_output[gl_WorkGroupSize.x];" NL "};" NL "void main() {" NL
			"  if (gl_GlobalInvocationID.x == 0) {" NL "    g_output[0] = vec2(-0.8, -0.8);" NL
			"  } else if (gl_GlobalInvocationID.x == 1) {" NL "    g_output[1] = vec2(0.8, -0.8);" NL
			"  } else if (gl_GlobalInvocationID.x == 2) {" NL "    g_output[2] = vec2(-0.8, 0.8);" NL
			"  } else if (gl_GlobalInvocationID.x == 3) {" NL "    g_output[3] = vec2(0.8, 0.8);" NL "  }" NL "}";

		m_program_ab = glCreateProgram();
		GLuint sh	= glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(m_program_ab, sh);
		glDeleteShader(sh);
		glShaderSource(sh, 1, &glsl_a, NULL);
		glCompileShader(sh);

		sh = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(m_program_ab, sh);
		glDeleteShader(sh);
		glShaderSource(sh, 1, &glsl_b, NULL);
		glCompileShader(sh);

		glProgramParameteri(m_program_ab, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glLinkProgram(m_program_ab);

		m_program_c = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &glsl_c);
		glGenVertexArrays(1, &m_vao);
		glGenProgramPipelines(1, &m_pipeline);
		glUseProgramStages(m_pipeline, GL_ALL_SHADER_BITS, m_program_ab);
		glUseProgramStages(m_pipeline, GL_COMPUTE_SHADER_BIT, m_program_c);

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec2) * 4, NULL, GL_STREAM_DRAW);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindProgramPipeline(m_pipeline);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (getWindowWidth() < 500 &&
			!ValidateReadBufferCenteredQuad(getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(m_program_ab);
		glDeleteProgram(m_program_c);
		glDeleteProgramPipelines(1, &m_pipeline);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};

class BasicSSOCase3 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Separable CS Programs - Compute stage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that compute shader stage selected with UseProgram command has precedence" NL
				  "over compute shader stage selected with BindProgramPipeline command.";
	}
	virtual std::string Method()
	{
		return NL "1. Create CS0 with CreateProgram command. Create CS1 with CreateShaderProgramv command." NL
				  "2. Verify that CS program selected with UseProgram is dispatched even if there is active" NL
				  "    compute stage bound by BindProgramPipeline.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program_a;
	GLuint m_program_b;
	GLuint m_pipeline;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program_a		 = 0;
		m_program_b		 = 0;
		m_pipeline		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_a =
			"#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(binding = 3, std430) buffer Output {" NL
			"  int g_output;" NL "};" NL "void main() {" NL "  g_output = 1;" NL "}";

		const char* const glsl_b =
			"#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(binding = 3, std430) buffer Output {" NL
			"  int g_output;" NL "};" NL "void main() {" NL "  g_output = 2;" NL "}";

		/* create program A */
		{
			m_program_a = glCreateProgram();
			GLuint sh   = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(m_program_a, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &glsl_a, NULL);
			glCompileShader(sh);
			glProgramParameteri(m_program_a, GL_PROGRAM_SEPARABLE, GL_TRUE);
			glLinkProgram(m_program_a);
		}
		m_program_b = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &glsl_b);

		/* create storage buffer */
		{
			int data = 0;
			glGenBuffers(1, &m_storage_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int), &data, GL_STREAM_READ);
		}

		glGenProgramPipelines(1, &m_pipeline);
		glUseProgramStages(m_pipeline, GL_ALL_SHADER_BITS, m_program_b);

		glUseProgram(m_program_a);
		glBindProgramPipeline(m_pipeline);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glUseProgram(0);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &data);
			if (data != 2)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 2." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glUseProgram(m_program_b);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &data);
			if (data != 2)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 2." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glUseProgram(0);
		glUseProgramStages(m_pipeline, GL_COMPUTE_SHADER_BIT, m_program_a);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glDeleteProgram(m_program_a);
		glDeleteProgram(m_program_b);
		glDeleteProgramPipelines(1, &m_pipeline);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class BasicAtomicCase1 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Atomic functions";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that atomicAdd function works as expected with int and uint parameters." NL
				  "2. Verify that shared memory can be used with atomic functions." NL
				  "3. Verify that groupMemoryBarrier() and barrier() built-in functions work as expected.";
	}
	virtual std::string Method()
	{
		return NL "1. Use shared memory as a 'counter' with-in one CS work group." NL
				  "2. Each shader invocation increments/decrements 'counter' value using atomicAdd function." NL
				  "3. Values returned by atomicAdd function are written to SSBO." NL
				  "4. Verify SSBO content (values from 0 to 7 should be written).";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 8) in;" NL "layout(std430, binding = 0) buffer Output {" NL
			   "  uint g_add_output[8];" NL "  int g_sub_output[8];" NL "};" NL "shared uint g_add_value;" NL
			   "shared int g_sub_value;" NL "void main() {" NL "  if (gl_LocalInvocationIndex == 0) {" NL
			   "    g_add_value = 0u;" NL "    g_sub_value = 7;" NL "  }" NL
			   "  g_add_output[gl_LocalInvocationIndex] = 0u;" NL "  g_sub_output[gl_LocalInvocationIndex] = 0;" NL
			   "  groupMemoryBarrier();" NL "  barrier();" NL
			   "  g_add_output[gl_LocalInvocationIndex] = atomicAdd(g_add_value, 1u);" NL
			   "  g_sub_output[gl_LocalInvocationIndex] = atomicAdd(g_sub_value, -1);" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 16 * sizeof(int), NULL, GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		std::vector<int> data(8);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int) * 8, &data[0]);
		std::sort(data.begin(), data.end());
		for (int i = 0; i < 8; ++i)
		{
			if (data[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at index " << i << " is "
													<< data[i] << " should be " << i << "." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * 8, sizeof(int) * 8, &data[0]);
		std::sort(data.begin(), data.end());
		for (int i = 0; i < 8; ++i)
		{
			if (data[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at index " << i << " is "
													<< data[i] << " should be " << i << "." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class BasicAtomicCase2 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Atomic functions - buffer variables";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that all atomic functions (atomicExchange, atomicMin, atomicMax," NL
				  "    atomicAnd, atomicOr, atomicXor and atomicCompSwap) works as expected with buffer variables." NL
				  "2. Verify that atomic functions work with parameters being constants and" NL
				  "    with parameters being uniforms." NL
				  "3. Verify that barrier() built-in function can be used in a control flow.";
	}
	virtual std::string Method()
	{
		return NL "1. Create CS that uses all atomic functions. Values returned by the atomic functions are written to "
				  "SSBO." NL "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Verify SSBO content." NL
				  "4. Repeat for different number of work groups and different work group sizes.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer[2];
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size, const uvec3& num_groups)
	{
		const uvec3		  global_size = local_size * num_groups;
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z() << ") in;" NL "const uvec3 kGlobalSize = uvec3(" << global_size.x()
		   << ", " << global_size.y() << ", " << global_size.z()
		   << ");" NL "layout(std430, binding = 0) buffer OutputU {" NL "  uint g_uint_out["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "};" NL "layout(std430, binding = 1) buffer OutputI {" NL "  int data["
		   << global_size.x() * global_size.y() * global_size.z()
		   << "];" NL "} g_int_out;" NL
			  "uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			  "  const uint global_index = gl_GlobalInvocationID.x +" NL
			  "                            gl_GlobalInvocationID.y * kGlobalSize.x +" NL
			  "                            gl_GlobalInvocationID.z * kGlobalSize.x * kGlobalSize.y;" NL
			  "  atomicExchange(g_uint_out[global_index], g_uint_value[0]);" NL
			  "  atomicMin(g_uint_out[global_index], g_uint_value[1]);" NL
			  "  atomicMax(g_uint_out[global_index], g_uint_value[2]);" NL
			  "  atomicAnd(g_uint_out[global_index], g_uint_value[3]);" NL
			  "  atomicOr(g_uint_out[global_index], g_uint_value[4]);" NL "  if (g_uint_value[0] > 0u) {" NL
			  "    barrier();" // not needed here, just check if compiler accepts it in a control flow
			NL "    atomicXor(g_uint_out[global_index], g_uint_value[5]);" NL "  }" NL
			  "  atomicCompSwap(g_uint_out[global_index], g_uint_value[6], g_uint_value[7]);" NL NL
			  "  atomicExchange(g_int_out.data[global_index], 3);" NL "  atomicMin(g_int_out.data[global_index], 1);" NL
			  "  atomicMax(g_int_out.data[global_index], 2);" NL "  atomicAnd(g_int_out.data[global_index], 0x1);" NL
			  "  atomicOr(g_int_out.data[global_index], 0x3);" NL "  atomicXor(g_int_out.data[global_index], 0x1);" NL
			  "  atomicCompSwap(g_int_out.data[global_index], 0x2, 0x7);" NL "}";
		return ss.str();
	}
	bool RunIteration(const uvec3& local_size, const uvec3& num_groups, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size, num_groups));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		const GLuint kBufferSize =
			local_size.x() * num_groups.x() * local_size.y() * num_groups.y() * local_size.z() * num_groups.z();

		if (m_storage_buffer[0] == 0)
			glGenBuffers(2, m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * kBufferSize, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLint) * kBufferSize, NULL, GL_DYNAMIC_DRAW);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(num_groups.x(), num_groups.y(), num_groups.z());
		}
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		std::vector<GLuint> udata(kBufferSize);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * kBufferSize, &udata[0]);
		for (GLuint i = 0; i < kBufferSize; ++i)
		{
			if (udata[i] != 7)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
													<< udata[i] << " should be 7." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		std::vector<GLint> idata(kBufferSize);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLint) * kBufferSize, &idata[0]);
		for (GLint i = 0; i < static_cast<GLint>(kBufferSize); ++i)
		{
			if (idata[i] != 7)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at index " << i << " is "
													<< idata[i] << " should be 7." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}
	virtual long Setup()
	{
		m_program			= 0;
		m_storage_buffer[0] = m_storage_buffer[1] = 0;
		m_dispatch_buffer						  = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		if (!RunIteration(uvec3(64, 1, 1), uvec3(8, 1, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 64), uvec3(1, 5, 2), true))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 4), uvec3(2, 2, 2), false))
			return ERROR;
		if (!RunIteration(uvec3(3, 2, 1), uvec3(1, 2, 3), true))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 2), uvec3(2, 4, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 7), uvec3(2, 1, 4), true))
			return ERROR;
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class BasicAtomicCase3 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Atomic functions - shared variables";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that all atomic functions (atomicExchange, atomicMin, atomicMax," NL
				  "    atomicAnd, atomicOr, atomicXor and atomicCompSwap) works as expected with shared variables." NL
				  "2. Verify that atomic functions work with parameters being constants and" NL
				  "    with parameters being uniforms." NL
				  "3. Verify that atomic functions can be used in a control flow.";
	}
	virtual std::string Method()
	{
		return NL "1. Create CS that uses all atomic functions. Values returned by the atomic functions are written to "
				  "SSBO." NL "2. Dispatch CS with DispatchCompute and DispatchComputeIndirect commands." NL
				  "3. Verify SSBO content." NL
				  "4. Repeat for different number of work groups and different work group sizes.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	std::string GenSource(const uvec3& local_size)
	{
		std::stringstream ss;
		ss << NL "layout(local_size_x = " << local_size.x() << ", local_size_y = " << local_size.y()
		   << ", local_size_z = " << local_size.z()
		   << ") in;" NL "layout(std430, binding = 0) buffer Output {" NL "  uint g_uint_out["
		   << local_size.x() * local_size.y() * local_size.z() << "];" NL "  int g_int_out["
		   << local_size.x() * local_size.y() * local_size.z() << "];" NL "};" NL "shared uint g_shared_uint["
		   << local_size.x() * local_size.y() * local_size.z() << "];" NL "shared int g_shared_int["
		   << local_size.x() * local_size.y() * local_size.z()
		   << "];" NL "uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL
			  "void main() {" NL "  atomicExchange(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[0]);" NL
			  "  atomicMin(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[1]);" NL
			  "  atomicMax(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[2]);" NL
			  "  atomicAnd(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[3]);" NL
			  "  atomicOr(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[4]);" NL
			  "  atomicXor(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[5]);" NL
			  "  atomicCompSwap(g_shared_uint[gl_LocalInvocationIndex], g_uint_value[6], g_uint_value[7]);" NL NL
			  "  atomicExchange(g_shared_int[gl_LocalInvocationIndex], 3);" NL
			  "  atomicMin(g_shared_int[gl_LocalInvocationIndex], 1);" NL
			  "  atomicMax(g_shared_int[gl_LocalInvocationIndex], 2);" NL
			  "  atomicAnd(g_shared_int[gl_LocalInvocationIndex], 0x1);" NL "  if (g_uint_value[1] > 0u) {" NL
			  "    atomicOr(g_shared_int[gl_LocalInvocationIndex], 0x3);" NL
			  "    atomicXor(g_shared_int[gl_LocalInvocationIndex], 0x1);" NL
			  "    atomicCompSwap(g_shared_int[gl_LocalInvocationIndex], 0x2, 0x7);" NL "  }" NL NL
			  "  g_uint_out[gl_LocalInvocationIndex] = g_shared_uint[gl_LocalInvocationIndex];" NL
			  "  g_int_out[gl_LocalInvocationIndex] = g_shared_int[gl_LocalInvocationIndex];" NL "}";
		return ss.str();
	}
	bool RunIteration(const uvec3& local_size, bool dispatch_indirect)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateComputeProgram(GenSource(local_size));
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		const GLuint kBufferSize = local_size.x() * local_size.y() * local_size.z();

		if (m_storage_buffer == 0)
			glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * kBufferSize * 2, NULL, GL_DYNAMIC_DRAW);

		glUseProgram(m_program);
		if (dispatch_indirect)
		{
			const GLuint num_groups[3] = { 1, 1, 1 };
			if (m_dispatch_buffer == 0)
				glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), &num_groups[0], GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
		}
		else
		{
			glDispatchCompute(1, 1, 1);
		}
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		std::vector<GLuint> udata(kBufferSize);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * kBufferSize, &udata[0]);
		for (GLuint i = 0; i < kBufferSize; ++i)
		{
			if (udata[i] != 7)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
													<< udata[i] << " should be 7." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		std::vector<GLint> idata(kBufferSize);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * kBufferSize, sizeof(GLint) * kBufferSize,
						   &idata[0]);
		for (GLint i = 0; i < static_cast<GLint>(kBufferSize); ++i)
		{
			if (idata[i] != 7)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
													<< idata[i] << " should be 7." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		return true;
	}
	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		if (!RunIteration(uvec3(64, 1, 1), false))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 64), true))
			return ERROR;
		if (!RunIteration(uvec3(1, 1, 4), false))
			return ERROR;
		if (!RunIteration(uvec3(3, 2, 1), true))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 2), false))
			return ERROR;
		if (!RunIteration(uvec3(2, 4, 7), true))
			return ERROR;
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class AdvancedCopyImage : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Copy Image";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that copying two textures using CS works as expected.";
	}
	virtual std::string Method()
	{
		return NL "Use shader image load and store operations to copy two textures in the CS.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_texture[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_texture, 0, sizeof(m_texture));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs = NL "#define TILE_WIDTH 16" NL "#define TILE_HEIGHT 16" NL
									   "const ivec2 kTileSize = ivec2(TILE_WIDTH, TILE_HEIGHT);" NL NL
									   "layout(binding = 0, rgba8) uniform image2D g_input_image;" NL
									   "layout(binding = 1, rgba8) uniform image2D g_output_image;" NL	NL
									   "layout(local_size_x=TILE_WIDTH, local_size_y=TILE_HEIGHT) in;" NL NL
									   "void main() {" NL "  const ivec2 tile_xy = ivec2(gl_WorkGroupID);" NL
									   "  const ivec2 thread_xy = ivec2(gl_LocalInvocationID);" NL
									   "  const ivec2 pixel_xy = tile_xy * kTileSize + thread_xy;" NL NL
									   "  vec4 pixel = imageLoad(g_input_image, pixel_xy);" NL
									   "  imageStore(g_output_image, pixel_xy, pixel);" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		std::vector<GLubyte> in_image(64 * 64 * 4, 0x0f);
		std::vector<GLubyte> out_image(64 * 64 * 4, 0x00);

		glGenTextures(2, m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &in_image[0]);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, &out_image[0]);

		glUseProgram(m_program);
		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glDispatchCompute(5, 4,
						  1); // 5 is on purpose, to ensure that out of bounds image load and stores have no effect
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		std::vector<GLubyte> data(64 * 64 * 4);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		for (std::size_t i = 0; i < data.size(); ++i)
		{
			if (getWindowWidth() > 100 && data[i] != 0x0f)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data at index " << i << " is " << data[i] << " should be " << 0x0f
					<< "." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteTextures(2, m_texture);
		return NO_ERROR;
	}
};

class AdvancedPipelinePreVS : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "CS as an additional pipeline stage - Before VS (1)";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that CS which runs just before VS and modifies VBO content works as expected.";
	}
	virtual std::string Method()
	{
		return NL "1. Prepare VBO and VAO for a drawing operation." NL "2. Run CS to modify existing VBO content." NL
				  "3. Issue MemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT) command." NL
				  "4. Issue draw call command." NL "5. Verify that the framebuffer content is as expected.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program[2];
	GLuint m_vertex_buffer;
	GLuint m_vertex_array;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		m_vertex_buffer = 0;
		m_vertex_array  = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "struct Vertex {" NL "  vec4 position;" NL "  vec4 color;" NL "};" NL
			   "layout(binding = 0, std430) buffer VertexBuffer {" NL "  Vertex g_vertex[];" NL "};" NL
			   "uniform float g_scale = 0.8;" NL "void main() {" NL
			   "  g_vertex[gl_GlobalInvocationID.x].position.xyz *= g_scale;" NL
			   "  g_vertex[gl_GlobalInvocationID.x].color *= vec4(0, 1, 0, 1);" NL "}";
		m_program[0] = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "layout(location = 1) in vec4 g_color;" NL
			   "out StageData {" NL "  vec4 color;" NL "} g_vs_out;" NL "void main() {" NL
			   "  gl_Position = g_position;" NL "  g_vs_out.color = g_color;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec4 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_color;" NL
			   "void main() {" NL "  g_color = g_fs_in.color;" NL "}";
		m_program[1] = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;

		/* vertex buffer */
		{
			const float data[] = { -1, -1, 0, 1, 1, 1, 1, 1, 1, -1, 0, 1, 1, 1, 1, 1,
								   -1, 1,  0, 1, 1, 1, 1, 1, 1, 1,  0, 1, 1, 1, 1, 1 };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), reinterpret_cast<void*>(sizeof(vec4)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_vertex_buffer);
		glUseProgram(m_program[0]);
		glDispatchCompute(1, 1, 1);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glBindVertexArray(m_vertex_array);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

		if (getWindowWidth() < 500 &&
			!ValidateReadBufferCenteredQuad(getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		for (int i = 0; i < 2; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedPipelineGenDrawCommands : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "CS as an additional pipeline stage - Before VS (2)";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that a complex scenario where CS is used to generate drawing commands" NL
				  "and write them to a draw indirect buffer works as expected. This is a practial usage of CS." NL
				  "CS is used for culling objects which are outside of the viewing frustum.";
	}
	virtual std::string Method()
	{
		return NL "1. Run CS which will generate four sets of draw call parameters and write them to the draw indirect "
				  "buffer." NL "2. One set of draw call parameters will be: 0, 0, 0, 0" NL
				  "    (which means that an object is outside of the viewing frustum and should not be drawn)." NL
				  "3. Issue MemoryBarrier(GL_COMMAND_BARRIER_BIT) command." NL
				  "4. Issue four draw indirect commands." NL "5. Verify that the framebuffer content is as expected.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program[2];
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLuint m_vertex_array;
	GLuint m_draw_buffer;
	GLuint m_object_buffer;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		m_vertex_buffer = 0;
		m_index_buffer  = 0;
		m_vertex_array  = 0;
		m_draw_buffer   = 0;
		m_object_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		GLint res;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &res);
		if (res <= 0)
		{
			OutputNotSupported("GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS <= 0");
			return NO_ERROR;
		}

		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "struct DrawCommand {" NL "  uint count;" NL
			   "  uint instance_count;" NL "  uint first_index;" NL "  int base_vertex;" NL "  uint base_instance;" NL
			   "};" NL "layout(std430) buffer;" NL "layout(binding = 0) readonly buffer ObjectBuffer {" NL
			   "  mat4 transform[4];" NL "  uint count[4];" NL "  uint first_index[4];" NL "} g_objects;" NL
			   "layout(binding = 1) writeonly buffer DrawCommandBuffer {" NL "  DrawCommand g_command[4];" NL "};" NL
			   "bool IsObjectVisible(uint id) {" NL
			   "  if (g_objects.transform[id][3].x < -1.0 || g_objects.transform[id][3].x > 1.0) return false;" NL
			   "  if (g_objects.transform[id][3][1] < -1.0 || g_objects.transform[id][3][1] > 1.0) return false;" NL
			   "  if (g_objects.transform[id][3][2] < -1.0 || g_objects.transform[id][3].z > 1.0) return false;" NL
			   "  return true;" NL "}" NL "void main() {" NL "  uint id = gl_GlobalInvocationID.x;" NL
			   "  g_command[id].count = 0;" NL "  g_command[id].instance_count = 0;" NL
			   "  g_command[id].first_index = 0;" NL "  g_command[id].base_vertex = 0;" NL
			   "  g_command[id].base_instance = 0;" NL "  if (IsObjectVisible(id)) {" NL
			   "    g_command[id].count = g_objects.count[id];" NL "    g_command[id].instance_count = 1;" NL
			   "    g_command[id].first_index = g_objects.first_index[id];" NL "  }" NL "}";
		m_program[0] = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "layout(location = 1) in vec3 g_color;" NL
			   "out StageData {" NL "  vec3 color;" NL "} g_vs_out;" NL
			   "layout(binding = 0, std430) buffer ObjectBuffer {" NL "  mat4 transform[4];" NL "  uint count[4];" NL
			   "  uint first_index[4];" NL "} g_objects;" NL "uniform int g_object_id;" NL "void main() {" NL
			   "  gl_Position = g_objects.transform[g_object_id] * g_position;" NL "  g_vs_out.color = g_color;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_color;" NL
			   "void main() {" NL "  g_color = vec4(g_fs_in.color, 1);" NL "}";
		m_program[1] = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;
		glViewport(0, 0, 100, 100);

		/* object buffer */
		{
			struct
			{
				mat4   transform[4];
				GLuint count[4];
				GLuint first_index[4];
			} data = {
				{ tcu::translationMatrix(vec3(-1.5f, -0.5f, 0.0f)), tcu::translationMatrix(vec3(0.5f, -0.5f, 0.0f)),
				  tcu::translationMatrix(vec3(-0.5f, 0.5f, 0.0f)), tcu::translationMatrix(vec3(0.5f, 0.5f, 0.0f)) },
				{ 4, 4, 4, 4 },
				{ 0, 4, 8, 12 }
			};
			glGenBuffers(1, &m_object_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_object_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}
		/* vertex buffer */
		{
			const vec3 data[] = { vec3(-0.4f, -0.4f, 0.0f), vec3(1, 0, 0), vec3(0.4f, -0.4f, 0.0f), vec3(1, 0, 0),
								  vec3(-0.4f, 0.4f, 0.0f),  vec3(1, 0, 0), vec3(0.4f, 0.4f, 0.0f),  vec3(1, 0, 0),
								  vec3(-0.4f, -0.4f, 0.0f), vec3(0, 1, 0), vec3(0.4f, -0.4f, 0.0f), vec3(0, 1, 0),
								  vec3(-0.4f, 0.4f, 0.0f),  vec3(0, 1, 0), vec3(0.4f, 0.4f, 0.0f),  vec3(0, 1, 0),
								  vec3(-0.4f, -0.4f, 0.0f), vec3(0, 0, 1), vec3(0.4f, -0.4f, 0.0f), vec3(0, 0, 1),
								  vec3(-0.4f, 0.4f, 0.0f),  vec3(0, 0, 1), vec3(0.4f, 0.4f, 0.0f),  vec3(0, 0, 1),
								  vec3(-0.4f, -0.4f, 0.0f), vec3(1, 1, 0), vec3(0.4f, -0.4f, 0.0f), vec3(1, 1, 0),
								  vec3(-0.4f, 0.4f, 0.0f),  vec3(1, 1, 0), vec3(0.4f, 0.4f, 0.0f),  vec3(1, 1, 0) };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		/* index buffer */
		{
			const GLushort data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
			glGenBuffers(1, &m_index_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		glGenBuffers(1, &m_draw_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_buffer);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, 4 * sizeof(GLuint) * 5, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), 0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), reinterpret_cast<void*>(sizeof(vec3)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBindVertexArray(0);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_draw_buffer);
		glUseProgram(m_program[0]);
		glDispatchCompute(1, 1, 1);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_buffer);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
		/* draw (CPU draw calls dispatch, could be done by the GPU with ARB_multi_draw_indirect) */
		{
			GLsizeiptr offset = 0;
			for (int i = 0; i < 4; ++i)
			{
				glUniform1i(glGetUniformLocation(m_program[1], "g_object_id"), i);
				glDrawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(offset));
				offset += 5 * sizeof(GLuint);
			}
		}
		if (getWindowWidth() >= 100 && getWindowHeight() >= 100 &&
			!ValidateWindow4Quads(vec3(0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		for (int i = 0; i < 2; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteBuffers(1, &m_index_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteBuffers(1, &m_draw_buffer);
		glDeleteBuffers(1, &m_object_buffer);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		return NO_ERROR;
	}
};

class AdvancedPipelineComputeChain : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Compute Chain";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that dispatching several compute kernels that work in a sequence" NL
				  "    with a common set of resources works as expected." NL
				  "2. Verify that indexing nested structures with built-in variables work as expected." NL
				  "3. Verify that two kernels can write to the same resource without MemoryBarrier" NL
				  "    command if target regions of memory do not overlap.";
	}
	virtual std::string Method()
	{
		return NL "1. Create a set of GPU resources (buffers, images, atomic counters)." NL
				  "2. Dispatch Kernel0 that write to these resources." NL "3. Issue MemoryBarrier command." NL
				  "4. Dispatch Kernel1 that read/write from/to these resources." NL "5. Issue MemoryBarrier command." NL
				  "6. Dispatch Kernel2 that read/write from/to these resources." NL
				  "7. Verify that content of all resources is as expected.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program[3];
	GLuint m_storage_buffer[4];
	GLuint m_counter_buffer;
	GLuint m_texture;
	GLuint m_fbo;

	std::string Common()
	{
		return NL "struct S0 {" NL "  int m0[8];" NL "};" NL "struct S1 {" NL "  S0 m0[8];" NL "};" NL
				  "layout(binding = 0, std430) buffer Buffer0 {" NL "  int m0[5];" NL "  S1 m1[8];" NL "} g_buffer0;" NL
				  "layout(binding = 1, std430) buffer Buffer1 {" NL "  uint data[8];" NL "} g_buffer1;" NL
				  "layout(binding = 2, std430) buffer Buffer2 {" NL "  int data[256];" NL "} g_buffer2;" NL
				  "layout(binding = 3, std430) buffer Buffer3 {" NL "  int data[256];" NL "} g_buffer3;" NL
				  "layout(binding = 4, std430) buffer Buffer4 {" NL "  mat4 data0;" NL "  mat4 data1;" NL
				  "} g_buffer4;" NL "layout(binding = 0, rgba32f) uniform image2D g_image0;" NL
				  "layout(binding = 1, offset = 8) uniform atomic_uint g_counter[2];";
	}
	std::string GenGLSL(int p)
	{
		std::stringstream ss;
		ss << Common();
		if (p == 0)
		{
			ss << NL "layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;" NL
					 "void UpdateBuffer0(uvec3 id, int add_val) {" NL "  if (id.x < 8 && id.y < 8 && id.z < 8) {" NL
					 "    g_buffer0.m1[id.z].m0[id.y].m0[id.x] += add_val;" NL "  }" NL "}" NL
					 "uniform int g_add_value = 1;" NL "uniform uint g_counter_y = 1;" NL
					 "uniform vec4 g_image_value = vec4(0.125, 0.25, 0.375, 0.5);" NL "void main() {" NL
					 "  uvec3 id = gl_GlobalInvocationID;" NL "  UpdateBuffer0(id, 1);" NL
					 "  UpdateBuffer0(id, g_add_value);" NL "  if (id == uvec3(1, g_counter_y, 1)) {" NL
					 "    uint idx = atomicCounterIncrement(g_counter[1]);" NL "    g_buffer1.data[idx] = idx;" NL
					 "    idx = atomicCounterIncrement(g_counter[1]);" NL "    g_buffer1.data[idx] = idx;" NL "  }" NL
					 "  if (id.x < 4 && id.y < 4 && id.z == 0) {" NL
					 "    vec4 v = imageLoad(g_image0, ivec2(id.xy));" NL
					 "    imageStore(g_image0, ivec2(id.xy), v + g_image_value);" NL "  }" NL
					 "  if (id.x < 2 && id.y == 0 && id.z == 0) {" NL "    g_buffer2.data[id.x] -= int(g_counter_y);" NL
					 "  }" NL "}";
		}
		else if (p == 1)
		{
			ss << NL "layout(local_size_x = 4, local_size_y = 4, local_size_z = 1) in;"
				// translation matrix
				NL "uniform mat4 g_mvp = mat4(1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  10.0, "
					 "20.0, 30.0, 1.0);" NL "void main() {" NL "  if (gl_GlobalInvocationID == uvec3(0)) {" NL
					 "    g_buffer4.data0 *= g_mvp;" NL "  }" NL "  if (gl_WorkGroupID == uvec3(0)) {" NL
					 "    g_buffer4.data1[gl_LocalInvocationID.y][gl_LocalInvocationID.x] = "
					 "g_mvp[gl_LocalInvocationID.x][gl_LocalInvocationID.y];" NL "  }" NL "}";
		}
		else if (p == 2)
		{
			ss << NL "layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;" NL "void main() {" NL "}";
		}
		return ss.str();
	}
	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_counter_buffer = 0;
		m_texture		 = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		using namespace tcu;

		for (int i = 0; i < 3; ++i)
		{
			m_program[i] = CreateComputeProgram(GenGLSL(i));
			glLinkProgram(m_program[i]);
			if (!CheckProgram(m_program[i]))
				return ERROR;
		}

		glGenBuffers(4, m_storage_buffer);
		/* storage buffer 0 */
		{
			std::vector<int> data(5 + 8 * 8 * 8);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(data.size() * sizeof(int)), &data[0], GL_STATIC_COPY);
		}
		/* storage buffer 1 */
		{
			const GLuint data[8] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_COPY);
		}
		/* storage buffer 2 & 3 */
		{
			std::vector<GLint> data(512, 7);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(data.size() * sizeof(GLint)), &data[0], GL_STATIC_COPY);

			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2], 0,
							  (GLsizeiptr)(sizeof(GLint) * data.size() / 2));
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[2],
							  (GLintptr)(sizeof(GLint) * data.size() / 2),
							  (GLsizeiptr)(sizeof(GLint) * data.size() / 2));
		}
		/* storage buffer 4 */
		{
			std::vector<mat4> data(2);
			data[0] = mat4(1);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(data.size() * sizeof(mat4)), &data[0], GL_STATIC_COPY);
		}
		/* counter buffer */
		{
			GLuint data[4] = { 0 };
			glGenBuffers(1, &m_counter_buffer);
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, m_counter_buffer);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(data), data, GL_STATIC_COPY);
		}
		/* texture */
		{
			std::vector<vec4> data(4 * 4, vec4(0.0f));
			glGenTextures(1, &m_texture);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGBA, GL_FLOAT, &data[0]);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glUseProgram(m_program[0]);
		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glDispatchCompute(2, 2, 2);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(3, 2, 2);

		glUseProgram(m_program[1]);
		glDispatchCompute(4, 3, 7);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT |
						GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/* validate texture */
		{
			std::vector<vec4> data(4 * 4);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glGenFramebuffers(1, &m_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
			std::vector<GLubyte> colorData(4 * 4 * 4);
			glReadPixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, &colorData[0]);
			for (int i = 0; i < 4 * 4 * 4; i += 4)
			{
				data[i / 4] =
					vec4(static_cast<GLfloat>(colorData[i] / 255.), static_cast<GLfloat>(colorData[i + 1] / 255.),
						 static_cast<GLfloat>(colorData[i + 2] / 255.), static_cast<GLfloat>(colorData[i + 3] / 255.));
			}
			for (std::size_t i = 0; i < data.size(); ++i)
			{
				if (!ColorEqual(data[i], vec4(0.25f, 0.5f, 0.75f, 1.0f), g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Invalid data at texture." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* validate storage buffer 0 */
		{
			std::vector<int> data(5 + 8 * 8 * 8);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(data.size() * sizeof(int)), &data[0]);
			for (std::size_t i = 5; i < data.size(); ++i)
			{
				if (data[i] != 4)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data is " << data[i]
														<< " should be 2." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* validate storage buffer 1 */
		{
			GLuint data[8];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (GLuint i = 0; i < 4; ++i)
			{
				if (data[i] != i)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data is " << data[i]
														<< " should be " << i << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* validate storage buffer 2 & 3 */
		{
			std::vector<GLint> data(512);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(sizeof(GLint) * data.size()), &data[0]);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != 5)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data is: " << data[i]
														<< " should be: 5." << tcu::TestLog::EndMessage;
					return ERROR;
				}
				if (data[i + 256] != 7)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data is: " << data[i + 256]
														<< " should be: 7." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* validate storage buffer 4 */
		{
			mat4 data[2];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data[0](0, 0));
			if (data[0] != translationMatrix(vec3(10.0f, 20.0f, 30.0f)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is incorrect." << tcu::TestLog::EndMessage;
				return ERROR;
			}
			if (data[1] != transpose(translationMatrix(vec3(10.0f, 20.0f, 30.0f))))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is incorrect." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		/* validate counter buffer */
		{
			GLuint data[4] = { 0 };
			glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(data), data);
			if (data[3] != 4)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is: " << data[3] << " should be: 4." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		for (int i = 0; i < 3; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(4, m_storage_buffer);
		glDeleteBuffers(1, &m_counter_buffer);
		glDeleteTextures(1, &m_texture);
		glDeleteFramebuffers(1, &m_fbo);
		return NO_ERROR;
	}
};

class AdvancedPipelinePostFS : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "CS as an additional pipeline stage - After FS";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that CS which runs just after FS to do a post-processing on a rendered image works as "
				  "expected." NL "2. Verify that CS used as a post-processing filter works as expected." NL
				  "3. Verify that several CS kernels which run in a sequence to do a post-processing on a rendered "
				  "image works as expected.";
	}
	virtual std::string Method()
	{
		return NL
			"1. Render image to Texture0 using VS and FS." NL
			"2. Use Texture0 as an input to Kernel0 which performs post-processing and writes result to Texture1." NL
			"3. Issue MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT) command." NL
			"4. Use Texture1 as an input to Kernel1 which performs post-processing and writes result to Texture0." NL
			"5. Issue MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT) command." NL
			"6. Verify content of the final post-processed image (Texture0).";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program[3];
	GLuint m_render_target[2];
	GLuint m_framebuffer;
	GLuint m_vertex_array;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		memset(m_render_target, 0, sizeof(m_render_target));
		m_framebuffer  = 0;
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			NL "const vec2 g_vertex[4] = vec2[4](vec2(0), vec2(-1, -1), vec2(3, -1), vec2(-1, 3));" NL
			   "void main() {" NL "  gl_Position = vec4(g_vertex[gl_VertexID], 0, 1);" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_color;" NL "void main() {" NL "  g_color = vec4(1, 0, 0, 1);" NL "}";

		m_program[0] = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		const char* const glsl_cs =
			NL "#define TILE_WIDTH 16" NL "#define TILE_HEIGHT 16" NL
			   "const ivec2 kTileSize = ivec2(TILE_WIDTH, TILE_HEIGHT);" NL NL
			   "layout(binding = 0, rgba32f) uniform image2D g_input_image;" NL
			   "layout(binding = 1, rgba32f) uniform image2D g_output_image;" NL	NL
			   "layout(local_size_x = TILE_WIDTH, local_size_y=TILE_HEIGHT) in;" NL NL "void main() {" NL
			   "  const ivec2 tile_xy = ivec2(gl_WorkGroupID);" NL
			   "  const ivec2 thread_xy = ivec2(gl_LocalInvocationID);" NL NL "  if (thread_xy == ivec2(0)) {" NL
			   "    const ivec2 pixel_xy = tile_xy * kTileSize;" NL "    for (int y = 0; y < TILE_HEIGHT; ++y) {" NL
			   "      for (int x = 0; x < TILE_WIDTH; ++x) {" NL
			   "        imageStore(g_output_image, pixel_xy + ivec2(x, y), vec4(0, 1, 0, 1));" NL "      }" NL
			   "    }" NL "  }" NL "}";

		m_program[1] = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;

		const char* const glsl_cs2 = NL "#define TILE_WIDTH 32" NL "#define TILE_HEIGHT 32" NL
										"const ivec2 kTileSize = ivec2(TILE_WIDTH, TILE_HEIGHT);" NL NL
										"layout(binding = 0, rgba32f) uniform image2D g_input_image;" NL
										"layout(binding = 1, rgba32f) uniform image2D g_output_image;" NL	NL
										"layout(local_size_x = TILE_WIDTH, local_size_y=TILE_HEIGHT) in;" NL NL
										"vec4 Process(vec4 ic) {" NL "  return ic + vec4(1, 0, 0, 0);" NL "}" NL
										"void main() {" NL "  const ivec2 tile_xy = ivec2(gl_WorkGroupID);" NL
										"  const ivec2 thread_xy = ivec2(gl_LocalInvocationID);" NL
										"  const ivec2 pixel_xy = tile_xy * kTileSize + thread_xy;" NL
										"  vec4 ic = imageLoad(g_input_image, pixel_xy);" NL
										"  imageStore(g_output_image, pixel_xy, Process(ic));" NL "}";
		m_program[2] = CreateComputeProgram(glsl_cs2);
		glLinkProgram(m_program[2]);
		if (!CheckProgram(m_program[2]))
			return ERROR;

		glGenVertexArrays(1, &m_vertex_array);

		/* init render targets */
		{
			std::vector<vec4> data(128 * 128);
			glGenTextures(2, m_render_target);
			for (int i = 0; i < 2; ++i)
			{
				glBindTexture(GL_TEXTURE_2D, m_render_target[i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 128, 128, 0, GL_RGBA, GL_FLOAT, &data[0][0]);
			}
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glGenFramebuffers(1, &m_framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_render_target[0], 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		glUseProgram(m_program[0]);
		glBindVertexArray(m_vertex_array);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, 128, 128);
		// draw full-viewport triangle
		glDrawArrays(GL_TRIANGLES, 1,
					 3); // note: <first> is 1 this means that gl_VertexID in the VS will be: 1, 2 and 3
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindImageTexture(0, m_render_target[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);  // input
		glBindImageTexture(1, m_render_target[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); // output
		glUseProgram(m_program[1]);
		glDispatchCompute(128 / 16, 128 / 16, 1);

		glBindImageTexture(0, m_render_target[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);  // input
		glBindImageTexture(1, m_render_target[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F); // output
		glUseProgram(m_program[2]);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(128 / 32, 128 / 32, 1);

		/* validate render target */
		{
			std::vector<vec4> data(128 * 128);
			glBindTexture(GL_TEXTURE_2D, m_render_target[0]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data[0][0]);
			for (std::size_t i = 0; i < data.size(); ++i)
			{
				if (!IsEqual(data[i], vec4(1, 1, 0, 1)))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Invalid data at index " << i << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		for (int i = 0; i < 3; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteTextures(2, m_render_target);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_framebuffer);
		return NO_ERROR;
	}
};

class AdvancedPipelinePostXFB : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "CS as an additional pipeline stage - After XFB";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that CS which process data fedback by VS works as expected." NL
				  "2. Verify that XFB and SSBO works correctly together in one shader." NL
				  "3. Verify that 'switch' statment which selects different execution path for each CS thread works as "
				  "expected.";
	}
	virtual std::string Method()
	{
		return NL "1. Draw triangle with XFB enabled. Some data is written to the XFB buffer." NL
				  "2. Use XFB buffer as 'input SSBO' in CS. Process data and write it to 'output SSBO'." NL
				  "3. Verify 'output SSBO' content.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program[2];
	GLuint m_storage_buffer;
	GLuint m_xfb_buffer;
	GLuint m_vertex_buffer;
	GLuint m_vertex_array;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		m_storage_buffer = 0;
		m_xfb_buffer	 = 0;
		m_vertex_buffer  = 0;
		m_vertex_array   = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		GLint res;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &res);
		if (res <= 0)
		{
			OutputNotSupported("GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS <= 0");
			return NO_ERROR;
		}

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "layout(location = 1) in vec4 g_color;" NL
			   "struct Vertex {" NL "  vec4 position;" NL "  vec4 color;" NL "};" NL "out StageData {" NL
			   "  vec4 color;" NL "} g_vs_out;" NL "layout(binding = 0, std430) buffer StageData {" NL
			   "  Vertex vertex[];" NL "} g_vs_buffer;" NL "void main() {" NL "  gl_Position = g_position;" NL
			   "  g_vs_out.color = g_color;" NL "  g_vs_buffer.vertex[gl_VertexID].position = g_position;" NL
			   "  g_vs_buffer.vertex[gl_VertexID].color = g_color;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec4 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_color;" NL
			   "void main() {" NL "  g_color = g_fs_in.color;" NL "}";

		m_program[0] = CreateProgram(glsl_vs, glsl_fs);
		/* setup xfb varyings */
		{
			const char* const var[2] = { "gl_Position", "StageData.color" };
			glTransformFeedbackVaryings(m_program[0], 2, var, GL_INTERLEAVED_ATTRIBS);
		}
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		const char* const glsl_cs =
			NL "layout(local_size_x = 3) in;" NL "struct Vertex {" NL "  vec4 position;" NL "  vec4 color;" NL "};" NL
			   "layout(binding = 3, std430) buffer Buffer {" NL "  Vertex g_vertex[3];" NL "};" NL
			   "uniform vec4 g_color1 = vec4(0, 0, 1, 0);" NL "uniform int g_two = 2;" NL
			   "void UpdateVertex2(int i) {" NL "  g_vertex[i].color -= vec4(-1, 1, 0, 0);" NL "}" NL "void main() {" NL
			   "  switch (gl_GlobalInvocationID.x) {" NL
			   "    case 0: g_vertex[gl_GlobalInvocationID.x].color += vec4(1, 0, 0, 0); break;" NL
			   "    case 1: g_vertex[1].color += g_color1; break;" NL "    case 2: UpdateVertex2(g_two); break;" NL
			   "    default: return;" NL "  }" NL "}";
		m_program[1] = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(vec4) * 2, NULL, GL_STATIC_COPY);

		glGenBuffers(1, &m_xfb_buffer);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfb_buffer);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 3 * sizeof(vec4) * 2, NULL, GL_STREAM_COPY);

		const float in_data[3 * 8] = { -1, -1, 0, 1, 0, 1, 0, 1, 3, -1, 0, 1, 0, 1, 0, 1, -1, 3, 0, 1, 0, 1, 0, 1 };
		glGenBuffers(1, &m_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(in_data), in_data, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), reinterpret_cast<void*>(sizeof(vec4)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[0]);
		glBindVertexArray(m_vertex_array);
		glBeginTransformFeedback(GL_TRIANGLES);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glEndTransformFeedback();

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_xfb_buffer);
		glUseProgram(m_program[1]);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate storage buffer */
		{
			float data[3 * 8];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			if (memcmp(data, in_data, sizeof(data)) != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data in shader storage buffer is incorrect."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		/* validate xfb buffer */
		{
			const float ref_data[3 * 8] = {
				-1, -1, 0, 1, 1, 1, 0, 1, 3, -1, 0, 1, 0, 1, 1, 1, -1, 3, 0, 1, 1, 0, 0, 1
			};

			float data[3 * 8];
			glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(data), data);
			if (memcmp(data, ref_data, sizeof(data)) != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data in xfb buffer is incorrect." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		for (int i = 0; i < 2; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_xfb_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedSharedIndexing : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Shared Memory - Indexing";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that indexing various types of shared memory works as expected." NL
				  "2. Verify that indexing shared memory with different types of expressions work as expected." NL
				  "3. Verify that all declaration types of shared structures are supported by the GLSL compiler.";
	}
	virtual std::string Method()
	{
		return NL "1. Create CS which uses shared memory in many different ways." NL
				  "2. Write to shared memory using different expressions." NL "3. Validate shared memory content." NL
				  "4. Use synchronization primitives (barrier, groupMemoryBarrier) where applicable.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everyting works as expected.";
	}

	GLuint m_program;
	GLuint m_texture;

	virtual long Setup()
	{
		m_program = 0;
		m_texture = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs = NL
			"layout(binding = 3, rgba32f) uniform image2D g_result_image;" NL
			"layout (local_size_x = 4,local_size_y=4 ) in;" NL "shared vec4 g_shared1[4];" NL
			"shared mat4 g_shared2;" NL "shared struct {" NL "  float data[4];" NL "} g_shared3[4];" NL
			"shared struct Type { float data[4]; } g_shared4[4];" NL "shared Type g_shared5[4];" NL
			"uniform bool g_true = true;" NL
			"uniform float g_values[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };" NL NL
			"void Sync() {" NL "  groupMemoryBarrier();" NL "  barrier();" NL "}" NL
			"void SetMemory(ivec2 xy, float value) {" NL "  g_shared1[xy.y][gl_LocalInvocationID.x] = value;" NL
			"  g_shared2[xy.y][xy.x] = value;" NL "  g_shared3[xy[1]].data[xy[0]] = value;" NL
			"  g_shared4[xy.y].data[xy[0]] = value;" NL
			"  g_shared5[gl_LocalInvocationID.y].data[gl_LocalInvocationID.x] = value;" NL "}" NL
			"bool CheckMemory(ivec2 xy, float expected) {" NL
			"  if (g_shared1[xy.y][xy[0]] != expected) return false;" NL
			"  if (g_shared2[xy[1]][xy[0]] != expected) return false;" NL
			"  if (g_shared3[gl_LocalInvocationID.y].data[gl_LocalInvocationID.x] != expected) return false;" NL
			"  if (g_shared4[gl_LocalInvocationID.y].data[xy.x] != expected) return false;" NL
			"  if (g_shared5[xy.y].data[xy.x] != expected) return false;" NL "  return true;" NL "}" NL
			"void main() {" NL "  const ivec2 thread_xy = ivec2(gl_LocalInvocationID);" NL
			"  vec4 result = vec4(0, 1, 0, 1);" NL NL
			"  SetMemory(thread_xy, g_values[gl_LocalInvocationIndex] * 1.0);" NL "  Sync();" NL
			"  if (!CheckMemory(thread_xy, g_values[gl_LocalInvocationIndex] * 1.0)) result = vec4(1, 0, 0, 1);" NL NL
			"  SetMemory(thread_xy, g_values[gl_LocalInvocationIndex] * -1.0);" NL "  Sync();" NL
			"  if (!CheckMemory(thread_xy, g_values[gl_LocalInvocationIndex] * -1.0)) result = vec4(1, 0, 0, 1);" NL NL
			"  if (g_true && gl_LocalInvocationID.x < 10) {" NL
			"    SetMemory(thread_xy, g_values[gl_LocalInvocationIndex] * 7.0);" NL "    Sync();" NL
			"    if (!CheckMemory(thread_xy, g_values[gl_LocalInvocationIndex] * 7.0)) result = vec4(1, 0, 0, 1);" NL
			"  }" NL NL "  imageStore(g_result_image, thread_xy, result);" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		/* init texture */
		{
			std::vector<vec4> data(4 * 4);
			glGenTextures(1, &m_texture);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGBA, GL_FLOAT, &data[0][0]);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glBindImageTexture(3, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);

		/* validate render target */
		{
			std::vector<vec4> data(4 * 4);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data[0][0]);
			for (std::size_t i = 0; i < data.size(); ++i)
			{
				if (!IsEqual(data[i], vec4(0, 1, 0, 1)))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Invalid data at index " << i << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteTextures(1, &m_texture);
		return NO_ERROR;
	}
};

class AdvancedSharedMax : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Shared Memory - 32K";
	}
	virtual std::string Purpose()
	{
		return NL "Support for 32K of shared memory is required by the OpenGL specifaction. Verify if an "
				  "implementation supports it.";
	}
	virtual std::string Method()
	{
		return NL "Create and dispatch CS which uses 32K of shared memory.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		m_program = 0;
		m_buffer  = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1024) in;" NL
			   "shared struct Type { vec4 v[2]; } g_shared[1024];" // 32768 bytes of shared memory
			NL "layout(std430) buffer Output {" NL "  Type g_output[1024];" NL "};" NL NL "void main() {" NL
			   "  const int id = int(gl_GlobalInvocationID.x);" NL
			   "  g_shared[id].v = vec4[2](vec4(1.0), vec4(1.0));" NL "  memoryBarrierShared();" NL "  barrier();" NL NL
			   "  vec4 sum = vec4(0.0);" NL "  int sum_count = 0;" NL "  for (int i = id - 3; i < id + 4; ++i) {" NL
			   "    if (id >= 0 && id < g_shared.length()) {" NL "      sum += g_shared[id].v[0];" NL
			   "      sum += g_shared[id].v[1];" NL "      sum_count += 2;" NL "    }" NL "  }" NL
			   "  if (any(greaterThan(abs((sum / sum_count) - vec4(1.0)), vec4(0.0000001f)))) return;" NL NL
			   "  g_output[id] = g_shared[id];" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		/* init buffer */
		{
			std::vector<vec4> data(1024 * 2);
			glGenBuffers(1, &m_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(sizeof(vec4) * data.size()), &data[0][0],
						 GL_DYNAMIC_COPY);
		}

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate buffer */
		{
			std::vector<vec4> data(1024 * 2);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(sizeof(vec4) * data.size()), &data[0][0]);
			for (std::size_t i = 0; i < data.size(); ++i)
			{
				if (!IsEqual(data[i], vec4(1.0f)))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Invalid data at index " << i << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

class AdvancedDynamicPaths : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Dynamic execution paths";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify case where each of the four threads takes different execution path in the CS." NL
				  "2. Execution path for each thread is not known at the compilation time." NL
				  "    Selection is made based on the result of the texture sampling." NL
				  "3. Verify that memory synchronization primitives (memoryBarrier* functions) are accepted" NL
				  "    in the control flow.";
	}
	virtual std::string Method()
	{
		return NL "1. Create and dispatch CS that takes different execution paths based on the result of the texture "
				  "sampling." NL "2. In each execution path use different resources (buffers, samplers, uniform "
				  "arrays) to compute output value.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_buffer[4];
	GLuint m_texture[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		memset(m_texture, 0, sizeof(m_texture));
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "layout(std140, binding = 0) buffer Output {" NL
			   "  vec4 g_output[4];" NL "};" NL "uniform isamplerBuffer g_path_buffer;" NL
			   "uniform vec4[4] g_input0 = vec4[4](vec4(100), vec4(200), vec4(300), vec4(400));" NL
			   "uniform samplerBuffer g_input1;" NL "layout(binding = 1, std430) buffer Input2 {" NL
			   "  vec4[4] g_input2;" NL "};" NL NL "void Path2(int id) {" NL
			   "  g_output[id] = texelFetch(g_input1, int(gl_LocalInvocationIndex));" NL "}" NL "void main() {" NL
			   "  const int id = int(gl_GlobalInvocationID.x);" NL
			   "  const int path = texelFetch(g_path_buffer, id).x;" NL NL "  if (path == 0) {" NL
			   "    g_output[id] = g_input0[gl_LocalInvocationID.x];" NL "    memoryBarrier();" NL
			   "  } else if (path == 1) {" NL "    return;" NL "  } else if (path == 2) {" NL "    Path2(id);" NL
			   "    return;" NL "  } else if (path == 3) {" NL "    g_output[id] = g_input2[path - 1];" NL
			   "    memoryBarrierBuffer();" NL "  }" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_buffer);
		glGenTextures(2, m_texture);

		/* init 'output' buffer */
		{
			std::vector<vec4> data(4, vec4(-100.0f));
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(sizeof(vec4) * data.size()), &data[0][0],
						 GL_DYNAMIC_COPY);
		}
		/* init 'input2' buffer */
		{
			const vec4 data[4] = { vec4(1.0f), vec4(2.0f), vec4(3.0f), vec4(4.0f) };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data[0][0], GL_DYNAMIC_COPY);
		}
		/* init 'path' buffer */
		{
			const int data[4] = { 3, 2, 1, 0 };
			glBindBuffer(GL_TEXTURE_BUFFER, m_buffer[2]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(data), &data[0], GL_STATIC_DRAW);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
			glBindTexture(GL_TEXTURE_BUFFER, m_texture[0]);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, m_buffer[2]);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
		}
		/* init 'input1' buffer */
		{
			const vec4 data[4] = { vec4(10.0f), vec4(20.0f), vec4(30.0f), vec4(40.0f) };
			glBindBuffer(GL_TEXTURE_BUFFER, m_buffer[3]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(data), &data[0], GL_STATIC_DRAW);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
			glBindTexture(GL_TEXTURE_BUFFER, m_texture[1]);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffer[3]);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
		}

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_path_buffer"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_input1"), 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, m_texture[1]);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate 'output' buffer */
		{
			vec4 data[4];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data[0][0]);

			const vec4 expected[4] = { vec4(3.0f), vec4(20.0f), vec4(-100.0f), vec4(400.0f) };
			for (int i = 0; i < 4; ++i)
			{
				if (!IsEqual(data[i], expected[i]))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Invalid data at index " << i << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_buffer);
		glDeleteTextures(2, m_texture);
		return NO_ERROR;
	}
};

class AdvancedResourcesMax : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Maximum number of resources in one shader";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that using 8 SSBOs, 12 UBOs, 8 atomic counters, 16 samplers" NL
				  "    and 8 images in one CS works as expected.";
	}
	virtual std::string Method()
	{
		return NL "Create and dispatch CS. Verify result.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer[8];
	GLuint m_uniform_buffer[12];
	GLuint m_atomic_buffer[8];
	GLuint m_texture_buffer[16];
	GLuint m_texture[16];
	GLuint m_image_buffer[8];
	GLuint m_image[8];

	bool RunIteration(GLuint index)
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			const GLuint data = i + 1;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}
		for (GLuint i = 0; i < 12; ++i)
		{
			const GLuint data = i + 1;
			glBindBufferBase(GL_UNIFORM_BUFFER, i, m_uniform_buffer[i]);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}
		for (GLuint i = 0; i < 8; ++i)
		{
			const GLuint data = i + 1;
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, i, m_atomic_buffer[i]);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}
		for (GLuint i = 0; i < 16; ++i)
		{
			const GLuint data = i + 1;
			glBindBuffer(GL_TEXTURE_BUFFER, m_texture_buffer[i]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(data), &data, GL_DYNAMIC_READ);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);

			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_BUFFER, m_texture[i]);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_texture_buffer[i]);
		}
		for (GLuint i = 0; i < 8; ++i)
		{
			const GLuint data = i + 1;
			glBindBuffer(GL_TEXTURE_BUFFER, m_image_buffer[i]);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(data), &data, GL_DYNAMIC_COPY);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);

			glBindTexture(GL_TEXTURE_BUFFER, m_image[i]);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_image_buffer[i]);
			glBindTexture(GL_TEXTURE_BUFFER, 0);

			glBindImageTexture(i, m_image[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
		}

		glUseProgram(m_program);
		glUniform1ui(glGetUniformLocation(m_program, "g_index"), index);
		/* uniform array */
		{
			std::vector<GLuint> data(480);
			for (GLuint i = 0; i < static_cast<GLuint>(data.size()); ++i)
				data[i]   = i + 1;
			glUniform1uiv(glGetUniformLocation(m_program, "g_uniform_def"), static_cast<GLsizei>(data.size()),
						  &data[0]);
		}
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool result = true;
		/* validate buffer */
		{
			GLuint data;
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[index]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);

			if (data != (index + 1) * 6)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data is " << data << " should be "
													<< (index + 1) * 6 << "." << tcu::TestLog::EndMessage;
				result = false;
			}
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
		return result;
	}
	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		memset(m_uniform_buffer, 0, sizeof(m_uniform_buffer));
		memset(m_atomic_buffer, 0, sizeof(m_atomic_buffer));
		memset(m_texture_buffer, 0, sizeof(m_texture_buffer));
		memset(m_texture, 0, sizeof(m_texture));
		memset(m_image_buffer, 0, sizeof(m_image_buffer));
		memset(m_image, 0, sizeof(m_image));
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std140, binding = 0) buffer ShaderStorageBlock {" NL
			   "  uint data;" NL "} g_shader_storage[8];" NL "layout(std140, binding = 0) uniform UniformBlock {" NL
			   "  uint data;" NL "} g_uniform[12];" NL "layout(binding = 0) uniform usamplerBuffer g_sampler[16];" NL
			   "layout(binding = 0, r32ui) uniform uimageBuffer g_image[8];" NL
			   "layout(binding = 0, offset = 0) uniform atomic_uint g_atomic_counter0;" NL
			   "layout(binding = 1, offset = 0) uniform atomic_uint g_atomic_counter1;" NL
			   "layout(binding = 2, offset = 0) uniform atomic_uint g_atomic_counter2;" NL
			   "layout(binding = 3, offset = 0) uniform atomic_uint g_atomic_counter3;" NL
			   "layout(binding = 4, offset = 0) uniform atomic_uint g_atomic_counter4;" NL
			   "layout(binding = 5, offset = 0) uniform atomic_uint g_atomic_counter5;" NL
			   "layout(binding = 6, offset = 0) uniform atomic_uint g_atomic_counter6;" NL
			   "layout(binding = 7, offset = 0) uniform atomic_uint g_atomic_counter7;" NL
			   "uniform uint g_uniform_def[480];" NL "uniform uint g_index = 0u;" NL NL "uint Add() {" NL
			   "  switch (g_index) {" NL "    case 0: return atomicCounter(g_atomic_counter0);" NL
			   "    case 1: return atomicCounter(g_atomic_counter1);" NL
			   "    case 2: return atomicCounter(g_atomic_counter2);" NL
			   "    case 3: return atomicCounter(g_atomic_counter3);" NL
			   "    case 4: return atomicCounter(g_atomic_counter4);" NL
			   "    case 5: return atomicCounter(g_atomic_counter5);" NL
			   "    case 6: return atomicCounter(g_atomic_counter6);" NL
			   "    case 7: return atomicCounter(g_atomic_counter7);" NL "  }" NL "}" NL "void main() {" NL
			   "  g_shader_storage[g_index].data += g_uniform[g_index].data;" NL
			   "  g_shader_storage[g_index].data += texelFetch(g_sampler[g_index], 0).x;" NL
			   "  g_shader_storage[g_index].data += imageLoad(g_image[g_index], 0).x;" NL
			   "  g_shader_storage[g_index].data += Add();" NL
			   "  g_shader_storage[g_index].data += g_uniform_def[g_index];" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(16, m_storage_buffer);
		glGenBuffers(12, m_uniform_buffer);
		glGenBuffers(8, m_atomic_buffer);
		glGenBuffers(16, m_texture_buffer);
		glGenTextures(16, m_texture);
		glGenBuffers(8, m_image_buffer);
		glGenTextures(8, m_image);

		if (!RunIteration(0))
			return ERROR;
		if (!RunIteration(1))
			return ERROR;
		if (!RunIteration(5))
			return ERROR;

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(16, m_storage_buffer);
		glDeleteBuffers(12, m_uniform_buffer);
		glDeleteBuffers(8, m_atomic_buffer);
		glDeleteBuffers(16, m_texture_buffer);
		glDeleteTextures(16, m_texture);
		glDeleteBuffers(8, m_image_buffer);
		glDeleteTextures(8, m_image);
		return NO_ERROR;
	}
};

class AdvancedFP64Case1 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "FP64 support - built-in math functions";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that selected double precision math functions works as expected in the CS.";
	}
	virtual std::string Method()
	{
		return NL "Create and dispatch CS which uses double precision math functions. Verify results.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer[4];
	GLuint m_uniform_buffer[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		memset(m_uniform_buffer, 0, sizeof(m_uniform_buffer));
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "layout(std140, binding = 0) buffer ShaderStorageBlock {" NL
			   "  double data;" NL "} g_shader_storage[4];" NL "layout(std140, binding = 0) uniform UniformBlock {" NL
			   "  double data;" NL "} g_uniform[2];" NL "uniform dvec2 g_uniform_def;" NL NL "void main() {" NL
			   "  if (gl_GlobalInvocationID.x == 0) {" NL
			   "    g_shader_storage[0].data = floor(g_uniform[0].data + 0.1LF);" // floor(1.1LF) == 1.0LF
			NL "  } else if (gl_GlobalInvocationID.x == 1) {" NL
			   "    g_shader_storage[1].data = ceil(g_uniform[1].data + 0.2LF);" // ceil(2.2LF) == 3.0LF
			NL "  } else if (gl_GlobalInvocationID.x == 2) {" NL
			   "    g_shader_storage[2].data = min(g_uniform_def[0] + 0.1LF, 1.0LF);" // min(1.1LF, 1.0LF) == 1.0LF
			NL "  } else if (gl_GlobalInvocationID.x == 3) {" NL
			   "    g_shader_storage[3].data = max(g_uniform_def[0], g_uniform_def.y);" // max(1.0LF, 2.0LF) == 2.0LF
			NL "  }" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_storage_buffer);
		for (GLuint i = 0; i < 4; ++i)
		{
			const GLdouble data = static_cast<GLdouble>(i + 1);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}

		glGenBuffers(2, m_uniform_buffer);
		for (GLuint i = 0; i < 2; ++i)
		{
			const GLdouble data = static_cast<GLdouble>(i + 1);
			glBindBufferBase(GL_UNIFORM_BUFFER, i, m_uniform_buffer[i]);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}

		glUseProgram(m_program);
		glUniform2d(glGetUniformLocation(m_program, "g_uniform_def"), 1.0, 2.0);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			const GLdouble expected[4] = { 1.0, 3.0, 1.0, 2.0 };
			for (int i = 0; i < 4; ++i)
			{
				GLdouble data;
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[i]);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
				if (data != expected[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Data at index " << i << " is " << data << " should be "
						<< expected[i] << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_storage_buffer);
		glDeleteBuffers(2, m_uniform_buffer);
		return NO_ERROR;
	}
};

class AdvancedFP64Case2 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "FP64 support - uniform variables";
	}
	virtual std::string Purpose()
	{
		return NL "1. Verify that all types of double precision uniform variables work as expected in CS." NL
				  "2. Verify that all double precision uniform variables can be updated with Uniform* and "
				  "ProgramUniform* commands." NL "3. Verify that re-linking CS program works as expected.";
	}
	virtual std::string Method()
	{
		return NL "1. Create CS which uses all (double precision) types of uniform variables." NL
				  "2. Update uniform variables with ProgramUniform* commands." NL
				  "3. Verify that uniform variables were updated correctly." NL "4. Re-link CS program." NL
				  "5. Update uniform variables with Uniform* commands." NL
				  "6. Verify that uniform variables were updated correctly.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs = NL
			"layout(local_size_x = 1) in;" NL "buffer Result {" NL "  int g_result;" NL "};" NL "uniform double g_0;" NL
			"uniform dvec2 g_1;" NL "uniform dvec3 g_2;" NL "uniform dvec4 g_3;" NL "uniform dmat2 g_4;" NL
			"uniform dmat2x3 g_5;" NL "uniform dmat2x4 g_6;" NL "uniform dmat3x2 g_7;" NL "uniform dmat3 g_8;" NL
			"uniform dmat3x4 g_9;" NL "uniform dmat4x2 g_10;" NL "uniform dmat4x3 g_11;" NL "uniform dmat4 g_12;" NL NL
			"void main() {" NL "  g_result = 1;" NL NL "  if (g_0 != 1.0LF) g_result = 0;" NL
			"  if (g_1 != dvec2(2.0LF, 3.0LF)) g_result = 0;" NL
			"  if (g_2 != dvec3(4.0LF, 5.0LF, 6.0LF)) g_result = 0;" NL
			"  if (g_3 != dvec4(7.0LF, 8.0LF, 9.0LF, 10.0LF)) g_result = 0;" NL NL
			"  if (g_4 != dmat2(11.0LF, 12.0LF, 13.0LF, 14.0LF)) g_result = 0;" NL
			"  if (g_5 != dmat2x3(15.0LF, 16.0LF, 17.0LF, 18.0LF, 19.0LF, 20.0LF)) g_result = 0;" NL
			"  if (g_6 != dmat2x4(21.0LF, 22.0LF, 23.0LF, 24.0LF, 25.0LF, 26.0LF, 27.0LF, 28.0LF)) g_result = 0;" NL NL
			"  if (g_7 != dmat3x2(29.0LF, 30.0LF, 31.0LF, 32.0LF, 33.0LF, 34.0LF)) g_result = 0;" NL
			"  if (g_8 != dmat3(35.0LF, 36.0LF, 37.0LF, 38.0LF, 39.0LF, 40.0LF, 41.0LF, 42.0LF, 43.0LF)) g_result = "
			"0;" NL "  if (g_9 != dmat3x4(44.0LF, 45.0LF, 46.0LF, 47.0LF, 48.0LF, 49.0LF, 50.0LF, 51.0LF, 52.0LF, "
			"53.0LF, 54.0LF, 55.0LF)) g_result = 0;" NL NL
			"  if (g_10 != dmat4x2(56.0, 57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0)) g_result = 0;" NL
			"  if (g_11 != dmat4x3(63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0, 70.0, 71.0, 27.0, 73, 74.0)) g_result = "
			"0;" NL "  if (g_12 != dmat4(75.0, 76.0, 77.0, 78.0, 79.0, 80.0, 81.0, 82.0, 83.0, 84.0, 85.0, 86.0, 87.0, "
			"88.0, 89.0, 90.0)) g_result = 0;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		/* create buffer */
		{
			const int data = 123;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		}

		glProgramUniform1d(m_program, glGetUniformLocation(m_program, "g_0"), 1.0);
		glProgramUniform2d(m_program, glGetUniformLocation(m_program, "g_1"), 2.0, 3.0);
		glProgramUniform3d(m_program, glGetUniformLocation(m_program, "g_2"), 4.0, 5.0, 6.0);
		glProgramUniform4d(m_program, glGetUniformLocation(m_program, "g_3"), 7.0, 8.0, 9.0, 10.0);

		/* mat2 */
		{
			const GLdouble value[4] = { 11.0, 12.0, 13.0, 14.0 };
			glProgramUniformMatrix2dv(m_program, glGetUniformLocation(m_program, "g_4"), 1, GL_FALSE, value);
		}
		/* mat2x3 */
		{
			const GLdouble value[6] = { 15.0, 16.0, 17.0, 18.0, 19.0, 20.0 };
			glProgramUniformMatrix2x3dv(m_program, glGetUniformLocation(m_program, "g_5"), 1, GL_FALSE, value);
		}
		/* mat2x4 */
		{
			const GLdouble value[8] = { 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0 };
			glProgramUniformMatrix2x4dv(m_program, glGetUniformLocation(m_program, "g_6"), 1, GL_FALSE, value);
		}

		/* mat3x2 */
		{
			const GLdouble value[6] = { 29.0, 30.0, 31.0, 32.0, 33.0, 34.0 };
			glProgramUniformMatrix3x2dv(m_program, glGetUniformLocation(m_program, "g_7"), 1, GL_FALSE, value);
		}
		/* mat3 */
		{
			const GLdouble value[9] = { 35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0 };
			glProgramUniformMatrix3dv(m_program, glGetUniformLocation(m_program, "g_8"), 1, GL_FALSE, value);
		}
		/* mat3x4 */
		{
			const GLdouble value[12] = { 44.0, 45.0, 46.0, 47.0, 48.0, 49.0, 50.0, 51.0, 52.0, 53.0, 54.0, 55.0 };
			glProgramUniformMatrix3x4dv(m_program, glGetUniformLocation(m_program, "g_9"), 1, GL_FALSE, value);
		}

		/* mat4x2 */
		{
			const GLdouble value[8] = { 56.0, 57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0 };
			glProgramUniformMatrix4x2dv(m_program, glGetUniformLocation(m_program, "g_10"), 1, GL_FALSE, value);
		}
		/* mat4x3 */
		{
			const GLdouble value[12] = { 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0, 70.0, 71.0, 27.0, 73, 74.0 };
			glProgramUniformMatrix4x3dv(m_program, glGetUniformLocation(m_program, "g_11"), 1, GL_FALSE, value);
		}
		/* mat4 */
		{
			const GLdouble value[16] = { 75.0, 76.0, 77.0, 78.0, 79.0, 80.0, 81.0, 82.0,
										 83.0, 84.0, 85.0, 86.0, 87.0, 88.0, 89.0, 90.0 };
			glProgramUniformMatrix4dv(m_program, glGetUniformLocation(m_program, "g_12"), 1, GL_FALSE, value);
		}

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		// re-link program (all uniforms will be set to zero)
		glLinkProgram(m_program);

		/* clear buffer */
		{
			const int data = 123;
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
		}

		glUniform1d(glGetUniformLocation(m_program, "g_0"), 1.0);
		glUniform2d(glGetUniformLocation(m_program, "g_1"), 2.0, 3.0);
		glUniform3d(glGetUniformLocation(m_program, "g_2"), 4.0, 5.0, 6.0);
		glUniform4d(glGetUniformLocation(m_program, "g_3"), 7.0, 8.0, 9.0, 10.0);

		/* mat2 */
		{
			const GLdouble value[4] = { 11.0, 12.0, 13.0, 14.0 };
			glUniformMatrix2dv(glGetUniformLocation(m_program, "g_4"), 1, GL_FALSE, value);
		}
		/* mat2x3 */
		{
			const GLdouble value[6] = { 15.0, 16.0, 17.0, 18.0, 19.0, 20.0 };
			glUniformMatrix2x3dv(glGetUniformLocation(m_program, "g_5"), 1, GL_FALSE, value);
		}
		/* mat2x4 */
		{
			const GLdouble value[8] = { 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0 };
			glUniformMatrix2x4dv(glGetUniformLocation(m_program, "g_6"), 1, GL_FALSE, value);
		}

		/* mat3x2 */
		{
			const GLdouble value[6] = { 29.0, 30.0, 31.0, 32.0, 33.0, 34.0 };
			glUniformMatrix3x2dv(glGetUniformLocation(m_program, "g_7"), 1, GL_FALSE, value);
		}
		/* mat3 */
		{
			const GLdouble value[9] = { 35.0, 36.0, 37.0, 38.0, 39.0, 40.0, 41.0, 42.0, 43.0 };
			glUniformMatrix3dv(glGetUniformLocation(m_program, "g_8"), 1, GL_FALSE, value);
		}
		/* mat3x4 */
		{
			const GLdouble value[12] = { 44.0, 45.0, 46.0, 47.0, 48.0, 49.0, 50.0, 51.0, 52.0, 53.0, 54.0, 55.0 };
			glUniformMatrix3x4dv(glGetUniformLocation(m_program, "g_9"), 1, GL_FALSE, value);
		}

		/* mat4x2 */
		{
			const GLdouble value[8] = { 56.0, 57.0, 58.0, 59.0, 60.0, 61.0, 62.0, 63.0 };
			glUniformMatrix4x2dv(glGetUniformLocation(m_program, "g_10"), 1, GL_FALSE, value);
		}
		/* mat4x3 */
		{
			const GLdouble value[12] = { 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0, 70.0, 71.0, 27.0, 73, 74.0 };
			glUniformMatrix4x3dv(glGetUniformLocation(m_program, "g_11"), 1, GL_FALSE, value);
		}
		/* mat4 */
		{
			const GLdouble value[16] = { 75.0, 76.0, 77.0, 78.0, 79.0, 80.0, 81.0, 82.0,
										 83.0, 84.0, 85.0, 86.0, 87.0, 88.0, 89.0, 90.0 };
			glUniformMatrix4dv(glGetUniformLocation(m_program, "g_12"), 1, GL_FALSE, value);
		}

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			int data;
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 1)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 1." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class AdvancedFP64Case3 : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "FP64 support - subroutines";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that subroutines that performs double precision computation works as expected in the CS.";
	}
	virtual std::string Method()
	{
		return NL
			"Create and dispatch CS that uses double precision math functions in subroutines to compute output values.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "uniform double[4] g_input;" NL "uniform int index;" NL
			   "layout(std430, binding = 0) buffer Output {" NL "  double g_output[4];" NL "};" NL
			   "subroutine double MathFunc(double x);" NL "subroutine uniform MathFunc g_func[4];" NL
			   "subroutine(MathFunc)" NL "double Func0(double x) {" NL "  return abs(x);" // abs(-1.0LF) == 1.0LF
			NL "}" NL "subroutine(MathFunc)" NL "double Func1(double x) {" NL
			   "  return round(x);" // round(2.2LF) == 2.0LF
			NL "}" NL "subroutine(MathFunc)" NL "double Func2(double x) {" NL
			   "  return sign(x);" // sign(3.0LF) == 1.0LF
			NL "}" NL "subroutine(MathFunc)" NL "double Func3(double x) {" NL
			   "  return fract(x);" // fract(4.1LF) == 0.1LF
			NL "}" NL "void main() {" NL "  int i = index;" NL "  g_output[i] = g_func[i](g_input[i]);" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(double), NULL, GL_STATIC_DRAW);

		const GLuint index_compute0 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Func0");
		const GLuint index_compute1 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Func1");
		const GLuint index_compute2 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Func2");
		const GLuint index_compute3 = glGetSubroutineIndex(m_program, GL_COMPUTE_SHADER, "Func3");
		const GLint  loc_compute0   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "g_func[0]");
		const GLint  loc_compute1   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "g_func[1]");
		const GLint  loc_compute2   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "g_func[2]");
		const GLint  loc_compute3   = glGetSubroutineUniformLocation(m_program, GL_COMPUTE_SHADER, "g_func[3]");

		glUseProgram(m_program);

		// setup subroutines
		GLuint indices[4];
		indices[loc_compute0] = index_compute0;
		indices[loc_compute1] = index_compute1;
		indices[loc_compute2] = index_compute2;
		indices[loc_compute3] = index_compute3;
		glUniformSubroutinesuiv(GL_COMPUTE_SHADER, 4, indices);

		/* set uniforms */
		{
			const GLdouble data[4] = { -1.0, 2.2, 3.0, 4.1 };
			glUniform1dv(glGetUniformLocation(m_program, "g_input"), 4, data);
		}
		glUniform1i(glGetUniformLocation(m_program, "index"), 0);
		glDispatchCompute(1, 1, 1);
		glUniform1i(glGetUniformLocation(m_program, "index"), 1);
		glDispatchCompute(1, 1, 1);
		glUniform1i(glGetUniformLocation(m_program, "index"), 2);
		glDispatchCompute(1, 1, 1);
		glUniform1i(glGetUniformLocation(m_program, "index"), 3);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* validate */
		{
			const GLdouble expected[4] = { 1.0, 2.0, 1.0, 0.1 };
			GLdouble	   data[4];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			for (int i = 0; i < 4; ++i)
			{
				if (fabs(data[i] - expected[i]) > g_color_eps.x())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Data at index " << i << " is " << data[i] << " should be "
						<< expected[i] << "." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class AdvancedConditionalDispatching : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Conditional Dispatching";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that DispatchCompute and DispatchComputeIndirect commands work as expected inside "
				  "conditional blocks.";
	}
	virtual std::string Method()
	{
		return NL "1. Render two quads. One will pass depth-test and the second one will not." NL
				  "2. Use GL_ANY_SAMPLES_PASSED query objects to 'remember' these results." NL
				  "3. Use DispatchCompute and DispatchComputeIndirect commands inside conditional blocks using both "
				  "query objects." NL
				  "4. Verify that DispatchCompute and DispatchComputeIndirect commands are only executed in" NL
				  "    the conditional block that uses query object that has passed depth-test.";
	}
	virtual std::string PassCriteria()
	{
		return NL "Everything works as expected.";
	}

	GLuint m_program_vsfs;
	GLuint m_program_cs;
	GLuint m_vertex_array;
	GLuint m_query[2];
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	virtual long Setup()
	{
		m_program_vsfs = 0;
		m_program_cs   = 0;
		m_vertex_array = 0;
		memset(m_query, 0, sizeof(m_query));
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_vs = NL
			"uniform float g_depth;" NL "uniform vec2[3] g_vertex = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));" NL
			"void main() {" NL "  gl_Position = vec4(g_vertex[gl_VertexID], g_depth, 1);" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_color;" NL "void main() {" NL "  g_color = vec4(0, 1, 0, 1);" NL "}";

		m_program_vsfs = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program_vsfs);
		if (!CheckProgram(m_program_vsfs))
			return ERROR;

		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL "  int g_output;" NL "};" NL
			   "void main() {" NL "  atomicAdd(g_output, 1);" NL "}";
		m_program_cs = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program_cs);
		if (!CheckProgram(m_program_cs))
			return ERROR;

		/* create storage buffer */
		{
			const int data = 0;
			glGenBuffers(1, &m_storage_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data, GL_DYNAMIC_COPY);
		}
		/* create dispatch buffer */
		{
			const GLuint data[3] = { 2, 2, 2 };
			glGenBuffers(1, &m_dispatch_buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glGenQueries(2, m_query);

		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_program_vsfs);
		glBindVertexArray(m_vertex_array);

		// this draw call will pass depth test
		glBeginQuery(GL_ANY_SAMPLES_PASSED, m_query[0]);
		glUniform1f(glGetUniformLocation(m_program_vsfs, "g_depth"), 0.0f);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glEndQuery(GL_ANY_SAMPLES_PASSED);

		// this draw call will NOT pass depth test
		glBeginQuery(GL_ANY_SAMPLES_PASSED, m_query[1]);
		glUniform1f(glGetUniformLocation(m_program_vsfs, "g_depth"), 0.5f);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glEndQuery(GL_ANY_SAMPLES_PASSED);

		glDisable(GL_DEPTH_TEST);

		glUseProgram(m_program_cs);

		// these commands should be executed normally
		glBeginConditionalRender(m_query[0], GL_QUERY_WAIT);
		glDispatchCompute(2, 2, 2);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchComputeIndirect(0);
		glEndConditionalRender();

		/* validate */
		{
			int data;
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 16)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 16." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		// these commands should be discarded
		glBeginConditionalRender(m_query[1], GL_QUERY_WAIT);
		glDispatchCompute(2, 2, 2);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchComputeIndirect(0);
		glEndConditionalRender();

		/* validate */
		{
			int data;
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), &data);
			if (data != 16 && m_context.getRenderContext().getRenderTarget().getDepthBits() != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 16." << tcu::TestLog::EndMessage;
				return ERROR;
			}
			else if (data != 32 && m_context.getRenderContext().getRenderTarget().getDepthBits() == 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is " << data << " should be 32." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program_vsfs);
		glDeleteProgram(m_program_cs);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteQueries(2, m_query);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class NegativeAPINoActiveProgram : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "API errors - no active program";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the OpenGL API.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint m_program;

	virtual long Setup()
	{
		m_program = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		glDispatchCompute(1, 2, 3);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION is generated by DispatchCompute or\n"
				<< "DispatchComputeIndirect if there is no active program for the compute\n"
				<< "shader stage." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		/* indirect dispatch */
		{
			GLuint		 buffer;
			const GLuint num_group[3] = { 3, 2, 1 };
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_group), num_group, GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
			glDeleteBuffers(1, &buffer);
			if (glGetError() != GL_INVALID_OPERATION)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "INVALID_OPERATION is generated by DispatchCompute or\n"
					<< "DispatchComputeIndirect if there is no active program for the compute\n"
					<< "shader stage." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "void main() {" NL "  gl_Position = g_position;" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_color;" NL "void main() {" NL "  g_color = vec4(1);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glUseProgram(m_program);

		glDispatchCompute(1, 2, 3);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION is generated by DispatchCompute or\n"
				<< "DispatchComputeIndirect if there is no active program for the compute\n"
				<< "shader stage." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		/* indirect dispatch */
		{
			GLuint		 buffer;
			const GLuint num_group[3] = { 3, 2, 1 };
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
			glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_group), num_group, GL_STATIC_DRAW);
			glDispatchComputeIndirect(0);
			glDeleteBuffers(1, &buffer);
			if (glGetError() != GL_INVALID_OPERATION)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "INVALID_OPERATION is generated by DispatchCompute or\n"
					<< "DispatchComputeIndirect if there is no active program for the compute\n"
					<< "shader stage." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		return NO_ERROR;
	}
};

class NegativeAPIWorkGroupCount : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "API errors - invalid work group count";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the OpenGL API.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
			   "void main() {" NL
			   "  g_output[gl_GlobalInvocationID.x * gl_GlobalInvocationID.y * gl_GlobalInvocationID.z] = 0;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 100000, NULL, GL_DYNAMIC_DRAW);

		GLint x, y, z;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &x);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &y);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &z);

		glUseProgram(m_program);

		glDispatchCompute(x + 1, 1, 1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by DispatchCompute if any of <num_groups_x>,\n"
				<< "<num_groups_y> or <num_groups_z> is greater than the value of\n"
				<< "MAX_COMPUTE_WORK_GROUP_COUNT for the corresponding dimension." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glDispatchCompute(1, y + 1, 1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by DispatchCompute if any of <num_groups_x>,\n"
				<< "<num_groups_y> or <num_groups_z> is greater than the value of\n"
				<< "MAX_COMPUTE_WORK_GROUP_COUNT for the corresponding dimension." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glDispatchCompute(1, 1, z + 1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by DispatchCompute if any of <num_groups_x>,\n"
				<< "<num_groups_y> or <num_groups_z> is greater than the value of\n"
				<< "MAX_COMPUTE_WORK_GROUP_COUNT for the corresponding dimension." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class NegativeAPIIndirect : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "API errors - incorrect DispatchComputeIndirect usage";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the OpenGL API.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_dispatch_buffer;

	virtual long Setup()
	{
		m_program		  = 0;
		m_storage_buffer  = 0;
		m_dispatch_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
			   "void main() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 100000, NULL, GL_DYNAMIC_DRAW);

		const GLuint num_groups[6] = { 1, 1, 1, 1, 1, 1 };
		glGenBuffers(1, &m_dispatch_buffer);
		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_dispatch_buffer);
		glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(num_groups), num_groups, GL_STATIC_COPY);

		glUseProgram(m_program);

		glDispatchComputeIndirect(-2);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by DispatchComputeIndirect if <indirect> is\n"
				<< "less than zero or not a multiple of four." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glDispatchComputeIndirect(3);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by DispatchComputeIndirect if <indirect> is\n"
				<< "less than zero or not a multiple of four." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glDispatchComputeIndirect(16);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_OPERATION is generated by DispatchComputeIndirect if no buffer is\n"
				<< "bound to DISPATCH_INDIRECT_BUFFER or if the command would source data\n"
				<< "beyond the end of the bound buffer object." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
		glDispatchComputeIndirect(0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_OPERATION is generated by DispatchComputeIndirect if no buffer is\n"
				<< "bound to DISPATCH_INDIRECT_BUFFER or if the command would source data\n"
				<< "beyond the end of the bound buffer object." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_dispatch_buffer);
		return NO_ERROR;
	}
};

class NegativeAPIProgram : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "API errors - program state";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the OpenGL API.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}
	virtual long Run()
	{
		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "void main() {" NL "  gl_Position = g_position;" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_color;" NL "void main() {" NL "  g_color = vec4(1);" NL "}";
		m_program = CreateProgram(glsl_vs, glsl_fs);

		GLint v[3];
		glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, v);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION is generated by GetProgramiv if <pname> is\n"
				<< "COMPUTE_LOCAL_WORK_SIZE and either the program has not been linked\n"
				<< "successfully, or has been linked but contains no compute shaders." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, v);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION is generated by GetProgramiv if <pname> is\n"
				<< "COMPUTE_LOCAL_WORK_SIZE and either the program has not been linked\n"
				<< "successfully, or has been linked but contains no compute shaders." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glDeleteProgram(m_program);

		const char* const glsl_cs =
			"#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL
			"  uint g_output[];" NL "};" NL "void main() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}";
		m_program = glCreateProgram();

		GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
		glAttachShader(m_program, sh);
		glDeleteShader(sh);
		glShaderSource(sh, 1, &glsl_cs, NULL);
		glCompileShader(sh);

		sh = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(m_program, sh);
		glDeleteShader(sh);
		glShaderSource(sh, 1, &glsl_vs, NULL);
		glCompileShader(sh);

		sh = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(m_program, sh);
		glDeleteShader(sh);
		glShaderSource(sh, 1, &glsl_fs, NULL);
		glCompileShader(sh);

		glLinkProgram(m_program);
		GLint status;
		glGetProgramiv(m_program, GL_LINK_STATUS, &status);
		if (status == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "LinkProgram will fail if <program> contains a combination"
				<< " of compute and\n non-compute shaders.\n"
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

class NegativeGLSLCompileTimeErrors : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Compile-time errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the GLSL compiler.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	static std::string Shader1(int x, int y, int z)
	{
		std::stringstream ss;
		ss << "#version 430 core" NL "layout(local_size_x = " << x << ", local_size_y = " << y
		   << ", local_size_z = " << z << ") in;" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
										  "void main() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}";
		return ss.str();
	}
	virtual long Run()
	{
		// gl_GlobalInvocationID requires "#version 430" or later or GL_ARB_compute_shader
		// extension enabled
		if (!Compile("#version 420 core" NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL
					 "  uint g_output[];" NL "};" NL "void main() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL
					 "}"))
			return ERROR;

		if (!Compile("#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(local_size_x = 2) in;" NL
					 "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL "void main() {" NL
					 "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}"))
			return ERROR;

		if (!Compile("#version 430 core" NL "layout(local_size_x = 1) in;" NL "in uint x;" NL
					 "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL "void main() {" NL
					 "  g_output[gl_GlobalInvocationID.x] = x;" NL "}"))
			return ERROR;

		if (!Compile("#version 430 core" NL "layout(local_size_x = 1) in;" NL "out uint x;" NL
					 "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL "void main() {" NL
					 "  g_output[gl_GlobalInvocationID.x] = 0;" NL "  x = 0;" NL "}"))
			return ERROR;

		{
			GLint x, y, z;
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &x);
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &y);
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &z);

			if (!Compile(Shader1(x + 1, 1, 1)))
				return ERROR;
			if (!Compile(Shader1(1, y + 1, 1)))
				return ERROR;
			if (!Compile(Shader1(1, 1, z + 1)))
				return ERROR;
		}

		return NO_ERROR;
	}

	bool Compile(const std::string& source)
	{
		const GLuint sh = glCreateShader(GL_COMPUTE_SHADER);

		const char* const src = source.c_str();
		glShaderSource(sh, 1, &src, NULL);
		glCompileShader(sh);

		GLchar log[1024];
		glGetShaderInfoLog(sh, sizeof(log), NULL, log);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader Info Log:\n"
											<< log << tcu::TestLog::EndMessage;

		GLint status;
		glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
		glDeleteShader(sh);

		if (status == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Compilation should fail." << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}
};

class NegativeGLSLLinkTimeErrors : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "Link-time errors";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that appropriate errors are generated by the GLSL linker.";
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
		// no layout
		if (!Link("#version 430 core" NL "void Run();" NL "void main() {" NL "  Run();" NL "}",
				  "#version 430 core" NL "layout(std430) buffer Output {" NL "  uint g_output[];" NL "};" NL
				  "void Run() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}"))
			return ERROR;

		if (!Link("#version 430 core" NL "layout(local_size_x = 2) in;" NL "void Run();" NL "void main() {" NL
				  "  Run();" NL "}",
				  "#version 430 core" NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Output {" NL
				  "  uint g_output[];" NL "};" NL "void Run() {" NL "  g_output[gl_GlobalInvocationID.x] = 0;" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Link(const std::string& cs0, const std::string& cs1)
	{
		const GLuint p = glCreateProgram();

		/* shader 0 */
		{
			GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src = cs0.c_str();
			glShaderSource(sh, 1, &src, NULL);
			glCompileShader(sh);

			GLint status;
			glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				glDeleteProgram(p);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "CS0 compilation should be ok." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		/* shader 1 */
		{
			GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src = cs1.c_str();
			glShaderSource(sh, 1, &src, NULL);
			glCompileShader(sh);

			GLint status;
			glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				glDeleteProgram(p);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "CS1 compilation should be ok." << tcu::TestLog::EndMessage;
				return false;
			}
		}

		glLinkProgram(p);

		GLchar log[1024];
		glGetProgramInfoLog(p, sizeof(log), NULL, log);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
											<< log << tcu::TestLog::EndMessage;

		GLint status;
		glGetProgramiv(p, GL_LINK_STATUS, &status);
		glDeleteProgram(p);

		if (status == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Link operation should fail." << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}
};

class BasicWorkGroupSizeIsConst : public ComputeShaderBase
{
	virtual std::string Title()
	{
		return NL "gl_WorkGroupSize is an constant";
	}
	virtual std::string Purpose()
	{
		return NL "Verify that gl_WorkGroupSize can be used as an constant expression.";
	}
	virtual std::string Method()
	{
		return NL "";
	}
	virtual std::string PassCriteria()
	{
		return NL "";
	}

	GLuint m_program;
	GLuint m_storage_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 2, local_size_y = 3, local_size_z = 4) in;" NL
			   "layout(std430, binding = 0) buffer Output {" NL "  uint g_buffer[22 + gl_WorkGroupSize.x];" NL "};" NL
			   "shared uint g_shared[gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z];" NL
			   "uniform uint g_uniform[gl_WorkGroupSize.z + 20] = { "
			   "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };" NL "void main() {" NL
			   "  g_shared[gl_LocalInvocationIndex] = 1U;" NL "  groupMemoryBarrier();" NL "  barrier();" NL
			   "  uint sum = 0;" NL
			   "  for (uint i = 0; i < gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z; ++i) {" NL
			   "    sum += g_shared[i];" NL "  }" NL "  sum += g_uniform[gl_LocalInvocationIndex];" NL
			   "  g_buffer[gl_LocalInvocationIndex] = sum;" NL "}";
		m_program = CreateComputeProgram(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 24 * sizeof(GLuint), NULL, GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		long	error = NO_ERROR;
		GLuint* data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer);
		data =
			static_cast<GLuint*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * 24, GL_MAP_READ_BIT));
		for (GLuint i = 0; i < 24; ++i)
		{
			if (data[i] != (i + 25))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data at index " << i << " is " << data[i] << " should be " << i + 25
					<< "." << tcu::TestLog::EndMessage;
				error = ERROR;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		return error;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};

} // anonymous namespace

ComputeShaderTests::ComputeShaderTests(deqp::Context& context) : TestCaseGroup(context, "compute_shader", "")
{
}

ComputeShaderTests::~ComputeShaderTests(void)
{
}

void ComputeShaderTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "simple-compute", TestSubcase::Create<SimpleCompute>));
	addChild(new TestSubcase(m_context, "one-work-group", TestSubcase::Create<BasicOneWorkGroup>));
	addChild(new TestSubcase(m_context, "resource-ubo", TestSubcase::Create<BasicResourceUBO>));
	addChild(new TestSubcase(m_context, "resource-texture", TestSubcase::Create<BasicResourceTexture>));
	addChild(new TestSubcase(m_context, "resource-image", TestSubcase::Create<BasicResourceImage>));
	addChild(new TestSubcase(m_context, "resource-atomic-counter", TestSubcase::Create<BasicResourceAtomicCounter>));
	addChild(new TestSubcase(m_context, "resource-subroutine", TestSubcase::Create<BasicResourceSubroutine>));
	addChild(new TestSubcase(m_context, "resource-uniform", TestSubcase::Create<BasicResourceUniform>));
	addChild(new TestSubcase(m_context, "built-in-variables", TestSubcase::Create<BasicBuiltinVariables>));
	addChild(new TestSubcase(m_context, "max", TestSubcase::Create<BasicMax>));
	addChild(new TestSubcase(m_context, "work-group-size", TestSubcase::Create<BasicWorkGroupSizeIsConst>));
	addChild(new TestSubcase(m_context, "build-monolithic", TestSubcase::Create<BasicBuildMonolithic>));
	addChild(new TestSubcase(m_context, "build-separable", TestSubcase::Create<BasicBuildSeparable>));
	addChild(new TestSubcase(m_context, "shared-simple", TestSubcase::Create<BasicSharedSimple>));
	addChild(new TestSubcase(m_context, "shared-struct", TestSubcase::Create<BasicSharedStruct>));
	addChild(new TestSubcase(m_context, "dispatch-indirect", TestSubcase::Create<BasicDispatchIndirect>));
	addChild(new TestSubcase(m_context, "sso-compute-pipeline", TestSubcase::Create<BasicSSOComputePipeline>));
	addChild(new TestSubcase(m_context, "sso-case2", TestSubcase::Create<BasicSSOCase2>));
	addChild(new TestSubcase(m_context, "sso-case3", TestSubcase::Create<BasicSSOCase3>));
	addChild(new TestSubcase(m_context, "atomic-case1", TestSubcase::Create<BasicAtomicCase1>));
	addChild(new TestSubcase(m_context, "atomic-case2", TestSubcase::Create<BasicAtomicCase2>));
	addChild(new TestSubcase(m_context, "atomic-case3", TestSubcase::Create<BasicAtomicCase3>));
	addChild(new TestSubcase(m_context, "copy-image", TestSubcase::Create<AdvancedCopyImage>));
	addChild(new TestSubcase(m_context, "pipeline-pre-vs", TestSubcase::Create<AdvancedPipelinePreVS>));
	addChild(
		new TestSubcase(m_context, "pipeline-gen-draw-commands", TestSubcase::Create<AdvancedPipelineGenDrawCommands>));
	addChild(new TestSubcase(m_context, "pipeline-compute-chain", TestSubcase::Create<AdvancedPipelineComputeChain>));
	addChild(new TestSubcase(m_context, "pipeline-post-fs", TestSubcase::Create<AdvancedPipelinePostFS>));
	addChild(new TestSubcase(m_context, "pipeline-post-xfb", TestSubcase::Create<AdvancedPipelinePostXFB>));
	addChild(new TestSubcase(m_context, "shared-indexing", TestSubcase::Create<AdvancedSharedIndexing>));
	addChild(new TestSubcase(m_context, "shared-max", TestSubcase::Create<AdvancedSharedMax>));
	addChild(new TestSubcase(m_context, "dynamic-paths", TestSubcase::Create<AdvancedDynamicPaths>));
	addChild(new TestSubcase(m_context, "resources-max", TestSubcase::Create<AdvancedResourcesMax>));
	addChild(new TestSubcase(m_context, "fp64-case1", TestSubcase::Create<AdvancedFP64Case1>));
	addChild(new TestSubcase(m_context, "fp64-case2", TestSubcase::Create<AdvancedFP64Case2>));
	addChild(new TestSubcase(m_context, "fp64-case3", TestSubcase::Create<AdvancedFP64Case3>));
	addChild(
		new TestSubcase(m_context, "conditional-dispatching", TestSubcase::Create<AdvancedConditionalDispatching>));
	addChild(new TestSubcase(m_context, "api-no-active-program", TestSubcase::Create<NegativeAPINoActiveProgram>));
	addChild(new TestSubcase(m_context, "api-work-group-count", TestSubcase::Create<NegativeAPIWorkGroupCount>));
	addChild(new TestSubcase(m_context, "api-indirect", TestSubcase::Create<NegativeAPIIndirect>));
	addChild(new TestSubcase(m_context, "api-program", TestSubcase::Create<NegativeAPIProgram>));
	addChild(
		new TestSubcase(m_context, "glsl-compile-time-errors", TestSubcase::Create<NegativeGLSLCompileTimeErrors>));
	addChild(new TestSubcase(m_context, "glsl-link-time-errors", TestSubcase::Create<NegativeGLSLLinkTimeErrors>));
}
} // gl4cts namespace
