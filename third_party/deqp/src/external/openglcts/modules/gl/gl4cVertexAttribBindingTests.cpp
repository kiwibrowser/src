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

#include "gl4cVertexAttribBindingTests.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include <cstdarg>

#include <cmath>

namespace gl4cts
{

using namespace glw;
using tcu::Vec4;
using tcu::IVec4;
using tcu::UVec4;
using tcu::DVec4;
using tcu::Vec3;
using tcu::IVec3;
using tcu::DVec3;
using tcu::Vec2;
using tcu::IVec2;
using tcu::UVec2;
using tcu::Mat4;

namespace
{

class VertexAttribBindingBase : public deqp::SubcaseBase
{

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

public:
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

	inline bool ColorEqual(const Vec4& c0, const Vec4& c1, const Vec4& epsilon)
	{
		if (fabs(c0[0] - c1[0]) > epsilon[0])
			return false;
		if (fabs(c0[1] - c1[1]) > epsilon[1])
			return false;
		if (fabs(c0[2] - c1[2]) > epsilon[2])
			return false;
		if (fabs(c0[3] - c1[3]) > epsilon[3])
			return false;
		return true;
	}

	inline bool ColorEqual(const Vec3& c0, const Vec3& c1, const Vec4& epsilon)
	{
		if (fabs(c0[0] - c1[0]) > epsilon[0])
			return false;
		if (fabs(c0[1] - c1[1]) > epsilon[1])
			return false;
		if (fabs(c0[2] - c1[2]) > epsilon[2])
			return false;
		return true;
	}

	bool CheckRectColor(const std::vector<Vec3>& fb, int fb_w, int rx, int ry, int rw, int rh, const Vec3& expected)
	{

		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		Vec4					 g_color_eps  = Vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		for (int y = ry; y < ry + rh; ++y)
		{
			for (int x = rx; x < rx + rw; ++x)
			{
				const int idx = y * fb_w + x;
				if (!ColorEqual(fb[idx], expected, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Incorrect framebuffer color at pixel (" << x << " " << y
						<< "). Color is (" << fb[idx][0] << " " << fb[idx][1] << " " << fb[idx][2]
						<< "). Color should be (" << expected[0] << " " << expected[1] << " " << expected[2] << ")"
						<< tcu::TestLog::EndMessage;
					return false;
				}
			}
		}
		return true;
	}

	bool CheckRectColor(const std::vector<Vec4>& fb, int fb_w, int rx, int ry, int rw, int rh, const Vec4& expected)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		Vec4					 g_color_eps  = Vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		for (int y = ry; y < ry + rh; ++y)
		{
			for (int x = rx; x < rx + rw; ++x)
			{
				const int idx = y * fb_w + x;
				if (!ColorEqual(fb[idx], expected, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Incorrect framebuffer color at pixel (" << x << " " << y
						<< "). Color is (" << fb[idx][0] << " " << fb[idx][1] << " " << fb[idx][2] << " " << fb[idx][3]
						<< "). Color should be (" << expected[0] << " " << expected[1] << " " << expected[2] << " "
						<< expected[3] << ")" << tcu::TestLog::EndMessage;
					return false;
				}
			}
		}
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

	bool IsEqual(IVec4 a, IVec4 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
	}

	bool IsEqual(UVec4 a, UVec4 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
	}

	bool IsEqual(Vec2 a, Vec2 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]);
	}

	bool IsEqual(IVec2 a, IVec2 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]);
	}

	bool IsEqual(UVec2 a, UVec2 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]);
	}

	const Mat4 Translation(float tx, float ty, float tz)
	{
		float d[] = { 1.0f, 0.0f, 0.0f, tx, 0.0f, 1.0f, 0.0f, ty, 0.0f, 0.0f, 1.0f, tz, 0.0f, 0.0f, 0.0f, 1.0f };
		return Mat4(d);
	}
};

//=============================================================================
// 1.1 BasicUsage
//-----------------------------------------------------------------------------
class BasicUsage : public VertexAttribBindingBase
{
	GLuint m_vsp, m_fsp, m_ppo, m_vao, m_vbo;

	virtual long Setup()
	{
		m_vsp = m_fsp = 0;
		glGenProgramPipelines(1, &m_ppo);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteProgramPipelines(1, &m_ppo);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "layout(location = 0) in vec4 vs_in_position;" NL
			"layout(location = 1) in vec3 vs_in_color;" NL "out StageData {" NL "  vec3 color;" NL "} vs_out;" NL
			"out gl_PerVertex { vec4 gl_Position; };" NL "void main() {" NL "  gl_Position = vs_in_position;" NL
			"  vs_out.color = vs_in_color;" NL "}";
		const char* const glsl_fs = "#version 430 core" NL "in StageData {" NL "  vec3 color;" NL "} fs_in;" NL
									"layout(location = 0) out vec4 fs_out_color;" NL "void main() {" NL
									"  fs_out_color = vec4(fs_in.color, 1);" NL "}";
		m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
		if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
			return ERROR;

		glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);

		{
			const float data[] = {
				-1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 1.0f, -1.0f, 0.0f,  1.0f, 0.0f, -1.0f, 1.0f, 0.0f,  1.0f,
				0.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,  1.0f, -1.0f, 1.0f,
				1.0f,  0.0f,  -1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  1.0f,  1.0f, 1.0f, 1.0f,  0.0f,
			};
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);
		glBindVertexBuffer(0, m_vbo, 0, 20);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glBindProgramPipeline(m_ppo);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool			  status = true;
		std::vector<Vec3> fb(getWindowWidth() * getWindowHeight());
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(1, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		return NO_ERROR;
	}
};

//=============================================================================
// BasicInputBase
//-----------------------------------------------------------------------------
class BasicInputBase : public VertexAttribBindingBase
{
	GLuint m_po, m_xfbo;

protected:
	Vec4	expected_data[64];
	GLsizei instance_count;
	GLint   base_instance;

	virtual long Setup()
	{
		m_po = 0;
		glGenBuffers(1, &m_xfbo);
		for (int i			 = 0; i < 64; ++i)
			expected_data[i] = Vec4(0.0f);
		instance_count		 = 1;
		base_instance		 = -1;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_po);
		glDeleteBuffers(1, &m_xfbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs = "#version 430 core" NL "layout(location = 0) in vec4 vs_in_attrib[16];" NL
									"out StageData {" NL "  vec4 attrib[16];" NL "} vs_out;" NL "void main() {" NL
									"  for (int i = 0; i < vs_in_attrib.length(); ++i) {" NL
									"    vs_out.attrib[i] = vs_in_attrib[i];" NL "  }" NL "}";
		m_po = glCreateProgram();
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(sh, 1, &glsl_vs, NULL);
			glCompileShader(sh);
			glAttachShader(m_po, sh);
			glDeleteShader(sh);
		}
		{
			const GLchar* const v[16] = { "StageData.attrib[0]",  "StageData.attrib[1]",  "StageData.attrib[2]",
										  "StageData.attrib[3]",  "StageData.attrib[4]",  "StageData.attrib[5]",
										  "StageData.attrib[6]",  "StageData.attrib[7]",  "StageData.attrib[8]",
										  "StageData.attrib[9]",  "StageData.attrib[10]", "StageData.attrib[11]",
										  "StageData.attrib[12]", "StageData.attrib[13]", "StageData.attrib[14]",
										  "StageData.attrib[15]" };
			glTransformFeedbackVaryings(m_po, 16, v, GL_INTERLEAVED_ATTRIBS);
		}
		glLinkProgram(m_po);
		if (!CheckProgram(m_po))
			return ERROR;

		{
			std::vector<GLubyte> zero(sizeof(expected_data));
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbo);
			glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(expected_data), &zero[0], GL_DYNAMIC_DRAW);
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_po);
		glBeginTransformFeedback(GL_POINTS);
		if (base_instance != -1)
		{
			glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 2, instance_count, static_cast<GLuint>(base_instance));
		}
		else
		{
			glDrawArraysInstanced(GL_POINTS, 0, 2, instance_count);
		}
		glEndTransformFeedback();

		Vec4 data[64];
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(Vec4) * 64, &data[0]);

		long status = NO_ERROR;
		for (int i = 0; i < 64; ++i)
		{
			if (!ColorEqual(expected_data[i], data[i], Vec4(0.01f)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is: " << data[i][0] << " " << data[i][1] << " " << data[i][2]
					<< " " << data[i][3] << ", data should be: " << expected_data[i][0] << " " << expected_data[i][1]
					<< " " << expected_data[i][2] << " " << expected_data[i][3] << ", index is: " << i
					<< tcu::TestLog::EndMessage;
				status = ERROR;
				break;
			}
		}
		return status;
	}
};

//=============================================================================
// 1.2.1 BasicInputCase1
//-----------------------------------------------------------------------------
class BasicInputCase1 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glBindVertexBuffer(0, m_vbo, 0, 12);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribBinding(1, 0);
		glEnableVertexAttribArray(1);

		expected_data[1]  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[17] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.2 BasicInputCase2
