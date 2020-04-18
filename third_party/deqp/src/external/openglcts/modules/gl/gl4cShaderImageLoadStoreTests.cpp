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

#include "gl4cShaderImageLoadStoreTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include <assert.h>
#include <climits>
#include <cstdarg>
#include <deque>
#include <iomanip>
#include <map>
#include <sstream>
#include <tcuFloat.hpp>

namespace gl4cts
{
using namespace glw;

namespace
{
typedef tcu::Vec2  vec2;
typedef tcu::Vec4  vec4;
typedef tcu::IVec4 ivec4;
typedef tcu::UVec4 uvec4;
typedef tcu::Mat4  mat4;

class ShaderImageLoadStoreBase : public deqp::SubcaseBase
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
		GLint imagesVS;
		glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &imagesVS);
		if (imagesVS >= requiredVS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredVS << " VS image uniforms but only " << imagesVS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInTCS(int requiredTCS)
	{
		GLint imagesTCS;
		glGetIntegerv(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, &imagesTCS);
		if (imagesTCS >= requiredTCS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredTCS << " TCS image uniforms but only " << imagesTCS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInTES(int requiredTES)
	{
		GLint imagesTES;
		glGetIntegerv(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, &imagesTES);
		if (imagesTES >= requiredTES)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredTES << " TES image uniforms but only " << imagesTES << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInGS(int requiredGS)
	{
		GLint imagesGS;
		glGetIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &imagesGS);
		if (imagesGS >= requiredGS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredGS << " GS image uniforms but only " << imagesGS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool SupportedInGeomStages(int required)
	{
		return SupportedInVS(required) && SupportedInTCS(required) && SupportedInTES(required) &&
			   SupportedInGS(required);
	}

	bool SupportedInStage(int stage, int required)
	{
		switch (stage)
		{
		case 0:
			return SupportedInVS(required);
		case 1:
			return SupportedInTCS(required);
		case 2:
			return SupportedInTES(required);
		case 3:
			return SupportedInGS(required);
		default:
			return true;
		}
	}

	bool SupportedSamples(int required)
	{
		int i;
		glGetIntegerv(GL_MAX_IMAGE_SAMPLES, &i);
		if (i >= required)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << required << " image samples but only " << i << " available." << std::endl;
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

	inline bool ColorEqual(const vec4& c0, const vec4& c1, const vec4& epsilon)
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

	bool IsEqual(vec4 a, vec4 b)
	{
		return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
	}

	bool Equal(const vec4& v0, const vec4& v1, GLenum internalformat)
	{
		if (internalformat == GL_RGBA16_SNORM || internalformat == GL_RG16_SNORM || internalformat == GL_R16_SNORM)
		{
			return ColorEqual(v0, v1, vec4(0.0001f));
		}
		else if (internalformat == GL_RGBA8_SNORM || internalformat == GL_RG8_SNORM || internalformat == GL_R8_SNORM)
		{
			return ColorEqual(v0, v1, vec4(0.01f));
		}
		return (v0[0] == v1[0]) && (v0[1] == v1[1]) && (v0[2] == v1[2]) && (v0[3] == v1[3]);
	}

	bool Equal(const ivec4& a, const ivec4& b, GLenum)
	{
		return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
	}

	bool Equal(const uvec4& a, const uvec4& b, GLenum)
	{
		return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
	}

	template <class T>
	std::string ToString(T v)
	{
		std::ostringstream s;
		s << "[";
		for (int i = 0; i < 4; ++i)
			s << v[i] << (i == 3 ? "" : ",");
		s << "]";
		return s.str();
	}

	bool ValidateReadBuffer(int x, int y, int w, int h, const vec4& expected)
	{
		bool					 status		  = true;
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		vec4					 g_color_eps  = vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		std::vector<vec4> fb(w * h);
		glReadPixels(x, y, w, h, GL_RGBA, GL_FLOAT, &fb[0]);

		for (int yy = 0; yy < h; ++yy)
		{
			for (int xx = 0; xx < w; ++xx)
			{
				const int idx = yy * w + xx;
				if (!ColorEqual(fb[idx], expected, g_color_eps))
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "First bad color: " << ToString(fb[idx])
						<< tcu::TestLog::EndMessage;
					status = false;
					return status;
				}
			}
		}
		return status;
	}

	bool CompileShader(GLuint shader)
	{
		glCompileShader(shader);

		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLsizei length;
			GLchar  log[1024];
			glGetShaderInfoLog(shader, sizeof(log), &length, log);
			if (length > 1)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader Info Log:\n"
													<< log << tcu::TestLog::EndMessage;
			}
			return false;
		}
		return true;
	}

	bool LinkProgram(GLuint program)
	{
		glLinkProgram(program);

		GLint status;
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
			return false;
		}
		return true;
	}

	GLuint BuildProgram(const char* src_vs, const char* src_tcs, const char* src_tes, const char* src_gs,
						const char* src_fs, bool* result = NULL)
	{
		const GLuint p = glCreateProgram();

		if (src_vs)
		{
			GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_vs, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_vs << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (src_tcs)
		{
			GLuint sh = glCreateShader(GL_TESS_CONTROL_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_tcs, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_tcs << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (src_tes)
		{
			GLuint sh = glCreateShader(GL_TESS_EVALUATION_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_tes, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_tes << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (src_gs)
		{
			GLuint sh = glCreateShader(GL_GEOMETRY_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_gs, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_gs << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (src_fs)
		{
			GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			glShaderSource(sh, 1, &src_fs, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_fs << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (!LinkProgram(p))
		{
			if (src_vs)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_vs << tcu::TestLog::EndMessage;
			if (src_tcs)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_tcs << tcu::TestLog::EndMessage;
			if (src_tes)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_tes << tcu::TestLog::EndMessage;
			if (src_gs)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_gs << tcu::TestLog::EndMessage;
			if (src_fs)
				m_context.getTestContext().getLog() << tcu::TestLog::Message << src_fs << tcu::TestLog::EndMessage;
			if (result)
				*result = false;
			return p;
		}

		return p;
	}

	GLuint BuildShaderProgram(GLenum type, const char* src)
	{
		const GLuint p = glCreateShaderProgramv(type, 1, &src);

		GLint status;
		glGetProgramiv(p, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLchar log[1024];
			glGetProgramInfoLog(p, sizeof(log), NULL, log);
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
												<< log << "\n"
												<< src << tcu::TestLog::EndMessage;
		}

		return p;
	}

	void CreateFullViewportQuad(GLuint* vao, GLuint* vbo, GLuint* ebo)
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

	std::string FormatEnumToString(GLenum e)
	{
		switch (e)
		{
		case GL_RGBA32F:
			return "rgba32f";
		case GL_RGBA16F:
			return "rgba16f";
		case GL_RG32F:
			return "rg32f";
		case GL_RG16F:
			return "rg16f";
		case GL_R11F_G11F_B10F:
			return "r11f_g11f_b10f";
		case GL_R32F:
			return "r32f";
		case GL_R16F:
			return "r16f";

		case GL_RGBA32UI:
			return "rgba32ui";
		case GL_RGBA16UI:
			return "rgba16ui";
		case GL_RGB10_A2UI:
			return "rgb10_a2ui";
		case GL_RGBA8UI:
			return "rgba8ui";
		case GL_RG32UI:
			return "rg32ui";
		case GL_RG16UI:
			return "rg16ui";
		case GL_RG8UI:
			return "rg8ui";
		case GL_R32UI:
			return "r32ui";
		case GL_R16UI:
			return "r16ui";
		case GL_R8UI:
			return "r8ui";

		case GL_RGBA32I:
			return "rgba32i";
		case GL_RGBA16I:
			return "rgba16i";
		case GL_RGBA8I:
			return "rgba8i";
		case GL_RG32I:
			return "rg32i";
		case GL_RG16I:
			return "rg16i";
		case GL_RG8I:
			return "rg8i";
		case GL_R32I:
			return "r32i";
		case GL_R16I:
			return "r16i";
		case GL_R8I:
			return "r8i";

		case GL_RGBA16:
			return "rgba16";
		case GL_RGB10_A2:
			return "rgb10_a2";
		case GL_RGBA8:
			return "rgba8";
		case GL_RG16:
			return "rg16";
		case GL_RG8:
			return "rg8";
		case GL_R16:
			return "r16";
		case GL_R8:
			return "r8";

		case GL_RGBA16_SNORM:
			return "rgba16_snorm";
		case GL_RGBA8_SNORM:
			return "rgba8_snorm";
		case GL_RG16_SNORM:
			return "rg16_snorm";
		case GL_RG8_SNORM:
			return "rg8_snorm";
		case GL_R16_SNORM:
			return "r16_snorm";
		case GL_R8_SNORM:
			return "r8_snorm";
		}

		assert(0);
		return "";
	}

	const char* StageName(int stage)
	{
		switch (stage)
		{
		case 0:
			return "Vertex Shader";
		case 1:
			return "Tessellation Control Shader";
		case 2:
			return "Tessellation Evaluation Shader";
		case 3:
			return "Geometry Shader";
		case 4:
			return "Compute Shader";
		}
		assert(0);
		return NULL;
	}

	template <typename T>
	GLenum Format();

	template <typename T>
	GLenum Type();

	template <typename T>
	std::string TypePrefix();

	template <typename T>
	GLenum ImageType(GLenum target);

	void ClearBuffer(GLenum buffer, GLint drawbuffer, const vec4& color)
	{
		glClearBufferfv(buffer, drawbuffer, &color[0]);
	}

	void ClearBuffer(GLenum buffer, GLint drawbuffer, const ivec4& color)
	{
		glClearBufferiv(buffer, drawbuffer, &color[0]);
	}

	void ClearBuffer(GLenum buffer, GLint drawbuffer, const uvec4& color)
	{
		glClearBufferuiv(buffer, drawbuffer, &color[0]);
	}

	bool CheckUniform(GLuint program, const std::string& name, const std::map<std::string, GLuint>& name_index_map,
					  GLint size, GLenum type)
	{
		std::map<std::string, GLuint>::const_iterator iter = name_index_map.find(name);
		assert(iter != name_index_map.end());

		GLchar  name_gl[32];
		GLsizei length_gl;
		GLint   size_gl;
		GLenum  type_gl;

		glGetActiveUniform(program, iter->second, sizeof(name_gl), &length_gl, &size_gl, &type_gl, name_gl);

		if (std::string(name_gl) != name)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Uniform name is " << name_gl
												<< " should be " << name << tcu::TestLog::EndMessage;
			return false;
		}
		if (length_gl != static_cast<GLsizei>(name.length()))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform length is " << length_gl << " should be " << name << "(" << name_gl
				<< ")" << tcu::TestLog::EndMessage;
			return false;
		}
		if (size_gl != size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform size is " << size_gl << " should be " << size << "(" << name_gl
				<< ")" << tcu::TestLog::EndMessage;
			return false;
		}
		if (type_gl != type)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Uniform type is " << type_gl << " should be " << type << "(" << name_gl
				<< ")" << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}

	bool CheckMax(GLenum pname, GLint min_value)
	{
		GLboolean b;
		GLint	 i;
		GLfloat   f;
		GLdouble  d;
		GLint64   i64;

		glGetIntegerv(pname, &i);
		if (i < min_value)
			return false;

		glGetBooleanv(pname, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
			return false;

		glGetFloatv(pname, &f);
		if (static_cast<GLint>(f) < min_value)
			return false;

		glGetDoublev(pname, &d);
		if (static_cast<GLint>(d) < min_value)
			return false;

		glGetInteger64v(pname, &i64);
		if (static_cast<GLint>(i64) < min_value)
			return false;

		return true;
	}

	bool CheckBinding(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access,
					  GLenum format)
	{
		GLint	 i;
		GLboolean b;

		glGetIntegeri_v(GL_IMAGE_BINDING_NAME, unit, &i);
		if (static_cast<GLuint>(i) != texture)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_NAME is " << i
												<< " should be " << texture << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_NAME, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_NAME (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LEVEL, unit, &i);
		if (i != level)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LEVEL is " << i
												<< " should be " << level << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_LEVEL, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_LEVEL (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LAYERED, unit, &i);
		if (i != layered)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYERED is " << i
												<< " should be " << layered << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_LAYERED, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYERED (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LAYER, unit, &i);
		if (i != layer)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYER is " << i
												<< " should be " << layer << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_LAYER, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYER (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_ACCESS, unit, &i);
		if (static_cast<GLenum>(i) != access)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_ACCESS is " << i
												<< " should be " << access << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_ACCESS, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_ACCESS (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_FORMAT, unit, &i);
		if (static_cast<GLenum>(i) != format)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_FORMAT is " << i
												<< " should be " << format << tcu::TestLog::EndMessage;
			return false;
		}
		glGetBooleani_v(GL_IMAGE_BINDING_FORMAT, unit, &b);
		if (b != (i ? GL_TRUE : GL_FALSE))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_BINDING_FORMAT (as boolean) is " << b << " should be "
				<< (i ? GL_TRUE : GL_FALSE) << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}
	const char* EnumToString(GLenum e)
	{
		switch (e)
		{
		case GL_TEXTURE_1D:
			return "GL_TEXTURE_1D";
		case GL_TEXTURE_2D:
			return "GL_TEXTURE_2D";
		case GL_TEXTURE_3D:
			return "GL_TEXTURE_3D";
		case GL_TEXTURE_RECTANGLE:
			return "GL_TEXTURE_RECTANGLE";
		case GL_TEXTURE_CUBE_MAP:
			return "GL_TEXTURE_CUBE_MAP";
		case GL_TEXTURE_1D_ARRAY:
			return "GL_TEXTURE_1D_ARRAY";
		case GL_TEXTURE_2D_ARRAY:
			return "GL_TEXTURE_2D_ARRAY";
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return "GL_TEXTURE_CUBE_MAP_ARRAY";

		default:
			assert(0);
			break;
		}
		return NULL;
	}
};

template <>
GLenum ShaderImageLoadStoreBase::Format<vec4>()
{
	return GL_RGBA;
}

template <>
GLenum ShaderImageLoadStoreBase::Format<ivec4>()
{
	return GL_RGBA_INTEGER;
}

template <>
GLenum ShaderImageLoadStoreBase::Format<uvec4>()
{
	return GL_RGBA_INTEGER;
}

template <>
GLenum ShaderImageLoadStoreBase::Format<GLint>()
{
	return GL_RED_INTEGER;
}

template <>
GLenum ShaderImageLoadStoreBase::Format<GLuint>()
{
	return GL_RED_INTEGER;
}

template <>
GLenum ShaderImageLoadStoreBase::Type<vec4>()
{
	return GL_FLOAT;
}

template <>
GLenum ShaderImageLoadStoreBase::Type<ivec4>()
{
	return GL_INT;
}

template <>
GLenum ShaderImageLoadStoreBase::Type<uvec4>()
{
	return GL_UNSIGNED_INT;
}

template <>
GLenum ShaderImageLoadStoreBase::Type<GLint>()
{
	return GL_INT;
}

template <>
GLenum ShaderImageLoadStoreBase::Type<GLuint>()
{
	return GL_UNSIGNED_INT;
}

template <>
std::string ShaderImageLoadStoreBase::TypePrefix<vec4>()
{
	return "";
}

template <>
std::string ShaderImageLoadStoreBase::TypePrefix<ivec4>()
{
	return "i";
}

template <>
std::string ShaderImageLoadStoreBase::TypePrefix<uvec4>()
{
	return "u";
}

template <>
std::string ShaderImageLoadStoreBase::TypePrefix<GLint>()
{
	return "i";
}

template <>
std::string ShaderImageLoadStoreBase::TypePrefix<GLuint>()
{
	return "u";
}

template <>
GLenum ShaderImageLoadStoreBase::ImageType<vec4>(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		return GL_IMAGE_1D;
	case GL_TEXTURE_2D:
		return GL_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_IMAGE_3D;
	case GL_TEXTURE_RECTANGLE:
		return GL_IMAGE_2D_RECT;
	case GL_TEXTURE_CUBE_MAP:
		return GL_IMAGE_CUBE;
	case GL_TEXTURE_BUFFER:
		return GL_IMAGE_BUFFER;
	case GL_TEXTURE_1D_ARRAY:
		return GL_IMAGE_1D_ARRAY;
	case GL_TEXTURE_2D_ARRAY:
		return GL_IMAGE_2D_ARRAY;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		return GL_IMAGE_CUBE_MAP_ARRAY;
	case GL_TEXTURE_2D_MULTISAMPLE:
		return GL_IMAGE_2D_MULTISAMPLE;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return GL_IMAGE_2D_MULTISAMPLE_ARRAY;
	}
	assert(0);
	return 0;
}

template <>
GLenum ShaderImageLoadStoreBase::ImageType<ivec4>(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		return GL_INT_IMAGE_1D;
	case GL_TEXTURE_2D:
		return GL_INT_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_INT_IMAGE_3D;
	case GL_TEXTURE_RECTANGLE:
		return GL_INT_IMAGE_2D_RECT;
	case GL_TEXTURE_CUBE_MAP:
		return GL_INT_IMAGE_CUBE;
	case GL_TEXTURE_BUFFER:
		return GL_INT_IMAGE_BUFFER;
	case GL_TEXTURE_1D_ARRAY:
		return GL_INT_IMAGE_1D_ARRAY;
	case GL_TEXTURE_2D_ARRAY:
		return GL_INT_IMAGE_2D_ARRAY;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		return GL_INT_IMAGE_CUBE_MAP_ARRAY;
	case GL_TEXTURE_2D_MULTISAMPLE:
		return GL_INT_IMAGE_2D_MULTISAMPLE;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
	}
	assert(0);
	return 0;
}

template <>
GLenum ShaderImageLoadStoreBase::ImageType<uvec4>(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_1D:
		return GL_UNSIGNED_INT_IMAGE_1D;
	case GL_TEXTURE_2D:
		return GL_UNSIGNED_INT_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_UNSIGNED_INT_IMAGE_3D;
	case GL_TEXTURE_RECTANGLE:
		return GL_UNSIGNED_INT_IMAGE_2D_RECT;
	case GL_TEXTURE_CUBE_MAP:
		return GL_UNSIGNED_INT_IMAGE_CUBE;
	case GL_TEXTURE_BUFFER:
		return GL_UNSIGNED_INT_IMAGE_BUFFER;
	case GL_TEXTURE_1D_ARRAY:
		return GL_UNSIGNED_INT_IMAGE_1D_ARRAY;
	case GL_TEXTURE_2D_ARRAY:
		return GL_UNSIGNED_INT_IMAGE_2D_ARRAY;
	case GL_TEXTURE_CUBE_MAP_ARRAY:
		return GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY;
	case GL_TEXTURE_2D_MULTISAMPLE:
		return GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE;
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
		return GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
	}
	assert(0);
	return 0;
}

//-----------------------------------------------------------------------------
// 1.1.1 BasicAPIGet
//-----------------------------------------------------------------------------
class BasicAPIGet : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		if (!CheckMax(GL_MAX_IMAGE_UNITS, 8))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_IMAGE_UNITS value is invalid." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, 8))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_IMAGE_SAMPLES, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_IMAGE_SAMPLES value is invalid." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_VERTEX_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_VERTEX_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_GEOMETRY_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, 8))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_FRAGMENT_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_COMBINED_IMAGE_UNIFORMS, 8))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_COMBINED_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.1.2 BasicAPIBind
//-----------------------------------------------------------------------------
class BasicAPIBind : public ShaderImageLoadStoreBase
{
	GLuint m_texture;

	virtual long Setup()
	{
		m_texture = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		for (GLuint index = 0; index < 8; ++index)
		{
			if (!CheckBinding(index, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Binding point " << index
													<< " has invalid default state." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, 16, 16, 4, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, 8, 8, 4, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 2, GL_R32F, 4, 4, 4, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 3, GL_R32F, 2, 2, 4, 0, GL_RED, GL_FLOAT, NULL);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 4, GL_R32F, 1, 1, 4, 0, GL_RED, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		if (!CheckBinding(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F))
			return ERROR;

		glBindImageTexture(1, m_texture, 1, GL_TRUE, 1, GL_WRITE_ONLY, GL_RGBA8);
		if (!CheckBinding(1, m_texture, 1, GL_TRUE, 1, GL_WRITE_ONLY, GL_RGBA8))
			return ERROR;

		glBindImageTexture(4, m_texture, 3, GL_FALSE, 2, GL_READ_ONLY, GL_RG16);
		if (!CheckBinding(4, m_texture, 3, GL_FALSE, 2, GL_READ_ONLY, GL_RG16))
			return ERROR;

		glBindImageTexture(7, m_texture, 4, GL_FALSE, 3, GL_READ_ONLY, GL_R32I);
		if (!CheckBinding(7, m_texture, 4, GL_FALSE, 3, GL_READ_ONLY, GL_R32I))
			return ERROR;

		glDeleteTextures(1, &m_texture);
		m_texture = 0;

		for (GLuint index = 0; index < 8; ++index)
		{
			GLint name;
			glGetIntegeri_v(GL_IMAGE_BINDING_NAME, index, &name);
			if (name != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Binding point " << index
					<< " should be set to 0 after texture deletion." << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteTextures(1, &m_texture);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.1.3 BasicAPIBarrier
//-----------------------------------------------------------------------------
class BasicAPIBarrier : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
		glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT);
		glMemoryBarrier(GL_UNIFORM_BARRIER_BIT);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
		glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		glMemoryBarrier(GL_TRANSFORM_FEEDBACK_BARRIER_BIT);
		glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT |
						GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT |
						GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT |
						GL_FRAMEBUFFER_BARRIER_BIT | GL_TRANSFORM_FEEDBACK_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.1.4 BasicAPITexParam
//-----------------------------------------------------------------------------
class BasicAPITexParam : public ShaderImageLoadStoreBase
{
	GLuint m_texture;

	virtual long Setup()
	{
		m_texture = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 16, 16, 0, GL_RED, GL_FLOAT, NULL);

		GLint   i;
		GLfloat f;
		GLuint  ui;

		glGetTexParameteriv(GL_TEXTURE_2D, GL_IMAGE_FORMAT_COMPATIBILITY_TYPE, &i);
		if (i != GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_FORMAT_COMPATIBILITY_TYPE should equal to "
				<< "GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE for textures allocated by the GL."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetTexParameterfv(GL_TEXTURE_2D, GL_IMAGE_FORMAT_COMPATIBILITY_TYPE, &f);
		if (static_cast<GLenum>(f) != GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_FORMAT_COMPATIBILITY_TYPE should equal to "
				<< "GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE for textures allocated by the GL."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetTexParameterIiv(GL_TEXTURE_2D, GL_IMAGE_FORMAT_COMPATIBILITY_TYPE, &i);
		if (i != GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_FORMAT_COMPATIBILITY_TYPE should equal to "
				<< "GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE for textures allocated by the GL."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glGetTexParameterIuiv(GL_TEXTURE_2D, GL_IMAGE_FORMAT_COMPATIBILITY_TYPE, &ui);
		if (ui != GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_IMAGE_FORMAT_COMPATIBILITY_TYPE should equal to "
				<< "GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE for textures allocated by the GL."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteTextures(1, &m_texture);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.2.1 BasicAllFormatsStore
//-----------------------------------------------------------------------------
class BasicAllFormatsStore : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Write(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Write(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Write(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint   program = BuildProgram(src_vs, NULL, NULL, NULL, GenFS(internalformat, write_value).c_str());
		const int	  kSize   = 16;
		std::vector<T> data(kSize * kSize);
		GLuint		   texture;
		glGenTextures(1, &texture);

		for (GLuint unit = 0; unit < 8; ++unit)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
			glBindTexture(GL_TEXTURE_2D, 0);

			glViewport(0, 0, kSize, kSize);
			glUseProgram(program);
			glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
			glBindVertexArray(m_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindTexture(GL_TEXTURE_2D, texture);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, Format<T>(), Type<T>(), &data[0]);

			for (int i = 0; i < kSize * kSize; ++i)
			{
				if (!Equal(data[i], expected_value, internalformat))
				{
					glDeleteTextures(1, &texture);
					glUseProgram(0);
					glDeleteProgram(program);
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Value is: " << ToString(data[i])
														<< ". Value should be: " << ToString(expected_value)
														<< ". Format is: " << FormatEnumToString(internalformat)
														<< ". Unit is: " << unit << tcu::TestLog::EndMessage;
					return false;
				}
			}

			if (unit < 7)
			{
				glUniform1i(glGetUniformLocation(program, "g_image"), static_cast<GLint>(unit + 1));
			}
		}

		glDeleteTextures(1, &texture);
		glUseProgram(0);
		glDeleteProgram(program);

		return true;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2D g_image;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
								 "  imageStore(g_image, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "  discard;" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.2.2 BasicAllFormatsLoad
//-----------------------------------------------------------------------------
class BasicAllFormatsLoad : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Read(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint   program = BuildProgram(src_vs, NULL, NULL, NULL, GenFS(internalformat, expected_value).c_str());
		const int	  kSize   = 16;
		std::vector<T> data(kSize * kSize, value);
		GLuint		   texture;
		glGenTextures(1, &texture);

		for (GLuint unit = 0; unit < 8; ++unit)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
			glBindTexture(GL_TEXTURE_2D, 0);

			glViewport(0, 0, kSize, kSize);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(program);
			glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
			glBindVertexArray(m_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
			{
				glDeleteTextures(1, &texture);
				glUseProgram(0);
				glDeleteProgram(program);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Bad load value. Format is: " << FormatEnumToString(internalformat)
					<< ". Unit is: " << unit << tcu::TestLog::EndMessage;
				return false;
			}

			if (unit < 7)
			{
				glUniform1i(glGetUniformLocation(program, "g_image"), static_cast<GLint>(unit + 1));
			}
		}

		glDeleteTextures(1, &texture);
		glUseProgram(0);
		glDeleteProgram(program);

		return true;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2D g_image;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image, coord);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "  else o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.2.3 BasicAllFormatsStoreGeometryStages
//-----------------------------------------------------------------------------
class BasicAllFormatsStoreGeometryStages : public ShaderImageLoadStoreBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInGeomStages(1))
			return NOT_SUPPORTED;
		glEnable(GL_RASTERIZER_DISCARD);

		if (!Write(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Write(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Write(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const GLuint program =
			BuildProgram(GenVS(internalformat, write_value).c_str(), GenTCS(internalformat, write_value).c_str(),
						 GenTES(internalformat, write_value).c_str(), GenGS(internalformat, write_value).c_str(), NULL);
		const int	  kSize = 1;
		std::vector<T> data(kSize * kSize);
		GLuint		   texture[4];
		glGenTextures(4, texture);

		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture[i]);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalformat, kSize, kSize, 1, 0, Format<T>(), Type<T>(), &data[0]);
		}
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image0"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image1"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image2"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image3"), 3);
		for (GLuint i = 0; i < 4; ++i)
		{
			glBindImageTexture(i, texture[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		}
		glBindVertexArray(m_vao);
		glPatchParameteri(GL_PATCH_VERTICES, 1);
		glDrawArrays(GL_PATCHES, 0, 1);
		glPatchParameteri(GL_PATCH_VERTICES, 3);

		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture[i]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, Format<T>(), Type<T>(), &data[0]);

			if (!Equal(data[0], expected_value, internalformat))
			{
				glDeleteTextures(4, texture);
				glUseProgram(0);
				glDeleteProgram(program);
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Value is: " << ToString(data[0])
													<< ". Value should be: " << ToString(expected_value)
													<< ". Format is: " << FormatEnumToString(internalformat)
													<< ". Stage is: " << StageName(i) << tcu::TestLog::EndMessage;
				return false;
			}
		}
		glDeleteTextures(4, texture);
		glUseProgram(0);
		glDeleteProgram(program);
		return true;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	template <typename T>
	std::string GenVS(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2DArray g_image0;" NL "void main() {" NL
								 "  ivec3 coord = ivec3(gl_VertexID, 0, 0);" NL "  imageStore(g_image0, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenTCS(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(vertices = 1) out;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image1;" NL "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL
			  "  gl_TessLevelInner[1] = 1;" NL "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL
			  "  gl_TessLevelOuter[2] = 1;" NL "  gl_TessLevelOuter[3] = 1;" NL
			  "  ivec3 coord = ivec3(gl_PrimitiveID, 0, 0);" NL "  imageStore(g_image1, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenTES(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(triangles, point_mode) in;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image2;" NL "void main() {" NL "  ivec3 coord = ivec3(gl_PrimitiveID, 0, 0);" NL
			  "  imageStore(g_image2, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenGS(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image3;" NL "void main() {" NL "  ivec3 coord = ivec3(gl_PrimitiveIDIn, 0, 0);" NL
			  "  imageStore(g_image3, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.2.4 BasicAllFormatsLoadGeometryStages
//-----------------------------------------------------------------------------
class BasicAllFormatsLoadGeometryStages : public ShaderImageLoadStoreBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInGeomStages(2))
			return NOT_SUPPORTED;
		glEnable(GL_RASTERIZER_DISCARD);

		if (!Read(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value)
	{
		const GLuint program = BuildProgram(
			GenVS(internalformat, expected_value).c_str(), GenTCS(internalformat, expected_value).c_str(),
			GenTES(internalformat, expected_value).c_str(), GenGS(internalformat, expected_value).c_str(), NULL);
		const int	  kSize = 1;
		std::vector<T> data(kSize * kSize, value);
		GLuint		   texture[8];
		glGenTextures(8, texture);

		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture[i]);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalformat, kSize, kSize, 1, 0, Format<T>(), Type<T>(), &data[0]);
		}
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		vec4 zero(0);
		for (int i = 4; i < 8; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, texture[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kSize, kSize, 0, GL_RGBA, GL_FLOAT, &zero);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image0"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image1"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image2"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image3"), 3);
		glUniform1i(glGetUniformLocation(program, "g_image0_result"), 4);
		glUniform1i(glGetUniformLocation(program, "g_image1_result"), 5);
		glUniform1i(glGetUniformLocation(program, "g_image2_result"), 6);
		glUniform1i(glGetUniformLocation(program, "g_image3_result"), 7);

		for (GLuint i = 0; i < 4; ++i)
		{
			glBindImageTexture(i, texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		}
		for (GLuint i = 4; i < 8; ++i)
		{
			glBindImageTexture(i, texture[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		}
		glBindVertexArray(m_vao);
		glPatchParameteri(GL_PATCH_VERTICES, 1);
		glDrawArrays(GL_PATCHES, 0, 1);
		glPatchParameteri(GL_PATCH_VERTICES, 3);

		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		vec4					 g_color_eps  = vec4(
			1.f / static_cast<float>(1 << pixelFormat.redBits), 1.f / static_cast<float>(1 << pixelFormat.greenBits),
			1.f / static_cast<float>(1 << pixelFormat.blueBits), 1.f / static_cast<float>(1 << pixelFormat.alphaBits));

		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, texture[i + 4]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			vec4 result;
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &result[0]);
			if (!ColorEqual(result, vec4(0, 1, 0, 1), g_color_eps))
			{
				glDeleteTextures(8, texture);
				glUseProgram(0);
				glDeleteProgram(program);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Bad load value. Format is: " << FormatEnumToString(internalformat)
					<< ". Stage is: " << StageName(i) << tcu::TestLog::EndMessage;
				return false;
			}
		}
		glDeleteTextures(8, texture);
		glUseProgram(0);
		glDeleteProgram(program);
		return true;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	template <typename T>
	std::string GenVS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>()
		   << "image2DArray g_image0;" NL "layout(rgba32f) writeonly uniform image2D g_image0_result;" NL
			  "void main() {" NL "  ivec3 coord = ivec3(gl_VertexID, 0, 0);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image0, coord);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value << ") imageStore(g_image0_result, coord.xy, vec4(1.0, 0.0, 0.0, 1.0));" NL
								"  else imageStore(g_image0_result, coord.xy, vec4(0.0, 1.0, 0.0, 1.0));" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenTCS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(vertices = 1) out;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") readonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image1;" NL "layout(rgba32f) writeonly uniform image2D g_image1_result;" NL
			  "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL "  gl_TessLevelInner[1] = 1;" NL
			  "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL "  gl_TessLevelOuter[2] = 1;" NL
			  "  gl_TessLevelOuter[3] = 1;" NL "  ivec3 coord = ivec3(gl_PrimitiveID, 0, 0);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image1, coord);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value << ") imageStore(g_image1_result, coord.xy, vec4(1.0, 0.0, 0.0, 1.0));" NL
								"  else imageStore(g_image1_result, coord.xy, vec4(0.0, 1.0, 0.0, 1.0));" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenTES(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(triangles, point_mode) in;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image2;" NL "layout(rgba32f) writeonly uniform image2D g_image2_result;" NL
			  "void main() {" NL "  ivec3 coord = ivec3(gl_PrimitiveID, 0, 0);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image2, coord);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value << ") imageStore(g_image2_result, coord.xy, vec4(1.0, 0.0, 0.0, 1.0));" NL
								"  else imageStore(g_image2_result, coord.xy, vec4(0.0, 1.0, 0.0, 1.0));" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenGS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image3;" NL "layout(rgba32f) writeonly uniform image2D g_image3_result;" NL
			  "void main() {" NL "  ivec3 coord = ivec3(gl_PrimitiveIDIn, 0, 0);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image3, coord);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value << ") imageStore(g_image3_result, coord.xy, vec4(1.0, 0.0, 0.0, 1.0));" NL
								"  else imageStore(g_image3_result, coord.xy, vec4(0.0, 1.0, 0.0, 1.0));" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.2.5 BasicAllFormatsLoadStoreComputeStage
//-----------------------------------------------------------------------------
class BasicAllFormatsLoadStoreComputeStage : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_ARB_compute_shader not supported, skipping test"
				<< tcu::TestLog::EndMessage;
			return NOT_SUPPORTED;
		}

		if (!Read(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Read(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Read(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Read(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Read(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Read(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Read(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Read(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Read(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;
		if (!Read(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Read(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value)
	{
		GLuint			  program;
		std::string		  source = GenCS<T>(internalformat);
		const char* const src	= source.c_str();
		GLuint			  sh	 = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(sh, 1, &src, NULL);
		glCompileShader(sh);
		program = glCreateProgram();
		glAttachShader(program, sh);
		glLinkProgram(program);
		glDeleteShader(sh);

		const int	  kSize = 1;
		std::vector<T> data(kSize * kSize, value);
		GLuint		   texture[2];
		glGenTextures(2, texture);

		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		vec4 zero(0);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &zero);

		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_read"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_write"), 1);

		glBindImageTexture(0, texture[0], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(1, texture[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glDispatchCompute(1, 1, 1);

		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, texture[i]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, Format<T>(), Type<T>(), &data[0]);

			if (!Equal(data[0], expected_value, internalformat))
			{
				glDeleteTextures(4, texture);
				glUseProgram(0);
				glDeleteProgram(program);
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Value is: " << ToString(data[0])
					<< ". Value should be: " << ToString(expected_value)
					<< ". Format is: " << FormatEnumToString(internalformat) << tcu::TestLog::EndMessage;
				return false;
			}
		}
		glDeleteTextures(2, texture);
		glUseProgram(0);
		glDeleteProgram(program);
		return true;
	}

	template <typename T>
	std::string GenCS(GLenum internalformat)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "#extension GL_ARB_compute_shader : require" NL "layout(local_size_x = 1) in;" NL
			  "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2D g_image_read;" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2D g_image_write;" NL "void main() {" NL
								 "  ivec2 coord = ivec2(int(gl_GlobalInvocationID.x), 0);" NL "  "
		   << TypePrefix<T>()
		   << "vec4 v = imageLoad(g_image_read, coord);" NL "  imageStore(g_image_write, coord, v);" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.1 BasicAllTargetsStore
//-----------------------------------------------------------------------------
class BasicAllTargetsStore : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Write(GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;

		if (!WriteCubeArray(GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!WriteCubeArray(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!WriteCubeArray(GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;

		if (SupportedSamples(4))
		{
			if (!WriteMS(GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
				return ERROR;

			GLint isamples;
			glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &isamples);
			if (isamples >= 4)
			{
				if (!WriteMS(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
					return ERROR;
				if (!WriteMS(GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
					return ERROR;
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFS(internalformat, write_value).c_str());
		GLuint		 textures[8];
		GLuint		 buffer;
		glGenTextures(8, textures);
		glGenBuffers(1, &buffer);

		const int	  kSize = 16;
		std::vector<T> data(kSize * kSize * 2);

		glBindTexture(GL_TEXTURE_1D, textures[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage1D(GL_TEXTURE_1D, 0, internalformat, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D, 0);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_3D, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE, textures[3]);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		glBufferData(GL_TEXTURE_BUFFER, kSize * sizeof(T), &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, textures[5]);
		glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glBindTexture(GL_TEXTURE_1D_ARRAY, textures[6]);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, internalformat, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D_ARRAY, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(4, textures[4], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(5, textures[5], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(6, textures[6], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(7, textures[7], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_1d"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program, "g_image_cube"), 4);
		glUniform1i(glGetUniformLocation(program, "g_image_buffer"), 5);
		glUniform1i(glGetUniformLocation(program, "g_image_1darray"), 6);
		glUniform1i(glGetUniformLocation(program, "g_image_2darray"), 7);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		glBindTexture(GL_TEXTURE_1D, textures[0]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_1D, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D, 0);
		for (int i = 0; i < kSize; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_1D target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}
		std::fill(data.begin(), data.end(), T(0));

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_2D target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_3D, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);
		for (int i = 0; i < kSize * kSize * 2; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_3D target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glBindTexture(GL_TEXTURE_RECTANGLE, textures[3]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_RECTANGLE, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_RECTANGLE target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			for (int face = 0; face < 6; ++face)
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, Format<T>(), Type<T>(), &data[0]);
				for (int i = 0; i < kSize * kSize; ++i)
				{
					if (!tcu::allEqual(data[i], expected_value))
					{
						status = false;
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message
							<< "GL_TEXTURE_CUBE_MAP_POSITIVE_X target failed. Value is: " << ToString(data[i])
							<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
						break;
					}
				}
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		glBindTexture(GL_TEXTURE_BUFFER, textures[5]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		glGetBufferSubData(GL_TEXTURE_BUFFER, 0, kSize * sizeof(T), &data[0]);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		for (int i = 0; i < kSize; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_BUFFER target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glBindTexture(GL_TEXTURE_1D_ARRAY, textures[6]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_1D_ARRAY, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D_ARRAY, 0);
		for (int i = 0; i < kSize * 2; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_1D_ARRAY target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		for (int i = 0; i < kSize * kSize * 2; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "GL_TEXTURE_2D_ARRAY target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(8, textures);
		glDeleteBuffers(1, &buffer);

		return status;
	}

	template <typename T>
	bool WriteMS(GLenum internalformat, const T& write_value, const T& expected_value)
	{

		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program	 = BuildProgram(src_vs, NULL, NULL, NULL, GenFSMS(internalformat, write_value).c_str());
		const GLuint val_program = BuildProgram(src_vs, NULL, NULL, NULL, GenFSMSVal(expected_value).c_str());
		GLuint		 textures[2];
		glGenTextures(2, textures);

		const int kSize = 16;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[0]);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalformat, kSize, kSize, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures[1]);
		glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 4, internalformat, kSize, kSize, 2, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(4, textures[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms_array"), 4);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures[1]);

		glUseProgram(val_program);
		glUniform1i(glGetUniformLocation(val_program, "g_sampler_2dms"), 0);
		glUniform1i(glGetUniformLocation(val_program, "g_sampler_2dms_array"), 1);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "GL_TEXTURE_2D_MULTISAMPLE or GL_TEXTURE_2D_MULTISAMPLE_ARRAY target failed."
				<< tcu::TestLog::EndMessage;
		}

		glActiveTexture(GL_TEXTURE0);
		glDeleteTextures(2, textures);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(val_program);

		return status;
	}

	template <typename T>
	bool WriteCubeArray(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program =
			BuildProgram(src_vs, NULL, NULL, NULL, GenFSCubeArray(internalformat, write_value).c_str());
		GLuint textures[1];
		glGenTextures(1, textures);

		const int kSize = 16;

		std::vector<T> data(kSize * kSize * 12);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[0]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalformat, kSize, kSize, 12, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glUseProgram(program);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		std::fill(data.begin(), data.end(), T(0));
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[0]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_CUBE_MAP_ARRAY, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
		for (int i = 0; i < kSize * kSize * 12; ++i)
		{
			if (!tcu::allEqual(data[i], expected_value))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GL_TEXTURE_CUBE_MAP_ARRAY target failed. Value is: " << ToString(data[i])
					<< ". Value should be: " << ToString(expected_value) << tcu::TestLog::EndMessage;
				break;
			}
		}

		glDeleteTextures(1, textures);
		glUseProgram(0);
		glDeleteProgram(program);

		return status;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image1D g_image_1d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>() << "image2D g_image_2d;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image3D g_image_3d;" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2DRect g_image_2drect;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>() << "imageCube g_image_cube;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "imageBuffer g_image_buffer;" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			  "  imageStore(g_image_1d, coord.x, "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_2d, coord, " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  imageStore(g_image_3d, ivec3(coord.xy, 0), " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  imageStore(g_image_3d, ivec3(coord.xy, 1), " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  imageStore(g_image_2drect, coord, " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 0), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 1), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 2), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 3), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 4), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_cube, ivec3(coord, 5), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_buffer, coord.x, " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_1darray, ivec2(coord.x, 0), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_1darray, ivec2(coord.x, 1), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_2darray, ivec3(coord, 0), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_2darray, ivec3(coord, 1), " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  discard;" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSMS(GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2DMS g_image_2dms;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DMSArray g_image_2dms_array;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			  "  imageStore(g_image_2dms, coord, 0, "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_2dms, coord, 1, "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_2dms, coord, 2, "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_2dms, coord, 3, "
		   << TypePrefix<T>() << "vec4" << write_value
		   << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 0), 0, " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 0), 1, " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 0), 2, "
		   << TypePrefix<T>() << "vec4" << write_value
		   << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 0), 3, " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 1), 0, " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 1), 1, "
		   << TypePrefix<T>() << "vec4" << write_value
		   << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 1), 2, " << TypePrefix<T>() << "vec4"
		   << write_value << ");" NL "  imageStore(g_image_2dms_array, ivec3(coord, 1), 3, " << TypePrefix<T>()
		   << "vec4" << write_value << ");" NL "  discard;" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSMSVal(const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "uniform " << TypePrefix<T>()
		   << "sampler2DMS g_sampler_2dms;" NL "uniform " << TypePrefix<T>()
		   << "sampler2DMSArray g_sampler_2dms_array;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  if (texelFetch(g_sampler_2dms, coord, 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (texelFetch(g_sampler_2dms, coord, 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (texelFetch(g_sampler_2dms, coord, 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (texelFetch(g_sampler_2dms, coord, 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 0), 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 0), 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 0), 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 0), 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 1), 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 1), 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 1), 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL
			  "  if (texelFetch(g_sampler_2dms_array, ivec3(coord, 1), 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSCubeArray(GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>()
		   << "imageCubeArray g_image_cube_array;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			  "  imageStore(g_image_cube_array, ivec3(coord, 0), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 1), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 2), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 3), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 4), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 5), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 6), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 7), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 8), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 9), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 10), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  imageStore(g_image_cube_array, ivec3(coord, 11), "
		   << TypePrefix<T>() << "vec4" << write_value << ");" NL "  discard;" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.2.1 BasicAllTargetsLoadNonMS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadNonMS : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Read(GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f)))
			return ERROR;
		if (!Read(GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000)))
			return ERROR;
		if (!Read(GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000)))
			return ERROR;

		if (!ReadCube(GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f)))
			return ERROR;
		if (!ReadCube(GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000)))
			return ERROR;
		if (!ReadCube(GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000)))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFS(internalformat, expected_value).c_str());
		GLuint		 textures[7];
		GLuint		 buffer;
		glGenTextures(7, textures);
		glGenBuffers(1, &buffer);

		const int	  kSize = 16;
		std::vector<T> data(kSize * kSize * 2, value);

		glBindTexture(GL_TEXTURE_1D, textures[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage1D(GL_TEXTURE_1D, 0, internalformat, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D, 0);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_3D, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE, textures[3]);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		glBufferData(GL_TEXTURE_BUFFER, kSize * sizeof(T), &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, textures[4]);
		glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glBindTexture(GL_TEXTURE_1D_ARRAY, textures[5]);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, internalformat, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D_ARRAY, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[6]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(4, textures[4], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(5, textures[5], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(6, textures[6], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_1d"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program, "g_image_buffer"), 4);
		glUniform1i(glGetUniformLocation(program, "g_image_1darray"), 5);
		glUniform1i(glGetUniformLocation(program, "g_image_2darray"), 6);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		std::map<std::string, GLuint> name_index_map;
		GLint uniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniforms);
		if (uniforms != 7)
		{
			status = false;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ACTIVE_UNIFORMS is " << uniforms
												<< " should be 7." << tcu::TestLog::EndMessage;
		}
		for (GLuint index = 0; index < static_cast<GLuint>(uniforms); ++index)
		{
			GLchar name[32];
			glGetActiveUniformName(program, index, sizeof(name), NULL, name);
			name_index_map.insert(std::make_pair(std::string(name), index));
		}

		if (!CheckUniform(program, "g_image_1d", name_index_map, 1, ImageType<T>(GL_TEXTURE_1D)))
			status = false;
		if (!CheckUniform(program, "g_image_2d", name_index_map, 1, ImageType<T>(GL_TEXTURE_2D)))
			status = false;
		if (!CheckUniform(program, "g_image_3d", name_index_map, 1, ImageType<T>(GL_TEXTURE_3D)))
			status = false;
		if (!CheckUniform(program, "g_image_2drect", name_index_map, 1, ImageType<T>(GL_TEXTURE_RECTANGLE)))
			status = false;
		if (!CheckUniform(program, "g_image_buffer", name_index_map, 1, ImageType<T>(GL_TEXTURE_BUFFER)))
			status = false;
		if (!CheckUniform(program, "g_image_1darray", name_index_map, 1, ImageType<T>(GL_TEXTURE_1D_ARRAY)))
			status = false;
		if (!CheckUniform(program, "g_image_2darray", name_index_map, 1, ImageType<T>(GL_TEXTURE_2D_ARRAY)))
			status = false;

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(7, textures);
		glDeleteBuffers(1, &buffer);

		return status;
	}

	template <typename T>
	bool ReadCube(GLenum internalformat, const T& value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program =
			BuildProgram(src_vs, NULL, NULL, NULL, GenFSCube(internalformat, expected_value).c_str());
		GLuint textures[2];
		glGenTextures(2, textures);

		const int	  kSize = 16;
		std::vector<T> data(kSize * kSize * 12, value);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[0]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[1]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalformat, kSize, kSize, 12, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_cube"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_cube_array"), 1);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		std::map<std::string, GLuint> name_index_map;
		GLint uniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniforms);
		if (uniforms != 2)
		{
			status = false;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ACTIVE_UNIFORMS is " << uniforms
												<< " should be 2." << tcu::TestLog::EndMessage;
		}
		for (GLuint index = 0; index < static_cast<GLuint>(uniforms); ++index)
		{
			GLchar name[32];
			glGetActiveUniformName(program, index, sizeof(name), NULL, name);
			name_index_map.insert(std::make_pair(std::string(name), index));
		}

		if (!CheckUniform(program, "g_image_cube", name_index_map, 1, ImageType<T>(GL_TEXTURE_CUBE_MAP)))
			status = false;
		if (!CheckUniform(program, "g_image_cube_array", name_index_map, 1, ImageType<T>(GL_TEXTURE_CUBE_MAP_ARRAY)))
			status = false;

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(2, textures);

		return status;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image1D g_image_1d;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>() << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") readonly uniform " << TypePrefix<T>() << "image3D g_image_3d;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2DRect g_image_2drect;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>() << "imageBuffer g_image_buffer;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") readonly uniform " << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  "
		   << TypePrefix<T>()
		   << "vec4 v;" NL "  v = imageLoad(g_image_1d, coord.x);" NL "  if (v != " << TypePrefix<T>() << "vec4"
		   << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.05);" NL "  v = imageLoad(g_image_2d, coord);" NL "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  v = imageLoad(g_image_3d, ivec3(coord.xy, 0));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.15);" NL "  v = imageLoad(g_image_3d, ivec3(coord.xy, 1));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  v = imageLoad(g_image_2drect, coord);" NL "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.25);" NL "  v = imageLoad(g_image_buffer, coord.x);" NL "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.6);" NL "  v = imageLoad(g_image_1darray, ivec2(coord.x, 0));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.65);" NL "  v = imageLoad(g_image_1darray, ivec2(coord.x, 1));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.7);" NL "  v = imageLoad(g_image_2darray, ivec3(coord, 0));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.75);" NL "  v = imageLoad(g_image_2darray, ivec3(coord, 1));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value << ") o_color = vec4(1.0, 0.0, 0.0, 0.8);" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSCube(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>()
		   << "imageCubeArray g_image_cube_array;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  "
		   << TypePrefix<T>()
		   << "vec4 v;" NL "  v = imageLoad(g_image_cube, ivec3(coord, 0));" NL "  if (v != " << TypePrefix<T>()
		   << "vec4" << expected_value << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL
										  "  v = imageLoad(g_image_cube, ivec3(coord, 1));" NL "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.35);" NL "  v = imageLoad(g_image_cube, ivec3(coord, 2));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.4);" NL "  v = imageLoad(g_image_cube, ivec3(coord, 3));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.45);" NL "  v = imageLoad(g_image_cube, ivec3(coord, 4));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.5);" NL "  v = imageLoad(g_image_cube, ivec3(coord, 5));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.55);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 0));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.05);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 1));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 2));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.15);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 3));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 4));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.25);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 5));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 6));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.35);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 7));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.4);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 8));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.45);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 9));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.5);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 10));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.55);" NL "  v = imageLoad(g_image_cube_array, ivec3(coord, 11));" NL
			  "  if (v != "
		   << TypePrefix<T>() << "vec4" << expected_value << ") o_color = vec4(1.0, 0.0, 0.0, 0.6);" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.2.2 BasicAllTargetsLoadMS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadMS : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedSamples(4))
			return NOT_SUPPORTED;

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!ReadMS(GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f)))
			return ERROR;

		GLint isamples;
		glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &isamples);
		if (isamples >= 4)
		{
			if (!ReadMS(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
				return ERROR;
			if (!ReadMS(GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	bool ReadMS(GLenum internalformat, const T& value, const T& expected_value)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFSMS(internalformat, expected_value).c_str());
		GLuint		 textures[2];
		glGenTextures(2, textures);

		const int kSize = 16;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[0]);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalformat, kSize, kSize, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures[1]);
		glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 4, internalformat, kSize, kSize, 2, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textures[0], 0);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, textures[1], 0, 0);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, textures[1], 0, 1);
		const GLenum draw_buffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, draw_buffers);
		ClearBuffer(GL_COLOR, 0, value);
		ClearBuffer(GL_COLOR, 1, value);
		ClearBuffer(GL_COLOR, 2, value);
		glDeleteFramebuffers(1, &fbo);

		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(4, textures[1], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms_array"), 4);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		std::map<std::string, GLuint> name_index_map;
		GLint uniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniforms);
		if (uniforms != 2)
		{
			status = false;
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "ACTIVE_UNIFORMS is " << uniforms
												<< " should be 2." << tcu::TestLog::EndMessage;
		}
		for (GLuint index = 0; index < static_cast<GLuint>(uniforms); ++index)
		{
			GLchar name[32];
			glGetActiveUniformName(program, index, sizeof(name), NULL, name);
			name_index_map.insert(std::make_pair(std::string(name), index));
		}

		if (!CheckUniform(program, "g_image_2dms", name_index_map, 1, ImageType<T>(GL_TEXTURE_2D_MULTISAMPLE)))
			status = false;
		if (!CheckUniform(program, "g_image_2dms_array", name_index_map, 1,
						  ImageType<T>(GL_TEXTURE_2D_MULTISAMPLE_ARRAY)))
			status = false;

		glDeleteTextures(2, textures);
		glUseProgram(0);
		glDeleteProgram(program);

		return status;
	}

	template <typename T>
	std::string GenFSMS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2DMS g_image_2dms;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>()
		   << "image2DMSArray g_image_2dms_array;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  if (imageLoad(g_image_2dms, coord, 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (imageLoad(g_image_2dms, coord, 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (imageLoad(g_image_2dms, coord, 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (imageLoad(g_image_2dms, coord, 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 0), 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 0), 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 0), 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 0), 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 1), 0) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 1), 1) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 1), 2) != "
		   << TypePrefix<T>() << "vec4" << expected_value
		   << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "  if (imageLoad(g_image_2dms_array, ivec3(coord, 1), 3) != "
		   << TypePrefix<T>() << "vec4" << expected_value << ") o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.3 BasicAllTargetsAtomic
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomic : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Atomic<GLint>(GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(GL_R32UI))
			return ERROR;

		if (!AtomicCube<GLint>(GL_R32I))
			return ERROR;
		if (!AtomicCube<GLuint>(GL_R32UI))
			return ERROR;

		GLint isamples;
		glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &isamples);
		if (SupportedSamples(4) && isamples >= 4)
		{
			if (!AtomicMS<GLint>(GL_R32I))
				return ERROR;
			if (!AtomicMS<GLuint>(GL_R32UI))
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		return NO_ERROR;
	}

	template <typename T>
	bool Atomic(GLenum internalformat)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFS<T>(internalformat).c_str());
		GLuint		 textures[7];
		GLuint		 buffer;
		glGenTextures(7, textures);
		glGenBuffers(1, &buffer);

		const int	  kSize = 16;
		std::vector<T> data(kSize * kSize * 2);

		glBindTexture(GL_TEXTURE_1D, textures[0]);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage1D(GL_TEXTURE_1D, 0, internalformat, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D, 0);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_3D, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE, textures[3]);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		glBindBuffer(GL_TEXTURE_BUFFER, buffer);
		glBufferData(GL_TEXTURE_BUFFER, kSize * sizeof(T), &data[0], GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, textures[4]);
		glTexBuffer(GL_TEXTURE_BUFFER, internalformat, buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glBindTexture(GL_TEXTURE_1D_ARRAY, textures[5]);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, internalformat, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_1D_ARRAY, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[6]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalformat, kSize, kSize, 2, 0, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(4, textures[4], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(5, textures[5], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(6, textures[6], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_1d"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program, "g_image_buffer"), 4);
		glUniform1i(glGetUniformLocation(program, "g_image_1darray"), 5);
		glUniform1i(glGetUniformLocation(program, "g_image_2darray"), 6);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, 1);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, 1, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(7, textures);
		glDeleteBuffers(1, &buffer);

		return status;
	}

	template <typename T>
	bool AtomicCube(GLenum internalformat)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFSCube<T>(internalformat).c_str());
		GLuint		 textures[2];
		glGenTextures(2, textures);

		const int	  kSize = 16;
		std::vector<T> data(kSize * kSize * 12);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[0]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalformat, kSize, kSize, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures[1]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, internalformat, kSize, kSize, 12, 0, Format<T>(), Type<T>(),
					 &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

		glBindImageTexture(0, textures[0], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_cube"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_cube_array"), 1);

		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(2, textures);

		return status;
	}

	template <typename T>
	bool AtomicMS(GLenum internalformat)
	{
		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, NULL, NULL, NULL, GenFSMS<T>(internalformat).c_str());
		GLuint		 textures[2];
		glGenTextures(2, textures);

		const int kSize = 16;

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[0]);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, internalformat, kSize, kSize, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures[1]);
		glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 4, internalformat, kSize, kSize, 2, GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);

		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textures[0], 0);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, textures[1], 0, 0);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, textures[1], 0, 1);
		const GLenum draw_buffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, draw_buffers);
		if (internalformat == GL_R32I)
		{
			const GLint value[4] = { 0, 0, 0, 0 };
			glClearBufferiv(GL_COLOR, 0, value);
			glClearBufferiv(GL_COLOR, 1, value);
			glClearBufferiv(GL_COLOR, 2, value);
		}
		else
		{
			const GLuint value[4] = { 0, 0, 0, 0 };
			glClearBufferuiv(GL_COLOR, 0, value);
			glClearBufferuiv(GL_COLOR, 1, value);
			glClearBufferuiv(GL_COLOR, 2, value);
		}
		glDeleteFramebuffers(1, &fbo);

		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(4, textures[1], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_2dms_array"), 4);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		bool status = true;

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			status = false;
		}

		glDeleteTextures(2, textures);
		glUseProgram(0);
		glDeleteProgram(program);

		return status;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image1D g_image_1d;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>() << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") coherent uniform " << TypePrefix<T>() << "image3D g_image_3d;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image2DRect g_image_2drect;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>() << "imageBuffer g_image_buffer;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") coherent uniform " << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);"

			NL "  if (imageAtomicAdd(g_image_1d, coord.x, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_1d, coord.x, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_1d, coord.x, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_1d, coord.x, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_1d, coord.x, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_1d, coord.x, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_1d, coord.x, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_1d, coord.x, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_1d, coord.x, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_2d, coord, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_2d, coord, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_2d, coord, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_2d, coord, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_2d, coord, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_2d, coord, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2d, coord, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_2d, coord, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2d, coord, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_3d, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_3d, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_3d, ivec3(coord, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_3d, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_3d, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_3d, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_3d, ivec3(coord, 0), 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL
			  "  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_2drect, coord, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_2drect, coord, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_2drect, coord, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_2drect, coord, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_2drect, coord, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_2drect, coord, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2drect, coord, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_2drect, coord, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2drect, coord, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_buffer, coord.x, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_buffer, coord.x, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_buffer, coord.x, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_buffer, coord.x, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_buffer, coord.x, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_buffer, coord.x, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_buffer, coord.x, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_buffer, coord.x, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_buffer, coord.x, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL
			  "  if (imageAtomicAdd(g_image_1darray, ivec2(coord.x, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_1darray, ivec2(coord.x, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_1darray, ivec2(coord.x, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_1darray, ivec2(coord.x, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_1darray, ivec2(coord.x, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_1darray, ivec2(coord.x, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_1darray, ivec2(coord.x, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicCompSwap(g_image_1darray, ivec2(coord.x, 0), 1, 6) != 1) o_color = "
			  "vec4(1.0, 0.0, 0.0, 1.0);" NL "  if (imageAtomicExchange(g_image_1darray, ivec2(coord.x, 0), "
			  "0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_2darray, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_2darray, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_2darray, ivec3(coord, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_2darray, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_2darray, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_2darray, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicCompSwap(g_image_2darray, ivec3(coord, 0), 1, 6) != 1) o_color = vec4(1.0, "
			  "0.0, 0.0, 1.0);" NL "  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), 0) != 6) "
			  "o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSCube(GLenum internalformat)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>()
		   << "imageCube g_image_cube_array;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);"

			NL "  if (imageAtomicAdd(g_image_cube, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_cube, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_cube, ivec3(coord, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_cube, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_cube, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_cube, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_cube, ivec3(coord, 0), 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL
			  "  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  if (imageAtomicAdd(g_image_cube_array, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicMin(g_image_cube_array, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, "
			  "0.0, 1.0);" NL "  if (imageAtomicMax(g_image_cube_array, ivec3(coord, 0), 4) != 2) o_color "
			  "= vec4(1.0, 0.0, 0.0, 1.0);" NL "  if (imageAtomicAnd(g_image_cube_array, "
			  "ivec3(coord, 0), 0) != 4) o_color = "
			  "vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_cube_array, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_cube_array, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicExchange(g_image_cube_array, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, "
			  "0.0, 0.0, 1.0);" NL "  if (imageAtomicCompSwap(g_image_cube_array, ivec3(coord, 0), 1, 6) != "
			  "1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_cube_array, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenFSMS(GLenum internalformat)
	{
		std::ostringstream os;
		os << "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image2DMS g_image_2dms;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>()
		   << "image2DMSArray g_image_2dms_array;" NL "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			  "  if (imageAtomicAdd(g_image_2dms, coord, 1, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMin(g_image_2dms, coord, 1, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicMax(g_image_2dms, coord, 1, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_2dms, coord, 1, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicOr(g_image_2dms, coord, 1, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicXor(g_image_2dms, coord, 1, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2dms, coord, 1, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicCompSwap(g_image_2dms, coord, 1, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2dms, coord, 1, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL NL
			  "  if (imageAtomicAdd(g_image_2dms_array, ivec3(coord, 1), 1, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicMin(g_image_2dms_array, ivec3(coord, 1), 1, 3) != 2) o_color = vec4(1.0, "
			  "0.0, 0.0, 1.0);" NL "  if (imageAtomicMax(g_image_2dms_array, ivec3(coord, 1), 1, 4) != 2) "
			  "o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicAnd(g_image_2dms_array, ivec3(coord, 1), 1, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, "
			  "1.0);" NL "  if (imageAtomicOr(g_image_2dms_array, ivec3(coord, 1), 1, 7) != 0) o_color = vec4(1.0, "
			  "0.0, 0.0, 1.0);" NL "  if (imageAtomicXor(g_image_2dms_array, ivec3(coord, 1), 1, 4) != 7) "
			  "o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  if (imageAtomicExchange(g_image_2dms_array, ivec3(coord, 1), 1, 1) != 3) o_color = vec4(1.0, 0.0, "
			  "0.0, 1.0);" NL "  if (imageAtomicCompSwap(g_image_2dms_array, ivec3(coord, 1), 1, 1, 6) != 1) o_color = "
			  "vec4(1.0, 0.0, 0.0, 1.0);" NL "  if (imageAtomicExchange(g_image_2dms_array, "
			  "ivec3(coord, 1), 1, 0) != 6) o_color = vec4(1.0, 0.0, "
			  "0.0, 1.0);" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// LoadStoreMachine
//-----------------------------------------------------------------------------
class LoadStoreMachine : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	int	m_stage;

	virtual long Setup()
	{
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const GLenum targets[] = { GL_TEXTURE_1D,		GL_TEXTURE_2D,
								   GL_TEXTURE_3D,		GL_TEXTURE_RECTANGLE,
								   GL_TEXTURE_CUBE_MAP, GL_TEXTURE_1D_ARRAY,
								   GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY };
		const int kTargets		= sizeof(targets) / sizeof(targets[0]);
		GLuint	program_store = 0;
		GLuint	program_load  = 0;
		if (m_stage == 0)
		{ // VS
			program_store =
				BuildProgram(GenStoreShader(m_stage, internalformat, write_value).c_str(), NULL, NULL, NULL, NULL);
			program_load =
				BuildProgram(GenLoadShader(m_stage, internalformat, expected_value).c_str(), NULL, NULL, NULL, NULL);
		}
		else if (m_stage == 1)
		{ // TCS
			const char* const glsl_vs  = "#version 420 core" NL "void main() {}";
			const char* const glsl_tes = "#version 420 core" NL "layout(quads, point_mode) in;" NL "void main() {}";
			program_store = BuildProgram(glsl_vs, GenStoreShader(m_stage, internalformat, write_value).c_str(),
										 glsl_tes, NULL, NULL);
			program_load = BuildProgram(glsl_vs, GenLoadShader(m_stage, internalformat, expected_value).c_str(),
										glsl_tes, NULL, NULL);
		}
		else if (m_stage == 2)
		{ // TES
			const char* const glsl_vs = "#version 420 core" NL "void main() {}";
			program_store =
				BuildProgram(glsl_vs, NULL, GenStoreShader(m_stage, internalformat, write_value).c_str(), NULL, NULL);
			program_load =
				BuildProgram(glsl_vs, NULL, GenLoadShader(m_stage, internalformat, expected_value).c_str(), NULL, NULL);
		}
		else if (m_stage == 3)
		{ // GS
			const char* const glsl_vs = "#version 420 core" NL "void main() {}";
			program_store =
				BuildProgram(glsl_vs, NULL, NULL, GenStoreShader(m_stage, internalformat, write_value).c_str(), NULL);
			program_load =
				BuildProgram(glsl_vs, NULL, NULL, GenLoadShader(m_stage, internalformat, expected_value).c_str(), NULL);
		}
		else if (m_stage == 4)
		{ // CS
			{
				std::string		  source = GenStoreShader(m_stage, internalformat, write_value);
				const char* const src	= source.c_str();
				GLuint			  sh	 = glCreateShader(GL_COMPUTE_SHADER);
				glShaderSource(sh, 1, &src, NULL);
				glCompileShader(sh);
				program_store = glCreateProgram();
				glAttachShader(program_store, sh);
				glLinkProgram(program_store);
				glDeleteShader(sh);
			}
			{
				std::string		  source = GenLoadShader(m_stage, internalformat, expected_value);
				const char* const src	= source.c_str();
				GLuint			  sh	 = glCreateShader(GL_COMPUTE_SHADER);
				glShaderSource(sh, 1, &src, NULL);
				glCompileShader(sh);
				program_load = glCreateProgram();
				glAttachShader(program_load, sh);
				glLinkProgram(program_load);
				glDeleteShader(sh);
			}
		}
		GLuint textures[kTargets], texture_result;
		glGenTextures(kTargets, textures);
		glGenTextures(1, &texture_result);

		glBindTexture(GL_TEXTURE_2D, texture_result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1);

		for (int i = 0; i < kTargets; ++i)
		{
			glBindTexture(targets[i], textures[i]);
			glTexParameteri(targets[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(targets[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			if (targets[i] == GL_TEXTURE_1D)
			{
				glTexStorage1D(targets[i], 1, internalformat, 1);
			}
			else if (targets[i] == GL_TEXTURE_2D || targets[i] == GL_TEXTURE_RECTANGLE)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 1);
			}
			else if (targets[i] == GL_TEXTURE_3D || targets[i] == GL_TEXTURE_2D_ARRAY)
			{
				glTexStorage3D(targets[i], 1, internalformat, 1, 1, 2);
			}
			else if (targets[i] == GL_TEXTURE_CUBE_MAP)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 1);
			}
			else if (targets[i] == GL_TEXTURE_CUBE_MAP_ARRAY)
			{
				glTexStorage3D(targets[i], 1, internalformat, 1, 1, 12);
			}
			else if (targets[i] == GL_TEXTURE_1D_ARRAY)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 2);
			}
		}
		glBindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(4, textures[4], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(5, textures[5], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(6, textures[6], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);
		glBindImageTexture(7, textures[7], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glUseProgram(program_store);
		glUniform1i(glGetUniformLocation(program_store, "g_image_1d"), 0);
		glUniform1i(glGetUniformLocation(program_store, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program_store, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program_store, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program_store, "g_image_cube"), 4);
		glUniform1i(glGetUniformLocation(program_store, "g_image_1darray"), 5);
		glUniform1i(glGetUniformLocation(program_store, "g_image_2darray"), 6);
		glUniform1i(glGetUniformLocation(program_store, "g_image_cube_array"), 7);

		glBindVertexArray(m_vao);
		if (m_stage == 1 || m_stage == 2)
		{ // TCS or TES
			glPatchParameteri(GL_PATCH_VERTICES, 1);
			glDrawArrays(GL_PATCHES, 0, 1);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
		}
		else if (m_stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		else
		{
			glDrawArrays(GL_POINTS, 0, 1);
		}

		bool status = true;
		for (int i = 0; i < kTargets; ++i)
		{
			glBindTexture(targets[i], textures[i]);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

			if (targets[i] == GL_TEXTURE_CUBE_MAP)
			{
				for (int face = 0; face < 6; ++face)
				{
					T data;
					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, Format<T>(), Type<T>(), &data[0]);
					if (!Equal(data, expected_value, internalformat))
					{
						status = false;
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Value is: " << ToString(data)
							<< ". Value should be: " << ToString(expected_value)
							<< ". Format is: " << FormatEnumToString(internalformat)
							<< ". Target is: " << EnumToString(targets[i]) << ". Stage is: " << StageName(m_stage)
							<< tcu::TestLog::EndMessage;
					}
				}
			}
			else
			{
				T data[12];
				memset(&data[0], 0, sizeof(data));
				glGetTexImage(targets[i], 0, Format<T>(), Type<T>(), &data[0]);

				int count = 1;
				if (targets[i] == GL_TEXTURE_3D || targets[i] == GL_TEXTURE_2D_ARRAY)
					count = 2;
				else if (targets[i] == GL_TEXTURE_CUBE_MAP_ARRAY)
					count = 12;
				else if (targets[i] == GL_TEXTURE_1D_ARRAY)
					count = 2;

				for (int j = 0; j < count; ++j)
				{
					if (!Equal(data[j], expected_value, internalformat))
					{
						status = false;
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Value is: " << ToString(data[j])
							<< ". Value should be: " << ToString(expected_value)
							<< ". Format is: " << FormatEnumToString(internalformat)
							<< ". Target is: " << EnumToString(targets[i]) << ". Stage is: " << StageName(m_stage)
							<< tcu::TestLog::EndMessage;
					}
				}
			}
		}
		glBindImageTexture(0, texture_result, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(4, textures[4], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(5, textures[5], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(6, textures[6], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(7, textures[7], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);

		glUseProgram(program_load);
		glUniform1i(glGetUniformLocation(program_load, "g_image_result"), 0);
		glUniform1i(glGetUniformLocation(program_load, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program_load, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program_load, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program_load, "g_image_cube"), 4);
		glUniform1i(glGetUniformLocation(program_load, "g_image_1darray"), 5);
		glUniform1i(glGetUniformLocation(program_load, "g_image_2darray"), 6);
		glUniform1i(glGetUniformLocation(program_load, "g_image_cube_array"), 7);

		if (m_stage == 1 || m_stage == 2)
		{ // TCS or TES
			glPatchParameteri(GL_PATCH_VERTICES, 1);
			glDrawArrays(GL_PATCHES, 0, 1);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
		}
		else if (m_stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		else
		{
			glDrawArrays(GL_POINTS, 0, 1);
		}
		{
			vec4 color;
			glBindTexture(GL_TEXTURE_2D, texture_result);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &color[0]);
			if (!tcu::allEqual(color, vec4(0, 1, 0, 1)))
			{
				status = false;
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Color is: " << ToString(color)
													<< ". Format is: " << FormatEnumToString(internalformat)
													<< ". Stage is: " << StageName(m_stage) << tcu::TestLog::EndMessage;
			}
		}
		glUseProgram(0);
		glDeleteProgram(program_store);
		glDeleteProgram(program_load);
		glDeleteTextures(kTargets, textures);
		glDeleteTextures(1, &texture_result);
		return status;
	}

	template <typename T>
	std::string GenStoreShader(int stage, GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		os << "#version 420 core";
		if (stage == 4)
		{ // CS
			os << NL "#extension GL_ARB_compute_shader : require";
		}
		os << NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image1D g_image_1d;" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>() << "image3D g_image_3d;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DRect g_image_2drect;" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>() << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout("
		   << FormatEnumToString(internalformat) << ") writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") writeonly uniform " << TypePrefix<T>() << "imageCubeArray g_image_cube_array;" NL "uniform "
		   << TypePrefix<T>() << "vec4 g_value = " << TypePrefix<T>() << "vec4" << write_value
		   << ";" NL "uniform int g_index[6] = int[](0, 1, 2, 3, 4, 5);";
		if (stage == 0)
		{ // VS
			os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_VertexID, g_index[0]);";
		}
		else if (stage == 1)
		{ // TCS
			os << NL "layout(vertices = 1) out;" NL "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL
					 "  gl_TessLevelInner[1] = 1;" NL "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL
					 "  gl_TessLevelOuter[2] = 1;" NL "  gl_TessLevelOuter[3] = 1;" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_index[0]);";
		}
		else if (stage == 2)
		{ // TES
			os << NL "layout(quads, point_mode) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_index[0]);";
		}
		else if (stage == 3)
		{ // GS
			os << NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveIDIn, g_index[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = 1) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_index[0]);";
		}
		os << NL "  imageStore(g_image_1d, coord.x, g_value);" NL "  imageStore(g_image_2d, coord, g_value);" NL
				 "  imageStore(g_image_3d, ivec3(coord.xy, g_index[0]), g_value);" NL
				 "  imageStore(g_image_3d, ivec3(coord.xy, g_index[1]), g_value);" NL
				 "  imageStore(g_image_2drect, coord, g_value);" NL "  for (int i = 0; i < 6; ++i) {" NL
				 "    imageStore(g_image_cube, ivec3(coord, g_index[i]), g_value);" NL "  }" NL
				 "  imageStore(g_image_1darray, ivec2(coord.x, g_index[0]), g_value);" NL
				 "  imageStore(g_image_1darray, ivec2(coord.x, g_index[1]), g_value);" NL
				 "  imageStore(g_image_2darray, ivec3(coord, g_index[0]), g_value);" NL
				 "  imageStore(g_image_2darray, ivec3(coord, g_index[1]), g_value);" NL
				 "  for (int i = 0; i < 6; ++i) {" NL
				 "    imageStore(g_image_cube_array, ivec3(coord, g_index[i]), g_value);" NL "  }" NL
				 "  for (int i = 0; i < 6; ++i) {" NL
				 "    imageStore(g_image_cube_array, ivec3(coord, g_index[i] + 6), g_value);" NL "  }" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenLoadShader(int stage, GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << "#version 420 core";
		if (stage == 4)
		{ // CS
			os << NL "#extension GL_ARB_compute_shader : require";
		}
		os << NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>() << "image3D g_image_3d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") readonly uniform " << TypePrefix<T>() << "image2DRect g_image_2drect;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat) << ") readonly uniform "
		   << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") readonly uniform " << TypePrefix<T>() << "image2DArray g_image_2darray;" NL "layout("
		   << FormatEnumToString(internalformat) << ") readonly uniform " << TypePrefix<T>()
		   << "imageCubeArray g_image_cube_array;" NL "layout(rgba32f) writeonly uniform image2D g_image_result;" NL
			  "uniform "
		   << TypePrefix<T>() << "vec4 g_value = " << TypePrefix<T>() << "vec4" << expected_value
		   << ";" NL "uniform int g_index[6] = int[](0, 1, 2, 3, 4, 5);";
		if (stage == 0)
		{ // VS
			os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_VertexID, g_index[0]);";
		}
		else if (stage == 1)
		{ // TCS
			os << NL "layout(vertices = 1) out;" NL "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL
					 "  gl_TessLevelInner[1] = 1;" NL "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL
					 "  gl_TessLevelOuter[2] = 1;" NL "  gl_TessLevelOuter[3] = 1;" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_index[0]);";
		}
		else if (stage == 2)
		{ // TES
			os << NL "layout(quads, point_mode) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_index[0]);";
		}
		else if (stage == 3)
		{ // GS
			os << NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveIDIn, g_index[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = 1) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_index[0]);";
		}
		os << NL "  vec4 r = vec4(0, 1, 0, 1);" NL "  " << TypePrefix<T>()
		   << "vec4 v;" NL "  v = imageLoad(g_image_2d, coord);" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  v = imageLoad(g_image_3d, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL "  v = imageLoad(g_image_2drect, coord);" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  v = imageLoad(g_image_cube, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL "  v = imageLoad(g_image_1darray, coord);" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  v = imageLoad(g_image_2darray, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL
			  "  v = imageLoad(g_image_cube_array, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, 0.0, 1.0);" NL "  imageStore(g_image_result, coord, r);" NL "}";
		return os.str();
	}

protected:
	long RunStage(int stage)
	{
		if (!SupportedInStage(stage, 8))
			return NOT_SUPPORTED;

		glEnable(GL_RASTERIZER_DISCARD);
		m_stage = stage;

		if (!Write(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_RG16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_R11F_G11F_B10F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R16I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RG8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 0, 1)))
			return ERROR;
		if (!Write(GL_R8I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RGB10_A2UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R16UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RG8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 0, 1)))
			return ERROR;
		if (!Write(GL_R8UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;

		if (!Write(GL_RGBA16, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RGB10_A2, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG16, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R16, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;
		if (!Write(GL_RG8, vec4(1.0f), vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_R8, vec4(1.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;

		//
		{
			if (!Write(GL_RGBA16_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
				return ERROR;
			if (!Write(GL_RG16_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
				return ERROR;
			if (!Write(GL_R16_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
				return ERROR;

			if (!Write(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
				return ERROR;
			if (!Write(GL_RG8_SNORM, vec4(-1.0f), vec4(-1.0f, -1.0f, 0.0f, 1.0f)))
				return ERROR;
			if (!Write(GL_R8_SNORM, vec4(-1.0f, 1.0f, -1.0f, 1.0f), vec4(-1.0f, 0.0f, 0.0f, 1.0f)))
				return ERROR;
		}
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// AtomicMachine
//-----------------------------------------------------------------------------
class AtomicMachine : public ShaderImageLoadStoreBase
{
	GLuint m_vao;

	virtual long Setup()
	{
		glEnable(GL_RASTERIZER_DISCARD);
		glGenVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}

	template <typename T>
	bool Atomic(int stage, GLenum internalformat)
	{
		GLuint program = 0;
		if (stage == 0)
		{ // VS
			program = BuildProgram(GenShader<T>(stage, internalformat).c_str(), NULL, NULL, NULL, NULL);
		}
		else if (stage == 1)
		{ // TCS
			const char* const glsl_vs  = "#version 420 core" NL "void main() {}";
			const char* const glsl_tes = "#version 420 core" NL "layout(quads, point_mode) in;" NL "void main() {}";
			program = BuildProgram(glsl_vs, GenShader<T>(stage, internalformat).c_str(), glsl_tes, NULL, NULL);
		}
		else if (stage == 2)
		{ // TES
			const char* const glsl_vs = "#version 420 core" NL "void main() {}";
			program = BuildProgram(glsl_vs, NULL, GenShader<T>(stage, internalformat).c_str(), NULL, NULL);
		}
		else if (stage == 3)
		{ // GS
			const char* const glsl_vs = "#version 420 core" NL "void main() {}";
			program = BuildProgram(glsl_vs, NULL, NULL, GenShader<T>(stage, internalformat).c_str(), NULL);
		}
		else if (stage == 4)
		{ // CS
			std::string		  source = GenShader<T>(stage, internalformat);
			const char* const src	= source.c_str();
			GLuint			  sh	 = glCreateShader(GL_COMPUTE_SHADER);
			glShaderSource(sh, 1, &src, NULL);
			glCompileShader(sh);
			program = glCreateProgram();
			glAttachShader(program, sh);
			glLinkProgram(program);
			glDeleteShader(sh);
		}
		GLuint texture_result;
		glGenTextures(1, &texture_result);
		glBindTexture(GL_TEXTURE_2D, texture_result);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1);

		const GLenum targets[] = { GL_TEXTURE_2D,	 GL_TEXTURE_3D,	   GL_TEXTURE_RECTANGLE, GL_TEXTURE_CUBE_MAP,
								   GL_TEXTURE_BUFFER, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_ARRAY };
		const int kTargets = sizeof(targets) / sizeof(targets[0]);

		GLuint textures[kTargets];
		GLuint buffer;
		glGenTextures(kTargets, textures);
		glGenBuffers(1, &buffer);

		for (int i = 0; i < kTargets; ++i)
		{
			glBindTexture(targets[i], textures[i]);
			if (targets[i] != GL_TEXTURE_BUFFER)
			{
				glTexParameteri(targets[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(targets[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			if (targets[i] == GL_TEXTURE_2D || targets[i] == GL_TEXTURE_RECTANGLE)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 1);
			}
			else if (targets[i] == GL_TEXTURE_3D || targets[i] == GL_TEXTURE_2D_ARRAY)
			{
				glTexStorage3D(targets[i], 1, internalformat, 1, 1, 2);
			}
			else if (targets[i] == GL_TEXTURE_CUBE_MAP)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 1);
			}
			else if (targets[i] == GL_TEXTURE_BUFFER)
			{
				glBindBuffer(GL_TEXTURE_BUFFER, buffer);
				glBufferData(GL_TEXTURE_BUFFER, 4, NULL, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_TEXTURE_BUFFER, 0);
				glTexBuffer(targets[i], internalformat, buffer);
			}
			else if (targets[i] == GL_TEXTURE_1D_ARRAY)
			{
				glTexStorage2D(targets[i], 1, internalformat, 1, 2);
			}
		}
		glBindImageTexture(0, texture_result, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(2, textures[1], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(3, textures[2], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(4, textures[3], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(5, textures[4], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(6, textures[5], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);
		glBindImageTexture(7, textures[6], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);

		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "g_image_result"), 0);
		glUniform1i(glGetUniformLocation(program, "g_image_2d"), 1);
		glUniform1i(glGetUniformLocation(program, "g_image_3d"), 2);
		glUniform1i(glGetUniformLocation(program, "g_image_2drect"), 3);
		glUniform1i(glGetUniformLocation(program, "g_image_cube"), 4);
		glUniform1i(glGetUniformLocation(program, "g_image_buffer"), 5);
		glUniform1i(glGetUniformLocation(program, "g_image_1darray"), 6);
		glUniform1i(glGetUniformLocation(program, "g_image_2darray"), 7);

		glBindVertexArray(m_vao);
		if (stage == 1 || stage == 2)
		{ // TCS or TES
			glPatchParameteri(GL_PATCH_VERTICES, 1);
			glDrawArrays(GL_PATCHES, 0, 1);
			glPatchParameteri(GL_PATCH_VERTICES, 3);
		}
		else if (stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		else
		{
			glDrawArrays(GL_POINTS, 0, 1);
		}

		bool status = true;
		{
			vec4 color;
			glBindTexture(GL_TEXTURE_2D, texture_result);
			glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &color[0]);
			if (!tcu::allEqual(color, vec4(0, 1, 0, 1)))
			{
				status = false;
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Color is: " << ToString(color)
													<< ". Format is: " << FormatEnumToString(internalformat)
													<< ". Stage is: " << StageName(stage) << tcu::TestLog::EndMessage;
			}
		}
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(7, textures);
		glDeleteTextures(1, &texture_result);
		glDeleteBuffers(1, &buffer);
		return status;
	}

	template <typename T>
	std::string GenShader(int stage, GLenum internalformat)
	{
		std::ostringstream os;
		os << "#version 420 core";
		if (stage == 4)
		{ // CS
			os << NL "#extension GL_ARB_compute_shader : require";
		}
		os << NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>() << "image3D g_image_3d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") coherent uniform " << TypePrefix<T>() << "image2DRect g_image_2drect;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat) << ") coherent uniform "
		   << TypePrefix<T>() << "imageBuffer g_image_buffer;" NL "layout(" << FormatEnumToString(internalformat)
		   << ") coherent uniform " << TypePrefix<T>() << "image1DArray g_image_1darray;" NL "layout("
		   << FormatEnumToString(internalformat) << ") coherent uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "layout(rgba32f) writeonly uniform image2D g_image_result;" NL
			  "uniform int g_value[6] = int[](0, 1, 2, 3, 4, 5);";
		if (stage == 0)
		{ // VS
			os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_VertexID, g_value[0]);";
		}
		else if (stage == 1)
		{ // TCS
			os << NL "layout(vertices = 1) out;" NL "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL
					 "  gl_TessLevelInner[1] = 1;" NL "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL
					 "  gl_TessLevelOuter[2] = 1;" NL "  gl_TessLevelOuter[3] = 1;" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_value[0]);";
		}
		else if (stage == 2)
		{ // TES
			os << NL "layout(quads, point_mode) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveID, g_value[0]);";
		}
		else if (stage == 3)
		{ // GS
			os << NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_PrimitiveIDIn, g_value[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = 1) in;" NL "void main() {" NL
					 "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_value[0]);";
		}
		os << NL
			"  vec4 o_color = vec4(0, 1, 0, 1);" NL "  imageAtomicExchange(g_image_2d, coord, 0);" NL
			"  if (imageAtomicAdd(g_image_2d, coord, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_2d, coord, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_2d, coord, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_2d, coord, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_2d, coord, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_2d, coord, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_2d, coord, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicCompSwap(g_image_2d, coord, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_2d, coord, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_3d, ivec3(coord, 0), 0);" NL
			"  if (imageAtomicAdd(g_image_3d, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_3d, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_3d, ivec3(coord, g_value[0]), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_3d, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_3d, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_3d, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicCompSwap(g_image_3d, ivec3(coord, 0), 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_2drect, coord, 0);" NL
			"  if (imageAtomicAdd(g_image_2drect, coord, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_2drect, coord, 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_2drect, coord, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_2drect, coord, 0) != g_value[4]) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_2drect, coord, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_2drect, coord, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_2drect, coord, 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicCompSwap(g_image_2drect, coord, g_value[1], 6) != 1) o_color = vec4(1.0, 0.0, 0.0, "
			"1.0);" NL "  if (imageAtomicExchange(g_image_2drect, coord, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_cube, ivec3(coord, 0), 0);" NL "  if (imageAtomicAdd(g_image_cube, "
			"ivec3(coord, 0), g_value[2]) != 0) "
			"o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_cube, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_cube, ivec3(coord, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_cube, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_cube, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_cube, ivec3(coord, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicCompSwap(g_image_cube, ivec3(coord, g_value[0]), 1, 6) != 1) o_color = vec4(1.0, 0.0, "
			"0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_buffer, coord.x, g_value[0]);" NL
			"  if (imageAtomicAdd(g_image_buffer, coord.x, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_buffer, coord.x, g_value[3]) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_buffer, coord.x, 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_buffer, coord.x, 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_buffer, coord.x, 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_buffer, coord.x, 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_buffer, coord.x, g_value[1]) != 3) o_color = vec4(1.0, 0.0, 0.0, "
			"1.0);" NL
			"  if (imageAtomicCompSwap(g_image_buffer, coord.x, 1, 6) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_buffer, coord.x, 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_1darray, ivec2(coord.x, 0), 0);" NL
			"  if (imageAtomicAdd(g_image_1darray, ivec2(coord.x, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_1darray, ivec2(coord.x, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_1darray, ivec2(coord.x, g_value[0]), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, "
			"1.0);" NL
			"  if (imageAtomicAnd(g_image_1darray, ivec2(coord.x, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_1darray, ivec2(coord.x, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_1darray, ivec2(coord.x, 0), 4) != 7) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_1darray, ivec2(coord.x, 0), 1) != 3) o_color = vec4(1.0, 0.0, 0.0, "
			"1.0);" NL "  if (imageAtomicCompSwap(g_image_1darray, ivec2(coord.x, 0), 1, 6) != 1) o_color = vec4(1.0, "
			"0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_1darray, ivec2(coord.x, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageAtomicExchange(g_image_2darray, ivec3(coord, 0), 0);" NL
			"  if (imageAtomicAdd(g_image_2darray, ivec3(coord, 0), 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMin(g_image_2darray, ivec3(coord, 0), 3) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicMax(g_image_2darray, ivec3(coord, 0), 4) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAnd(g_image_2darray, ivec3(coord, 0), 0) != 4) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicOr(g_image_2darray, ivec3(coord, 0), 7) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicXor(g_image_2darray, ivec3(coord, 0), g_value[4]) != 7) o_color = vec4(1.0, 0.0, 0.0, "
			"1.0);" NL "  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), 1) != 3) o_color = vec4(1.0, 0.0, "
			"0.0, 1.0);" NL "  if (imageAtomicCompSwap(g_image_2darray, ivec3(coord, 0), 1, 6) != 1) "
			"o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), 0) != 6) o_color = vec4(1.0, 0.0, 0.0, 1.0);"

			NL "  imageStore(g_image_result, coord, o_color);" NL "}";
		return os.str();
	}

protected:
	long RunStage(int stage)
	{
		if (!SupportedInStage(stage, 8))
			return NOT_SUPPORTED;
		if (!Atomic<GLint>(stage, GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(stage, GL_R32UI))
			return ERROR;
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.3.4 BasicAllTargetsLoadStoreVS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreVS : public LoadStoreMachine
{
	virtual long Run()
	{
		return RunStage(0);
	}
};
//-----------------------------------------------------------------------------
// 1.3.5 BasicAllTargetsLoadStoreTCS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreTCS : public LoadStoreMachine
{
	virtual long Run()
	{
		return RunStage(1);
	}
};
//-----------------------------------------------------------------------------
// 1.3.6 BasicAllTargetsLoadStoreTES
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreTES : public LoadStoreMachine
{
	virtual long Run()
	{
		return RunStage(2);
	}
};
//-----------------------------------------------------------------------------
// 1.3.7 BasicAllTargetsLoadStoreGS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreGS : public LoadStoreMachine
{
	virtual long Run()
	{
		return RunStage(3);
	}
};
//-----------------------------------------------------------------------------
// 1.3.8 BasicAllTargetsLoadStoreCS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreCS : public LoadStoreMachine
{
	virtual long Run()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_ARB_compute_shader not supported, skipping test"
				<< tcu::TestLog::EndMessage;
			return NO_ERROR;
		}

		return RunStage(4);
	}
};
//-----------------------------------------------------------------------------
// 1.3.9 BasicAllTargetsAtomicVS
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicVS : public AtomicMachine
{
	virtual long Run()
	{
		return RunStage(0);
	}
};
//-----------------------------------------------------------------------------
// 1.3.10 BasicAllTargetsAtomicTCS
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicTCS : public AtomicMachine
{
	virtual long Run()
	{
		return RunStage(1);
	}
};
//-----------------------------------------------------------------------------
// 1.3.11 BasicAllTargetsAtomicTES
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicTES : public AtomicMachine
{
	virtual long Run()
	{
		return RunStage(2);
	}
};
//-----------------------------------------------------------------------------
// 1.3.12 BasicAllTargetsAtomicGS
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicGS : public AtomicMachine
{
	virtual long Run()
	{
		return RunStage(3);
	}
};
//-----------------------------------------------------------------------------
// 1.3.13 BasicAllTargetsAtomicCS
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicCS : public AtomicMachine
{
	virtual long Run()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_ARB_compute_shader not supported, skipping test"
				<< tcu::TestLog::EndMessage;
			return NO_ERROR;
		}

		return RunStage(4);
	}
};
//-----------------------------------------------------------------------------
// 1.4.1 BasicGLSLMisc
//-----------------------------------------------------------------------------
class BasicGLSLMisc : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_program;
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		m_texture = 0;
		m_program = 0;
		m_vao = m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int		  kSize = 32;
		std::vector<vec4> data(kSize * kSize * 4, vec4(0.0f));

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, kSize, kSize, 4, 0, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		const char* src_fs =
			"#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) coherent volatile restrict uniform image2D g_image_layer0;" NL
			"layout(rgba32f) volatile uniform image2D g_image_layer1;" NL
			"void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			"  imageStore(g_image_layer0, coord, vec4(1.0));" NL "  memoryBarrier();" NL
			"  imageStore(g_image_layer0, coord, vec4(2.0));" NL "  memoryBarrier();" NL
			"  imageStore(g_image_layer0, coord, vec4(0.0, 1.0, 0.0, 1.0));" NL "  memoryBarrier();" NL
			"  o_color = imageLoad(g_image_layer0, coord) + imageLoad(g_image_layer1, coord);" NL "}";
		m_program = BuildProgram(src_vs, NULL, NULL, NULL, src_fs);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 1, GL_READ_ONLY, GL_RGBA32F);

		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, kSize, kSize);

		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_image_layer0"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image_layer1"), 1);

		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, kSize, kSize, vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteTextures(1, &m_texture);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		glUseProgram(0);
		glDeleteProgram(m_program);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.4.2 BasicGLSLEarlyFragTests
//-----------------------------------------------------------------------------
class BasicGLSLEarlyFragTests : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program[2];
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		m_texture[0] = m_texture[1] = 0;
		m_program[0] = m_program[1] = 0;
		m_vao = m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		int ds = m_context.getRenderContext().getRenderTarget().getDepthBits();

		const int		  kSize = 32;
		std::vector<vec4> data(kSize * kSize);

		glGenTextures(2, m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kSize, kSize, 0, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kSize, kSize, 0, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		const char* glsl_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							  "  gl_Position = i_position;" NL "}";
		const char* glsl_early_frag_tests_fs =
			"#version 420 core" NL "layout(early_fragment_tests) in;" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) coherent uniform image2D g_image;" NL "void main() {" NL
			"  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image, coord, vec4(1.0));" NL
			"  o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		const char* glsl_fs = "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
							  "layout(rgba32f) coherent uniform image2D g_image;" NL "void main() {" NL
							  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image, coord, vec4(1.0));" NL
							  "  o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		m_program[0] = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_early_frag_tests_fs);
		m_program[1] = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glViewport(0, 0, kSize, kSize);
		glBindVertexArray(m_vao);

		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0, 1.0f, 0.0, 1.0f);
		glClearDepthf(0.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_program[0]);
		glUniform1i(glGetUniformLocation(m_program[0], "g_image"), 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glUniform1i(glGetUniformLocation(m_program[1], "g_image"), 1);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data[0]);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (IsEqual(data[i], vec4(1.0f)) && ds != 0)
				return ERROR;
		}

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data[0]);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!IsEqual(data[i], vec4(1.0f)) && ds != 0)
				return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepthf(1.0f);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteTextures(2, m_texture);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		glUseProgram(0);
		glDeleteProgram(m_program[0]);
		glDeleteProgram(m_program[1]);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.4.3 BasicGLSLConst
//-----------------------------------------------------------------------------
class BasicGLSLConst : public ShaderImageLoadStoreBase
{
	GLuint m_program;
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		m_program = 0;
		m_vao = m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool isAtLeast44Context =
			glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 4));

		const char* src_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							 "  gl_Position = i_position;" NL "}";
		std::ostringstream src_fs;
		src_fs << "#version " << (isAtLeast44Context ? "440" : "420") << " core";
		src_fs << NL "layout(location = 0) out vec4 o_color;" NL "uniform int MaxImageUnits;" NL
					 "uniform int MaxCombinedShaderOutputResources;" NL "uniform int MaxImageSamples;" NL
					 "uniform int MaxVertexImageUniforms;" NL "uniform int MaxTessControlImageUniforms;" NL
					 "uniform int MaxTessEvaluationImageUniforms;" NL "uniform int MaxGeometryImageUniforms;" NL
					 "uniform int MaxFragmentImageUniforms;" NL "uniform int MaxCombinedImageUniforms;" NL
					 "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
					 "  if (gl_MaxImageUnits != MaxImageUnits) o_color = vec4(1.0, 0.0, 0.0, 0.1);";
		if (isAtLeast44Context)
			src_fs << NL "  if (gl_MaxCombinedShaderOutputResources != MaxCombinedShaderOutputResources) o_color = "
						 "vec4(0.2, 0.0, 0.0, 0.2);";
		else
			src_fs << NL "  if (gl_MaxCombinedImageUnitsAndFragmentOutputs != MaxCombinedShaderOutputResources) "
						 "o_color = vec4(0.2, 0.0, 0.0, 0.2);";
		src_fs << NL
			"  if (gl_MaxImageSamples != MaxImageSamples) o_color = vec4(1.0, 0.0, 0.0, 0.3);" NL
			"  if (gl_MaxVertexImageUniforms != MaxVertexImageUniforms) o_color = vec4(1.0, 0.0, 0.0, 0.4);" NL
			"  if (gl_MaxTessControlImageUniforms != MaxTessControlImageUniforms) o_color = vec4(1.0, 0.0, 0.0, "
			"0.5);" NL "  if (gl_MaxTessEvaluationImageUniforms != MaxTessEvaluationImageUniforms) o_color = vec4(1.0, "
			"0.0, 0.0, 0.6);" NL
			"  if (gl_MaxGeometryImageUniforms != MaxGeometryImageUniforms) o_color = vec4(1.0, 0.0, 0.0, 0.7);" NL
			"  if (gl_MaxFragmentImageUniforms != MaxFragmentImageUniforms) o_color = vec4(1.0, 0.0, 0.0, 0.8);" NL
			"  if (gl_MaxCombinedImageUniforms != MaxCombinedImageUniforms) o_color = vec4(1.0, 0.0, 0.0, 0.9);" NL "}";

		m_program = BuildProgram(src_vs, NULL, NULL, NULL, src_fs.str().c_str());
		glUseProgram(m_program);

		GLint i;
		glGetIntegerv(GL_MAX_IMAGE_UNITS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxImageUnits"), i);

		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxCombinedShaderOutputResources"), i);

		glGetIntegerv(GL_MAX_IMAGE_SAMPLES, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxImageSamples"), i);

		glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxVertexImageUniforms"), i);

		glGetIntegerv(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxTessControlImageUniforms"), i);

		glGetIntegerv(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxTessEvaluationImageUniforms"), i);

		glGetIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxGeometryImageUniforms"), i);

		glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxFragmentImageUniforms"), i);

		glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxCombinedImageUniforms"), i);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		glUseProgram(0);
		glDeleteProgram(m_program);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.1.1 AdvancedSyncImageAccess
//-----------------------------------------------------------------------------
class AdvancedSyncImageAccess : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_buffer_tex;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_attribless_vao;

	virtual long Setup()
	{
		m_buffer		 = 0;
		m_buffer_tex	 = 0;
		m_store_program  = 0;
		m_draw_program   = 0;
		m_attribless_vao = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		const char* const glsl_store_vs =
			"#version 420 core" NL "writeonly uniform imageBuffer g_output_data;" NL "void main() {" NL
			"  vec2[4] data = vec2[4](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));" NL
			"  imageStore(g_output_data, gl_VertexID, vec4(data[gl_VertexID], 0.0, 1.0));" NL "}";
		const char* const glsl_draw_vs =
			"#version 420 core" NL "out vec4 vs_color;" NL "layout(rg32f) readonly uniform imageBuffer g_image;" NL
			"uniform samplerBuffer g_sampler;" NL "void main() {" NL "  vec4 pi = imageLoad(g_image, gl_VertexID);" NL
			"  vec4 ps = texelFetch(g_sampler, gl_VertexID);" NL
			"  if (pi != ps) vs_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "  else vs_color = vec4(0.0, 1.0, 0.0, 1.0);" NL
			"  gl_Position = pi;" NL "}";
		const char* const glsl_draw_fs =
			"#version 420 core" NL "in vec4 vs_color;" NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
			"  o_color = vs_color;" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, NULL, NULL, NULL, NULL);
		m_draw_program  = BuildProgram(glsl_draw_vs, NULL, NULL, NULL, glsl_draw_fs);

		glGenVertexArrays(1, &m_attribless_vao);
		glBindVertexArray(m_attribless_vao);

		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec2) * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, m_buffer);

		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RG32F);

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_store_program);
		glDrawArrays(GL_POINTS, 0, 4);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		glDisable(GL_RASTERIZER_DISCARD);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.1.2 AdvancedSyncVertexArray
//-----------------------------------------------------------------------------
class AdvancedSyncVertexArray : public ShaderImageLoadStoreBase
{
	GLuint m_position_buffer;
	GLuint m_color_buffer;
	GLuint m_element_buffer;
	GLuint m_position_buffer_tex;
	GLuint m_color_buffer_tex;
	GLuint m_element_buffer_tex;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_attribless_vao;
	GLuint m_draw_vao;

	virtual long Setup()
	{
		m_position_buffer	 = 0;
		m_color_buffer		  = 0;
		m_element_buffer	  = 0;
		m_position_buffer_tex = 0;
		m_color_buffer_tex	= 0;
		m_element_buffer_tex  = 0;
		m_store_program		  = 0;
		m_draw_program		  = 0;
		m_attribless_vao	  = 0;
		m_draw_vao			  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(3))
			return NOT_SUPPORTED;
		const char* const glsl_store_vs =
			"#version 420 core" NL "layout(rg32f) writeonly uniform imageBuffer g_position_buffer;" NL
			"layout(rgba32f) writeonly uniform imageBuffer g_color_buffer;" NL
			"layout(r32ui) writeonly uniform uimageBuffer g_element_buffer;" NL "uniform vec4 g_color;" NL
			"void main() {" NL "  vec2[4] data = vec2[4](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));" NL
			"  imageStore(g_position_buffer, gl_VertexID, vec4(data[gl_VertexID], 0.0, 1.0));" NL
			"  imageStore(g_color_buffer, gl_VertexID, g_color);" NL
			"  imageStore(g_element_buffer, gl_VertexID, uvec4(gl_VertexID));" NL "}";
		const char* const glsl_draw_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
			"layout(location = 1) in vec4 i_color;" NL "out vec4 vs_color;" NL "void main() {" NL
			"  gl_Position = i_position;" NL "  vs_color = i_color;" NL "}";
		const char* const glsl_draw_fs =
			"#version 420 core" NL "in vec4 vs_color;" NL "layout(location = 0) out vec4 o_color;" NL "void main() {" NL
			"  o_color = vs_color;" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, NULL, NULL, NULL, NULL);
		glUseProgram(m_store_program);
		glUniform1i(glGetUniformLocation(m_store_program, "g_position_buffer"), 0);
		glUniform1i(glGetUniformLocation(m_store_program, "g_color_buffer"), 1);
		glUniform1i(glGetUniformLocation(m_store_program, "g_element_buffer"), 2);
		glUseProgram(0);

		m_draw_program = BuildProgram(glsl_draw_vs, NULL, NULL, NULL, glsl_draw_fs);

		glGenBuffers(1, &m_position_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_position_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec2) * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenBuffers(1, &m_color_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_color_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenBuffers(1, &m_element_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_element_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_position_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_position_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, m_position_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_color_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_color_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_color_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_element_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_element_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, m_element_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glGenVertexArrays(1, &m_attribless_vao);

		glGenVertexArrays(1, &m_draw_vao);
		glBindVertexArray(m_draw_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_position_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, m_color_buffer);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_buffer);
		glBindVertexArray(0);

		glEnable(GL_RASTERIZER_DISCARD);
		glBindImageTexture(0, m_position_buffer_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
		glBindImageTexture(1, m_color_buffer_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(2, m_element_buffer_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 1.0f, 0.0f, 1.0f);
		glBindVertexArray(m_attribless_vao);
		glDrawArrays(GL_POINTS, 0, 4);

		glDisable(GL_RASTERIZER_DISCARD);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_draw_vao);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 0.0f, 1.0f, 1.0f);
		glBindVertexArray(m_attribless_vao);
		glDrawArrays(GL_POINTS, 0, 4);

		glDisable(GL_RASTERIZER_DISCARD);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_draw_vao);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 0, 1, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteBuffers(1, &m_position_buffer);
		glDeleteBuffers(1, &m_color_buffer);
		glDeleteBuffers(1, &m_element_buffer);
		glDeleteTextures(1, &m_position_buffer_tex);
		glDeleteTextures(1, &m_color_buffer_tex);
		glDeleteTextures(1, &m_element_buffer_tex);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		glDeleteVertexArrays(1, &m_draw_vao);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.1.4 AdvancedSyncDrawIndirect
//-----------------------------------------------------------------------------
class AdvancedSyncDrawIndirect : public ShaderImageLoadStoreBase
{
	GLuint m_draw_command_buffer;
	GLuint m_draw_command_buffer_tex;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_attribless_vao;
	GLuint m_draw_vao;
	GLuint m_draw_vbo;

	virtual long Setup()
	{
		m_draw_command_buffer	 = 0;
		m_draw_command_buffer_tex = 0;
		m_store_program			  = 0;
		m_draw_program			  = 0;
		m_attribless_vao		  = 0;
		m_draw_vao				  = 0;
		m_draw_vbo				  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		const char* const glsl_store_vs =
			"#version 420 core" NL "writeonly uniform uimageBuffer g_draw_command_buffer;" NL "void main() {" NL
			"  imageStore(g_draw_command_buffer, 0, uvec4(4, 1, 0, 0));" NL "}";
		const char* const glsl_draw_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
										 "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_draw_fs = "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
										 "void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, NULL, NULL, NULL, NULL);
		m_draw_program  = BuildProgram(glsl_draw_vs, NULL, NULL, NULL, glsl_draw_fs);

		glGenBuffers(1, &m_draw_command_buffer);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_command_buffer);
		glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(uvec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glGenTextures(1, &m_draw_command_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_draw_command_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, m_draw_command_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glGenVertexArrays(1, &m_attribless_vao);
		CreateFullViewportQuad(&m_draw_vao, &m_draw_vbo, NULL);

		glBindImageTexture(0, m_draw_command_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_store_program);
		glBindVertexArray(m_attribless_vao);
		glDrawArrays(GL_POINTS, 0, 1);

		glDisable(GL_RASTERIZER_DISCARD);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_draw_vao);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_draw_command_buffer);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
		glDrawArraysIndirect(GL_TRIANGLE_STRIP, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_draw_command_buffer);
		glDeleteTextures(1, &m_draw_command_buffer_tex);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		glDeleteVertexArrays(1, &m_draw_vao);
		glDeleteBuffers(1, &m_draw_vbo);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.1.5 AdvancedSyncTextureUpdate
//-----------------------------------------------------------------------------
class AdvancedSyncTextureUpdate : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_pbo;

	virtual long Setup()
	{
		m_texture		= 0;
		m_store_program = 0;
		m_draw_program  = 0;
		m_vao			= 0;
		m_vbo			= 0;
		m_pbo			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "out vec2 vs_texcoord;" NL
			"void main() {" NL "  gl_Position = i_position;" NL "  vs_texcoord = 0.5 + 0.5 * i_position.xy;" NL "}";
		const char* const glsl_store_fs =
			"#version 420 core" NL "writeonly uniform image2D g_image;" NL "void main() {" NL
			"  imageStore(g_image, ivec2(gl_FragCoord.xy), gl_FragCoord);" NL "  discard;" NL "}";
		const char* const glsl_draw_fs =
			"#version 420 core" NL "in vec2 vs_texcoord;" NL "layout(location = 0) out vec4 o_color;" NL
			"uniform sampler2D g_sampler;" NL "void main() {" NL "  o_color = texture(g_sampler, vs_texcoord);" NL "}";
		m_store_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_store_fs);
		m_draw_program  = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_draw_fs);

		std::vector<vec4> data(16 * 16, vec4(1.0f));
		glGenBuffers(1, &m_pbo);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, 16 * 16 * sizeof(vec4), &data[0], GL_DYNAMIC_READ);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 16, 16, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glViewport(0, 0, 16, 16);
		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUseProgram(m_store_program);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_FLOAT, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(1, 1, 1, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_pbo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.1.6 AdvancedSyncImageAccess2
//-----------------------------------------------------------------------------
class AdvancedSyncImageAccess2 : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_texture		= 0;
		m_store_program = 0;
		m_draw_program  = 0;
		m_vao			= 0;
		m_vbo			= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "out vec2 vs_texcoord;" NL
			"void main() {" NL "  gl_Position = i_position;" NL "  vs_texcoord = 0.5 + 0.5 * i_position.xy;" NL "}";
		const char* const glsl_store_fs =
			"#version 420 core" NL "writeonly uniform image2D g_image;" NL "uniform vec4 g_color;" NL "void main() {" NL
			"  imageStore(g_image, ivec2(gl_FragCoord.xy), g_color);" NL "  discard;" NL "}";
		const char* const glsl_draw_fs =
			"#version 420 core" NL "in vec2 vs_texcoord;" NL "layout(location = 0) out vec4 o_color;" NL
			"uniform sampler2D g_sampler;" NL "void main() {" NL "  o_color = texture(g_sampler, vs_texcoord);" NL "}";
		m_store_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_store_fs);
		m_draw_program  = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_draw_fs);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 1.0f, 0.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glUseProgram(m_draw_program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.1.7 AdvancedSyncBufferUpdate
//-----------------------------------------------------------------------------
class AdvancedSyncBufferUpdate : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_buffer_tex;
	GLuint m_store_program;
	GLuint m_attribless_vao;

	virtual long Setup()
	{
		m_buffer		 = 0;
		m_buffer_tex	 = 0;
		m_store_program  = 0;
		m_attribless_vao = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		const char* const glsl_store_vs =
			"#version 420 core" NL "writeonly uniform imageBuffer g_output_data;" NL "void main() {" NL
			"  imageStore(g_output_data, gl_VertexID, vec4(gl_VertexID));" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, NULL, NULL, NULL, NULL);

		glGenVertexArrays(1, &m_attribless_vao);
		glBindVertexArray(m_attribless_vao);

		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * 1000, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffer);

		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glEnable(GL_RASTERIZER_DISCARD);
		glUseProgram(m_store_program);
		glDrawArrays(GL_POINTS, 0, 1000);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		vec4* ptr =
			reinterpret_cast<vec4*>(glMapBufferRange(GL_TEXTURE_BUFFER, 0, 1000 * sizeof(vec4), GL_MAP_READ_BIT));
		for (int i = 0; i < 1000; ++i)
		{
			if (!IsEqual(ptr[i], vec4(static_cast<float>(i))))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Bad buffer value found at index " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteProgram(m_store_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.2.1 AdvancedAllStagesOneImage
//-----------------------------------------------------------------------------
class AdvancedAllStagesOneImage : public ShaderImageLoadStoreBase
{
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ebo;
	GLuint m_buffer;
	GLuint m_buffer_tex;
	GLuint m_texture;

	virtual long Setup()
	{
		m_program	= 0;
		m_vao		 = 0;
		m_vbo		 = 0;
		m_ebo		 = 0;
		m_buffer	 = 0;
		m_buffer_tex = 0;
		m_texture	= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInGeomStages(2))
			return NOT_SUPPORTED;
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
			"layout(r32i) coherent uniform iimageBuffer g_buffer;" NL
			"layout(r32ui) coherent uniform uimage2D g_image;" NL "void main() {" NL
			"  gl_Position = i_position + vec4(1.0, 1.0, 0.0, 0.0);" NL "  imageAtomicAdd(g_buffer, 0, 1);" NL
			"  imageAtomicAdd(g_image, ivec2(0, 0), 2u);" NL "}";
		const char* const glsl_tcs =
			"#version 420 core" NL "layout(vertices = 1) out;" NL
			"layout(r32i) coherent uniform iimageBuffer g_buffer;" NL
			"layout(r32ui) coherent uniform uimage2D g_image;" NL "void main() {" NL
			"  gl_out[gl_InvocationID].gl_Position = gl_in[0].gl_Position;" NL "  gl_TessLevelInner[0] = 1.0;" NL
			"  gl_TessLevelInner[1] = 1.0;" NL "  gl_TessLevelOuter[0] = 1.0;" NL "  gl_TessLevelOuter[1] = 1.0;" NL
			"  gl_TessLevelOuter[2] = 1.0;" NL "  gl_TessLevelOuter[3] = 1.0;" NL "  imageAtomicAdd(g_buffer, 0, 1);" NL
			"  imageAtomicAdd(g_image, ivec2(0, 0), 2u);" NL "}";
		const char* const glsl_tes =
			"#version 420 core" NL "layout(triangles, point_mode) in;" NL
			"layout(r32i) coherent uniform iimageBuffer g_buffer;" NL
			"layout(r32ui) coherent uniform uimage2D g_image;" NL "void main() {" NL
			"  imageAtomicAdd(g_buffer, 0, 1);" NL "  imageAtomicAdd(g_image, ivec2(0, 0), 2u);" NL
			"  gl_Position = gl_in[0].gl_Position;" NL "}";
		const char* const glsl_gs =
			"#version 420 core" NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL
			"layout(r32i) coherent uniform iimageBuffer g_buffer;" NL
			"layout(r32ui) coherent uniform uimage2D g_image;" NL "void main() {" NL
			"  imageAtomicAdd(g_buffer, 0, 1);" NL "  imageAtomicAdd(g_image, ivec2(0, 0), 2u);" NL
			"  gl_Position = gl_in[0].gl_Position;" NL "  EmitVertex();" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(r32i) coherent uniform iimageBuffer g_buffer;" NL
			"layout(r32ui) coherent uniform uimage2D g_image;" NL "void main() {" NL
			"  imageAtomicAdd(g_buffer, 0, 1);" NL "  imageAtomicAdd(g_image, ivec2(0, 0), 2u);" NL
			"  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "}";
		m_program = BuildProgram(glsl_vs, glsl_tcs, glsl_tes, glsl_gs, glsl_fs);
		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_buffer"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image"), 1);

		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		GLint i32 = 0;
		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, 4, &i32, GL_STATIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, m_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		GLuint ui32 = 0;
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 1, 1, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &ui32);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		glBindVertexArray(m_vao);
		glPatchParameteri(GL_PATCH_VERTICES, 1);

		glDrawElementsInstancedBaseVertex(GL_PATCHES, 1, GL_UNSIGNED_SHORT, 0, 1, 0);
		glDrawElementsInstanced(GL_PATCHES, 1, GL_UNSIGNED_SHORT, 0, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glGetBufferSubData(GL_TEXTURE_BUFFER, 0, 4, &i32);
		if (i32 < 20 || i32 > 50)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Buffer value is " << i32
												<< " should be in the range [20;50]" << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &ui32);
		if (ui32 < 40 || ui32 != static_cast<GLuint>(2 * i32))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Texture value is " << ui32 << " should be "
												<< (2 * i32) << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.3.1 AdvancedMemoryDependentInvocation
//-----------------------------------------------------------------------------
class AdvancedMemoryDependentInvocation : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_buffer_tex;
	GLuint m_texture;
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_buffer	 = 0;
		m_buffer_tex = 0;
		m_texture	= 0;
		m_program	= 0;
		m_vao		 = 0;
		m_vbo		 = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(2))
			return NOT_SUPPORTED;
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
			"layout(location = 1) in vec4 i_color;" NL "out vec4 vs_color;" NL
			"layout(rgba32f) coherent uniform imageBuffer g_buffer;" NL
			"layout(rgba32f) coherent uniform image2D g_image;" NL "void main() {" NL "  gl_Position = i_position;" NL
			"  vs_color = i_color;" NL "  imageStore(g_buffer, 0, vec4(1.0));" NL
			"  imageStore(g_image, ivec2(0, 0), vec4(2.0));" NL "  memoryBarrier();" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "in vec4 vs_color;" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) coherent uniform imageBuffer g_buffer;" NL
			"layout(rgba32f) coherent uniform image2D g_image;" NL "void main() {" NL "  o_color = vs_color;" NL
			"  if (imageLoad(g_buffer, 0) != vec4(1.0)) o_color = vec4(1.0, 0.0, 0.0, 0.1);" NL
			"  if (imageLoad(g_image, ivec2(0, 0)) != vec4(2.0)) o_color = vec4(1.0, 0.0, 0.0, 0.2);" NL "}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);
		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_buffer"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image"), 1);

		vec4 zero(0);
		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4), &zero, GL_STATIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, &zero);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(1, &m_texture);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.3.2 AdvancedMemoryOrder
//-----------------------------------------------------------------------------
class AdvancedMemoryOrder : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_buffer_tex;
	GLuint m_texture;
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_buffer	 = 0;
		m_buffer_tex = 0;
		m_texture	= 0;
		m_program	= 0;
		m_vao		 = 0;
		m_vbo		 = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "out vec4 vs_color;" NL
			"layout(rgba32f) coherent uniform imageBuffer g_buffer;" NL "void main() {" NL
			"  gl_Position = i_position;" NL "  vs_color = vec4(0, 1, 0, 1);" NL
			"  imageStore(g_buffer, gl_VertexID, vec4(1.0));" NL "  imageStore(g_buffer, gl_VertexID, vec4(2.0));" NL
			"  imageStore(g_buffer, gl_VertexID, vec4(3.0));" NL
			"  if (imageLoad(g_buffer, gl_VertexID) != vec4(3.0)) vs_color = vec4(1, 0, 0, 1);" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "in vec4 vs_color;" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) uniform image2D g_image;" NL "void main() {" NL "  o_color = vs_color;" NL
			"  ivec2 coord = ivec2(gl_FragCoord);" NL "  for (int i = 0; i < 3; ++i) {" NL
			"    imageStore(g_image, coord, vec4(i));" NL "    vec4 v = imageLoad(g_image, coord);" NL
			"    if (v != vec4(i)) {" NL "      o_color = vec4(v.xyz, 0.0);" NL "      break;" NL "    }" NL "  }" NL
			"}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);
		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_buffer"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image"), 1);

		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * 4, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);

		glGenTextures(1, &m_buffer_tex);
		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffer);
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		std::vector<vec4> data(getWindowWidth() * getWindowHeight());
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(1, &m_texture);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.4.1 AdvancedSSOSimple
//-----------------------------------------------------------------------------
class AdvancedSSOSimple : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_pipeline[2];
	GLuint m_vsp, m_fsp0, m_fsp1;
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		glGenTextures(1, &m_texture);
		glGenProgramPipelines(2, m_pipeline);
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "out gl_PerVertex {" NL
			"  vec4 gl_Position;" NL "};" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs0 =
			"#version 420 core" NL "layout(rgba32f) uniform image2D g_image[4];" NL "void main() {" NL
			"  for (int i = 0; i < g_image.length(); ++i) {" NL
			"    imageStore(g_image[i], ivec2(gl_FragCoord), vec4(1.0));" NL "  }" NL "  discard;" NL "}";
		const char* const glsl_fs1 =
			"#version 420 core" NL "writeonly uniform image2D g_image[4];" NL "void main() {" NL
			"  for (int i = 0; i < g_image.length(); ++i) {" NL
			"    imageStore(g_image[i], ivec2(gl_FragCoord), vec4(2.0));" NL "  }" NL "  discard;" NL "}";
		m_vsp  = BuildShaderProgram(GL_VERTEX_SHADER, glsl_vs);
		m_fsp0 = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs0);
		m_fsp1 = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs1);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glProgramUniform1i(m_fsp0, glGetUniformLocation(m_fsp0, "g_image[0]"), 0);
		glProgramUniform1i(m_fsp0, glGetUniformLocation(m_fsp0, "g_image[1]"), 2);
		glProgramUniform1i(m_fsp0, glGetUniformLocation(m_fsp0, "g_image[2]"), 4);
		glProgramUniform1i(m_fsp0, glGetUniformLocation(m_fsp0, "g_image[3]"), 6);

		glProgramUniform1i(m_fsp1, glGetUniformLocation(m_fsp1, "g_image[0]"), 1);
		glProgramUniform1i(m_fsp1, glGetUniformLocation(m_fsp1, "g_image[1]"), 3);
		glProgramUniform1i(m_fsp1, glGetUniformLocation(m_fsp1, "g_image[2]"), 5);
		glProgramUniform1i(m_fsp1, glGetUniformLocation(m_fsp1, "g_image[3]"), 7);

		glUseProgramStages(m_pipeline[0], GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline[0], GL_FRAGMENT_SHADER_BIT, m_fsp0);

		glUseProgramStages(m_pipeline[1], GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline[1], GL_FRAGMENT_SHADER_BIT, m_fsp1);

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 8, 0, GL_RGBA, GL_FLOAT,
					 NULL);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 1, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture, 0, GL_FALSE, 2, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(3, m_texture, 0, GL_FALSE, 3, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(4, m_texture, 0, GL_FALSE, 4, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(5, m_texture, 0, GL_FALSE, 5, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(6, m_texture, 0, GL_FALSE, 6, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(7, m_texture, 0, GL_FALSE, 7, GL_READ_WRITE, GL_RGBA32F);

		glBindVertexArray(m_vao);

		glBindProgramPipeline(m_pipeline[0]);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);

		glBindProgramPipeline(m_pipeline[1]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		std::vector<vec4> data(getWindowWidth() * getWindowHeight() * 8);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_FLOAT, &data[0]);

		for (int layer = 0; layer < 8; ++layer)
		{
			for (int h = 0; h < getWindowHeight(); ++h)
			{
				for (int w = 0; w < getWindowWidth(); ++w)
				{
					const vec4 c = data[layer * getWindowWidth() * getWindowHeight() + h * getWindowWidth() + w];
					if (layer % 2)
					{
						if (!IsEqual(c, vec4(2.0f)))
						{
							return ERROR;
						}
					}
					else
					{
						if (!IsEqual(c, vec4(1.0f)))
						{
							return ERROR;
						}
					}
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_vbo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp0);
		glDeleteProgram(m_fsp1);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteProgramPipelines(2, m_pipeline);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.4.2 AdvancedSSOAtomicCounters
//-----------------------------------------------------------------------------
class AdvancedSSOAtomicCounters : public ShaderImageLoadStoreBase
{
	GLuint m_buffer, m_buffer_tex;
	GLuint m_counter_buffer;
	GLuint m_transform_buffer;
	GLuint m_pipeline;
	GLuint m_vao, m_vbo;
	GLuint m_vsp, m_fsp;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		m_vsp = 0;
		m_fsp = 0;
		glGenBuffers(1, &m_buffer);
		glGenTextures(1, &m_buffer_tex);
		glGenBuffers(1, &m_counter_buffer);
		glGenBuffers(1, &m_transform_buffer);
		glGenProgramPipelines(1, &m_pipeline);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);
		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
			"layout(location = 1) in vec4 i_color;" NL "layout(location = 3) out vec4 o_color;" NL
			"out gl_PerVertex {" NL "  vec4 gl_Position;" NL "};" NL "layout(std140) uniform Transform {" NL
			"  mat4 mvp;" NL "} g_transform;" NL "writeonly uniform imageBuffer g_buffer;" NL
			"layout(binding = 0, offset = 0) uniform atomic_uint g_counter;" NL "void main() {" NL
			"  gl_Position = g_transform.mvp * i_position;" NL "  o_color = i_color;" NL
			"  const uint index = atomicCounterIncrement(g_counter);" NL
			"  imageStore(g_buffer, int(index), gl_Position);" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "layout(location = 3) in vec4 i_color;" NL
			"layout(location = 0) out vec4 o_color;" NL "void main() {" NL "  o_color = i_color;" NL "}";
		m_vsp = BuildShaderProgram(GL_VERTEX_SHADER, glsl_vs);
		m_fsp = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs);

		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_fsp);

		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * 4, NULL, GL_STATIC_DRAW);

		glBindTexture(GL_TEXTURE_BUFFER, m_buffer_tex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_buffer);

		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_counter_buffer);
		vec4 zero(0);
		glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_STATIC_DRAW);

		glBindBuffer(GL_UNIFORM_BUFFER, m_transform_buffer);
		mat4 identity(1);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4), &identity, GL_STATIC_DRAW);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_counter_buffer);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_transform_buffer);
		glBindImageTexture(0, m_buffer_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindVertexArray(m_vao);
		glBindProgramPipeline(m_pipeline);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, 1, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}

		std::vector<vec4> data(4);
		glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
		glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(vec4) * 4, &data[0]);

		for (int i = 0; i < 4; ++i)
		{
			if (!IsEqual(data[i], vec4(-1.0f, -1.0f, 0.0f, 1.0f)) && !IsEqual(data[i], vec4(1.0f, -1.0f, 0.0f, 1.0f)) &&
				!IsEqual(data[i], vec4(-1.0f, 1.0f, 0.0f, 1.0f)) && !IsEqual(data[i], vec4(1.0f, 1.0f, 0.0f, 1.0f)))
			{
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_buffer);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_counter_buffer);
		glDeleteBuffers(1, &m_transform_buffer);
		glDeleteTextures(1, &m_buffer_tex);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_fsp);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteProgramPipelines(1, &m_pipeline);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.4.3 AdvancedSSOSubroutine
