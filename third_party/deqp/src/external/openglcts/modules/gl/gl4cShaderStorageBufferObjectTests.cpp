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

#include "gl4cShaderStorageBufferObjectTests.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include <assert.h>
#include <cmath>
#include <cstdarg>

namespace gl4cts
{
using namespace glw;

namespace
{
typedef tcu::Vec2  vec2;
typedef tcu::Vec3  vec3;
typedef tcu::Vec4  vec4;
typedef tcu::IVec4 ivec4;
typedef tcu::UVec4 uvec4;
typedef tcu::Mat4  mat4;

enum ShaderStage
{
	vertex,
	fragment,
	compute
};

enum BufferLayout
{
	std140,
	std430,
	shared,
	packed
};

enum ElementType
{
	vector,
	matrix_cm,
	matrix_rm,
	structure
};

enum BindingSeq
{
	bindbasebefore,
	bindbaseafter,
	bindrangeoffset,
	bindrangesize
};

const char* const kGLSLVer = "#version 430 core\n";

class ShaderStorageBufferObjectBase : public deqp::SubcaseBase
{
	virtual std::string Title()
	{
		return "";
	}

	virtual std::string Purpose()
	{
		return "";
	}

	virtual std::string Method()
	{
		return "";
	}

	virtual std::string PassCriteria()
	{
		return "";
	}

public:
	bool SupportedInVS(int requiredVS)
	{
		GLint blocksVS;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &blocksVS);
		if (blocksVS >= requiredVS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredVS << " VS storage blocks but only " << blocksVS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInTCS(int requiredTCS)
	{
		GLint blocksTCS;
		glGetIntegerv(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &blocksTCS);
		if (blocksTCS >= requiredTCS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredTCS << " TCS storage blocks but only " << blocksTCS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInTES(int requiredTES)
	{
		GLint blocksTES;
		glGetIntegerv(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &blocksTES);
		if (blocksTES >= requiredTES)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredTES << " TES storage blocks but only " << blocksTES << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInGS(int requiredGS)
	{
		GLint blocksGS;
		glGetIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &blocksGS);
		if (blocksGS >= requiredGS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredGS << " GS storage blocks but only " << blocksGS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
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

	inline bool ColorEqual(const vec3& c0, const vec3& c1, const vec4& epsilon)
	{
		if (fabs(c0[0] - c1[0]) > epsilon[0])
			return false;
		if (fabs(c0[1] - c1[1]) > epsilon[1])
			return false;
		if (fabs(c0[2] - c1[2]) > epsilon[2])
			return false;
		return true;
	}

	bool CheckProgram(GLuint program)
	{
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

		return status == GL_TRUE ? true : false;
	}

	GLuint CreateProgram(const std::string& vs, const std::string& tcs, const std::string& tes, const std::string& gs,
						 const std::string& fs)
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
		if (!tcs.empty())
		{
			const GLuint sh = glCreateShader(GL_TESS_CONTROL_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, tcs.c_str() };
			glShaderSource(sh, 2, src, NULL);
			glCompileShader(sh);
		}
		if (!tes.empty())
		{
			const GLuint sh = glCreateShader(GL_TESS_EVALUATION_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, tes.c_str() };
			glShaderSource(sh, 2, src, NULL);
			glCompileShader(sh);
		}
		if (!gs.empty())
		{
			const GLuint sh = glCreateShader(GL_GEOMETRY_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, gs.c_str() };
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

	GLuint CreateProgramCS(const std::string& cs)
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

	GLuint BuildShaderProgram(GLenum type, const std::string& source)
	{
		if (type == GL_COMPUTE_SHADER)
		{
			const char* const src[3] = { kGLSLVer, "#extension GL_ARB_compute_shader : require\n", source.c_str() };
			return glCreateShaderProgramv(type, 3, src);
		}

		const char* const src[2] = { kGLSLVer, source.c_str() };
		return glCreateShaderProgramv(type, 2, src);
	}

	bool ValidateReadBuffer(int x, int y, int w, int h, const vec3& expected)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		vec4					 g_color_eps  = vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		std::vector<vec3> display(w * h);
		glReadPixels(x, y, w, h, GL_RGB, GL_FLOAT, &display[0]);

		bool result = true;
		for (int j = 0; j < h; ++j)
		{
			for (int i = 0; i < w; ++i)
			{
				if (!ColorEqual(display[j * w + i], expected, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Color at (" << x + i << ", " << y + j << ") is ("
						<< display[j * w + i][0] << " " << display[j * w + i][1] << " " << display[j * w + i][2]
						<< ") should be (" << expected[0] << " " << expected[1] << " " << expected[2] << ")."
						<< tcu::TestLog::EndMessage;
					result = false;
				}
			}
		}

		return result;
	}

	bool ValidateWindow4Quads(const vec3& lb, const vec3& rb, const vec3& rt, const vec3& lt, int* bad_pixels = NULL)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		vec4					 g_color_eps  = vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		const int		  width  = 100;
		const int		  height = 100;
		std::vector<vec3> fb(width * height);
		glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, &fb[0]);

		bool status = true;
		int  bad	= 0;

		// left-bottom quad
		for (int y = 10; y < height / 2 - 10; ++y)
		{
			for (int x = 10; x < width / 2 - 10; ++x)
			{
				const int idx = y * width + x;
				if (!ColorEqual(fb[idx], lb, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "First bad color (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Left-bottom quad checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
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
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "right-bottom quad checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
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
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "right-top quad checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
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
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "left-top quad checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
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
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "middle horizontal line checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
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
						<< tcu::TestLog::Message << "Bad color at (" << x << ", " << y << "): " << fb[idx][0] << " "
						<< fb[idx][1] << " " << fb[idx][2] << tcu::TestLog::EndMessage;
					status = false;
					bad++;
				}
			}
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "middle vertical line checking failed. Bad pixels: " << bad
				<< tcu::TestLog::EndMessage;
			//return status;
		}

		if (bad_pixels)
			*bad_pixels = bad;
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Bad pixels: " << (bad_pixels == NULL ? 0 : *bad_pixels)
			<< ", counted bad: " << bad << tcu::TestLog::EndMessage;
		return status;
	}

	const mat4 Translation(float tx, float ty, float tz)
	{
		float d[] = { 1.0f, 0.0f, 0.0f, tx, 0.0f, 1.0f, 0.0f, ty, 0.0f, 0.0f, 1.0f, tz, 0.0f, 0.0f, 0.0f, 1.0f };
		return mat4(d);
	}

	const char* GLenumToString(GLenum e)
	{
		switch (e)
		{
		case GL_SHADER_STORAGE_BUFFER_BINDING:
			return "GL_SHADER_STORAGE_BUFFER_BINDING";
		case GL_SHADER_STORAGE_BUFFER_START:
			return "GL_SHADER_STORAGE_BUFFER_START";
		case GL_SHADER_STORAGE_BUFFER_SIZE:
			return "GL_SHADER_STORAGE_BUFFER_SIZE";
		case GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS";
		case GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS";
		case GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS";
		case GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS";
		case GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS";
		case GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS";
		case GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS:
			return "GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS";
		case GL_MAX_SHADER_STORAGE_BLOCK_SIZE:
			return "GL_MAX_SHADER_STORAGE_BLOCK_SIZE";
		case GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS:
			return "GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS";
		case GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES:
			return "GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES";
		case GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT:
			return "GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT";

		default:
			assert(0);
			break;
		}
		return NULL;
	}
};

//-----------------------------------------------------------------------------
// 1.1 BasicBasic
//-----------------------------------------------------------------------------

class BasicBasic : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer;
	GLuint m_vertex_array;

	bool RunIteration(GLuint index)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glShaderStorageBlockBinding(m_program, 0, index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_buffer);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, 0);

		return ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0));
	}

	virtual long Setup()
	{
		m_program	  = 0;
		m_buffer	   = 0;
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(std430, binding = 1) buffer InputBuffer {" NL "  vec4 position[3];" NL "} g_input_buffer;" NL
			   "void main() {" NL "  gl_Position = g_input_buffer.position[gl_VertexID];" NL "}";

		const char* const glsl_fs = NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
									   "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		const float data[12] = { -1.0f, -1.0f, 0.0f, 1.0f, 3.0f, -1.0f, 0.0f, 1.0f, -1.0f, 3.0f, 0.0f, 1.0f };
		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);

		glUseProgram(m_program);

		for (GLuint i = 0; i < 8; ++i)
		{
			if (!RunIteration(i))
				return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicBasicCS : public ShaderStorageBufferObjectBase
{
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
		const char* const glsl_cs = NL "layout(local_size_x = 1) in;" NL "buffer Buffer {" NL "  int result;" NL "};" NL
									   "void main() {" NL "  result = 7;" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4, 0, GL_STATIC_READ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GLint* out_data = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4, GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		if (*out_data == 7)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.2 BasicMax
//-----------------------------------------------------------------------------

class BasicMax : public ShaderStorageBufferObjectBase
{
	bool Check(GLenum e, GLint64 value, bool max_value)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		glGetIntegerv(e, &i);
		glGetInteger64v(e, &i64);
		glGetFloatv(e, &f);
		glGetDoublev(e, &d);
		glGetBooleanv(e, &b);

		bool status = true;
		if (max_value)
		{
			if (static_cast<GLint64>(i) < value)
				status = false;
			if (i64 < value)
				status = false;
			if (static_cast<GLint64>(f) < value)
				status = false;
			if (static_cast<GLint64>(d) < value)
				status = false;

			if (!status)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << GLenumToString(e) << " is " << i << " should be at least "
					<< static_cast<GLint>(value) << tcu::TestLog::EndMessage;
			}
		}
		else
		{
			if (static_cast<GLint64>(i) > value)
				status = false;
			if (i64 > value)
				status = false;
			if (static_cast<GLint64>(f) > value)
				status = false;
			if (static_cast<GLint64>(d) > value)
				status = false;

			if (!status)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << GLenumToString(e) << " is " << i << " should be at most "
					<< static_cast<GLint>(value) << tcu::TestLog::EndMessage;
			}
		}
		return status;
	}

	virtual long Run()
	{
		if (!Check(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, 0, true))
			return ERROR;
		if (!Check(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, 0, true))
			return ERROR;
		if (!Check(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, 0, true))
			return ERROR;
		if (!Check(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, 0, true))
			return ERROR;
		if (!Check(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, 8, true))
			return ERROR;
		if (!Check(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, 8, true))
			return ERROR;
		if (!Check(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, 8, true))
			return ERROR;
		if (!Check(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, 16777216 /* 2^24 */, true))
			return ERROR;
		if (!Check(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, 8, true))
			return ERROR;
		if (!Check(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, 8, true))
			return ERROR;
		if (!Check(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, 256, false))
			return ERROR;
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.3 BasicBinding
//-----------------------------------------------------------------------------

class BasicBinding : public ShaderStorageBufferObjectBase
{
	GLuint m_buffer[4];

	bool Check(GLenum e, GLuint expected)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		glGetIntegerv(e, &i);
		glGetInteger64v(e, &i64);
		glGetFloatv(e, &f);
		glGetDoublev(e, &d);
		glGetBooleanv(e, &b);

		bool status = true;
		if (static_cast<GLuint>(i) != expected)
			status = false;
		if (static_cast<GLuint>(i64) != expected)
			status = false;
		if (static_cast<GLuint>(f) != expected)
			status = false;
		if (static_cast<GLuint>(d) != expected)
			status = false;
		if (b != (expected != 0 ? GL_TRUE : GL_FALSE))
			status = false;

		if (!status)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << GLenumToString(e) << " is " << i
												<< " should be " << expected << tcu::TestLog::EndMessage;
		}
		return status;
	}

	bool CheckIndexed(GLenum e, GLuint index, GLuint expected)
	{
		GLint	 i;
		GLint64   i64;
		GLfloat   f;
		GLdouble  d;
		GLboolean b;

		glGetIntegeri_v(e, index, &i);
		glGetInteger64i_v(e, index, &i64);
		glGetFloati_v(e, index, &f);
		glGetDoublei_v(e, index, &d);
		glGetBooleani_v(e, index, &b);

		bool status = true;
		if (static_cast<GLuint>(i) != expected)
			status = false;
		if (static_cast<GLuint>(i64) != expected)
			status = false;
		if (static_cast<GLuint>(f) != expected)
			status = false;
		if (static_cast<GLuint>(d) != expected)
			status = false;
		if (b != (expected != 0 ? GL_TRUE : GL_FALSE))
			status = false;

		if (!status)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << GLenumToString(e) << " at index " << index
												<< " is " << i << " should be " << expected << tcu::TestLog::EndMessage;
		}
		return status;
	}

	virtual long Setup()
	{
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		// check default state
		if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, 0))
			return ERROR;
		for (GLuint i = 0; i < 8; ++i)
		{
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 0))
				return ERROR;
		}

		glGenBuffers(4, m_buffer);
		for (GLuint i = 0; i < 8; ++i)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_buffer[0]);

			if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, m_buffer[0]))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, m_buffer[0]))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 0))
				return ERROR;

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);

			if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 0))
				return ERROR;
		}

		for (GLuint i = 0; i < 8; ++i)
		{
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, m_buffer[0], 256, 512);

			if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, m_buffer[0]))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, m_buffer[0]))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 256))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 512))
				return ERROR;

			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, 0, 512, 128);

			if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 0))
				return ERROR;

			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, 0, 0, 0);

			if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, i, 0))
				return ERROR;
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, i, 0))
				return ERROR;
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[2]);
		if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, m_buffer[2]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, 0, m_buffer[2]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, 0, 0))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, 0, 0))
			return ERROR;

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_buffer[3]);
		if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, m_buffer[3]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, 5, m_buffer[3]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, 5, 0))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, 5, 0))
			return ERROR;

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 7, m_buffer[1], 2048, 1000);
		if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, m_buffer[1]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, 7, m_buffer[1]))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_START, 7, 2048))
			return ERROR;
		if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_SIZE, 7, 1000))
			return ERROR;

		glDeleteBuffers(4, m_buffer);
		memset(m_buffer, 0, sizeof(m_buffer));

		if (!Check(GL_SHADER_STORAGE_BUFFER_BINDING, 0))
			return ERROR;
		for (GLuint i = 0; i < 8; ++i)
		{
			if (!CheckIndexed(GL_SHADER_STORAGE_BUFFER_BINDING, i, 0))
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(4, m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.4 BasicSyntax
//-----------------------------------------------------------------------------

class BasicSyntax : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer;
	GLuint m_vertex_array;

	bool RunIteration(const char* vs, const char* fs)
	{
		if (m_program != 0)
			glDeleteProgram(m_program);
		m_program = CreateProgram(vs, fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return false;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		return ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0));
	}

	virtual long Setup()
	{
		m_program	  = 0;
		m_buffer	   = 0;
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const int		  kCount		  = 8;
		const char* const glsl_vs[kCount] = {
			NL "layout(std430) buffer Buffer {" NL "  vec4 position[3];" NL "} g_input_buffer;" NL "void main() {" NL
			   "  gl_Position = g_input_buffer.position[gl_VertexID];" NL "}",
			NL "coherent buffer Buffer {" NL "  buffer vec4 position0;" NL "  coherent vec4 position1;" NL
			   "  restrict readonly vec4 position2;" NL "} g_input_buffer;" NL "void main() {" NL
			   "  if (gl_VertexID == 0) gl_Position = g_input_buffer.position0;" NL
			   "  if (gl_VertexID == 1) gl_Position = g_input_buffer.position1;" NL
			   "  if (gl_VertexID == 2) gl_Position = g_input_buffer.position2;" NL "}",
			NL "layout(std140, binding = 0) readonly buffer Buffer {" NL "  readonly vec4 position[];" NL "};" NL
			   "void main() {" NL "  gl_Position = position[gl_VertexID];" NL "}",
			NL "layout(std430, column_major, std140, std430, row_major, packed, shared) buffer;" NL
			   "layout(std430) buffer;" NL "coherent restrict volatile buffer Buffer {" NL
			   "  restrict coherent vec4 position[];" NL "} g_buffer;" NL "void main() {" NL
			   "  gl_Position = g_buffer.position[gl_VertexID];" NL "}",
			NL "buffer Buffer {" NL "  vec4 position[3];" NL "} g_buffer[1];" NL "void main() {" NL
			   "  gl_Position = g_buffer[0].position[gl_VertexID];" NL "}",
			NL "layout(shared) coherent buffer Buffer {" NL "  restrict volatile vec4 position0;" NL
			   "  buffer readonly vec4 position1;" NL "  vec4 position2;" NL "} g_buffer[1];" NL "void main() {" NL
			   "  if (gl_VertexID == 0) gl_Position = g_buffer[0].position0;" NL
			   "  else if (gl_VertexID == 1) gl_Position = g_buffer[0].position1;" NL
			   "  else if (gl_VertexID == 2) gl_Position = g_buffer[0].position2;" NL "}",
			NL "layout(packed) coherent buffer Buffer {" NL "  vec4 position01[];" NL "  vec4 position2;" NL
			   "} g_buffer;" NL "void main() {" NL "  if (gl_VertexID == 0) gl_Position = g_buffer.position01[0];" NL
			   "  else if (gl_VertexID == 1) gl_Position = g_buffer.position01[1];" NL
			   "  else if (gl_VertexID == 2) gl_Position = g_buffer.position2;" NL "}",
			NL "layout(std430) coherent buffer Buffer {" NL "  coherent vec4 position01[];" NL "  vec4 position2[];" NL
			   "} g_buffer;" NL "void main() {" NL "  switch (gl_VertexID) {" NL
			   "    case 0: gl_Position = g_buffer.position01[0]; break;" NL
			   "    case 1: gl_Position = g_buffer.position01[1]; break;" NL
			   "    case 2: gl_Position = g_buffer.position2[gl_VertexID - 2]; break;" NL "  }" NL "}",
		};
		const char* const glsl_fs = NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
									   "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";

		// full viewport triangle
		const float data[12] = { -1.0f, -1.0f, 0.0f, 1.0f, 3.0f, -1.0f, 0.0f, 1.0f, -1.0f, 3.0f, 0.0f, 1.0f };
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);

		for (int i = 0; i < kCount; ++i)
		{
			if (!RunIteration(glsl_vs[i], glsl_fs))
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.5 BasicSyntaxSSO
//-----------------------------------------------------------------------------

class BasicSyntaxSSO : public ShaderStorageBufferObjectBase
{
	GLuint m_pipeline;
	GLuint m_vsp, m_fsp;
	GLuint m_buffer;
	GLuint m_vertex_array;

	bool RunIteration(const char* vs)
	{
		if (m_vsp != 0)
			glDeleteProgram(m_vsp);
		m_vsp = BuildShaderProgram(GL_VERTEX_SHADER, vs);
		if (!CheckProgram(m_vsp))
			return false;

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_vsp);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		return ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0));
	}

	virtual long Setup()
	{
		m_pipeline = 0;
		m_vsp = m_fsp  = 0;
		m_buffer	   = 0;
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		const int		  kCount		  = 8;
		const char* const glsl_vs[kCount] = {
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "layout(std430) buffer Buffer {" NL
			   "  vec4 position[3];" NL "} g_input_buffer;" NL "void main() {" NL
			   "  gl_Position = g_input_buffer.position[gl_VertexID];" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "coherent buffer Buffer {" NL
			   "  vec4 position0;" NL "  coherent vec4 position1;" NL "  restrict readonly vec4 position2;" NL
			   "} g_input_buffer;" NL "void main() {" NL
			   "  if (gl_VertexID == 0) gl_Position = g_input_buffer.position0;" NL
			   "  if (gl_VertexID == 1) gl_Position = g_input_buffer.position1;" NL
			   "  if (gl_VertexID == 2) gl_Position = g_input_buffer.position2;" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL
			   "layout(std140, binding = 0) readonly buffer Buffer {" NL "  readonly vec4 position[];" NL "};" NL
			   "void main() {" NL "  gl_Position = position[gl_VertexID];" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL
			   "layout(std430, column_major, std140, std430, row_major, packed, shared) buffer;" NL
			   "layout(std430) buffer;" NL "coherent restrict volatile buffer Buffer {" NL
			   "  restrict coherent vec4 position[];" NL "} g_buffer;" NL "void main() {" NL
			   "  gl_Position = g_buffer.position[gl_VertexID];" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "buffer Buffer {" NL "  vec4 position[3];" NL
			   "} g_buffer[1];" NL "void main() {" NL "  gl_Position = g_buffer[0].position[gl_VertexID];" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "layout(shared) coherent buffer Buffer {" NL
			   "  restrict volatile vec4 position0;" NL "  readonly vec4 position1;" NL "  vec4 position2;" NL
			   "} g_buffer[1];" NL "void main() {" NL "  if (gl_VertexID == 0) gl_Position = g_buffer[0].position0;" NL
			   "  else if (gl_VertexID == 1) gl_Position = g_buffer[0].position1;" NL
			   "  else if (gl_VertexID == 2) gl_Position = g_buffer[0].position2;" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "layout(packed) coherent buffer Buffer {" NL
			   "  vec4 position01[];" NL "  vec4 position2;" NL "} g_buffer;" NL "void main() {" NL
			   "  if (gl_VertexID == 0) gl_Position = g_buffer.position01[0];" NL
			   "  else if (gl_VertexID == 1) gl_Position = g_buffer.position01[1];" NL
			   "  else if (gl_VertexID == 2) gl_Position = g_buffer.position2;" NL "}",
			NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "layout(std430) coherent buffer Buffer {" NL
			   "  coherent vec4 position01[];" NL "  vec4 position2[];" NL "} g_buffer;" NL "void main() {" NL
			   "  switch (gl_VertexID) {" NL "    case 0: gl_Position = g_buffer.position01[0]; break;" NL
			   "    case 1: gl_Position = g_buffer.position01[1]; break;" NL
			   "    case 2: gl_Position = g_buffer.position2[gl_VertexID - 2]; break;" NL "  }" NL "}",
		};
		const char* const glsl_fs = NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
									   "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";
		m_fsp = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs);
		if (!CheckProgram(m_fsp))
			return ERROR;

		glGenProgramPipelines(1, &m_pipeline);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_fsp);

		// full viewport triangle
		const float data[12] = { -1.0f, -1.0f, 0.0f, 1.0f, 3.0f, -1.0f, 0.0f, 1.0f, -1.0f, 3.0f, 0.0f, 1.0f };
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		glBindProgramPipeline(m_pipeline);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);

		for (int i = 0; i < kCount; ++i)
		{
			if (!RunIteration(glsl_vs[i]))
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgramPipelines(1, &m_pipeline);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteBuffers(1, &m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.6.x BasicStdLayoutBase
//-----------------------------------------------------------------------------

class BasicStdLayoutBaseVS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];
	GLuint m_vertex_array;

	virtual const char* GetInput(std::vector<GLubyte>& in_data) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		std::vector<GLubyte> in_data;
		const char*			 glsl_vs = GetInput(in_data);

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		// output buffer
		std::vector<GLubyte> out_data(in_data.size());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data.size(), &out_data[0], GL_STATIC_DRAW);

		// input buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data.size(), &in_data[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &m_vertex_array);
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[1]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data.size(), &out_data[0]);

		bool status = true;
		for (size_t i = 0; i < in_data.size(); ++i)
		{
			if (in_data[i] != out_data[i])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
					<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[i]) << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicStdLayoutBaseCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];

	virtual const char* GetInput(std::vector<GLubyte>& in_data) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		std::vector<GLubyte> in_data;
		std::stringstream	ss;
		ss << "layout(local_size_x = 1) in;\n" << GetInput(in_data);

		m_program = CreateProgramCS(ss.str());
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data.size(), &in_data[0], GL_STATIC_DRAW);
		std::vector<GLubyte> out_d(in_data.size());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)out_d.size(), &out_d[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLubyte* out_data =
			(GLubyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data.size(), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;

		bool status = true;

		for (size_t i = 0; i < in_data.size(); ++i)
		{
			if (in_data[i] != out_data[i])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
					<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[i]) << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.6.1 BasicStd430LayoutCase1
//-----------------------------------------------------------------------------
const char* GetInput430c1(std::vector<GLubyte>& in_data)
{
	in_data.resize(6 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	int*   ip = reinterpret_cast<int*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 4.0f;
	ip[4]	 = 5;
	ip[5]	 = 6;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  float data0;" NL "  float data1[3];" NL
			  "  ivec2 data2;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL "  float data0;" NL
			  "  float data1[3];" NL "  ivec2 data2;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL "}";
}

class BasicStd430LayoutCase1VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c1(in_data);
	}
};

class BasicStd430LayoutCase1CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c1(in_data);
	}
};
//-----------------------------------------------------------------------------
// 1.6.2 BasicStd430LayoutCase2
//-----------------------------------------------------------------------------
const char* GetInput430c2(std::vector<GLubyte>& in_data)
{
	in_data.resize(20 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 4.0f;
	fp[4]	 = 5.0f;
	fp[5]	 = 6.0f;
	fp[8]	 = 7.0f;
	fp[9]	 = 8.0f;
	fp[10]	= 9.0f;
	fp[12]	= 10.0f;
	fp[13]	= 11.0f;
	fp[14]	= 12.0f;
	fp[16]	= 13.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  float data0;" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  readonly vec3 data3[2];" NL "  float data4;" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  float data0;" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  vec3 data3[2];" NL "  float data4;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL
			  "  for (int i = 0; i < g_input.data3.length(); ++i) g_output.data3[i] = g_input.data3[i];" NL
			  "  g_output.data4 = g_input.data4;" NL "}";
}

class BasicStd430LayoutCase2VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c2(in_data);
	}
};

class BasicStd430LayoutCase2CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c2(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.6.3 BasicStd430LayoutCase3
//-----------------------------------------------------------------------------
const char* GetInput430c3(std::vector<GLubyte>& in_data)
{
	in_data.resize(16 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 4.0f;
	fp[5]	 = 5.0f;
	fp[6]	 = 6.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 7.0f;
	fp[9]	 = 8.0f;
	fp[10]	= 9.0f;
	fp[11]	= 10.0f;
	fp[12]	= 11.0f;
	fp[13]	= 12.0f;
	fp[14]	= 13.0f;
	fp[15]	= 14.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(column_major) mat2x3 data0;" NL
			  "  layout(row_major) mat4x2 data1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(column_major) mat2x3 data0;" NL "  layout(row_major) mat4x2 data1;" NL "} g_output;" NL
			  "void main() {" NL "  g_output.data0 = g_input.data0;" NL "  g_output.data1 = g_input.data1;" NL "}";
}

class BasicStd430LayoutCase3VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c3(in_data);
	}
};

class BasicStd430LayoutCase3CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c3(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.6.4 BasicStd430LayoutCase4
//-----------------------------------------------------------------------------
const char* GetInput430c4(std::vector<GLubyte>& in_data)
{
	in_data.resize(20 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 4.0f;
	fp[4]	 = 5.0f;
	fp[5]	 = 6.0f;
	fp[6]	 = 7.0f;
	fp[7]	 = 8.0f;
	fp[8]	 = 9.0f;
	fp[9]	 = 10.0f;
	fp[10]	= 11.0f;
	fp[12]	= 12.0f;
	fp[13]	= 13.0f;
	fp[14]	= 14.0f;
	fp[16]	= 15.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  mat4x2 data0;" NL "  mat2x3 data1;" NL
			  "  float data2;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL "  mat4x2 data0;" NL
			  "  mat2x3 data1;" NL "  float data2;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL "  g_output.data1 = g_input.data1;" NL
			  "  g_output.data2 = g_input.data2;" NL "}";
}

class BasicStd430LayoutCase4VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c4(in_data);
	}
};

class BasicStd430LayoutCase4CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c4(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.6.5 BasicStd430LayoutCase5
//-----------------------------------------------------------------------------
const char* GetInput430c5(std::vector<GLubyte>& in_data)
{
	in_data.resize(8 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 3.0f;
	fp[2]	 = 5.0f;
	fp[3]	 = 7.0f;
	fp[4]	 = 2.0f;
	fp[5]	 = 4.0f;
	fp[6]	 = 6.0f;
	fp[7]	 = 8.0f;

	return NL "layout(std430, binding = 0, row_major) buffer Input {" NL "  mat4x2 data0;" NL "} g_input;" NL
			  "layout(std430, binding = 1, row_major) buffer Output {" NL "  mat4x2 data0;" NL "} g_output;" NL
			  "void main() {" NL "  g_output.data0 = g_input.data0;" NL "}";
}

class BasicStd430LayoutCase5VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c5(in_data);
	}
};