//-----------------------------------------------------------------------------
class BasicInputCase2 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribBinding(1, 0);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(7, 1, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribFormat(15, 2, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(7, 0);
		glVertexAttribBinding(15, 0);
		glBindVertexBuffer(0, m_vbo, 0, 12);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(7);
		glEnableVertexAttribArray(15);

		expected_data[0]  = Vec4(1.0f, 2.0f, 0.0f, 1.0f);
		expected_data[1]  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[7]  = Vec4(3.0f, 0.0f, 0.0f, 1.0f);
		expected_data[15] = Vec4(2.0f, 3.0f, 0.0f, 1.0f);
		expected_data[16] = Vec4(4.0f, 5.0f, 0.0f, 1.0f);
		expected_data[17] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[23] = Vec4(6.0f, 0.0f, 0.0f, 1.0f);
		expected_data[31] = Vec4(5.0f, 6.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.3 BasicInputCase3
//-----------------------------------------------------------------------------
class BasicInputCase3 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, 36 * 2, NULL, GL_STATIC_DRAW);
		{
			GLubyte d[] = { 1, 2, 3, 4 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec3), &Vec3(5.0f, 6.0f, 7.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(Vec2), &Vec2(8.0f, 9.0f)[0]);
		{
			GLubyte d[] = { 10, 11, 12, 13 };
			glBufferSubData(GL_ARRAY_BUFFER, 0 + 36, sizeof(d), d);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 16 + 36, sizeof(Vec3), &Vec3(14.0f, 15.0f, 16.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 28 + 36, sizeof(Vec2), &Vec2(17.0f, 18.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glEnableVertexAttribArray(1);
		glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0);
		glVertexAttribBinding(1, 3);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 16);
		glVertexAttribBinding(2, 3);
		glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 28);
		glVertexAttribBinding(0, 3);
		glBindVertexBuffer(3, m_vbo, 0, 36);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);

		expected_data[0]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[1]	  = Vec4(5.0f, 6.0f, 7.0f, 1.0f);
		expected_data[2]	  = Vec4(8.0f, 9.0f, 0.0f, 1.0f);
		expected_data[0 + 16] = Vec4(10.0f, 11.0f, 12.0f, 13.0f);
		expected_data[1 + 16] = Vec4(14.0f, 15.0f, 16.0f, 1.0f);
		expected_data[2 + 16] = Vec4(17.0f, 18.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.4 BasicInputCase4
//-----------------------------------------------------------------------------
class BasicInputCase4 : public BasicInputBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 20 * 2, NULL, GL_STATIC_DRAW);
		{
			GLbyte d[] = { -127, 127, -127, 127 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		{
			GLushort d[] = { 1, 2, 3, 4 };
			glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
		}
		{
			GLuint d[] = { 5, 6 };
			glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
		}
		{
			GLbyte d[] = { 127, -127, 127, -127 };
			glBufferSubData(GL_ARRAY_BUFFER, 0 + 20, sizeof(d), d);
		}
		{
			GLushort d[] = { 7, 8, 9, 10 };
			glBufferSubData(GL_ARRAY_BUFFER, 4 + 20, sizeof(d), d);
		}
		{
			GLuint d[] = { 11, 12 };
			glBufferSubData(GL_ARRAY_BUFFER, 12 + 20, sizeof(d), d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 24 * 2 + 8, NULL, GL_STATIC_DRAW);
		{
			GLdouble d[] = { 0.0, 100.0, 200.0 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		{
			GLdouble d[] = { 300.0, 400.0 };
			glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 4, GL_BYTE, GL_TRUE, 0);
		glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_FALSE, 4);
		glVertexAttribFormat(2, 2, GL_UNSIGNED_INT, GL_FALSE, 12);
		glVertexAttribFormat(5, 2, GL_DOUBLE, GL_FALSE, 0);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);
		glVertexAttribBinding(2, 0);
		glVertexAttribBinding(5, 6);
		glBindVertexBuffer(0, m_vbo[0], 0, 20);
		glBindVertexBuffer(6, m_vbo[1], 8, 24);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(5);

		expected_data[0]	  = Vec4(-1.0f, 1.0f, -1.0f, 1.0f);
		expected_data[1]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[2]	  = Vec4(5.0f, 6.0f, 0.0f, 1.0f);
		expected_data[5]	  = Vec4(100.0f, 200.0f, 0.0f, 1.0f);
		expected_data[0 + 16] = Vec4(1.0f, -1.0f, 1.0f, -1.0f);
		expected_data[1 + 16] = Vec4(7.0f, 8.0f, 9.0f, 10.0f);
		expected_data[2 + 16] = Vec4(11.0f, 12.0f, 0.0f, 1.0f);
		expected_data[5 + 16] = Vec4(300.0f, 400.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.5 BasicInputCase5
//-----------------------------------------------------------------------------
class BasicInputCase5 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		const int kStride = 116;
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
		{
			GLubyte d[] = { 0, 0xff, 0xff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		{
			GLushort d[] = { 0, 0xffff, 0xffff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
		}
		{
			GLuint d[] = { 0, 0xffffffff, 0xffffffff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
		}
		{
			GLbyte d[] = { 0, -127, 127, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
		}
		{
			GLshort d[] = { 0, -32767, 32767, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
		}
		{
			GLint d[] = { 0, -2147483647, 2147483647, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
		}
		{
			GLfloat d[] = { 0, 1.0f, 2.0f, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
		}
		{
			GLdouble d[] = { 0, 10.0, 20.0, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
		}
		{
			GLubyte d[] = { 0, 0xff / 4, 0xff / 2, 0xff };
			glBufferSubData(GL_ARRAY_BUFFER, 104, sizeof(d), d);
		}
		{
			GLuint d = 0 | (1023 << 10) | (511 << 20) | (1 << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 108, sizeof(d), &d);
		}
		{
			GLint d = 0 | (511 << 10) | (255 << 20) | (0 << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 112, sizeof(d), &d);
		}

		{
			GLubyte d[] = { 0xff, 0xff, 0xff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
		}
		{
			GLushort d[] = { 0xffff, 0xffff, 0xffff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
		}
		{
			GLuint d[] = { 0xffffffff, 0xffffffff, 0xffffffff / 2, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
		}
		{
			GLbyte d[] = { 127, -127, 127, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
		}
		{
			GLshort d[] = { 32767, -32767, 32767, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
		}
		{
			GLint d[] = { 2147483647, -2147483647, 2147483647, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
		}
		{
			GLfloat d[] = { 0, 3.0f, 4.0f, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
		}
		{
			GLdouble d[] = { 0, 30.0, 40.0, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
		}
		{
			GLubyte d[] = { 0xff, 0xff / 2, 0xff / 4, 0 };
			glBufferSubData(GL_ARRAY_BUFFER, 104 + kStride, sizeof(d), d);
		}
		{
			GLuint d = 0 | (1023 << 10) | (511 << 20) | (2u << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 108 + kStride, sizeof(d), &d);
		}
		{
			GLint d = (-511 & 0x3ff) | (511 << 10) | (255 << 20) | 3 << 30;
			glBufferSubData(GL_ARRAY_BUFFER, 112 + kStride, sizeof(d), &d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0);
		glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_TRUE, 4);
		glVertexAttribFormat(2, 4, GL_UNSIGNED_INT, GL_TRUE, 12);
		glVertexAttribFormat(3, 4, GL_BYTE, GL_TRUE, 28);
		glVertexAttribFormat(4, 4, GL_SHORT, GL_TRUE, 32);
		glVertexAttribFormat(5, 4, GL_INT, GL_TRUE, 40);
		glVertexAttribFormat(6, 4, GL_FLOAT, GL_TRUE, 56);
		glVertexAttribFormat(7, 4, GL_DOUBLE, GL_TRUE, 72);
		glVertexAttribFormat(8, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, 104);
		glVertexAttribFormat(9, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 108);
		glVertexAttribFormat(10, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 108);
		glVertexAttribFormat(11, 4, GL_INT_2_10_10_10_REV, GL_TRUE, 112);
		glVertexAttribFormat(12, GL_BGRA, GL_INT_2_10_10_10_REV, GL_TRUE, 112);

		for (GLuint i = 0; i < 13; ++i)
		{
			glVertexAttribBinding(i, 0);
			glEnableVertexAttribArray(i);
		}
		glBindVertexBuffer(0, m_vbo, 0, kStride);

		expected_data[0]	   = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
		expected_data[1]	   = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
		expected_data[2]	   = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
		expected_data[3]	   = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
		expected_data[4]	   = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
		expected_data[5]	   = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
		expected_data[6]	   = Vec4(0.0f, 1.0f, 2.0f, 0.0f);
		expected_data[7]	   = Vec4(0.0f, 10.0f, 20.0f, 0.0f);
		expected_data[8]	   = Vec4(0.5f, 0.25f, 0.0f, 1.0f);
		expected_data[9]	   = Vec4(0.0f, 1.0f, 0.5f, 0.33f);
		expected_data[10]	  = Vec4(0.5f, 1.0f, 0.0f, 0.33f);
		expected_data[11]	  = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
		expected_data[12]	  = Vec4(0.5f, 1.0f, 0.0f, 0.0f);
		expected_data[0 + 16]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
		expected_data[1 + 16]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
		expected_data[2 + 16]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
		expected_data[3 + 16]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
		expected_data[4 + 16]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
		expected_data[5 + 16]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
		expected_data[6 + 16]  = Vec4(0.0f, 3.0f, 4.0f, 0.0f);
		expected_data[7 + 16]  = Vec4(0.0f, 30.0f, 40.0f, 0.0f);
		expected_data[8 + 16]  = Vec4(0.25f, 0.5f, 1.0f, 0.0f);
		expected_data[9 + 16]  = Vec4(0.0f, 1.0f, 0.5f, 0.66f);
		expected_data[10 + 16] = Vec4(0.5f, 1.0f, 0.0f, 0.66f);
		expected_data[11 + 16] = Vec4(-1.0f, 1.0f, 0.5f, -1.0f);
		expected_data[12 + 16] = Vec4(0.5f, 1.0f, -1.0f, -1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.6 BasicInputCase6
//-----------------------------------------------------------------------------
class BasicInputCase6 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		const int kStride = 112;
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
		{
			GLubyte d[] = { 1, 2, 3, 4 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		{
			GLushort d[] = { 5, 6, 7, 8 };
			glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
		}
		{
			GLuint d[] = { 9, 10, 11, 12 };
			glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
		}
		{
			GLbyte d[] = { -1, 2, -3, 4 };
			glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
		}
		{
			GLshort d[] = { -5, 6, -7, 8 };
			glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
		}
		{
			GLint d[] = { -9, 10, -11, 12 };
			glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
		}
		{
			GLfloat d[] = { -13.0f, 14.0f, -15.0f, 16.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
		}
		{
			GLdouble d[] = { -18.0, 19.0, -20.0, 21.0 };
			glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
		}
		{
			GLuint d = 0 | (11 << 10) | (12 << 20) | (2u << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 104, sizeof(d), &d);
		}
		{
			GLint d = 0 | ((0xFFFFFFF5 << 10) & (0x3ff << 10)) | (12 << 20) | (1 << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 108, sizeof(d), &d);
		}
		{
			GLubyte d[] = { 22, 23, 24, 25 };
			glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
		}
		{
			GLushort d[] = { 26, 27, 28, 29 };
			glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
		}
		{
			GLuint d[] = { 30, 31, 32, 33 };
			glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
		}
		{
			GLbyte d[] = { -34, 35, -36, 37 };
			glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
		}
		{
			GLshort d[] = { -38, 39, -40, 41 };
			glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
		}
		{
			GLint d[] = { -42, 43, -44, 45 };
			glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
		}
		{
			GLfloat d[] = { -46.0f, 47.0f, -48.0f, 49.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
		}
		{
			GLdouble d[] = { -50.0, 51.0, -52.0, 53.0 };
			glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
		}
		{
			GLuint d = 0 | (11 << 10) | (12 << 20) | (1 << 30);
			glBufferSubData(GL_ARRAY_BUFFER, 104 + kStride, sizeof(d), &d);
		}
		{
			GLint d = 123 | ((0xFFFFFFFD << 10) & (0x3ff << 10)) | ((0xFFFFFE0C << 20) & (0x3ff << 20)) |
					  ((0xFFFFFFFF << 30) & (0x3 << 30));
			glBufferSubData(GL_ARRAY_BUFFER, 108 + kStride, sizeof(d), &d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0);
		glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_FALSE, 4);
		glVertexAttribFormat(2, 4, GL_UNSIGNED_INT, GL_FALSE, 12);
		glVertexAttribFormat(3, 4, GL_BYTE, GL_FALSE, 28);
		glVertexAttribFormat(4, 4, GL_SHORT, GL_FALSE, 32);
		glVertexAttribFormat(5, 4, GL_INT, GL_FALSE, 40);
		glVertexAttribFormat(6, 4, GL_FLOAT, GL_FALSE, 56);
		glVertexAttribFormat(7, 4, GL_DOUBLE, GL_FALSE, 72);
		glVertexAttribFormat(8, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, 104);
		glVertexAttribFormat(9, 4, GL_INT_2_10_10_10_REV, GL_FALSE, 108);
		for (GLuint i = 0; i < 10; ++i)
		{
			glVertexAttribBinding(i, 0);
			glEnableVertexAttribArray(i);
		}
		glBindVertexBuffer(0, m_vbo, 0, kStride);

		expected_data[0]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[1]	  = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[2]	  = Vec4(9.0f, 10.0f, 11.0f, 12.0f);
		expected_data[3]	  = Vec4(-1.0f, 2.0f, -3.0f, 4.0f);
		expected_data[4]	  = Vec4(-5.0f, 6.0f, -7.0f, 8.0f);
		expected_data[5]	  = Vec4(-9.0f, 10.0f, -11.0f, 12.0f);
		expected_data[6]	  = Vec4(-13.0f, 14.0f, -15.0f, 16.0f);
		expected_data[7]	  = Vec4(-18.0f, 19.0f, -20.0f, 21.0f);
		expected_data[8]	  = Vec4(0.0f, 11.0f, 12.0f, 2.0f);
		expected_data[9]	  = Vec4(0.0f, -11.0f, 12.0f, 1.0f);
		expected_data[0 + 16] = Vec4(22.0f, 23.0f, 24.0f, 25.0f);
		expected_data[1 + 16] = Vec4(26.0f, 27.0f, 28.0f, 29.0f);
		expected_data[2 + 16] = Vec4(30.0f, 31.0f, 32.0f, 33.0f);
		expected_data[3 + 16] = Vec4(-34.0f, 35.0f, -36.0f, 37.0f);
		expected_data[4 + 16] = Vec4(-38.0f, 39.0f, -40.0f, 41.0f);
		expected_data[5 + 16] = Vec4(-42.0f, 43.0f, -44.0f, 45.0f);
		expected_data[6 + 16] = Vec4(-46.0f, 47.0f, -48.0f, 49.0f);
		expected_data[7 + 16] = Vec4(-50.0f, 51.0f, -52.0f, 53.0f);
		expected_data[8 + 16] = Vec4(0.0f, 11.0f, 12.0f, 1.0f);
		expected_data[9 + 16] = Vec4(123.0f, -3.0f, -500.0f, -1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.7 BasicInputCase7
//-----------------------------------------------------------------------------
class BasicInputCase7 : public BasicInputBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 6 * 4, NULL, GL_STATIC_DRAW);
		{
			GLfloat d[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 10 * 4, NULL, GL_STATIC_DRAW);
		{
			GLfloat d[] = { -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f, -10.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribFormat(5, 4, GL_FLOAT, GL_FALSE, 12);
		glVertexAttribFormat(14, 2, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 1);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(5, 15);
		glVertexAttribBinding(14, 7);
		glBindVertexBuffer(0, m_vbo[0], 0, 12);
		glBindVertexBuffer(1, m_vbo[0], 4, 4);
		glBindVertexBuffer(7, m_vbo[1], 8, 16);
		glBindVertexBuffer(15, m_vbo[1], 12, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(5);
		glEnableVertexAttribArray(14);

		base_instance		   = 0;
		expected_data[0]	   = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[1]	   = Vec4(2.0f, 3.0f, 4.0f, 1.0f);
		expected_data[2]	   = Vec4(3.0f, 0.0f, 0.0f, 1.0f);
		expected_data[5]	   = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
		expected_data[14]	  = Vec4(-5.0f, -6.0f, 0.0f, 1.0f);
		expected_data[0 + 16]  = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[1 + 16]  = Vec4(3.0f, 4.0f, 5.0f, 1.0f);
		expected_data[2 + 16]  = Vec4(4.0f, 0.0f, 0.0f, 1.0f);
		expected_data[5 + 16]  = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
		expected_data[14 + 16] = Vec4(-9.0f, -10.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.8 BasicInputCase8
//-----------------------------------------------------------------------------
class BasicInputCase8 : public BasicInputBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 6 * 4, NULL, GL_STATIC_DRAW);
		{
			GLfloat d[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 10 * 4, NULL, GL_STATIC_DRAW);
		{
			GLfloat d[] = { -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f, -10.0f };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribFormat(5, 4, GL_FLOAT, GL_FALSE, 12);
		glVertexAttribFormat(14, 2, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 1);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(5, 15);
		glVertexAttribBinding(14, 7);
		glBindVertexBuffer(0, m_vbo[0], 0, 12);
		glBindVertexBuffer(1, m_vbo[0], 4, 4);
		glBindVertexBuffer(7, m_vbo[1], 8, 16);
		glBindVertexBuffer(15, m_vbo[1], 12, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(5);
		glEnableVertexAttribArray(14);

		expected_data[0]	   = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[1]	   = Vec4(2.0f, 3.0f, 4.0f, 1.0f);
		expected_data[2]	   = Vec4(3.0f, 0.0f, 0.0f, 1.0f);
		expected_data[5]	   = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
		expected_data[14]	  = Vec4(-5.0f, -6.0f, 0.0f, 1.0f);
		expected_data[0 + 16]  = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[1 + 16]  = Vec4(3.0f, 4.0f, 5.0f, 1.0f);
		expected_data[2 + 16]  = Vec4(4.0f, 0.0f, 0.0f, 1.0f);
		expected_data[5 + 16]  = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
		expected_data[14 + 16] = Vec4(-9.0f, -10.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.9 BasicInputCase9
//-----------------------------------------------------------------------------
class BasicInputCase9 : public BasicInputBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(1.0f, 2.0f, 3.0f, 4.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(5.0f, 6.0f, 7.0f, 8.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(Vec4), &Vec4(9.0f, 10.0f, 11.0f, 12.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(10.0f, 20.0f, 30.0f, 40.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(50.0f, 60.0f, 70.0f, 80.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(4, 2, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(4, 3);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(4);
		glBindVertexBuffer(0, m_vbo[0], 0, 16);
		glBindVertexBuffer(1, m_vbo[0], 0, 16);
		glBindVertexBuffer(3, m_vbo[1], 4, 8);
		glVertexBindingDivisor(1, 1);

		instance_count		  = 2;
		base_instance		  = 0;
		expected_data[0]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[2]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[4]	  = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
		expected_data[0 + 16] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[2 + 16] = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[4 + 16] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);

		expected_data[0 + 32]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[2 + 32]	  = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[4 + 32]	  = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
		expected_data[0 + 16 + 32] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[2 + 16 + 32] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[4 + 16 + 32] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.10 BasicInputCase10
//-----------------------------------------------------------------------------
class BasicInputCase10 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 24, sizeof(Vec3), &Vec3(7.0f, 8.0f, 9.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(2, 3);
		glBindVertexBuffer(0, m_vbo, 0, 12);
		glBindVertexBuffer(3, m_vbo, 4, 8);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glVertexBindingDivisor(0, 1);
		glVertexBindingDivisor(3, 0);

		instance_count		  = 2;
		base_instance		  = 1;
		expected_data[0]	  = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[2]	  = Vec4(3.0f, 4.0f, 0.0f, 1.0f);
		expected_data[0 + 16] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[2 + 16] = Vec4(5.0f, 6.0f, 0.0f, 1.0f);

		expected_data[0 + 32]	  = Vec4(7.0f, 8.0f, 9.0f, 1.0f);
		expected_data[2 + 32]	  = Vec4(3.0f, 4.0f, 0.0f, 1.0f);
		expected_data[0 + 16 + 32] = Vec4(7.0f, 8.0f, 9.0f, 1.0f);
		expected_data[2 + 16 + 32] = Vec4(5.0f, 6.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.11 BasicInputCase11
//-----------------------------------------------------------------------------
class BasicInputCase11 : public BasicInputBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(1.0f, 2.0f, 3.0f, 4.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(5.0f, 6.0f, 7.0f, 8.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(Vec4), &Vec4(9.0f, 10.0f, 11.0f, 12.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(10.0f, 20.0f, 30.0f, 40.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(50.0f, 60.0f, 70.0f, 80.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(4, 2, GL_FLOAT, GL_FALSE, 4);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(4, 2);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(4);
		glBindVertexBuffer(0, m_vbo[0], 0, 16);
		glBindVertexBuffer(1, m_vbo[0], 0, 16);
		glBindVertexBuffer(2, m_vbo[1], 4, 8);
		glVertexBindingDivisor(1, 1);

		instance_count		  = 2;
		expected_data[0]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[2]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[4]	  = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
		expected_data[0 + 16] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[2 + 16] = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[4 + 16] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);

		expected_data[0 + 32]	  = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
		expected_data[2 + 32]	  = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[4 + 32]	  = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
		expected_data[0 + 16 + 32] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[2 + 16 + 32] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
		expected_data[4 + 16 + 32] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// 1.2.12 BasicInputCase12
//-----------------------------------------------------------------------------
class BasicInputCase12 : public BasicInputBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 16; ++i)
		{
			glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribBinding(0, 1);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		expected_data[0]	  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[1]	  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
		expected_data[0 + 16] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		expected_data[1 + 16] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
		return BasicInputBase::Run();
	}
};

//=============================================================================
// BasicInputIBase
//-----------------------------------------------------------------------------
class BasicInputIBase : public VertexAttribBindingBase
{
	GLuint m_po, m_xfbo;

protected:
	IVec4   expected_datai[32];
	UVec4   expected_dataui[32];
	GLsizei instance_count;
	GLuint  base_instance;

	virtual long Setup()
	{
		m_po = 0;
		glGenBuffers(1, &m_xfbo);
		for (int i = 0; i < 32; ++i)
		{
			expected_datai[i]  = IVec4(0);
			expected_dataui[i] = UVec4(0);
		}
		instance_count = 1;
		base_instance  = 0;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_po);
		glDeleteBuffers(1, &m_xfbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "layout(location = 0) in ivec4 vs_in_attribi[8];" NL
			"layout(location = 8) in uvec4 vs_in_attribui[8];" NL "out StageData {" NL "  ivec4 attribi[8];" NL
			"  uvec4 attribui[8];" NL "} vs_out;" NL "void main() {" NL
			"  for (int i = 0; i < vs_in_attribi.length(); ++i) {" NL "    vs_out.attribi[i] = vs_in_attribi[i];" NL
			"  }" NL "  for (int i = 0; i < vs_in_attribui.length(); ++i) {" NL
			"    vs_out.attribui[i] = vs_in_attribui[i];" NL "  }" NL "}";
		m_po = glCreateProgram();
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(sh, 1, &glsl_vs, NULL);
			glCompileShader(sh);
			glAttachShader(m_po, sh);
			glDeleteShader(sh);
		}
		{
			const GLchar* const v[16] = { "StageData.attribi[0]",  "StageData.attribi[1]",  "StageData.attribi[2]",
										  "StageData.attribi[3]",  "StageData.attribi[4]",  "StageData.attribi[5]",
										  "StageData.attribi[6]",  "StageData.attribi[7]",  "StageData.attribui[0]",
										  "StageData.attribui[1]", "StageData.attribui[2]", "StageData.attribui[3]",
										  "StageData.attribui[4]", "StageData.attribui[5]", "StageData.attribui[6]",
										  "StageData.attribui[7]" };
			glTransformFeedbackVaryings(m_po, 16, v, GL_INTERLEAVED_ATTRIBS);
		}
		glLinkProgram(m_po);
		if (!CheckProgram(m_po))
			return ERROR;

		{
			std::vector<GLubyte> zero(64 * 16);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbo);
			glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, (GLsizeiptr)zero.size(), &zero[0], GL_DYNAMIC_COPY);
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_po);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 2, instance_count, base_instance);
		glEndTransformFeedback();

		IVec4 datai[32];
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0 * 16 * 8, 16 * 8, &datai[0]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 2 * 16 * 8, 16 * 8, &datai[8]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 4 * 16 * 8, 16 * 8, &datai[16]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 6 * 16 * 8, 16 * 8, &datai[24]);
		UVec4 dataui[32];
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 1 * 16 * 8, 16 * 8, &dataui[0]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 3 * 16 * 8, 16 * 8, &dataui[8]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 5 * 16 * 8, 16 * 8, &dataui[16]);
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 7 * 16 * 8, 16 * 8, &dataui[24]);

		for (int i = 0; i < 32; ++i)
		{
			if (!IsEqual(expected_datai[i], datai[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Datai is: " << datai[i][0] << " " << datai[i][1] << " " << datai[i][2]
					<< " " << datai[i][3] << ", datai should be: " << expected_datai[i][0] << " "
					<< expected_datai[i][1] << " " << expected_datai[i][2] << " " << expected_datai[i][3]
					<< ", index is: " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
			if (!IsEqual(expected_dataui[i], dataui[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Dataui is: " << dataui[i][0] << " " << dataui[i][1] << " "
					<< dataui[i][2] << " " << dataui[i][3] << ", dataui should be: " << expected_dataui[i][0] << " "
					<< expected_dataui[i][1] << " " << expected_dataui[i][2] << " " << expected_dataui[i][3]
					<< ", index is: " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		return NO_ERROR;
	}
};

//=============================================================================
// 1.3.1 BasicInputICase1
//-----------------------------------------------------------------------------
class BasicInputICase1 : public BasicInputIBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputIBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputIBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			glVertexAttribI4i(i, 0, 0, 0, 0);
			glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
		}
		const int kStride = 88;
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
		{
			GLbyte d[] = { 1, -2, 3, -4 };
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
		}
		{
			GLshort d[] = { 5, -6, 7, -8 };
			glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
		}
		{
			GLint d[] = { 9, -10, 11, -12 };
			glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
		}
		{
			GLubyte d[] = { 13, 14, 15, 16 };
			glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
		}
		{
			GLushort d[] = { 17, 18, 19, 20 };
			glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
		}
		{
			GLuint d[] = { 21, 22, 23, 24 };
			glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
		}
		{
			GLint d[] = { 90, -91, 92, -93 };
			glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
		}
		{
			GLuint d[] = { 94, 95, 96, 97 };
			glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
		}

		{
			GLbyte d[] = { 25, -26, 27, -28 };
			glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
		}
		{
			GLshort d[] = { 29, -30, 31, -32 };
			glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
		}
		{
			GLint d[] = { 33, -34, 35, -36 };
			glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
		}
		{
			GLubyte d[] = { 37, 38, 39, 40 };
			glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
		}
		{
			GLushort d[] = { 41, 42, 43, 44 };
			glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
		}
		{
			GLuint d[] = { 45, 46, 47, 48 };
			glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
		}
		{
			GLint d[] = { 98, -99, 100, -101 };
			glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
		}
		{
			GLuint d[] = { 102, 103, 104, 105 };
			glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribIFormat(0, 1, GL_BYTE, 0);
		glVertexAttribIFormat(1, 2, GL_SHORT, 4);
		glVertexAttribIFormat(2, 3, GL_INT, 12);
		glVertexAttribIFormat(3, 4, GL_INT, 56);
		glVertexAttribIFormat(8, 3, GL_UNSIGNED_BYTE, 28);
		glVertexAttribIFormat(9, 2, GL_UNSIGNED_SHORT, 32);
		glVertexAttribIFormat(10, 1, GL_UNSIGNED_INT, 40);
		glVertexAttribIFormat(11, 4, GL_UNSIGNED_INT, 72);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(1, 0);
		glVertexAttribBinding(2, 0);
		glVertexAttribBinding(3, 0);
		glVertexAttribBinding(8, 0);
		glVertexAttribBinding(9, 0);
		glVertexAttribBinding(10, 0);
		glVertexAttribBinding(11, 0);
		glBindVertexBuffer(0, m_vbo, 0, kStride);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(8);
		glEnableVertexAttribArray(9);
		glEnableVertexAttribArray(10);
		glEnableVertexAttribArray(11);

		expected_datai[0]   = IVec4(1, 0, 0, 1);
		expected_datai[1]   = IVec4(5, -6, 0, 1);
		expected_datai[2]   = IVec4(9, -10, 11, 1);
		expected_datai[3]   = IVec4(90, -91, 92, -93);
		expected_dataui[0]  = UVec4(13, 14, 15, 1);
		expected_dataui[1]  = UVec4(17, 18, 0, 1);
		expected_dataui[2]  = UVec4(21, 0, 0, 1);
		expected_dataui[3]  = UVec4(94, 95, 96, 97);
		expected_datai[8]   = IVec4(25, 0, 0, 1);
		expected_datai[9]   = IVec4(29, -30, 0, 1);
		expected_datai[10]  = IVec4(33, -34, 35, 1);
		expected_datai[11]  = IVec4(98, -99, 100, -101);
		expected_dataui[8]  = UVec4(37, 38, 39, 1);
		expected_dataui[9]  = UVec4(41, 42, 0, 1);
		expected_dataui[10] = UVec4(45, 0, 0, 1);
		expected_dataui[11] = UVec4(102, 103, 104, 105);
		return BasicInputIBase::Run();
	}
};

//=============================================================================
// 1.3.2 BasicInputICase2
//-----------------------------------------------------------------------------
class BasicInputICase2 : public BasicInputIBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputIBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputIBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			glVertexAttribI4i(i, 0, 0, 0, 0);
			glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(IVec3) * 2, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IVec3), &IVec3(1, 2, 3)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(IVec3), &IVec3(4, 5, 6)[0]);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(UVec4), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(UVec4), &UVec4(10, 20, 30, 40)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribIFormat(0, 3, GL_INT, 0);
		glVertexAttribIFormat(2, 2, GL_INT, 4);
		glVertexAttribIFormat(15, 1, GL_UNSIGNED_INT, 0);
		glVertexAttribBinding(0, 2);
		glVertexAttribBinding(2, 0);
		glVertexAttribBinding(15, 7);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(15);
		glBindVertexBuffer(0, m_vbo[0], 0, 8);
		glBindVertexBuffer(2, m_vbo[0], 0, 12);
		glBindVertexBuffer(7, m_vbo[1], 4, 16);
		glVertexBindingDivisor(0, 1);
		glVertexBindingDivisor(2, 0);
		glVertexBindingDivisor(7, 2);

		instance_count		= 2;
		expected_datai[0]   = IVec4(1, 2, 3, 1);
		expected_datai[2]   = IVec4(2, 3, 0, 1);
		expected_dataui[7]  = UVec4(20, 0, 0, 1);
		expected_datai[8]   = IVec4(4, 5, 6, 1);
		expected_datai[10]  = IVec4(2, 3, 0, 1);
		expected_dataui[15] = UVec4(20, 0, 0, 1);

		expected_datai[16]  = IVec4(1, 2, 3, 1);
		expected_datai[18]  = IVec4(4, 5, 0, 1);
		expected_dataui[23] = UVec4(20, 0, 0, 1);
		expected_datai[24]  = IVec4(4, 5, 6, 1);
		expected_datai[26]  = IVec4(4, 5, 0, 1);
		expected_dataui[31] = UVec4(20, 0, 0, 1);
		return BasicInputIBase::Run();
	}
};

//=============================================================================
// 1.3.3 BasicInputICase3
//-----------------------------------------------------------------------------
class BasicInputICase3 : public BasicInputIBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputIBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputIBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			glVertexAttribI4i(i, 0, 0, 0, 0);
			glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(IVec3) * 2, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IVec3), &IVec3(1, 2, 3)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(IVec3), &IVec3(4, 5, 6)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glVertexAttribIPointer(7, 3, GL_INT, 12, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glVertexAttribIFormat(0, 2, GL_INT, 4);
		glVertexAttribBinding(0, 7);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(7);

		expected_datai[0]	 = IVec4(2, 3, 0, 1);
		expected_datai[7]	 = IVec4(1, 2, 3, 1);
		expected_datai[0 + 8] = IVec4(5, 6, 0, 1);
		expected_datai[7 + 8] = IVec4(4, 5, 6, 1);
		return BasicInputIBase::Run();
	}
};

//=============================================================================
// BasicInputLBase
//-----------------------------------------------------------------------------
class BasicInputLBase : public VertexAttribBindingBase
{
	GLuint m_po, m_xfbo;

protected:
	DVec4   expected_data[32];
	GLsizei instance_count;
	GLuint  base_instance;

	virtual long Setup()
	{
		m_po = 0;
		glGenBuffers(1, &m_xfbo);
		for (int i = 0; i < 32; ++i)
		{
			expected_data[i] = DVec4(0.0);
		}
		instance_count = 1;
		base_instance  = 0;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_po);
		glDeleteBuffers(1, &m_xfbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "layout(location = 0) in dvec4 vs_in_attrib[8];" NL "out StageData {" NL
			"  dvec4 attrib[8];" NL "} vs_out;" NL "void main() {" NL "  vs_out.attrib[0] = vs_in_attrib[0];" NL
			"  vs_out.attrib[1] = vs_in_attrib[1];" NL "  vs_out.attrib[2] = vs_in_attrib[2];" NL
			"  vs_out.attrib[3] = vs_in_attrib[3];" NL "  vs_out.attrib[4] = vs_in_attrib[4];" NL
			"  vs_out.attrib[5] = vs_in_attrib[5];" NL "  vs_out.attrib[6] = vs_in_attrib[6];" NL
			"  vs_out.attrib[7] = vs_in_attrib[7];" NL "}";
		m_po = glCreateProgram();
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(sh, 1, &glsl_vs, NULL);
			glCompileShader(sh);
			glAttachShader(m_po, sh);
			glDeleteShader(sh);
		}
		{
			const GLchar* const v[8] = {
				"StageData.attrib[0]", "StageData.attrib[1]", "StageData.attrib[2]", "StageData.attrib[3]",
				"StageData.attrib[4]", "StageData.attrib[5]", "StageData.attrib[6]", "StageData.attrib[7]",
			};
			glTransformFeedbackVaryings(m_po, 8, v, GL_INTERLEAVED_ATTRIBS);
		}
		glLinkProgram(m_po);
		if (!CheckProgram(m_po))
			return ERROR;

		{
			std::vector<GLubyte> zero(sizeof(expected_data));
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbo);
			glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, (GLsizeiptr)zero.size(), &zero[0], GL_DYNAMIC_COPY);
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_po);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 2, instance_count, base_instance);
		glEndTransformFeedback();

		DVec4 data[32];
		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(DVec4) * 32, &data[0]);

		for (int i = 0; i < 32; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (expected_data[i][j] != 12345.0 && expected_data[i][j] != data[i][j])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Data is: " << data[i][0] << " " << data[i][1] << " " << data[i][2]
						<< " " << data[i][3] << ", data should be: " << expected_data[i][0] << " "
						<< expected_data[i][1] << " " << expected_data[i][2] << " " << expected_data[i][3]
						<< ", index is: " << i << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
};

//=============================================================================
// 1.4.1 BasicInputLCase1
//-----------------------------------------------------------------------------
class BasicInputLCase1 : public BasicInputLBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		BasicInputLBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputLBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			glVertexAttribL4d(i, 0.0, 0.0, 0.0, 0.0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DVec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DVec4), &DVec4(1.0, 2.0, 3.0, 4.0)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(DVec4), &DVec4(5.0, 6.0, 7.0, 8.0)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 64, sizeof(DVec4), &DVec4(9.0, 10.0, 11.0, 12.0)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribLFormat(0, 4, GL_DOUBLE, 0);
		glVertexAttribLFormat(3, 3, GL_DOUBLE, 8);
		glVertexAttribLFormat(5, 2, GL_DOUBLE, 0);
		glVertexAttribLFormat(7, 1, GL_DOUBLE, 24);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(3, 2);
		glVertexAttribBinding(5, 0);
		glVertexAttribBinding(7, 6);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(5);
		glEnableVertexAttribArray(7);
		glBindVertexBuffer(0, m_vbo, 0, 32);
		glBindVertexBuffer(2, m_vbo, 0, 16);
		glBindVertexBuffer(6, m_vbo, 16, 0);

		expected_data[0]	 = DVec4(1.0, 2.0, 3.0, 4.0);
		expected_data[3]	 = DVec4(2.0, 3.0, 4.0, 12345.0);
		expected_data[5]	 = DVec4(1.0, 2.0, 12345.0, 12345.0);
		expected_data[7]	 = DVec4(6.0, 12345.0, 12345.0, 12345.0);
		expected_data[0 + 8] = DVec4(5.0, 6.0, 7.0, 8.0);
		expected_data[3 + 8] = DVec4(4.0, 5.0, 6.0, 12345.0);
		expected_data[5 + 8] = DVec4(5.0, 6.0, 12345.0, 12345.0);
		expected_data[7 + 8] = DVec4(6.0, 12345.0, 12345.0, 12345.0);
		return BasicInputLBase::Run();
	}
};

//=============================================================================
// 1.4.2 BasicInputLCase2
//-----------------------------------------------------------------------------
class BasicInputLCase2 : public BasicInputLBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		BasicInputLBase::Setup();
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		BasicInputLBase::Cleanup();
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint i = 0; i < 8; ++i)
		{
			glVertexAttribL4d(i, 0.0, 0.0, 0.0, 0.0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DVec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DVec4), &DVec4(1.0, 2.0, 3.0, 4.0)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(DVec4), &DVec4(5.0, 6.0, 7.0, 8.0)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 64, sizeof(DVec4), &DVec4(9.0, 10.0, 11.0, 12.0)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(DVec4) * 3, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DVec4), &DVec4(10.0, 20.0, 30.0, 40.0)[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(DVec4), &DVec4(50.0, 60.0, 70.0, 80.0)[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);
		glVertexAttribLFormat(0, 4, GL_DOUBLE, 0);
		glVertexAttribLFormat(2, 4, GL_DOUBLE, 0);
		glVertexAttribLFormat(4, 2, GL_DOUBLE, 8);
		glVertexAttribBinding(0, 0);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(4, 2);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(4);
		glBindVertexBuffer(0, m_vbo[0], 0, 32);
		glBindVertexBuffer(1, m_vbo[0], 0, 32);
		glBindVertexBuffer(2, m_vbo[1], 8, 16);
		glVertexBindingDivisor(1, 1);

		instance_count		 = 2;
		expected_data[0]	 = DVec4(1.0, 2.0, 3.0, 4.0);
		expected_data[2]	 = DVec4(1.0, 2.0, 3.0, 4.0);
		expected_data[4]	 = DVec4(30.0, 40.0, 12345.0, 12345.0);
		expected_data[0 + 8] = DVec4(5.0, 6.0, 7.0, 8.0);
		expected_data[2 + 8] = DVec4(1.0, 2.0, 3.0, 4.0);
		expected_data[4 + 8] = DVec4(50.0, 60.0, 12345.0, 12345.0);

		expected_data[0 + 16]	 = DVec4(1.0, 2.0, 3.0, 4.0);
		expected_data[2 + 16]	 = DVec4(5.0, 6.0, 7.0, 8.0);
		expected_data[4 + 16]	 = DVec4(30.0, 40.0, 12345.0, 12345.0);
		expected_data[0 + 8 + 16] = DVec4(5.0, 6.0, 7.0, 8.0);
		expected_data[2 + 8 + 16] = DVec4(5.0, 6.0, 7.0, 8.0);
		expected_data[4 + 8 + 16] = DVec4(50.0, 60.0, 12345.0, 12345.0);
		return BasicInputLBase::Run();
	}
};

//=============================================================================
// 1.5 BasicState1
//-----------------------------------------------------------------------------
class BasicState1 : public VertexAttribBindingBase
{
	GLuint m_vao, m_vbo[2];

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(2, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
		if (p < 16)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_BINDINGS is " << p
												<< " but must be at least 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p < 2047)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET is "
												<< p << " but must be at least 2047." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindVertexArray(m_vao);
		// check default state
		for (GLuint i = 0; i < 16; ++i)
		{
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_BINDING, &p);
			if (static_cast<GLint>(i) != p)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_BINDING(" << i
													<< ") is " << p << " should be " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
			glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
			if (p != 0)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(" << i
													<< ") is " << p << " should be 0." << tcu::TestLog::EndMessage;
				return ERROR;
			}
			GLint64 p64;
			glGetInteger64i_v(GL_VERTEX_BINDING_OFFSET, i, &p64);
			if (p64 != 0)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_OFFSET(" << i
													<< ") should be 0." << tcu::TestLog::EndMessage;
				return ERROR;
			}
			glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, i, &p);
			if (p != 16)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_STRIDE(" << i
													<< ") is " << p << " should be 16." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		glVertexAttribFormat(0, 2, GL_BYTE, GL_TRUE, 16);
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &p);
		if (p != 2)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_SIZE(0) is " << p
												<< " should be 2." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &p);
		if (p != GL_BYTE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(0) is " << p
												<< " should be GL_BYTE." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &p);
		if (p != GL_TRUE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_NORMALIZED(0) is "
												<< p << " should be GL_TRUE." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p != 16)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(0) is "
												<< p << " should be 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glVertexAttribIFormat(2, 3, GL_INT, 512);
		glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &p);
		if (p != 3)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_SIZE(2) is " << p
												<< " should be 3." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_TYPE, &p);
		if (p != GL_INT)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(2) is " << p
												<< " should be GL_INT." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p != 512)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(2) is "
												<< p << " should be 512." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &p);
		if (p != GL_TRUE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_INTEGER(2) is " << p
												<< " should be GL_TRUE." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glVertexAttribLFormat(15, 1, GL_DOUBLE, 1024);
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_ARRAY_SIZE, &p);
		if (p != 1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_SIZE(15) is " << p
												<< " should be 1." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_ARRAY_TYPE, &p);
		if (p != GL_DOUBLE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(15) is " << p
												<< " should be GL_DOUBLE." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p != 1024)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(15) is " << p
												<< " should be 1024." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_ARRAY_LONG, &p);
		if (p != GL_TRUE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_LONG(15) is " << p
												<< " should be GL_TRUE." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glVertexAttribBinding(0, 7);
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_BINDING, &p);
		if (p != 7)
			return ERROR;
		glVertexAttribBinding(3, 7);
		glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_BINDING, &p);
		if (p != 7)
			return ERROR;
		glVertexAttribBinding(9, 0);
		glGetVertexAttribiv(9, GL_VERTEX_ATTRIB_BINDING, &p);
		if (p != 0)
			return ERROR;
		glVertexAttribBinding(15, 1);
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_BINDING, &p);
		if (p != 1)
			return ERROR;
		glVertexAttribBinding(15, 15);
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_BINDING, &p);
		if (p != 15)
			return ERROR;

		glBindVertexBuffer(0, m_vbo[0], 1024, 128);
		GLint64 p64;
		glGetInteger64i_v(GL_VERTEX_BINDING_OFFSET, 0, &p64);
		if (p64 != 1024)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_BINDING_OFFSET(0) should be 1024." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, 0, &p);
		if (p != 128)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_STRIDE(0) is " << p
												<< "should be 128." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &p);
		if (p != 0)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_STRIDE(0) is " << p
												<< "should be 0." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &p);
		if (p != 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING(0) is " << p << "should be 0."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindVertexBuffer(15, m_vbo[1], 16, 32);
		glGetInteger64i_v(GL_VERTEX_BINDING_OFFSET, 15, &p64);
		if (p64 != 16)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_BINDING_OFFSET(15) should be 16." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, 15, &p);
		if (p != 32)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_STRIDE(15) is " << p
												<< " should be 32." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &p);
		if (p != 0)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_STRIDE(15) is " << p
												<< " should be 0." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetVertexAttribiv(15, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &p);
		if (p != static_cast<GLint>(m_vbo[1]))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING(15) is " << p << " should be "
				<< m_vbo[1] << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
};

//=============================================================================
// 1.6 BasicState2
//-----------------------------------------------------------------------------
class BasicState2 : public VertexAttribBindingBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		GLint p;
		glBindVertexArray(m_vao);
		for (GLuint i = 0; i < 16; ++i)
		{
			glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, i, &p);
			if (glGetError() != GL_NO_ERROR)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, ...) command generates error."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}
			if (p != 0)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_DIVISOR(" << i
													<< ") is " << p << " should be 0." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		glVertexBindingDivisor(1, 2);
		glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, 1, &p);
		if (p != 2)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_DIVISOR(1) is %d should be 2."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};

class VertexAttribState : public deqp::GLWrapper
{
public:
	int		  array_enabled;
	int		  array_size;
	int		  array_stride;
	int		  array_type;
	int		  array_normalized;
	int		  array_integer;
	int		  array_long;
	int		  array_divisor;
	deUintptr array_pointer;
	int		  array_buffer_binding;
	int		  binding;
	int		  relative_offset;
	int		  index;

	VertexAttribState(int attribindex)
		: array_enabled(0)
		, array_size(4)
		, array_stride(0)
		, array_type(GL_FLOAT)
		, array_normalized(0)
		, array_integer(0)
		, array_long(0)
		, array_divisor(0)
		, array_pointer(0)
		, array_buffer_binding(0)
		, binding(attribindex)
		, relative_offset(0)
		, index(attribindex)
	{
	}

	bool stateVerify()
	{
		GLint p;
		bool  status = true;
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &p);
		if (p != array_enabled)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_ENABLED(" << index << ") is " << p << " should be "
				<< array_enabled << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &p);
		if (p != array_size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_SIZE(" << index << ") is " << p << " should be "
				<< array_size << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &p);
		if (p != array_stride)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_STRIDE(" << index << ") is " << p << " should be "
				<< array_stride << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &p);
		if (p != array_type)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(" << index << ") is " << tcu::toHex(p)
				<< " should be " << tcu::toHex(array_type) << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &p);
		if (p != array_normalized)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_NORMALIZED(" << index << ") is " << p
				<< " should be " << array_normalized << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &p);
		if (p != array_integer)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_INTEGER(" << index << ") is " << p << " should be "
				<< array_integer << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_LONG, &p);
		if (p != array_long)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_LONG(" << index << ") is " << p << " should be "
				<< array_long << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &p);
		if (p != array_divisor)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_DIVISOR(" << index << ") is " << p << " should be "
				<< array_divisor << tcu::TestLog::EndMessage;
			status = false;
		}
		void* pp;
		glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pp);
		if (reinterpret_cast<deUintptr>(pp) != array_pointer)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_POINTER(" << index << ") is "
				<< reinterpret_cast<deUintptr>(pp) << " should be " << array_pointer << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &p);
		if (p != array_buffer_binding)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING(" << index << ") is " << p
				<< " should be " << array_buffer_binding << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_BINDING, &p);
		if (static_cast<GLint>(binding) != p)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_BINDING(" << index
												<< ") is " << p << " should be " << binding << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p != relative_offset)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(" << index << ") is " << p
				<< " should be " << relative_offset << tcu::TestLog::EndMessage;
			status = false;
		}
		return status;
	}
};

class VertexBindingState : public deqp::GLWrapper
{
public:
	int		 buffer;
	long int offset;
	int		 stride;
	int		 divisor;
	int		 index;

	VertexBindingState(int bindingindex) : buffer(0), offset(0), stride(16), divisor(0), index(bindingindex)
	{
	}

	bool stateVerify()
	{
		bool  status = true;
		GLint p;
		glGetIntegeri_v(GL_VERTEX_BINDING_BUFFER, index, &p);
		if (p != buffer)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_BUFFER(" << index
												<< ") is " << p << " should be " << buffer << tcu::TestLog::EndMessage;
			status = false;
		}
		GLint64 p64;
		glGetInteger64i_v(GL_VERTEX_BINDING_OFFSET, index, &p64);
		if (p64 != offset)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_VERTEX_BINDING_OFFSET(" << index << ") is " << p64 << " should be "
				<< offset << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, index, &p);
		if (p != stride)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_STRIDE(" << index
												<< ") is " << p << " should be " << stride << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, index, &p);
		if (p != divisor)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_VERTEX_BINDING_DIVISOR(" << index
												<< ") is " << p << " should be " << divisor << tcu::TestLog::EndMessage;
			status = false;
		}
		return status;
	}
};

//=============================================================================
// 1.6 BasicState3
//-----------------------------------------------------------------------------
class BasicState3 : public VertexAttribBindingBase
{

	GLuint m_vao, m_vbo[3];

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(3, m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(3, m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool status = true;
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
		if (p < 16)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_BINDINGS is %d but must be at least 16."
				<< tcu::TestLog::EndMessage;
			status = false;
		}
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		if (p < 2047)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET is "
												<< p << " but must be at least 2047." << tcu::TestLog::EndMessage;
			status = false;
		}
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &p);
		if (p < 2048)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_STRIDE is " << p
												<< " but must be at least 2048." << tcu::TestLog::EndMessage;
			status = false;
		}

		glBindVertexArray(m_vao);

		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &p);
		if (0 != p)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_ELEMENT_ARRAY_BUFFER_BINDING is " << p
												<< " should be 0." << tcu::TestLog::EndMessage;
			status = false;
		}
		for (GLuint i = 0; i < 16; ++i)
		{
			VertexAttribState va(i);
			if (!va.stateVerify())
				status = false;
		}
		for (GLuint i = 0; i < 16; ++i)
		{
			VertexBindingState vb(i);
			if (!vb.stateVerify())
				status = false;
		}
		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Default state check failed." << tcu::TestLog::EndMessage;
			status = false;
		}

		VertexAttribState va0(0);
		va0.array_size		 = 2;
		va0.array_type		 = GL_BYTE;
		va0.array_normalized = 1;
		va0.relative_offset  = 16;
		VertexBindingState vb0(0);
		glVertexAttribFormat(0, 2, GL_BYTE, GL_TRUE, 16);
		if (!va0.stateVerify() || !vb0.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribFormat state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		VertexAttribState va2(2);
		va2.array_size		= 3;
		va2.array_type		= GL_DOUBLE;
		va2.array_long		= 1;
		va2.relative_offset = 512;
		VertexBindingState vb2(2);
		glVertexAttribLFormat(2, 3, GL_DOUBLE, 512);
		if (!va2.stateVerify() || !vb2.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribIFormat state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		va0.array_buffer_binding = m_vbo[0];
		vb0.buffer				 = m_vbo[0];
		vb0.offset				 = 2048;
		vb0.stride				 = 128;
		glBindVertexBuffer(0, m_vbo[0], 2048, 128);
		if (!va0.stateVerify() || !vb0.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindVertexBuffer state change check failed." << tcu::TestLog::EndMessage;
			status = false;
		}

		va2.array_buffer_binding = m_vbo[2];
		vb2.buffer				 = m_vbo[2];
		vb2.offset				 = 64;
		vb2.stride				 = 256;
		glBindVertexBuffer(2, m_vbo[2], 64, 256);
		if (!va2.stateVerify() || !vb2.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindVertexBuffer state change check failed." << tcu::TestLog::EndMessage;
			status = false;
		}

		glVertexAttribBinding(2, 0);
		va2.binding				 = 0;
		va2.array_buffer_binding = m_vbo[0];
		if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		VertexAttribState  va15(15);
		VertexBindingState vb15(15);
		glVertexAttribBinding(0, 15);
		va0.binding				 = 15;
		va0.array_buffer_binding = 0;
		if (!va0.stateVerify() || !vb0.stateVerify() || !va15.stateVerify() || !vb15.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		glBindVertexBuffer(15, m_vbo[1], 16, 32);
		va0.array_buffer_binding  = m_vbo[1];
		va15.array_buffer_binding = m_vbo[1];
		vb15.buffer				  = m_vbo[1];
		vb15.offset				  = 16;
		vb15.stride				  = 32;
		if (!va0.stateVerify() || !vb0.stateVerify() || !va15.stateVerify() || !vb15.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindVertexBuffer state change check failed." << tcu::TestLog::EndMessage;
			status = false;
		}

		glVertexAttribLFormat(15, 1, GL_DOUBLE, 1024);
		va15.array_size		 = 1;
		va15.array_long		 = 1;
		va15.array_type		 = GL_DOUBLE;
		va15.relative_offset = 1024;
		if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
			!va15.stateVerify() || !vb15.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribFormat state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[2]);
		glVertexAttribPointer(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 8, (void*)640);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		va0.array_size			 = 4;
		va0.array_type			 = GL_UNSIGNED_BYTE;
		va0.array_stride		 = 8;
		va0.array_pointer		 = 640;
		va0.relative_offset		 = 0;
		va0.array_normalized	 = 0;
		va0.binding				 = 0;
		va0.array_buffer_binding = m_vbo[2];
		vb0.buffer				 = m_vbo[2];
		vb0.offset				 = 640;
		vb0.stride				 = 8;
		va2.array_buffer_binding = m_vbo[2];
		if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
			!va15.stateVerify() || !vb15.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribPointer state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		glBindVertexBuffer(0, m_vbo[1], 80, 24);
		vb0.buffer				 = m_vbo[1];
		vb0.offset				 = 80;
		vb0.stride				 = 24;
		va2.array_buffer_binding = m_vbo[1];
		va0.array_buffer_binding = m_vbo[1];
		if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
			!va15.stateVerify() || !vb15.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glBindVertexBuffer state change check failed." << tcu::TestLog::EndMessage;
			status = false;
		}

		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}
};

//=============================================================================
// 1.7 BasicState4
//-----------------------------------------------------------------------------
class BasicState4 : public VertexAttribBindingBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool status = true;
		glBindVertexArray(m_vao);

		for (GLuint i = 0; i < 16; ++i)
		{
			VertexAttribState  va(i);
			VertexBindingState vb(i);
			glVertexAttribDivisor(i, i + 7);
			va.array_divisor = i + 7;
			vb.divisor		 = i + 7;
			if (!va.stateVerify() || !vb.stateVerify())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glVertexAttribDivisor state change check failed."
					<< tcu::TestLog::EndMessage;
				status = false;
			}
		}
		for (GLuint i = 0; i < 16; ++i)
		{
			VertexAttribState  va(i);
			VertexBindingState vb(i);
			glVertexBindingDivisor(i, i);
			va.array_divisor = i;
			vb.divisor		 = i;
			if (!va.stateVerify() || !vb.stateVerify())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glVertexBindingDivisor state change check failed."
					<< tcu::TestLog::EndMessage;
				status = false;
			}
		}

		glVertexAttribBinding(2, 5);
		VertexAttribState va5(5);
		va5.array_divisor = 5;
		VertexBindingState vb5(5);
		vb5.divisor = 5;
		VertexAttribState va2(2);
		va2.array_divisor = 5;
		VertexBindingState vb2(2);
		vb2.divisor = 2;
		va2.binding = 5;
		if (!va5.stateVerify() || !vb5.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		glVertexAttribDivisor(2, 23);
		va2.binding		  = 2;
		va2.array_divisor = 23;
		vb2.divisor		  = 23;
		if (!va5.stateVerify() || !vb5.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glVertexAttribDivisor state change check failed."
				<< tcu::TestLog::EndMessage;
			status = false;
		}

		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}
};