//-----------------------------------------------------------------------------
class AdvancedSSOSubroutine : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_attribless_vao;
	GLuint m_program;
	GLint  m_draw_buffer;

	virtual long Setup()
	{
		glGenTextures(1, &m_texture);
		glGenVertexArrays(1, &m_attribless_vao);

		const char* const glsl_vs = "#version 420 core" NL "const int kSize = 3;" NL
									"const vec2 g_triangle[kSize] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));" NL
									"void main() {" NL "  gl_Position = vec4(g_triangle[gl_VertexID], 0, 1);" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "writeonly uniform image2DArray g_image0;" NL
			"writeonly uniform image2DArray g_image1;" NL "subroutine void Brush(ivec2 coord);" NL
			"subroutine uniform Brush g_brush;" NL "subroutine(Brush) void Brush0(ivec2 coord) {" NL
			"  imageStore(g_image0, ivec3(coord, 0), vec4(1.0, 0.0, 0.0, 1.0));" NL
			"  imageStore(g_image0, ivec3(coord, 1), vec4(0.0, 1.0, 0.0, 1.0));" NL
			"  imageStore(g_image0, ivec3(coord, 2), vec4(0.0, 0.0, 1.0, 1.0));" NL "}" NL
			"subroutine(Brush) void Brush1(ivec2 coord) {" NL
			"  imageStore(g_image1, ivec3(coord, 0), vec4(0.0, 1.0, 0.0, 1.0));" NL
			"  imageStore(g_image1, ivec3(coord, 1), vec4(0.0, 0.0, 1.0, 1.0));" NL
			"  imageStore(g_image1, ivec3(coord, 2), vec4(1.0, 0.0, 0.0, 1.0));" NL "}" NL "void main() {" NL
			"  g_brush(ivec2(gl_FragCoord));" NL "}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_image0"), 1);
		glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_image1"), 1);

		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 3, 0, GL_RGBA, GL_FLOAT,
					 NULL);

		glGetIntegerv(GL_DRAW_BUFFER, &m_draw_buffer);

		glDrawBuffer(GL_NONE);
		glBindImageTexture(1, m_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
		glUseProgram(m_program);
		glBindVertexArray(m_attribless_vao);

		const GLuint indices[2] = { glGetSubroutineIndex(m_program, GL_FRAGMENT_SHADER, "Brush0"),
									glGetSubroutineIndex(m_program, GL_FRAGMENT_SHADER, "Brush1") };

		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &indices[0]);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);

		std::vector<vec4> data(getWindowWidth() * getWindowHeight() * 3);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_FLOAT, &data[0]);

		for (int layer = 0; layer < 3; ++layer)
		{
			for (int h = 0; h < getWindowHeight(); ++h)
			{
				for (int w = 0; w < getWindowWidth(); ++w)
				{
					const vec4 c = data[layer * getWindowWidth() * getWindowHeight() + h * getWindowWidth() + w];
					if (layer == 0 && !IsEqual(c, vec4(1.0f, 0.0f, 0.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (1 0 0 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
					else if (layer == 1 && !IsEqual(c, vec4(0.0f, 1.0f, 0.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (0 1 0 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
					else if (layer == 2 && !IsEqual(c, vec4(0.0f, 0.0f, 1.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (0 0 1 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
		}

		glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &indices[1]);
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 3, 1, 0);

		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_FLOAT, &data[0]);

		for (int layer = 0; layer < 3; ++layer)
		{
			for (int h = 0; h < getWindowHeight(); ++h)
			{
				for (int w = 0; w < getWindowWidth(); ++w)
				{
					const vec4 c = data[layer * getWindowWidth() * getWindowHeight() + h * getWindowWidth() + w];
					if (layer == 0 && !IsEqual(c, vec4(0.0f, 1.0f, 0.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (0 1 0 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
					else if (layer == 1 && !IsEqual(c, vec4(0.0f, 0.0f, 1.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (0 0 1 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
					else if (layer == 2 && !IsEqual(c, vec4(1.0f, 0.0f, 0.0f, 1.0f)))
					{
						m_context.getTestContext().getLog()
							<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
							<< ") should be (1 0 0 1)" << tcu::TestLog::EndMessage;
						return ERROR;
					}
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDrawBuffer(m_draw_buffer);
		glDeleteTextures(1, &m_texture);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.4.4 AdvancedSSOPerSample
//-----------------------------------------------------------------------------
class AdvancedSSOPerSample : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_pipeline;
	GLuint m_vao, m_vbo, m_ebo;
	GLuint m_vsp, m_store_fsp, m_load_fsp;

	virtual long Setup()
	{
		m_vao		= 0;
		m_vbo		= 0;
		m_ebo		= 0;
		m_vsp		= 0;
		m_store_fsp = 0;
		m_load_fsp  = 0;
		glGenTextures(1, &m_texture);
		glGenProgramPipelines(1, &m_pipeline);

		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedSamples(4))
			return NOT_SUPPORTED;

		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		const char* const glsl_vs =
			"#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "out gl_PerVertex {" NL
			"  vec4 gl_Position;" NL "};" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_store_fs =
			"#version 420 core" NL "layout(rgba32f) writeonly uniform image2DMS g_image;" NL "void main() {" NL
			"  imageStore(g_image, ivec2(gl_FragCoord), gl_SampleID, vec4(gl_SampleID+1));" NL "}";
		const char* const glsl_load_fs =
			"#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) readonly uniform image2DMS g_image;" NL "void main() {" NL
			"  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "  if (imageLoad(g_image, ivec2(gl_FragCoord), gl_SampleID) != "
			"vec4(gl_SampleID+1)) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		m_vsp		= BuildShaderProgram(GL_VERTEX_SHADER, glsl_vs);
		m_store_fsp = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_store_fs);
		m_load_fsp  = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_load_fs);

		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_texture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA32F, getWindowWidth(), getWindowHeight(),
								GL_FALSE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_vsp);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_store_fsp);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glBindProgramPipeline(m_pipeline);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_load_fsp);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_vsp);
		glDeleteProgram(m_store_fsp);
		glDeleteProgram(m_load_fsp);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteProgramPipelines(1, &m_pipeline);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.5 AdvancedCopyImage
//-----------------------------------------------------------------------------
class AdvancedCopyImage : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint m_vao, m_vbo, m_ebo;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		const char* const glsl_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
									"void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "layout(rgba32f) readonly uniform image2D g_input_image;" NL
			"layout(rgba32f) writeonly uniform image2D g_output_image;" NL "void main() {" NL
			"  ivec2 coord = ivec2(gl_FragCoord);" NL
			"  imageStore(g_output_image, coord, imageLoad(g_input_image, coord));" NL "  discard;" NL "}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_input_image"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_output_image"), 1);

		std::vector<vec4> data(getWindowWidth() * getWindowHeight(), vec4(7.0f));
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_FLOAT, &data[0]);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_FLOAT, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		std::vector<vec4> rdata(getWindowWidth() * getWindowHeight());
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &rdata[0]);

		for (int h = 0; h < getWindowHeight(); ++h)
		{
			for (int w = 0; w < getWindowWidth(); ++w)
			{
				if (!IsEqual(rdata[h * getWindowWidth() + w], vec4(7.0f)))
				{
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.6 AdvancedAllMips
//-----------------------------------------------------------------------------
class AdvancedAllMips : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_store_program, m_load_program;
	GLuint m_vao, m_vbo, m_ebo;

	virtual long Setup()
	{
		glGenTextures(1, &m_texture);
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		const char* const glsl_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
									"void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_store_fs =
			"#version 420 core" NL "layout(rgba32f) uniform image2D g_image[6];" NL "void main() {" NL
			"  for (int i = 0; i < 6; ++i) {" NL "    imageStore(g_image[i], ivec2(gl_FragCoord), vec4(i));" NL "  }" NL
			"  discard;" NL "}";
		const char* const glsl_load_fs =
			"#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(rgba32f) uniform image2D g_image[6];" NL "void main() {" NL
			"  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "  for (int i = 0; i < 6; ++i) {" NL
			"    const ivec2 coord = ivec2(gl_FragCoord);" NL "    const vec4 c = imageLoad(g_image[i], coord);" NL
			"    if (c != vec4(i) && c != vec4(0.0)) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "  }" NL "}";
		m_store_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_store_fs);
		m_load_program  = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_load_fs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glUseProgram(m_store_program);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[0]"), 0);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[1]"), 1);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[2]"), 2);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[3]"), 3);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[4]"), 4);
		glUniform1i(glGetUniformLocation(m_store_program, "g_image[5]"), 5);
		glUseProgram(0);

		glUseProgram(m_load_program);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[0]"), 0);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[1]"), 1);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[2]"), 2);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[3]"), 3);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[4]"), 4);
		glUniform1i(glGetUniformLocation(m_load_program, "g_image[5]"), 5);
		glUseProgram(0);

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 32, 32, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 16, 16, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA32F, 8, 8, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA32F, 4, 4, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA32F, 2, 2, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexImage2D(GL_TEXTURE_2D, 5, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 1, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture, 2, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(3, m_texture, 3, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(4, m_texture, 4, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(5, m_texture, 5, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glViewport(0, 0, 32, 32);
		glBindVertexArray(m_vao);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_store_program);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(m_load_program);
		glDrawElementsInstancedBaseVertex(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		if (!ValidateReadBuffer(0, 0, 32, 32, vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_load_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 2.7 AdvancedCast
//-----------------------------------------------------------------------------
class AdvancedCast : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint m_vao, m_vbo, m_ebo;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		const char* const glsl_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
									"void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs =
			"#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
			"layout(r32i) coherent uniform iimage2D g_image0;" NL "layout(r32ui) coherent uniform uimage2D g_image1;" NL
			"void main() {" NL "  o_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "  ivec2 coord = ivec2(gl_FragCoord);" NL
			"  if (imageAtomicAdd(g_image0, coord, 2) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAdd(g_image0, coord, -1) != 2) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAdd(g_image1, coord, 1) != 0) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			"  if (imageAtomicAdd(g_image1, coord, 2) != 1) o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		glUseProgram(m_program);
		glUniform1i(glGetUniformLocation(m_program, "g_image0"), 0);
		glUniform1i(glGetUniformLocation(m_program, "g_image1"), 1);

		{
			std::vector<GLubyte> data(getWindowWidth() * getWindowHeight() * 4);
			glBindTexture(GL_TEXTURE_2D, m_texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
						 &data[0]);

			glBindTexture(GL_TEXTURE_2D, m_texture[1]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
						 &data[0]);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glDrawElementsInstancedBaseInstance(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1, 0);

		std::vector<GLubyte> data(getWindowWidth() * getWindowHeight() * 4);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

		for (int h = 0; h < getWindowHeight(); ++h)
		{
			for (int w = 0; w < getWindowWidth(); ++w)
			{
				const GLubyte c[4] = {
					data[h * (getWindowWidth() * 4) + w * 4 + 0], data[h * (getWindowWidth() * 4) + w * 4 + 1],
					data[h * (getWindowWidth() * 4) + w * 4 + 2], data[h * (getWindowWidth() * 4) + w * 4 + 3],
				};
				if (c[0] != 1 || c[1] != 0 || c[2] != 0 || c[3] != 0)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
						<< ") should be (1 0 0 0)" << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

		for (int h = 0; h < getWindowHeight(); ++h)
		{
			for (int w = 0; w < getWindowWidth(); ++w)
			{
				const GLubyte c[4] = {
					data[h * (getWindowWidth() * 4) + w * 4 + 0], data[h * (getWindowWidth() * 4) + w * 4 + 1],
					data[h * (getWindowWidth() * 4) + w * 4 + 2], data[h * (getWindowWidth() * 4) + w * 4 + 3],
				};
				if (c[0] != 3 || c[1] != 0 || c[2] != 0 || c[3] != 0)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Color is (" << c[0] << " " << c[1] << c[2] << " " << c[3]
						<< ") should be (3 0 0 0)" << tcu::TestLog::EndMessage;
					return ERROR;
				}
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
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};

/** Test "imageLoad() and imageStore() for single-byte data alignment" description follows.
 *
 *  Steps:
 *  - create two textures: "source" and "destination". Fill "source"
 *  texture with unique values. Fill "destination" texture with zeros,
 *  - prepare a program object that will read texel from "source" image at given
 *  coordinates and write its value to "destination" image at same
 *  coordinates,
 *  - bind "source" and "destination" textures as "source" and "destination"
 *  image uniforms,
 *  - render "full screen" quad (left bottom corner at -1,-1 and right top
 *  corner at 1,1),
 *  - verify that texel values in "destination" texture match those in
 *  "source" texture (use glGetTexImage).
 *
 *  Test with 2D R8UI textures with following dimensions:
 *  - 16x16,
 *  - 16x17,
 *  - 17x16,
 *  - 17x17,
 *  - 16x18,
 *  - 18x16,
 *  - 18x18,
 *  - 19x16,
 *  - 16x19,
 *  - 19x19.
 *
 *  Note that default data alignment should cause problems with packing/
 *  /unpacking. Therefore GL_PACK_ALIGNMENT and GL_UNPACK_ALIGNMENT parameters
 *  of pixel storage mode have to be changed to one byte alignment.
 *
 *  Program should consist of vertex and fragment shader. Vertex shader should
 *  pass vertex position through. Fragment shader should do imageLoad() and
 *  imageStore() operations at coordinates gl_FragCoord.
 **/
class ImageLoadStoreDataAlignmentTest : public ShaderImageLoadStoreBase
{
private:
	/* Structures */
	struct TextureDimensions
	{
		GLuint m_width;
		GLuint m_height;

		TextureDimensions(GLuint width, GLuint height) : m_width(width), m_height(height)
		{
		}
	};

	/* Typedefs */
	typedef std::deque<TextureDimensions> TextureDimensionsList;

	/* Fields */
	GLuint				  m_destination_texture_id;
	GLuint				  m_program_id;
	TextureDimensionsList m_texture_dimensions;
	GLuint				  m_source_texture_id;
	GLuint				  m_vertex_array_object_id;
	GLuint				  m_vertex_buffer_id;

public:
	/* Constructor */
	ImageLoadStoreDataAlignmentTest()
		: m_destination_texture_id(0)
		, m_program_id(0)
		, m_source_texture_id(0)
		, m_vertex_array_object_id(0)
		, m_vertex_buffer_id(0)
	{
		/* Nothing to be done here */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Setup()
	{
		/* Shaders code */
		const char* const vertex_shader_code = "#version 400 core\n"
											   "#extension GL_ARB_shader_image_load_store : require\n"
											   "\n"
											   "precision highp float;\n"
											   "\n"
											   "in vec4 vs_in_position;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    gl_Position = vs_in_position;\n"
											   "}\n";

		const char* const fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(r8ui) writeonly uniform uimage2D u_destination_image;\n"
			"layout(r8ui) readonly  uniform uimage2D u_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    uvec4 loaded_color = imageLoad (u_source_image,      ivec2(gl_FragCoord));\n"
			"                         imageStore(u_destination_image, ivec2(gl_FragCoord), loaded_color);\n"
			"\n"
			"    discard;\n"
			"}\n";

		/* Vertex postions for "full screen" quad, made with triangle strip */
		static const GLfloat m_vertex_buffer_data[] = {
			-1.0f, -1.0f, 0.0f, 1.0f, /* left bottom */
			-1.0f, 1.0f,  0.0f, 1.0f, /* left top */
			1.0f,  -1.0f, 0.0f, 1.0f, /* right bottom */
			1.0f,  1.0f,  0.0f, 1.0f, /* right top */
		};

		/* Result of BuildProgram operation */
		bool is_program_correct = true; /* BuildProgram set false when it fails, but it does not set true on success */

		/* Add all tested texture dimensions */
		m_texture_dimensions.push_back(TextureDimensions(16, 16));
		m_texture_dimensions.push_back(TextureDimensions(16, 17));
		m_texture_dimensions.push_back(TextureDimensions(17, 16));
		m_texture_dimensions.push_back(TextureDimensions(17, 17));
		m_texture_dimensions.push_back(TextureDimensions(16, 18));
		m_texture_dimensions.push_back(TextureDimensions(18, 16));
		m_texture_dimensions.push_back(TextureDimensions(18, 18));
		m_texture_dimensions.push_back(TextureDimensions(16, 19));
		m_texture_dimensions.push_back(TextureDimensions(19, 16));
		m_texture_dimensions.push_back(TextureDimensions(19, 19));

		/* Clean previous error */
		glGetError();

		/* Set single-byte data alignment */
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		GLU_EXPECT_NO_ERROR(glGetError(), "PixelStorei");

		/* Prepare buffer with vertex positions of "full screen" quad" */
		glGenBuffers(1, &m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenBuffers");

		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertex_buffer_data), m_vertex_buffer_data, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(glGetError(), "BufferData");

		/* Generate vertex array object */
		glGenVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenVertexArrays");

		/* Prepare program object */
		m_program_id = BuildProgram(vertex_shader_code, 0 /* src_tcs */, 0 /* src_tes */, 0 /*src_gs */,
									fragment_shader_code, &is_program_correct);
		if (false == is_program_correct)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		/* Reset OpenGL state */
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		/* Delete program */
		if (0 != m_program_id)
		{
			glDeleteProgram(m_program_id);
			m_program_id = 0;
		}

		/* Delete textures */
		if (0 != m_destination_texture_id)
		{
			glDeleteTextures(1, &m_destination_texture_id);
			m_destination_texture_id = 0;
		}

		if (0 != m_source_texture_id)
		{
			glDeleteTextures(1, &m_source_texture_id);
			m_source_texture_id = 0;
		}

		/* Delete vertex array object */
		if (0 != m_vertex_array_object_id)
		{
			glDeleteVertexArrays(1, &m_vertex_array_object_id);
			m_vertex_array_object_id = 0;
		}

		/* Delete buffer */
		if (0 != m_vertex_buffer_id)
		{
			glDeleteBuffers(1, &m_vertex_buffer_id);
			m_vertex_buffer_id = 0;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool result = true;

		/* For each dimension */
		for (TextureDimensionsList::const_iterator it = m_texture_dimensions.begin(); m_texture_dimensions.end() != it;
			 ++it)
		{
			/* Prepare "source" and "destination" textures */
			GLU_EXPECT_NO_ERROR(Create2DR8UIDestinationTexture(it->m_width, it->m_height, m_destination_texture_id),
								"Create2DR8UIDestinationTexture");
			GLU_EXPECT_NO_ERROR(Create2DR8UISourceTexture(it->m_width, it->m_height, m_source_texture_id),
								"Create2DR8UISourceTexture");

			/* Copy texture data with imageLoad() and imageStore() operations */
			Copy2DR8UITexture(m_destination_texture_id, m_source_texture_id);

			/* Compare "source" and "destination" textures */
			if (false ==
				Compare2DR8UITextures(m_destination_texture_id, m_source_texture_id, it->m_width, it->m_height))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Copy with imageLoad and imageStore failed for textures: " << it->m_width << "x" << it->m_height
					<< ". Source and destination textures are different" << tcu::TestLog::EndMessage;

				result = false;
			}

			/* Destroy "source" and "destination" textures */
			glDeleteTextures(1, &m_destination_texture_id);
			glDeleteTextures(1, &m_source_texture_id);

			m_destination_texture_id = 0;
			m_source_texture_id		 = 0;
		}

		if (false == result)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

private:
	/* Private methods */

	/** Binds a texture to user-specified image unit and updates relevant sampler uniform
	 *
	 * @param program_id   Program object id
	 * @param texture_id   Texture id
	 * @param image_unit   Index of image unit
	 * @param uniform_name Name of image uniform
	 **/
	void BindTextureToImage(GLuint program_id, GLuint texture_id, GLuint image_unit, const char* uniform_name)
	{
		/* Uniform location and invalid value */
		static const GLint invalid_uniform_location = -1;
		GLint			   image_uniform_location   = 0;

		/* Get uniform location */
		image_uniform_location = glGetUniformLocation(program_id, uniform_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");
		if (invalid_uniform_location == image_uniform_location)
		{
			throw tcu::InternalError("A required uniform is considered inactive", uniform_name, __FILE__, __LINE__);
		}

		/* Bind texture to image unit */
		glBindImageTexture(image_unit, texture_id, 0 /* level */, GL_FALSE, 0 /* layer */, GL_READ_WRITE, GL_R8UI);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindImageTexture");

		/* Set uniform to image unit */
		glUniform1i(image_uniform_location, image_unit);
		GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");
	}

	/** Compare two 2D R8UI textures
	 *
	 * @param left_texture_id  Id of "left" texture object
	 * @param right_texture_id Id of "right" texture object
	 * @param width            Width of the textures
	 * @param height           Height of the textures
	 *
	 * @return true when texture data is identical, false otherwise
	 **/
	bool Compare2DR8UITextures(GLuint left_texture_id, GLuint right_texture_id, GLuint width, GLuint height)
	{
		/* Size of textures */
		const GLuint texture_data_size = width * height;

		/* Storage for texture data */
		std::vector<GLubyte> left_texture_data;
		std::vector<GLubyte> right_texture_data;

		/* Alocate memory for texture data */
		left_texture_data.resize(texture_data_size);
		right_texture_data.resize(texture_data_size);

		/* Get "left" texture data */
		glBindTexture(GL_TEXTURE_2D, left_texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &left_texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		/* Get "right" texture data */
		glBindTexture(GL_TEXTURE_2D, right_texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &right_texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		/* Compare texels */
		return (0 == memcmp(&left_texture_data[0], &right_texture_data[0], texture_data_size));
	}

	/** Copy contents of "source" texture to "destination" texture with imageLoad() and imageStore() operations
	 *
	 * @param destination_texture_id Id of "destination" texture object
	 * @param source_texture_id      Id of "source" texture object
	 **/
	void Copy2DR8UITexture(GLuint destination_texture_id, GLuint source_texture_id)
	{
		/* Uniform names */
		static const char* const destination_image_uniform_name = "u_destination_image";
		static const char* const source_image_uniform_name		= "u_source_image";

		/* Attribute name */
		static const char* const position_attribute_name = "vs_in_position";

		/* Attribute location and invalid value */
		static const GLint invalid_attribute_location  = -1;
		GLint			   position_attribute_location = 0;

		/* Set current program */
		glUseProgram(m_program_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "UseProgram");

		/* Bind vertex array object */
		glBindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindVertexArray");

		/* Bind buffer with quad vertex positions */
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		/* Set up position attribute */
		position_attribute_location = glGetAttribLocation(m_program_id, position_attribute_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetAttribLocation");
		if (invalid_attribute_location == position_attribute_location)
		{
			throw tcu::InternalError("Attribute location is not available", position_attribute_name, __FILE__,
									 __LINE__);
		}

		glVertexAttribPointer(position_attribute_location, 4 /*size */, GL_FLOAT, GL_FALSE /* normalized */,
							  0 /* stride */, 0);
		GLU_EXPECT_NO_ERROR(glGetError(), "VertexAttribPointer");

		glEnableVertexAttribArray(position_attribute_location);
		GLU_EXPECT_NO_ERROR(glGetError(), "EnableVertexAttribArray");

		/* Set up textures as source and destination images */
		BindTextureToImage(m_program_id, destination_texture_id, 0 /* image_unit */, destination_image_uniform_name);
		BindTextureToImage(m_program_id, source_texture_id, 1 /* image_unit */, source_image_uniform_name);

		/* Execute draw */
		glDrawArrays(GL_TRIANGLE_STRIP, 0 /* first vertex */, 4 /* number of vertices */);
		GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");
	}

	/** Create 2D R8UI texture and fills it with zeros
	 *
	 * @param width          Width of created texture
	 * @param height         Height of created texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum Create2DR8UIDestinationTexture(GLuint width, GLuint height, GLuint& out_texture_id)
	{
		/* Texture size */
		const GLuint texture_size = width * height;

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_size);

		/* Set all texels */
		for (GLuint i = 0; i < texture_size; ++i)
		{
			texture_data[i] = 0;
		}

		/* Create texture */
		return Create2DR8UITexture(width, height, texture_data, out_texture_id);
	}

	/** Create 2D R8UI texture and fills it with increasing values, starting from 0
	 *
	 * @param width          Width of created texture
	 * @param height         Height of created texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum Create2DR8UISourceTexture(GLuint width, GLuint height, GLuint& out_texture_id)
	{
		/* Texture size */
		const GLuint texture_size = width * height;

		/* Value of texel */
		GLubyte texel_value = 0;

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_size);

		/* Set all texels */
		for (GLuint i = 0; i < texture_size; ++i)
		{
			texture_data[i] = texel_value++;
		}

		/* Create texture */
		return Create2DR8UITexture(width, height, texture_data, out_texture_id);
	}

	/** Create 2D R8UI texture and fills it with user-provided data
	 *
	 * @param width          Width of created texture
	 * @param height         Height of created texture
	 * @param texture_data   Texture data
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum Create2DR8UITexture(GLuint width, GLuint height, const std::vector<GLubyte>& texture_data,
							   GLuint& out_texture_id)
	{
		GLenum err		  = 0;
		GLuint texture_id = 0;

		/* Generate texture */
		glGenTextures(1, &texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			return err;
		}

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Allocate storage and fill texture */
		glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_R8UI, width, height, 0 /* border */, GL_RED_INTEGER,
					 GL_UNSIGNED_BYTE, &texture_data[0]);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Make texture complete */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Set out_texture_id */
		out_texture_id = texture_id;

		/* Done */
		return GL_NO_ERROR;
	}
};

/** Test "imageLoad() and imageStore() for non-layered image bindings" description follows.
 *
 *  Steps: same as in test 1 (ImageLoadStoreDataAlignmentTest).
 *
 *  Test non-layered image bindings (BindImageTexture <layered>: false) with:
 *  | Type           | Dimensions |
 *  | 2D_ARRAY       | 64x64x6    |
 *  | 3D             | 64x64x6    |
 *  | CUBE_MAP       | 64         |
 *  | CUBE_MAP_ARRAY | 64x3       |
 *
 *  Use RGBA8 format. All layers shall be tested.
 *
 *  Program should consist of vertex and fragment shader. Vertex shader should
 *  pass vertex position through. Fragment shader should do imageLoad() and
 *  imageStore() operations at coordinates gl_FragCoord. Fragment shader should
 *  use image2D as image type.
 **/
class ImageLoadStoreNonLayeredBindingTest : public ShaderImageLoadStoreBase
{
private:
	/* Structures */
	struct TextureShapeDefinition
	{
		GLuint m_edge;
		GLuint m_n_elements;
		GLenum m_type;

		TextureShapeDefinition(GLuint edge, GLuint n_elements, GLenum type)
			: m_edge(edge), m_n_elements(n_elements), m_type(type)
		{
		}
	};

	/* Typedefs */
	typedef std::deque<TextureShapeDefinition> TextureShapeDefinitionList;

	/* Fields */
	GLuint					   m_destination_texture_id;
	GLuint					   m_program_id;
	TextureShapeDefinitionList m_texture_shape_definitions;
	GLuint					   m_source_texture_id;
	GLuint					   m_vertex_array_object_id;
	GLuint					   m_vertex_buffer_id;

public:
	/* Constructor */
	ImageLoadStoreNonLayeredBindingTest()
		: m_destination_texture_id(0)
		, m_program_id(0)
		, m_source_texture_id(0)
		, m_vertex_array_object_id(0)
		, m_vertex_buffer_id(0)
	{
		/* Nothing to be done here */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Setup()
	{
		/* Shaders code */
		const char* const vertex_shader_code = "#version 400 core\n"
											   "#extension GL_ARB_shader_image_load_store : require\n"
											   "\n"
											   "precision highp float;\n"
											   "\n"
											   "in vec4 vs_in_position;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    gl_Position = vs_in_position;\n"
											   "}\n";

		const char* const fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(rgba8) writeonly uniform image2D u_destination_image;\n"
			"layout(rgba8) readonly  uniform image2D u_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vec4 loaded_color = imageLoad (u_source_image,      ivec2(gl_FragCoord));\n"
			"                        imageStore(u_destination_image, ivec2(gl_FragCoord), loaded_color);\n"
			"\n"
			"    discard;\n"
			"}\n";

		/* Vertex postions for "full screen" quad, defined as a triangle strip */
		static const GLfloat m_vertex_buffer_data[] = {
			-1.0f, -1.0f, 0.0f, 1.0f, /* left bottom */
			-1.0f, 1.0f,  0.0f, 1.0f, /* left top */
			1.0f,  -1.0f, 0.0f, 1.0f, /* right bottom */
			1.0f,  1.0f,  0.0f, 1.0f, /* right top */
		};

		/* Result of BuildProgram operation */
		bool is_program_correct = true; /* BuildProgram set false when it fails, but it does not set true on success */

		/* Add all tested texture shapes */
		int texture_edge = de::min(64, de::min(getWindowHeight(), getWindowWidth()));
		m_texture_shape_definitions.push_back(
			TextureShapeDefinition(texture_edge /* edge */, 6 /* n_elements */, GL_TEXTURE_2D_ARRAY));
		m_texture_shape_definitions.push_back(
			TextureShapeDefinition(texture_edge /* edge */, 6 /* n_elements */, GL_TEXTURE_3D));
		m_texture_shape_definitions.push_back(
			TextureShapeDefinition(texture_edge /* edge */, 1 /* n_elements */, GL_TEXTURE_CUBE_MAP));
		m_texture_shape_definitions.push_back(
			TextureShapeDefinition(texture_edge /* edge */, 3 /* n_elements */, GL_TEXTURE_CUBE_MAP_ARRAY));

		/* Prepare buffer with vertex positions of "full screen" quad" */
		glGenBuffers(1, &m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenBuffers");

		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertex_buffer_data), m_vertex_buffer_data, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(glGetError(), "BufferData");

		/* Generate vertex array object */
		glGenVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenVertexArrays");

		/* Prepare program object */
		m_program_id = BuildProgram(vertex_shader_code, 0 /* src_tcs */, 0 /* src_tes */, 0 /* src_gs */,
									fragment_shader_code, &is_program_correct);
		if (false == is_program_correct)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		/* Reset OpenGL state */
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		glBindTexture(GL_TEXTURE_3D, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		/* Delete program */
		if (0 != m_program_id)
		{
			glDeleteProgram(m_program_id);
			m_program_id = 0;
		}

		/* Delete textures */
		if (0 != m_destination_texture_id)
		{
			glDeleteTextures(1, &m_destination_texture_id);
			m_destination_texture_id = 0;
		}

		if (0 != m_source_texture_id)
		{
			glDeleteTextures(1, &m_source_texture_id);
			m_source_texture_id = 0;
		}

		/* Delete vertex array object */
		if (0 != m_vertex_array_object_id)
		{
			glDeleteVertexArrays(1, &m_vertex_array_object_id);
			m_vertex_array_object_id = 0;
		}

		/* Delete buffer */
		if (0 != m_vertex_buffer_id)
		{
			glDeleteBuffers(1, &m_vertex_buffer_id);
			m_vertex_buffer_id = 0;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool result = true;

		/* For each shape */
		for (TextureShapeDefinitionList::const_iterator it = m_texture_shape_definitions.begin();
			 m_texture_shape_definitions.end() != it; ++it)
		{
			const GLuint n_layers = GetTotalNumberOfLayers(it->m_n_elements, it->m_type);

			/* Prepare "source" and "destination" textures */
			GLU_EXPECT_NO_ERROR(
				CreateRGBA8DestinationTexture(it->m_edge, it->m_n_elements, it->m_type, m_destination_texture_id),
				"Create2DR8UIDestinationTexture");
			GLU_EXPECT_NO_ERROR(CreateRGBA8SourceTexture(it->m_edge, it->m_n_elements, it->m_type, m_source_texture_id),
								"Create2DR8UISourceTexture");

			/* Copy texture data with imageLoad() and imageStore() operations */
			CopyRGBA8Texture(m_destination_texture_id, m_source_texture_id, n_layers);

			/* Compare "source" and "destination" textures */
			if (false ==
				CompareRGBA8Textures(m_destination_texture_id, m_source_texture_id, it->m_edge, n_layers, it->m_type))
			{
				const char* texture_type = "";
				switch (it->m_type)
				{
				case GL_TEXTURE_2D_ARRAY:
					texture_type = "2d array";
					break;
				case GL_TEXTURE_3D:
					texture_type = "3d";
					break;
				case GL_TEXTURE_CUBE_MAP:
					texture_type = "Cube map";
					break;
				case GL_TEXTURE_CUBE_MAP_ARRAY:
					texture_type = "Cube map array";
					break;
				}

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Copy  with imageLoad and imageStore failed for texture type: " << texture_type
					<< ". Source and destination textures are different" << tcu::TestLog::EndMessage;

				result = false;
			}

			/* Destroy "source" and "destination" textures */
			glDeleteTextures(1, &m_destination_texture_id);
			glDeleteTextures(1, &m_source_texture_id);

			m_destination_texture_id = 0;
			m_source_texture_id		 = 0;
		}

		if (false == result)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

private:
	/* Private methods */

	/** Binds a texture to user-specified image unit and update relevant sampler uniform
	 *
	 * @param program_id   Program object id
	 * @param texture_id   Texture id
	 * @param image_unit   Index of image unit
	 * @param layer        Index of layer bound to unit
	 * @param uniform_name Name of image uniform
	 **/
	void BindTextureToImage(GLuint program_id, GLuint texture_id, GLuint image_unit, GLuint layer,
							const char* uniform_name)
	{
		static const GLint invalid_uniform_location = -1;
		GLint			   image_uniform_location   = 0;

		/* Get uniform location */
		image_uniform_location = glGetUniformLocation(program_id, uniform_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");
		if (invalid_uniform_location == image_uniform_location)
		{
			throw tcu::InternalError("Uniform location is not available", uniform_name, __FILE__, __LINE__);
		}

		/* Bind texture to image unit */
		glBindImageTexture(image_unit, texture_id, 0 /* level */, GL_FALSE /* layered */, layer, GL_READ_WRITE,
						   GL_RGBA8);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindImageTexture");

		/* Set uniform to image unit */
		glUniform1i(image_uniform_location, image_unit);
		GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");
	}

	/** Compare two 2D R8UI textures
	 *
	 * @param left_texture_id  Id of "left" texture object
	 * @param right_texture_id Id of "right" texture object
	 * @param edge             Length of texture edge
	 * @param n_layers         Number of layers to compare
	 * @param type             Type of texture
	 *
	 * @return true when texture data is found identical, false otherwise
	 **/
	bool CompareRGBA8Textures(GLuint left_texture_id, GLuint right_texture_id, GLuint edge, GLuint n_layers,
							  GLenum type)
	{
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = edge * edge * n_layers * n_components;

		/* Storage for texture data */
		std::vector<GLubyte> left_texture_data;
		std::vector<GLubyte> right_texture_data;

		ExtractTextureData(left_texture_id, edge, n_layers, type, left_texture_data);
		ExtractTextureData(right_texture_id, edge, n_layers, type, right_texture_data);

		/* Compare texels */
		return (0 == memcmp(&left_texture_data[0], &right_texture_data[0], texture_data_size));
	}

	/** Copy contents of "source" texture to "destination" texture with imageLoad() and imageStore() operations
	 *
	 * @param destination_texture_id Id of "destination" texture object
	 * @param source_texture_id      Id of "source" texture object
	 * @param n_layers               Number of layers
	 **/
	void CopyRGBA8Texture(GLuint destination_texture_id, GLuint source_texture_id, GLuint n_layers)
	{
		for (GLuint layer = 0; layer < n_layers; ++layer)
		{
			CopyRGBA8TextureLayer(destination_texture_id, source_texture_id, layer);
		}
	}

	/** Copy contents of layer from "source" texture to "destination" texture with imageLoad() and imageStore() operations
	 *
	 * @param destination_texture_id Id of "destination" texture object
	 * @param source_texture_id      Id of "source" texture object
	 * @param layer                  Index of layer
	 **/
	void CopyRGBA8TextureLayer(GLuint destination_texture_id, GLuint source_texture_id, GLuint layer)
	{
		/* Uniform names */
		static const char* const destination_image_uniform_name = "u_destination_image";
		static const char* const source_image_uniform_name		= "u_source_image";

		/* Attribute name */
		static const char* const position_attribute_name = "vs_in_position";

		/* Attribute location and invalid value */
		static const GLint invalid_attribute_location  = -1;
		GLint			   position_attribute_location = 0;

		/* Set current program */
		glUseProgram(m_program_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "UseProgram");

		/* Bind vertex array object */
		glBindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindVertexArray");

		/* Bind buffer with quad vertex positions */
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		/* Set up vertex attribute array for position attribute */
		position_attribute_location = glGetAttribLocation(m_program_id, position_attribute_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetAttribLocation");
		if (invalid_attribute_location == position_attribute_location)
		{
			throw tcu::InternalError("Attribute location is not available", position_attribute_name, __FILE__,
									 __LINE__);
		}

		glVertexAttribPointer(position_attribute_location, 4 /*size */, GL_FLOAT, GL_FALSE /* normalized */,
							  0 /* stride */, 0 /* pointer */);
		GLU_EXPECT_NO_ERROR(glGetError(), "VertexAttribPointer");

		glEnableVertexAttribArray(position_attribute_location);
		GLU_EXPECT_NO_ERROR(glGetError(), "EnableVertexAttribArray");

		/* Set up textures as source and destination image samplers */
		BindTextureToImage(m_program_id, destination_texture_id, 0 /* image_unit */, layer,
						   destination_image_uniform_name);
		BindTextureToImage(m_program_id, source_texture_id, 1 /* image_unit */, layer, source_image_uniform_name);

		/* Execute draw */
		glDrawArrays(GL_TRIANGLE_STRIP, 0 /* first vertex */, 4 /* number of vertices */);
		GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");
	}

	/** Creates RGBA8 texture of given type and fills it with zeros
	 *
	 * @param edge           Edge of created texture
	 * @param n_elements     Number of elements in texture array
	 * @param target         Target of created texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum CreateRGBA8DestinationTexture(GLuint edge, GLuint n_elements, GLenum target, GLuint& out_texture_id)
	{
		/* Constasts to calculate texture size */
		static const GLuint n_components = 4; /* RGBA */
		const GLuint		layer_size   = edge * edge * n_components;
		const GLuint		n_layers	 = GetTotalNumberOfLayers(n_elements, target);
		const GLuint		texture_size = layer_size * n_layers;

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_size);

		/* Set all texels */
		for (GLuint i = 0; i < texture_size; ++i)
		{
			texture_data[i] = 0;
		}

		/* Create texture */
		return CreateRGBA8Texture(edge, target, n_layers, texture_data, out_texture_id);
	}

	/** Creates RGBA8 texture and fills it with [x, y, layer, 0xaa]
	 *
	 * @param edge           Edge of created texture
	 * @param n_elements     Number of elements in texture array
	 * @param target         Target of created texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum CreateRGBA8SourceTexture(GLuint edge, GLuint n_elements, GLenum target, GLuint& out_texture_id)
	{
		/* Constants to calculate texture size */
		static const GLuint n_components = 4; /* RGBA */
		const GLuint		layer_size   = edge * edge * n_components;
		const GLuint		n_layers	 = GetTotalNumberOfLayers(n_elements, target);
		const GLuint		texture_size = layer_size * n_layers;

		/* Value of texel */
		GLubyte texel[4] = { 0, 0, 0, 0xaa };

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_size);

		/* Set all texels */
		for (GLuint layer = 0; layer < n_layers; ++layer)
		{
			const GLuint layer_offset = layer_size * layer;

			texel[2] = static_cast<GLubyte>(layer);

			for (GLuint y = 0; y < edge; ++y)
			{
				const GLuint line_offset = y * edge * n_components + layer_offset;

				texel[1] = static_cast<GLubyte>(y);

				for (GLuint x = 0; x < edge; ++x)
				{
					const GLuint texel_offset = x * n_components + line_offset;
					texel[0]				  = static_cast<GLubyte>(x);

					for (GLuint component = 0; component < n_components; ++component)
					{
						texture_data[texel_offset + component] = texel[component];
					}
				}
			}
		}

		/* Create texture */
		return CreateRGBA8Texture(edge, target, n_layers, texture_data, out_texture_id);
	}

	/** Creates RGBA8 texture of given type and fills it provided data
	 *
	 * @param edge           Edge of created texture
	 * @param n_elements     Number of elements in texture array
	 * @param target         Target of created texture
	 * @param texture_data   Texture data
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum CreateRGBA8Texture(GLuint edge, GLenum target, GLuint n_layers, const std::vector<GLubyte>& texture_data,
							  GLuint& out_texture_id)
	{
		GLenum err		  = 0;
		GLuint texture_id = 0;

		/* Generate texture */
		glGenTextures(1, &texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			return err;
		}

		/* Bind texture */
		glBindTexture(target, texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Allocate storage and fill texture */
		if (GL_TEXTURE_CUBE_MAP != target)
		{
			glTexImage3D(target, 0 /* level */, GL_RGBA8, edge, edge, n_layers, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[0]);
		}
		else
		{
			const GLuint n_components = 4;
			const GLuint layer_size   = edge * edge * n_components;

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[0 * layer_size]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[1 * layer_size]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[2 * layer_size]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[3 * layer_size]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[4 * layer_size]);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0 /* level */, GL_RGBA8, edge, edge, 0 /* border */, GL_RGBA,
						 GL_UNSIGNED_BYTE, &texture_data[5 * layer_size]);
		}
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Make texture complete */
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Set out_texture_id */
		out_texture_id = texture_id;

		/* Done */
		return GL_NO_ERROR;
	}

	/** Extracts texture data
	 *
	 * @param texture_id   Id of texture object
	 * @param edge         Length of texture edge
	 * @param n_layers     Number of layers
	 * @param target       Target of texture
	 * @param texture_data Extracted texture data
	 **/
	void ExtractTextureData(GLuint texture_id, GLuint edge, GLuint n_layers, GLenum target,
							std::vector<GLubyte>& texture_data)
	{
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = edge * edge * n_layers * n_components;

		/* Alocate memory for texture data */
		texture_data.resize(texture_data_size);

		/* Bind texture */
		glBindTexture(target, texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		/* Get data */
		if (GL_TEXTURE_CUBE_MAP != target)
		{
			glGetTexImage(target, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
		}
		else
		{
			const GLuint layer_size = edge * edge * n_components;

			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0 * layer_size]);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[1 * layer_size]);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[2 * layer_size]);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[3 * layer_size]);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[4 * layer_size]);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[5 * layer_size]);
		}

		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");
	}

	/** Get number of layers per single element for given type of texture
	 *
	 * @param target Target of texture
	 *
	 * @return Number of layers
	 **/
	GLuint GetLayersPerElement(GLenum target)
	{
		switch (target)
		{
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
			return 1;
			break;
		case GL_TEXTURE_CUBE_MAP:
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			return 6;
			break;
		default:
			throw tcu::InternalError("Not supported texture type", "", __FILE__, __LINE__);
			break;
		}
	}

	/** Get total number of layers in texture of given type and number of array elements
	 *
	 * @param n_elements Number of elements in texture array
	 * @param target     Target of texture
	 *
	 * @return Number of layers
	 **/
	GLuint GetTotalNumberOfLayers(GLuint n_elements, GLenum target)
	{
		return GetLayersPerElement(target) * n_elements;
	}
};

/** Test "imageLoad() and imageStore() for incomplete textures" description follows.
 *
 *  Load from incomplete textures should return 0.
 *  Store to incomplete textures should be ignored.
 *
 *  Steps:
 *  - create two incomplete textures: "incomplete_source" and
 *  "incomplete_destination",
 *  - create two complete textures: "complete_source" and
 *  "complete_destination",
 *  - fill all textures with unique values,
 *  - prepare program that will:
 *      * load texel from "incomplete_source" and store its value to
 *      "complete_destination",
 *      * load texel from "complete_source" and store its value to
 *      "incomplete_destination".
 *  - bind textures to corresponding image uniforms
 *  - execute program for all texels,
 *  - verify that "incomplete_destination" was not modified and
 *  "complete_destination" is filled with zeros.
 *
 *  Texture is considered incomplete when it has enabled mipmaping (see below)
 *  and does not have all mipmap levels defined.  But for the case of Image
 *  accessing, it is considered invalid if it is mipmap-incomplete and the
 *  level is different to the base level (base-incomplete).
 *
 *  Creation of incomplete texture:
 *  - generate and bind texture object id,
 *  - call TexImage2D with <level>: 0,
 *  - set GL_TEXTURE_MIN_FILTER? parameter to GL_NEAREST_MIPMAP_LINEAR, (to make
 *  sure, it should be initial value),
 *  - set GL_TEXTURE_BASE_LEVEL parameter to 0.
 *  - set GL_TEXTURE_MAX_LEVEL parameter, for 64x64 set 7 (log2(min(width,
 *  height)).
 *
 *  Creation of complete texture:
 *  - generate and bind texture object id,
 *  - call TexImage2D with <level>: 0,
 *  - set GL_TEXTURE_BASE_LEVEL parameter to 0.
 *  - set GL_TEXTURE_MAX_LEVEL parameter to 0.
 *
 *  Binding:
 *  - Set level == base_level for complete destinations.
 *  - Set level != base_level for incomplete destinations that are using
 *    mipmap-incomplete textures.
 *
 *  Test with 2D 64x64 RGBA8 textures.
 *
 *  Program should consist of vertex and fragment shader. Vertex shader should
 *  pass vertex position through. Fragment shader should do imageLoad() and
 *  imageStore() operations at coordinates gl_FragCoord.
 **/
class ImageLoadStoreIncompleteTexturesTest : public ShaderImageLoadStoreBase
{
private:
	/* Constants */
	/* Magic numbers that will identify textures, which will be used as their
	 * texel value.
	 */
	static const GLubyte m_complete_destination_magic_number   = 0x11;
	static const GLubyte m_complete_source_magic_number		   = 0x22;
	static const GLubyte m_incomplete_destination_magic_number = 0x33;
	static const GLubyte m_incomplete_source_magic_number	  = 0x44;

	/* Texture edge */
	GLuint m_texture_edge;

	/* Fields */
	GLuint m_complete_destination_texture_id;
	GLuint m_complete_source_texture_id;
	GLuint m_incomplete_destination_texture_id;
	GLuint m_incomplete_source_texture_id;
	GLuint m_program_id;
	GLuint m_vertex_array_object_id;
	GLuint m_vertex_buffer_id;

public:
	/* Constructor */
	ImageLoadStoreIncompleteTexturesTest()
		: m_texture_edge(0)
		, m_complete_destination_texture_id(0)
		, m_complete_source_texture_id(0)
		, m_incomplete_destination_texture_id(0)
		, m_incomplete_source_texture_id(0)
		, m_program_id(0)
		, m_vertex_array_object_id(0)
		, m_vertex_buffer_id(0)
	{
		/* Nothing to be done here */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Setup()
	{
		/* Shaders code */
		const char* const vertex_shader_code = "#version 400 core\n"
											   "#extension GL_ARB_shader_image_load_store : require\n"
											   "\n"
											   "precision highp float;\n"
											   "\n"
											   "in vec4 vs_in_position;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    gl_Position = vs_in_position;\n"
											   "}\n";

		const char* const fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(rgba8) writeonly uniform image2D u_complete_destination_image;\n"
			"layout(rgba8) readonly  uniform image2D u_complete_source_image;\n"
			"layout(rgba8) writeonly uniform image2D u_incomplete_destination_image;\n"
			"layout(rgba8) readonly  uniform image2D u_incomplete_source_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vec4 complete_loaded_color   = imageLoad (u_complete_source_image,   ivec2(gl_FragCoord));\n"
			"    vec4 incomplete_loaded_color = imageLoad (u_incomplete_source_image, ivec2(gl_FragCoord));\n"

			"    imageStore(u_complete_destination_image,\n"
			"               ivec2(gl_FragCoord),\n"
			"               incomplete_loaded_color);\n"
			"    imageStore(u_incomplete_destination_image,\n"
			"               ivec2(gl_FragCoord),\n"
			"               complete_loaded_color);\n"
			"\n"
			"    discard;\n"
			"}\n";

		/* Vertex postions for "full screen" quad, made with triangle strip */
		static const GLfloat m_vertex_buffer_data[] = {
			-1.0f, -1.0f, 0.0f, 1.0f, /* left bottom */
			-1.0f, 1.0f,  0.0f, 1.0f, /* left top */
			1.0f,  -1.0f, 0.0f, 1.0f, /* right bottom */
			1.0f,  1.0f,  0.0f, 1.0f, /* right top */
		};

		/* Result of BuildProgram operation */
		bool is_program_correct = true; /* BuildProgram set false when it fails, but it does not set true on success */

		/* Clean previous error */
		glGetError();

		m_texture_edge = de::min(64, de::min(getWindowHeight(), getWindowWidth()));

		/* Prepare textures */
		GLU_EXPECT_NO_ERROR(
			Create2DRGBA8CompleteTexture(m_complete_destination_magic_number, m_complete_destination_texture_id),
			"Create2DRGBA8CompleteTexture");
		GLU_EXPECT_NO_ERROR(Create2DRGBA8CompleteTexture(m_complete_source_magic_number, m_complete_source_texture_id),
							"Create2DRGBA8CompleteTexture");
		GLU_EXPECT_NO_ERROR(
			Create2DRGBA8IncompleteTexture(m_incomplete_destination_magic_number, m_incomplete_destination_texture_id),
			"Create2DRGBA8IncompleteTexture");
		GLU_EXPECT_NO_ERROR(
			Create2DRGBA8IncompleteTexture(m_incomplete_source_magic_number, m_incomplete_source_texture_id),
			"Create2DRGBA8IncompleteTexture");

		/* Prepare buffer with vertex positions of "full screen" quad" */
		glGenBuffers(1, &m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenBuffers");

		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertex_buffer_data), m_vertex_buffer_data, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(glGetError(), "BufferData");

		/* Generate vertex array object */
		glGenVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenVertexArrays");

		/* Prepare program object */
		m_program_id = BuildProgram(vertex_shader_code, 0 /* src_tcs */, 0 /* src_tes */, 0 /*src_gs */,
									fragment_shader_code, &is_program_correct);

		if (false == is_program_correct)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		/* Reset OpenGL state */
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);

		/* Delete program */
		if (0 != m_program_id)
		{
			glDeleteProgram(m_program_id);
			m_program_id = 0;
		}

		/* Delete textures */
		if (0 != m_complete_destination_texture_id)
		{
			glDeleteTextures(1, &m_complete_destination_texture_id);
			m_complete_destination_texture_id = 0;
		}

		if (0 != m_complete_source_texture_id)
		{
			glDeleteTextures(1, &m_complete_source_texture_id);
			m_complete_source_texture_id = 0;
		}

		if (0 != m_incomplete_destination_texture_id)
		{
			glDeleteTextures(1, &m_incomplete_destination_texture_id);
			m_incomplete_destination_texture_id = 0;
		}

		if (0 != m_incomplete_source_texture_id)
		{
			glDeleteTextures(1, &m_incomplete_source_texture_id);
			m_incomplete_source_texture_id = 0;
		}

		/* Delete vertex array object */
		if (0 != m_vertex_array_object_id)
		{
			glDeleteVertexArrays(1, &m_vertex_array_object_id);
			m_vertex_array_object_id = 0;
		}

		/* Delete buffer */
		if (0 != m_vertex_buffer_id)
		{
			glDeleteBuffers(1, &m_vertex_buffer_id);
			m_vertex_buffer_id = 0;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool result = true;

		/* Copy textures data with imageLoad() and imageStore() operations */
		Copy2DRGBA8Textures(m_complete_destination_texture_id, m_incomplete_destination_texture_id,
							m_complete_source_texture_id, m_incomplete_source_texture_id);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		/* Verify that store to "incomplete destination" was ignored */
		if (true ==
			CheckIfTextureWasModified(m_incomplete_destination_texture_id, m_incomplete_destination_magic_number))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Problem with imageStore, operation modified contents of incomplete texture"
				<< tcu::TestLog::EndMessage;

			result = false;
		}

		/* Verify that load from "incomplete source" returned 0 */
		if (false == CheckIfTextureIsBlack(m_complete_destination_texture_id))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Problem with imageLoad, operation returned non 0 result for incomplete texture"
				<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (false == result)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

private:
	/* Private methods */

	/** Bind texture to image unit and sets image uniform to that unit
	 *
	 * @param program_id   Program object id
	 * @param texture_id   Texture id
	 * @param level        Texture level
	 * @param image_unit   Index of image unit
	 * @param uniform_name Name of image uniform
	 **/
	void BindTextureToImage(GLuint program_id, GLuint texture_id, GLint level, GLuint image_unit, const char* uniform_name)
	{
		/* Uniform location and invalid value */
		static const GLint invalid_uniform_location = -1;
		GLint			   image_uniform_location   = 0;

		/* Get uniform location */
		image_uniform_location = glGetUniformLocation(program_id, uniform_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");
		if (invalid_uniform_location == image_uniform_location)
		{
			throw tcu::InternalError("Uniform location is not available", uniform_name, __FILE__, __LINE__);
		}

		/* Bind texture to image unit */
		glBindImageTexture(image_unit, texture_id, level, GL_FALSE, 0 /* layer */, GL_READ_WRITE, GL_RGBA8);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindImageTexture");

		/* Set uniform to image unit */
		glUniform1i(image_uniform_location, image_unit);
		GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");
	}

	/** Check if texture is filled with black color, zeros
	 *
	 * @param texture_id Id of texture object
	 *
	 * @return true when texture is fully black, false otherwise
	 **/
	bool CheckIfTextureIsBlack(GLuint texture_id)
	{
		/* Constants to calculate size of texture */
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = m_texture_edge * m_texture_edge * n_components;

		/* Storage for texture data */
		std::vector<GLubyte> black_texture_data;
		std::vector<GLubyte> texture_data;

		/* Allocate memory */
		black_texture_data.resize(texture_data_size);
		texture_data.resize(texture_data_size);

		/* Set all texels to black */
		for (GLuint i = 0; i < texture_data_size; ++i)
		{
			black_texture_data[i] = 0;
		}

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		/* Compare texels */
		return (0 == memcmp(&texture_data[0], &black_texture_data[0], texture_data_size));
	}

	/** Check if texture was modified
	 *
	 * @param texture_id   Id of texture object
	 * @param nagic_number Magic number that was to create texture
	 *
	 * @return true if texture contents match expected values, false otherwise
	 **/
	bool CheckIfTextureWasModified(GLuint texture_id, GLubyte magic_number)
	{
		/* Constants to calculate size of texture */
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = m_texture_edge * m_texture_edge * n_components;

		/* Storage for texture data */
		std::vector<GLubyte> expected_texture_data;
		std::vector<GLubyte> texture_data;

		/* Allocate memory */
		expected_texture_data.resize(texture_data_size);
		texture_data.resize(texture_data_size);

		/* Prepare expected texels */
		for (GLuint y = 0; y < m_texture_edge; ++y)
		{
			const GLuint line_offset = y * m_texture_edge * n_components;

			for (GLuint x = 0; x < m_texture_edge; ++x)
			{
				const GLuint texel_offset = x * n_components + line_offset;

				SetTexel(&expected_texture_data[texel_offset], static_cast<GLubyte>(x), static_cast<GLubyte>(y),
						 magic_number);
			}
		}

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		/* Compare texels, true when textures are different */
		return (0 != memcmp(&texture_data[0], &expected_texture_data[0], texture_data_size));
	}

	/** Copy contents of "source" textures to "destination" textures with imageLoad() and imageStore() operations
	 *
	 * @param complete_destination_texture_id   Id of "complete destination" texture object
	 * @param incomplete_destination_texture_id Id of "incomplete destination" texture object
	 * @param complete_source_texture_id        Id of "complete source" texture object
	 * @param incomplete_source_texture_id      Id of "incomplete source" texture object
	 **/
	void Copy2DRGBA8Textures(GLuint complete_destination_texture_id, GLuint incomplete_destination_texture_id,
							 GLuint complete_source_texture_id, GLuint incomplete_source_texture_id)
	{
		/* Uniform names */
		static const char* const complete_destination_image_uniform_name   = "u_complete_destination_image";
		static const char* const complete_source_image_uniform_name		   = "u_complete_source_image";
		static const char* const incomplete_destination_image_uniform_name = "u_incomplete_destination_image";
		static const char* const incomplete_source_image_uniform_name	  = "u_incomplete_source_image";

		/* Attribute name */
		static const char* const position_attribute_name = "vs_in_position";

		/* Attribute location and invalid value */
		static const GLint invalid_attribute_location  = -1;
		GLint			   position_attribute_location = 0;

		/* Set current program */
		glUseProgram(m_program_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "UseProgram");

		/* Bind vertex array object */
		glBindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindVertexArray");

		/* Bind buffer with quad vertex positions */
		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindBuffer");

		/* Setup position attribute */
		position_attribute_location = glGetAttribLocation(m_program_id, position_attribute_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetAttribLocation");
		if (invalid_attribute_location == position_attribute_location)
		{
			throw tcu::InternalError("Attribute location is not available", position_attribute_name, __FILE__,
									 __LINE__);
		}

		glVertexAttribPointer(position_attribute_location, 4 /*size */, GL_FLOAT, GL_FALSE, 0 /* stride */, 0);
		GLU_EXPECT_NO_ERROR(glGetError(), "VertexAttribPointer");

		glEnableVertexAttribArray(position_attribute_location);
		GLU_EXPECT_NO_ERROR(glGetError(), "EnableVertexAttribArray");

		/* Setup textures as source and destination images */
		BindTextureToImage(m_program_id, complete_destination_texture_id,
						   0 /* texture level */, 0 /* image_unit */,
						   complete_destination_image_uniform_name);
		BindTextureToImage(m_program_id, complete_source_texture_id,
						   0 /* texture level */, 1 /* image_unit */,
						   complete_source_image_uniform_name);
		BindTextureToImage(m_program_id, incomplete_destination_texture_id,
						   2 /* texture level */, 2 /* image_unit */,
						   incomplete_destination_image_uniform_name);
		BindTextureToImage(m_program_id, incomplete_source_texture_id,
						   2 /* texture level */, 3 /* image_unit */,
						   incomplete_source_image_uniform_name);

		/* Execute draw */
		glDrawArrays(GL_TRIANGLE_STRIP, 0 /* first vertex */, 4 /* number of vertices */);
		GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");
	}

	/** Create complete 2D RGBA8 texture.
	 *
	 * @param magic_number   Magic number of texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum Create2DRGBA8CompleteTexture(GLubyte magic_number, GLuint& out_texture_id)
	{
		/* Constants to calculate size of texture */
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = m_texture_edge * m_texture_edge * n_components;

		/* Error code */
		GLenum err = 0;

		/* Texture id */
		GLuint texture_id = 0;

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_data_size);

		/* Prepare texture data */
		for (GLuint y = 0; y < m_texture_edge; ++y)
		{
			const GLuint line_offset = y * m_texture_edge * n_components;

			for (GLuint x = 0; x < m_texture_edge; ++x)
			{
				const GLuint texel_offset = x * n_components + line_offset;

				SetTexel(&texture_data[texel_offset], static_cast<GLubyte>(x), static_cast<GLubyte>(y), magic_number);
			}
		}

		/* Generate texture */
		glGenTextures(1, &texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			return err;
		}

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Allocate storage and fill texture */
		glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA8, m_texture_edge, m_texture_edge, 0 /* border */, GL_RGBA,
					 GL_UNSIGNED_BYTE, &texture_data[0]);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Make texture complete */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Set out_texture_id */
		out_texture_id = texture_id;

		/* Done */
		return GL_NO_ERROR;
	}

	/** Create incomplete 2D RGBA8 texture
	 *
	 * @param magic_number   Magic number of texture
	 * @param out_texture_id Id of created texture, not modified if operation fails
	 *
	 * @return GL_NO_ERROR if operation was successful, GL error code otherwise
	 **/
	GLenum Create2DRGBA8IncompleteTexture(GLubyte magic_number, GLuint& out_texture_id)
	{
		/* Constants to calculate size of texture */
		static const GLuint n_components	  = 4; /* RGBA */
		const GLuint		texture_data_size = m_texture_edge * m_texture_edge * n_components;

		/* Error code */
		GLenum err = 0;

		/* Texture id */
		GLuint texture_id = 0;

		/* Prepare storage for texture data */
		std::vector<GLubyte> texture_data;
		texture_data.resize(texture_data_size);

		/* Prepare texture data */
		for (GLuint y = 0; y < m_texture_edge; ++y)
		{
			const GLuint line_offset = y * m_texture_edge * n_components;

			for (GLuint x = 0; x < m_texture_edge; ++x)
			{
				const GLuint texel_offset = x * n_components + line_offset;

				SetTexel(&texture_data[texel_offset], static_cast<GLubyte>(x), static_cast<GLubyte>(y), magic_number);
			}
		}

		/* Generate texture */
		glGenTextures(1, &texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			return err;
		}

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Allocate storage and fill texture */
		glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA8, m_texture_edge, m_texture_edge, 0 /* border */, GL_RGBA,
					 GL_UNSIGNED_BYTE, &texture_data[0]);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Make texture incomplete */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 7);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		err = glGetError();
		if (GL_NO_ERROR != err)
		{
			glDeleteTextures(1, &texture_id);
			return err;
		}

		/* Set out_texture_id */
		out_texture_id = texture_id;

		/* Done */
		return GL_NO_ERROR;
	}

	/** Prepare "unique" texels.
	 *  Texel is assigned with such values: [x_coordinate, y_coordinate, magic_number, 0xcc].
	 *
	 * @param texel        Storage of texel
	 * @param x_coordinate X coordiante of texel
	 * @param y_coordinate Y coordinate of texel
	 * @param magic_number Magic number of texture
	 **/
	void SetTexel(GLubyte texel[4], GLubyte x_coordinate, GLubyte y_coordinate, GLubyte magic_number)
	{
		texel[0] = x_coordinate;
		texel[1] = y_coordinate;
		texel[2] = magic_number;
		texel[3] = 0xcc;
	}
};

/** Test "Refer to the same image unit using multiple uniforms", description follows.
 *
 * Steps:
 * - prepare program object, see details below,
 * - prepare 2D R32I texture, width should be equal to the number of image
 * uniforms used by program object, height should be 2, fill first row with
 * unique values, fill second row with zeros,
 * - bind texture to first image unit,
 * - set all image uniforms to first image unit,
 * - execute program for a single vertex,
 * - verify that:
 *     - values in first row were negated,
 *     - values from first row were copied to second row,
 *
 * Repeat steps to test all shader stages that support at least 2 image
 * uniforms.
 *
 * Program has to contain all necessary shader stages. Use boilerplate shaders
 * for shader stages that are not important for the test.
 *
 * Tested shader stage should:
 * - Use as many different image formats as possible, image formats compatible
 * with R32I:
 *     * rg16f
 *     * r11f_g11f_b10f
 *     * r32f
 *     * rgb10_a2ui
 *     * rgba8ui
 *     * rg16ui
 *     * r32ui
 *     * rgba8i
 *     * rg16i
 *     * r32i
 *     * rgb10_a2
 *     * rgba8
 *     * rg16
 *     * rgba8_snorm
 *     * rg16_snorm.
 * - Declare maximum allowed number of image uniforms,
 *
 *     layout(format) uniform gimage2D u_image;
 *
 * where <format> is selected image format, <gimage2D> is type of 2D image
 * compatible with <format> and <u_image> is unique name of uniform.
 * Note that image uniforms cannot be declared as array, due to different image
 * formats. Therefore separate uniforms have to be used.
 * - Include following code snippet:
 * for (int i = 0; i < gl_Max*ImageUniforms; ++i)
 * {
 *     vec row_1_coord(i,0);
 *     vec row_2_coord(i,1);
 *
 *     row_1_value = imageLoad(u_image[i], row_1_coord);
 *     imageStore(u_image[i], row_1_coord, -row_1_value);
 *     imageStore(u_image[i], row_2_coord, row_1_value);
 * }
 * where gl_Max*ImageUniforms is the constant corresponding to tested shader
 * stage.
 **/
class ImageLoadStoreMultipleUniformsTest : public ShaderImageLoadStoreBase
{
private:
	/* Types */
	/** Details of image format
	 *
	 **/
	struct imageFormatDetails
	{
		typedef bool (*verificationRoutine)(GLint, GLint, GLint);

		const char*			m_image_format;
		const char*			m_image_type;
		const char*			m_color_type;
		GLenum				m_image_unit_format;
		verificationRoutine m_verification_routine;
	};

	template <typename T, GLuint SIZE, GLuint OFFSET, bool = (OFFSET < sizeof(T) * CHAR_BIT)>
	struct Masks
	{
		/** Get mask of bits used to store in bit-field
		 *
		 * @return Mask
		 **/
		static inline T RawMask()
		{
			static const T mask = ValueMask() << OFFSET;

			return mask;
		}

		/** Get mask of bits used to store value.
		 *
		 * @return Mask
		 **/
		static inline T ValueMask()
		{
			static const T mask = (1 << SIZE) - 1;

			return mask;
		}

		/** Get offset.
		 *
		 * @return offset
		 **/
		static inline T Offset()
		{
			return OFFSET;
		}
	};

	template <typename T, GLuint SIZE, GLuint OFFSET>
	struct Masks<T, SIZE, OFFSET, false>
	{
		/** Get mask of bits used to store in bit-field
		 *
		 * @return Mask
		 **/
		static inline T RawMask()
		{
			DE_ASSERT(DE_FALSE && "Shouldn't be called");
			return 0;
		}

		/** Get mask of bits used to store value.
		 *
		 * @return Mask
		 **/
		static inline T ValueMask()
		{
			DE_ASSERT(DE_FALSE && "Shouldn't be called");
			return 0;
		}

		/** Get offset.
		 *
		 * @return offset
		 **/
		static inline T Offset()
		{
			DE_ASSERT(DE_FALSE && "Shouldn't be called");
			return 0;
		}
	};

	/** Template class for accessing integer values stored in bit-fields
	 *
	 **/
	template <typename T, GLuint SIZE, GLuint OFFSET>
	class Integer
	{
	public:
		/** Constructor
		 *
		 **/
		Integer(T raw) : m_raw(raw)
		{
		}

		/** Extract value from bit-field
		 *
		 * @return Value
		 **/
		T Get() const
		{
			const T mask = Masks<T, SIZE, OFFSET>::RawMask();

			const T bits   = m_raw & mask;
			const T result = (unsigned)bits >> Masks<T, SIZE, OFFSET>::Offset();

			return result;
		}

		/** Extract value from bit-field and negate it
		 *
		 * @return Negated value
		 **/
		T GetNegated() const
		{
			const T mask  = Masks<T, SIZE, OFFSET>::ValueMask();
			const T value = Get();

			return Clamp((~value) + 1) & mask;
		}

		T Clamp(T n) const
		{
			const bool isUnsigned = (T(0) < T(-1));
			const T	min		  = T(isUnsigned ? 0.L : -pow(2.L, int(SIZE - 1)));
			const T	max		  = T(isUnsigned ? pow(2.L, int(SIZE)) - 1.L : pow(2.L, int(SIZE - 1)) - 1.L);
			const T	x		  = n > max ? max : n;
			return x < min ? min : x;
		}

	private:
		T m_raw;
	};

	/* Enums */
	/** Shader stage identification
	 *
	 **/
	enum shaderStage
	{
		fragmentShaderStage				 = 2,
		geometryShaderStage				 = 4,
		tesselationControlShaderStage	= 8,
		tesselationEvalutaionShaderStage = 16,
		vertexShaderStage				 = 32,
	};

	/** Test result
	 *
	 **/
	enum testResult
	{
		testFailed		 = -1,
		testNotSupported = 1,
		testPassed		 = 0
	};

	/* Constants */
	static const GLint m_min_required_image_uniforms = 2;

	/* Fields */
	GLuint m_program_to_test_fs_stage_id;
	GLuint m_program_to_test_gs_stage_id;
	GLuint m_program_to_test_tcs_stage_id;
	GLuint m_program_to_test_tes_stage_id;
	GLuint m_program_to_test_vs_stage_id;
	GLuint m_texture_to_test_fs_stage_id;
	GLuint m_texture_to_test_gs_stage_id;
	GLuint m_texture_to_test_tcs_stage_id;
	GLuint m_texture_to_test_tes_stage_id;
	GLuint m_texture_to_test_vs_stage_id;
	GLuint m_vertex_array_object_id;

public:
	/* Constructor */
	ImageLoadStoreMultipleUniformsTest()
		: m_program_to_test_fs_stage_id(0)
		, m_program_to_test_gs_stage_id(0)
		, m_program_to_test_tcs_stage_id(0)
		, m_program_to_test_tes_stage_id(0)
		, m_program_to_test_vs_stage_id(0)
		, m_texture_to_test_fs_stage_id(0)
		, m_texture_to_test_gs_stage_id(0)
		, m_texture_to_test_tcs_stage_id(0)
		, m_texture_to_test_tes_stage_id(0)
		, m_texture_to_test_vs_stage_id(0)
		, m_vertex_array_object_id(0)
	{
		/* Nothing to be done here */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Setup()
	{
		/* Prepare programs */
		m_program_to_test_fs_stage_id  = buildProgramToTestShaderStage(fragmentShaderStage);
		m_program_to_test_gs_stage_id  = buildProgramToTestShaderStage(geometryShaderStage);
		m_program_to_test_tcs_stage_id = buildProgramToTestShaderStage(tesselationControlShaderStage);
		m_program_to_test_tes_stage_id = buildProgramToTestShaderStage(tesselationEvalutaionShaderStage);
		m_program_to_test_vs_stage_id  = buildProgramToTestShaderStage(vertexShaderStage);

		/* Prepare textures */
		m_texture_to_test_fs_stage_id  = createTextureToTestShaderStage(fragmentShaderStage);
		m_texture_to_test_gs_stage_id  = createTextureToTestShaderStage(geometryShaderStage);
		m_texture_to_test_tcs_stage_id = createTextureToTestShaderStage(tesselationControlShaderStage);
		m_texture_to_test_tes_stage_id = createTextureToTestShaderStage(tesselationEvalutaionShaderStage);
		m_texture_to_test_vs_stage_id  = createTextureToTestShaderStage(vertexShaderStage);

		/* Generate vertex array object */
		glGenVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenVertexArrays");

		/* Bind vertex array object */
		glBindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindVertexArray");

		/* Set vertices number for patches */
		glPatchParameteri(GL_PATCH_VERTICES, 1);

		/* Done */
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);

		/* Delete programs */
		if (0 != m_program_to_test_fs_stage_id)
		{
			glDeleteProgram(m_program_to_test_fs_stage_id);
			m_program_to_test_fs_stage_id = 0;
		}

		if (0 != m_program_to_test_gs_stage_id)
		{
			glDeleteProgram(m_program_to_test_gs_stage_id);
			m_program_to_test_gs_stage_id = 0;
		}

		if (0 != m_program_to_test_tcs_stage_id)
		{
			glDeleteProgram(m_program_to_test_tcs_stage_id);
			m_program_to_test_tcs_stage_id = 0;
		}

		if (0 != m_program_to_test_tes_stage_id)
		{
			glDeleteProgram(m_program_to_test_tes_stage_id);
			m_program_to_test_tes_stage_id = 0;
		}

		if (0 != m_program_to_test_vs_stage_id)
		{
			glDeleteProgram(m_program_to_test_vs_stage_id);
			m_program_to_test_vs_stage_id = 0;
		}

		/* Delete textures */
		if (0 != m_texture_to_test_fs_stage_id)
		{
			glDeleteTextures(1, &m_texture_to_test_fs_stage_id);
			m_texture_to_test_fs_stage_id = 0;
		}

		if (0 != m_texture_to_test_gs_stage_id)
		{
			glDeleteTextures(1, &m_texture_to_test_gs_stage_id);
			m_texture_to_test_gs_stage_id = 0;
		}

		if (0 != m_texture_to_test_tcs_stage_id)
		{
			glDeleteTextures(1, &m_texture_to_test_tcs_stage_id);
			m_texture_to_test_tcs_stage_id = 0;
		}

		if (0 != m_texture_to_test_tes_stage_id)
		{
			glDeleteTextures(1, &m_texture_to_test_tes_stage_id);
			m_texture_to_test_tes_stage_id = 0;
		}

		if (0 != m_texture_to_test_vs_stage_id)
		{
			glDeleteTextures(1, &m_texture_to_test_vs_stage_id);
			m_texture_to_test_vs_stage_id = 0;
		}

		/* Delete vertex array object id */
		if (0 != m_vertex_array_object_id)
		{
			glDeleteVertexArrays(1, &m_vertex_array_object_id);
			m_vertex_array_object_id = 0;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool result = true;

		if (testFailed == testShaderStage(fragmentShaderStage))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Problems with fragment shader stage!"
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (testFailed == testShaderStage(geometryShaderStage))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Problems with geometry shader stage!"
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (testFailed == testShaderStage(tesselationControlShaderStage))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Problems with tesselation control shader stage!"
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (testFailed == testShaderStage(tesselationEvalutaionShaderStage))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Problems with tesselation evaluation shader stage!"
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (testFailed == testShaderStage(vertexShaderStage))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Problems with vertex shader stage!"
												<< tcu::TestLog::EndMessage;

			result = false;
		}

		if (false == result)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

private:
	/* Static routines */
	/** Provide image format details for given index
	 *
	 * @param index       Index
	 * @param out_details Image format detail instance
	 **/
	static void getImageUniformDeclarationDetails(GLuint index, imageFormatDetails& out_details)
	{
		static const ImageLoadStoreMultipleUniformsTest::imageFormatDetails default_format_details = {
			"r32i", "iimage2D", "ivec4", GL_R32I, ImageLoadStoreMultipleUniformsTest::verifyInteger
		};

		static const ImageLoadStoreMultipleUniformsTest::imageFormatDetails format_details[] = {
			{ "rg16f", "image2D", "vec4", GL_RG16F, ImageLoadStoreMultipleUniformsTest::verifyFloat<GLushort> },
			{ "r11f_g11f_b10f", "image2D", "vec4", GL_R11F_G11F_B10F,
			  ImageLoadStoreMultipleUniformsTest::verifyFloatUnsigned<GLuint> },
			{ "r32f", "image2D", "vec4", GL_R32F, ImageLoadStoreMultipleUniformsTest::verifyFloat<GLuint> },
			{ "rgb10_a2", "image2D", "vec4", GL_RGB10_A2,
			  ImageLoadStoreMultipleUniformsTest::verifyFloatUnsigned<GLuint> },
			{ "rgba8", "image2D", "vec4", GL_RGBA8, ImageLoadStoreMultipleUniformsTest::verifyFloatUnsigned<GLuint> },
			{ "rg16", "image2D", "vec4", GL_RG16, ImageLoadStoreMultipleUniformsTest::verifyFloatUnsigned<GLuint> },
			{ "rgba8_snorm", "image2D", "vec4", GL_RGBA8_SNORM,
			  ImageLoadStoreMultipleUniformsTest::verifyFloatSignedNorm<GLbyte> },
			{ "rg16_snorm", "image2D", "vec4", GL_RG16_SNORM,
			  ImageLoadStoreMultipleUniformsTest::verifyFloatSignedNorm<GLshort> },
			{ "rgb10_a2ui", "uimage2D", "uvec4", GL_RGB10_A2UI,
			  ImageLoadStoreMultipleUniformsTest::verifyInteger<10, 10, 10, 2, GLuint> },
			{ "rgba8ui", "uimage2D", "uvec4", GL_RGBA8UI,
			  ImageLoadStoreMultipleUniformsTest::verifyInteger<8, 8, 8, 8, GLuint> },
			{ "rg16ui", "uimage2D", "uvec4", GL_RG16UI,
			  ImageLoadStoreMultipleUniformsTest::verifyInteger<16, 16, 0, 0, GLuint> },
			{ "r32ui", "uimage2D", "uvec4", GL_R32UI, ImageLoadStoreMultipleUniformsTest::verifyInteger },
			{ "rgba8i", "iimage2D", "ivec4", GL_RGBA8I,
			  ImageLoadStoreMultipleUniformsTest::verifyInteger<8, 8, 8, 8, GLint> },
			{ "rg16i", "iimage2D", "ivec4", GL_RG16I,
			  ImageLoadStoreMultipleUniformsTest::verifyInteger<16, 16, 0, 0, GLint> }
		};

		static const GLuint n_imageUniformFormatDetails =
			sizeof(format_details) / sizeof(ImageLoadStoreMultipleUniformsTest::imageFormatDetails);

		if (n_imageUniformFormatDetails <= index)
		{
			out_details = default_format_details;
		}
		else
		{
			out_details = format_details[index];
		}
	}

	/** Write name of image uniform at given index to output stream
	 *
	 * @param stream Output stream
	 * @param index  Index
	 **/
	static void writeImageUniformNameToStream(std::ostream& stream, GLuint index)
	{
		/* u_image_0 */
		stream << "u_image_" << index;
	}

	/** Write name of variable used to store value loaded from image at given index to output stream
	 *
	 * @param stream Output stream
	 * @param index  Index
	 **/
	static void writeLoadedValueVariableNameToStream(std::ostream& stream, GLuint index)
	{
		/* loaded_value_0 */
		stream << "loaded_value_" << index;
	}

	/** Write name of variable used to store coordinate of texel at given row to output stream
	 *
	 * @param stream Output stream
	 * @param index  Index of image uniform
	 * @param row    Row of image
	 **/
	static void writeCoordinatesVariableNameToStream(std::ostream& stream, GLuint index, GLuint row)
	{
		/* row_0_coordinates_0 */
		stream << "row_" << row << "_coordinates_" << index;
	}

	struct imageUniformDeclaration
	{
		imageUniformDeclaration(GLuint index) : m_index(index)
		{
		}

		GLuint m_index;
	};

	/** Write declaration of image uniform at given index to output stream
	 *
	 * @param stream                   Output stream
	 * @param imageUniformDeclaration  Declaration details
	 *
	 * @return stream
	 **/
	friend std::ostream& operator<<(std::ostream& stream, const imageUniformDeclaration& declaration)
	{
		ImageLoadStoreMultipleUniformsTest::imageFormatDetails format_details;
		getImageUniformDeclarationDetails(declaration.m_index, format_details);

		/* layout(r32f) uniform image2D u_image_0; */
		stream << "layout(" << format_details.m_image_format << ") uniform " << format_details.m_image_type << " ";

		ImageLoadStoreMultipleUniformsTest::writeImageUniformNameToStream(stream, declaration.m_index);

		stream << ";";

		return stream;
	}

	struct imageLoadCall
	{
		imageLoadCall(GLuint index) : m_index(index)
		{
		}

		GLuint m_index;
	};

	/* Stream operators */
	/** Write code that execute imageLoad routine for image at given index to output stream
	 *
	 * @param stream Output stream
	 * @param load   imageLoad call details
	 *
	 * @return stream
	 **/
	friend std::ostream& operator<<(std::ostream& stream, const imageLoadCall& load)
	{
		ImageLoadStoreMultipleUniformsTest::imageFormatDetails format_details;
		getImageUniformDeclarationDetails(load.m_index, format_details);

		/* vec4 loaded_value_0 = imageLoad(u_image_0, row_0_coordinates_0); */
		stream << format_details.m_color_type << " ";

		writeLoadedValueVariableNameToStream(stream, load.m_index);

		stream << " = imageLoad(";

		writeImageUniformNameToStream(stream, load.m_index);

		stream << ", ";

		writeCoordinatesVariableNameToStream(stream, load.m_index, 0 /* row */);

		stream << ");";

		return stream;
	}

	struct imageStoreCall
	{
		imageStoreCall(GLuint index, GLuint row) : m_index(index), m_row(row)
		{
		}

		GLuint m_index;
		GLuint m_row;
	};

	/** Write code that execute imageStore to image at given index to output stream
	 *
	 * @param stream Output stream
	 * @param store  imageStore call details
	 *
	 * @return stream
	 **/
	friend std::ostream& operator<<(std::ostream& stream, const imageStoreCall& store)
	{
		/* imageStore(u_image_0, row_0_coordinates_0, -loaded_value_0); */
		stream << "imageStore(";

		writeImageUniformNameToStream(stream, store.m_index);

		stream << ", ";

		writeCoordinatesVariableNameToStream(stream, store.m_index, store.m_row);

		if (0 == store.m_row)
		{
			stream << ", -";
		}
		else
		{
			stream << ", ";
		}

		writeLoadedValueVariableNameToStream(stream, store.m_index);
		stream << ");";

		return stream;
	}

	struct coordinatesVariableDeclaration
	{
		coordinatesVariableDeclaration(GLuint index, GLuint row) : m_index(index), m_row(row)
		{
		}
		GLuint m_index;
		GLuint m_row;
	};

	/** Write declaration of variable for coordinate at given row to output stream
	 *
	 * @param stream      Output stream
	 * @param declaration Declaration details
	 *
	 * @return stream
	 **/
	friend std::ostream& operator<<(std::ostream& stream, const coordinatesVariableDeclaration& declaration)
	{
		stream << "const ivec2 ";

		writeCoordinatesVariableNameToStream(stream, declaration.m_index, declaration.m_row);

		stream << " = ivec2(" << declaration.m_index << ", " << declaration.m_row << ");";

		return stream;
	}

	/* Methods */
	/** Build program to test specified shader stage
	 *
	 * Throws exception in case of any failure
	 *
	 * @param stage Stage id
	 *
	 * @return Program id
	 **/
	GLuint buildProgramToTestShaderStage(shaderStage stage)
	{
		static const char* const boilerplate_fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    discard;\n"
			"}\n";

		static const char* const boilerplate_tesselation_evaluation_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(quads, equal_spacing, ccw) in;\n"
			"\n"
			"void main()\n"
			"{\n"
			"\n"
			"}\n";

		static const char* const boilerplate_vertex_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(location = 0) in vec4 i_position;\n"
			"\n"
			"void main()\n"
			"{\n"
			"  gl_Position = i_position;\n"
			"}\n";

		const char* fragment_shader_code			   = boilerplate_fragment_shader_code;
		const char* geometry_shader_code			   = 0;
		bool		is_program_built				   = true;
		GLuint		program_object_id				   = 0;
		const char* tesselation_control_shader_code	= 0;
		const char* tesselation_evaluation_shader_code = 0;
		std::string tested_shader_stage_code;
		const char* vertex_shader_code = boilerplate_vertex_shader_code;

		/* Get source code for tested shader stage */
		prepareShaderForTestedShaderStage(stage, tested_shader_stage_code);

		if (true == tested_shader_stage_code.empty())
		{
			return 0;
		}

		/* Set up source code for all required stages */
		switch (stage)
		{
		case fragmentShaderStage:
			fragment_shader_code = tested_shader_stage_code.c_str();
			break;

		case geometryShaderStage:
			geometry_shader_code = tested_shader_stage_code.c_str();
			break;

		case tesselationControlShaderStage:
			tesselation_control_shader_code	= tested_shader_stage_code.c_str();
			tesselation_evaluation_shader_code = boilerplate_tesselation_evaluation_shader_code;
			break;

		case tesselationEvalutaionShaderStage:
			tesselation_evaluation_shader_code = tested_shader_stage_code.c_str();
			break;

		case vertexShaderStage:
			vertex_shader_code = tested_shader_stage_code.c_str();
			break;

		default:
			TCU_FAIL("Invalid shader stage");
		}

		/* Build program */
		program_object_id =
			BuildProgram(vertex_shader_code, tesselation_control_shader_code, tesselation_evaluation_shader_code,
						 geometry_shader_code, fragment_shader_code, &is_program_built);

		/* Check if program was built */
		if (false == is_program_built)
		{
			throw tcu::InternalError("Failed to build program", "", __FILE__, __LINE__);
		}

		/* Done */
		return program_object_id;
	}

	/** Create texture to test given shader stage
	 *
	 * Throws exception in case of any failure
	 *
	 * @param stage Stage id
	 *
	 * @return Texture id
	 **/
	GLuint createTextureToTestShaderStage(shaderStage stage)
	{
		GLenum			   error			  = glGetError();
		const GLint		   max_image_uniforms = getMaximumImageUniformsForStage(stage);
		GLuint			   texture_id		  = 0;
		std::vector<GLint> texture_data;

		const GLsizei height = 2;
		const GLsizei width  = max_image_uniforms;

		if (m_min_required_image_uniforms > max_image_uniforms)
		{
			return 0;
		}

		/* Generate texture id */
		glGenTextures(1, &texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenTextures");

		/* Bind texture */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		error = glGetError();
		if (GL_NO_ERROR != error)
		{
			glDeleteTextures(1, &texture_id);
			GLU_EXPECT_NO_ERROR(error, "BindTexture");
		}

		/* Prepare storage for texture data */
		texture_data.resize(width * height);
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			texture_data[i]			= getExpectedValue(i);
			texture_data[i + width] = 0;
		}

		/* Create first level of texture */
		glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_R32I, width, height, 0 /*border */, GL_RED_INTEGER, GL_INT,
					 &texture_data[0]);
		error = glGetError();
		if (GL_NO_ERROR != error)
		{
			glDeleteTextures(1, &texture_id);
			GLU_EXPECT_NO_ERROR(error, "TexImage2D");
		}

		/* Make texture complete */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		error = glGetError();
		if (GL_NO_ERROR != error)
		{
			glDeleteTextures(1, &texture_id);
			GLU_EXPECT_NO_ERROR(error, "TexParameteri");
		}

		/* Done */
		return texture_id;
	}

	/** Get value of texel for image at given index
	 *
	 * @param index Index of image uniform
	 *
	 * @return Value of texel
	 **/
	GLint getExpectedValue(GLint index)
	{
		// To fix denorm issues with r32f, rg16f and r11f_g11f_b10f
		// we set one bit in the exponent of each component of those pixel format
		return 0x40104200 + index;
	}

	/** Get name of uniform at given index
	 *
	 * @param index    Index of uniform
	 * @param out_name Name of uniform
	 **/
	void getImageUniformName(GLuint index, std::string& out_name)
	{
		std::stringstream stream;

		writeImageUniformNameToStream(stream, index);

		out_name = stream.str();
	}

	/** Get maximum number of image uniforms allowed for given shader stage
	 *
	 * @param stage Stage id
	 *
	 * @return Maximum allowed image uniforms
	 **/
	GLint getMaximumImageUniformsForStage(shaderStage stage)
	{
		GLint  max_image_uniforms = 0;
		GLenum pname			  = 0;

		switch (stage)
		{
		case fragmentShaderStage:
			pname = GL_MAX_FRAGMENT_IMAGE_UNIFORMS;
			break;

		case geometryShaderStage:
			pname = GL_MAX_GEOMETRY_IMAGE_UNIFORMS;
			break;

		case tesselationControlShaderStage:
			pname = GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS;
			break;

		case tesselationEvalutaionShaderStage:
			pname = GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS;
			break;

		case vertexShaderStage:
			pname = GL_MAX_VERTEX_IMAGE_UNIFORMS;
			break;

		default:
			TCU_FAIL("Invalid shader stage");
		}

		glGetIntegerv(pname, &max_image_uniforms);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetIntegerv");

		return max_image_uniforms;
	}

	/** Prepare source for tested shader stage
	 *
	 * @param stage    Stage id
	 * @param out_code Source code
	 **/
	void prepareShaderForTestedShaderStage(shaderStage stage, std::string& out_code)
	{
		GLint			  max_image_uniforms	= getMaximumImageUniformsForStage(stage);
		const char*		  stage_specific_layout = "";
		const char*		  stage_specific_predicate = "true";
		std::stringstream stream;

		if (m_min_required_image_uniforms > max_image_uniforms)
		{
			return;
		}

		/* Expected result follows
		 *
		 * #version 400 core
		 * #extension GL_ARB_shader_image_load_store : require
		 *
		 * precision highp float;
		 *
		 * stage_specific_layout goes here
		 *
		 * Uniform declarations go here
		 *
		 * void main()
		 * {
		 *     const ivec2 row_0_coordinates(0, 0);
		 *     const ivec2 row_1_coordinates(0, 1);
		 *
		 *     For each index <0, GL_MAX_*_IMAGE_UNIFORMS>
		 *
		 *     vec4 loaded_value_0 = imageLoad(u_image_0, row_0_coordinates);
		 *
		 *     imageStore(u_image_0, row_0_coordinates, -loaded_value_0);
		 *     imageStore(u_image_0, row_1_coordinates, loaded_value_0);
		 * }
		 */

		/* Get piece of code specific for stage */
		switch (stage)
		{
		case fragmentShaderStage:
			break;

		case geometryShaderStage:
			stage_specific_layout = "layout(points) in;\n"
									"layout(points, max_vertices = 1) out;\n"
									"\n";
			break;

		case tesselationControlShaderStage:
			stage_specific_layout = "layout(vertices = 4) out;\n"
									"\n";
			stage_specific_predicate = "gl_InvocationID == 0";
			break;

		case tesselationEvalutaionShaderStage:
			stage_specific_layout = "layout(quads, equal_spacing, ccw) in;\n"
									"\n";
			break;

		case vertexShaderStage:
			break;

		default:
			TCU_FAIL("Invalid shader stage");
		}

		/* Preamble */
		stream << "#version 400 core\n"
				  "#extension GL_ARB_shader_image_load_store : require\n"
				  "\n"
				  "precision highp float;\n"
				  "\n"
			   << stage_specific_layout;

		/* Image uniforms declarations */
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			stream << imageUniformDeclaration(i) << "\n";
		}

		/* Main opening */
		stream << "\n"
				  "void main()\n"
				  "{\n";

		stream << "    if (" << stage_specific_predicate << ")\n";
		stream << "    {\n";

		/* imageLoad and imageStores for each uniform */
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			stream << "        " << coordinatesVariableDeclaration(i, 0) << "\n"
				   << "        " << coordinatesVariableDeclaration(i, 1) << "\n"
				   << "\n"
				   << "        " << imageLoadCall(i) << "\n"
				   << "\n"
				   << "        " << imageStoreCall(i, 0) << "\n"
				   << "        " << imageStoreCall(i, 1) << "\n";

			if (max_image_uniforms > i + 1)
			{
				stream << "\n";
			}
		}

		stream << "    }\n";

		/* Main closing */
		stream << "}\n\n";

		/* Done */
		out_code = stream.str();
	}

	/** Test given shader stage
	 *
	 * @param stage Stage id
	 *
	 * @return m_test_not_supported if shader stage does not support at least m_min_required_image_uniforms image uniforms;
	 *         testFailed when test result is negative;
	 *         m_test_passed when test result is positive;
	 **/
	testResult testShaderStage(shaderStage stage)
	{
		std::string		   image_uniform_name;
		static const GLint invalid_uniform_location = -1;
		const GLint		   max_image_uniforms		= getMaximumImageUniformsForStage(stage);
		GLenum			   primitive_mode			= GL_POINTS;
		GLuint			   program_id				= 0;
		testResult		   result					= testPassed;
		std::vector<GLint> texture_data;
		GLuint			   texture_id = 0;

		static const GLuint height = 2;
		const GLuint		width  = max_image_uniforms;

		const GLuint		positive_value_index = width;
		static const GLuint negated_value_index  = 0;

		if (m_min_required_image_uniforms > max_image_uniforms)
		{
			return testNotSupported;
		}

		/* Select program and texture ids for given stage */
		switch (stage)
		{
		case fragmentShaderStage:
			program_id = m_program_to_test_fs_stage_id;
			texture_id = m_texture_to_test_fs_stage_id;
			break;

		case geometryShaderStage:
			program_id = m_program_to_test_gs_stage_id;
			texture_id = m_texture_to_test_gs_stage_id;
			break;

		case tesselationControlShaderStage:
			primitive_mode = GL_PATCHES;
			program_id	 = m_program_to_test_tcs_stage_id;
			texture_id	 = m_texture_to_test_tcs_stage_id;
			break;

		case tesselationEvalutaionShaderStage:
			primitive_mode = GL_PATCHES;
			program_id	 = m_program_to_test_tes_stage_id;
			texture_id	 = m_texture_to_test_tes_stage_id;
			break;

		case vertexShaderStage:
			program_id = m_program_to_test_vs_stage_id;
			texture_id = m_texture_to_test_vs_stage_id;
			break;

		default:
			TCU_FAIL("Invalid shader stage");
		}

		/* Set program */
		glUseProgram(program_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "UseProgram");

		/* Bind texture to image units */
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			imageFormatDetails format_details;
			getImageUniformDeclarationDetails(i, format_details);

			glBindImageTexture(i /* unit */, texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
							   GL_READ_WRITE, format_details.m_image_unit_format);
			GLU_EXPECT_NO_ERROR(glGetError(), "BindImageTexture");
		}

		/* Set all image uniforms to corresponding image units */
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			/* Get name */
			getImageUniformName(i, image_uniform_name);

			/* Get location */
			GLint image_uniform_location = glGetUniformLocation(program_id, image_uniform_name.c_str());
			GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");

			if (invalid_uniform_location == image_uniform_location)
			{
				throw tcu::InternalError("Uniform location is not available", image_uniform_name.c_str(), __FILE__,
										 __LINE__);
			}

			/* Set uniform value */
			glUniform1i(image_uniform_location, i /* image_unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");
		}

		/* Execute draw */
		glDrawArrays(primitive_mode, 0 /* first vertex */, 1 /* one vertex */);
		GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		texture_data.resize(width * height);

		/* Get texture data */
		glBindTexture(GL_TEXTURE_2D, texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, &texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		/* Verify each image uniform */
		for (GLint i = 0; i < max_image_uniforms; ++i)
		{
			imageFormatDetails format_details;
			getImageUniformDeclarationDetails(i, format_details);

			if (false ==
				format_details.m_verification_routine(getExpectedValue(i), texture_data[positive_value_index + i],
													  texture_data[negated_value_index + i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Invalid result!"
					<< " Image format: " << format_details.m_image_format << " Original value: "
					<< "0x" << std::setw(8) << std::setfill('0') << std::setbase(16) << getExpectedValue(i)
					<< " Copied value: "
					<< "0x" << std::setw(8) << std::setfill('0') << std::setbase(16)
					<< texture_data[positive_value_index + i] << " Negated value: "
					<< "0x" << std::setw(8) << std::setfill('0') << std::setbase(16)
					<< texture_data[negated_value_index + i] << tcu::TestLog::EndMessage;

				result = testFailed;
			}
		}

		/* Done */
		return result;
	}

	/** Verifies if original_value, positive_value and negated_value match
	 *
	 * @tparam T Type used during verification process, it should match float values by size
	 *
	 * @param original_value Original value of texel, used when creating a texture
	 * @param positive_value Value stored by shader as read
	 * @param negated_value  Value stored by shader after negation
	 *
	 * @return true if values match, false otherwise
	 **/
	template <typename T>
	static bool verifyFloat(GLint original_value, GLint positive_value, GLint negated_value)
	{
		if (original_value != positive_value)
		{
			return false;
		}

		static const GLuint n_elements		  = sizeof(GLint) / sizeof(T); /* 1, 2, 4 */
		static const GLuint sign_bit_index	= sizeof(T) * 8 - 1;		   /* 7, 15, 31 */
		static const T		sign_bit_mask	 = 1 << sign_bit_index;	   /* 0x80.. */
		static const T		sign_bit_inv_mask = (T)~sign_bit_mask;		   /* 0x7f.. */

		const T* positive_elements = (T*)&positive_value;
		const T* negated_elements  = (T*)&negated_value;

		for (GLuint i = 0; i < n_elements; ++i)
		{
			const T positive_element = positive_elements[i];
			const T negated_element  = negated_elements[i];

			const T positive_sign_bit = positive_element & sign_bit_mask;
			const T negated_sign_bit  = negated_element & sign_bit_mask;

			const T positive_data = positive_element & sign_bit_inv_mask;
			const T negated_data  = negated_element & sign_bit_inv_mask;

			/* Compare data bits */
			if (positive_data != negated_data)
			{
				return false;
			}

			/* Verify that sign bit is inverted */
			if (positive_sign_bit == negated_sign_bit)
			{
				return false;
			}
		}

		return true;
	}

	/** Verifies if original_value, positive_value and negated_value match
	 *
	 * @tparam T Type used during verification process, it should match float values by size
	 *
	 * @param original_value Original value of texel, used when creating a texture
	 * @param positive_value Value stored by shader as read
	 * @param negated_value  Value stored by shader after negation
	 *
	 * @return true if values match, false otherwise
	 **/
	template <typename T>
	static bool verifyFloatSignedNorm(GLint original_value, GLint positive_value, GLint negated_value)
	{
		if (original_value != positive_value)
		{
			return false;
		}

		static const GLuint n_elements = sizeof(GLint) / sizeof(T); /* 1, 2, 4 */

		const T* positive_elements = (T*)&positive_value;
		const T* negated_elements  = (T*)&negated_value;

		for (GLuint i = 0; i < n_elements; ++i)
		{
			const T positive_element = positive_elements[i];
			const T negated_element  = negated_elements[i];

			/* Compare data bits */
			if (positive_element != -negated_element)
			{
				return false;
			}
		}

		return true;
	}

	/** Verifies if original_value, positive_value and negated_value match
	 *
	 * @tparam R Number of bits for red channel
	 * @tparam G Number of bits for green channel
	 * @tparam B Number of bits for blue channel
	 * @tparam A Number of bits for alpha channel
	 *
	 * @param original_value Original value of texel, used when creating a texture
	 * @param positive_value Value stored by shader as read
	 * @param negated_value  Value stored by shader after negation
	 *
	 * @return true if values match, false otherwise
	 **/
	template <GLuint R, GLuint G, GLuint B, GLuint A, typename T>
	static bool verifyInteger(GLint original_value, GLint positive_value, GLint negated_value)
	{
		if (original_value != positive_value)
		{
			return false;
		}

		Integer<T, R, 0> positive_red(positive_value);
		Integer<T, R, 0> negated_red(negated_value);

		Integer<T, G, R> positive_green(positive_value);
		Integer<T, G, R> negated_green(negated_value);

		Integer<T, B, R + G> positive_blue(positive_value);
		Integer<T, B, R + G> negated_blue(negated_value);

		Integer<T, A, R + G + B> positive_alpha(positive_value);
		Integer<T, A, R + G + B> negated_alpha(negated_value);

		if (((0 != R) && (positive_red.GetNegated() != negated_red.Get())) ||
			((0 != B) && (positive_blue.GetNegated() != negated_blue.Get())) ||
			((0 != G) && (positive_green.GetNegated() != negated_green.Get())) ||
			((0 != A) && (positive_alpha.GetNegated() != negated_alpha.Get())))
		{
			return false;
		}

		return true;
	}

	/** Verifies if original_value, positive_value and negated_value match
	 *
	 * @param original_value Original value of texel, used when creating a texture
	 * @param positive_value Value stored by shader as read
	 * @param negated_value  Value stored by shader after negation
	 *
	 * @return true if values match, false otherwise
	 **/
	static bool verifyInteger(GLint original_value, GLint positive_value, GLint negated_value)
	{
		if (original_value != positive_value)
		{
			return false;
		}

		if (positive_value != -negated_value)
		{
			return false;
		}

		return true;
	}

	/** Verifies if original_value, positive_value and negated_value match
	 *
	 * @param original_value Original value of texel, used when creating a texture
	 * @param positive_value Value stored by shader as read
	 * @param negated_value  Value stored by shader after negation
	 *
	 * @return true if values match, false otherwise
	 **/
	template <typename T>
	static bool verifyFloatUnsigned(GLint original_value, GLint positive_value, GLint negated_value)
	{
		if (original_value != positive_value)
		{
			return false;
		}

		if (0 != negated_value)
		{
			return false;
		}

		return true;
	}
};

/** Test "Early fragment tests" description follows.
 *
 *  BasicGLSLEarlyFragTests verifies that:
 *  - early z test is applied when enabled,
 *  - early z test is not applied when disabled.
 *
 *  Proposed modifications:
 *  - verify that early z test does not discard all fragments when enabled,
 *  - verify that early stencil test is applied when enabled,
 *  - verify that early stencil test does not discard all fragments when
 *  enabled,
 *  - verify that early stencil test is not applied when disabled.
 *
 *  Steps:
 *  - prepare 2 programs that store 1.0 at red channel to image in fragment
 *  shader stage:
 *      a) one program should enable early fragment tests
 *      ("layout(early_fragment_tests) in;"),
 *      b) second program should disable early fragment tests,
 *  - prepare frame buffer with 64x64 R32F color and GL_DEPTH_STENCIL
 *  depth-stencil attachments,
 *  - prepare 2D texture 64x64 R32F,
 *  - enable depth test,
 *  - verify that early z test is applied when enabled:
 *      - use program enabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 0 and depth 0.5,
 *      - fill texture with zeros,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad (left bottom corner at -1,-1 and right top
 *      corner at 1,1) at z: 0.75
 *      - verify that texture is still filled with zeros,
 *  - verify that early z test does not discard all fragments:
 *      - use program enabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 0 and depth 0.5,
 *      - fill texture with zeros,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad at z: 0.25
 *      - verify that texture is now filled with 1.0,
 *  -verify that early z test is not applied when disabled:
 *      - use program disabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 0 and depth 0.5,
 *      - fill texture with zeros,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad at z: 0.75
 *      - verify that texture is now filled with 1.0.
 *  - disable depth test
 *  - enable stencil test
 *  - verify that early stencil test is applied when enabled:
 *      - use program enabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 0 and depth 1,
 *      - fill texture with zeros,
 *      - set stencil test to:
 *          - <func> to GL_LESS,
 *          - <ref> to 128,
 *          - <mask> 0xffffffff,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad at z: 0,
 *      - verify that texture is still filled with zeros,
 *  - verify that early stencil test does not discard all fragments:
 *      - use program enabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 128 and depth 1,
 *      - fill texture with zeros,
 *      - set stencil test to:
 *          - <func> to GL_LESS,
 *          - <ref> to 0,
 *          - <mask> 0xffffffff,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad at z: 0,
 *      - verify that texture is now filled with 1.0,
 *  - verify that early stencil test is not applied when disabled:
 *      - use program disabling early fragment tests,
 *      - clean frame buffer with color: 0, stencil: 0 and depth 1,
 *      - fill texture with zeros,
 *      - set stencil test to:
 *          - <func> to GL_LESS,
 *          - <ref> to 128,
 *          - <mask> 0xffffffff,
 *      - bind texture to image uniform,
 *      - draw "full screen" quad at z: 0,
 *      - verify that texture is now filled with 1.0
 **/
class ImageLoadStoreEarlyFragmentTestsTest : public ShaderImageLoadStoreBase
{
private:
	/* Constants */
	GLuint			   m_image_edge;
	static const GLint m_invalid_uniform_location = -1;

	/* Types */
	/** Store id and uniform locations for a single program object
	 *
	 **/
	struct programDetails
	{
		GLint  m_depth_uniform_location;
		GLint  m_image_uniform_location;
		GLuint m_program_id;

		programDetails()
			: m_depth_uniform_location(ImageLoadStoreEarlyFragmentTestsTest::m_invalid_uniform_location)
			, m_image_uniform_location(ImageLoadStoreEarlyFragmentTestsTest::m_invalid_uniform_location)
			, m_program_id(0)
		{
			/* Nothing to be done here */
		}
	};

	/* Fileds */
	/* Storage for texture data */
	std::vector<GLfloat> m_clean_texture_data;
	std::vector<GLfloat> m_extracted_texture_data;

	/* Program details */
	programDetails m_disabled_early_tests;
	programDetails m_enabled_early_tests;

	/* Ids of GL objects */
	GLuint m_color_renderbuffer_id;
	GLuint m_depth_stencil_renderbuffer_id;
	GLuint m_framebuffer_id;
	GLuint m_texture_id;
	GLuint m_vertex_array_object_id;

public:
	/* Constructor */
	ImageLoadStoreEarlyFragmentTestsTest()
		: m_image_edge(0)
		, m_color_renderbuffer_id(0)
		, m_depth_stencil_renderbuffer_id(0)
		, m_framebuffer_id(0)
		, m_texture_id(0)
		, m_vertex_array_object_id(0)
	{
		/* Nothing to be done here */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Cleanup()
	{
		/* Restore defaults */
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glUseProgram(0);

		/* Delete objects */
		if (0 != m_disabled_early_tests.m_program_id)
		{
			glDeleteProgram(m_disabled_early_tests.m_program_id);
			m_disabled_early_tests.m_program_id = 0;
		}

		if (0 != m_enabled_early_tests.m_program_id)
		{
			glDeleteProgram(m_enabled_early_tests.m_program_id);
			m_enabled_early_tests.m_program_id = 0;
		}

		if (0 != m_color_renderbuffer_id)
		{
			glDeleteRenderbuffers(1, &m_color_renderbuffer_id);
			m_color_renderbuffer_id = 0;
		}

		if (0 != m_depth_stencil_renderbuffer_id)
		{
			glDeleteRenderbuffers(1, &m_depth_stencil_renderbuffer_id);
			m_depth_stencil_renderbuffer_id = 0;
		}

		if (0 != m_framebuffer_id)
		{
			glDeleteFramebuffers(1, &m_framebuffer_id);
			m_framebuffer_id = 0;
		}

		if (0 != m_texture_id)
		{
			glDeleteTextures(1, &m_texture_id);
			m_texture_id = 0;
		}

		if (0 != m_vertex_array_object_id)
		{
			glDeleteVertexArrays(1, &m_vertex_array_object_id);
			m_vertex_array_object_id = 0;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		bool result = true;

		/* Bind texture to first image unit */
		glBindImageTexture(0 /* first image unit */, m_texture_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
						   GL_READ_WRITE, GL_R32F);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindImageTexture");

		/* Run tests for depth test */
		if (false == testEarlyZ())
		{
			result = false;
		}

		/* Run tests for stencil test */
		if (false == testEarlyStencil())
		{
			result = false;
		}

		/* Return ERROR if any problem was found */
		if (false == result)
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Setup()
	{
		m_image_edge = de::min(64, de::min(getWindowHeight(), getWindowWidth()));

		/* Prepare storage for texture data */
		m_clean_texture_data.resize(m_image_edge * m_image_edge);
		m_extracted_texture_data.resize(m_image_edge * m_image_edge);

		/* Prepare programs, framebuffer and texture */
		buildPrograms();
		createFramebuffer();
		createTexture();

		/* Generate vertex array object */
		glGenVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenVertexArrays");

		/* Bind vertex array object */
		glBindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindVertexArray");

		/* Set clear color */
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		GLU_EXPECT_NO_ERROR(glGetError(), "ClearColor");

		/* Done */
		return NO_ERROR;
	}

private:
	/** Build two programs: with enabled and disabled early fragment tests
	 *
	 **/
	void buildPrograms()
	{
		static const char* const fragment_shader_with_disabled_early_tests =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(r32f) uniform image2D u_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vec4 color = vec4(1.0, 0, 0, 0);\n"
			"\n"
			"    imageStore(u_image, ivec2(gl_FragCoord.xy), color);\n"
			"\n"
			"    discard;\n"
			"}\n\n";

		static const char* const fragment_shader_with_enabled_early_tests =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(early_fragment_tests) in;\n"
			"\n"
			"layout(r32f) uniform image2D u_image;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vec4 color = vec4(1.0, 0, 0, 0);\n"
			"\n"
			"    imageStore(u_image, ivec2(gl_FragCoord.xy), color);\n"
			"\n"
			"    discard;\n"
			"}\n\n";

		static const char* const geometry_shader_code = "#version 400 core\n"
														"#extension GL_ARB_shader_image_load_store : require\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"layout(points)                           in;\n"
														"layout(triangle_strip, max_vertices = 4) out;\n"
														"\n"
														"uniform float u_depth;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    // Left-bottom\n"
														"    gl_Position = vec4(-1, -1, u_depth, 1);\n"
														"    EmitVertex();\n"
														"\n"
														"    // Left-top\n"
														"    gl_Position = vec4(-1,  1, u_depth, 1);\n"
														"    EmitVertex();\n"
														"\n"
														"    // Right-bottom\n"
														"    gl_Position = vec4( 1, -1, u_depth, 1);\n"
														"    EmitVertex();\n"
														"\n"
														"    // Right-top\n"
														"    gl_Position = vec4( 1,  1, u_depth, 1);\n"
														"    EmitVertex();\n"
														"}\n\n";

		static const char* const vertex_shader_code = "#version 400 core\n"
													  "#extension GL_ARB_shader_image_load_store : require\n"
													  "\n"
													  "precision highp float;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "}\n\n";

		prepareProgramDetails(fragment_shader_with_disabled_early_tests, geometry_shader_code, vertex_shader_code,
							  m_disabled_early_tests);

		prepareProgramDetails(fragment_shader_with_enabled_early_tests, geometry_shader_code, vertex_shader_code,
							  m_enabled_early_tests);
	}

	/** Fill texture with zeros
	 *
	 **/
	void cleanTexture()
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* xoffset */, 0 /* yoffset */, m_image_edge, m_image_edge,
						GL_RED, GL_FLOAT, &m_clean_texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "TexSubImage2D");
	}

	/** Create and bind (draw) framebuffer with color and depth-stencil attachments
	 *
	 **/
	void createFramebuffer()
	{
		/* Generate render buffers */
		glGenRenderbuffers(1, &m_color_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenRenderbuffers");

		glGenRenderbuffers(1, &m_depth_stencil_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenRenderbuffers");

		/* Generate and bind framebuffer object */
		glGenFramebuffers(1, &m_framebuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenFramebuffers");

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindFramebuffer");

		/* Prepare color render buffer */
		glBindRenderbuffer(GL_RENDERBUFFER, m_color_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindRenderbuffer");

		glRenderbufferStorage(GL_RENDERBUFFER, GL_R32F, m_image_edge, m_image_edge);
		GLU_EXPECT_NO_ERROR(glGetError(), "RenderbufferStorage");

		/* Set up color attachment */
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "FramebufferRenderbuffer");

		/* Prepare depth-stencil render buffer */
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth_stencil_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindRenderbuffer");

		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_image_edge, m_image_edge);
		GLU_EXPECT_NO_ERROR(glGetError(), "RenderbufferStorage");

		/* Set up depth-stencil attachment */
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
								  m_depth_stencil_renderbuffer_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "FramebufferRenderbuffer");
	}

	/** Create 2D R32F texture
	 *
	 **/
	void createTexture()
	{
		glGenTextures(1, &m_texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "GenTextures");

		glBindTexture(GL_TEXTURE_2D, m_texture_id);
		GLU_EXPECT_NO_ERROR(glGetError(), "BindTexture");

		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, m_image_edge, m_image_edge);
		GLU_EXPECT_NO_ERROR(glGetError(), "TexStorage2D");
	}

	/** Extracts red channel from texture and verify if all texels are set to specified value
	 *
	 * @param value Expected value
	 *
	 * @return true if all texel match expected value, false otherwise
	 **/
	bool isTextureFilledWithValue(GLfloat value)
	{
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &m_extracted_texture_data[0]);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetTexImage");

		for (GLuint i = 0; i < m_image_edge * m_image_edge; ++i)
		{
			if (value != m_extracted_texture_data[i])
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Texel at location " << i
													<< " has invalid value: " << m_extracted_texture_data[i]
													<< " expected: " << value << tcu::TestLog::EndMessage;

				return false;
			}
		}

		return true;
	}

	/** Build program, extract location of uniforms and store results in programDetails instance
	 *
	 * Throws tcu::InternalError if uniforms are inactive
	 *
	 * @param fragment_shader_code Source of fragment shader
	 * @param geometry_shader_code Source of geometry shader
	 * @param vertex_shader_code   Source of vertex shader
	 * @param out_program_details  Instance of programDetails
	 **/
	void prepareProgramDetails(const char* fragment_shader_code, const char* geometry_shader_code,
							   const char* vertex_shader_code, programDetails& out_program_details)
	{
		static const char* const depth_uniform_name = "u_depth";
		static const char* const image_uniform_name = "u_image";
		bool					 is_program_built   = true;

		GLuint program_id = BuildProgram(vertex_shader_code, 0 /* src_tcs */, 0 /* src_tes */, geometry_shader_code,
										 fragment_shader_code, &is_program_built);

		if (false == is_program_built)
		{
			throw tcu::InternalError("Failed to build program", "", __FILE__, __LINE__);
		}

		/* Get depth uniform location */
		GLint depth_uniform_location = glGetUniformLocation(program_id, depth_uniform_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");

		if (m_invalid_uniform_location == depth_uniform_location)
		{
			throw tcu::InternalError("Uniform is not active", image_uniform_name, __FILE__, __LINE__);
		}

		/* Get image uniform location */
		GLint image_uniform_location = glGetUniformLocation(program_id, image_uniform_name);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetUniformLocation");

		if (m_invalid_uniform_location == image_uniform_location)
		{
			throw tcu::InternalError("Uniform is not active", image_uniform_name, __FILE__, __LINE__);
		}

		/* Store results */
		out_program_details.m_depth_uniform_location = depth_uniform_location;
		out_program_details.m_image_uniform_location = image_uniform_location;
		out_program_details.m_program_id			 = program_id;
	}

	/** Test if early fragment stencil test works as expected.
	 *
	 * @return true if successful, false otherwise
	 **/
	bool testEarlyStencil()
	{
		bool result = true;

		glEnable(GL_STENCIL_TEST);
		GLU_EXPECT_NO_ERROR(glGetError(), "glEnable");

		glClearDepthf(1.0f);
		GLU_EXPECT_NO_ERROR(glGetError(), "ClearDepthf");

		/* verify that early stencil test is applied when enabled */
		{
			glUseProgram(m_enabled_early_tests.m_program_id);
			GLU_EXPECT_NO_ERROR(glGetError(), "glUseProgram");

			glClearStencil(0);
			GLU_EXPECT_NO_ERROR(glGetError(), "ClearStencil");

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glStencilFunc(GL_LESS, 128, 0xffffffff);
			GLU_EXPECT_NO_ERROR(glGetError(), "StencilFunc");

			glUniform1i(m_enabled_early_tests.m_image_uniform_location, 0 /* first unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");

			glUniform1f(m_enabled_early_tests.m_depth_uniform_location, 0.0f /* depth */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1f");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(0.0f))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Problem with early stencil test. It is not applied"
													<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		/* verify that early stencil test does not discard all fragments */
		{
			glClearStencil(128);
			GLU_EXPECT_NO_ERROR(glGetError(), "ClearStencil");

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glStencilFunc(GL_LESS, 0, 0xffffffff);
			GLU_EXPECT_NO_ERROR(glGetError(), "StencilFunc");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(1.0f))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Problem with early stencil test. It discards fragments, that shall be drawn"
					<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		/* verify that early stencil test is not applied when disabled */
		{
			glUseProgram(m_disabled_early_tests.m_program_id);
			GLU_EXPECT_NO_ERROR(glGetError(), "glUseProgram");

			glClearStencil(0);
			GLU_EXPECT_NO_ERROR(glGetError(), "ClearStencil");

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glStencilFunc(GL_LESS, 128, 0xffffffff);
			GLU_EXPECT_NO_ERROR(glGetError(), "StencilFunc");

			glUniform1i(m_disabled_early_tests.m_image_uniform_location, 0 /* first unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");

			glUniform1f(m_disabled_early_tests.m_depth_uniform_location, 0.0f /* depth */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1f");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(1.0f))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Problem with early stencil test. It is applied when disabled"
													<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		glDisable(GL_STENCIL_TEST);
		GLU_EXPECT_NO_ERROR(glGetError(), "Disable");

		/* Done */
		return result;
	}

	/** Test if early fragment depth test works as expected.
	 *
	 * @return true if successful, false otherwise
	 **/
	bool testEarlyZ()
	{
		bool result = true;

		glEnable(GL_DEPTH_TEST);
		GLU_EXPECT_NO_ERROR(glGetError(), "glEnable");

		glClearDepthf(0.5f);
		GLU_EXPECT_NO_ERROR(glGetError(), "ClearDepthf");

		glClearStencil(0);
		GLU_EXPECT_NO_ERROR(glGetError(), "ClearStencil");

		/* verify that early z test is applied when enabled */
		{
			glUseProgram(m_enabled_early_tests.m_program_id);
			GLU_EXPECT_NO_ERROR(glGetError(), "glUseProgram");

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glUniform1i(m_enabled_early_tests.m_image_uniform_location, 0 /* first unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");

			glUniform1f(m_enabled_early_tests.m_depth_uniform_location, 0.5f /* depth */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1f");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(0.0f))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Problem with early z test. It is not applied"
													<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		/* verify that early z test does not discard all fragments */
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glUniform1i(m_enabled_early_tests.m_image_uniform_location, 0 /* first unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");

			glUniform1f(m_enabled_early_tests.m_depth_uniform_location, -0.5f /* depth */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1f");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(1.0f))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Problem with early z test. It discards fragments, that shall be drawn"
					<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		/* verify that early z test is not applied when disabled */
		{
			glUseProgram(m_disabled_early_tests.m_program_id);
			GLU_EXPECT_NO_ERROR(glGetError(), "glUseProgram");

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(glGetError(), "Clear");

			cleanTexture();

			glUniform1i(m_disabled_early_tests.m_image_uniform_location, 0 /* first unit */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1i");

			glUniform1f(m_disabled_early_tests.m_depth_uniform_location, 0.5f /* depth */);
			GLU_EXPECT_NO_ERROR(glGetError(), "Uniform1f");

			glDrawArrays(GL_POINTS, 0 /* first */, 1 /* number of vertices */);
			GLU_EXPECT_NO_ERROR(glGetError(), "DrawArrays");

			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			GLU_EXPECT_NO_ERROR(glGetError(), "MemoryBarrier");

			if (false == isTextureFilledWithValue(1.0f))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "Problem with early z test. It is applied when disabled"
													<< tcu::TestLog::EndMessage;

				result = false;
			}
		}

		glDisable(GL_DEPTH_TEST);
		GLU_EXPECT_NO_ERROR(glGetError(), "Disable");

		/* Done */
		return result;
	}
};

//-----------------------------------------------------------------------------
// 4.1 NegativeUniform
//-----------------------------------------------------------------------------
class NegativeUniform : public ShaderImageLoadStoreBase
{
	GLuint m_program;

	virtual long Setup()
	{
		m_program = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* glsl_vs = "#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL
							  "  gl_Position = i_position;" NL "}";
		const char* glsl_fs = "#version 420 core" NL "writeonly uniform image2D g_image;" NL "void main() {" NL
							  "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image, coord, vec4(0.0));" NL
							  "  discard;" NL "}";
		m_program = BuildProgram(glsl_vs, NULL, NULL, NULL, glsl_fs);

		GLint max_image_units;
		glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_units);
		glUseProgram(m_program);

		glUniform1i(glGetUniformLocation(m_program, "g_image"), -1);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glUniform1i should generate INVALID_VALUE when <value> is less than zero."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUniform1i(glGetUniformLocation(m_program, "g_image"), max_image_units);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glUniform1i should generate INVALID_VALUE when <value> is greater than or equal to the value of "
				<< "MAX_IMAGE_UNITS." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		GLint i = -3;
		glUniform1iv(glGetUniformLocation(m_program, "g_image"), 1, &i);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glUniform1iv should generate INVALID_VALUE when <value> is less than zero."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		i = max_image_units + 1;
		glUniform1iv(glGetUniformLocation(m_program, "g_image"), 1, &i);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glUniform1iv should generate INVALID_VALUE when <value> is greater "
											"than or equal to the value of MAX_IMAGE_UNITS."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glUniform1ui(glGetUniformLocation(m_program, "g_image"), 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glUniform1iv should generate INVALID_OPERATION if the location refers to an image variable."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		glUniform2i(glGetUniformLocation(m_program, "g_image"), 0, 0);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glUniform2i should generate INVALID_OPERATION if the location refers to an image variable."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		{
			glUseProgram(0);

			glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_image"), -1);
			if (glGetError() != GL_INVALID_VALUE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glProgramUniform1i should generate INVALID_VALUE when <value> is less than zero."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}
			glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_image"), max_image_units);
			if (glGetError() != GL_INVALID_VALUE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glProgramUniform1i should generate INVALID_VALUE when <value> is greater than or equal to the "
					   "value of MAX_IMAGE_UNITS."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}

			GLint ii = -3;
			glProgramUniform1iv(m_program, glGetUniformLocation(m_program, "g_image"), 1, &ii);
			if (glGetError() != GL_INVALID_VALUE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glProgramUniform1iv should generate INVALID_VALUE when <value> is less than zero."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}
			ii = max_image_units + 1;
			glProgramUniform1iv(m_program, glGetUniformLocation(m_program, "g_image"), 1, &ii);
			if (glGetError() != GL_INVALID_VALUE)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glProgramUniform1iv should generate INVALID_VALUE when <value> "
												"is greater than or equal to the value of MAX_IMAGE_UNITS."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}

			glProgramUniform1ui(m_program, glGetUniformLocation(m_program, "g_image"), 0);
			if (glGetError() != GL_INVALID_OPERATION)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glProgramUniform1ui should generate INVALID_OPERATION if the "
												"location refers to an image variable."
					<< tcu::TestLog::EndMessage;
				return ERROR;
			}
			glProgramUniform2i(m_program, glGetUniformLocation(m_program, "g_image"), 0, 0);
			if (glGetError() != GL_INVALID_OPERATION)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glProgramUniform2i should generate INVALID_OPERATION if the "
												"location refers to an image variable."
					<< tcu::TestLog::EndMessage;
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
//-----------------------------------------------------------------------------
// 4.2 NegativeBind
//-----------------------------------------------------------------------------
class NegativeBind : public ShaderImageLoadStoreBase
{
	virtual long Setup()
	{
		return NO_ERROR;
	}

	virtual long Run()
	{
		glBindImageTexture(100, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <unit> is "
											"greater than or equal to the value of MAX_IMAGE_UNITS."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(0, 123, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <texture> is not "
											"the name of an existing texture object."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(1, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA + 1234);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "BindImageTexture should generate INVALID_VALUE if <format> is not a legal format."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 4.3 NegativeCompileErrors
//-----------------------------------------------------------------------------
class NegativeCompileErrors : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba32f) writeonly readonly uniform image2D g_image;" NL "void main() {" NL
					 "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba32f) writeonly uniform image2D g_image;" NL "void main() {" NL
					 "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "in vec4 i_color;" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba32f) readonly uniform image2D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec2(0), i_color);" NL "  o_color = i_color;" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL "uniform image2D g_image;" NL
					 "void main() {" NL "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "readonly uniform image2D g_image;" NL "void main() {" NL
					 "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rg16i) uniform image1D g_image;" NL "void main() {" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rg16) uniform iimage2D g_image;" NL "void main() {" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(r32f) coherent uniform image2D g_image;" NL "void main() {" NL
					 "  imageAtomicAdd(g_image, ivec2(1), 10);" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(r16i) coherent uniform iimage2D g_image;" NL "void main() {" NL
					 "  imageAtomicAdd(g_image, ivec2(1), 1u);" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(r32ui) uniform iimage3D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec3(1), ivec4(1));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba8) uniform uimage2DArray g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec3(0), uvec4(1));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba32f) coherent uniform image2D g_image;" NL "vec4 Load(image2D image) {" NL
					 "  return imageLoad(image, vec2(0));" NL "}" NL "void main() {" NL "  o_color = Load(g_image);" NL
					 "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Compile(const std::string& source)
	{
		const GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);

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
//-----------------------------------------------------------------------------
// 4.4 NegativeLinkErrors
//-----------------------------------------------------------------------------
class NegativeLinkErrors : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		if (!SupportedInVS(1))
			return NOT_SUPPORTED;

		if (!Link("#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
				  "layout(rgba32f) uniform image1D g_image;" NL "void main() {" NL
				  "  imageStore(g_image, gl_VertexID, vec4(0));" NL "  gl_Position = i_position;" NL "}",
				  "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
				  "layout(rgba32f) uniform image2D g_image;" NL "void main() {" NL
				  "  imageStore(g_image, ivec2(gl_FragCoord), vec4(1.0));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Link("#version 420 core" NL "layout(location = 0) in vec4 i_position;" NL
				  "layout(rgba32f) uniform image1D g_image;" NL "void main() {" NL
				  "  imageStore(g_image, gl_VertexID, vec4(0));" NL "  gl_Position = i_position;" NL "}",
				  "#version 420 core" NL "layout(location = 0) out vec4 o_color;" NL
				  "layout(rg32f) uniform image1D g_image;" NL "void main() {" NL
				  "  imageStore(g_image, int(gl_FragCoord.x), vec4(1.0));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Link(const std::string& vs, const std::string& fs)
	{
		const GLuint p = glCreateProgram();

		const GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(p, vsh);
		glDeleteShader(vsh);
		const char* const vssrc = vs.c_str();
		glShaderSource(vsh, 1, &vssrc, NULL);
		glCompileShader(vsh);

		const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(p, fsh);
		glDeleteShader(fsh);
		const char* const fssrc = fs.c_str();
		glShaderSource(fsh, 1, &fssrc, NULL);
		glCompileShader(fsh);

		GLint status;
		glGetShaderiv(vsh, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			glDeleteProgram(p);
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "VS compilation should be ok." << tcu::TestLog::EndMessage;
			return false;
		}
		glGetShaderiv(fsh, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			glDeleteProgram(p);
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "FS compilation should be ok." << tcu::TestLog::EndMessage;
			return false;
		}

		glLinkProgram(p);

		GLchar log[1024];
		glGetProgramInfoLog(p, sizeof(log), NULL, log);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
											<< log << tcu::TestLog::EndMessage;

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

/** Negative Test "Active image uniform limits", description follows.
 *
 *  Program that exceeds resource limits should not compile and/or link.
 *
 *  Steps:
 *  - try to compile and link a program that uses too many image uniforms in
 *  fragment shader stage,
 *  - try to compile and link a program that uses too many image uniforms in
 *  vertex shader stage,
 *  - try to compile and link a program that uses too many image uniforms in
 *  tessellation control shader stage,
 *  - try to compile and link a program that uses too many image uniforms in
 *  tessellation evaluation shader stage,
 *  - try to compile and link a program that uses too many image uniforms in
 *  geometry shader stage,
 *  - try to compile and link a program that uses too many image uniforms in all
 *  shader stages combined, any single stage should not exceed its limits, this
 *  step might be impossible to fulfill.
 *
 *  Test should use the following declaration of image uniforms:
 *  layout(r32i) uniform iimage2D u_image[N_UNIFORMS];
 *
 *  For cases where limit for single stage is tested, N_UNIFORMS should be
 *  defined as gl_Max*ImageUniforms + 1, where gl_Max*ImageUniforms is constant
 *  corresponding to tested shader stage.
 *
 *  For case where limit for combined stages is tested:
 *  - u_image name should be appended with the name of shader stage, like
 *  u_image_vertex,
 *  - N_UNIFORMS should be defined as gl_Max*ImageUniforms, where
 *  gl_Max*ImageUniforms is constant corresponding to tested shader stage,
 *  - compilation and linking shall succeed, when sum of all
 *  gl_Max*ImageUniforms corresponding to shader stages is equal (or less) to
 *  gl_MaxCombinedImageUniforms.
 *
 *  All defined image uniforms have to be active. Each shader stage that declare
 *  image uniforms should include following code snippet:
 *  value = 1;
 *  for (int i = 0; i < N_UNIFORMS; ++i)
 *  {
 *      value = imageAtomicAdd(u_image[i], coord, value);
 *  }
 **/
class ImageLoadStoreUniformLimitsTest : public ShaderImageLoadStoreBase
{
private:
	/* Fields */
	/* Results */
	bool m_result_for_combined;
	bool m_result_for_fragment_shader;
	bool m_result_for_geometry_shader;
	bool m_result_for_tesselation_control_shader;
	bool m_result_for_tesselatioon_evaluation_shader;
	bool m_result_for_vertex_shader;

public:
	/* Constructor */
	ImageLoadStoreUniformLimitsTest()
		: m_result_for_combined(false)
		, m_result_for_fragment_shader(false)
		, m_result_for_geometry_shader(false)
		, m_result_for_tesselation_control_shader(false)
		, m_result_for_tesselatioon_evaluation_shader(false)
		, m_result_for_vertex_shader(false)
	{
		/* Nothing to be done */
	}

	/* Methods inherited from SubcaseBase */
	virtual long Cleanup()
	{
		/* Done */
		return NO_ERROR;
	}

	virtual long Run()
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "This test tries to build invalid programs, expect error messages about "
											   "exceeded number of active image uniforms"
											<< tcu::TestLog::EndMessage;

		testFragmentShaderStage();
		testGeometryShaderStage();
		testTesselationControlShaderStage();
		testTesselationEvaluationShaderStage();
		testVertexShaderStage();
		testCombinedShaderStages();

		/* Return error if any stage failed */
		if ((false == m_result_for_combined) || (false == m_result_for_fragment_shader) ||
			(false == m_result_for_geometry_shader) || (false == m_result_for_tesselation_control_shader) ||
			(false == m_result_for_tesselatioon_evaluation_shader) || (false == m_result_for_vertex_shader))
		{
			return ERROR;
		}

		/* Done */
		return NO_ERROR;
	}

	virtual long Setup()
	{
		/* Done */
		return NO_ERROR;
	}

private:
	/** Test fragment shader stage
	 *
	 **/
	void testFragmentShaderStage()
	{
		static const char* const fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxFragmentImageUniforms + 1\n"
			"\n"
			"flat in ivec2 vs_fs_coord;\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_fs_coord, value);\n"
			"    }\n"
			"\n"
			"    discard;\n"
			"}\n\n";

		static const char* const vertex_shader_code = "#version 400 core\n"
													  "#extension GL_ARB_shader_image_load_store : require\n"
													  "\n"
													  "precision highp float;\n"
													  "\n"
													  "     in  ivec2 vs_in_coord;\n"
													  "flat out ivec2 vs_fs_coord;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    vs_fs_coord = vs_in_coord;\n"
													  "}\n\n";

		m_result_for_fragment_shader = !doesProgramLink(fragment_shader_code, 0 /* geometry_shader_code */,
														0 /* tesselation_control_shader_code */,
														0 /* tesselation_evaluation_shader_code */, vertex_shader_code);

		if (false == m_result_for_fragment_shader)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Program which exceeds limit of GL_MAX_FRAGMENT_IMAGE_UNIFORMS was linked successfully."
				<< " File: " << __FILE__ << " Line: " << __LINE__ << " Shader code:\n"
				<< fragment_shader_code << tcu::TestLog::EndMessage;
		}
	}

	/** Test geometry shader stage
	 *
	 **/
	void testGeometryShaderStage()
	{
		static const char* const fragment_shader_code = "#version 400 core\n"
														"#extension GL_ARB_shader_image_load_store : require\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    discard;\n"
														"}\n\n";

		static const char* const geometry_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(points)                   in;\n"
			"layout(points, max_vertices = 1) out;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxGeometryImageUniforms + 1\n"
			"\n"
			"in ivec2 vs_gs_coord[];\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_gs_coord[0], value);\n"
			"    }\n"
			"}\n\n";

		static const char* const vertex_shader_code = "#version 400 core\n"
													  "#extension GL_ARB_shader_image_load_store : require\n"
													  "\n"
													  "precision highp float;\n"
													  "\n"
													  "in  ivec2 vs_in_coord;\n"
													  "out ivec2 vs_gs_coord;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    vs_gs_coord = vs_in_coord;\n"
													  "}\n\n";

		m_result_for_geometry_shader =
			!doesProgramLink(fragment_shader_code, geometry_shader_code, 0 /* tesselation_control_shader_code */,
							 0 /* tesselation_evaluation_shader_code */, vertex_shader_code);

		if (false == m_result_for_geometry_shader)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Program which exceeds limit of GL_MAX_GEOMETRY_IMAGE_UNIFORMS was linked successfully."
				<< " File: " << __FILE__ << " Line: " << __LINE__ << " Shader code:\n"
				<< geometry_shader_code << tcu::TestLog::EndMessage;
		}
	}

	/** Test tesselation control shader stage
	 *
	 **/
	void testTesselationControlShaderStage()
	{
		static const char* const fragment_shader_code = "#version 400 core\n"
														"#extension GL_ARB_shader_image_load_store : require\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    discard;\n"
														"}\n\n";

		static const char* const tesselation_control_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(vertices = 4) out;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxTessControlImageUniforms + 1\n"
			"\n"
			"in ivec2 vs_tcs_coord[];\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_tcs_coord[0], value);\n"
			"    }\n"
			"}\n\n";

		static const char* const tesselation_evaluation_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(quads, equal_spacing, ccw) in;\n"
			"\n"
			"void main()\n"
			"{\n"
			"}\n";

		static const char* const vertex_shader_code = "#version 400 core\n"
													  "#extension GL_ARB_shader_image_load_store : require\n"
													  "\n"
													  "precision highp float;\n"
													  "\n"
													  "in  ivec2 vs_in_coord;\n"
													  "out ivec2 vs_tcs_coord;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    vs_tcs_coord = vs_in_coord;\n"
													  "}\n\n";

		m_result_for_tesselation_control_shader =
			!doesProgramLink(fragment_shader_code, 0 /* geometry_shader_code */, tesselation_control_shader_code,
							 tesselation_evaluation_shader_code, vertex_shader_code);

		if (false == m_result_for_tesselation_control_shader)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Program which exceeds limit of GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS was linked successfully."
				<< " File: " << __FILE__ << " Line: " << __LINE__ << " Shader code:\n"
				<< tesselation_control_shader_code << tcu::TestLog::EndMessage;
		}
	}

	/** Test teselation evaluation shader stage
	 *
	 **/
	void testTesselationEvaluationShaderStage()
	{
		static const char* const fragment_shader_code = "#version 400 core\n"
														"#extension GL_ARB_shader_image_load_store : require\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    discard;\n"
														"}\n\n";

		static const char* const tesselation_evaluation_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(quads, equal_spacing, ccw) in;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxTessEvaluationImageUniforms + 1\n"
			"\n"
			"in ivec2 vs_tes_coord[];\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_tes_coord[0], value);\n"
			"    }\n"
			"}\n\n";

		static const char* const vertex_shader_code = "#version 400 core\n"
													  "#extension GL_ARB_shader_image_load_store : require\n"
													  "\n"
													  "precision highp float;\n"
													  "\n"
													  "in  ivec2 vs_in_coord;\n"
													  "out ivec2 vs_tes_coord;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    vs_tes_coord = vs_in_coord;\n"
													  "}\n\n";

		m_result_for_tesselatioon_evaluation_shader = !doesProgramLink(
			fragment_shader_code, 0 /* geometry_shader_code */, 0 /* tesselation_control_shader_code */,
			tesselation_evaluation_shader_code, vertex_shader_code);

		if (false == m_result_for_tesselatioon_evaluation_shader)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Program which exceeds limit of GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS was linked successfully."
				<< " File: " << __FILE__ << " Line: " << __LINE__ << " Shader code:\n"
				<< tesselation_evaluation_shader_code << tcu::TestLog::EndMessage;
		}
	}

	/** Test vertex shader stage
	 *
	 **/
	void testVertexShaderStage()
	{
		static const char* const fragment_shader_code = "#version 400 core\n"
														"#extension GL_ARB_shader_image_load_store : require\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    discard;\n"
														"}\n\n";

		static const char* const vertex_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"in ivec2 vs_in_coord;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxVertexImageUniforms + 1\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_in_coord, value);\n"
			"    }\n"
			"}\n\n";

		m_result_for_vertex_shader = !doesProgramLink(fragment_shader_code, 0 /* geometry_shader_code */,
													  0 /* tesselation_control_shader_code */,
													  0 /* tesselation_evaluation_shader_code */, vertex_shader_code);

		if (false == m_result_for_vertex_shader)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Program which exceeds limit of GL_MAX_VERTEX_IMAGE_UNIFORMS was linked successfully."
				<< " File: " << __FILE__ << " Line: " << __LINE__ << " Shader code:\n"
				<< vertex_shader_code << tcu::TestLog::EndMessage;
		}
	}

	/** Test combined shader stages
	 *
	 **/
	void testCombinedShaderStages()
	{
		std::string fragment_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxFragmentImageUniforms\n"
			"\n"
			"flat in ivec2 gs_fs_coord;\n"
			"\n"
			"layout(r32i) uniform iimage2D u_image_fragment[N_UNIFORMS];\n"
			"\n"
			"void main()\n"
			"{\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image_fragment[i], gs_fs_coord, value);\n"
			"    }\n"
			"\n"
			"    discard;\n"
			"}\n\n";

		std::string geometry_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(points)                   in;\n"
			"layout(points, max_vertices = 1) out;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxGeometryImageUniforms\n"
			"\n"
			"flat in  ivec2 tes_gs_coord[];\n"
			"flat out ivec2 gs_fs_coord;\n"
			"\n"
			"#ifdef IMAGES\n"
			"layout(r32i) uniform iimage2D u_image_geometry[N_UNIFORMS];\n"
			"#endif\n"
			"\n"
			"void main()\n"
			"{\n"
			"#ifdef IMAGES\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image_geometry[i], tes_gs_coord[0], value);\n"
			"    }\n"
			"\n"
			"#endif\n"
			"    gs_fs_coord = tes_gs_coord[0];\n"
			"    EmitVertex();\n"
			"}\n\n";

		std::string tesselation_control_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(vertices = 4) out;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxTessControlImageUniforms\n"
			"\n"
			"flat in  ivec2 vs_tcs_coord[];\n"
			"flat out ivec2 tcs_tes_coord[];\n"
			"\n"
			"#ifdef IMAGES\n"
			"layout(r32i) uniform iimage2D u_image_tess_control[N_UNIFORMS];\n"
			"#endif\n"
			"\n"
			"void main()\n"
			"{\n"
			"#ifdef IMAGES\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image_tess_control[i], vs_tcs_coord[0], value);\n"
			"    }\n"
			"\n"
			"#endif\n"
			"    tcs_tes_coord[gl_InvocationID] = vs_tcs_coord[0];\n"
			"}\n\n";

		std::string tesselation_evaluation_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"layout(quads, equal_spacing, ccw) in;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxTessEvaluationImageUniforms\n"
			"\n"
			"flat in  ivec2 tcs_tes_coord[];\n"
			"flat out ivec2 tes_gs_coord;\n"
			"\n"
			"#ifdef IMAGES\n"
			"layout(r32i) uniform iimage2D u_image_tess_evaluation[N_UNIFORMS];\n"
			"#endif\n"
			"\n"
			"void main()\n"
			"{\n"
			"#ifdef IMAGES\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image_tess_evaluation[i], tcs_tes_coord[0], value);\n"
			"    }\n"
			"\n"
			"#endif\n"
			"    tes_gs_coord = tcs_tes_coord[0];\n"
			"}\n\n";

		std::string vertex_shader_code =
			"#version 400 core\n"
			"#extension GL_ARB_shader_image_load_store : require\n"
			"\n"
			"precision highp float;\n"
			"\n"
			"     in  ivec2 vs_in_coord;\n"
			"flat out ivec2 vs_tcs_coord;\n"
			"\n"
			"#define N_UNIFORMS gl_MaxVertexImageUniforms\n"
			"\n"
			"#ifdef IMAGES\n"
			"layout(r32i) uniform iimage2D u_image[N_UNIFORMS];\n"
			"#endif\n"
			"\n"
			"void main()\n"
			"{\n"
			"#ifdef IMAGES\n"
			"    int value = 1;\n"
			"\n"
			"    for (int i = 0; i < N_UNIFORMS; ++i)\n"
			"    {\n"
			"        value = imageAtomicAdd(u_image[i], vs_in_coord, value);\n"
			"    }\n"
			"\n"
			"#endif\n"
			"    vs_tcs_coord = vs_tcs_coord;\n"
			"}\n\n";

		/* Active image uniform limits */
		GLint max_combined_image_uniforms				= 0;
		GLint max_fragment_image_uniforms				= 0;
		GLint max_geometry_image_uniforms				= 0;
		GLint max_tesselation_control_image_uniforms	= 0;
		GLint max_tesselation_evaluation_image_uniforms = 0;
		GLint max_vertex_image_uniforms					= 0;

		/* Get limit values */
		glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, &max_combined_image_uniforms);
		glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &max_fragment_image_uniforms);
		glGetIntegerv(GL_MAX_GEOMETRY_IMAGE_UNIFORMS, &max_geometry_image_uniforms);
		glGetIntegerv(GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, &max_tesselation_control_image_uniforms);
		glGetIntegerv(GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, &max_tesselation_evaluation_image_uniforms);
		glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &max_vertex_image_uniforms);
		GLU_EXPECT_NO_ERROR(glGetError(), "GetIntegerv");

		if (max_vertex_image_uniforms)
			vertex_shader_code.insert(18, "#define IMAGES\n");
		if (max_geometry_image_uniforms)
			geometry_shader_code.insert(18, "#define IMAGES\n");
		if (max_tesselation_control_image_uniforms)
			tesselation_control_shader_code.insert(18, "#define IMAGES\n");
		if (max_tesselation_evaluation_image_uniforms)
			tesselation_evaluation_shader_code.insert(18, "#define IMAGES\n");

		/* Check if program builds */
		m_result_for_combined =
			!doesProgramLink(fragment_shader_code.c_str(),
							 geometry_shader_code.c_str(),
							 tesselation_control_shader_code.c_str(),
							 tesselation_evaluation_shader_code.c_str(),
							 vertex_shader_code.c_str());

		/* Result depends on the limit values */
		if (max_combined_image_uniforms >=
			(max_fragment_image_uniforms + max_geometry_image_uniforms + max_tesselation_control_image_uniforms +
			 max_tesselation_evaluation_image_uniforms + max_vertex_image_uniforms))
		{
			/* In this case, combined image uniforms limit cannot be exeeded, therefore successful linking is expected */
			m_result_for_combined = !m_result_for_combined;

			if (false == m_result_for_combined)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "There was an error while building a program."
					<< " File: " << __FILE__ << " Line: " << __LINE__ << " Vertex shader code:\n"
					<< vertex_shader_code << "\nTesselation control shader code:\n"
					<< tesselation_control_shader_code << "\nTesselation evaluation shader code:\n"
					<< tesselation_evaluation_shader_code << "\nGeometry shader code:\n"
					<< geometry_shader_code << "\nFragment shader code:\n"
					<< fragment_shader_code << tcu::TestLog::EndMessage;
			}
		}
		else
		{
			/* In this case, combined image uniforms can be exceeded, therefore failed linking is expected */
			if (false == m_result_for_combined)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "Program which exceeds limit of GL_MAX_COMBINED_IMAGE_UNIFORMS was linked successfully."
					<< " File: " << __FILE__ << " Line: " << __LINE__ << " Vertex shader code:\n"
					<< vertex_shader_code << "\nTesselation control shader code:\n"
					<< tesselation_control_shader_code << "\nTesselation evaluation shader code:\n"
					<< tesselation_evaluation_shader_code << "\nGeometry shader code:\n"
					<< geometry_shader_code << "\nFragment shader code:\n"
					<< fragment_shader_code << tcu::TestLog::EndMessage;
			}
		}
	}

	/** Check if program builds successfully
	 *
	 * @param fragment_shader_code               Source code for fragment shader stage
	 * @param geometry_shader_code               Source code for geometry shader stage
	 * @param tesselation_control_shader_code    Source code for tesselation control shader stage
	 * @param tesselation_evaluation_shader_code Source code for tesselation evaluation shader stage
	 * @param vertex_shader_code                 Source code for vertex shader stage
	 *
	 * @return true if program was built without errors, false otherwise
	 **/
	bool doesProgramLink(const char* fragment_shader_code, const char* geometry_shader_code,
						 const char* tesselation_control_shader_code, const char* tesselation_evaluation_shader_code,
						 const char* vertex_shader_code)
	{
		bool   is_program_built = true;
		GLuint program_id		= 0;

		program_id =
			BuildProgram(vertex_shader_code, tesselation_control_shader_code, tesselation_evaluation_shader_code,
						 geometry_shader_code, fragment_shader_code, &is_program_built);

		if (0 != program_id)
		{
			glDeleteProgram(program_id);
		}

		return is_program_built;
	}
};
}

ShaderImageLoadStoreTests::ShaderImageLoadStoreTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_image_load_store", "")
{
}

ShaderImageLoadStoreTests::~ShaderImageLoadStoreTests(void)
{
}

void ShaderImageLoadStoreTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "basic-api-get", TestSubcase::Create<BasicAPIGet>));
	addChild(new TestSubcase(m_context, "basic-api-bind", TestSubcase::Create<BasicAPIBind>));
	addChild(new TestSubcase(m_context, "basic-api-barrier", TestSubcase::Create<BasicAPIBarrier>));
	addChild(new TestSubcase(m_context, "basic-api-texParam", TestSubcase::Create<BasicAPITexParam>));
	addChild(new TestSubcase(m_context, "basic-allFormats-store", TestSubcase::Create<BasicAllFormatsStore>));
	addChild(new TestSubcase(m_context, "basic-allFormats-load", TestSubcase::Create<BasicAllFormatsLoad>));
	addChild(new TestSubcase(m_context, "basic-allFormats-storeGeometryStages",
							 TestSubcase::Create<BasicAllFormatsStoreGeometryStages>));
	addChild(new TestSubcase(m_context, "basic-allFormats-loadGeometryStages",
							 TestSubcase::Create<BasicAllFormatsLoadGeometryStages>));
	addChild(new TestSubcase(m_context, "basic-allFormats-loadStoreComputeStage",
							 TestSubcase::Create<BasicAllFormatsLoadStoreComputeStage>));
	addChild(new TestSubcase(m_context, "basic-allTargets-store", TestSubcase::Create<BasicAllTargetsStore>));
	addChild(new TestSubcase(m_context, "basic-allTargets-load-nonMS", TestSubcase::Create<BasicAllTargetsLoadNonMS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-load-ms", TestSubcase::Create<BasicAllTargetsLoadMS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomic", TestSubcase::Create<BasicAllTargetsAtomic>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreVS", TestSubcase::Create<BasicAllTargetsLoadStoreVS>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreTCS", TestSubcase::Create<BasicAllTargetsLoadStoreTCS>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreTES", TestSubcase::Create<BasicAllTargetsLoadStoreTES>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreGS", TestSubcase::Create<BasicAllTargetsLoadStoreGS>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreCS", TestSubcase::Create<BasicAllTargetsLoadStoreCS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicVS", TestSubcase::Create<BasicAllTargetsAtomicVS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicTCS", TestSubcase::Create<BasicAllTargetsAtomicTCS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicGS", TestSubcase::Create<BasicAllTargetsAtomicGS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicCS", TestSubcase::Create<BasicAllTargetsAtomicCS>));
	addChild(new TestSubcase(m_context, "basic-glsl-misc", TestSubcase::Create<BasicGLSLMisc>));
	addChild(new TestSubcase(m_context, "basic-glsl-earlyFragTests", TestSubcase::Create<BasicGLSLEarlyFragTests>));
	addChild(new TestSubcase(m_context, "basic-glsl-const", TestSubcase::Create<BasicGLSLConst>));
	addChild(new TestSubcase(m_context, "advanced-sync-imageAccess", TestSubcase::Create<AdvancedSyncImageAccess>));
	addChild(new TestSubcase(m_context, "advanced-sync-vertexArray", TestSubcase::Create<AdvancedSyncVertexArray>));
	addChild(new TestSubcase(m_context, "advanced-sync-drawIndirect", TestSubcase::Create<AdvancedSyncDrawIndirect>));
	addChild(new TestSubcase(m_context, "advanced-sync-textureUpdate", TestSubcase::Create<AdvancedSyncTextureUpdate>));
	addChild(new TestSubcase(m_context, "advanced-sync-imageAccess2", TestSubcase::Create<AdvancedSyncImageAccess2>));
	addChild(new TestSubcase(m_context, "advanced-sync-bufferUpdate", TestSubcase::Create<AdvancedSyncBufferUpdate>));
	addChild(new TestSubcase(m_context, "advanced-allStages-oneImage", TestSubcase::Create<AdvancedAllStagesOneImage>));
	addChild(new TestSubcase(m_context, "advanced-memory-dependentInvocation",
							 TestSubcase::Create<AdvancedMemoryDependentInvocation>));
	addChild(new TestSubcase(m_context, "advanced-memory-order", TestSubcase::Create<AdvancedMemoryOrder>));
	addChild(new TestSubcase(m_context, "advanced-sso-simple", TestSubcase::Create<AdvancedSSOSimple>));
	addChild(new TestSubcase(m_context, "advanced-sso-atomicCounters", TestSubcase::Create<AdvancedSSOAtomicCounters>));
	addChild(new TestSubcase(m_context, "advanced-sso-subroutine", TestSubcase::Create<AdvancedSSOSubroutine>));
	addChild(new TestSubcase(m_context, "advanced-sso-perSample", TestSubcase::Create<AdvancedSSOPerSample>));
	addChild(new TestSubcase(m_context, "advanced-copyImage", TestSubcase::Create<AdvancedCopyImage>));
	addChild(new TestSubcase(m_context, "advanced-allMips", TestSubcase::Create<AdvancedAllMips>));
	addChild(new TestSubcase(m_context, "advanced-cast", TestSubcase::Create<AdvancedCast>));
	addChild(
		new TestSubcase(m_context, "single-byte_data_alignment", TestSubcase::Create<ImageLoadStoreDataAlignmentTest>));
	addChild(
		new TestSubcase(m_context, "non-layered_binding", TestSubcase::Create<ImageLoadStoreNonLayeredBindingTest>));
	addChild(
		new TestSubcase(m_context, "incomplete_textures", TestSubcase::Create<ImageLoadStoreIncompleteTexturesTest>));
	addChild(new TestSubcase(m_context, "multiple-uniforms", TestSubcase::Create<ImageLoadStoreMultipleUniformsTest>));
	addChild(
		new TestSubcase(m_context, "early-fragment-tests", TestSubcase::Create<ImageLoadStoreEarlyFragmentTestsTest>));
	addChild(new TestSubcase(m_context, "negative-uniform", TestSubcase::Create<NegativeUniform>));
	addChild(new TestSubcase(m_context, "negative-bind", TestSubcase::Create<NegativeBind>));
	addChild(new TestSubcase(m_context, "negative-compileErrors", TestSubcase::Create<NegativeCompileErrors>));
	addChild(new TestSubcase(m_context, "negative-linkErrors", TestSubcase::Create<NegativeLinkErrors>));
	addChild(new TestSubcase(m_context, "uniform-limits", TestSubcase::Create<ImageLoadStoreUniformLimitsTest>));
}
}