class BasicStd430LayoutCase5CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c5(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.6.6 BasicStd430LayoutCase6
//-----------------------------------------------------------------------------
const char* GetInput430c6(std::vector<GLubyte>& in_data)
{
	in_data.resize(92 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 4.0f;
	fp[4]	 = 5.0f;
	fp[5]	 = 0.0f;
	fp[6]	 = 6.0f;
	fp[7]	 = 7.0f;
	fp[8]	 = 8.0f;
	fp[9]	 = 9.0f;
	fp[10]	= 10.0f;
	fp[11]	= 11.0f;
	fp[12]	= 12.0f;
	fp[13]	= 0.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	fp[16]	= 13.0f;
	fp[17]	= 14.0f;
	fp[18]	= 15.0f;
	fp[19]	= 0.0f;
	fp[20]	= 16.0f;
	fp[21]	= 17.0f;
	fp[22]	= 18.0f;
	fp[23]	= 0.0f;
	fp[24]	= 19.0f;
	fp[25]	= 20.0f;
	fp[26]	= 21.0f;
	fp[27]	= 22.0f;
	fp[28]	= 23.0f;
	fp[29]	= 24.0f;
	fp[30]	= 25.0f;
	fp[31]	= 26.0f;
	fp[32]	= 27.0f;
	fp[33]	= 28.0f;
	fp[34]	= 0.0f;
	fp[35]	= 0.0f;
	fp[36]	= 29.0f;
	fp[37]	= 30.0f;
	fp[38]	= 31.0f;
	fp[39]	= 0.0f;
	fp[40]	= 32.0f;
	fp[41]	= 33.0f;
	fp[42]	= 34.0f;
	fp[43]	= 0.0f;
	fp[44]	= 35.0f;
	fp[45]	= 36.0f;
	fp[46]	= 37.0f;
	fp[47]	= 0.0f;
	fp[48]	= 38.0f;
	fp[49]	= 39.0f;
	fp[50]	= 40.0f;
	fp[51]	= 0.0f;
	fp[52]	= 41.0f;
	fp[53]	= 42.0f;
	fp[54]	= 43.0f;
	fp[55]	= 0.0f;
	fp[56]	= 44.0f;
	fp[57]	= 45.0f;
	fp[58]	= 46.0f;
	fp[59]	= 0.0f;
	fp[60]	= 47.0f;
	fp[61]	= 48.0f;
	fp[62]	= 49.0f;
	fp[63]	= 50.0f;
	fp[64]	= 51.0f;
	fp[65]	= 52.0f;
	fp[66]	= 53.0f;
	fp[67]	= 54.0f;
	fp[68]	= 55.0f;
	fp[69]	= 56.0f;
	fp[70]	= 57.0f;
	fp[71]	= 58.0f;
	fp[72]	= 59.0f;
	fp[73]	= 60.0f;
	fp[74]	= 61.0f;
	fp[75]	= 62.0f;
	fp[76]	= 63.0f;
	fp[77]	= 64.0f;
	fp[78]	= 65.0f;
	fp[79]	= 66.0f;
	fp[80]	= 67.0f;
	fp[81]	= 68.0f;
	fp[82]	= 69.0f;
	fp[83]	= 70.0f;
	fp[84]	= 71.0f;
	fp[85]	= 72.0f;
	fp[86]	= 73.0f;
	fp[87]	= 74.0f;
	fp[88]	= 75.0f;
	fp[89]	= 76.0f;
	fp[90]	= 77.0f;
	fp[91]	= 78.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  float data0[2];" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  float data3[5];" NL "  vec3 data4[2];" NL "  float data5[2];" NL
			  "  mat2 data6[2];" NL "  mat3 data7[2];" NL "  mat4 data8[2];" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  float data0[2];" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  float data3[5];" NL "  vec3 data4[2];" NL "  float data5[2];" NL
			  "  mat2 data6[2];" NL "  mat3 data7[2];" NL "  mat4 data8[2];" NL "} g_output;" NL "void main() {" NL
			  "  for (int i = 0; i < g_input.data0.length(); ++i) g_output.data0[i] = g_input.data0[i];" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL
			  "  for (int i = 0; i < g_input.data3.length(); ++i) g_output.data3[i] = g_input.data3[i];" NL
			  "  for (int i = 0; i < g_input.data4.length(); ++i) g_output.data4[i] = g_input.data4[i];" NL
			  "  for (int i = 0; i < g_input.data5.length(); ++i) g_output.data5[i] = g_input.data5[i];" NL
			  "  for (int i = 0; i < g_input.data6.length(); ++i) g_output.data6[i] = g_input.data6[i];" NL
			  "  for (int i = 0; i < g_input.data7.length(); ++i) g_output.data7[i] = g_input.data7[i];" NL
			  "  for (int i = 0; i < g_input.data8.length(); ++i) g_output.data8[i] = g_input.data8[i];" NL "}";
}

class BasicStd430LayoutCase6VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c6(in_data);
	}
};

class BasicStd430LayoutCase6CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c6(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.6.7 BasicStd430LayoutCase7
//-----------------------------------------------------------------------------
const char* GetInput430c7(std::vector<GLubyte>& in_data)
{
	in_data.resize(36 * 4);
	int*   ip = reinterpret_cast<int*>(&in_data[0]);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	ip[0]	 = 1;
	ip[1]	 = 0;
	ip[2]	 = 2;
	ip[3]	 = 3;
	fp[4]	 = 4.0f;
	fp[5]	 = 0.0f;
	fp[6]	 = 0.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 5.0f;
	fp[9]	 = 6.0f;
	fp[10]	= 7.0f;
	fp[11]	= 0.0f;
	fp[12]	= 8.0f;
	fp[13]	= 0.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	fp[16]	= 9.0f;
	fp[17]	= 10.0f;
	fp[18]	= 11.0f;
	fp[19]	= 0.0f;
	ip[20]	= 12;
	ip[21]	= 13;
	ip[22]	= 14;
	ip[23]	= 15;
	fp[24]	= 16.0f;
	fp[25]	= 0.0f;
	fp[26]	= 0.0f;
	fp[27]	= 0.0f;
	fp[28]	= 17.0f;
	fp[29]	= 18.0f;
	fp[30]	= 19.0f;
	fp[31]	= 0.0f;
	ip[32]	= 20;
	ip[33]	= 21;
	ip[34]	= 22;
	ip[35]	= 23;

	return NL "struct Struct0 {" NL "  ivec2 m0;" NL "};" NL "struct Struct1 {" NL "  vec3 m0;" NL "};" NL
			  "struct Struct3 {" NL "  int m0;" NL "};" NL "struct Struct2 {" NL "  float m0;" NL "  Struct1 m1;" NL
			  "  Struct0 m2;" NL "  int m3;" NL "  Struct3 m4;" NL "};" NL
			  "layout(std430, binding = 0) buffer Input {" NL "  int data0;" NL "  Struct0 data1;" NL
			  "  float data2;" NL "  Struct1 data3;" NL "  Struct2 data4[2];" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  int data0;" NL "  Struct0 data1;" NL
			  "  float data2;" NL "  Struct1 data3;" NL "  Struct2 data4[2];" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL "  g_output.data1 = g_input.data1;" NL
			  "  g_output.data2 = g_input.data2;" NL "  g_output.data3 = g_input.data3;" NL
			  "  for (int i = 0; i < g_input.data4.length(); ++i) g_output.data4[i] = g_input.data4[i];" NL "}";
}

class BasicStd430LayoutCase7VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c7(in_data);
	}
};

class BasicStd430LayoutCase7CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput430c7(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.7.1 BasicStd140LayoutCase1
//-----------------------------------------------------------------------------
const char* GetInput140c1(std::vector<GLubyte>& in_data)
{
	in_data.resize(8 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 0.0f;
	fp[2]	 = 0.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 2.0f;

	return NL "layout(std140, binding = 0) buffer Input {" NL "  float data0[2];" NL "} g_input;" NL
			  "layout(std140, binding = 1) buffer Output {" NL "  float data0[2];" NL "} g_output;" NL
			  "void main() {" NL
			  "  for (int i = 0; i < g_input.data0.length(); ++i) g_output.data0[i] = g_input.data0[i];" NL "}";
}

class BasicStd140LayoutCase1VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c1(in_data);
	}
};

class BasicStd140LayoutCase1CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c1(in_data);
	}
};
//-----------------------------------------------------------------------------
// 1.7.2 BasicStd140LayoutCase2
//-----------------------------------------------------------------------------
const char* GetInput140c2(std::vector<GLubyte>& in_data)
{
	in_data.resize(20 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	int*   ip = reinterpret_cast<int*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 0.0f;
	fp[2]	 = 0.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 2.0f;
	fp[5]	 = 0.0f;
	fp[6]	 = 0.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 3.0f;
	fp[9]	 = 0.0f;
	fp[10]	= 0.0f;
	fp[11]	= 0.0f;
	fp[12]	= 4.0f;
	fp[13]	= 0.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	ip[16]	= 5;
	ip[17]	= 6;

	return NL "layout(std140, binding = 0) buffer Input {" NL "  float data0;" NL "  float data1[3];" NL
			  "  ivec2 data2;" NL "} g_input;" NL "layout(std140, binding = 1) buffer Output {" NL "  float data0;" NL
			  "  float data1[3];" NL "  ivec2 data2;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL "}";
}

class BasicStd140LayoutCase2VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c2(in_data);
	}
};

class BasicStd140LayoutCase2CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c2(in_data);
	}
};
//-----------------------------------------------------------------------------
// 1.7.3 BasicStd140LayoutCase3
//-----------------------------------------------------------------------------
const char* GetInput140c3(std::vector<GLubyte>& in_data)
{
	in_data.resize(32 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 0.0f;
	fp[2]	 = 0.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 2.0f;
	fp[5]	 = 0.0f;
	fp[6]	 = 0.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 3.0f;
	fp[9]	 = 0.0f;
	fp[10]	= 0.0f;
	fp[11]	= 0.0f;
	fp[12]	= 4.0f;
	fp[13]	= 0.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	fp[16]	= 5.0f;
	fp[17]	= 6.0f;
	fp[18]	= 0.0f;
	fp[19]	= 0.0f;
	fp[20]	= 7.0f;
	fp[21]	= 8.0f;
	fp[22]	= 9.0f;
	fp[23]	= 0.0f;
	fp[24]	= 10.0f;
	fp[25]	= 11.0f;
	fp[26]	= 12.0f;
	fp[27]	= 0.0f;
	fp[28]	= 13.0f;

	return NL "layout(std140, binding = 0) buffer Input {" NL "  float data0;" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  readonly vec3 data3[2];" NL "  float data4;" NL "} g_input;" NL
			  "layout(std140, binding = 1) buffer Output {" NL "  float data0;" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  vec3 data3[2];" NL "  float data4;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL
			  "  for (int i = 0; i < g_input.data3.length(); ++i) g_output.data3[i] = g_input.data3[i];" NL
			  "  g_output.data4 = g_input.data4;" NL "}";
}

class BasicStd140LayoutCase3VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c3(in_data);
	}
};

class BasicStd140LayoutCase3CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c3(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.7.4 BasicStd140LayoutCase4
//-----------------------------------------------------------------------------
const char* GetInput140c4(std::vector<GLubyte>& in_data)
{
	in_data.resize(28 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 0.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 3.0f;
	fp[5]	 = 4.0f;
	fp[6]	 = 0.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 5.0f;
	fp[9]	 = 6.0f;
	fp[10]	= 0.0f;
	fp[11]	= 0.0f;
	fp[12]	= 7.0f;
	fp[13]	= 8.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	fp[16]	= 9.0f;
	fp[17]	= 10.0f;
	fp[18]	= 11.0f;
	fp[19]	= 0.0f;
	fp[20]	= 12.0f;
	fp[21]	= 13.0f;
	fp[22]	= 14.0f;
	fp[23]	= 0.0f;
	fp[24]	= 15.0f;

	return NL "layout(std140, binding = 0) buffer Input {" NL "  mat4x2 data0;" NL "  mat2x3 data1;" NL
			  "  float data2;" NL "} g_input;" NL "layout(std140, binding = 1) buffer Output {" NL "  mat4x2 data0;" NL
			  "  mat2x3 data1;" NL "  float data2;" NL "} g_output;" NL "void main() {" NL
			  "  g_output.data0 = g_input.data0;" NL "  g_output.data1 = g_input.data1;" NL
			  "  g_output.data2 = g_input.data2;" NL "}";
}

class BasicStd140LayoutCase4VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c4(in_data);
	}
};

class BasicStd140LayoutCase4CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c4(in_data);
	}
};
//-----------------------------------------------------------------------------
// 1.7.5 BasicStd140LayoutCase5
//-----------------------------------------------------------------------------
const char* GetInput140c5(std::vector<GLubyte>& in_data)
{
	in_data.resize(8 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 2.0f;
	fp[2]	 = 3.0f;
	fp[3]	 = 4.0f;
	fp[4]	 = 5.0f;
	fp[5]	 = 6.0f;
	fp[6]	 = 7.0f;
	fp[7]	 = 8.0f;

	return NL "layout(std140, binding = 0, row_major) buffer Input {" NL "  mat4x2 data0;" NL "} g_input;" NL
			  "layout(std140, binding = 1, row_major) buffer Output {" NL "  mat4x2 data0;" NL "} g_output;" NL
			  "void main() {" NL "  g_output.data0 = g_input.data0;" NL "}";
}

class BasicStd140LayoutCase5VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c5(in_data);
	}
};

class BasicStd140LayoutCase5CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c5(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.7.6 BasicStd140LayoutCase6
//-----------------------------------------------------------------------------
const char* GetInput140c6(std::vector<GLubyte>& in_data)
{
	in_data.resize(96 * 4);
	float* fp = reinterpret_cast<float*>(&in_data[0]);
	fp[0]	 = 1.0f;
	fp[1]	 = 0.0f;
	fp[2]	 = 0.0f;
	fp[3]	 = 0.0f;
	fp[4]	 = 2.0f;
	fp[5]	 = 0.0f;
	fp[6]	 = 0.0f;
	fp[7]	 = 0.0f;
	fp[8]	 = 3.0f;
	fp[9]	 = 0.0f;
	fp[10]	= 0.0f;
	fp[11]	= 0.0f;
	fp[12]	= 4.0f;
	fp[13]	= 0.0f;
	fp[14]	= 0.0f;
	fp[15]	= 0.0f;
	fp[16]	= 5.0f;
	fp[17]	= 0.0f;
	fp[18]	= 0.0f;
	fp[19]	= 0.0f;
	fp[20]	= 6.0f;
	fp[21]	= 7.0f;
	fp[22]	= 8.0f;
	fp[23]	= 9.0f;
	fp[24]	= 10.0f;
	fp[25]	= 11.0f;
	fp[26]	= 0.0f;
	fp[27]	= 0.0f;
	fp[28]	= 12.0f;
	fp[29]	= 13.0f;
	fp[30]	= 0.0f;
	fp[31]	= 0.0f;
	fp[32]	= 14.0f;
	fp[33]	= 15.0f;
	fp[34]	= 0.0f;
	fp[35]	= 0.0f;
	fp[36]	= 16.0f;
	fp[37]	= 17.0f;
	fp[38]	= 0.0f;
	fp[39]	= 0.0f;
	fp[40]	= 18.0f;
	fp[41]	= 19.0f;
	fp[42]	= 20.0f;
	fp[43]	= 0.0f;
	fp[44]	= 21.0f;
	fp[45]	= 22.0f;
	fp[46]	= 23.0f;
	fp[47]	= 0.0f;
	fp[48]	= 24.0f;
	fp[49]	= 25.0f;
	fp[50]	= 26.0f;
	fp[51]	= 0.0f;
	fp[52]	= 27.0f;
	fp[53]	= 28.0f;
	fp[54]	= 29.0f;
	fp[55]	= 0.0f;
	fp[56]	= 30.0f;
	fp[57]	= 31.0f;
	fp[58]	= 32.0f;
	fp[59]	= 0.0f;
	fp[60]	= 33.0f;
	fp[61]	= 34.0f;
	fp[62]	= 35.0f;
	fp[63]	= 0.0f;
	fp[64]	= 36.0f;
	fp[65]	= 37.0f;
	fp[66]	= 38.0f;
	fp[67]	= 39.0f;
	fp[68]	= 40.0f;
	fp[69]	= 41.0f;
	fp[70]	= 42.0f;
	fp[71]	= 43.0f;
	fp[72]	= 44.0f;
	fp[73]	= 45.0f;
	fp[74]	= 46.0f;
	fp[75]	= 47.0f;
	fp[76]	= 48.0f;
	fp[77]	= 49.0f;
	fp[78]	= 50.0f;
	fp[79]	= 51.0f;
	fp[80]	= 52.0f;
	fp[81]	= 68.0f;
	fp[82]	= 69.0f;
	fp[83]	= 70.0f;
	fp[84]	= 56.0f;
	fp[85]	= 72.0f;
	fp[86]	= 73.0f;
	fp[87]	= 74.0f;
	fp[88]	= 60.0f;
	fp[89]	= 76.0f;
	fp[90]	= 77.0f;
	fp[91]	= 78.0f;
	fp[92]	= 64.0f;
	fp[93]	= 80.0f;
	fp[94]	= 81.0f;
	fp[95]	= 82.0f;

	return NL "layout(std140, binding = 0) buffer Input {" NL "  float data0[2];" NL "  float data1[3];" NL
			  "  vec2 data2;" NL "  vec2 data3;" NL "  mat2 data4[2];" NL "  mat3 data5[2];" NL "  mat4 data6[2];" NL
			  "} g_input;" NL "layout(std140, binding = 1) buffer Output {" NL "  float data0[2];" NL
			  "  float data1[3];" NL "  vec2 data2;" NL "  vec2 data3;" NL "  mat2 data4[2];" NL "  mat3 data5[2];" NL
			  "  mat4 data6[2];" NL "} g_output;" NL "void main() {" NL
			  "  for (int i = 0; i < g_input.data0.length(); ++i) g_output.data0[i] = g_input.data0[i];" NL
			  "  for (int i = 0; i < g_input.data1.length(); ++i) g_output.data1[i] = g_input.data1[i];" NL
			  "  g_output.data2 = g_input.data2;" NL "  g_output.data3 = g_input.data3;" NL
			  "  for (int i = 0; i < g_input.data4.length(); ++i) g_output.data4[i] = g_input.data4[i];" NL
			  "  for (int i = 0; i < g_input.data5.length(); ++i) g_output.data5[i] = g_input.data5[i];" NL
			  "  for (int i = 0; i < g_input.data6.length(); ++i) g_output.data6[i] = g_input.data6[i];" NL "}";
}

class BasicStd140LayoutCase6VS : public BasicStdLayoutBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c6(in_data);
	}
};