//=============================================================================
// 2.1 AdvancedBindingUpdate
//-----------------------------------------------------------------------------
class AdvancedBindingUpdate : public VertexAttribBindingBase
{
	GLuint m_vao[2], m_vbo[2], m_ebo[2], m_vsp, m_fsp, m_ppo;

	virtual long Setup()
	{
		glGenVertexArrays(2, m_vao);
		glGenBuffers(2, m_vbo);
		glGenBuffers(2, m_ebo);
		m_vsp = m_fsp = 0;
		glGenProgramPipelines(1, &m_ppo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(2, m_vao);
		glDeleteBuffers(2, m_vbo);
		glDeleteBuffers(2, m_ebo);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteProgramPipelines(1, &m_ppo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "layout(location = 0) in vec4 vs_in_position;" NL
			"layout(location = 1) in vec2 vs_in_color_rg;" NL "layout(location = 2) in float vs_in_color_b;" NL
			"layout(location = 3) in dvec3 vs_in_data0;" NL "layout(location = 4) in ivec2 vs_in_data1;" NL
			"out StageData {" NL "  vec2 color_rg;" NL "  float color_b;" NL "  dvec3 data0;" NL "  ivec2 data1;" NL
			"} vs_out;" NL "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "void main() {" NL
			"  vs_out.data1 = vs_in_data1;" NL "  vs_out.data0 = vs_in_data0;" NL "  vs_out.color_b = vs_in_color_b;" NL
			"  vs_out.color_rg = vs_in_color_rg;" NL "  gl_Position = vs_in_position;" NL "}";
		const char* const glsl_fs = "#version 430 core" NL "in StageData {" NL "  vec2 color_rg;" NL
									"  float color_b;" NL "  flat dvec3 data0;" NL "  flat ivec2 data1;" NL
									"} fs_in;" NL "layout(location = 0) out vec4 fs_out_color;" NL
									"uniform dvec3 g_expected_data0;" NL "uniform ivec2 g_expected_data1;" NL
									"void main() {" NL "  fs_out_color = vec4(fs_in.color_rg, fs_in.color_b, 1);" NL
									"  if (fs_in.data0 != g_expected_data0) fs_out_color = vec4(1);" NL
									"  if (fs_in.data1 != g_expected_data1) fs_out_color = vec4(1);" NL "}";
		m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
		if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
			return ERROR;

		glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);

		const GLsizei  kStride[2] = { 52, 62 };
		const GLintptr kOffset[2] = { 0, 8 };
		/* Workaround for alignment issue that may result in bus error on some platforms */
		union {
			double d[3];
			char   c[3 * sizeof(double)];
		} u;

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
		{
			glBufferData(GL_ARRAY_BUFFER, kOffset[0] + 4 * kStride[0], NULL, GL_STATIC_DRAW);
			GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			*reinterpret_cast<Vec2*>(&ptr[kOffset[0] + 0 * kStride[0]]) = Vec2(-1.0f, -1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[0] + 1 * kStride[0]]) = Vec2(1.0f, -1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[0] + 2 * kStride[0]]) = Vec2(1.0f, 1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[0] + 3 * kStride[0]]) = Vec2(-1.0f, 1.0f);

			for (int i = 0; i < 4; ++i)
			{
				*reinterpret_cast<Vec2*>(&ptr[kOffset[0] + 8 + i * kStride[0]])   = Vec2(0.0f, 1.0f);
				*reinterpret_cast<float*>(&ptr[kOffset[0] + 16 + i * kStride[0]]) = 0.0f;
				//*reinterpret_cast<DVec3 *>(&ptr[kOffset[0] + 20 + i * kStride[0]]) = DVec3(1.0, 2.0, 3.0);

				memcpy(u.d, DVec3(1.0, 2.0, 3.0).m_data, 3 * sizeof(double));
				memcpy(&ptr[kOffset[0] + 20 + i * kStride[0]], u.c, 3 * sizeof(double));
				*reinterpret_cast<IVec2*>(&ptr[kOffset[0] + 44 + i * kStride[0]]) = IVec2(1, 2);
			}
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
		{
			glBufferData(GL_ARRAY_BUFFER, kOffset[1] + 4 * kStride[1], NULL, GL_STATIC_DRAW);
			GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			*reinterpret_cast<Vec2*>(&ptr[kOffset[1] + 0 * kStride[1]]) = Vec2(-1.0f, 1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[1] + 1 * kStride[1]]) = Vec2(1.0f, 1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[1] + 2 * kStride[1]]) = Vec2(1.0f, -1.0f);
			*reinterpret_cast<Vec2*>(&ptr[kOffset[1] + 3 * kStride[1]]) = Vec2(-1.0f, -1.0f);

			for (int i = 0; i < 4; ++i)
			{
				*reinterpret_cast<Vec2*>(&ptr[kOffset[1] + 8 + i * kStride[1]])   = Vec2(0.0f, 0.0f);
				*reinterpret_cast<float*>(&ptr[kOffset[1] + 16 + i * kStride[1]]) = 1.0f;
				//*reinterpret_cast<DVec3 *>(&ptr[kOffset[1] + 20 + i * kStride[1]]) = DVec3(4.0, 5.0, 6.0);

				memcpy(u.d, DVec3(4.0, 5.0, 6.0).m_data, 3 * sizeof(double));
				memcpy(&ptr[kOffset[1] + 20 + i * kStride[1]], u.c, 3 * sizeof(double));
				*reinterpret_cast<IVec2*>(&ptr[kOffset[1] + 44 + i * kStride[1]]) = IVec2(3, 4);
			}
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
		{
			GLushort data[4] = { 0, 1, 3, 2 };
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
		{
			GLuint data[4] = { 3, 2, 0, 1 };
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao[0]);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 16);
		glVertexAttribLFormat(3, 3, GL_DOUBLE, 20);
		glVertexAttribIFormat(4, 2, GL_INT, 44);

		for (GLuint i = 0; i < 5; ++i)
		{
			glVertexAttribBinding(i, 0);
			glEnableVertexAttribArray(i);
		}
		glBindVertexArray(m_vao[1]);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 16);
		glVertexAttribLFormat(3, 3, GL_DOUBLE, 20);
		glVertexAttribIFormat(4, 2, GL_INT, 44);
		glVertexAttribBinding(0, 1);
		glVertexAttribBinding(1, 8);
		glVertexAttribBinding(2, 1);
		glVertexAttribBinding(3, 1);
		glVertexAttribBinding(4, 8);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glBindVertexBuffer(1, m_vbo[1], kOffset[1], kStride[1]);
		glBindVertexBuffer(8, m_vbo[0], kOffset[0], kStride[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
		glBindVertexArray(0);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindProgramPipeline(m_ppo);
		glBindVertexArray(m_vao[0]);

		glProgramUniform3d(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data0"), 1.0, 2.0, 3.0);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 1, 2);

		glBindVertexBuffer(0, m_vbo[0], kOffset[0], kStride[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		bool			  status = true;
		std::vector<Vec3> fb(getWindowWidth() * getWindowHeight());
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glProgramUniform3d(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data0"), 4.0, 5.0, 6.0);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 3, 4);

		glBindVertexBuffer(0, m_vbo[1], kOffset[1], kStride[1]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1, 0, 0);

		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 0, 1)))
			status = false;
		if (!status)
			return ERROR;

		glBindVertexBuffer(0, 0, 0, 0);

		glProgramUniform3d(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data0"), 1.0, 2.0, 3.0);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 1, 2);

		for (GLuint i = 0; i < 5; ++i)
			glVertexAttribBinding(i, 15);
		glBindVertexBuffer(15, m_vbo[0], kOffset[0], kStride[0]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glBindVertexBuffer(15, 0, 0, 0);

		glProgramUniform3d(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data0"), 1.0, 2.0, 3.0);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 3, 4);

		glBindVertexBuffer(7, m_vbo[0], kOffset[0], kStride[0]);
		glBindVertexBuffer(12, m_vbo[1], kOffset[1], kStride[1]);
		glVertexAttribBinding(0, 7);
		glVertexAttribBinding(1, 12);
		glVertexAttribBinding(2, 12);
		glVertexAttribBinding(3, 7);
		glVertexAttribBinding(4, 12);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 0, 1)))
			status = false;
		if (!status)
			return ERROR;

		glClear(GL_COLOR_BUFFER_BIT);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 0, 0);
		glDisableVertexAttribArray(4);
		glVertexAttribI4i(4, 0, 0, 0, 0);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 0, 1)))
			status = false;
		if (!status)
			return ERROR;

		glProgramUniform3d(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data0"), 4.0, 5.0, 6.0);
		glProgramUniform2i(m_fsp, glGetUniformLocation(m_fsp, "g_expected_data1"), 1, 2);

		glBindVertexArray(m_vao[1]);
		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1, 0, 0);

		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0.0f, 1.0f, 1.0f)))
			status = false;
		if (!status)
			return ERROR;

		return NO_ERROR;
	}
};