class BasicStd140LayoutCase6CS : public BasicStdLayoutBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data)
	{
		return GetInput140c6(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.8.1 BasicAtomicCase1
//-----------------------------------------------------------------------------
class BasicAtomicCase1 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[6];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		if (!SupportedInGS(2))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_in_position;" NL
			   "layout(std430, binding = 0) coherent buffer VSuint {" NL "  uint g_uint_out[4];" NL "};" NL
			   "layout(std430, binding = 1) coherent buffer VSint {" NL "  int data[4];" NL "} g_int_out;" NL
			   "uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			   "  gl_Position = g_in_position;" NL NL
			   "  if (atomicExchange(g_uint_out[gl_VertexID], g_uint_value[1]) != 0u) return;" NL
			   "  if (atomicAdd(g_uint_out[gl_VertexID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicMin(g_uint_out[gl_VertexID], g_uint_value[1]) != 3u) return;" NL
			   "  if (atomicMax(g_uint_out[gl_VertexID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicAnd(g_uint_out[gl_VertexID], g_uint_value[3]) != 2u) return;" NL
			   "  if (atomicOr(g_uint_out[gl_VertexID], g_uint_value[4]) != 0u) return;" NL
			   "  if (g_uint_value[0] > 0u) {" NL
			   "    if (atomicXor(g_uint_out[gl_VertexID], g_uint_value[5]) != 3u) return;" NL "  }" NL
			   "  if (atomicCompSwap(g_uint_out[gl_VertexID], g_uint_value[6], g_uint_value[7]) != 2u) {" NL
			   "    g_uint_out[gl_VertexID] = 1u;" NL "    return;" NL "  }" NL NL
			   "  if (atomicExchange(g_int_out.data[gl_VertexID], 1) != 0) return;" NL
			   "  if (atomicAdd(g_int_out.data[gl_VertexID], 2) != 1) return;" NL
			   "  if (atomicMin(g_int_out.data[gl_VertexID], 1) != 3) return;" NL
			   "  if (atomicMax(g_int_out.data[gl_VertexID], 2) != 1) return;" NL
			   "  if (atomicAnd(g_int_out.data[gl_VertexID], 0x1) != 2) return;" NL
			   "  if (atomicOr(g_int_out.data[gl_VertexID], 0x3) != 0) return;" NL
			   "  if (atomicXor(g_int_out.data[gl_VertexID], 0x1) != 3) return;" NL
			   "  if (atomicCompSwap(g_int_out.data[gl_VertexID], 0x2, 0x7) != 2) {" NL
			   "    g_int_out.data[gl_VertexID] = 1;" NL "    return;" NL "  }" NL "}";

		const char* const glsl_gs = NL
			"layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL
			"layout(std430, binding = 2) coherent buffer GSuint {" NL "  uint data[4];" NL "} g_uint_gs;" NL
			"layout(std430, binding = 3) coherent buffer GSint {" NL "  int data[4];" NL "} g_int_gs;" NL
			"uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			"  gl_Position = gl_in[0].gl_Position;" NL "  gl_PrimitiveID = gl_PrimitiveIDIn;" NL "  EmitVertex();" NL NL
			"  if (atomicExchange(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[1]) != 0u) return;" NL
			"  if (atomicAdd(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[2]) != 1u) return;" NL
			"  if (atomicMin(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[1]) != 3u) return;" NL
			"  if (atomicMax(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[2]) != 1u) return;" NL
			"  if (atomicAnd(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[3]) != 2u) return;" NL
			"  if (atomicOr(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[4]) != 0u) return;" NL
			"  if (g_uint_value[0] > 0u) {" NL
			"    if (atomicXor(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[5]) != 3u) return;" NL "  }" NL
			"  if (atomicCompSwap(g_uint_gs.data[gl_PrimitiveIDIn], g_uint_value[6], g_uint_value[7]) != 2u) {" NL
			"    g_uint_gs.data[gl_PrimitiveIDIn] = 1u;" NL "    return;" NL "  }" NL NL
			"  if (atomicExchange(g_int_gs.data[gl_PrimitiveIDIn], 1) != 0) return;" NL
			"  if (atomicAdd(g_int_gs.data[gl_PrimitiveIDIn], 2) != 1) return;" NL
			"  if (atomicMin(g_int_gs.data[gl_PrimitiveIDIn], 1) != 3) return;" NL
			"  if (atomicMax(g_int_gs.data[gl_PrimitiveIDIn], 2) != 1) return;" NL
			"  if (atomicAnd(g_int_gs.data[gl_PrimitiveIDIn], 0x1) != 2) return;" NL
			"  if (atomicOr(g_int_gs.data[gl_PrimitiveIDIn], 0x3) != 0) return;" NL
			"  if (atomicXor(g_int_gs.data[gl_PrimitiveIDIn], 0x1) != 3) return;" NL
			"  if (atomicCompSwap(g_int_gs.data[gl_PrimitiveIDIn], 0x2, 0x7) != 2) {" NL
			"    g_int_gs.data[gl_PrimitiveIDIn] = 1;" NL "    return;" NL "  }" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "layout(std430, binding = 4) coherent buffer FSuint {" NL
			   "  uint data[4];" NL "} g_uint_fs;" NL "layout(std430, binding = 5) coherent buffer FSint {" NL
			   "  int data[4];" NL "} g_int_fs;" NL
			   "uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			   "  g_fs_out = vec4(0, 1, 0, 1);" NL NL
			   "  if (atomicExchange(g_uint_fs.data[gl_PrimitiveID], g_uint_value[1]) != 0u) return;" NL
			   "  if (atomicAdd(g_uint_fs.data[gl_PrimitiveID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicMin(g_uint_fs.data[gl_PrimitiveID], g_uint_value[1]) != 3u) return;" NL
			   "  if (atomicMax(g_uint_fs.data[gl_PrimitiveID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicAnd(g_uint_fs.data[gl_PrimitiveID], g_uint_value[3]) != 2u) return;" NL
			   "  if (atomicOr(g_uint_fs.data[gl_PrimitiveID], g_uint_value[4]) != 0u) return;" NL
			   "  if (g_uint_value[0] > 0u) {" NL
			   "    if (atomicXor(g_uint_fs.data[gl_PrimitiveID], g_uint_value[5]) != 3u) return;" NL "  }" NL
			   "  if (atomicCompSwap(g_uint_fs.data[gl_PrimitiveID], g_uint_value[6], g_uint_value[7]) != 2u) {" NL
			   "    g_uint_fs.data[gl_PrimitiveID] = 1u;" NL "    return;" NL "  }" NL NL
			   "  if (atomicExchange(g_int_fs.data[gl_PrimitiveID], 1) != 0) return;" NL
			   "  if (atomicAdd(g_int_fs.data[gl_PrimitiveID], 2) != 1) return;" NL
			   "  if (atomicMin(g_int_fs.data[gl_PrimitiveID], 1) != 3) return;" NL
			   "  if (atomicMax(g_int_fs.data[gl_PrimitiveID], 2) != 1) return;" NL
			   "  if (atomicAnd(g_int_fs.data[gl_PrimitiveID], 0x1) != 2) return;" NL
			   "  if (atomicOr(g_int_fs.data[gl_PrimitiveID], 0x3) != 0) return;" NL
			   "  if (atomicXor(g_int_fs.data[gl_PrimitiveID], 0x1) != 3) return;" NL
			   "  if (atomicCompSwap(g_int_fs.data[gl_PrimitiveID], 0x2, 0x7) != 2) {" NL
			   "    g_int_fs.data[gl_PrimitiveID] = 1;" NL "    return;" NL "  }" NL "}";
		m_program = CreateProgram(glsl_vs, "", "", glsl_gs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(6, m_storage_buffer);
		for (GLuint i = 0; i < 6; ++i)
		{
			const int data[4] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		/* vertex buffer */
		{
			const float data[] = { -0.8f, -0.8f, 0.8f, -0.8f, -0.8f, 0.8f, 0.8f, 0.8f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		for (int ii = 0; ii < 3; ++ii)
		{
			/* uint data */
			{
				GLuint data[4];
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[ii * 2 + 0]);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
				for (GLuint i = 0; i < 4; ++i)
				{
					if (data[i] != 7)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
															<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
			/* int data */
			{
				GLint data[4];
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[ii * 2 + 1]);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
				for (GLint i = 0; i < 4; ++i)
				{
					if (data[i] != 7)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
															<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(6, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicAtomicCase1CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "layout(std430, binding = 2) coherent buffer FSuint {" NL
			   "  uint data[4];" NL "} g_uint_fs;" NL "layout(std430, binding = 3) coherent buffer FSint {" NL
			   "  int data[4];" NL "} g_int_fs;" NL "uniform uint g_uint_value[8];" NL "void main() {" NL
			   "  if (atomicExchange(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[1]) != 0u) return;" NL
			   "  if (atomicAdd(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicMin(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[1]) != 3u) return;" NL
			   "  if (atomicMax(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicAnd(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[3]) != 2u) return;" NL
			   "  if (atomicOr(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[4]) != 0u) return;" NL
			   "  if (g_uint_value[0] > 0u) {" NL
			   "    if (atomicXor(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[5]) != 3u) return;" NL "  }" NL
			   "  if (atomicCompSwap(g_uint_fs.data[gl_LocalInvocationIndex], g_uint_value[6], g_uint_value[7]) != 2u) "
			   "{" NL "    g_uint_fs.data[gl_LocalInvocationIndex] = 1u;" NL "    return;" NL "  }" NL
			   "  if (atomicExchange(g_int_fs.data[gl_LocalInvocationIndex], 1) != 0) return;" NL
			   "  if (atomicAdd(g_int_fs.data[gl_LocalInvocationIndex], 2) != 1) return;" NL
			   "  if (atomicMin(g_int_fs.data[gl_LocalInvocationIndex], 1) != 3) return;" NL
			   "  if (atomicMax(g_int_fs.data[gl_LocalInvocationIndex], 2) != 1) return;" NL
			   "  if (atomicAnd(g_int_fs.data[gl_LocalInvocationIndex], 0x1) != 2) return;" NL
			   "  if (atomicOr(g_int_fs.data[gl_LocalInvocationIndex], 0x3) != 0) return;" NL
			   "  if (atomicXor(g_int_fs.data[gl_LocalInvocationIndex], 0x1) != 3) return;" NL
			   "  if (atomicCompSwap(g_int_fs.data[gl_LocalInvocationIndex], 0x2, 0x7) != 2) {" NL
			   "    g_int_fs.data[gl_LocalInvocationIndex] = 1;" NL "    return;" NL "  }" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);
		for (GLuint i = 0; i < 2; ++i)
		{
			const int data[4] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i + 2, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		glUseProgram(m_program);
		GLuint unif[8] = { 3, 1, 2, 1, 3, 1, 2, 7 };
		glUniform1uiv(glGetUniformLocation(m_program, "g_uint_value[0]"), 8, unif);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* uint data */
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			GLuint* data = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (GLuint i = 0; i < 4; ++i)
			{
				if (data[i] != 7)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
														<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* int data */
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			GLint* data = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (GLint i = 0; i < 4; ++i)
			{
				if (data[i] != 7)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
														<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.8.2 BasicAtomicCase2
//-----------------------------------------------------------------------------
class BasicAtomicCase2 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[4];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInTCS(2))
			return NOT_SUPPORTED;
		if (!SupportedInTES(2))
			return NOT_SUPPORTED;

		const char* const glsl_vs = NL "layout(location = 0) in vec4 g_in_position;" NL "void main() {" NL
									   "  gl_Position = g_in_position;" NL "}";

		const char* const glsl_tcs = NL
			"layout(vertices = 1) out;" NL "layout(std430, binding = 0) buffer TCSuint {" NL "  uint g_uint_out[1];" NL
			"};" NL "layout(std430, binding = 1) buffer TCSint {" NL "  int data[1];" NL "} g_int_out;" NL
			"uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			"  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;" NL
			"  if (gl_InvocationID == 0) {" NL "    gl_TessLevelInner[0] = 1.0;" NL "    gl_TessLevelInner[1] = 1.0;" NL
			"    gl_TessLevelOuter[0] = 1.0;" NL "    gl_TessLevelOuter[1] = 1.0;" NL
			"    gl_TessLevelOuter[2] = 1.0;" NL "    gl_TessLevelOuter[3] = 1.0;" NL "  }" NL
			"  if (atomicAdd(g_uint_out[gl_InvocationID], g_uint_value[0]) != 0u) return;" NL
			"  if (atomicExchange(g_uint_out[gl_InvocationID], g_uint_value[0]) != 3u) return;" NL
			"  if (atomicMin(g_uint_out[gl_InvocationID], g_uint_value[1]) != 3u) return;" NL
			"  if (atomicMax(g_uint_out[gl_InvocationID], g_uint_value[2]) != 1u) return;" NL
			"  if (atomicAnd(g_uint_out[gl_InvocationID], g_uint_value[3]) != 2u) return;" NL
			"  if (atomicOr(g_uint_out[gl_InvocationID], g_uint_value[4]) != 0u) return;" NL
			"  if (g_uint_value[0] > 0u) {" NL
			"    if (atomicXor(g_uint_out[gl_InvocationID], g_uint_value[5]) != 3u) return;" NL "  }" NL
			"  if (atomicCompSwap(g_uint_out[gl_InvocationID], g_uint_value[6], g_uint_value[7]) != 2u) {" NL
			"    g_uint_out[gl_InvocationID] = 1u;" NL "    return;" NL "  }" NL NL
			"  if (atomicAdd(g_int_out.data[gl_InvocationID], 3) != 0) return;" NL
			"  if (atomicExchange(g_int_out.data[gl_InvocationID], 3) != 3) return;" NL
			"  if (atomicMin(g_int_out.data[gl_InvocationID], 1) != 3) return;" NL
			"  if (atomicMax(g_int_out.data[gl_InvocationID], 2) != 1) return;" NL
			"  if (atomicAnd(g_int_out.data[gl_InvocationID], 0x1) != 2) return;" NL
			"  if (atomicOr(g_int_out.data[gl_InvocationID], 0x3) != 0) return;" NL
			"  if (atomicXor(g_int_out.data[gl_InvocationID], 0x1) != 3) return;" NL
			"  if (atomicCompSwap(g_int_out.data[gl_InvocationID], 0x2, 0x7) != 2) {" NL
			"    g_int_out.data[gl_InvocationID] = 1;" NL "    return;" NL "  }" NL "}";

		const char* const glsl_tes =
			NL "layout(quads, point_mode) in;" NL "layout(std430, binding = 2) buffer TESuint {" NL "  uint data[1];" NL
			   "} g_uint_tes;" NL "layout(std430, binding = 3) buffer TESint {" NL "  int data[1];" NL "} g_int_tes;" NL
			   "uniform uint g_uint_value[8] = uint[8](3u, 1u, 2u, 0x1u, 0x3u, 0x1u, 0x2u, 0x7u);" NL "void main() {" NL
			   "  gl_Position = gl_in[0].gl_Position;" NL NL
			   "  if (atomicExchange(g_uint_tes.data[gl_PrimitiveID], g_uint_value[1]) != 0u) return;" NL
			   "  if (atomicAdd(g_uint_tes.data[gl_PrimitiveID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicMin(g_uint_tes.data[gl_PrimitiveID], g_uint_value[1]) != 3u) return;" NL
			   "  if (atomicMax(g_uint_tes.data[gl_PrimitiveID], g_uint_value[2]) != 1u) return;" NL
			   "  if (atomicAnd(g_uint_tes.data[gl_PrimitiveID], g_uint_value[3]) != 2u) return;" NL
			   "  if (atomicOr(g_uint_tes.data[gl_PrimitiveID], g_uint_value[4]) != 0u) return;" NL
			   "  if (g_uint_value[0] > 0u) {" NL
			   "    if (atomicXor(g_uint_tes.data[gl_PrimitiveID], g_uint_value[5]) != 3u) return;" NL "  }" NL
			   "  if (atomicCompSwap(g_uint_tes.data[gl_PrimitiveID], g_uint_value[6], g_uint_value[7]) != 2u) {" NL
			   "    g_uint_tes.data[gl_PrimitiveID] = 1u;" NL "    return;" NL "  }" NL NL
			   "  if (atomicExchange(g_int_tes.data[gl_PrimitiveID], 1) != 0) return;" NL
			   "  if (atomicAdd(g_int_tes.data[gl_PrimitiveID], 2) != 1) return;" NL
			   "  if (atomicMin(g_int_tes.data[gl_PrimitiveID], 1) != 3) return;" NL
			   "  if (atomicMax(g_int_tes.data[gl_PrimitiveID], 2) != 1) return;" NL
			   "  if (atomicAnd(g_int_tes.data[gl_PrimitiveID], 0x1) != 2) return;" NL
			   "  if (atomicOr(g_int_tes.data[gl_PrimitiveID], 0x3) != 0) return;" NL
			   "  if (atomicXor(g_int_tes.data[gl_PrimitiveID], 0x1) != 3) return;" NL
			   "  if (atomicCompSwap(g_int_tes.data[gl_PrimitiveID], 0x2, 0x7) != 2) {" NL
			   "    g_int_tes.data[gl_PrimitiveID] = 1;" NL "    return;" NL "  }" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "void main() {" NL "  g_fs_out = vec4(0, 1, 0, 1);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_tcs, glsl_tes, "", glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_storage_buffer);
		for (GLuint i = 0; i < 4; ++i)
		{
			const int data[1] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		/* vertex buffer */
		{
			const float data[2] = { 0.0f, 0.0f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glPatchParameteri(GL_PATCH_VERTICES, 1);
		glDrawArrays(GL_PATCHES, 0, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		for (int ii = 0; ii < 2; ++ii)
		{
			/* uint data */
			{
				GLuint data[1];
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[ii * 2 + 0]);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
				for (GLuint i = 0; i < 1; ++i)
				{
					if (data[i] != 7)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
															<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
			/* int data */
			{
				GLint data[1];
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[ii * 2 + 1]);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
				for (GLint i = 0; i < 1; ++i)
				{
					if (data[i] != 7)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
															<< data[i] << " should be 7." << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 1.8.3 BasicAtomicCase3
//-----------------------------------------------------------------------------
class BasicAtomicCase3 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		m_vertex_array   = 0;
		m_vertex_buffer  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		if (!SupportedInGS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_in_position;" NL "layout(std430, binding = 0) buffer Buffer {" NL
			   "  uvec4 u[4];" NL "  ivec3 i[4];" NL "} g_vs_buffer;" NL "void main() {" NL
			   "  gl_Position = g_in_position;" NL "  atomicAdd(g_vs_buffer.u[0].x, g_vs_buffer.u[gl_VertexID][1]);" NL
			   "  atomicAdd(g_vs_buffer.u[0][0], g_vs_buffer.u[gl_VertexID].z);" NL
			   "  atomicAdd(g_vs_buffer.i[0].x, g_vs_buffer.i[gl_VertexID][1]);" NL
			   "  atomicAdd(g_vs_buffer.i[0][0], g_vs_buffer.i[gl_VertexID].z);" NL "}";

		const char* const glsl_gs = NL
			"layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL
			"layout(std430, binding = 0) buffer Buffer {" NL "  uvec4 u[4];" NL "  ivec3 i[4];" NL "} g_gs_buffer;" NL
			"void main() {" NL "  gl_Position = gl_in[0].gl_Position;" NL "  gl_PrimitiveID = gl_PrimitiveIDIn;" NL
			"  EmitVertex();" NL "  atomicAdd(g_gs_buffer.u[0].x, g_gs_buffer.u[gl_PrimitiveIDIn][1]);" NL
			"  atomicAdd(g_gs_buffer.i[0].x, g_gs_buffer.i[gl_PrimitiveIDIn][1]);" NL "}";

		const char* const glsl_fs = NL
			"layout(location = 0) out vec4 g_fs_out;" NL "layout(std430, binding = 0) buffer Buffer {" NL
			"  uvec4 u[4];" NL "  ivec3 i[4];" NL "} g_fs_buffer;" NL "void main() {" NL
			"  g_fs_out = vec4(0, 1, 0, 1);" NL "  atomicAdd(g_fs_buffer.u[0].x, g_fs_buffer.u[gl_PrimitiveID][1]);" NL
			"  atomicAdd(g_fs_buffer.i[0].x, g_fs_buffer.i[gl_PrimitiveID][1]);" NL "}";

		m_program = CreateProgram(glsl_vs, "", "", glsl_gs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		/* init storage buffer */
		{
			glGenBuffers(1, &m_storage_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(int) * 4, NULL, GL_DYNAMIC_DRAW);
			ivec4* ptr = reinterpret_cast<ivec4*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
			if (!ptr)
				return ERROR;
			for (int i = 0; i < 4; ++i)
			{
				ptr[i * 2]	 = ivec4(0, 1, 2, 0);
				ptr[i * 2 + 1] = ivec4(0, 1, 2, 0);
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		/* init vertex buffer */
		{
			const float data[] = { -0.8f, -0.8f, 0.8f, -0.8f, -0.8f, 0.8f, 0.8f, 0.8f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GLuint u;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4, &u);
		if (u != 20)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Data at offset 0 is " << u << " should be 20." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		GLint i;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(GLuint) * 4, 4, &i);
		if (i != 20)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Data at offset 0 is " << i << " should be 20." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicAtomicCase3CS : public ShaderStorageBufferObjectBase
{
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
			NL "layout(local_size_y = 4) in;" NL "layout(std430) coherent buffer Buffer {" NL "  uvec4 u[4];" NL
			   "  ivec3 i[4];" NL "} g_fs_buffer;" NL "void main() {" NL
			   "  atomicAdd(g_fs_buffer.u[0].x, g_fs_buffer.u[gl_LocalInvocationID.y][2]);" NL
			   "  atomicAdd(g_fs_buffer.i[0].x, 2 * g_fs_buffer.i[gl_LocalInvocationID.y][1]);" NL
			   "  atomicAdd(g_fs_buffer.u[0].x, g_fs_buffer.u[gl_LocalInvocationID.y].z);" NL
			   "  atomicAdd(g_fs_buffer.i[0].x, 2 * g_fs_buffer.i[gl_LocalInvocationID.y].y);" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		/* init storage buffer */
		{
			glGenBuffers(1, &m_storage_buffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(int) * 4, NULL, GL_DYNAMIC_DRAW);
			ivec4* ptr = reinterpret_cast<ivec4*>(
				glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8 * sizeof(int) * 4, GL_MAP_WRITE_BIT));
			if (!ptr)
				return ERROR;
			for (int i = 0; i < 4; ++i)
			{
				ptr[i * 2]	 = ivec4(0, 1, 2, 0);
				ptr[i * 2 + 1] = ivec4(0, 1, 2, 0);
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GLuint* u = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4, GL_MAP_READ_BIT);
		if (!u)
			return ERROR;
		if (*u != 16)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at offset 0 is " << *u
												<< " should be 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLint* i = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 64, 4, GL_MAP_READ_BIT);
		if (!i)
			return ERROR;
		if (*i != 16)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Data at offset 0 is " << *i
												<< " should be 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

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
//-----------------------------------------------------------------------------
// 1.8.4 BasicAtomicCase4
//-----------------------------------------------------------------------------

class BasicAtomicCase4 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_in_position;" NL "layout(std430, binding = 0) buffer Counters {" NL
			   "  uint g_uint_counter;" NL "  int g_int_counter;" NL "};" NL
			   "layout(std430, binding = 1) buffer Output {" NL "  uint udata[8];" NL "  int idata[8];" NL
			   "} g_output;" NL "void main() {" NL "  gl_Position = g_in_position;" NL
			   "  const uint uidx = atomicAdd(g_uint_counter, 1u);" NL
			   "  const int iidx = atomicAdd(g_int_counter, -1);" NL "  g_output.udata[uidx] = uidx;" NL
			   "  g_output.idata[iidx] = iidx;" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "layout(std430, binding = 0) buffer Counters {" NL
			   "  uint g_uint_counter;" NL "  int g_int_counter;" NL "};" NL
			   "layout(std430, binding = 1) buffer Output {" NL "  uint udata[8];" NL "  int idata[8];" NL
			   "} g_output;" NL "void main() {" NL "  g_fs_out = vec4(0, 1, 0, 1);" NL
			   "  const uint uidx = atomicAdd(g_uint_counter, 1u);" NL
			   "  const int iidx = atomicAdd(g_int_counter, -1);" NL "  g_output.udata[uidx] = uidx;" NL
			   "  g_output.idata[iidx] = iidx;" NL "}";
		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);
		/* counter buffer */
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
			int* ptr = reinterpret_cast<int*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
			if (!ptr)
				return ERROR;
			*ptr++ = 0;
			*ptr++ = 7;
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* output buffer */
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 16 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
		}
		/* vertex buffer */
		{
			const float data[] = { -0.8f, -0.8f, 0.8f, -0.8f, -0.8f, 0.8f, 0.8f, 0.8f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GLuint udata[8];
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(udata), udata);
		for (GLuint i = 0; i < 8; ++i)
		{
			if (udata[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
													<< udata[i] << " should be " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		GLint idata[8];
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(udata), sizeof(idata), idata);
		for (GLint i = 0; i < 8; ++i)
		{
			if (idata[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
													<< idata[i] << " should be " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicAtomicCase4CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 2, local_size_y = 2, local_size_z = 2) in;" NL
			   "layout(std430, binding = 0) coherent buffer Counters {" NL "  uint g_uint_counter;" NL
			   "  int g_int_counter;" NL "};" NL "layout(std430, binding = 1) buffer Output {" NL "  uint udata[8];" NL
			   "  int idata[8];" NL "} g_output;" NL "void main() {" NL
			   "  uint uidx = atomicAdd(g_uint_counter, 1u);" NL "  int iidx = atomicAdd(g_int_counter, -1);" NL
			   "  g_output.udata[uidx] = uidx;" NL "  g_output.idata[iidx] = iidx;" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);
		/* counter buffer */
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
			int* ptr = reinterpret_cast<int*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8, GL_MAP_WRITE_BIT));
			if (!ptr)
				return ERROR;
			*ptr++ = 0;
			*ptr++ = 7;
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* output buffer */
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, 16 * sizeof(int), NULL, GL_DYNAMIC_DRAW);
		}
		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GLuint* udata = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 32, GL_MAP_READ_BIT);
		if (!udata)
			return ERROR;
		for (GLuint i = 0; i < 8; ++i)
		{
			if (udata[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "uData at index " << i << " is "
													<< udata[i] << " should be " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLint* idata = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 32, 32, GL_MAP_READ_BIT);
		if (!idata)
			return ERROR;
		for (GLint i = 0; i < 8; ++i)
		{
			if (idata[i] != i)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "iData at index " << i << " is "
													<< idata[i] << " should be " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.9.x BasicStdLayoutBase2
//-----------------------------------------------------------------------------

class BasicStdLayoutBase2VS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[8];
	GLuint m_vertex_array;

	virtual const char* GetInput(std::vector<GLubyte> in_data[4]) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(8))
			return NOT_SUPPORTED;
		std::vector<GLubyte> in_data[4];
		const char*			 glsl_vs = GetInput(in_data);

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(8, m_buffer);

		for (GLuint i = 0; i < 4; ++i)
		{
			// input buffers
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[i].size(), &in_data[i][0], GL_STATIC_DRAW);

			// output buffers
			std::vector<GLubyte> out_data(in_data[i].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i + 4, m_buffer[i + 4]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[i].size(), &out_data[0], GL_STATIC_DRAW);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		bool status = true;
		for (int j = 0; j < 4; ++j)
		{
			std::vector<GLubyte> out_data(in_data[j].size());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[j + 4]);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data[j].size(), &out_data[0]);

			for (size_t i = 0; i < in_data[j].size(); ++i)
			{
				if (in_data[j][i] != out_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
						<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[j][i])
						<< tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(8, m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicStdLayoutBase2CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[8];

	virtual const char* GetInput(std::vector<GLubyte> in_data[4]) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		std::vector<GLubyte> in_data[4];
		std::stringstream	ss;
		GLint				 blocksCS;
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &blocksCS);
		if (blocksCS < 8)
			return NO_ERROR;
		ss << "layout(local_size_x = 1) in;\n" << GetInput(in_data);
		m_program = CreateProgramCS(ss.str());
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(8, m_buffer);

		for (GLuint i = 0; i < 4; ++i)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[i].size(), &in_data[i][0], GL_STATIC_DRAW);

			std::vector<GLubyte> out_data(in_data[i].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i + 4, m_buffer[i + 4]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[i].size(), &out_data[0], GL_STATIC_DRAW);
		}

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;
		for (int j = 0; j < 4; ++j)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[j + 4]);
			GLubyte* out_data =
				(GLubyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data[j].size(), GL_MAP_READ_BIT);
			if (!out_data)
				return ERROR;

			for (size_t i = 0; i < in_data[j].size(); ++i)
			{
				if (in_data[j][i] != out_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
						<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[j][i])
						<< tcu::TestLog::EndMessage;
					status = false;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(8, m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.9.1 BasicStdLayoutCase1
//-----------------------------------------------------------------------------
const char* GetInputC1(std::vector<GLubyte> in_data[4])
{
	for (int i = 0; i < 4; ++i)
	{
		in_data[i].resize(1 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[i][0]);
		fp[0]	 = static_cast<float>(i + 1) * 1.0f;
	}

	return NL "layout(std430, binding = 0) buffer Input {" NL "  float data0;" NL "} g_input[4];" NL
			  "layout(std430, binding = 4) buffer Output {" NL "  float data0;" NL "} g_output[4];" NL
			  "void main() {" NL "  for (int i = 0; i < 4; ++i) {" NL "    g_output[i].data0 = g_input[i].data0;" NL
			  "  }" NL "}";
}

class BasicStdLayoutCase1VS : public BasicStdLayoutBase2VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC1(in_data);
	}
};

class BasicStdLayoutCase1CS : public BasicStdLayoutBase2CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC1(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.9.2 BasicStdLayoutCase2
//-----------------------------------------------------------------------------
const char* GetInputC2(std::vector<GLubyte> in_data[4])
{
	/* input 0, std140 */
	{
		in_data[0].resize(12 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[0][0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 0.0f;
		fp[2]	 = 0.0f;
		fp[3]	 = 0.0f;
		fp[4]	 = 2.0f;
		fp[5]	 = 0.0f;
		fp[6]	 = 0.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 3.0f;
		fp[9]	 = 0.0f;
		fp[10]	= 0.0f;
		fp[11]	= 0.0f;
	}
	/* input 1, std430 */
	{
		in_data[1].resize(3 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[1][0]);
		fp[0]	 = 4.0f;
		fp[1]	 = 5.0f;
		fp[2]	 = 6.0f;
	}
	/* input 2, std140 */
	{
		in_data[2].resize(12 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[2][0]);
		fp[0]	 = 7.0f;
		fp[1]	 = 0.0f;
		fp[2]	 = 0.0f;
		fp[3]	 = 0.0f;
		fp[4]	 = 8.0f;
		fp[5]	 = 0.0f;
		fp[6]	 = 0.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 9.0f;
		fp[9]	 = 0.0f;
		fp[10]	= 0.0f;
		fp[11]	= 0.0f;
	}
	/* input 3, std430 */
	{
		in_data[3].resize(3 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[3][0]);
		fp[0]	 = 10.0f;
		fp[1]	 = 11.0f;
		fp[2]	 = 12.0f;
	}
	return NL "layout(std140, binding = 0) buffer Input0 {" NL "  float data0[3];" NL "} g_input0;" NL
			  "layout(std430, binding = 1) buffer Input1 {" NL "  float data0[3];" NL "} g_input1;" NL
			  "layout(std140, binding = 2) buffer Input2 {" NL "  float data0[3];" NL "} g_input2;" NL
			  "layout(std430, binding = 3) buffer Input3 {" NL "  float data0[3];" NL "} g_input3;" NL
			  "layout(std140, binding = 4) buffer Output0 {" NL "  float data0[3];" NL "} g_output0;" NL
			  "layout(std430, binding = 5) buffer Output1 {" NL "  float data0[3];" NL "} g_output1;" NL
			  "layout(std140, binding = 6) buffer Output2 {" NL "  float data0[3];" NL "} g_output2;" NL
			  "layout(std430, binding = 7) buffer Output3 {" NL "  float data0[3];" NL "} g_output3;" NL
			  "void main() {" NL
			  "  for (int i = 0; i < g_input0.data0.length(); ++i) g_output0.data0[i] = g_input0.data0[i];" NL
			  "  for (int i = 0; i < g_input1.data0.length(); ++i) g_output1.data0[i] = g_input1.data0[i];" NL
			  "  for (int i = 0; i < g_input2.data0.length(); ++i) g_output2.data0[i] = g_input2.data0[i];" NL
			  "  for (int i = 0; i < g_input3.data0.length(); ++i) g_output3.data0[i] = g_input3.data0[i];" NL "}";
}

class BasicStdLayoutCase2VS : public BasicStdLayoutBase2VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC2(in_data);
	}
};

class BasicStdLayoutCase2CS : public BasicStdLayoutBase2CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC2(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.9.3 BasicStdLayoutCase3
//-----------------------------------------------------------------------------
const char* GetInputC3(std::vector<GLubyte> in_data[4])
{
	/* input 0, std140 */
	{
		in_data[0].resize(62 * 4);
		float* fp							= reinterpret_cast<float*>(&in_data[0][0]);
		int*   ip							= reinterpret_cast<int*>(&in_data[0][0]);
		ip[0]								= 1;
		ip[1]								= 0;
		ip[2]								= 0;
		ip[3]								= 0;
		fp[4]								= 2.0f;
		fp[5]								= 0.0f;
		fp[6]								= 0.0f;
		fp[7]								= 0.0f;
		fp[8]								= 3.0f;
		fp[9]								= 0.0f;
		fp[10]								= 0.0f;
		fp[11]								= 0.0f;
		fp[12]								= 4.0f;
		fp[13]								= 0.0f;
		fp[14]								= 0.0f;
		fp[15]								= 0.0f;
		fp[16]								= 5.0f;
		fp[17]								= 0.0f;
		fp[18]								= 0.0f;
		fp[19]								= 0.0f;
		fp[20]								= 6.0f;
		fp[21]								= 0.0f;
		fp[22]								= 0.0f;
		fp[23]								= 0.0f;
		fp[24]								= 7.0f;
		fp[25]								= 8.0f;
		fp[26]								= 0.0f;
		fp[27]								= 0.0f;
		fp[28]								= 9.0f;
		fp[29]								= 10.0f;
		fp[30]								= 0.0f;
		fp[31]								= 0.0f;
		fp[32]								= 11.0f;
		fp[33]								= 12.0f;
		fp[34]								= 0.0f;
		fp[35]								= 0.0f;
		*reinterpret_cast<double*>(fp + 36) = 13.0;
		*reinterpret_cast<double*>(fp + 38) = 0.0;
		*reinterpret_cast<double*>(fp + 40) = 14.0;
		*reinterpret_cast<double*>(fp + 42) = 0.0;
		*reinterpret_cast<double*>(fp + 44) = 15.0;
		*reinterpret_cast<double*>(fp + 46) = 0.0;
		ip[48]								= 16;
		ip[49]								= 0;
		ip[50]								= 0;
		ip[51]								= 0;
		ip[52]								= 0;
		ip[53]								= 0;
		ip[54]								= 0;
		ip[55]								= 0;
		*reinterpret_cast<double*>(fp + 56) = 17.0;
		*reinterpret_cast<double*>(fp + 58) = 18.0;
		*reinterpret_cast<double*>(fp + 60) = 19.0;
	}
	/* input 1, std430 */
	{
		in_data[1].resize(32 * 4);
		float* fp							= reinterpret_cast<float*>(&in_data[1][0]);
		int*   ip							= reinterpret_cast<int*>(&in_data[1][0]);
		ip[0]								= 1;
		fp[1]								= 2.0f;
		fp[2]								= 3.0f;
		fp[3]								= 4.0f;
		fp[4]								= 5.0f;
		fp[5]								= 6.0f;
		fp[6]								= 7.0f;
		fp[7]								= 8.0f;
		fp[8]								= 9.0f;
		fp[9]								= 10.0f;
		fp[10]								= 11.0f;
		fp[11]								= 12.0f;
		*reinterpret_cast<double*>(fp + 12) = 13.0;
		*reinterpret_cast<double*>(fp + 14) = 14.0;
		*reinterpret_cast<double*>(fp + 16) = 15.0;
		ip[18]								= 16;
		ip[19]								= 0;
		*reinterpret_cast<double*>(fp + 20) = 0.0;
		*reinterpret_cast<double*>(fp + 22) = 0.0;
		*reinterpret_cast<double*>(fp + 24) = 17.0;
		*reinterpret_cast<double*>(fp + 26) = 18.0;
		*reinterpret_cast<double*>(fp + 28) = 19.0;
	}
	/* input 2, std140 */
	{
		in_data[2].resize(5 * 4);
		int* ip = reinterpret_cast<int*>(&in_data[2][0]);
		ip[0]   = 1;
		ip[1]   = 0;
		ip[2]   = 0;
		ip[3]   = 0;
		ip[4]   = 2;
	}
	/* input 3, std430 */
	{
		in_data[3].resize(2 * 4);
		int* ip = reinterpret_cast<int*>(&in_data[3][0]);
		ip[0]   = 1;
		ip[1]   = 2;
	}
	return NL "layout(std140, binding = 0) buffer Input0 {" NL "  int data0;" NL "  float data1[5];" NL
			  "  mat3x2 data2;" NL "  double data3;" NL "  double data4[2];" NL "  int data5;" NL "  dvec3 data6;" NL
			  "} g_input0;" NL "layout(std430, binding = 1) buffer Input1 {" NL "  int data0;" NL "  float data1[5];" NL
			  "  mat3x2 data2;" NL "  double data3;" NL "  double data4[2];" NL "  int data5;" NL "  dvec3 data6;" NL
			  "} g_input1;" NL "struct Struct0 {" NL "  int data0;" NL "};" NL
			  "layout(std140, binding = 2) buffer Input2 {" NL "  int data0;" NL "  Struct0 data1;" NL "} g_input2;" NL
			  "layout(std430, binding = 3) buffer Input3 {" NL "  int data0;" NL "  Struct0 data1;" NL "} g_input3;"

		NL "layout(std140, binding = 4) buffer Output0 {" NL "  int data0;" NL "  float data1[5];" NL
			  "  mat3x2 data2;" NL "  double data3;" NL "  double data4[2];" NL "  int data5;" NL "  dvec3 data6;" NL
			  "} g_output0;" NL "layout(std430, binding = 5) buffer Output1 {" NL "  int data0;" NL
			  "  float data1[5];" NL "  mat3x2 data2;" NL "  double data3;" NL "  double data4[2];" NL "  int data5;" NL
			  "  dvec3 data6;" NL "} g_output1;" NL "layout(std140, binding = 6) buffer Output2 {" NL "  int data0;" NL
			  "  Struct0 data1;" NL "} g_output2;" NL "layout(std430, binding = 7) buffer Output3 {" NL
			  "  int data0;" NL "  Struct0 data1;" NL "} g_output3;" NL "void main() {" NL
			  "  g_output0.data0 = g_input0.data0;" NL
			  "  for (int i = 0; i < g_input0.data1.length(); ++i) g_output0.data1[i] = g_input0.data1[i];" NL
			  "  g_output0.data2 = g_input0.data2;" NL "  g_output0.data3 = g_input0.data3;" NL
			  "  for (int i = 0; i < g_input0.data4.length(); ++i) g_output0.data4[i] = g_input0.data4[i];" NL
			  "  g_output0.data5 = g_input0.data5;" NL "  g_output0.data6 = g_input0.data6;"

		NL "  g_output1.data0 = g_input1.data0;" NL
			  "  for (int i = 0; i < g_input1.data1.length(); ++i) g_output1.data1[i] = g_input1.data1[i];" NL
			  "  g_output1.data2 = g_input1.data2;" NL "  g_output1.data3 = g_input1.data3;" NL
			  "  for (int i = 0; i < g_input1.data4.length(); ++i) g_output1.data4[i] = g_input1.data4[i];" NL
			  "  g_output1.data5 = g_input1.data5;" NL "  g_output1.data6 = g_input1.data6;"

		NL "  g_output2.data0 = g_input2.data0;" NL "  g_output2.data1 = g_input2.data1;"

		NL "  g_output3.data0 = g_input3.data0;" NL "  g_output3.data1 = g_input3.data1;" NL "}";
}

class BasicStdLayoutCase3VS : public BasicStdLayoutBase2VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC3(in_data);
	}
};

class BasicStdLayoutCase3CS : public BasicStdLayoutBase2CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC3(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.9.4 BasicStdLayoutCase4
//-----------------------------------------------------------------------------
const char* GetInputC4(std::vector<GLubyte> in_data[4])
{
	/* input 0, std140 */
	{
		in_data[0].resize(60 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[0][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[0][0]);
		ip[0]	 = 1;
		ip[1]	 = 0;
		ip[2]	 = 0;
		ip[3]	 = 0;
		ip[4]	 = 2;
		ip[5]	 = 3;
		ip[6]	 = 0;
		ip[7]	 = 0;
		ip[8]	 = 4;
		ip[9]	 = 5;
		ip[10]	= 0;
		ip[11]	= 0;
		fp[12]	= 6.0f;
		fp[13]	= 0.0f;
		fp[14]	= 0.0f;
		fp[15]	= 0.0f;
		fp[16]	= 7.0f;
		fp[17]	= 8.0f;
		fp[18]	= 0.0f;
		fp[19]	= 0.0f;
		ip[20]	= 9;
		ip[21]	= 10;
		ip[22]	= 11;
		ip[23]	= 0;
		fp[24]	= 12.0f;
		fp[25]	= 13.0f;
		fp[26]	= 0.0f;
		fp[27]	= 0.0f;
		ip[28]	= 14;
		ip[29]	= 15;
		ip[30]	= 16;
		ip[31]	= 0;
		fp[32]	= 17.0f;
		fp[33]	= 0.0f;
		fp[34]	= 0.0f;
		fp[35]	= 0.0f;
		ip[36]	= 18;
		ip[37]	= 0;
		ip[38]	= 0;
		ip[39]	= 0;
		ip[40]	= 19;
		ip[41]	= 20;
		ip[42]	= 0;
		ip[43]	= 0;
		ip[44]	= 21;
		ip[45]	= 0;
		ip[45]	= 0;
		ip[45]	= 0;
		fp[48]	= 22.0f;
		fp[49]	= 23.0f;
		fp[50]	= 0.0f;
		fp[51]	= 0.0f;
		ip[52]	= 24;
		ip[53]	= 25;
		ip[54]	= 26;
		ip[55]	= 0;
		fp[56]	= 27.0f;
	}
	/* input 1, std140 */
	{
		in_data[1].resize(60 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[1][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[1][0]);
		ip[0]	 = 101;
		ip[1]	 = 0;
		ip[2]	 = 0;
		ip[3]	 = 0;
		ip[4]	 = 102;
		ip[5]	 = 103;
		ip[6]	 = 0;
		ip[7]	 = 0;
		ip[8]	 = 104;
		ip[9]	 = 105;
		ip[10]	= 0;
		ip[11]	= 0;
		fp[12]	= 106.0f;
		fp[13]	= 0.0f;
		fp[14]	= 0.0f;
		fp[15]	= 0.0f;
		fp[16]	= 107.0f;
		fp[17]	= 108.0f;
		fp[18]	= 0.0f;
		fp[19]	= 0.0f;
		ip[20]	= 109;
		ip[21]	= 110;
		ip[22]	= 111;
		ip[23]	= 0;
		fp[24]	= 112.0f;
		fp[25]	= 113.0f;
		fp[26]	= 0.0f;
		fp[27]	= 0.0f;
		ip[28]	= 114;
		ip[29]	= 115;
		ip[30]	= 116;
		ip[31]	= 0;
		fp[32]	= 117.0f;
		fp[33]	= 0.0f;
		fp[34]	= 0.0f;
		fp[35]	= 0.0f;
		ip[36]	= 118;
		ip[37]	= 0;
		ip[38]	= 0;
		ip[39]	= 0;
		ip[40]	= 119;
		ip[41]	= 120;
		ip[42]	= 0;
		ip[43]	= 0;
		ip[44]	= 121;
		ip[45]	= 0;
		ip[45]	= 0;
		ip[45]	= 0;
		fp[48]	= 122.0f;
		fp[49]	= 123.0f;
		fp[50]	= 0.0f;
		fp[51]	= 0.0f;
		ip[52]	= 124;
		ip[53]	= 125;
		ip[54]	= 126;
		ip[55]	= 0;
		fp[56]	= 127.0f;
	}
	/* input 2, std430 */
	{
		in_data[2].resize(48 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[2][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[2][0]);
		ip[0]	 = 1000;
		ip[1]	 = 0;
		ip[2]	 = 1001;
		ip[3]	 = 1002;
		ip[4]	 = 1003;
		ip[5]	 = 1004;
		fp[6]	 = 1005.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 1006.0f;
		fp[9]	 = 1007.0f;
		fp[10]	= 0.0f;
		fp[11]	= 0.0f;
		ip[12]	= 1008;
		ip[13]	= 1009;
		ip[14]	= 1010;
		ip[15]	= 0;
		fp[16]	= 1011.0f;
		fp[17]	= 1012.0f;
		fp[18]	= 0.0f;
		fp[19]	= 0.0f;
		ip[20]	= 1013;
		ip[21]	= 1014;
		ip[22]	= 1015;
		ip[23]	= 0;
		fp[24]	= 1016.0f;
		fp[25]	= 0.0f;
		fp[26]	= 0.0f;
		fp[27]	= 0.0f;
		ip[28]	= 1017;
		ip[29]	= 0;
		ip[30]	= 1018;
		ip[31]	= 1019;
		ip[32]	= 1020;
		ip[33]	= 0;
		ip[34]	= 0;
		ip[35]	= 0;
		fp[36]	= 1021.0f;
		fp[37]	= 1022.0f;
		fp[38]	= 0.0f;
		fp[39]	= 0.0f;
		ip[40]	= 1023;
		ip[41]	= 1024;
		ip[42]	= 1025;
		ip[43]	= 0;
		fp[44]	= 1026.0f;
	}
	/* input 3, std430 */
	{
		in_data[3].resize(48 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[3][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[3][0]);
		ip[0]	 = 10000;
		ip[1]	 = 0;
		ip[2]	 = 10001;
		ip[3]	 = 10002;
		ip[4]	 = 10003;
		ip[5]	 = 10004;
		fp[6]	 = 10005.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 10006.0f;
		fp[9]	 = 10007.0f;
		fp[10]	= 0.0f;
		fp[11]	= 0.0f;
		ip[12]	= 10008;
		ip[13]	= 10009;
		ip[14]	= 10010;
		ip[15]	= 0;
		fp[16]	= 10011.0f;
		fp[17]	= 10012.0f;
		fp[18]	= 0.0f;
		fp[19]	= 0.0f;
		ip[20]	= 10013;
		ip[21]	= 10014;
		ip[22]	= 10015;
		ip[23]	= 0;
		fp[24]	= 10016.0f;
		fp[25]	= 0.0f;
		fp[26]	= 0.0f;
		fp[27]	= 0.0f;
		ip[28]	= 10017;
		ip[29]	= 0;
		ip[30]	= 10018;
		ip[31]	= 10019;
		ip[32]	= 10020;
		ip[33]	= 0;
		ip[34]	= 0;
		ip[35]	= 0;
		fp[36]	= 10021.0f;
		fp[37]	= 10022.0f;
		fp[38]	= 0.0f;
		fp[39]	= 0.0f;
		ip[40]	= 10023;
		ip[41]	= 10024;
		ip[42]	= 10025;
		ip[43]	= 0;
		fp[44]	= 10026.0f;
	}
	return NL
		"struct Struct0 {" NL "  ivec2 data0;" NL "};" NL "struct Struct1 {" NL "  vec2 data0;" NL "  ivec3 data1;" NL
		"};" NL "struct Struct2 {" NL "  int data0;" NL "  Struct0 data1;" NL "  int data2;" NL "  Struct1 data3;" NL
		"  float data4;" NL "};" NL "layout(std140, binding = 0) buffer Input01 {" NL "  int data0;" NL
		"  Struct0 data1[2];" NL "  float data2;" NL "  Struct1 data3[2];" NL "  float data4;" NL "  Struct2 data5;" NL
		"} g_input01[2];" NL "layout(std430, binding = 2) buffer Input23 {" NL "  int data0;" NL
		"  Struct0 data1[2];" NL "  float data2;" NL "  Struct1 data3[2];" NL "  float data4;" NL "  Struct2 data5;" NL
		"} g_input23[2];"

		NL "layout(std140, binding = 4) buffer Output01 {" NL "  int data0;" NL "  Struct0 data1[2];" NL
		"  float data2;" NL "  Struct1 data3[2];" NL "  float data4;" NL "  Struct2 data5;" NL "} g_output01[2];" NL
		"layout(std430, binding = 6) buffer Output23 {" NL "  int data0;" NL "  Struct0 data1[2];" NL
		"  float data2;" NL "  Struct1 data3[2];" NL "  float data4;" NL "  Struct2 data5;" NL "} g_output23[2];" NL NL
		"void main() {" NL "  for (int b = 0; b < g_input01.length(); ++b) {" NL
		"    g_output01[b].data0 = g_input01[b].data0;" NL
		"    for (int i = 0; i < g_input01[b].data1.length(); ++i) g_output01[b].data1[i] = g_input01[b].data1[i];" NL
		"    g_output01[b].data2 = g_input01[b].data2;" NL "    g_output01[b].data3[0] = g_input01[b].data3[0];" NL
		"    g_output01[b].data3[1] = g_input01[b].data3[1];" NL "    g_output01[b].data4 = g_input01[b].data4;" NL
		"  }" NL "  g_output01[0].data5 = g_input01[0].data5;" NL "  g_output01[1].data5 = g_input01[1].data5;" NL NL
		"  for (int b = 0; b < g_input23.length(); ++b) {" NL "    g_output23[b].data0 = g_input23[b].data0;" NL
		"    for (int i = 0; i < g_input23[b].data1.length(); ++i) g_output23[b].data1[i] = g_input23[b].data1[i];" NL
		"    g_output23[b].data2 = g_input23[b].data2;" NL "    g_output23[b].data3[0] = g_input23[b].data3[0];" NL
		"    g_output23[b].data3[1] = g_input23[b].data3[1];" NL "    g_output23[b].data4 = g_input23[b].data4;" NL
		"  }" NL "  g_output23[0].data5 = g_input23[0].data5;" NL "  g_output23[1].data5 = g_input23[1].data5;" NL "}";
}

class BasicStdLayoutCase4VS : public BasicStdLayoutBase2VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC4(in_data);
	}
};

class BasicStdLayoutCase4CS : public BasicStdLayoutBase2CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[4])
	{
		return GetInputC4(in_data);
	}
};
//-----------------------------------------------------------------------------
// 1.10.x BasicOperationsBase
//-----------------------------------------------------------------------------

class BasicOperationsBaseVS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];
	GLuint m_vertex_array;

	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		std::vector<GLubyte> in_data;
		std::vector<GLubyte> expected_data;
		const char*			 glsl_vs = GetInput(in_data, expected_data);

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		/* output buffer */
		{
			std::vector<GLubyte> zero(expected_data.size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)expected_data.size(), &zero[0], GL_STATIC_DRAW);
		}
		// input buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data.size(), &in_data[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &m_vertex_array);
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		std::vector<GLubyte> out_data(expected_data.size());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[1]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)out_data.size(), &out_data[0]);

		bool status = true;
		for (size_t i = 0; i < out_data.size(); ++i)
		{
			if (expected_data[i] != out_data[i])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Byte at index " << static_cast<int>(i)
													<< " is " << tcu::toHex(out_data[i]) << " should be "
													<< tcu::toHex(expected_data[i]) << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicOperationsBaseCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];

	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		std::vector<GLubyte> in_data;
		std::vector<GLubyte> expected_data;

		std::stringstream ss;
		ss << "layout(local_size_x = 1) in;\n" << GetInput(in_data, expected_data);
		m_program = CreateProgramCS(ss.str());
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		/* output buffer */
		{
			std::vector<GLubyte> zero(expected_data.size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)expected_data.size(), &zero[0], GL_STATIC_DRAW);
		}
		// input buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data.size(), &in_data[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glUniform3f(glGetUniformLocation(m_program, "g_value0"), 10.0, 20.0, 30.0);
		glUniform1i(glGetUniformLocation(m_program, "g_index1"), 1);
		glUniform1i(glGetUniformLocation(m_program, "g_index2"), 2);
		glDispatchCompute(1, 1, 1);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[1]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLubyte* out_data =
			(GLubyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)expected_data.size(), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;

		bool status = true;
		for (size_t i = 0; i < expected_data.size(); ++i)
		{
			if (expected_data[i] != out_data[i])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Byte at index " << static_cast<int>(i)
													<< " is " << tcu::toHex(out_data[i]) << " should be "
													<< tcu::toHex(expected_data[i]) << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 1.10.1 BasicOperationsCase1
//-----------------------------------------------------------------------------
const char* GetInputOp1(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
{
	/* input */
	{
		in_data.resize(16 * 9);
		int*   ip = reinterpret_cast<int*>(&in_data[0]);
		float* fp = reinterpret_cast<float*>(&in_data[0]);
		ip[0]	 = 1;
		ip[1]	 = 2;
		ip[2]	 = 3;
		ip[3]	 = 4;
		fp[4]	 = 1.0f;
		fp[5]	 = 2.0f;
		fp[6]	 = 3.0f;
		fp[7]	 = 0.0f;
		ip[8]	 = 1;
		ip[9]	 = 2;
		ip[10]	= 3;
		ip[11]	= 4;
		ip[12]	= 1;
		ip[13]	= -2;
		ip[14]	= 3;
		ip[15]	= 4;
		fp[16]	= 1.0f;
		fp[17]	= 2.0f;
		fp[18]	= 3.0f;
		fp[19]	= 4.0f;
		fp[20]	= 1.0f;
		fp[21]	= 2.0f;
		fp[22]	= 3.0f;
		fp[23]	= 4.0f;
		fp[24]	= 1.0f;
		fp[25]	= 2.0f;
		fp[26]	= 3.0f;
		fp[27]	= 4.0f;
		fp[28]	= 1.0f;
		fp[29]	= 2.0f;
		fp[30]	= 3.0f;
		fp[31]	= 4.0f;
		fp[32]	= 1.0f;
		fp[33]	= 0.0f;
		fp[34]	= 0.0f;
		fp[35]	= 4.0f;
	}
	/* expected output */
	{
		out_data.resize(16 * 9);
		int*   ip = reinterpret_cast<int*>(&out_data[0]);
		float* fp = reinterpret_cast<float*>(&out_data[0]);
		ip[0]	 = 4;
		ip[1]	 = 3;
		ip[2]	 = 2;
		ip[3]	 = 1;
		fp[4]	 = 3.0f;
		fp[5]	 = 2.0f;
		fp[6]	 = 1.0f;
		fp[7]	 = 0.0f;
		ip[8]	 = 4;
		ip[9]	 = 1;
		ip[10]	= 0;
		ip[11]	= 3;
		ip[12]	= 10;
		ip[13]	= 4;
		ip[14]	= -2;
		ip[15]	= 20;
		fp[16]	= 50.0f;
		fp[17]	= 5.0f;
		fp[18]	= 2.0f;
		fp[19]	= 30.0f;
		fp[20]	= 4.0f;
		fp[21]	= 2.0f;
		fp[22]	= 3.0f;
		fp[23]	= 1.0f;
		fp[24]	= 4.0f;
		fp[25]	= 3.0f;
		fp[26]	= 2.0f;
		fp[27]	= 1.0f;
		fp[28]	= 2.0f;
		fp[29]	= 2.0f;
		fp[30]	= 2.0f;
		fp[31]	= 2.0f;
		fp[32]	= 4.0f;
		fp[33]	= 0.0f;
		fp[34]	= 0.0f;
		fp[35]	= 1.0f;
	}
	return NL "layout(std430, binding = 0) buffer Input {" NL "  ivec4 data0;" NL "  vec3 data1;" NL "  uvec4 data2;" NL
			  "  ivec4 data3;" NL "  vec4 data4;" NL "  mat4 data5;" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  ivec4 data0;" NL "  vec3 data1;" NL
			  "  uvec4 data2;" NL "  ivec4 data3;" NL "  vec4 data4;" NL "  mat4 data5;" NL "} g_output;" NL
			  "uniform vec3 g_value0 = vec3(10, 20, 30);" NL "uniform int g_index1 = 1;" NL "void main() {" NL
			  "  int index0 = 0;" NL "  g_output.data0.wzyx = g_input.data0;" NL
			  "  g_output.data1 = g_input.data1.zyx;" NL "  g_output.data2.xwy = g_input.data2.wzx;" NL
			  "  g_output.data3.xw = ivec2(10, 20);" NL "  g_output.data3.zy = g_input.data3.yw;" NL
			  "  g_output.data4.wx = g_value0.xz;" NL "  g_output.data4.wx += g_value0.yy;" NL
			  "  g_output.data4.yz = g_input.data4.xx + g_input.data4.wx;" NL
			  "  g_output.data5[g_index1 - 1].wyzx = vec4(1, 2, 3, 4);" NL
			  "  g_output.data5[g_index1 + index0] = g_input.data5[g_index1].wzyx;" NL
			  "  g_output.data5[1 + g_index1] = g_input.data5[g_index1 + 1].yyyy;" NL
			  "  g_output.data5[5 - g_index1 - 1].wx = g_input.data5[4 - g_index1].xw;" NL "}";
}

class BasicOperationsCase1VS : public BasicOperationsBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
	{
		return GetInputOp1(in_data, out_data);
	}
};

class BasicOperationsCase1CS : public BasicOperationsBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
	{
		return GetInputOp1(in_data, out_data);
	}
};
//-----------------------------------------------------------------------------
// 1.10.2 BasicOperationsCase2
//-----------------------------------------------------------------------------
const char* GetInputOp2(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
{
	/* input */
	{
		in_data.resize(16 * 8);
		float* fp = reinterpret_cast<float*>(&in_data[0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 0.0f;
		fp[2]	 = 0.0f;
		fp[3]	 = 0.0f;
		fp[4]	 = 0.0f;
		fp[5]	 = 1.0f;
		fp[6]	 = 0.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 0.0f;
		fp[9]	 = 0.0f;
		fp[10]	= 1.0f;
		fp[11]	= 0.0f;
		fp[12]	= 0.0f;
		fp[13]	= 0.0f;
		fp[14]	= 0.0f;
		fp[15]	= 1.0f;

		fp[16] = 2.0f;
		fp[17] = 0.0f;
		fp[18] = 0.0f;
		fp[19] = 0.0f;
		fp[20] = 0.0f;
		fp[21] = 3.0f;
		fp[22] = 0.0f;
		fp[23] = 0.0f;
		fp[24] = 0.0f;
		fp[25] = 0.0f;
		fp[26] = 4.0f;
		fp[27] = 0.0f;
		fp[28] = 0.0f;
		fp[29] = 0.0f;
		fp[30] = 0.0f;
		fp[31] = 5.0f;
	}
	/* expected output */
	{
		out_data.resize(16 * 5);
		float* fp = reinterpret_cast<float*>(&out_data[0]);
		fp[0]	 = 2.0f;
		fp[1]	 = 0.0f;
		fp[2]	 = 0.0f;
		fp[3]	 = 0.0f;
		fp[4]	 = 0.0f;
		fp[5]	 = 3.0f;
		fp[6]	 = 0.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 0.0f;
		fp[9]	 = 0.0f;
		fp[10]	= 4.0f;
		fp[11]	= 0.0f;
		fp[12]	= 0.0f;
		fp[13]	= 0.0f;
		fp[14]	= 0.0f;
		fp[15]	= 5.0f;

		fp[16] = 0.0f;
		fp[17] = 1.0f;
		fp[18] = 4.0f;
		fp[19] = 0.0f;
	}
	return NL "layout(std430, binding = 0) buffer Input {" NL "  mat4 data0;" NL "  mat4 data1;" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  mat4 data0;" NL "  vec4 data1;" NL "} g_output;" NL
			  "uniform int g_index2 = 2;" NL "void main() {" NL
			  "  g_output.data0 = matrixCompMult(g_input.data0, g_input.data1);" NL
			  "  g_output.data1 = g_input.data0[1] + g_input.data1[g_index2];" NL "}";
}

class BasicOperationsCase2VS : public BasicOperationsBaseVS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
	{
		return GetInputOp2(in_data, out_data);
	}
};

class BasicOperationsCase2CS : public BasicOperationsBaseCS
{
	virtual const char* GetInput(std::vector<GLubyte>& in_data, std::vector<GLubyte>& out_data)
	{
		return GetInputOp2(in_data, out_data);
	}
};

//-----------------------------------------------------------------------------
// 1.11.x BasicStdLayoutBase3
//-----------------------------------------------------------------------------
class BasicStdLayoutBase3VS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[4];
	GLuint m_vertex_array;

	virtual const char* GetInput(std::vector<GLubyte> in_data[2]) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		std::vector<GLubyte> in_data[2];
		const char*			 glsl_vs = GetInput(in_data);

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_buffer);

		// input buffers
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)in_data[0].size(), &in_data[0][0], GL_STATIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[1].size(), &in_data[1][0], GL_STATIC_DRAW);

		/* output buffer 0 */
		{
			std::vector<GLubyte> out_data(in_data[0].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[0].size(), &out_data[0], GL_STATIC_DRAW);
		}
		/* output buffer 1 */
		{
			std::vector<GLubyte> out_data(in_data[1].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[1].size(), &out_data[0], GL_STATIC_DRAW);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		bool status = true;
		for (int j = 0; j < 2; ++j)
		{
			std::vector<GLubyte> out_data(in_data[j].size());
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[j + 2]);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data[j].size(), &out_data[0]);

			for (size_t i = 0; i < in_data[j].size(); ++i)
			{
				if (in_data[j][i] != out_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
						<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[j][i])
						<< tcu::TestLog::EndMessage;
					status = false;
				}
			}
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicStdLayoutBase3CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[4];

	virtual const char* GetInput(std::vector<GLubyte> in_data[2]) = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		std::vector<GLubyte> in_data[2];

		std::stringstream ss;
		ss << "layout(local_size_x = 1) in;\n" << GetInput(in_data);
		m_program = CreateProgramCS(ss.str());
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_buffer);

		// input buffers
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)in_data[0].size(), &in_data[0][0], GL_STATIC_DRAW);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[1].size(), &in_data[1][0], GL_STATIC_DRAW);

		/* output buffer 0 */
		{
			std::vector<GLubyte> out_data(in_data[0].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[0].size(), &out_data[0], GL_STATIC_DRAW);
		}
		/* output buffer 1 */
		{
			std::vector<GLubyte> out_data(in_data[1].size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)in_data[1].size(), &out_data[0], GL_STATIC_DRAW);
		}

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_index1"), 1);
		glDispatchCompute(1, 1, 1);

		bool status = true;
		for (int j = 0; j < 2; ++j)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[j + 2]);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			GLubyte* out_data =
				(GLubyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)in_data[j].size(), GL_MAP_READ_BIT);
			if (!out_data)
				return ERROR;

			for (size_t i = 0; i < in_data[j].size(); ++i)
			{
				if (in_data[j][i] != out_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Byte at index " << static_cast<int>(i) << " is "
						<< tcu::toHex(out_data[i]) << " should be " << tcu::toHex(in_data[j][i])
						<< tcu::TestLog::EndMessage;
					status = false;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.11.1 Basic_UBO_SSBO_LayoutCase1
//-----------------------------------------------------------------------------
const char* GetInputUBO1(std::vector<GLubyte> in_data[2])
{
	/* UBO */
	{
		in_data[0].resize(12 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[0][0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 0.0f;
		fp[2]	 = 0.0f;
		fp[3]	 = 0.0f;
		fp[4]	 = 2.0f;
		fp[5]	 = 0.0f;
		fp[6]	 = 0.0f;
		fp[7]	 = 0.0f;
		fp[8]	 = 3.0f;
		fp[9]	 = 0.0f;
		fp[10]	= 0.0f;
		fp[11]	= 0.0f;
	}
	/* SSBO */
	{
		in_data[1].resize(3 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[1][0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 2.0f;
		fp[2]	 = 3.0f;
	}
	return NL
		"layout(std140, binding = 0) uniform InputUBO {" NL "  float data0;" NL "  float data1[2];" NL
		"} g_input_ubo;" NL "layout(std430, binding = 0) buffer InputSSBO {" NL "  float data0;" NL
		"  float data1[2];" NL "} g_input_ssbo;" NL "layout(std140, binding = 1) buffer OutputUBO {" NL
		"  float data0;" NL "  float data1[2];" NL "} g_output_ubo;" NL
		"layout(std430, binding = 2) buffer OutputSSBO {" NL "  float data0;" NL "  float data1[2];" NL
		"} g_output_ssbo;" NL "void main() {" NL "  g_output_ubo.data0 = g_input_ubo.data0;" NL
		"  for (int i = 0; i < g_input_ubo.data1.length(); ++i) g_output_ubo.data1[i] = g_input_ubo.data1[i];" NL
		"  g_output_ssbo.data0 = g_input_ssbo.data0;" NL
		"  for (int i = 0; i < g_input_ssbo.data1.length(); ++i) g_output_ssbo.data1[i] = g_input_ssbo.data1[i];" NL
		"}";
}

class Basic_UBO_SSBO_LayoutCase1VS : public BasicStdLayoutBase3VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[2])
	{
		return GetInputUBO1(in_data);
	}
};

class Basic_UBO_SSBO_LayoutCase1CS : public BasicStdLayoutBase3CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[2])
	{
		return GetInputUBO1(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.11.2 Basic_UBO_SSBO_LayoutCase2
//-----------------------------------------------------------------------------
const char* GetInputUBO2(std::vector<GLubyte> in_data[2])
{
	/* UBO */
	{
		in_data[0].resize(280 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[0][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[0][0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 2.0f;
		fp[2]	 = 3.0f;
		fp[3]	 = 4.0f;
		fp[4]	 = 5.0f;
		fp[5]	 = 6.0f;
		fp[6]	 = 7.0f;
		fp[8]	 = 8.0f;
		fp[8]	 = 9.0f;
		fp[12]	= 10.0f;
		fp[16]	= 11.0f;
		fp[20]	= 12.0f;
		fp[24]	= 13.0f;

		ip[28] = 14;
		for (int i = 0; i < 20; ++i)
		{
			fp[32 + i * 4] = static_cast<float>(15 + i);
		}
		ip[112] = 140;
		for (int i = 0; i < 20; ++i)
		{
			fp[116 + i * 4] = static_cast<float>(150 + i);
		}
		ip[196] = 1400;
		for (int i = 0; i < 20; ++i)
		{
			fp[200 + i * 4] = static_cast<float>(1500 + i);
		}
	}
	/* SSBO */
	{
		in_data[1].resize(76 * 4);
		float* fp = reinterpret_cast<float*>(&in_data[1][0]);
		int*   ip = reinterpret_cast<int*>(&in_data[1][0]);
		fp[0]	 = 1.0f;
		fp[1]	 = 2.0f;
		fp[2]	 = 3.0f;
		fp[3]	 = 4.0f;
		fp[4]	 = 5.0f;
		fp[5]	 = 6.0f;
		fp[6]	 = 7.0f;
		fp[7]	 = 8.0f;
		fp[8]	 = 9.0f;
		fp[9]	 = 10.0f;
		fp[10]	= 11.0f;
		fp[11]	= 12.0f;
		fp[12]	= 13.0f;
		ip[13]	= 14;
		fp[14]	= 15.0f;
		fp[15]	= 16.0f;
		fp[16]	= 17.0f;
		fp[17]	= 18.0f;
		fp[18]	= 19.0f;
		fp[19]	= 20.0f;
		fp[20]	= 21.0f;
		fp[21]	= 22.0f;
		fp[22]	= 23.0f;
		fp[23]	= 24.0f;
		fp[24]	= 25.0f;
		fp[25]	= 26.0f;
		fp[26]	= 27.0f;
		fp[27]	= 28.0f;
		fp[28]	= 29.0f;
		fp[29]	= 30.0f;
		fp[30]	= 31.0f;
		fp[31]	= 32.0f;
		fp[32]	= 33.0f;
		fp[33]	= 34.0f;
		ip[34]	= 35;
		fp[35]	= 36.0f;
		fp[36]	= 37.0f;
		fp[37]	= 38.0f;
		fp[38]	= 39.0f;
		fp[39]	= 40.0f;
		fp[40]	= 41.0f;
		fp[41]	= 42.0f;
		fp[42]	= 43.0f;
		fp[43]	= 44.0f;
		fp[44]	= 45.0f;
		fp[45]	= 46.0f;
		fp[46]	= 47.0f;
		fp[47]	= 48.0f;
		fp[48]	= 49.0f;
		fp[49]	= 50.0f;
		fp[50]	= 51.0f;
		fp[51]	= 52.0f;
		fp[52]	= 53.0f;
		fp[53]	= 54.0f;
		fp[54]	= 55.0f;
		ip[55]	= 56;
		fp[56]	= 57.0f;
		fp[57]	= 58.0f;
		fp[58]	= 59.0f;
		fp[59]	= 60.0f;
		fp[60]	= 61.0f;
		fp[61]	= 62.0f;
		fp[62]	= 63.0f;
		fp[63]	= 64.0f;
		fp[64]	= 65.0f;
		fp[65]	= 66.0f;
		fp[66]	= 67.0f;
		fp[67]	= 68.0f;
		fp[68]	= 69.0f;
		fp[69]	= 70.0f;
		fp[70]	= 71.0f;
		fp[71]	= 72.0f;
		fp[72]	= 73.0f;
		fp[73]	= 74.0f;
		fp[74]	= 75.0f;
		fp[75]	= 76.0f;
	}
	return NL
		"struct MM {" NL "  float mm_a[5];" NL "};" NL "struct TT {" NL "  int tt_a;" NL "  MM  tt_b[4];" NL "};" NL
		"layout(std140, binding = 0) uniform InputUBO {" NL "  vec4  a;" NL "  vec4  b;" NL "  float c;" NL
		"  float d[4];" NL "  TT    e[3];" NL "} g_input_ubo;" NL "layout(std430, binding = 0) buffer InputSSBO {" NL
		"  vec4  a;" NL "  vec4  b;" NL "  float c;" NL "  float d[4];" NL "  TT    e[3];" NL "} g_input_ssbo;" NL
		"layout(std140, binding = 1) buffer OutputUBO {" NL "  vec4  a;" NL "  vec4  b;" NL "  float c;" NL
		"  float d[4];" NL "  TT    e[3];" NL "} g_output_ubo;" NL "layout(std430, binding = 2) buffer OutputSSBO {" NL
		"  vec4  a;" NL "  vec4  b;" NL "  float c;" NL "  float d[4];" NL "  TT    e[3];" NL "} g_output_ssbo;" NL
		"uniform int g_index1 = 1;" NL "void main() {" NL "  int index0 = 0;" NL NL
		"  g_output_ubo.a = g_input_ubo.a;" NL "  g_output_ubo.b = g_input_ubo.b;" NL
		"  g_output_ubo.c = g_input_ubo.c;" NL
		"  for (int i = 0; i < g_input_ubo.d.length(); ++i) g_output_ubo.d[i] = g_input_ubo.d[i];" NL
		"    for (int j = 0; j < g_input_ubo.e.length(); ++j) {" NL
		"    g_output_ubo.e[j].tt_a = g_input_ubo.e[j].tt_a;" NL
		"    for (int i = 0; i < g_input_ubo.e[j].tt_b.length(); ++i) {" NL
		"      g_output_ubo.e[j].tt_b[i].mm_a[0] = g_input_ubo.e[j].tt_b[i].mm_a[0];" NL
		"      g_output_ubo.e[j].tt_b[index0 + i].mm_a[1] = g_input_ubo.e[j].tt_b[i].mm_a[1];" NL
		"      g_output_ubo.e[j].tt_b[i].mm_a[2] = g_input_ubo.e[j].tt_b[i].mm_a[2 + index0];" NL
		"      g_output_ubo.e[j + 1 - g_index1].tt_b[i].mm_a[4 - g_index1] = g_input_ubo.e[j].tt_b[i].mm_a[2 + "
		"g_index1];" NL "      g_output_ubo.e[j].tt_b[i].mm_a[4] = g_input_ubo.e[j].tt_b[i - index0].mm_a[4];" NL
		"    }" NL "  }" NL NL "  g_output_ssbo.a = g_input_ssbo.a;" NL "  g_output_ssbo.b = g_input_ssbo.b;" NL
		"  g_output_ssbo.c = g_input_ssbo.c;" NL
		"  for (int i = 0; i < g_input_ssbo.d.length(); ++i) g_output_ssbo.d[i] = g_input_ssbo.d[i];" NL
		"  for (int j = 0; j < g_input_ssbo.e.length(); ++j) {" NL
		"    g_output_ssbo.e[j].tt_a = g_input_ssbo.e[j].tt_a;" NL
		"    for (int i = 0; i < g_input_ssbo.e[j].tt_b.length(); ++i) {" NL
		"      g_output_ssbo.e[j + index0].tt_b[i].mm_a[0] = g_input_ssbo.e[j].tt_b[i].mm_a[index0];" NL
		"      g_output_ssbo.e[j].tt_b[i + index0].mm_a[1] = g_input_ssbo.e[j].tt_b[i].mm_a[g_index1];" NL
		"      g_output_ssbo.e[j].tt_b[i].mm_a[2] = g_input_ssbo.e[j].tt_b[i].mm_a[1 + g_index1];" NL
		"      g_output_ssbo.e[j - index0].tt_b[i].mm_a[g_index1 + 2] = g_input_ssbo.e[j].tt_b[i].mm_a[4 - "
		"g_index1];" NL "      g_output_ssbo.e[j].tt_b[i].mm_a[4] = g_input_ssbo.e[j].tt_b[i].mm_a[4];" NL "    }" NL
		"  }" NL "}";
}

class Basic_UBO_SSBO_LayoutCase2VS : public BasicStdLayoutBase3VS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[2])
	{
		return GetInputUBO2(in_data);
	}
};

class Basic_UBO_SSBO_LayoutCase2CS : public BasicStdLayoutBase3CS
{
	virtual const char* GetInput(std::vector<GLubyte> in_data[2])
	{
		return GetInputUBO2(in_data);
	}
};

//-----------------------------------------------------------------------------
// 1.12.x BasicMatrixOperationsBase
//-----------------------------------------------------------------------------

class BasicMatrixOperationsBaseVS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];
	GLuint m_vertex_array;

	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected) = 0;

	static bool Equal(float a, float b)
	{
		return fabsf(a - b) < 0.001f;
	}

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		std::vector<float> in;
		std::vector<float> expected;
		const char*		   glsl_vs = GetInput(in, expected);

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		/* output buffer */
		{
			std::vector<float> zero(expected.size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(expected.size() * sizeof(float)), &zero[0],
						 GL_STATIC_DRAW);
		}
		// input buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(in.size() * sizeof(float)), &in[0], GL_STATIC_DRAW);

		glGenVertexArrays(1, &m_vertex_array);
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		std::vector<float> out_data(expected.size());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[1]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)(out_data.size() * sizeof(float)), &out_data[0]);

		bool status = true;
		for (size_t i = 0; i < out_data.size(); ++i)
		{
			if (!Equal(expected[i], out_data[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Float at index " << static_cast<int>(i) << " is " << out_data[i]
					<< " should be " << expected[i] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class BasicMatrixOperationsBaseCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_buffer[2];

	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected) = 0;

	static bool Equal(float a, float b)
	{
		return fabsf(a - b) < 0.001f;
	}

	virtual long Setup()
	{
		m_program = 0;
		memset(m_buffer, 0, sizeof(m_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		std::vector<float> in;
		std::vector<float> expected;
		std::stringstream  ss;
		ss << "layout(local_size_x = 1) in;\n" << GetInput(in, expected);
		m_program = CreateProgramCS(ss.str());
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_buffer);

		/* output buffer */
		{
			std::vector<float> zero(expected.size());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(expected.size() * sizeof(float)), &zero[0],
						 GL_STATIC_DRAW);
		}
		// input buffer
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(in.size() * sizeof(float)), &in[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[1]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		float* out_data = (float*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
												   (GLsizeiptr)(expected.size() * sizeof(float)), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;

		bool status = true;
		for (size_t i = 0; i < expected.size(); ++i)
		{
			if (!Equal(expected[i], out_data[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Float at index " << static_cast<int>(i) << " is " << out_data[i]
					<< " should be " << expected[i] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (!status)
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.12.1 BasicMatrixOperationsCase1
//-----------------------------------------------------------------------------
const char* GetInputM1(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(8);
	in[0] = 1.0f;
	in[2] = 3.0f;
	in[1] = 2.0f;
	in[3] = 4.0f;
	in[4] = 1.0f;
	in[6] = 3.0f;
	in[5] = 2.0f;
	in[7] = 4.0f;
	expected.resize(4);
	expected[0] = 7.0f;
	expected[2] = 15.0f;
	expected[1] = 10.0f;
	expected[3] = 22.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  mat2 m0;" NL "  mat2 m1;" NL "} g_input;" NL
			  "layout(std430, binding = 1) buffer Output {" NL "  mat2 m;" NL "} g_output;" NL
			  "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase1VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM1(in, expected);
	}
};

class BasicMatrixOperationsCase1CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM1(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.12.2 BasicMatrixOperationsCase2
//-----------------------------------------------------------------------------
const char* GetInputM2(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(16);
	expected.resize(4);
	// mat3x2
	in[0] = 1.0f;
	in[2] = 3.0f;
	in[4] = 5.0f;
	in[1] = 2.0f;
	in[3] = 4.0f;
	in[5] = 6.0f;
	// mat2x3
	in[8]  = 1.0f;
	in[12] = 4.0f;
	in[9]  = 2.0f;
	in[13] = 5.0f;
	in[10] = 3.0f;
	in[14] = 6.0f;
	// mat2
	expected[0] = 22.0f;
	expected[2] = 49.0f;
	expected[1] = 28.0f;
	expected[3] = 64.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(column_major) mat3x2 m0;" NL
			  "  layout(column_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(column_major) mat2 m;" NL "} g_output;" NL
			  "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase2VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM2(in, expected);
	}
};

class BasicMatrixOperationsCase2CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM2(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.12.3 BasicMatrixOperationsCase3
//-----------------------------------------------------------------------------
const char* GetInputM3(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(16);
	expected.resize(4);
	// row major mat3x2
	in[0] = 1.0f;
	in[1] = 3.0f;
	in[2] = 5.0f;
	in[4] = 2.0f;
	in[5] = 4.0f;
	in[6] = 6.0f;
	// row major mat2x3
	in[8]  = 1.0f;
	in[9]  = 4.0f;
	in[10] = 2.0f;
	in[11] = 5.0f;
	in[12] = 3.0f;
	in[13] = 6.0f;
	// row major mat2
	expected[0] = 22.0f;
	expected[1] = 49.0f;
	expected[2] = 28.0f;
	expected[3] = 64.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(row_major) mat3x2 m0;" NL
			  "  layout(row_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(row_major) mat2 m;" NL "} g_output;" NL "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase3VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM3(in, expected);
	}
};

class BasicMatrixOperationsCase3CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM3(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.12.4 BasicMatrixOperationsCase4
//-----------------------------------------------------------------------------
const char* GetInputM4(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(16);
	expected.resize(4);
	// column major mat3x2
	in[0] = 1.0f;
	in[2] = 3.0f;
	in[4] = 5.0f;
	in[1] = 2.0f;
	in[3] = 4.0f;
	in[5] = 6.0f;
	// row major mat2x3
	in[8]  = 1.0f;
	in[9]  = 4.0f;
	in[10] = 2.0f;
	in[11] = 5.0f;
	in[12] = 3.0f;
	in[13] = 6.0f;
	// column major mat2
	expected[0] = 13.0f;
	expected[1] = 16.0f;
	expected[2] = 37.0f;
	expected[3] = 46.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(column_major) mat3x2 m0;" NL
			  "  layout(row_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(column_major) mat2 m;" NL "} g_output;" NL
			  "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase4VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM4(in, expected);
	}
};

class BasicMatrixOperationsCase4CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM4(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.12.5 BasicMatrixOperationsCase5
//-----------------------------------------------------------------------------
const char* GetInputM5(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(16);
	expected.resize(4);
	// column major mat3x2
	in[0] = 1.0f;
	in[2] = 3.0f;
	in[4] = 5.0f;
	in[1] = 2.0f;
	in[3] = 4.0f;
	in[5] = 6.0f;
	// row major mat2x3
	in[8]  = 1.0f;
	in[9]  = 4.0f;
	in[10] = 2.0f;
	in[11] = 5.0f;
	in[12] = 3.0f;
	in[13] = 6.0f;
	// row major mat2
	expected[0] = 13.0f;
	expected[1] = 37.0f;
	expected[2] = 16.0f;
	expected[3] = 46.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(column_major) mat3x2 m0;" NL
			  "  layout(row_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(row_major) mat2 m;" NL "} g_output;" NL "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase5VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM5(in, expected);
	}
};

class BasicMatrixOperationsCase5CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM5(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.12.6 BasicMatrixOperationsCase6
//-----------------------------------------------------------------------------
const char* GetInputM6(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(20);
	expected.resize(4);
	// row major mat3x2
	in[0] = 1.0f;
	in[1] = 3.0f;
	in[2] = 5.0f;
	in[4] = 2.0f;
	in[5] = 4.0f;
	in[6] = 6.0f;
	// column major mat2x3
	in[8]  = 1.0f;
	in[12] = 4.0f;
	in[9]  = 2.0f;
	in[13] = 5.0f;
	in[10] = 3.0f;
	in[14] = 6.0f;
	// column major mat2
	expected[0] = 22.0f;
	expected[1] = 28.0f;
	expected[2] = 49.0f;
	expected[3] = 64.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(row_major) mat3x2 m0;" NL
			  "  layout(column_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(column_major) mat2 m;" NL "} g_output;" NL
			  "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase6VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM6(in, expected);
	}
};

class BasicMatrixOperationsCase6CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM6(in, expected);
	}
};
//-----------------------------------------------------------------------------
// 1.12.7 BasicMatrixOperationsCase7
//-----------------------------------------------------------------------------
const char* GetInputM7(std::vector<float>& in, std::vector<float>& expected)
{
	in.resize(20);
	expected.resize(4);
	// row major mat3x2
	in[0] = 1.0f;
	in[1] = 3.0f;
	in[2] = 5.0f;
	in[4] = 2.0f;
	in[5] = 4.0f;
	in[6] = 6.0f;
	// column major mat2x3
	in[8]  = 1.0f;
	in[12] = 4.0f;
	in[9]  = 2.0f;
	in[13] = 5.0f;
	in[10] = 3.0f;
	in[14] = 6.0f;
	// row major mat2
	expected[0] = 22.0f;
	expected[1] = 49.0f;
	expected[2] = 28.0f;
	expected[3] = 64.0f;

	return NL "layout(std430, binding = 0) buffer Input {" NL "  layout(row_major) mat3x2 m0;" NL
			  "  layout(column_major) mat2x3 m1;" NL "} g_input;" NL "layout(std430, binding = 1) buffer Output {" NL
			  "  layout(row_major) mat2 m;" NL "} g_output;" NL "void main() { g_output.m = g_input.m0 * g_input.m1; }";
}

class BasicMatrixOperationsCase7VS : public BasicMatrixOperationsBaseVS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM7(in, expected);
	}
};

class BasicMatrixOperationsCase7CS : public BasicMatrixOperationsBaseCS
{
	virtual const char* GetInput(std::vector<float>& in, std::vector<float>& expected)
	{
		return GetInputM7(in, expected);
	}
};

//-----------------------------------------------------------------------------
// 1.13 BasicNoBindingLayout
//-----------------------------------------------------------------------------
class BasicNoBindingLayout : public ShaderStorageBufferObjectBase
{
	GLuint m_vsp, m_ppo, m_ssbo, m_vao;

	virtual long Setup()
	{
		m_vsp = 0;
		glGenProgramPipelines(1, &m_ppo);
		glGenBuffers(1, &m_ssbo);
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteProgram(m_vsp);
		glDeleteProgramPipelines(1, &m_ppo);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(3))
			return NOT_SUPPORTED;

		const char* const glsl_vs = "#version 430 core" NL "layout(std430) buffer Output0 { int data; } g_output0;" NL
									"layout(std430) buffer Output1 { int g_output1; };" NL
									"layout(std430) buffer Output2 { int data; } g_output2;" NL "void main() {" NL
									"  g_output0.data = 1;" NL "  g_output1 = 2;" NL "  g_output2.data = 3;" NL "}";
		m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		if (!CheckProgram(m_vsp))
			return ERROR;
		glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);

		glShaderStorageBlockBinding(m_vsp, glGetProgramResourceIndex(m_vsp, GL_SHADER_STORAGE_BLOCK, "Output0"), 1);
		glShaderStorageBlockBinding(m_vsp, glGetProgramResourceIndex(m_vsp, GL_SHADER_STORAGE_BLOCK, "Output1"), 5);
		glShaderStorageBlockBinding(m_vsp, glGetProgramResourceIndex(m_vsp, GL_SHADER_STORAGE_BLOCK, "Output2"), 7);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, m_ssbo, 0, 4);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 5, m_ssbo, 256, 4);
		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 7, m_ssbo, 512, 4);

		glEnable(GL_RASTERIZER_DISCARD);
		glBindProgramPipeline(m_ppo);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_POINTS, 0, 1);

		int data;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4, &data);
		if (data != 1)
			return ERROR;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 256, 4, &data);
		if (data != 2)
			return ERROR;
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 512, 4, &data);
		if (data != 3)
			return ERROR;

		return NO_ERROR;
	}
};

//----------------------------------------------------------------------------
// 1.14 BasicReadonlyWriteonly
//-----------------------------------------------------------------------------
class BasicReadonlyWriteonly : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430, binding = 0) buffer Input {" NL
			   "  readonly writeonly int g_in[];" NL "};" NL "layout(std430, binding = 1) buffer Output {" NL
			   "  int count;" NL "} g_output;" NL "void main() {" NL "  g_output.count = g_in.length();" NL "}";

		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);

		/* Input */
		int input_data[] = { 1, 2, 3 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(input_data), input_data, GL_STATIC_DRAW);

		/* Output */
		int output_data[] = { 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(output_data), output_data, GL_DYNAMIC_COPY);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
		int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4, GL_MAP_READ_BIT);
		if (!data)
			return ERROR;
		if (*data != DE_LENGTH_OF_ARRAY(input_data))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Buffer data is " << *data << " should be "
												<< sizeof(input_data) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		return NO_ERROR;
	}
};

//----------------------------------------------------------------------------
// 1.15 BasicNameMatch
//-----------------------------------------------------------------------------
class BasicNameMatch : public ShaderStorageBufferObjectBase
{
	virtual long Run()
	{
		GLint blocksVS, blocksFS;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &blocksVS);
		glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &blocksFS);
		if ((blocksVS == 0) || (blocksFS == 0))
			return NOT_SUPPORTED;

		// check if link error is generated when one of matched blocks has instance name and other doesn't
		std::string vs1("buffer Buf { float x; };\n"
						"void main() {\n"
						"  gl_Position = vec4(x);\n"
						"}");
		std::string fs1("buffer Buf { float x; } b;\n"
						"out vec4 color;\n"
						"void main() {\n"
						"  color = vec4(b.x);\n"
						"}");
		if (Link(vs1, fs1))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Linking should fail." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		// check if linking succeeds when both matched blocks are lacking an instance name
		std::string vs2("buffer Buf { float x; };\n"
						"void main() {\n"
						"  gl_Position = vec4(x);\n"
						"}");
		std::string fs2("buffer Buf { float x; };\n"
						"out vec4 color;\n"
						"void main() {\n"
						"  color = vec4(x);\n"
						"}");
		if (!Link(vs2, fs2))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Linking should succeed." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		// check if linking succeeds when both matched blocks have different instance names
		std::string vs3("buffer Buf { float x; } a;\n"
						"void main() {\n"
						"  gl_Position = vec4(a.x);\n"
						"}");
		std::string fs3("buffer Buf { float x; } b;\n"
						"out vec4 color;\n"
						"void main() {\n"
						"  color = vec4(b.x);\n"
						"}");
		if (!Link(vs3, fs3))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Linking should succeed." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	bool Link(const std::string& vs, const std::string& fs)
	{
		GLuint program = CreateProgram(vs, fs);
		glLinkProgram(program);
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		glDeleteProgram(program);
		return (status == GL_TRUE);
	}
};