//=============================================================================
// 2.2 AdvancedInstancing
//-----------------------------------------------------------------------------
class AdvancedInstancing : public VertexAttribBindingBase
{
	GLuint m_pipeline;
	GLuint m_vsp, m_fsp;
	GLuint m_vertex_array[2];
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLuint m_object_id_buffer;
	GLuint m_transform_texture, m_transform_buffer;

	virtual long Setup()
	{
		glGenProgramPipelines(1, &m_pipeline);
		m_vsp = m_fsp = 0;
		glGenVertexArrays(2, m_vertex_array);
		glGenBuffers(1, &m_vertex_buffer);
		glGenBuffers(1, &m_index_buffer);
		glGenBuffers(1, &m_object_id_buffer);
		glGenBuffers(1, &m_transform_buffer);
		glGenTextures(1, &m_transform_texture);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgramPipelines(1, &m_pipeline);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteVertexArrays(2, m_vertex_array);
		glDeleteBuffers(1, &m_vertex_buffer);
		glDeleteBuffers(1, &m_index_buffer);
		glDeleteBuffers(1, &m_object_id_buffer);
		glDeleteBuffers(1, &m_transform_buffer);
		glDeleteTextures(1, &m_transform_texture);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_ver = "#version 430 core\n";
		const char* const glsl =
			NL "#if defined(VS_PASS_THROUGH)" NL "layout(location = 0) in vec4 vs_in_position;" NL
			   "layout(location = 1) in vec3 vs_in_normal;" NL "layout(location = 2) in int vs_in_object_id;" NL
			   "out StageData {" NL "  float f;" NL "  vec3 normal;" NL "  int object_id;" NL "} vs_out;" NL
			   "out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL
			   "layout(binding = 0) uniform samplerBuffer g_transform_buffer;" NL "mat4 GetTransformMatrix(int id) {" NL
			   "  return mat4(texelFetch(g_transform_buffer, id * 4)," NL
			   "              texelFetch(g_transform_buffer, id * 4 + 1)," NL
			   "              texelFetch(g_transform_buffer, id * 4 + 2)," NL
			   "              texelFetch(g_transform_buffer, id * 4 + 3));" NL "}" NL "void main() {" NL
			   "  gl_Position = GetTransformMatrix(vs_in_object_id) * vs_in_position;" NL "  vs_out.f = 123.0;" NL
			   "  vs_out.normal = vs_in_normal;" NL "  vs_out.object_id = vs_in_object_id;" NL "}" NL
			   "#elif defined(FS_SOLID_COLOR)" NL "in StageData {" NL "  float f;" NL "  vec3 normal;" NL
			   "  flat int object_id;" NL "} fs_in;" NL "layout(location = 0) out vec4 g_color;" NL "void main() {" NL
			   "  if (fs_in.object_id == 0) g_color = vec4(1, 0, 0, 1);" NL
			   "  else if (fs_in.object_id == 1) g_color = vec4(0, 1, 0, 1);" NL
			   "  else if (fs_in.object_id == 2) g_color = vec4(0, 0, 1, 1);" NL
			   "  else if (fs_in.object_id == 3) g_color = vec4(1, 1, 0, 1);" NL "}" NL "#endif";
		/* VS_PASS_THROUGH */
		{
			const char* const src[] = { glsl_ver, "#define VS_PASS_THROUGH\n", glsl };
			m_vsp					= glCreateShaderProgramv(GL_VERTEX_SHADER, sizeof(src) / sizeof(src[0]), src);
		}
		/* FS_SOLID_COLOR */
		{
			const char* const src[] = { glsl_ver, "#define FS_SOLID_COLOR\n", glsl };
			m_fsp					= glCreateShaderProgramv(GL_FRAGMENT_SHADER, sizeof(src) / sizeof(src[0]), src);
		}
		if (!CheckProgram(m_vsp))
			return ERROR;
		if (!CheckProgram(m_fsp))
			return ERROR;

		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_fsp);

		{
			const float data[] = { -0.5f, -0.5f, 1.0f,  0.0f, 0.0f,  1.5f,  -0.5f, 1.0f, 0.0f,  0.0f,  -0.5f, 1.5f,
								   1.0f,  0.0f,  0.0f,  1.5f, 1.5f,  1.0f,  0.0f,  0.0f, -1.5f, -0.5f, 0.0f,  1.0f,
								   0.0f,  0.5f,  -0.5f, 0.0f, 1.0f,  0.0f,  -1.5f, 1.5f, 0.0f,  1.0f,  0.0f,  0.5f,
								   1.5f,  0.0f,  1.0f,  0.0f, -1.5f, -1.5f, 0.0f,  0.0f, 1.0f,  0.5f,  -1.5f, 0.0f,
								   0.0f,  1.0f,  -1.5f, 0.5f, 0.0f,  0.0f,  1.0f,  0.5f, 0.5f,  0.0f,  0.0f,  1.0f,
								   -0.5f, -1.5f, 1.0f,  1.0f, 0.0f,  1.5f,  -1.5f, 1.0f, 1.0f,  0.0f,  -0.5f, 0.5f,
								   1.0f,  1.0f,  0.0f,  1.5f, 0.5f,  1.0f,  1.0f,  0.0f };
			glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		{
			const unsigned int data[] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 };
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		{
			const int data[] = { 0, 1, 2, 3 };
			glBindBuffer(GL_ARRAY_BUFFER, m_object_id_buffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		glBindVertexArray(m_vertex_array[0]);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 8);
		glVertexAttribIFormat(2, 1, GL_INT, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribBinding(1, 0);
		glBindVertexBuffer(0, m_vertex_buffer, 0, 20);
		glBindVertexBuffer(2, m_object_id_buffer, 0, 4);
		glVertexBindingDivisor(2, 1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBindVertexArray(m_vertex_array[1]);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribIFormat(2, 1, GL_INT, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribBinding(2, 13);
		glVertexAttribBinding(1, 14);
		glVertexAttribBinding(0, 15);
		glBindVertexBuffer(15, m_vertex_buffer, 0, 20);
		glBindVertexBuffer(14, m_vertex_buffer, 8, 20);
		glBindVertexBuffer(13, m_object_id_buffer, 0, 4);
		glVertexBindingDivisor(13, 1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBindVertexArray(0);

		{
			const Mat4 data[] = { Translation(-0.5f, -0.5f, 0.0f), Translation(0.5f, -0.5f, 0.0f),
								  Translation(0.5f, 0.5f, 0.0f), Translation(-0.5f, 0.5f, 0.0f) };
			glBindBuffer(GL_TEXTURE_BUFFER, m_transform_buffer);
			glBufferData(GL_TEXTURE_BUFFER, sizeof(data), &data, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
		glBindTexture(GL_TEXTURE_BUFFER, m_transform_texture);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_transform_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindProgramPipeline(m_pipeline);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, m_transform_texture);
		glBindVertexArray(m_vertex_array[0]);

		std::vector<Vec3> fb(getWindowWidth() * getWindowHeight());
		bool			  status = true;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1, 0, 0);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(1, 0, 0)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(4 * sizeof(unsigned int)), 1, 4, 1);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(8 * sizeof(unsigned int)), 1, 8, 2);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 0, 1)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(12 * sizeof(unsigned int)), 1, 12, 3);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(1, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glBindVertexArray(m_vertex_array[1]);

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(8 * sizeof(unsigned int)), 1, 8, 2);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 0, 1)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1, 0, 0);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(1, 0, 0)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(4 * sizeof(unsigned int)), 1, 4, 1);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(0, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT,
													  (void*)(12 * sizeof(unsigned int)), 1, 12, 3);
		glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
		if (!CheckRectColor(fb, getWindowWidth(), 0, 0, getWindowWidth(), getWindowHeight(), Vec3(1, 1, 0)))
			status = false;
		if (!status)
			return ERROR;

		return NO_ERROR;
	}
};