//-----------------------------------------------------------------------------
// 2.1 AdvancedSwitchBuffers
//-----------------------------------------------------------------------------
class AdvancedSwitchBuffers : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[5];
	GLuint m_vertex_array;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "struct VertexData {" NL "  vec2 position;" NL "  vec3 color;" NL "};" NL
			   "layout(binding = 0, std430) buffer Input {" NL "  VertexData vertex[];" NL "} g_vs_in;" NL
			   "out StageData {" NL "  vec3 color;" NL "} g_vs_out;" NL "void main() {" NL
			   "  gl_Position = vec4(g_vs_in.vertex[gl_VertexID].position, 0, 1);" NL
			   "  g_vs_out.color = g_vs_in.vertex[gl_VertexID].color;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_fs_out;" NL
			   "void main() {" NL "  g_fs_out = vec4(g_fs_in.color, 1);" NL "}";
		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(5, m_storage_buffer);

		/* left, bottom, red quad */
		{
			const float data[] = { -0.4f - 0.5f, -0.4f - 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   0.4f - 0.5f,  -0.4f - 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   -0.4f - 0.5f, 0.4f - 0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   0.4f - 0.5f,  0.4f - 0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* right, bottom, green quad */
		{
			const float data[] = { -0.4f + 0.5f, -0.4f - 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  -0.4f - 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   -0.4f + 0.5f, 0.4f - 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  0.4f - 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* left, top, blue quad */
		{
			const float data[] = { -0.4f - 0.5f, -0.4f + 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   0.4f - 0.5f,  -0.4f + 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   -0.4f - 0.5f, 0.4f + 0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   0.4f - 0.5f,  0.4f + 0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* right, top, yellow quad */
		{
			const float data[] = { -0.4f + 0.5f, -0.4f + 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  -0.4f + 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   -0.4f + 0.5f, 0.4f + 0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  0.4f + 0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 32 * 4, NULL, GL_STATIC_DRAW);

		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[0]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 0, sizeof(float) * 32);
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[1]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 32, sizeof(float) * 32);
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[2]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 2 * sizeof(float) * 32,
							sizeof(float) * 32);
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[3]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 3 * sizeof(float) * 32,
							sizeof(float) * 32);

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);

		glClear(GL_COLOR_BUFFER_BIT);
		for (int i = 0; i < 4; ++i)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[i]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		for (int i = 0; i < 4; ++i)
		{
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[4], i * sizeof(float) * 32,
							  sizeof(float) * 32);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
		}
		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(5, m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

class AdvancedSwitchBuffersCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[6];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(binding = 0, std430) buffer Input {" NL "  uint cookie[4];" NL
			   "} g_in;" NL "layout(binding = 1, std430) buffer Output {" NL "  uvec4 digest;" NL "} ;" NL
			   "void main() {" NL "  switch (g_in.cookie[0]+g_in.cookie[1]+g_in.cookie[2]+g_in.cookie[3]) {" NL
			   "    case 0x000000ffu: digest.x = 0xff000000u; break;" NL
			   "    case 0x0000ff00u: digest.y = 0x00ff0000u; break;" NL
			   "    case 0x00ff0000u: digest.z = 0x0000ff00u; break;" NL
			   "    case 0xff000000u: digest.w = 0x000000ffu; break;" NL "  }" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(6, m_storage_buffer);

		const GLubyte data0[] = { 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0, 0x88, 0, 0, 0, 0x22 };
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data0), data0, GL_STATIC_DRAW);
		const GLubyte data1[] = { 0, 0, 0x44, 0, 0, 0, 0x22, 0, 0, 0, 0x88, 0, 0, 0, 0x11, 0 };
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data1), data1, GL_STATIC_DRAW);
		const GLubyte data2[] = { 0, 0x88, 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0, 0x22, 0, 0 };
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data2), data2, GL_STATIC_DRAW);
		const GLubyte data3[] = { 0x22, 0, 0, 0, 0x88, 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0 };
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data3), data3, GL_STATIC_DRAW);

		GLint alignment;
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
		GLint offset = static_cast<GLint>(sizeof(data0) > (GLuint)alignment ? sizeof(data0) : alignment);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, offset * 4, NULL, GL_STATIC_DRAW);

		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[0]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 0, sizeof(data0));
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[1]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, offset, sizeof(data0));
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[2]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 2 * offset, sizeof(data0));
		glBindBuffer(GL_COPY_READ_BUFFER, m_storage_buffer[3]);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 3 * offset, sizeof(data0));

		const GLubyte data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[5]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		glUseProgram(m_program);
		for (int i = 0; i < 4; ++i)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[i]);
			glDispatchCompute(1, 1, 1);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[5]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLuint* out_data = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data0), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		GLuint expected[4] = { 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff };
		if (out_data[0] != expected[0] || out_data[1] != expected[1] || out_data[2] != expected[2] ||
			out_data[3] != expected[3])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Received: " << tcu::toHex(out_data[0]) << ", " << tcu::toHex(out_data[1])
				<< ", " << tcu::toHex(out_data[2]) << ", " << tcu::toHex(out_data[3])
				<< ", but expected: " << tcu::toHex(expected[0]) << ", " << tcu::toHex(expected[1]) << ", "
				<< tcu::toHex(expected[2]) << ", " << tcu::toHex(expected[3]) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		for (int i = 0; i < 4; ++i)
		{
			glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[4], i * offset, sizeof(data0));
			glDispatchCompute(1, 1, 1);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[5]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		out_data = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data0), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		if (out_data[0] != expected[0] || out_data[1] != expected[1] || out_data[2] != expected[2] ||
			out_data[3] != expected[3])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Received: " << tcu::toHex(out_data[0]) << ", " << tcu::toHex(out_data[1])
				<< ", " << tcu::toHex(out_data[2]) << ", " << tcu::toHex(out_data[3])
				<< ", but expected: " << tcu::toHex(expected[0]) << ", " << tcu::toHex(expected[1]) << ", "
				<< tcu::toHex(expected[2]) << ", " << tcu::toHex(expected[3]) << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(6, m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.2 AdvancedSwitchPrograms
//-----------------------------------------------------------------------------

class AdvancedSwitchPrograms : public ShaderStorageBufferObjectBase
{
	GLuint m_program[4];
	GLuint m_storage_buffer[4];
	GLuint m_vertex_array;
	GLuint m_fbo, m_rt;

	std::string GenSource(int binding)
	{
		std::stringstream ss;
		ss << NL "struct VertexData {" NL "  vec2 position;" NL "  vec3 color;" NL "};" NL "layout(binding = "
		   << binding << ", std430) buffer Input {" NL "  VertexData vertex[];" NL "} g_vs_in;" NL "out StageData {" NL
						 "  vec3 color;" NL "} g_vs_out;" NL "void main() {" NL
						 "  gl_Position = vec4(g_vs_in.vertex[gl_VertexID].position, 0, 1);" NL
						 "  g_vs_out.color = g_vs_in.vertex[gl_VertexID].color;" NL "}";
		return ss.str();
	}

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_fs_out;" NL
			   "void main() {" NL "  g_fs_out = vec4(g_fs_in.color, 1);" NL "}";
		for (int i = 0; i < 4; ++i)
		{
			m_program[i] = CreateProgram(GenSource(i), glsl_fs);
			glLinkProgram(m_program[i]);
			if (!CheckProgram(m_program[i]))
				return ERROR;
		}

		glGenBuffers(4, m_storage_buffer);

		/* left, bottom, red quad */
		{
			const float data[] = { -0.4f - 0.5f, -0.4f - 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   0.4f - 0.5f,  -0.4f - 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   -0.4f - 0.5f, 0.4f - 0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
								   0.4f - 0.5f,  0.4f - 0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* right, bottom, green quad */
		{
			const float data[] = { -0.4f + 0.5f, -0.4f - 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  -0.4f - 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   -0.4f + 0.5f, 0.4f - 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  0.4f - 0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* left, top, blue quad */
		{
			const float data[] = { -0.4f - 0.5f, -0.4f + 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   0.4f - 0.5f,  -0.4f + 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   -0.4f - 0.5f, 0.4f + 0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
								   0.4f - 0.5f,  0.4f + 0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* right, top, yellow quad */
		{
			const float data[] = { -0.4f + 0.5f, -0.4f + 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  -0.4f + 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   -0.4f + 0.5f, 0.4f + 0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
								   0.4f + 0.5f,  0.4f + 0.5f,  0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);

		glClear(GL_COLOR_BUFFER_BIT);
		for (int i = 0; i < 4; ++i)
		{
			glUseProgram(m_program[i]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glShaderStorageBlockBinding(m_program[0], 0, 3);
		glUseProgram(m_program[0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateWindow4Quads(vec3(0), vec3(0), vec3(1, 1, 0), vec3(0)))
		{
			return ERROR;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glShaderStorageBlockBinding(m_program[3], 0, 0);
		glUseProgram(m_program[3]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0), vec3(0), vec3(0)))
		{
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		for (int i = 0; i < 4; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(4, m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

class AdvancedSwitchProgramsCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program[4];
	GLuint m_storage_buffer[5];

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	std::string GenSource(int binding)
	{
		std::stringstream ss;
		ss << NL "layout(local_size_x = 1) in;" NL "layout(binding = " << binding
		   << ", std430) buffer Input {" NL "  uint cookie[4];" NL "} g_in;" NL
			  "layout(binding = 0, std430) buffer Output {" NL "  uvec4 digest;" NL "} ;" NL "void main() {" NL
			  "  switch (g_in.cookie[0]+g_in.cookie[1]+g_in.cookie[2]+g_in.cookie[3]) {" NL
			  "    case 0x000000ffu: digest.x = 0xff000000u; break;" NL
			  "    case 0x0000ff00u: digest.y = 0x00ff0000u; break;" NL
			  "    case 0x00ff0000u: digest.z = 0x0000ff00u; break;" NL
			  "    case 0xff000000u: digest.w = 0x000000ffu; break;" NL "  }" NL "}";
		return ss.str();
	}

	virtual long Run()
	{
		for (int i = 0; i < 4; ++i)
		{
			m_program[i] = CreateProgramCS(GenSource(i + 1));
			glLinkProgram(m_program[i]);
			if (!CheckProgram(m_program[i]))
				return ERROR;
		}

		glGenBuffers(5, m_storage_buffer);

		const GLubyte data0[] = { 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0, 0x88, 0, 0, 0, 0x22 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data0), data0, GL_STATIC_DRAW);
		const GLubyte data1[] = { 0, 0, 0x44, 0, 0, 0, 0x22, 0, 0, 0, 0x88, 0, 0, 0, 0x11, 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data1), data1, GL_STATIC_DRAW);
		const GLubyte data2[] = { 0, 0x88, 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0, 0x22, 0, 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data2), data2, GL_STATIC_DRAW);
		const GLubyte data3[] = { 0x22, 0, 0, 0, 0x88, 0, 0, 0, 0x11, 0, 0, 0, 0x44, 0, 0, 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data3), data3, GL_STATIC_DRAW);

		const GLubyte data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

		for (int i = 0; i < 4; ++i)
		{
			glUseProgram(m_program[i]);
			glDispatchCompute(1, 1, 1);
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLuint* out_data = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data0), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		GLuint expected[4] = { 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff };
		if (out_data[0] != expected[0] || out_data[1] != expected[1] || out_data[2] != expected[2] ||
			out_data[3] != expected[3])
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Received: " << tcu::toHex(out_data[0]) << ", " << tcu::toHex(out_data[1])
				<< ", " << tcu::toHex(out_data[2]) << ", " << tcu::toHex(out_data[3])
				<< ", but expected: " << tcu::toHex(expected[0]) << ", " << tcu::toHex(expected[1]) << ", "
				<< tcu::toHex(expected[2]) << ", " << tcu::toHex(expected[3]) << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		for (int i = 0; i < 4; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(5, m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.3.1 AdvancedWriteFragment
//-----------------------------------------------------------------------------

class AdvancedWriteFragment : public ShaderStorageBufferObjectBase
{
	GLuint m_program[2];
	GLuint m_storage_buffer;
	GLuint m_counter_buffer;
	GLuint m_attribless_vertex_array;
	GLuint m_draw_vertex_array;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		m_storage_buffer		  = 0;
		m_counter_buffer		  = 0;
		m_attribless_vertex_array = 0;
		m_draw_vertex_array		  = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs0 = NL
			"out StageData {" NL "  vec2 position;" NL "  vec3 color;" NL "} g_vs_out;" NL
			"const vec2 g_quad[4] = vec2[4](vec2(-0.4, -0.4), vec2(0.4, -0.4), vec2(-0.4, 0.4), vec2(0.4, 0.4));" NL
			"const vec2 g_offset[4] = vec2[4](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5));" NL
			"const vec3 g_color[4] = vec3[4](vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), vec3(1, 1, 0));" NL
			"void main() {" NL "  vec2 pos = g_quad[gl_VertexID] + g_offset[gl_InstanceID];" NL
			"  gl_Position = vec4(pos, 0, 1);" NL "  g_vs_out.position = pos;" NL
			"  g_vs_out.color = g_color[gl_InstanceID];" NL "}";

		const char* const glsl_fs0 =
			NL "in StageData {" NL "  vec2 position;" NL "  vec3 color;" NL "} g_fs_in;" NL
			   "layout(location = 0) out vec4 g_fs_out;" NL "struct FragmentData {" NL "  vec2 position;" NL
			   "  vec3 color;" NL "};" NL "layout(std430, binding = 3) buffer Output {" NL
			   "  FragmentData g_fragment[];" NL "};" NL "uniform uint g_max_fragment_count = 100 * 100;" NL
			   "layout(binding = 2, offset = 0) uniform atomic_uint g_fragment_counter;" NL "void main() {" NL
			   "  uint fragment_number = atomicCounterIncrement(g_fragment_counter);" NL
			   "  if (fragment_number < g_max_fragment_count) {" NL
			   "    g_fragment[fragment_number].position = g_fs_in.position;" NL
			   "    g_fragment[fragment_number].color = g_fs_in.color;" NL "  }" NL
			   "  g_fs_out = vec4(g_fs_in.color, 1);" NL "}";

		m_program[0] = CreateProgram(glsl_vs0, glsl_fs0);
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		const char* const glsl_vs1 =
			NL "layout(location = 0) in vec4 g_in_position;" NL "layout(location = 1) in vec4 g_in_color;" NL
			   "out StageData {" NL "  vec3 color;" NL "} g_vs_out;" NL "void main() {" NL
			   "  gl_Position = vec4(g_in_position.xy, 0, 1);" NL "  g_vs_out.color = g_in_color.rgb;" NL "}";

		const char* const glsl_fs1 =
			NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_fs_out;" NL
			   "void main() {" NL "  g_fs_out = vec4(g_fs_in.color, 1);" NL "}";

		m_program[1] = CreateProgram(glsl_vs1, glsl_fs1);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;

		// The first pass renders four squares on-screen, and writes a
		// record to the SSBO for each fragment processed.  The rectangles
		// will be 40x40 when using a 100x100 viewport, so we expect 1600
		// pixels per rectangle or 6400 pixels total.  Size the SSBO
		// accordingly, and render the second pass (sourcing the SSBO as a
		// vertex buffer) with an identical number of points.  If we have
		// a larger buffer and draw more points on the second pass, those
		// may overwrite "real" points using garbage position/color.
		int expectedPixels = 6400;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, expectedPixels * 32, NULL, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &m_counter_buffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_counter_buffer);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_DRAW);
		uvec4 zero(0);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 4, &zero);

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_attribless_vertex_array);

		glGenVertexArrays(1, &m_draw_vertex_array);
		glBindVertexArray(m_draw_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_storage_buffer);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 32, 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 32, reinterpret_cast<void*>(16));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[0]);
		glUniform1ui(glGetUniformLocation(m_program[0], "g_max_fragment_count"), expectedPixels);
		glBindVertexArray(m_attribless_vertex_array);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 4);

		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glBindVertexArray(m_draw_vertex_array);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glDrawArrays(GL_POINTS, 0, expectedPixels);
		int bad_pixels;

		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1), &bad_pixels) &&
			bad_pixels > 2)
		{
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		for (int i = 0; i < 2; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_counter_buffer);
		glDeleteVertexArrays(1, &m_attribless_vertex_array);
		glDeleteVertexArrays(1, &m_draw_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.3.2 AdvancedWriteGeometry
//-----------------------------------------------------------------------------
class AdvancedWriteGeometry : public ShaderStorageBufferObjectBase
{
	GLuint m_program[2];
	GLuint m_storage_buffer;
	GLuint m_vertex_array[2];
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		memset(m_program, 0, sizeof(m_program));
		m_storage_buffer = 0;
		memset(m_vertex_array, 0, sizeof(m_vertex_array));
		m_vertex_buffer = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInGS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs = NL "layout(location = 0) in vec4 g_in_position;" NL "void main() {" NL
									   "  gl_Position = g_in_position;" NL "}";

		const char* const glsl_gs =
			NL "layout(triangles) in;" NL "layout(triangle_strip, max_vertices = 3) out;" NL
			   "layout(std430, binding = 1) buffer OutputBuffer {" NL "  vec4 g_output_buffer[];" NL "};" NL
			   "void main() {" NL "  for (int i = 0; i < 3; ++i) {" NL
			   "    const int idx = gl_PrimitiveIDIn * 3 + i;" NL "    g_output_buffer[idx] = gl_in[i].gl_Position;" NL
			   "    gl_Position = g_output_buffer[idx];" NL "    EmitVertex();" NL "  }" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "void main() {" NL "  g_fs_out = vec4(0, 1, 0, 1);" NL "}";

		m_program[0] = CreateProgram(glsl_vs, "", "", glsl_gs, glsl_fs);
		glLinkProgram(m_program[0]);
		if (!CheckProgram(m_program[0]))
			return ERROR;

		m_program[1] = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program[1]);
		if (!CheckProgram(m_program[1]))
			return ERROR;

		/* vertex buffer */
		{
			const float data[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);

		glGenVertexArrays(2, m_vertex_array);

		glBindVertexArray(m_vertex_array[0]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glBindVertexArray(m_vertex_array[1]);
		glBindBuffer(GL_ARRAY_BUFFER, m_storage_buffer);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[0]);
		glBindVertexArray(m_vertex_array[0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glBindVertexArray(m_vertex_array[1]);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
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
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(2, m_vertex_array);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.3.3 AdvancedWriteTessellation
//-----------------------------------------------------------------------------

class AdvancedWriteTessellation : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_counter_buffer;
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		m_counter_buffer = 0;
		m_vertex_array   = 0;
		m_vertex_buffer  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInTES(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs = NL "layout(location = 0) in vec4 g_in_position;" NL "void main() {" NL
									   "  gl_Position = g_in_position;" NL "}";

		const char* const glsl_tes =
			NL "layout(quads) in;" NL "struct VertexData {" NL "  int valid;" NL "  vec4 position;" NL "};" NL
			   "layout(std430, binding = 2) buffer VertexBuffer {" NL "  VertexData g_vertex_buffer[];" NL "};" NL
			   "layout(binding = 2, offset = 0) uniform atomic_uint g_vertex_counter;" NL "void main() {" NL
			   "  const uint idx = atomicCounterIncrement(g_vertex_counter);" NL
			   "  vec4 p0 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);" NL
			   "  vec4 p1 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);" NL
			   "  vec4 p = mix(p0, p1, gl_TessCoord.y);" NL "  g_vertex_buffer[idx].position = p;" NL
			   "  g_vertex_buffer[idx].valid = 1;" NL "  gl_Position = g_vertex_buffer[idx].position;" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "void main() {" NL "  g_fs_out = vec4(0, 1, 0, 1);" NL "}";

		m_program = CreateProgram(glsl_vs, "", glsl_tes, "", glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		/* vertex buffer */
		{
			const float data[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenBuffers(1, &m_counter_buffer);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_counter_buffer);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, 4, NULL, GL_DYNAMIC_DRAW);
		uvec4 zero;
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, 4, &zero);

		struct
		{
			int  valid;
			int  pad[3];
			vec4 position;
		} data[6];
		memset(data, 0, sizeof(data));
		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glPatchParameteri(GL_PATCH_VERTICES, 4);
		glDrawArrays(GL_PATCHES, 0, 4);
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
		for (int i = 0; i < 4; ++i)
		{
			vec4 p = data[i].position;
			if (p[2] != 0.0f || p[3] != 1.0f)
				return ERROR;
			if (data[i].valid != 1)
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_counter_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.4.1 AdvancedIndirectAddressingCase1
//-----------------------------------------------------------------------------
class AdvancedIndirectAddressingCase1 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[4];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(4))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec2 g_in_position;" NL "struct Material {" NL "  vec3 color;" NL "};" NL
			   "layout(binding = 0, std430) buffer MaterialBuffer {" NL "  Material g_material[4];" NL "};" NL
			   "layout(binding = 1, std430) buffer MaterialIDBuffer {" NL "  uint g_material_id[4];" NL "};" NL
			   "layout(binding = 2, std430) buffer TransformBuffer {" NL "  vec2 translation[4];" NL "} g_transform;" NL
			   "layout(binding = 3, std430) buffer TransformIDBuffer {" NL "  uint g_transform_id[4];" NL "};" NL
			   "out StageData {" NL "  vec3 color;" NL "} g_vs_out;" NL "void main() {" NL
			   "  const uint mid = g_material_id[gl_InstanceID];" NL "  Material m = g_material[mid];" NL
			   "  const uint tid = g_transform_id[gl_InstanceID];" NL "  vec2 t = g_transform.translation[tid];" NL
			   "  gl_Position = vec4(g_in_position + t, 0, 1);" NL "  g_vs_out.color = m.color;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  vec3 color;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_fs_out;" NL
			   "void main() {" NL "  g_fs_out = vec4(g_fs_in.color, 1);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(4, m_storage_buffer);

		/* material buffer */
		{
			const float data[] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
								   0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* material id buffer */
		{
			const unsigned int data[] = { 2, 3, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* transform buffer */
		{
			const float data[] = { -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* transform id buffer */
		{
			const unsigned int data[] = { 3, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		/* vertex buffer */
		{
			const float data[] = { -0.4f, -0.4f, 0.4f, -0.4f, -0.4f, 0.4f, 0.4f, 0.4f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 4);

		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 0, 1), vec3(0, 0, 1)))
		{
			return ERROR;
		}

		/* update material id buffer with BufferSubData */
		{
			const unsigned int data[] = { 3, 2, 1, 0 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
		}

		/* update transform id buffer with BufferData */
		{
			const unsigned int data[] = { 0, 1, 2, 3 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 4);
		if (!ValidateWindow4Quads(vec3(1, 1, 0), vec3(0, 0, 1), vec3(1, 0, 0), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(4, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

class AdvancedIndirectAddressingCase1CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[5];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool status = true;

		const char* const glsl_cs =
			NL "layout(local_size_x = 2, local_size_y = 2) in;" NL "struct Material {" NL "  vec3 color;" NL "};" NL
			   "layout(binding = 0, std430) buffer MaterialBuffer {" NL "  Material g_material[4];" NL "};" NL
			   "layout(binding = 1, std430) buffer MaterialIDBuffer {" NL "  uint g_material_id[4];" NL "};" NL
			   "layout(binding = 2, std430) buffer TransformBuffer {" NL "  vec2 translation[4];" NL "} g_transform;" NL
			   "layout(binding = 3, std430) buffer TransformIDBuffer {" NL "  uint g_transform_id[4];" NL "};" NL
			   "layout(binding = 4, std430) buffer OutputBuffer {" NL "  vec3 color[16];" NL "  vec2 pos[16];" NL
			   "};" NL "vec2 g_in_position[4] = vec2[4](vec2(-0.4f, -0.4f), vec2(0.4f, -0.4f), vec2(-0.4f, 0.4f), "
			   "vec2(0.4f, 0.4f));" NL "void main() {" NL "  uint mid = g_material_id[gl_WorkGroupID.x];" NL
			   "  Material m = g_material[mid];" NL "  uint tid = g_transform_id[gl_WorkGroupID.x];" NL
			   "  vec2 t = g_transform.translation[tid];" NL
			   "  pos[gl_LocalInvocationIndex + gl_WorkGroupID.x * gl_WorkGroupSize.x * gl_WorkGroupSize.y] " NL
			   "    = g_in_position[gl_LocalInvocationIndex] + t;" NL "  color[gl_LocalInvocationIndex + "
			   "gl_WorkGroupID.x * gl_WorkGroupSize.x * "
			   "gl_WorkGroupSize.y] = m.color;" NL "}";

		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(5, m_storage_buffer);

		/* material buffer */
		{
			const float data[] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
								   0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* material id buffer */
		{
			const unsigned int data[] = { 2, 3, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* transform buffer */
		{
			const float data[] = { -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}
		/* transform id buffer */
		{
			const unsigned int data[] = { 3, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 16 * 4 * 4 + 16 * 2 * 4, 0, GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(4, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLfloat* out_data =
			(GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16 * 4 * 4 + 16 * 2 * 4, GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		GLfloat expected[16 * 4 + 16 * 2] = {
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
			1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.1f,  0.1f,  0.9f,  0.1f,  0.1f,  0.9f,
			0.9f,  0.9f,  0.1f,  -0.9f, 0.9f,  -0.9f, 0.1f,  -0.1f, 0.9f,  -0.1f, -0.9f, -0.9f, -0.1f, -0.9f,
			-0.9f, -0.1f, -0.1f, -0.1f, -0.9f, 0.1f,  -0.1f, 0.1f,  -0.9f, 0.9f,  -0.1f, 0.9f
		};
		for (int i = 0; i < 16; ++i)
		{
			if (out_data[i * 4 + 0] != expected[i * 4 + 0] || out_data[i * 4 + 1] != expected[i * 4 + 1] ||
				out_data[i * 4 + 2] != expected[i * 4 + 2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 4 + 0] << ", " << out_data[i * 4 + 1]
					<< ", " << out_data[i * 4 + 2] << ", but expected: " << expected[i * 4 + 0] << ", "
					<< expected[i * 4 + 1] << ", " << expected[i * 4 + 2] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		for (int i = 32; i < 32 + 16; ++i)
		{
			if (fabs(out_data[i * 2 + 0] - expected[i * 2 + 0]) > 1e-6 ||
				fabs(out_data[i * 2 + 1] - expected[i * 2 + 1]) > 1e-6)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 2 + 0] << ", " << out_data[i * 2 + 1]
					<< ", but expected: " << expected[i * 2 + 0] << ", " << expected[i * 2 + 1]
					<< tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		/* update material id buffer with BufferSubData */
		{
			const unsigned int data[] = { 3, 2, 1, 0 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
		}

		/* update transform id buffer with BufferData */
		{
			const unsigned int data[] = { 0, 1, 2, 3 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);
		}

		glUseProgram(m_program);
		glDispatchCompute(4, 1, 1);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLfloat* out_data2 =
			(GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16 * 4 * 4 + 16 * 2 * 4, GL_MAP_READ_BIT);
		if (!out_data2)
			return ERROR;
		GLfloat expected2[16 * 4 + 16 * 2] = {
			1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f, 0.0f,
			0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f,
			1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
			-0.9f, -0.9f, -0.1f, -0.9f, -0.9f, -0.1f, -0.1f, -0.1f, 0.1f, -0.9f, 0.9f, -0.9f, 0.1f, -0.1f, 0.9f, -0.1f,
			-0.9f, 0.1f,  -0.1f, 0.1f,  -0.9f, 0.9f,  -0.1f, 0.9f,  0.1f, 0.1f,  0.9f, 0.1f,  0.1f, 0.9f,  0.9f, 0.9f
		};
		for (int i = 0; i < 16; ++i)
		{
			if (out_data2[i * 4 + 0] != expected2[i * 4 + 0] || out_data2[i * 4 + 1] != expected2[i * 4 + 1] ||
				out_data2[i * 4 + 2] != expected2[i * 4 + 2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data2[i * 4 + 0] << ", " << out_data2[i * 4 + 1]
					<< ", " << out_data2[i * 4 + 2] << ", but expected: " << expected2[i * 4 + 0] << ", "
					<< expected2[i * 4 + 1] << ", " << expected2[i * 4 + 2] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		for (int i = 32; i < 32 + 16; ++i)
		{
			if (fabs(out_data2[i * 2 + 0] - expected2[i * 2 + 0]) > 1e-6 ||
				fabs(out_data2[i * 2 + 1] - expected2[i * 2 + 1]) > 1e-6)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data2[i * 2 + 0] << ", " << out_data2[i * 2 + 1]
					<< ", but expected: " << expected2[i * 2 + 0] << ", " << expected2[i * 2 + 1]
					<< tcu::TestLog::EndMessage;
				status = false;
			}
		}

		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(5, m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.4.2 AdvancedIndirectAddressingCase2
//-----------------------------------------------------------------------------

class AdvancedIndirectAddressingCase2 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[8];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(4))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 1) in vec2 g_in_position;" NL "layout(binding = 0, std430) buffer Transform {" NL
			   "  vec2 translation;" NL "} g_transform[4];" NL "uniform uint g_transform_id = 2;" NL "void main() {" NL
			   "  gl_Position = vec4(g_in_position + g_transform[g_transform_id].translation, 0, 1);" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "layout(binding = 4, std430) buffer Material {" NL
			   "  vec3 color;" NL "} g_material[4];" NL "uniform int g_material_id = 1;" NL "void main() {" NL
			   "  g_fs_out = vec4(g_material[g_material_id].color, 1);" NL "}";
		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(8, m_storage_buffer);

		/* transform buffers */
		{
			const float data[4][2] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { -0.5f, 0.5f }, { 0.5f, 0.5f } };

			for (GLuint i = 0; i < 4; ++i)
			{
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data[i]), data[i], GL_DYNAMIC_DRAW);
			}
		}
		/* material buffers */
		{
			const float data[4][4] = { { 1.0f, 0.0f, 0.0f, 0.0f },
									   { 0.0f, 1.0f, 0.0f, 0.0f },
									   { 0.0f, 0.0f, 1.0f, 0.0f },
									   { 1.0f, 1.0f, 0.0f, 0.0f } };
			for (GLuint i = 0; i < 4; ++i)
			{
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i + 4, m_storage_buffer[i + 4]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data[i]), data[i], GL_DYNAMIC_DRAW);
			}
		}

		/* vertex buffer */
		{
			const float data[] = { -0.4f, -0.4f, 0.4f, -0.4f, -0.4f, 0.4f, 0.4f, 0.4f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);

		glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateWindow4Quads(vec3(0), vec3(0), vec3(0), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 2);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateWindow4Quads(vec3(0, 0, 1), vec3(0), vec3(0), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 1);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 3);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
		if (!ValidateWindow4Quads(vec3(0, 0, 1), vec3(1, 1, 0), vec3(0), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 3);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateWindow4Quads(vec3(0, 0, 1), vec3(1, 1, 0), vec3(1, 0, 0), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		// once again with only one validation at the end
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 2);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 1);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 2);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 1);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 3);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glUniform1ui(glGetUniformLocation(m_program, "g_transform_id"), 3);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[7]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateWindow4Quads(vec3(0, 0, 1), vec3(1, 1, 0), vec3(1, 1, 0), vec3(0, 1, 0)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(8, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

class AdvancedIndirectAddressingCase2CS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[5];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		GLint blocksC;
		glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &blocksC);
		if (blocksC < 8)
			return NOT_SUPPORTED;
		bool status = true;

		const char* const glsl_cs =
			NL "layout(local_size_x = 4) in;" NL "layout(binding = 0, std430) buffer Material {" NL "  vec3 color;" NL
			   "} g_material[4];" NL "layout(binding = 4, std430) buffer OutputBuffer {" NL "  vec3 color[4];" NL
			   "};" NL "uniform int g_material_id;" NL "void main() {" NL
			   "  color[gl_LocalInvocationIndex] = vec3(g_material[g_material_id].color);" NL "}";

		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(5, m_storage_buffer);

		/* material buffers */
		const float data[4][3] = {
			{ 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }
		};

		for (GLuint i = 0; i < 4; ++i)
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data[i]), data[i], GL_DYNAMIC_DRAW);
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * 4 * 4, 0, GL_STATIC_DRAW);

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 1);
		glDispatchCompute(1, 1, 1);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLfloat* out_data = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4 * 4, GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		const float* expected = &data[1][0];

		for (int i = 0; i < 4; ++i)
		{
			if (out_data[i * 4 + 0] != expected[0] || out_data[i * 4 + 1] != expected[1] ||
				out_data[i * 4 + 2] != expected[2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 4 + 0] << ", " << out_data[i * 4 + 1]
					<< ", " << out_data[i * 4 + 2] << ", but expected: " << expected[0] << ", " << expected[1] << ", "
					<< expected[2] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glUniform1i(glGetUniformLocation(m_program, "g_material_id"), 3);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		out_data = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4 * 4, GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		expected = &data[3][0];

		for (int i = 0; i < 4; ++i)
		{
			if (out_data[i * 4 + 0] != expected[0] || out_data[i * 4 + 1] != expected[1] ||
				out_data[i * 4 + 2] != expected[2])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 4 + 0] << ", " << out_data[i * 4 + 1]
					<< ", " << out_data[i * 4 + 2] << ", but expected: " << expected[0] << ", " << expected[1] << ", "
					<< expected[2] << tcu::TestLog::EndMessage;
				status = false;
			}
		}

		if (!status)
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(5, m_storage_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.5.1 AdvancedReadWriteCase1
//-----------------------------------------------------------------------------
class AdvancedReadWriteCase1 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer;
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;

	virtual long Setup()
	{
		m_program		 = 0;
		m_storage_buffer = 0;
		m_vertex_array   = 0;
		m_vertex_buffer  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs = NL "layout(location = 0) in vec4 g_in_position;" NL "coherent buffer Buffer {" NL
									   "  vec4 in_color;" NL "  vec4 out_color;" NL "} g_buffer;" NL "void main() {" NL
									   "  if (gl_VertexID == 0) {" NL "    g_buffer.out_color = g_buffer.in_color;" NL
									   "    memoryBarrier();" NL "  }" NL "  gl_Position = g_in_position;" NL "}";

		const char* const glsl_fs =
			NL "layout(location = 0) out vec4 g_fs_out;" NL "coherent buffer Buffer {" NL "  vec4 in_color;" NL
			   "  vec4 out_color;" NL "} g_buffer;" NL "void main() {" NL "  g_fs_out = g_buffer.out_color;" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);
		float* ptr = reinterpret_cast<float*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
		if (!ptr)
			return ERROR;
		*ptr++ = 0.0f;
		*ptr++ = 1.0f;
		*ptr++ = 0.0f;
		*ptr++ = 1.0f;
		*ptr++ = 0.0f;
		*ptr++ = 0.0f;
		*ptr++ = 0.0f;
		*ptr++ = 0.0f;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		/* vertex buffer */
		{
			const float data[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(0, 1, 0)))
		{
			return ERROR;
		}

		// update input color
		ptr = reinterpret_cast<float*>(
			glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 4, GL_MAP_WRITE_BIT));
		if (!ptr)
			return ERROR;
		*ptr++ = 1.0f;
		*ptr++ = 0.0f;
		*ptr++ = 1.0f;
		*ptr++ = 1.0f;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec3(1.0f, 0.0f, 1.0f)))
		{
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedReadWriteCase1CS : public ShaderStorageBufferObjectBase
{
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
		bool status = true;

		const char* const glsl_cs = NL
			"layout(local_size_x = 128) in;" NL "struct s {" NL "  int ene;" NL "  int due;" NL "  int like;" NL
			"  int fake;" NL "};" NL "layout(std430) coherent buffer Buffer {" NL "  s a[128];" NL "} g_buffer;" NL
			"void main() {" NL "  g_buffer.a[gl_LocalInvocationIndex].due = g_buffer.a[gl_LocalInvocationIndex].ene;" NL
			"  groupMemoryBarrier();" NL "  barrier();" NL "  g_buffer.a[(gl_LocalInvocationIndex + 1u) % 128u].like = "
			"g_buffer.a[(gl_LocalInvocationIndex + 1u) % 128u].due;" NL "  groupMemoryBarrier();" NL "  barrier();" NL
			"  g_buffer.a[(gl_LocalInvocationIndex + 17u) % 128u].fake "
			"= g_buffer.a[(gl_LocalInvocationIndex + 17u) % "
			"128u].like;" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		GLint data[128 * 4];
		for (int i = 0; i < 128; ++i)
		{
			data[i * 4]		= i + 256;
			data[i * 4 + 1] = 0;
			data[i * 4 + 2] = 0;
			data[i * 4 + 3] = 0;
		}
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data[0], GL_DYNAMIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLint* out_data = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		for (int i = 0; i < 128; ++i)
		{
			if (out_data[i * 4 + 3] != data[i * 4])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 4 + 3] << ", "
					<< ", but expected: " << data[i * 4] << " -> " << out_data[i * 4 + 1] << " -> "
					<< out_data[i * 4 + 2] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		for (int i = 0; i < 128; ++i)
		{
			data[i * 4]		= i + 512;
			data[i * 4 + 1] = 0;
			data[i * 4 + 2] = 0;
			data[i * 4 + 3] = 0;
		}
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data[0], GL_DYNAMIC_DRAW);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		out_data = (GLint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;
		for (int i = 0; i < 128; ++i)
		{
			if (out_data[i * 4 + 3] != data[i * 4])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Received: " << out_data[i * 4 + 3] << ", "
					<< ", but expected: " << data[i * 4] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.6.1 AdvancedUsageCase1
//-----------------------------------------------------------------------------

class AdvancedUsageCase1 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[3];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "layout(location = 1) in int g_object_id;" NL
			   "out StageData {" NL "  flat int object_id;" NL "} g_vs_out;" NL
			   "layout(binding = 0, std430) buffer TransformBuffer {" NL "  mat4 g_transform[];" NL "};" NL
			   "void main() {" NL "  mat4 mvp = g_transform[g_object_id];" NL "  gl_Position = mvp * g_position;" NL
			   "  g_vs_out.object_id = g_object_id;" NL "}";

		const char* const glsl_fs = NL
			"in StageData {" NL "  flat int object_id;" NL "} g_fs_in;" NL "layout(location = 0) out vec4 g_fs_out;" NL
			"struct Material {" NL "  vec3 color;" NL "};" NL "layout(binding = 1, std430) buffer MaterialBuffer {" NL
			"  Material g_material[4];" NL "};" NL "layout(binding = 2, std430) buffer MaterialIDBuffer {" NL
			"  int g_material_id[4];" NL "};" NL "void main() {" NL "  int mid = g_material_id[g_fs_in.object_id];" NL
			"  Material m = g_material[mid];" NL "  g_fs_out = vec4(m.color, 1);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(3, m_storage_buffer);

		/* transform buffer */
		{
			mat4 data[] = { Translation(-0.5f, -0.5f, 0.0f), Translation(0.5f, -0.5f, 0.0f),
							Translation(-0.5f, 0.5f, 0.0f), Translation(0.5f, 0.5f, 0.0f) };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* material buffer */
		{
			vec4 data[] = { vec4(1, 0, 0, 1), vec4(0, 1, 0, 1), vec4(0, 0, 1, 0), vec4(1, 1, 0, 1) };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* material id buffer */
		{
			int data[] = { 0, 1, 2, 3 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* vertex buffer */
		{
			struct
			{
				vec2 position;
				int  object_id;
			} data[] = { { vec2(-0.4f, -0.4f), 0 }, { vec2(0.4f, -0.4f), 0 },  { vec2(-0.4f, 0.4f), 0 },
						 { vec2(0.4f, 0.4f), 0 },   { vec2(-0.4f, -0.4f), 1 }, { vec2(0.4f, -0.4f), 1 },
						 { vec2(-0.4f, 0.4f), 1 },  { vec2(0.4f, 0.4f), 1 },   { vec2(-0.4f, -0.4f), 2 },
						 { vec2(0.4f, -0.4f), 2 },  { vec2(-0.4f, 0.4f), 2 },  { vec2(0.4f, 0.4f), 2 },
						 { vec2(-0.4f, -0.4f), 3 }, { vec2(0.4f, -0.4f), 3 },  { vec2(-0.4f, 0.4f), 3 },
						 { vec2(0.4f, 0.4f), 3 } };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2) + sizeof(int), 0);
		glVertexAttribIPointer(1, 1, GL_INT, sizeof(vec2) + sizeof(int), reinterpret_cast<void*>(sizeof(vec2)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		/* draw */
		{
			const GLint   first[4] = { 0, 4, 8, 12 };
			const GLsizei count[4] = { 4, 4, 4, 4 };
			glMultiDrawArrays(GL_TRIANGLE_STRIP, first, count, 4);
		}
		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(0, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(3, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.6.2 AdvancedUsageSync
//-----------------------------------------------------------------------------

class AdvancedUsageSync : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[7];
	GLuint m_vertex_array;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(3))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(std430, binding = 0) coherent buffer Buffer0 {" NL "  int g_data0, g_inc0;" NL
			   "  int g_data1, g_inc1;" NL "};" NL "layout(std430, binding = 1) buffer Buffer12 {" NL
			   "  int inc, data;" NL "} g_buffer12[2];" NL NL "void Modify(int path) {" NL "  if (path == 0) {" NL
			   "    atomicAdd(g_data0, g_inc0);" NL "    atomicAdd(g_data1, g_inc0);" NL "  } else if (path == 1) {" NL
			   "    atomicAdd(g_data0, - g_inc0);" NL "    atomicAdd(g_data1, - g_inc0);" NL
			   "  } else if (path == 2) {" NL "    atomicAdd(g_data0, g_inc1);" NL "    atomicAdd(g_data1, g_inc1);" NL
			   "  }" NL NL "  if (path == 0) {" NL "    g_buffer12[0].data += g_buffer12[1].inc;" NL
			   "  } else if (path == 1) {" NL "    g_buffer12[1].data += g_buffer12[0].inc;" NL "  }" NL "}" NL NL
			   "void main() {" NL "  Modify(gl_VertexID);" NL "  gl_Position = vec4(0, 0, 0, 1);" NL "}";

		const char* glsl_fs =
			NL "layout(binding = 3, std430) coherent buffer Buffer3 {" NL "  int data;" NL "} g_buffer3;" NL
			   "layout(std430, binding = 4) coherent buffer Buffer4 {" NL "  int data0, inc0;" NL
			   "  int data1, inc1;" NL "} g_buffer4;" NL "layout(std430, binding = 5) buffer Buffer56 {" NL
			   "  int inc, data;" NL "} g_buffer56[2];" NL NL "void ModifyFS(int path) {" NL "  if (path == 0) {" NL
			   "    atomicAdd(g_buffer4.data0, g_buffer4.inc0);" NL "    atomicAdd(g_buffer4.data1, g_buffer4.inc0);" NL
			   "  } else if (path == 1) {" NL "    atomicAdd(g_buffer4.data0, - g_buffer4.inc0);" NL
			   "    atomicAdd(g_buffer4.data1, - g_buffer4.inc0);" NL "  } else if (path == 2) {" NL
			   "    atomicAdd(g_buffer4.data0, g_buffer4.inc1);" NL "    atomicAdd(g_buffer4.data1, g_buffer4.inc1);" NL
			   "  }" NL NL "  if (path == 0) {" NL "    g_buffer56[0].data += g_buffer56[1].inc;" NL
			   "  } else if (path == 1) {" NL "    g_buffer56[1].data += g_buffer56[0].inc;" NL "  }" NL "}" NL
			   "void main() {" NL "  atomicAdd(g_buffer3.data, 1);" NL "  ModifyFS(gl_PrimitiveID);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenVertexArrays(1, &m_vertex_array);
		glGenBuffers(7, m_storage_buffer);

		/* Buffer0 */
		{
			int data[4] = { 0, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer1 */
		{
			int data[2] = { 3, 1 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer2 */
		{
			int data[2] = { 2, 4 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer3 */
		{
			int data[1] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer4 */
		{
			int data[4] = { 0, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer5 */
		{
			int data[2] = { 3, 1 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_storage_buffer[5]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer6 */
		{
			int data[2] = { 2, 4 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_storage_buffer[6]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}

		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);

		glDrawArrays(GL_POINTS, 0, 3);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDrawArrays(GL_POINTS, 0, 3);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* Buffer0 */
		{
			const int ref_data[4] = { 4, 1, 4, 2 };
			int		  data[4];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 4; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer0] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer1 */
		{
			const int ref_data[2] = { 3, 5 };
			int		  data[2];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer1] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer2 */
		{
			const int ref_data[2] = { 2, 10 };
			int		  data[2];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer2] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer3 */
		{
			const int ref_data[1] = { 6 };
			int		  data[1];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 1; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer3] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer4 */
		{
			const int ref_data[4] = { 4, 1, 4, 2 };
			int		  data[4];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 4; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer4] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer5 */
		{
			const int ref_data[2] = { 3, 5 };
			int		  data[2];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[5]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer5] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer6 */
		{
			const int ref_data[2] = { 2, 10 };
			int		  data[2];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[6]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer6] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
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
		glDeleteBuffers(7, m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedUsageSyncCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[7];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430, binding = 0) coherent buffer Buffer0 {" NL
			   "  int g_data0, g_inc0;" NL "  int g_data1, g_inc1;" NL "};" NL
			   "layout(std430, binding = 1) buffer Buffer12 {" NL "  int inc, data;" NL "} g_buffer12[2];" NL
			   "layout(binding = 3, std430) coherent buffer Buffer3 {" NL "  int data;" NL "} g_buffer3;" NL
			   "layout(std430, binding = 4) coherent buffer Buffer4 {" NL "  int data0, inc0;" NL
			   "  int data1, inc1;" NL "} g_buffer4;" NL "layout(std430, binding = 5) buffer Buffer56 {" NL
			   "  int inc, data;" NL "} g_buffer56[2];" NL NL "void Modify1(int path) {" NL "  if (path == 0) {" NL
			   "    atomicAdd(g_data0, g_inc0);" NL "    atomicAdd(g_data1, g_inc0);" NL "  } else if (path == 1) {" NL
			   "    atomicAdd(g_data0, - g_inc0);" NL "    atomicAdd(g_data1, - g_inc0);" NL
			   "  } else if (path == 2) {" NL "    atomicAdd(g_data0, g_inc1);" NL "    atomicAdd(g_data1, g_inc1);" NL
			   "  }" NL "  if (path == 0) {" NL "    g_buffer12[0].data += g_buffer12[1].inc;" NL
			   "  } else if (path == 1) {" NL "    g_buffer12[1].data += g_buffer12[0].inc;" NL "  }" NL "}" NL NL
			   "void Modify2(int path) {" NL "  if (path == 0) {" NL
			   "    atomicAdd(g_buffer4.data0, g_buffer4.inc0);" NL "    atomicAdd(g_buffer4.data1, g_buffer4.inc0);" NL
			   "  } else if (path == 1) {" NL "    atomicAdd(g_buffer4.data0, - g_buffer4.inc0);" NL
			   "    atomicAdd(g_buffer4.data1, - g_buffer4.inc0);" NL "  } else if (path == 2) {" NL
			   "    atomicAdd(g_buffer4.data0, g_buffer4.inc1);" NL "    atomicAdd(g_buffer4.data1, g_buffer4.inc1);" NL
			   "  }" NL "  if (path == 0) {" NL "    g_buffer56[0].data += g_buffer56[1].inc;" NL
			   "  } else if (path == 1) {" NL "    g_buffer56[1].data += g_buffer56[0].inc;" NL "  }" NL "}" NL NL
			   "void main() {" NL "  Modify1(int(gl_WorkGroupID.z));" NL "  atomicAdd(g_buffer3.data, 1);" NL
			   "  Modify2(int(gl_WorkGroupID.z));" NL "}";

		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(7, m_storage_buffer);

		/* Buffer0 */
		{
			int data[4] = { 0, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer1 */
		{
			int data[2] = { 3, 1 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer2 */
		{
			int data[2] = { 2, 4 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer3 */
		{
			int data[1] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer4 */
		{
			int data[4] = { 0, 1, 0, 2 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer5 */
		{
			int data[2] = { 3, 1 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, m_storage_buffer[5]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer6 */
		{
			int data[2] = { 2, 4 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_storage_buffer[6]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}

		glUseProgram(m_program);

		glDispatchCompute(1, 1, 3);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute(1, 1, 3);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* Buffer0 */
		{
			const int ref_data[4] = { 4, 1, 4, 2 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 4; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer0] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer1 */
		{
			const int ref_data[2] = { 3, 5 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer1] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer2 */
		{
			const int ref_data[2] = { 2, 10 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[2]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer2] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer3 */
		{
			const int ref_data[1] = { 6 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[3]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 1; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer3] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer4 */
		{
			const int ref_data[4] = { 4, 1, 4, 2 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 4; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer4] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer5 */
		{
			const int ref_data[2] = { 3, 5 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[5]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer5] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		/* Buffer6 */
		{
			const int ref_data[2] = { 2, 10 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[6]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 8, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer6] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(7, m_storage_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.6.3 AdvancedUsageOperators
//-----------------------------------------------------------------------------
class AdvancedUsageOperators : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];
	GLuint m_vertex_array;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(std430, binding = 0) buffer Buffer0 {" NL "  readonly int g_i0;" NL "  int g_o0;" NL "};" NL
			   "layout(std430, binding = 1) buffer Buffer1 {" NL "  int i0;" NL "} g_buffer1;" NL
			   "uniform int g_values[] = int[](1, 2, 3, 4, 5, 6);" NL "void main() {" NL "  g_o0 += g_i0;" NL
			   "  g_o0 <<= 1;" NL "  g_o0 = g_i0 > g_o0 ? g_i0 : g_o0;" NL "  g_o0 *= g_i0;" NL
			   "  g_o0 = --g_o0 + g_values[g_i0];" NL "  g_o0++;" NL "  ++g_o0;" NL "  g_buffer1.i0 = 0xff2f;" NL
			   "  g_o0 &= g_buffer1.i0;" NL "}";

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenVertexArrays(1, &m_vertex_array);
		glGenBuffers(2, m_storage_buffer);

		/* Buffer0 */
		{
			int data[4] = { 3, 5 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer1 */
		{
			int data[1] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* Buffer0 */
		{
			const int ref_data[2] = { 3, 37 };
			int		  data[4];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer0] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		/* Buffer0 */
		{
			const int ref_data[1] = { 0xff2f };
			int		  data[1];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[1]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			for (int i = 0; i < 1; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer1] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedUsageOperatorsCS : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[2];

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430, binding = 0) buffer Buffer0 {" NL
			   "  readonly int g_i0;" NL "  int g_o0;" NL "};" NL "layout(std430, binding = 1) buffer Buffer1 {" NL
			   "  int i0;" NL "} g_buffer1;" NL "const int g_values[6] = int[](1, 2, 3, 4, 5, 6);" NL "void main() {" NL
			   "  g_o0 += g_i0;" NL "  g_o0 <<= 1;" NL "  g_o0 = g_i0 > g_o0 ? g_i0 : g_o0;" NL "  g_o0 *= g_i0;" NL
			   "  g_o0 = --g_o0 + g_values[g_i0];" NL "  g_o0++;" NL "  ++g_o0;" NL "  g_buffer1.i0 = 0xff2f;" NL
			   "  g_o0 &= g_buffer1.i0;" NL "}";

		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(2, m_storage_buffer);

		/* Buffer0 */
		{
			int data[4] = { 3, 5 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* Buffer1 */
		{
			int data[1] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* Buffer0 */
		{
			const int ref_data[2] = { 3, 37 };
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[0]);
			int* data = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 16, GL_MAP_READ_BIT);
			if (!data)
				return ERROR;
			for (int i = 0; i < 2; ++i)
			{
				if (data[i] != ref_data[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "[Buffer0] Data at index " << i << " is " << data[i]
						<< " should be " << ref_data[i] << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(2, m_storage_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.7 AdvancedUnsizedArrayLength
//-----------------------------------------------------------------------------
class AdvancedUnsizedArrayLength : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[5];
	GLuint m_vertex_array;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(5))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(std430, binding = 0) readonly buffer Input0 {" NL "  int g_input0[];" NL "};" NL
			   "layout(std430, binding = 1) readonly buffer Input1 {" NL "  int data[];" NL "} g_input1;" NL
			   "layout(std430, binding = 2) readonly buffer Input23 {" NL "  int data[];" NL "} g_input23[2];" NL
			   "layout(std430, binding = 4) buffer Output {" NL "  int g_length[4];" NL "  int g_length2;" NL "};" NL
			   "void main() {" NL "  g_length[0] = g_input0.length();" NL "  g_length[1] = g_input1.data.length();" NL
			   "  g_length[2] = g_input23[0].data.length();" NL "  g_length[3] = g_input23[1].data.length();" NL
			   "  g_length2 = g_length.length();" NL "}";

		m_program = CreateProgram(glsl_vs, "");
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenVertexArrays(1, &m_vertex_array);
		glGenBuffers(5, m_storage_buffer);

		/* input 0 */
		{
			int data[7] = { 1, 2, 3, 4, 5, 6, 7 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* input 1 */
		{
			int data[5] = { 1, 2, 3, 4, 5 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* input 2 */
		{
			int data[3] = { 1, 2, 3 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_storage_buffer[2]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* input 3 */
		{
			int data[4] = { 1, 2, 3, 4 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_storage_buffer[3]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}
		/* output */
		{
			int data[5] = { 0 };
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_storage_buffer[4]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_DYNAMIC_COPY);
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArrays(GL_POINTS, 0, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		/* output */
		{
			int data[5];
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), data);
			bool status = true;
			if (data[0] != 7)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 0 length is " << data[0]
													<< " should be 7." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (data[1] != 5)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 1 length is " << data[1]
													<< " should be 5." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (data[2] != 3)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 2 length is " << data[2]
													<< " should be 3." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (data[3] != 4)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 3 length is " << data[3]
													<< " should be 4." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (data[4] != 4)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 4 length is " << data[4]
													<< " should be 4." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (!status)
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(5, m_storage_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}
};

class AdvancedUnsizedArrayLength2 : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[8];
	GLuint m_vertex_array;

	virtual void SetPath() = 0;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array = 0;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(8, m_storage_buffer);
		if (stage != compute)
			glDeleteVertexArrays(1, &m_vertex_array);
		return NO_ERROR;
	}

	std::string BuildShaderPT(int stagept)
	{
		std::ostringstream os;
		if (stagept == vertex)
		{
			os << NL "void main() {" NL "  gl_Position = vec4(0,0,0,1);";
		}
		if (stagept == fragment)
		{
			os << NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
					 "  o_color = vec4(0.0, 1.0, 0.0, 1.0);";
		}
		os << NL "}";
		return os.str();
	}

	std::string BuildShader()
	{
		std::ostringstream os;
		std::string		   e[4][7] = { { "bvec3", "vec4", "ivec3", "ivec3", "uvec2", "vec2", "uvec4" },
								{ "mat2", "mat3", "mat4", "mat4", "mat2x3", "mat3x2", "mat4x2" },
								{ "mat2", "mat3", "mat4", "mat4", "mat2x3", "mat3x2", "mat4x2" },
								{ "S0", "S1", "S2", "S2", "S4", "S5", "S6" } };

		std::string sd =
			NL "struct S0 {" NL "  float f;" NL "  int i;" NL "  uint ui;" NL "  bool b;" NL "};" NL "struct S1 {" NL
			   "  ivec3 iv;" NL "  bvec2 bv;" NL "  vec4 v;" NL "  uvec2 uv;" NL "};" NL "struct S2 {" NL
			   "  mat2x2 m22;" NL "  mat4x4 m44;" NL "  mat2x3 m23;" NL "  mat4x2 m42;" NL "  mat3x4 m34;" NL "};" NL
			   "struct S4 {" NL "  float f[1];" NL "  int i[2];" NL "  uint ui[3];" NL "  bool b[4];" NL
			   "  ivec3 iv[5];" NL "  bvec2 bv[6];" NL "  vec4  v[7];" NL "  uvec2 uv[8];" NL "};" NL "struct S5 {" NL
			   "  S0 s0;" NL "  S1 s1;" NL "  S2 s2;" NL "};" NL "struct S6 {" NL "  S4 s4[3];" NL "};";

		std::string lo   = "";
		std::string l[4] = { "std140", "std430", "shared", "packed" };
		lo += l[layout];

		if (etype == matrix_rm)
			lo += ", row_major";

		std::string decl = sd + NL "layout(" + lo + ") buffer;" NL "layout(binding = 0) readonly buffer Input0 {" +
						   ((other_members) ? ("\n  " + e[etype][0] + " pad0;") : "") + NL "  " + e[etype][0] +
						   " g_input0[];" NL "};" NL "layout(binding = 1) readonly buffer Input1 {" +
						   ((other_members) ? ("\n  " + e[etype][1] + " pad1;") : "") + NL "  " + e[etype][1] +
						   " data[];" NL "} g_input1;" NL "layout(binding = 2) readonly buffer Input23 {" +
						   ((other_members) ? ("\n  " + e[etype][2] + " pad2;") : "") + NL "  " + e[etype][2] +
						   " data[];" NL "} g_input23[2];" NL "layout(binding = 4) buffer Output0 {" +
						   ((other_members) ? ("\n  " + e[etype][4] + " pad4;") : "") + NL "  " + e[etype][4] +
						   " data[];" NL "} g_output0;" NL "layout(binding = 5) readonly buffer Input4 {" +
						   ((other_members) ? ("\n  " + e[etype][5] + " pad5;") : "") + NL "  " + e[etype][5] +
						   " data[];" NL "} g_input4;" NL "layout(binding = 6) buffer Output1 {" +
						   ((other_members) ? ("\n  " + e[etype][6] + " pad6;") : "") + NL "  " + e[etype][6] +
						   " data[];" NL "} g_output1;" NL "layout(std430, binding = 7) buffer Output {" NL
						   "  int g_length[];" NL "};";

		std::string expr =
			NL "  g_length[0] = g_input0.length();" NL "  g_length[1] = g_input1.data.length();" NL
			   "  g_length[2] = g_input23[0].data.length();" NL "  g_length[3] = g_input23[1].data.length();" NL
			   "  g_length[4] = g_output0.data.length();" NL "  g_length[5] = g_input4.data.length();" NL
			   "  g_length[6] = g_output1.data.length();";

		std::string lastelemexpr =
			NL "  g_output0.data[g_output0.data.length()-2] += g_output0.data[g_output0.data.length()-1];" NL
			   "  g_output1.data[g_output1.data.length()-2] += g_output1.data[g_output1.data.length()-1];";

		if (length_as_index)
			expr += lastelemexpr;
		if (stage == vertex)
		{
			os << decl << NL "void main() {" NL "  gl_Position = vec4(0,0,0,1);" << expr;
		}
		if (stage == fragment)
		{
			os << NL "layout(location = 0) out vec4 o_color;" << decl
			   << NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" << expr;
		}
		if (stage == compute)
		{
			os << NL "layout(local_size_x = 1) in;" << decl << NL "void main() {" << expr;
		}
		os << NL "}";
		return os.str();
	}

	virtual long Run()
	{
		const int kSize = 100000;
		const int kBufs = 8;
		SetPath();
		if (stage == vertex && !SupportedInVS(8))
			return NOT_SUPPORTED;
		GLint blocksC;
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &blocksC);
		GLint minA;
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &minA);
		if (blocksC < kBufs)
			return NOT_SUPPORTED;
		if (stage == vertex)
		{
			std::string glsl_vs = BuildShader();
			std::string glsl_fs = BuildShaderPT(fragment);
			m_program			= CreateProgram(glsl_vs, glsl_fs);
		}
		else if (stage == fragment)
		{
			std::string glsl_vs = BuildShaderPT(vertex);
			std::string glsl_fs = BuildShader();
			m_program			= CreateProgram(glsl_vs, glsl_fs);
		}
		else
		{ // compute
			std::string glsl_cs = BuildShader();
			m_program			= CreateProgramCS(glsl_cs);
		}
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;
		glUseProgram(m_program);

		glGenBuffers(kBufs, m_storage_buffer);
		int sizes[kBufs] = { 7, 5, 3, 4, 23, 123, 419, 8 };

		int columns[4][kBufs] = { { 1, 1, 1, 1, 1, 1, 1, 1 },   // vector: 1 col
								  { 2, 3, 4, 4, 2, 3, 4, 1 },   // mat: # of cols
								  { 2, 3, 4, 4, 3, 2, 2, 1 },   // RM mat: # of rows
								  { 1, 1, 1, 1, 1, 1, 1, 1 } }; // structure: not used

		int scalars[4][kBufs] = { { 4, 4, 4, 4, 2, 2, 4, 1 },   //vector: size
								  { 2, 4, 4, 4, 4, 2, 2, 1 },   //matrix column_major: rows
								  { 2, 4, 4, 4, 2, 4, 4, 1 },   //matrix row_major: columns
								  { 1, 1, 1, 1, 1, 1, 1, 1 } }; //structure: not used

		int mindw[4][kBufs] = { { 3, 4, 3, 3, 2, 2, 4, 1 }, // # of real 32bit items
								{ 4, 9, 16, 16, 6, 6, 8, 1 },
								{ 4, 9, 16, 16, 6, 6, 8, 1 },
								{ 4, 11, 35, 35, 81, 127, 381, 1 } };
		int std430struct[kBufs] = { 4, 16, 48, 48, 88, 68, 264, 1 };
		int std140struct[kBufs] = { 4, 16, 60, 60, 144, 80, 432, 1 };
		int bufsize[kBufs][2]   = { { 0 } };

		std::vector<ivec4> data(kSize, ivec4(41));
		for (int i = 0; i < kBufs; ++i)
		{
			if (layout == std430)
			{
				bufsize[i][1] = 4 * columns[etype][i] * scalars[etype][i];
				if (etype == structure)
				{
					bufsize[i][1] = 4 * std430struct[i];
				}
			}
			else if (layout == std140)
			{
				bufsize[i][1] = 4 * columns[etype][i] * 4;
				if (etype == structure)
				{
					bufsize[i][1] = 4 * std140struct[i];
				}
			}
			else
			{
				bufsize[i][1] = 4 * mindw[etype][i];
			}
			bufsize[i][0] = sizes[i] * bufsize[i][1];
			if (i == kBufs - 1 || bind_seq == bindbasebefore)
			{ // never trim feedback storage
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, bufsize[i][0], &data[0], GL_DYNAMIC_COPY);
			}
			else
			{
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[i]);
				glBufferData(GL_SHADER_STORAGE_BUFFER, bufsize[i][0], &data[0], GL_DYNAMIC_COPY);
				if (bind_seq == bindbaseafter)
				{
					glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i]);
				}
				else if (bind_seq == bindrangeoffset && 2 * bufsize[i][1] >= minA)
				{
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i], 2 * bufsize[i][1],
									  bufsize[i][0] - 2 * bufsize[i][1]); // without 2 elements
				}
				else
				{ // bind_seq == bindrangesize || 2*bufsize[i][1] < minA
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, m_storage_buffer[i], 0,
									  bufsize[i][0] - 2 * bufsize[i][1]); // without 2 elements
				}
			}
		}

		if (stage != compute)
		{
			glGenVertexArrays(1, &m_vertex_array);
			glBindVertexArray(m_vertex_array);
			glDrawArrays(GL_POINTS, 0, 1);
		}
		else
		{
			glDispatchCompute(1, 1, 1);
		}
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[kBufs - 1]);
		int* dataout = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * sizes[kBufs - 1], GL_MAP_READ_BIT);
		if (!dataout)
			return ERROR;
		bool status = true;
		for (int i = 0; i < kBufs - 1; ++i)
		{
			if (other_members)
				sizes[i] -= 1; // space consumed by a pad
			if (bind_seq == bindrangesize || bind_seq == bindrangeoffset)
				sizes[i] -= 2; // space constrained by offset of range size
			if ((layout == std140 || layout == std430) && dataout[i] != sizes[i])
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Array " << i << " length is " << dataout[i] << " should be "
					<< sizes[i] << tcu::TestLog::EndMessage;
				status = false;
			}
			if ((layout == packed || layout == shared) && (dataout[i] > sizes[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Array " << i << " length is " << dataout[i]
					<< " should be not greater that " << sizes[i] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		if (length_as_index)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[4]);
			dataout = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufsize[4][0], GL_MAP_READ_BIT);
			if (!dataout)
				return ERROR;
			int i = (sizes[4] - 2) * columns[etype][4] * scalars[etype][4];
			if (dataout[i] != 82)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 4 index " << i << " is "
													<< dataout[i] << " should be 82." << tcu::TestLog::EndMessage;
				status = false;
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_storage_buffer[6]);
			dataout = (int*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufsize[6][0], GL_MAP_READ_BIT);
			if (!dataout)
				return ERROR;
			i = (sizes[6] - 2) * columns[etype][6] * scalars[etype][6];
			if (dataout[i] != 82)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Array 6 index " << i << " is "
													<< dataout[i] << " should be 82." << tcu::TestLog::EndMessage;
				status = false;
			}
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}

		if (!status)
			return ERROR;

		return NO_ERROR;
	}

public:
	int  stage;
	int  etype;
	int  layout;
	bool other_members;
	int  bind_seq;
	bool length_as_index;

	AdvancedUnsizedArrayLength2()
		: m_program(0)
		, m_vertex_array(0)
		, stage(compute)
		, etype(vector)
		, layout(std430)
		, other_members(false)
		, bind_seq(bindbasebefore)
		, length_as_index(false)
	{
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec_indexing : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		length_as_index = true;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec_after : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		bind_seq = bindbaseafter;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec_offset : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		bind_seq = bindrangeoffset;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec_size : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		bind_seq = bindrangesize;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype = vector;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_matC : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype = matrix_cm;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_matR : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype = matrix_rm;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_struct : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype = structure;
	}
};

class AdvancedUnsizedArrayLength_cs_std140_vec : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = compute;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_cs_std140_matC : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = matrix_cm;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_cs_std140_matR : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = matrix_rm;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_cs_std140_struct : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = structure;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_cs_packed_vec : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = vector;
		layout = packed;
	}
};

class AdvancedUnsizedArrayLength_cs_packed_matC : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = matrix_cm;
		layout = packed;
	}
};

class AdvancedUnsizedArrayLength_cs_shared_matR : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype  = matrix_rm;
		layout = shared;
	}
};

class AdvancedUnsizedArrayLength_fs_std430_vec : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = fragment;
		etype  = vector;
		layout = std430;
	}
};

class AdvancedUnsizedArrayLength_fs_std430_matC_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage		  = fragment;
		etype		  = matrix_cm;
		layout		  = std430;
		other_members = true;
	}
};

class AdvancedUnsizedArrayLength_fs_std140_matR : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = fragment;
		etype  = matrix_rm;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_fs_std140_struct : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = fragment;
		etype  = structure;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_vs_std430_vec_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage		  = vertex;
		etype		  = vector;
		layout		  = std430;
		other_members = true;
	}
};

class AdvancedUnsizedArrayLength_vs_std140_matC : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = vertex;
		etype  = matrix_cm;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_vs_packed_matR : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = vertex;
		etype  = matrix_rm;
		layout = packed;
	}
};

class AdvancedUnsizedArrayLength_vs_std140_struct : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		stage  = vertex;
		etype  = structure;
		layout = std140;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_vec_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype		  = vector;
		other_members = true;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_matC_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype		  = matrix_cm;
		other_members = true;
	}
};

class AdvancedUnsizedArrayLength_cs_std140_matR_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype		  = matrix_rm;
		layout		  = std140;
		other_members = true;
	}
};

class AdvancedUnsizedArrayLength_cs_std430_struct_pad : public AdvancedUnsizedArrayLength2
{
public:
	virtual void SetPath()
	{
		etype		  = structure;
		other_members = true;
	}
};

//-----------------------------------------------------------------------------
// 2.8 AdvancedMatrix
//-----------------------------------------------------------------------------

class AdvancedMatrix : public ShaderStorageBufferObjectBase
{
	GLuint m_program;
	GLuint m_storage_buffer[3];
	GLuint m_vertex_array;
	GLuint m_vertex_buffer;
	GLuint m_fbo, m_rt;

	virtual long Setup()
	{
		m_program = 0;
		memset(m_storage_buffer, 0, sizeof(m_storage_buffer));
		m_vertex_array  = 0;
		m_vertex_buffer = 0;
		glGenFramebuffers(1, &m_fbo);
		glGenTextures(1, &m_rt);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 g_position;" NL "out StageData {" NL "  flat int instance_id;" NL
			   "} vs_out;" NL "layout(binding = 0, std430) coherent buffer Buffer0 {" NL "  mat3x4 g_transform[4];" NL
			   "  mat4x3 g_color;" NL "  mat3 g_data0;" NL "};" NL
			   "layout(binding = 1, std430) readonly buffer Buffer1 {" NL "  mat4 color;" NL "} g_buffer1;" NL
			   "uniform int g_index1 = 1;" NL "uniform int g_index2 = 2;" NL "void main() {" NL
			   "  gl_Position = vec4(transpose(g_transform[gl_InstanceID]) * g_position, 1);" NL
			   "  g_color[gl_InstanceID] = g_buffer1.color[gl_InstanceID].rgb;" NL
			   "  if (gl_VertexID == 0 && gl_InstanceID == 0) {" NL "    g_data0[1][1] = 1.0;" NL
			   "    g_data0[g_index1][g_index2] += 3.0;" NL "  }" NL "  memoryBarrier();" NL
			   "  vs_out.instance_id = gl_InstanceID;" NL "}";

		const char* const glsl_fs =
			NL "in StageData {" NL "  flat int instance_id;" NL "} fs_in;" NL
			   "layout(location = 0) out vec4 g_ocolor;" NL "layout(binding = 0, std430) coherent buffer Buffer0 {" NL
			   "  mat3x4 g_transform[4];" NL "  mat4x3 g_color;" NL "  mat3 g_data0;" NL "};" NL
			   "uniform int g_index1 = 1;" NL "uniform int g_index2 = 2;" NL "void main() {" NL
			   "  if (g_data0[g_index1][g_index1] != 1.0) g_ocolor = vec4(0);" NL
			   "  else if (g_data0[g_index1][g_index2] != 3.0) g_ocolor = vec4(0);" NL
			   "  else g_ocolor = vec4(g_color[fs_in.instance_id], 1);" NL "}";

		m_program = CreateProgram(glsl_vs, glsl_fs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(3, m_storage_buffer);

		/* transform buffer */
		{
			float data[48 + 16 + 12 + 16] = {
				1.0f, 0.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f,
				0.0f, 1.0f, 0.0f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f,
				0.0f, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
			};
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer[0]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* transform buffer */
		{
			float data[16] = {
				1.0f, 0.0f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.5f,
			};
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_storage_buffer[1]);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		/* vertex buffer */
		{
			float data[8] = { -0.4f, -0.4f, 0.4f, -0.4f, -0.4f, 0.4f, 0.4f, 0.4f };
			glGenBuffers(1, &m_vertex_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		glBindTexture(GL_TEXTURE_2D, m_rt);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 100, 100);
		glBindTexture(GL_TEXTURE_2D, 0);
		glViewport(0, 0, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rt, 0);

		glGenVertexArrays(1, &m_vertex_array);
		glBindVertexArray(m_vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 4);

		if (!ValidateWindow4Quads(vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), vec3(1, 1, 0)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(3, m_storage_buffer);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteFramebuffers(1, &m_fbo);
		glDeleteTextures(1, &m_rt);
		return NO_ERROR;
	}
};

class AdvancedMatrixCS : public ShaderStorageBufferObjectBase
{
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
		bool			  status = true;
		const char* const glsl_cs =
			NL "layout(local_size_x = 1) in;" NL "layout(std430) buffer Buffer {" NL "  mat4x3 dst4x3;" NL
			   "  mat4 dst4;" NL "  mat4 src4;" NL "} b;" NL "uniform int g_index1;" NL "uniform int g_index2;" NL
			   "void main() {" NL "  b.dst4x3[gl_LocalInvocationIndex] = b.src4[gl_LocalInvocationIndex].rgb;" NL
			   "  b.dst4x3[gl_LocalInvocationIndex + 1u] = b.src4[gl_LocalInvocationIndex + 1u].aar;" NL
			   "  b.dst4[g_index2][g_index1] = 17.0;" NL "  b.dst4[g_index2][g_index1] += 6.0;" NL
			   "  b.dst4[3][0] = b.src4[3][0] != 44.0 ? 3.0 : 7.0;" NL "  b.dst4[3][1] = b.src4[3][1];" NL "}";
		m_program = CreateProgramCS(glsl_cs);
		glLinkProgram(m_program);
		if (!CheckProgram(m_program))
			return ERROR;

		glGenBuffers(1, &m_storage_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_storage_buffer);
		GLfloat data[16 + 16 + 16];
		for (int i  = 0; i < 32; ++i)
			data[i] = 0.0f;
		for (int i  = 32; i < 48; ++i)
			data[i] = (GLfloat)i;
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(data), &data[0], GL_DYNAMIC_DRAW);

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_index1"), 1);
		glUniform1i(glGetUniformLocation(m_program, "g_index2"), 2);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLfloat* out_data = (GLfloat*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(data), GL_MAP_READ_BIT);
		if (!out_data)
			return ERROR;

		GLfloat expected[32] = { 32.0f, 33.0f, 34.0f, 0.0f, 39.0f, 39.0f, 36.0f, 0.0f,
								 0.0f,  0.0f,  0.0f,  0.0f, 0.0f,  0.0f,  0.0f,  0.0f,

								 0.0f,  0.0f,  0.0f,  0.0f, 0.0f,  0.0f,  0.0f,  0.0f,
								 0.0f,  23.0f, 0.0f,  0.0f, 7.0f,  45.0f, 0.0f,  0.0f };
		for (int i = 0; i < 32; ++i)
		{
			if (out_data[i] != expected[i])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Received: " << out_data[i]
													<< ", but expected: " << expected[i] << tcu::TestLog::EndMessage;
				status = false;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_storage_buffer);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 4.1.1 NegativeAPIBind
//-----------------------------------------------------------------------------

class NegativeAPIBind : public ShaderStorageBufferObjectBase
{
	virtual long Run()
	{
		GLint bindings;
		GLint alignment;
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);
		glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE is generated by BindBufferBase if <target> is\n"
				   "SHADER_STORAGE_BUFFER and <index> is greater than or equal to the value of\n"
				   "MAX_SHADER_STORAGE_BUFFER_BINDINGS."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindings, 0, 0, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "INVALID_VALUE is generated by BindBufferRange if <target> is\n"
				   "SHADER_STORAGE_BUFFER and <index> is greater than or equal to the value of\n"
				   "MAX_SHADER_STORAGE_BUFFER_BINDINGS."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 0, alignment - 1, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE is generated by BindBufferRange if <target> is\n"
											"SHADER_STORAGE_BUFFER and <offset> is not a multiple of the value of\n"
											"SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 4.1.2 NegativeAPIBlockBinding
//-----------------------------------------------------------------------------

class NegativeAPIBlockBinding : public ShaderStorageBufferObjectBase
{
	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		GLint bindings;
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);

		const char* const glsl_vs =
			"#version 430 core" NL "buffer Buffer {" NL "  int x;" NL "};" NL "void main() {" NL "  x = 0;" NL "}";
		const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(sh, 1, &glsl_vs, NULL);
		glCompileShader(sh);

		const GLuint p = glCreateProgram();
		glAttachShader(p, sh);
		glDeleteShader(sh);
		glLinkProgram(p);

		glShaderStorageBlockBinding(p, 0, bindings);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "An INVALID_VALUE error is generated if storageBlockBinding is\n"
											"greater than or equal to the value of MAX_SHADER_STORAGE_BUFFER_BINDINGS."
				<< tcu::TestLog::EndMessage;
			glDeleteProgram(p);
			return ERROR;
		}

		glShaderStorageBlockBinding(p, 1, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "An INVALID_VALUE error is generated if storageBlockIndex is not an\n"
											"active shader storage block index in program."
				<< tcu::TestLog::EndMessage;
			glDeleteProgram(p);
			return ERROR;
		}

		glShaderStorageBlockBinding(0, 0, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "An INVALID_VALUE error is generated if program is not the name of "
											"either a program or shader object."
				<< tcu::TestLog::EndMessage;
			glDeleteProgram(p);
			return ERROR;
		}

		glShaderStorageBlockBinding(sh, 0, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "An INVALID_OPERATION error is generated if program is the name of a shader object."
				<< tcu::TestLog::EndMessage;
			glDeleteProgram(p);
			return ERROR;
		}

		glDeleteProgram(p);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 4.2.1 NegativeGLSLCompileTime
//-----------------------------------------------------------------------------

class NegativeGLSLCompileTime : public ShaderStorageBufferObjectBase
{
	static std::string Shader1(int binding)
	{
		std::stringstream ss;
		ss << NL "layout(binding = " << binding
		   << ") buffer Buffer {" NL "  int x;" NL "};" NL "void main() {" NL "  x = 0;" NL "}";
		return ss.str();
	}

	static std::string Shader2(int binding)
	{
		std::stringstream ss;
		ss << NL "layout(binding = " << binding
		   << ") buffer Buffer {" NL "  int x;" NL "} g_array[4];" NL "void main() {" NL "  g_array[0].x = 0;" NL
			  "  g_array[1].x = 0;" NL "  g_array[2].x = 0;" NL "  g_array[3].x = 0;" NL "}";
		return ss.str();
	}

	virtual long Run()
	{
		GLint bindings;
		glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &bindings);

		//  initialization of buffer block member 'x' not allowed
		if (!Compile(NL "buffer Buffer { int x = 10; };" NL "void main() {" NL "  x = 0;" NL "}"))
			return ERROR;

		//  syntax error, unexpected '-', expecting integer constant or unsigned integer constant at token "-"
		if (!Compile(Shader1(-1)))
			return ERROR;
		//  invalid value 96 for layout specifier 'binding'
		if (!Compile(Shader1(bindings)))
			return ERROR;

		//  invalid value 98 for layout specifier 'binding'
		if (!Compile(Shader2(bindings - 2)))
			return ERROR;

		//  OpenGL does not allow declaring buffer variable 'x' in the global scope. Use buffer blocks instead
		if (!Compile(NL "buffer int x;" NL "void main() {" NL "  x = 0;" NL "}"))
			return ERROR;

		// OpenGL requires buffer variables to be declared in a shader storage block in the global scope
		if (!Compile(NL "buffer Buffer { int y; };" NL "void main() {" NL "  y = 0;" NL "  buffer int x = 0;" NL "}"))
			return ERROR;

		//  OpenGL does not allow a parameter to be a buffer
		if (!Compile(NL "buffer Buffer { int y; };" NL "void Modify(buffer int a) {" NL "  atomicAdd(a, 1);" NL "}" NL
						"void main() {" NL "  Modify(y);" NL "}"))
			return ERROR;

		//  layout specifier 'std430', incompatible with 'uniform blocks'
		if (!Compile(NL "layout(std430) uniform UBO { int x; };" NL "buffer SSBO { int y; };" NL "void main() {" NL
						"  y = x;" NL "}"))
			return ERROR;

		//  unknown layout specifier 'std430'
		if (!Compile(NL "buffer SSBO {" NL "  layout(std430) int x;" NL "};" NL "void main() {" NL "  x = 0;" NL "}"))
			return ERROR;

		//  unknown layout specifier 'binding = 1'
		if (!Compile(NL "buffer SSBO {" NL "  layout(binding = 1) int x;" NL "};" NL "void main() {" NL "  x = 0;" NL
						"}"))
			return ERROR;

		//  OpenGL does not allow writing to readonly variable 'x'
		if (!Compile(NL "readonly buffer SSBO {" NL "  int x;" NL "};" NL "void main() {" NL "  x = 0;" NL "}"))
			return ERROR;

		//  OpenGL does not allow reading writeonly variable 'y'
		if (!Compile(NL "buffer SSBO {" NL "  int x;" NL "};" NL "writeonly buffer SSBO2 {" NL "  int y;" NL "};" NL
						"void main() {" NL "  x = y;" NL "}"))
			return ERROR;

		//  OpenGL does not allow writing to readonly variable 'z'
		if (!Compile(NL "buffer SSBO {" NL "  int x;" NL "};" NL "buffer SSBO2 {" NL "  writeonly int y;" NL
						"  readonly int z;" NL "};" NL "void main() {" NL "  x = y;" NL "  z = 0;" NL "}"))
			return ERROR;

		//  OpenGL does not allow having both readonly and writeonly qualifiers on a variable
		if (!Compile(NL "buffer SSBO {" NL "  int x;" NL "};" NL "readonly buffer SSBO2 {" NL "  writeonly int y;" NL
						"};" NL "void main() {" NL "  x = y;" NL "}"))
			return ERROR;

		// ["layout(binding = 1) buffer;" should cause compile-time error
		if (!Compile(NL "layout(binding = 1) buffer;" NL "buffer SSBO {" NL "  int x;" NL "};" NL "void main() {" NL
						"  x = 0;" NL "}"))
			return ERROR;

		// ["  atomicAdd(y, 2);"  should cause compile-time error
		if (!Compile(NL "buffer Buffer { int x; };" NL "int y;" NL "void main() {" NL "  atomicAdd(x, 1);" NL
						"  atomicAdd(y, 2);" //
					 NL "}"))
			return ERROR;

		if (!Compile( // can't construct vector from an array
				NL "buffer b {" NL "  vec4 x[10];" NL "};" NL "void main() {" NL "  vec4 y = vec4(x);" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Compile(const std::string& source)
	{
		const char* const csVer  = "#version 430 core";
		const char* const src[2] = { csVer, source.c_str() };
		const GLuint	  sh	 = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(sh, 2, src, NULL);
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

//-----------------------------------------------------------------------------
// 4.2.2 NegativeGLSLLinkTime
//-----------------------------------------------------------------------------
class NegativeGLSLLinkTime : public ShaderStorageBufferObjectBase
{
	virtual long Run()
	{
		//   declaration of "x" conflicts with previous declaration at 0(4)
		//   declaration of "Buffer" conflicts with previous declaration at 0(4)
		if (!Link("#version 430 core" NL "buffer Buffer { int x; };" NL "void Run();" NL "void main() {" NL
				  "  Run();" NL "  x += 2;" NL "}",
				  "#version 430 core" NL "buffer Buffer { uint x; };" NL "void Run() {" NL "  x += 3;" NL "}"))
			return ERROR;

		//  declaration of "Buffer" conflicts with previous declaration at 0(4)
		if (!Link("#version 430 core" NL "buffer Buffer { int x; int y; };" NL "void Run();" NL "void main() {" NL
				  "  Run();" NL "  x += 2;" NL "}",
				  "#version 430 core" NL "buffer Buffer { int x; };" NL "void Run() {" NL "  x += 3;" NL "}"))
			return ERROR;

		//  declaration of "Buffer" conflicts with previous declaration at 0(4)
		if (!Link("#version 430 core" NL "buffer Buffer { int y; };" NL "void Run();" NL "void main() {" NL
				  "  Run();" NL "  y += 2;" NL "}",
				  "#version 430 core" NL "buffer Buffer { int x; };" NL "void Run() {" NL "  x += 3;" NL "}"))
			return ERROR;

		//  declaration of "g_buffer" conflicts with previous declaration at 0(4)
		//  declaration of "Buffer" conflicts with previous declaration at 0(4)
		if (!Link("#version 430 core" NL "buffer Buffer { int x; } g_buffer[2];" NL "void Run();" NL "void main() {" NL
				  "  Run();" NL "  g_buffer[0].x += 2;" NL "}",
				  "#version 430 core" NL "buffer Buffer { int x; } g_buffer[3];" NL "void Run() {" NL
				  "  g_buffer[1].x += 3;" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Link(const std::string& cs0, const std::string& cs1)
	{
		const GLuint p = glCreateProgram();

		/* shader 0 */
		{
			GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
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
					<< tcu::TestLog::Message << "VS0 compilation should be ok." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		/* shader 1 */
		{
			GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
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
					<< tcu::TestLog::Message << "VS1 compilation should be ok." << tcu::TestLog::EndMessage;
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

} // anonymous namespace

ShaderStorageBufferObjectTests::ShaderStorageBufferObjectTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_storage_buffer_object", "")
{
}

ShaderStorageBufferObjectTests::~ShaderStorageBufferObjectTests(void)
{
}

void ShaderStorageBufferObjectTests::init()
{
	using namespace deqp;

	addChild(new TestSubcase(m_context, "basic-basic", TestSubcase::Create<BasicBasic>));
	addChild(new TestSubcase(m_context, "basic-basic-cs", TestSubcase::Create<BasicBasicCS>));
	addChild(new TestSubcase(m_context, "basic-max", TestSubcase::Create<BasicMax>));
	addChild(new TestSubcase(m_context, "basic-binding", TestSubcase::Create<BasicBinding>));
	addChild(new TestSubcase(m_context, "basic-syntax", TestSubcase::Create<BasicSyntax>));
	addChild(new TestSubcase(m_context, "basic-syntaxSSO", TestSubcase::Create<BasicSyntaxSSO>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case1-vs", TestSubcase::Create<BasicStd430LayoutCase1VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case1-cs", TestSubcase::Create<BasicStd430LayoutCase1CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case2-vs", TestSubcase::Create<BasicStd430LayoutCase2VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case2-cs", TestSubcase::Create<BasicStd430LayoutCase2CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case3-vs", TestSubcase::Create<BasicStd430LayoutCase3VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case3-cs", TestSubcase::Create<BasicStd430LayoutCase3CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case4-vs", TestSubcase::Create<BasicStd430LayoutCase4VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case4-cs", TestSubcase::Create<BasicStd430LayoutCase4CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case5-vs", TestSubcase::Create<BasicStd430LayoutCase5VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case5-cs", TestSubcase::Create<BasicStd430LayoutCase5CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case6-vs", TestSubcase::Create<BasicStd430LayoutCase6VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case6-cs", TestSubcase::Create<BasicStd430LayoutCase6CS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case7-vs", TestSubcase::Create<BasicStd430LayoutCase7VS>));
	addChild(new TestSubcase(m_context, "basic-std430Layout-case7-cs", TestSubcase::Create<BasicStd430LayoutCase7CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case1-vs", TestSubcase::Create<BasicStd140LayoutCase1VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case1-cs", TestSubcase::Create<BasicStd140LayoutCase1CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case2-vs", TestSubcase::Create<BasicStd140LayoutCase2VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case2-cs", TestSubcase::Create<BasicStd140LayoutCase2CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case3-vs", TestSubcase::Create<BasicStd140LayoutCase3VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case3-cs", TestSubcase::Create<BasicStd140LayoutCase3CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case4-vs", TestSubcase::Create<BasicStd140LayoutCase4VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case4-cs", TestSubcase::Create<BasicStd140LayoutCase4CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case5-vs", TestSubcase::Create<BasicStd140LayoutCase5VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case5-cs", TestSubcase::Create<BasicStd140LayoutCase5CS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case6-vs", TestSubcase::Create<BasicStd140LayoutCase6VS>));
	addChild(new TestSubcase(m_context, "basic-std140Layout-case6-cs", TestSubcase::Create<BasicStd140LayoutCase6CS>));
	addChild(new TestSubcase(m_context, "basic-atomic-case1", TestSubcase::Create<BasicAtomicCase1>));
	addChild(new TestSubcase(m_context, "basic-atomic-case1-cs", TestSubcase::Create<BasicAtomicCase1CS>));
	addChild(new TestSubcase(m_context, "basic-atomic-case2", TestSubcase::Create<BasicAtomicCase2>));
	addChild(new TestSubcase(m_context, "basic-atomic-case3", TestSubcase::Create<BasicAtomicCase3>));
	addChild(new TestSubcase(m_context, "basic-atomic-case3-cs", TestSubcase::Create<BasicAtomicCase3CS>));
	addChild(new TestSubcase(m_context, "basic-atomic-case4", TestSubcase::Create<BasicAtomicCase4>));
	addChild(new TestSubcase(m_context, "basic-atomic-case4-cs", TestSubcase::Create<BasicAtomicCase4CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case1-vs", TestSubcase::Create<BasicStdLayoutCase1VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case1-cs", TestSubcase::Create<BasicStdLayoutCase1CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case2-vs", TestSubcase::Create<BasicStdLayoutCase2VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case2-cs", TestSubcase::Create<BasicStdLayoutCase2CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case3-vs", TestSubcase::Create<BasicStdLayoutCase3VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case3-cs", TestSubcase::Create<BasicStdLayoutCase3CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case4-vs", TestSubcase::Create<BasicStdLayoutCase4VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout-case4-cs", TestSubcase::Create<BasicStdLayoutCase4CS>));
	addChild(new TestSubcase(m_context, "basic-operations-case1-vs", TestSubcase::Create<BasicOperationsCase1VS>));
	addChild(new TestSubcase(m_context, "basic-operations-case1-cs", TestSubcase::Create<BasicOperationsCase1CS>));
	addChild(new TestSubcase(m_context, "basic-operations-case2-vs", TestSubcase::Create<BasicOperationsCase2VS>));
	addChild(new TestSubcase(m_context, "basic-operations-case2-cs", TestSubcase::Create<BasicOperationsCase2CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout_UBO_SSBO-case1-vs",
							 TestSubcase::Create<Basic_UBO_SSBO_LayoutCase1VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout_UBO_SSBO-case1-cs",
							 TestSubcase::Create<Basic_UBO_SSBO_LayoutCase1CS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout_UBO_SSBO-case2-vs",
							 TestSubcase::Create<Basic_UBO_SSBO_LayoutCase2VS>));
	addChild(new TestSubcase(m_context, "basic-stdLayout_UBO_SSBO-case2-cs",
							 TestSubcase::Create<Basic_UBO_SSBO_LayoutCase2CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case1-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase1VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case1-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase1CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case2-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase2VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case2-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase2CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case3-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase3VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case3-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase3CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case4-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase4VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case4-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase4CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case5-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase5VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case5-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase5CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case6-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase6VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case6-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase6CS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case7-vs",
							 TestSubcase::Create<BasicMatrixOperationsCase7VS>));
	addChild(new TestSubcase(m_context, "basic-matrixOperations-case7-cs",
							 TestSubcase::Create<BasicMatrixOperationsCase7CS>));
	addChild(new TestSubcase(m_context, "basic-noBindingLayout", TestSubcase::Create<BasicNoBindingLayout>));
	addChild(new TestSubcase(m_context, "basic-readonly-writeonly", TestSubcase::Create<BasicReadonlyWriteonly>));
	addChild(new TestSubcase(m_context, "basic-name-match", TestSubcase::Create<BasicNameMatch>));
	addChild(new TestSubcase(m_context, "advanced-switchBuffers", TestSubcase::Create<AdvancedSwitchBuffers>));
	addChild(new TestSubcase(m_context, "advanced-switchBuffers-cs", TestSubcase::Create<AdvancedSwitchBuffersCS>));
	addChild(new TestSubcase(m_context, "advanced-switchPrograms", TestSubcase::Create<AdvancedSwitchPrograms>));
	addChild(new TestSubcase(m_context, "advanced-switchPrograms-cs", TestSubcase::Create<AdvancedSwitchProgramsCS>));
	addChild(new TestSubcase(m_context, "advanced-write-fragment", TestSubcase::Create<AdvancedWriteFragment>));
	addChild(new TestSubcase(m_context, "advanced-write-geometry", TestSubcase::Create<AdvancedWriteGeometry>));
	addChild(new TestSubcase(m_context, "advanced-write-tessellation", TestSubcase::Create<AdvancedWriteTessellation>));
	addChild(new TestSubcase(m_context, "advanced-indirectAddressing-case1",
							 TestSubcase::Create<AdvancedIndirectAddressingCase1>));
	addChild(new TestSubcase(m_context, "advanced-indirectAddressing-case1-cs",
							 TestSubcase::Create<AdvancedIndirectAddressingCase1CS>));
	addChild(new TestSubcase(m_context, "advanced-indirectAddressing-case2",
							 TestSubcase::Create<AdvancedIndirectAddressingCase2>));
	addChild(new TestSubcase(m_context, "advanced-indirectAddressing-case2-cs",
							 TestSubcase::Create<AdvancedIndirectAddressingCase2CS>));
	addChild(new TestSubcase(m_context, "advanced-readWrite-case1", TestSubcase::Create<AdvancedReadWriteCase1>));
	addChild(new TestSubcase(m_context, "advanced-readWrite-case1-cs", TestSubcase::Create<AdvancedReadWriteCase1CS>));
	addChild(new TestSubcase(m_context, "advanced-usage-case1", TestSubcase::Create<AdvancedUsageCase1>));
	addChild(new TestSubcase(m_context, "advanced-usage-sync", TestSubcase::Create<AdvancedUsageSync>));
	addChild(new TestSubcase(m_context, "advanced-usage-sync-cs", TestSubcase::Create<AdvancedUsageSyncCS>));
	addChild(new TestSubcase(m_context, "advanced-usage-operators", TestSubcase::Create<AdvancedUsageOperators>));
	addChild(new TestSubcase(m_context, "advanced-usage-operators-cs", TestSubcase::Create<AdvancedUsageOperatorsCS>));
	addChild(
		new TestSubcase(m_context, "advanced-unsizedArrayLength", TestSubcase::Create<AdvancedUnsizedArrayLength>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-matC",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_matC>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-matR",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_matR>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-struct",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_struct>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std140-vec",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std140_vec>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std140-matC",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std140_matC>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std140-matR",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std140_matR>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std140-struct",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std140_struct>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-packed-vec",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_packed_vec>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-packed-matC",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_packed_matC>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-shared-matR",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_shared_matR>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-fs-std430-vec",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_fs_std430_vec>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-fs-std430-matC-pad",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_fs_std430_matC_pad>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-fs-std140-matR",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_fs_std140_matR>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-fs-std140-struct",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_fs_std140_struct>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-vs-std430-vec",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_vs_std430_vec_pad>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-vs-std140-matC",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_vs_std140_matC>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-vs-packed-matR",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_vs_packed_matR>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-vs-std140-struct",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_vs_std140_struct>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec-pad",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec_pad>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-matC-pad",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_matC_pad>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std140-matR-pad",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std140_matR_pad>));
	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-struct-pad",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_struct_pad>));

	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec-bindrangeOffset",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec_offset>));

	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec-bindrangeSize",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec_size>));

	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec-bindbaseAfter",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec_after>));

	addChild(new TestSubcase(m_context, "advanced-unsizedArrayLength-cs-std430-vec-indexing",
							 TestSubcase::Create<AdvancedUnsizedArrayLength_cs_std430_vec_indexing>));

	addChild(new TestSubcase(m_context, "advanced-matrix", TestSubcase::Create<AdvancedMatrix>));
	addChild(new TestSubcase(m_context, "advanced-matrix-cs", TestSubcase::Create<AdvancedMatrixCS>));
	addChild(new TestSubcase(m_context, "negative-api-bind", TestSubcase::Create<NegativeAPIBind>));
	addChild(new TestSubcase(m_context, "negative-api-blockBinding", TestSubcase::Create<NegativeAPIBlockBinding>));
	addChild(new TestSubcase(m_context, "negative-glsl-compileTime", TestSubcase::Create<NegativeGLSLCompileTime>));
	addChild(new TestSubcase(m_context, "negative-glsl-linkTime", TestSubcase::Create<NegativeGLSLLinkTime>));
}

} // namespace gl4cts