//=============================================================================
// 2.3 AdvancedIterations
//-----------------------------------------------------------------------------
class AdvancedIterations : public VertexAttribBindingBase
{
	GLuint m_po, m_vao[2], m_xfo[2], m_buffer[2];

	virtual long Setup()
	{
		m_po = 0;
		glGenVertexArrays(2, m_vao);
		glGenTransformFeedbacks(2, m_xfo);
		glGenBuffers(2, m_buffer);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteProgram(m_po);
		glDeleteVertexArrays(2, m_vao);
		glDeleteTransformFeedbacks(2, m_xfo);
		glDeleteBuffers(2, m_buffer);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "in ivec4 vs_in_data;" NL "out StageData {" NL "  ivec4 data;" NL "} vs_out;" NL
			"void main() {" NL "  vs_out.data = vs_in_data + 1;" NL "}";

		m_po = glCreateProgram();
		{
			const GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(sh, 1, &glsl_vs, NULL);
			glCompileShader(sh);
			glAttachShader(m_po, sh);
			glDeleteShader(sh);
		}
		if (!RelinkProgram(1))
			return ERROR;

		glBindBuffer(GL_ARRAY_BUFFER, m_buffer[0]);
		IVec4 zero(0);
		glBufferData(GL_ARRAY_BUFFER, 16, &zero, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer[1]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, &zero, GL_DYNAMIC_READ);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

		glBindVertexArray(m_vao[0]);
		glVertexAttribIFormat(1, 4, GL_INT, 0);
		glEnableVertexAttribArray(1);
		glBindVertexBuffer(1, m_buffer[0], 0, 16);
		glBindVertexArray(m_vao[1]);
		glVertexAttribIFormat(1, 4, GL_INT, 0);
		glEnableVertexAttribArray(1);
		glBindVertexBuffer(1, m_buffer[1], 0, 16);
		glBindVertexArray(0);

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfo[0]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[1]);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfo[1]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[0]);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_po);

		for (int i = 0; i < 10; ++i)
		{
			glBindVertexArray(m_vao[i % 2]);
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfo[i % 2]);
			glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, 1);
			glEndTransformFeedback();
		}
		{
			IVec4 data;
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer[0]);
			glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 16, &data[0]);
			if (!IsEqual(data, IVec4(10)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is: " << data[0] << " " << data[1] << " " << data[2] << " "
					<< data[3] << ", data should be: 10 10 10 10." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		if (!RelinkProgram(5))
			return ERROR;

		glBindVertexArray(m_vao[0]);
		glDisableVertexAttribArray(1);
		glBindVertexBuffer(1, 0, 0, 0);
		glVertexAttribIFormat(5, 4, GL_INT, 0);
		glEnableVertexAttribArray(5);
		glBindVertexBuffer(5, m_buffer[0], 0, 16);
		glBindVertexArray(m_vao[1]);
		glDisableVertexAttribArray(1);
		glBindVertexBuffer(1, 0, 0, 0);
		glVertexAttribIFormat(5, 4, GL_INT, 0);
		glEnableVertexAttribArray(5);
		glBindVertexBuffer(7, m_buffer[1], 0, 16);
		glVertexAttribBinding(5, 7);
		glBindVertexArray(0);

		for (int i = 0; i < 10; ++i)
		{
			glBindVertexArray(m_vao[i % 2]);
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfo[i % 2]);
			glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, 1);
			glEndTransformFeedback();
		}
		{
			IVec4 data;
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer[0]);
			glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 16, &data[0]);
			if (!IsEqual(data, IVec4(20)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is: " << data[0] << " " << data[1] << " " << data[2] << " "
					<< data[3] << ", data should be: 20 20 20 20." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		if (!RelinkProgram(11))
			return ERROR;
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		glBindVertexArray(m_vao[0]);
		glDisableVertexAttribArray(5);
		glBindVertexBuffer(5, 0, 0, 0);
		glVertexAttribIFormat(11, 4, GL_INT, 0);
		glEnableVertexAttribArray(11);
		for (int i = 0; i < 10; ++i)
		{
			glBindVertexBuffer(11, m_buffer[i % 2], 0, 16);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[(i + 1) % 2]);
			glBeginTransformFeedback(GL_POINTS);
			glDrawArrays(GL_POINTS, 0, 1);
			glEndTransformFeedback();
		}
		{
			IVec4 data;
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer[0]);
			glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 16, &data[0]);
			if (!IsEqual(data, IVec4(30)))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Data is: " << data[0] << " " << data[1] << " " << data[2] << " "
					<< data[3] << ", data should be: 30 30 30 30." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	bool RelinkProgram(GLuint index)
	{
		glBindAttribLocation(m_po, index, "vs_in_data");
		{
			const GLchar* const v[1] = { "StageData.data" };
			glTransformFeedbackVaryings(m_po, 1, v, GL_INTERLEAVED_ATTRIBS);
		}
		glLinkProgram(m_po);
		if (!CheckProgram(m_po))
			return false;
		return true;
	}
};

//=============================================================================
// 2.4 AdvancedLargeStrideAndOffsetsNewAndLegacyAPI
//-----------------------------------------------------------------------------
class AdvancedLargeStrideAndOffsetsNewAndLegacyAPI : public VertexAttribBindingBase
{
	GLuint m_vsp, m_ppo, m_ssbo, m_vao, m_vbo;

	virtual long Setup()
	{
		m_vsp = 0;
		glGenProgramPipelines(1, &m_ppo);
		glGenBuffers(1, &m_ssbo);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteProgram(m_vsp);
		glDeleteProgramPipelines(1, &m_ppo);
		glDeleteBuffers(1, &m_ssbo);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 430 core" NL "layout(location = 0) in vec2 vs_in_attrib0;" NL
			"layout(location = 4) in ivec2 vs_in_attrib1;" NL "layout(location = 8) in uvec2 vs_in_attrib2;" NL
			"layout(location = 15) in float vs_in_attrib3;" NL "layout(std430, binding = 1) buffer Output {" NL
			"  vec2 attrib0[4];" NL "  ivec2 attrib1[4];" NL "  uvec2 attrib2[4];" NL "  float attrib3[4];" NL
			"} g_output;" NL "void main() {" NL
			"  g_output.attrib0[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib0;" NL
			"  g_output.attrib1[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib1;" NL
			"  g_output.attrib2[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib2;" NL
			"  g_output.attrib3[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib3;" NL "}";
		m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		if (!CheckProgram(m_vsp))
			return ERROR;
		glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);

		{
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
			glBufferData(GL_ARRAY_BUFFER, 100000, NULL, GL_STATIC_DRAW);
			GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			*reinterpret_cast<Vec2*>(&ptr[16 + 0 * 2048])	= Vec2(1.0f, 2.0f);
			*reinterpret_cast<Vec2*>(&ptr[16 + 1 * 2048])	= Vec2(3.0f, 4.0f);
			*reinterpret_cast<IVec2*>(&ptr[128 + 0 * 2048])  = IVec2(5, 6);
			*reinterpret_cast<IVec2*>(&ptr[128 + 1 * 2048])  = IVec2(7, 8);
			*reinterpret_cast<UVec2*>(&ptr[1024 + 0 * 2048]) = UVec2(9, 10);
			*reinterpret_cast<UVec2*>(&ptr[1024 + 1 * 2048]) = UVec2(11, 12);
			*reinterpret_cast<float*>(&ptr[2032 + 0 * 2048]) = 13.0f;
			*reinterpret_cast<float*>(&ptr[2032 + 1 * 2048]) = 14.0f;

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		glBindVertexArray(m_vao);
		glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 16);
		glVertexAttribIFormat(8, 2, GL_UNSIGNED_INT, 1024);
		glVertexAttribFormat(15, 1, GL_FLOAT, GL_FALSE, 2032);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glVertexAttribIPointer(4, 2, GL_INT, 2048, reinterpret_cast<void*>(128));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glVertexAttribBinding(8, 3);
		glVertexAttribBinding(15, 3);
		glBindVertexBuffer(0, m_vbo, 0, 2048);
		glBindVertexBuffer(3, m_vbo, 0, 2048);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(8);
		glEnableVertexAttribArray(15);
		glBindVertexArray(0);

		std::vector<GLubyte> data((sizeof(Vec2) + sizeof(IVec2) + sizeof(UVec2) + sizeof(float)) * 4, 0xff);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)data.size(), &data[0], GL_DYNAMIC_DRAW);

		glEnable(GL_RASTERIZER_DISCARD);
		glBindProgramPipeline(m_ppo);
		glBindVertexArray(m_vao);
		glDrawArraysInstanced(GL_POINTS, 0, 2, 2);

		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
			GLubyte* ptr = static_cast<GLubyte*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));

			Vec2 i0_v0_a0 = *reinterpret_cast<Vec2*>(&ptr[0]);
			Vec2 i0_v1_a0 = *reinterpret_cast<Vec2*>(&ptr[8]);
			Vec2 i1_v0_a0 = *reinterpret_cast<Vec2*>(&ptr[16]);
			Vec2 i1_v1_a0 = *reinterpret_cast<Vec2*>(&ptr[24]);

			if (!IsEqual(i0_v0_a0, Vec2(1.0f, 2.0f)))
				return ERROR;
			if (!IsEqual(i0_v1_a0, Vec2(3.0f, 4.0f)))
				return ERROR;
			if (!IsEqual(i1_v0_a0, Vec2(1.0f, 2.0f)))
				return ERROR;
			if (!IsEqual(i1_v1_a0, Vec2(3.0f, 4.0f)))
				return ERROR;

			IVec2 i0_v0_a1 = *reinterpret_cast<IVec2*>(&ptr[32]);
			IVec2 i0_v1_a1 = *reinterpret_cast<IVec2*>(&ptr[40]);
			IVec2 i1_v0_a1 = *reinterpret_cast<IVec2*>(&ptr[48]);
			IVec2 i1_v1_a1 = *reinterpret_cast<IVec2*>(&ptr[56]);

			if (!IsEqual(i0_v0_a1, IVec2(5, 6)))
				return ERROR;
			if (!IsEqual(i0_v1_a1, IVec2(7, 8)))
				return ERROR;
			if (!IsEqual(i1_v0_a1, IVec2(5, 6)))
				return ERROR;
			if (!IsEqual(i1_v1_a1, IVec2(7, 8)))
				return ERROR;

			UVec2 i0_v0_a2 = *reinterpret_cast<UVec2*>(&ptr[64]);
			UVec2 i0_v1_a2 = *reinterpret_cast<UVec2*>(&ptr[72]);
			UVec2 i1_v0_a2 = *reinterpret_cast<UVec2*>(&ptr[80]);
			UVec2 i1_v1_a2 = *reinterpret_cast<UVec2*>(&ptr[88]);

			if (!IsEqual(i0_v0_a2, UVec2(9, 10)))
				return ERROR;
			if (!IsEqual(i0_v1_a2, UVec2(11, 12)))
				return ERROR;
			if (!IsEqual(i1_v0_a2, UVec2(9, 10)))
				return ERROR;
			if (!IsEqual(i1_v1_a2, UVec2(11, 12)))
				return ERROR;

			float i0_v0_a3 = *reinterpret_cast<float*>(&ptr[96]);
			float i0_v1_a3 = *reinterpret_cast<float*>(&ptr[100]);
			float i1_v0_a3 = *reinterpret_cast<float*>(&ptr[104]);
			float i1_v1_a3 = *reinterpret_cast<float*>(&ptr[108]);

			if (i0_v0_a3 != 13.0f)
				return ERROR;
			if (i0_v1_a3 != 14.0f)
				return ERROR;
			if (i1_v0_a3 != 13.0f)
				return ERROR;
			if (i1_v1_a3 != 14.0f)
				return ERROR;
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
		return NO_ERROR;
	}
};

//=============================================================================
// 4.1 NegativeBindVertexBuffer
//-----------------------------------------------------------------------------
class NegativeBindVertexBuffer : public VertexAttribBindingBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);

		glBindVertexBuffer(0, 1234, 0, 12);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
		glBindVertexBuffer(p + 1, m_vbo, 0, 12);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindVertexBuffer(0, m_vbo, -10, 12);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glBindVertexBuffer(0, m_vbo, 0, -12);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindVertexArray(0);
		glBindVertexBuffer(0, m_vbo, 0, 12);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}
};

//=============================================================================
// 4.2 NegativeVertexAttribFormat
//-----------------------------------------------------------------------------
class NegativeVertexAttribFormat : public VertexAttribBindingBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	virtual long Run()
	{
		GLenum glError;

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(m_vao);

		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
		glVertexAttribFormat(p + 1, 4, GL_FLOAT, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(p + 2, 4, GL_INT, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(p + 3, 4, GL_DOUBLE, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glVertexAttribFormat(0, 0, GL_FLOAT, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribFormat(0, 5, GL_FLOAT, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(0, 5, GL_INT, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(0, 0, GL_DOUBLE, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(0, GL_BGRA, GL_INT, 0);
		glError = glGetError();
		if (glError != GL_INVALID_OPERATION && glError != GL_INVALID_VALUE)
		{
			//two possible errors here: INVALID_VALUE because GL_BGRA used in *IFormat
			//function AND INVALID_OPERATION because GL_BGRA used with GL_INT
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(0, GL_BGRA, GL_DOUBLE, 0);
		glError = glGetError();
		if (glError != GL_INVALID_OPERATION && glError != GL_INVALID_VALUE)
		{
			//two possible errors here: INVALID_VALUE because GL_BGRA used in *IFormat
			//function AND INVALID_OPERATION because GL_BGRA used with GL_DOUBLE
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribFormat(0, 4, GL_R32F, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_ENUM should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(0, 4, GL_FLOAT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_ENUM should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(0, 4, GL_INT, 0);
		if (glGetError() != GL_INVALID_ENUM)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_ENUM should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribFormat(0, GL_BGRA, GL_FLOAT, GL_TRUE, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribFormat(0, 3, GL_INT_2_10_10_10_REV, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribFormat(0, GL_BGRA, GL_UNSIGNED_BYTE, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
		glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, p + 10);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(0, 4, GL_INT, p + 10);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(0, 4, GL_DOUBLE, p + 10);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glBindVertexArray(0);
		glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribIFormat(0, 4, GL_INT, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glVertexAttribLFormat(0, 4, GL_DOUBLE, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};

//=============================================================================
// 4.3 NegativeVertexAttribBinding
//-----------------------------------------------------------------------------
class NegativeVertexAttribBinding : public VertexAttribBindingBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		glBindVertexArray(m_vao);
		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
		glVertexAttribBinding(p + 1, 0);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
		glVertexAttribBinding(0, p + 1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glBindVertexArray(0);
		glVertexAttribBinding(0, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};
//=============================================================================
// 4.4 NegativeVertexAttribDivisor
//-----------------------------------------------------------------------------
class NegativeVertexAttribDivisor : public VertexAttribBindingBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		glBindVertexArray(m_vao);
		GLint p;
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
		glVertexBindingDivisor(p + 1, 1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_VALUE should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		glBindVertexArray(0);
		glVertexBindingDivisor(0, 1);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "INVALID_OPERATION should be generated." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};

} // anonymous namespace

VertexAttribBindingTests::VertexAttribBindingTests(deqp::Context& context)
	: TestCaseGroup(context, "vertex_attrib_binding", "")
{
}

VertexAttribBindingTests::~VertexAttribBindingTests(void)
{
}

void VertexAttribBindingTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "basic-usage", TestSubcase::Create<BasicUsage>));
	addChild(new TestSubcase(m_context, "basic-input-case1", TestSubcase::Create<BasicInputCase1>));
	addChild(new TestSubcase(m_context, "basic-input-case2", TestSubcase::Create<BasicInputCase2>));
	addChild(new TestSubcase(m_context, "basic-input-case3", TestSubcase::Create<BasicInputCase3>));
	addChild(new TestSubcase(m_context, "basic-input-case4", TestSubcase::Create<BasicInputCase4>));
	addChild(new TestSubcase(m_context, "basic-input-case5", TestSubcase::Create<BasicInputCase5>));
	addChild(new TestSubcase(m_context, "basic-input-case6", TestSubcase::Create<BasicInputCase6>));
	addChild(new TestSubcase(m_context, "basic-input-case7", TestSubcase::Create<BasicInputCase7>));
	addChild(new TestSubcase(m_context, "basic-input-case8", TestSubcase::Create<BasicInputCase8>));
	addChild(new TestSubcase(m_context, "basic-input-case9", TestSubcase::Create<BasicInputCase9>));
	addChild(new TestSubcase(m_context, "basic-input-case10", TestSubcase::Create<BasicInputCase10>));
	addChild(new TestSubcase(m_context, "basic-input-case11", TestSubcase::Create<BasicInputCase11>));
	addChild(new TestSubcase(m_context, "basic-input-case12", TestSubcase::Create<BasicInputCase12>));
	addChild(new TestSubcase(m_context, "basic-inputI-case1", TestSubcase::Create<BasicInputICase1>));
	addChild(new TestSubcase(m_context, "basic-inputI-case2", TestSubcase::Create<BasicInputICase2>));
	addChild(new TestSubcase(m_context, "basic-inputI-case3", TestSubcase::Create<BasicInputICase3>));
	addChild(new TestSubcase(m_context, "basic-inputL-case1", TestSubcase::Create<BasicInputLCase1>));
	addChild(new TestSubcase(m_context, "basic-inputL-case2", TestSubcase::Create<BasicInputLCase2>));
	addChild(new TestSubcase(m_context, "basic-state1", TestSubcase::Create<BasicState1>));
	addChild(new TestSubcase(m_context, "basic-state2", TestSubcase::Create<BasicState2>));
	addChild(new TestSubcase(m_context, "basic-state3", TestSubcase::Create<BasicState3>));
	addChild(new TestSubcase(m_context, "basic-state4", TestSubcase::Create<BasicState4>));
	addChild(new TestSubcase(m_context, "advanced-bindingUpdate", TestSubcase::Create<AdvancedBindingUpdate>));
	addChild(new TestSubcase(m_context, "advanced-instancing", TestSubcase::Create<AdvancedInstancing>));
	addChild(new TestSubcase(m_context, "advanced-iterations", TestSubcase::Create<AdvancedIterations>));
	addChild(new TestSubcase(m_context, "advanced-largeStrideAndOffsetsNewAndLegacyAPI",
							 TestSubcase::Create<AdvancedLargeStrideAndOffsetsNewAndLegacyAPI>));
	addChild(new TestSubcase(m_context, "negative-bindVertexBuffer", TestSubcase::Create<NegativeBindVertexBuffer>));
	addChild(
		new TestSubcase(m_context, "negative-vertexAttribFormat", TestSubcase::Create<NegativeVertexAttribFormat>));
	addChild(
		new TestSubcase(m_context, "negative-vertexAttribBinding", TestSubcase::Create<NegativeVertexAttribBinding>));
	addChild(
		new TestSubcase(m_context, "negative-vertexAttribDivisor", TestSubcase::Create<NegativeVertexAttribDivisor>));
}

} // namespace gl4cts
