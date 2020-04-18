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

#include "gl4cES31CompatibilityTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include <assert.h>
#include <cstdarg>
#include <map>

namespace gl4cts
{
namespace es31compatibility
{
using namespace glw;
namespace
{
typedef tcu::Vec2  vec2;
typedef tcu::Vec4  vec4;
typedef tcu::IVec4 ivec4;
typedef tcu::UVec4 uvec4;
typedef tcu::Mat4  mat4;

enum Target
{
	T2D = 0,
	T3D,
	TCM,
	T2DA
};

const char* const kGLSLVer = "#version 310 es";
const char* const kGLSLSIA = NL "#extension GL_OES_shader_image_atomic : require";
const char* const kGLSLPrec =
	NL "precision highp float;" NL "precision highp int;" NL "precision highp sampler2D;" NL
	   "precision highp sampler3D;" NL "precision highp samplerCube;" NL "precision highp sampler2DArray;" NL
	   "precision highp isampler2D;" NL "precision highp isampler3D;" NL "precision highp isamplerCube;" NL
	   "precision highp isampler2DArray;" NL "precision highp usampler2D;" NL "precision highp usampler3D;" NL
	   "precision highp usamplerCube;" NL "precision highp usampler2DArray;" NL "precision highp image2D;" NL
	   "precision highp image3D;" NL "precision highp imageCube;" NL "precision highp image2DArray;" NL
	   "precision highp iimage2D;" NL "precision highp iimage3D;" NL "precision highp iimageCube;" NL
	   "precision highp iimage2DArray;" NL "precision highp uimage2D;" NL "precision highp uimage3D;" NL
	   "precision highp uimageCube;" NL "precision highp uimage2DArray;";

class ShaderImageLoadStoreBase : public deqp::SubcaseBase
{
public:
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

	bool IsVSFSAvailable(int requiredVS, int requiredFS)
	{
		GLint imagesVS, imagesFS;
		glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &imagesVS);
		glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &imagesFS);
		if (imagesVS >= requiredVS && imagesFS >= requiredFS)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << requiredVS << " VS image uniforms but only " << imagesVS << " available."
				   << std::endl
				   << "Required " << requiredFS << " FS image uniforms but only " << imagesFS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}
	bool IsSSBInVSFSAvailable(int required)
	{
		GLint blocksVS, blocksFS;
		glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &blocksVS);
		glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &blocksFS);
		if (blocksVS >= required && blocksFS >= required)
			return true;
		else
		{
			std::ostringstream reason;
			reason << "Required " << required << " VS storage blocks but only " << blocksVS << " available."
				   << std::endl
				   << "Required " << required << " FS storage blocks but only " << blocksFS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
	}

	bool IsImageAtomicSupported()
	{
		bool is_at_least_gl_45 =
			(glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
		bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");
		if (!(is_at_least_gl_45 || is_arb_es31_compatibility))
		{
			std::ostringstream reason;
			reason << "Required GL_OES_shader_image_atomic is not available." << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
		return true;
	}

	bool AreOutputsAvailable(int required)
	{
		GLint outputs;
		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &outputs);
		if (outputs < required)
		{
			std::ostringstream reason;
			reason << "Required " << required << " shader output resources but only " << outputs << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
		return true;
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

	bool Equal(const vec4& v0, const vec4& v1, GLenum internalformat)
	{
		if (internalformat == GL_RGBA8_SNORM || internalformat == GL_RGBA8)
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

	template <typename T>
	bool CompareValues(T* map_data, int kSize, const T& expected_value, GLenum internalformat = 0, int layers = 1)
	{
		for (int i = 0; i < kSize * kSize * layers; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str() << "." << tcu::TestLog::EndMessage;
				return false;
			}
		}
		return true;
	}
	template <typename T>
	bool CompareValues(bool always, T* map_data, int kSize, const T& expected_value, GLenum internalformat = 0,
					   int layers = 1)
	{
		(void)internalformat;
		for (int i = 0; i < kSize * kSize * layers; ++i)
		{
			if (always)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str() << "." << tcu::TestLog::EndMessage;
			}
		}
		return true;
	}

	bool CheckFB(vec4 expected)
	{
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		const tcu::PixelFormat&  pixelFormat  = renderTarget.getPixelFormat();
		vec4 g_color_eps = vec4(1.f / (float)(1 << pixelFormat.redBits), 1.f / (float)(1 << pixelFormat.greenBits),
								1.f / (float)(1 << pixelFormat.blueBits), 1.f);
		vec4				 g_color_max = vec4(255);
		std::vector<GLubyte> fb(getWindowWidth() * getWindowHeight() * 4);
		int					 fb_w = getWindowWidth();
		int					 fb_h = getWindowHeight();
		glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, &fb[0]);
		for (GLint i = 0, y = 0; y < fb_h; ++y)
			for (GLint x = 0; x < fb_w; ++x, i += 4)
			{
				if (fabs(fb[i + 0] / g_color_max[0] - expected[0]) > g_color_eps[0] ||
					fabs(fb[i + 1] / g_color_max[1] - expected[1]) > g_color_eps[1] ||
					fabs(fb[i + 2] / g_color_max[2] - expected[2]) > g_color_eps[2])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Incorrect framebuffer color at pixel (" << x << ", " << y
						<< "). Color is (" << fb[i + 0] / g_color_max[0] << ", " << fb[i + 1] / g_color_max[1] << ", "
						<< fb[i + 2] / g_color_max[2] << "). Color should be (" << expected[0] << ", " << expected[1]
						<< ", " << expected[2] << ")." << tcu::TestLog::EndMessage;
					return false;
				}
			}
		return true;
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
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader Info Log:\n"
													<< log << tcu::TestLog::EndMessage;
			}
			return false;
		}
		return true;
	}

	GLuint BuildProgram(const char* src_vs, const char* src_fs, bool SIAvs = false, bool SIAfs = false)
	{
		std::ostringstream osvs, osfs;
		osvs << kGLSLVer << (SIAvs ? kGLSLSIA : "\n") << kGLSLPrec;
		osfs << kGLSLVer << (SIAfs ? kGLSLSIA : "\n") << kGLSLPrec;
		std::string hvs = osvs.str();
		std::string hfs = osfs.str();

		const GLuint p = glCreateProgram();

		if (src_vs)
		{
			GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { hvs.c_str(), src_vs };
			glShaderSource(sh, 2, src, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << src[0] << src[1] << tcu::TestLog::EndMessage;
				return p;
			}
		}
		if (src_fs)
		{
			GLuint sh = glCreateShader(GL_FRAGMENT_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { hfs.c_str(), src_fs };
			glShaderSource(sh, 2, src, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << src[0] << src[1] << tcu::TestLog::EndMessage;
				return p;
			}
		}
		if (!LinkProgram(p))
		{
			if (src_vs)
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << hvs.c_str() << src_vs << tcu::TestLog::EndMessage;
			if (src_fs)
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << hfs.c_str() << src_fs << tcu::TestLog::EndMessage;
			return p;
		}

		return p;
	}

	GLuint CreateComputeProgram(const std::string& cs, bool SIA = false)
	{
		std::ostringstream oscs;
		oscs << kGLSLVer << (SIA ? kGLSLSIA : "\n") << kGLSLPrec;
		std::string  hcs = oscs.str();
		const GLuint p   = glCreateProgram();

		if (!cs.empty())
		{
			const GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { hcs.c_str(), cs.c_str() };
			glShaderSource(sh, 2, src, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << src[0] << src[1] << tcu::TestLog::EndMessage;
				return p;
			}
		}
		if (!LinkProgram(p))
		{
			if (!cs.empty())
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << hcs.c_str() << cs.c_str() << tcu::TestLog::EndMessage;
			return p;
		}

		return p;
	}

	GLuint BuildShaderProgram(GLenum type, const char* src)
	{
		const char* const src3[3] = { kGLSLVer, kGLSLPrec, src };
		const GLuint	  p		  = glCreateShaderProgramv(type, 3, src3);
		GLint			  status;
		glGetProgramiv(p, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLchar log[1024];
			glGetProgramInfoLog(p, sizeof(log), NULL, log);
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program Info Log:\n"
												<< log << "\n"
												<< src3[0] << "\n"
												<< src3[1] << "\n"
												<< src3[2] << tcu::TestLog::EndMessage;
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

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 11,

							  reinterpret_cast<void*>(sizeof(float) * 5));

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
		case GL_R32F:
			return "r32f";

		case GL_RGBA32UI:
			return "rgba32ui";
		case GL_RGBA16UI:
			return "rgba16ui";
		case GL_RGBA8UI:
			return "rgba8ui";
		case GL_R32UI:
			return "r32ui";

		case GL_RGBA32I:
			return "rgba32i";
		case GL_RGBA16I:
			return "rgba16i";
		case GL_RGBA8I:
			return "rgba8i";
		case GL_R32I:
			return "r32i";

		case GL_RGBA8:
			return "rgba8";

		case GL_RGBA8_SNORM:
			return "rgba8_snorm";
		}

		assert(0);
		return "";
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

	bool CheckMax(GLenum pname, GLint min_value)
	{
		GLboolean b;
		GLint	 i;
		GLfloat   f;
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
												<< " should be " << texture << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LEVEL, unit, &i);
		if (i != level)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LEVEL is " << i
												<< " should be " << level << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LAYERED, unit, &i);
		glGetBooleani_v(GL_IMAGE_BINDING_LAYERED, unit, &b);
		if (i != layered || b != layered)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYERED is " << i
												<< " should be " << layered << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_LAYER, unit, &i);
		if (i != layer)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_LAYER is " << i
												<< " should be " << layer << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_ACCESS, unit, &i);
		if (static_cast<GLenum>(i) != access)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_ACCESS is " << i
												<< " should be " << access << "." << tcu::TestLog::EndMessage;
			return false;
		}

		glGetIntegeri_v(GL_IMAGE_BINDING_FORMAT, unit, &i);
		if (static_cast<GLenum>(i) != format)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GL_IMAGE_BINDING_FORMAT is " << i
												<< " should be " << format << "." << tcu::TestLog::EndMessage;
			return false;
		}

		return true;
	}
	const char* EnumToString(GLenum e)
	{
		switch (e)
		{
		case GL_TEXTURE_2D:
			return "GL_TEXTURE_2D";
		case GL_TEXTURE_3D:
			return "GL_TEXTURE_3D";
		case GL_TEXTURE_CUBE_MAP:
			return "GL_TEXTURE_CUBE_MAP";
		case GL_TEXTURE_2D_ARRAY:
			return "GL_TEXTURE_2D_ARRAY";

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
	case GL_TEXTURE_2D:
		return GL_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_IMAGE_3D;
	case GL_TEXTURE_CUBE_MAP:
		return GL_IMAGE_CUBE;
	case GL_TEXTURE_2D_ARRAY:
		return GL_IMAGE_2D_ARRAY;
	}
	assert(0);
	return 0;
}

template <>
GLenum ShaderImageLoadStoreBase::ImageType<ivec4>(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_2D:
		return GL_INT_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_INT_IMAGE_3D;
	case GL_TEXTURE_CUBE_MAP:
		return GL_INT_IMAGE_CUBE;
	case GL_TEXTURE_2D_ARRAY:
		return GL_INT_IMAGE_2D_ARRAY;
	}
	assert(0);
	return 0;
}

template <>
GLenum ShaderImageLoadStoreBase::ImageType<uvec4>(GLenum target)
{
	switch (target)
	{
	case GL_TEXTURE_2D:
		return GL_UNSIGNED_INT_IMAGE_2D;
	case GL_TEXTURE_3D:
		return GL_UNSIGNED_INT_IMAGE_3D;
	case GL_TEXTURE_CUBE_MAP:
		return GL_UNSIGNED_INT_IMAGE_CUBE;
	case GL_TEXTURE_2D_ARRAY:
		return GL_UNSIGNED_INT_IMAGE_2D_ARRAY;
	}
	assert(0);
	return 0;
}

int Components(GLenum e)
{
	return (e == GL_RED || e == GL_RED_INTEGER) ? 1 : 4;
}

bool Shorts(GLenum e)
{
	return (e == GL_RGBA16I || e == GL_RGBA16UI);
}

bool Bytes(GLenum e)
{
	return (e == GL_RGBA8I || e == GL_RGBA8UI || e == GL_RGBA8 || e == GL_RGBA8_SNORM);
}

template <typename T>
class ShortByteData
{
public:
	std::vector<T>		 data;
	std::vector<GLshort> datas;
	std::vector<GLbyte>  datab;

	ShortByteData(int size, const T& value, GLenum internalformat, GLenum format)
		: data(size * size, value), datas(size * size * 4), datab(size * size * 4)
	{
		if (Components(format) == 1)
			for (unsigned i = 0; i < data.size() / 4; ++i)
			{
				data[i][0] = data[i * 4][0];
				data[i][1] = data[i * 4 + 1][0];
				data[i][2] = data[i * 4 + 2][0];
				data[i][3] = data[i * 4 + 3][0];
			}
		if (Shorts(internalformat))
		{
			for (unsigned i = 0; i < datas.size(); i += 4)
			{
				datas[i]	 = static_cast<GLshort>(data[i / 4][0]);
				datas[i + 1] = static_cast<GLshort>(data[i / 4][1]);
				datas[i + 2] = static_cast<GLshort>(data[i / 4][2]);
				datas[i + 3] = static_cast<GLshort>(data[i / 4][3]);
			}
		}
		if (Bytes(internalformat))
		{
			for (unsigned i = 0; i < datas.size(); i += 4)
			{
				if (internalformat == GL_RGBA8I || internalformat == GL_RGBA8UI)
				{
					datab[i]	 = static_cast<GLbyte>(data[i / 4][0]);
					datab[i + 1] = static_cast<GLbyte>(data[i / 4][1]);
					datab[i + 2] = static_cast<GLbyte>(data[i / 4][2]);
					datab[i + 3] = static_cast<GLbyte>(data[i / 4][3]);
				}
				else if (internalformat == GL_RGBA8)
				{
					datab[i]	 = static_cast<GLbyte>(data[i / 4][0] * 255);
					datab[i + 1] = static_cast<GLbyte>(data[i / 4][1] * 255);
					datab[i + 2] = static_cast<GLbyte>(data[i / 4][2] * 255);
					datab[i + 3] = static_cast<GLbyte>(data[i / 4][3] * 255);
				}
				else
				{ // GL_RGBA8_SNORM
					datab[i]	 = static_cast<GLbyte>(data[i / 4][0] * 127);
					datab[i + 1] = static_cast<GLbyte>(data[i / 4][1] * 127);
					datab[i + 2] = static_cast<GLbyte>(data[i / 4][2] * 127);
					datab[i + 3] = static_cast<GLbyte>(data[i / 4][3] * 127);
				}
			}
		}
	}
};

//-----------------------------------------------------------------------------
// 1.1.1 BasicAPIGet
//-----------------------------------------------------------------------------
class BasicAPIGet : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		if (!CheckMax(GL_MAX_IMAGE_UNITS, 4))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_IMAGE_UNITS value is invalid." << tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, 4))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_VERTEX_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_VERTEX_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_FRAGMENT_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_COMBINED_IMAGE_UNIFORMS, 4))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_COMBINED_IMAGE_UNIFORMS value is invalid."
				<< tcu::TestLog::EndMessage;
			return ERROR;
		}
		if (!CheckMax(GL_MAX_COMPUTE_IMAGE_UNIFORMS, 4))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "GL_MAX_COMPUTE_IMAGE_UNIFORMS value is invalid."
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
		bool status = true;
		for (GLuint index = 0; index < 4; ++index)
		{
			if (!CheckBinding(index, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI))
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "Binding point " << index
													<< " has invalid default state." << tcu::TestLog::EndMessage;
				status = false;
			}
		}

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 4, GL_RG32F, 16, 16, 4);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		if (!CheckBinding(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F))
			status = false;

		glBindImageTexture(3, m_texture, 1, GL_TRUE, 1, GL_WRITE_ONLY, GL_RGBA8);
		if (!CheckBinding(3, m_texture, 1, GL_TRUE, 1, GL_WRITE_ONLY, GL_RGBA8))
			status = false;

		glBindImageTexture(1, m_texture, 3, GL_FALSE, 2, GL_READ_WRITE, GL_RGBA8UI);
		if (!CheckBinding(1, m_texture, 3, GL_FALSE, 2, GL_READ_WRITE, GL_RGBA8UI))
			status = false;

		glBindImageTexture(2, m_texture, 4, GL_FALSE, 3, GL_READ_ONLY, GL_R32I);
		if (!CheckBinding(2, m_texture, 4, GL_FALSE, 3, GL_READ_ONLY, GL_R32I))
			status = false;

		glDeleteTextures(1, &m_texture);
		m_texture = 0;

		for (GLuint index = 0; index < 4; ++index)
		{
			GLint name;
			glGetIntegeri_v(GL_IMAGE_BINDING_NAME, index, &name);
			if (name != 0)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Binding point " << index
					<< " should be set to 0 after texture deletion." << tcu::TestLog::EndMessage;
				status = false;
			}
			if (!CheckBinding(index, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI))
				status = false;
		}

		return status ? NO_ERROR : ERROR;
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
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT | GL_UNIFORM_BARRIER_BIT |
						GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_COMMAND_BARRIER_BIT |
						GL_PIXEL_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT |
						GL_FRAMEBUFFER_BARRIER_BIT | GL_TRANSFORM_FEEDBACK_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT |
						GL_SHADER_STORAGE_BARRIER_BIT);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		return NO_ERROR;
	}
};

class BasicAPIBarrierByRegion : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{
		glMemoryBarrierByRegion(GL_UNIFORM_BARRIER_BIT);
		glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
		glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glMemoryBarrierByRegion(GL_FRAMEBUFFER_BARRIER_BIT);
		glMemoryBarrierByRegion(GL_ATOMIC_COUNTER_BARRIER_BIT);
		glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);

		glMemoryBarrierByRegion(GL_UNIFORM_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT |
								GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT |
								GL_ATOMIC_COUNTER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		glMemoryBarrierByRegion(GL_ALL_BARRIER_BITS);
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
		glTexStorage2D(GL_TEXTURE_2D, 5, GL_RG32F, 16, 16);

		GLint   i;
		GLfloat f;

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
class BasicAllFormatsStoreFS : public ShaderImageLoadStoreBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 1))
			return NOT_SUPPORTED;

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Write(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;

		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;
		if (!Write(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;

		if (!Write(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;
		if (!Write(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;

		if (!Write(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		GLuint		   program = BuildProgram(src_vs, GenFS(internalformat, write_value).c_str());
		const int	  kSize   = 11;
		std::vector<T> data(kSize * kSize);
		GLuint		   texture;
		glGenTextures(1, &texture);
		glUseProgram(program);

		GLuint unit = 2;
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_2D, 0);

		glViewport(0, 0, kSize, kSize);
		glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, texture);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		GLuint		   c_program = CreateComputeProgram(GenC(write_value));
		std::vector<T> out_data(kSize * kSize);
		GLuint		   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		T* map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str()
					<< ". Format is: " << FormatEnumToString(internalformat).c_str() << ". Unit is: " << unit << "."
					<< tcu::TestLog::EndMessage;
				glDeleteTextures(1, &texture);
				glUseProgram(0);
				glDeleteProgram(program);
				glDeleteProgram(c_program);
				glDeleteBuffers(1, &m_buffer);
				return false;
			}
		}
		glDeleteTextures(1, &texture);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(c_program);
		glDeleteBuffers(1, &m_buffer);
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
		os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) writeonly uniform "
		   << TypePrefix<T>() << "image2D g_image;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
								 "  imageStore(g_image, coord, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "  discard;" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenC(const T& value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "uniform "
		   << TypePrefix<T>() << "sampler2D g_sampler;" NL "layout(std430) buffer OutputBuffer {" NL "  "
		   << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL
			  "  data[gl_LocalInvocationIndex] = texelFetch(g_sampler, ivec2(gl_LocalInvocationID), 0);" NL
			  "  //data[gl_LocalInvocationIndex] = "
		   << value << ";" NL "}";
		return os.str();
	}
};

class BasicAllFormatsStoreCS : public ShaderImageLoadStoreBase
{
	virtual long Setup()
	{
		return NO_ERROR;
	}

	template <typename T>
	std::string GenCS(GLenum internalformat, const T& value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 4" NL "layout(" << FormatEnumToString(internalformat) << ") writeonly uniform "
		   << TypePrefix<T>()
		   << "image2D g_image;" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "void main() {" NL
			  "  ivec2 thread_xy = ivec2(gl_LocalInvocationID);" NL "  imageStore(g_image, thread_xy, "
		   << TypePrefix<T>() << "vec4" << value << ");" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenC(const T& value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 4" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "uniform "
		   << TypePrefix<T>() << "sampler2D g_sampler;" NL "layout(std430) buffer OutputBuffer {" NL "  "
		   << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL
			  "  data[gl_LocalInvocationIndex] = texelFetch(g_sampler, ivec2(gl_LocalInvocationID), 0);" NL
			  "  //data[gl_LocalInvocationIndex] = "
		   << TypePrefix<T>() << "vec4" << value << ";" NL "}";
		return os.str();
	}

	template <typename T>
	bool WriteCS(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const int kSize   = 4;
		GLuint	program = CreateComputeProgram(GenCS(internalformat, write_value));

		std::vector<T> data(kSize * kSize);
		GLuint		   texture;
		glGenTextures(1, &texture);

		GLuint unit = 0;
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat);
		glUseProgram(program);
		glDispatchCompute(1, 1, 1);

		glBindTexture(GL_TEXTURE_2D, texture);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		GLuint		   c_program = CreateComputeProgram(GenC(expected_value));
		std::vector<T> out_data(kSize * kSize);
		GLuint		   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		T* map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str()
					<< ". Format is: " << FormatEnumToString(internalformat).c_str() << ". Unit is: " << unit << "."
					<< tcu::TestLog::EndMessage;
				glDeleteTextures(1, &texture);
				glUseProgram(0);
				glDeleteProgram(program);
				glDeleteProgram(c_program);
				glDeleteBuffers(1, &m_buffer);
				return false;
			}
		}
		glDeleteTextures(1, &texture);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(c_program);
		glDeleteBuffers(1, &m_buffer);

		return true;
	}

	virtual long Run()
	{

		if (!WriteCS(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!WriteCS(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!WriteCS(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;

		if (!WriteCS(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!WriteCS(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;
		if (!WriteCS(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!WriteCS(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;

		if (!WriteCS(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!WriteCS(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;
		if (!WriteCS(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!WriteCS(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;

		if (!WriteCS(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;

		if (!WriteCS(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		return NO_ERROR;
	}
};
//-----------------------------------------------------------------------------
// 1.2.2 BasicAllFormatsLoad
//-----------------------------------------------------------------------------
class BasicAllFormatsLoadFS : public ShaderImageLoadStoreBase
{
	GLuint m_vao, m_vbo;

	virtual long Setup()
	{
		m_vao = 0;
		m_vbo = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 1) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Read(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f), GL_RGBA, GL_FLOAT))
			return ERROR;
		if (!Read(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), GL_RED, GL_FLOAT))
			return ERROR;
		if (!Read(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f), GL_RGBA, GL_FLOAT))
			return ERROR;

		if (!Read(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1), GL_RED_INTEGER, GL_INT))
			return ERROR;
		if (!Read(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_SHORT))
			return ERROR;
		if (!Read(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_BYTE))
			return ERROR;

		if (!Read(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1), GL_RED_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT))
			return ERROR;
		if (!Read(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_BYTE))
			return ERROR;

		if (!Read(GL_RGBA8, vec4(1.0f), vec4(1.0f), GL_RGBA, GL_UNSIGNED_BYTE))
			return ERROR;

		if (!Read(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f), GL_RGBA, GL_BYTE))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value, GLenum format, GLenum type)
	{
		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		GLuint			 program = BuildProgram(src_vs, GenFS(internalformat, expected_value).c_str());
		const int		 kSize   = 11;
		ShortByteData<T> d(kSize, value, internalformat, format);
		GLuint			 texture;
		glGenTextures(1, &texture);
		GLuint unit = 1;
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		if (Shorts(internalformat))
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datas[0]);
		else if (Bytes(internalformat))
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datab[0]);
		else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glViewport(0, 0, kSize, kSize);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program);
		glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);
		glBindVertexArray(m_vao);

		std::vector<T> out_data(kSize * kSize);
		GLuint		   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		T* map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str()
					<< ". Format is: " << FormatEnumToString(internalformat).c_str() << ". Unit is: " << unit << "."
					<< tcu::TestLog::EndMessage;
				glUseProgram(0);
				glDeleteProgram(program);
				glDeleteTextures(1, &texture);
				glDeleteBuffers(1, &m_buffer);
				return false;
			}
		}
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(1, &texture);
		glDeleteBuffers(1, &m_buffer);
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
		os << NL "#define KSIZE 11" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 1) readonly uniform " << TypePrefix<T>()
		   << "image2D g_image;" NL "layout(std430) buffer OutputBuffer {" NL "  " << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image, coord);" NL "  data[coord.y * KSIZE + coord.x] = v;" NL
								 "  //data[coord.y * KSIZE + coord.x] = "
		   << TypePrefix<T>() << "vec4" << expected_value << ";" NL "  discard;" NL "}";
		return os.str();
	}
};

class BasicAllFormatsLoadCS : public ShaderImageLoadStoreBase
{
	virtual long Setup()
	{
		return NO_ERROR;
	}

	template <typename T>
	std::string GenCS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 4" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 1) readonly uniform " << TypePrefix<T>()
		   << "image2D g_image;" NL "layout(std430) buffer OutputBuffer {" NL "  " << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			  "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image, coord);" NL "  data[gl_LocalInvocationIndex] = v;" NL
								 "  //data[gl_LocalInvocationIndex] = "
		   << TypePrefix<T>() << "vec4" << expected_value << ";" NL "}";
		return os.str();
	}

	virtual long Run()
	{
		if (!ReadCS(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f), GL_RGBA, GL_FLOAT))
			return ERROR;
		if (!ReadCS(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f), GL_RED, GL_FLOAT))
			return ERROR;
		if (!ReadCS(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f), GL_RGBA, GL_FLOAT))
			return ERROR;

		if (!ReadCS(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!ReadCS(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1), GL_RED_INTEGER, GL_INT))
			return ERROR;
		if (!ReadCS(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_SHORT))
			return ERROR;
		if (!ReadCS(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4), GL_RGBA_INTEGER, GL_BYTE))
			return ERROR;

		if (!ReadCS(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!ReadCS(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1), GL_RED_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!ReadCS(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT))
			return ERROR;
		if (!ReadCS(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3), GL_RGBA_INTEGER, GL_UNSIGNED_BYTE))
			return ERROR;

		if (!ReadCS(GL_RGBA8, vec4(1.0f), vec4(1.0f), GL_RGBA, GL_UNSIGNED_BYTE))
			return ERROR;

		if (!ReadCS(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f), GL_RGBA, GL_BYTE))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool ReadCS(GLenum internalformat, const T& value, const T& expected_value, GLenum format, GLenum type)
	{
		GLuint			 program = CreateComputeProgram(GenCS(internalformat, expected_value));
		const int		 kSize   = 4;
		ShortByteData<T> d(kSize, value, internalformat, format);
		GLuint			 texture;
		glGenTextures(1, &texture);

		GLuint unit = 1;
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		if (Shorts(internalformat))
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datas[0]);
		else if (Bytes(internalformat))
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datab[0]);
		else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(program);
		glBindImageTexture(unit, texture, 0, GL_FALSE, 0, GL_READ_ONLY, internalformat);

		std::vector<T> out_data(kSize * kSize);
		GLuint		   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		T* map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str()
					<< ". Format is: " << FormatEnumToString(internalformat).c_str() << ". Unit is: " << unit << "."
					<< tcu::TestLog::EndMessage;
				glUseProgram(0);
				glDeleteProgram(program);
				glDeleteTextures(1, &texture);
				glDeleteBuffers(1, &m_buffer);
				return false;
			}
		}
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(1, &texture);
		glDeleteBuffers(1, &m_buffer);
		return true;
	}

	virtual long Cleanup()
	{
		return NO_ERROR;
	}
};

class BasicAllFormatsLoadStoreComputeStage : public ShaderImageLoadStoreBase
{
	virtual long Run()
	{

		if (!Read(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(2.0f, 4.0f, 6.0f, 8.0f), GL_RGBA, GL_FLOAT))
			return ERROR;
		if (!Read(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(2.0f, 0.0f, 0.0f, 1.0f), GL_RED, GL_FLOAT))
			return ERROR;
		if (!Read(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(2.0f, 4.0f, 6.0f, 8.0f), GL_RGBA, GL_FLOAT))
			return ERROR;

		if (!Read(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(2, -4, 6, -8), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(GL_R32I, ivec4(1, -2, 3, -4), ivec4(2, 0, 0, 1), GL_RED_INTEGER, GL_INT))
			return ERROR;
		if (!Read(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(2, -4, 6, -8), GL_RGBA_INTEGER, GL_SHORT))
			return ERROR;
		if (!Read(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(2, -4, 6, -8), GL_RGBA_INTEGER, GL_BYTE))
			return ERROR;

		if (!Read(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 2, 4, 6), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(14, 0, 0, 1), GL_RED_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 2, 4, 6), GL_RGBA_INTEGER, GL_UNSIGNED_SHORT))
			return ERROR;
		if (!Read(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 2, 4, 6), GL_RGBA_INTEGER, GL_UNSIGNED_BYTE))
			return ERROR;

		if (!Read(GL_RGBA8, vec4(0.5f), vec4(1.0f), GL_RGBA, GL_UNSIGNED_BYTE))
			return ERROR;
		if (!Read(GL_RGBA8_SNORM, vec4(0.5f, 0.0f, 0.5f, -0.5f), vec4(1.0f, 0.0f, 1.0f, -1.0f), GL_RGBA, GL_BYTE))
			return ERROR;

		return NO_ERROR;
	}

	template <typename T>
	bool Read(GLenum internalformat, const T& value, const T& expected_value, GLenum format, GLenum type)
	{
		GLuint program = CreateComputeProgram(GenCS(internalformat, expected_value));

		const int		 kSize = 8;
		ShortByteData<T> d(kSize, value, internalformat, format);
		GLuint			 texture[2];
		glGenTextures(2, texture);

		/* read texture */
		{
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
			if (Shorts(internalformat))
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datas[0]);
			else if (Bytes(internalformat))
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.datab[0]);
			else
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &d.data[0]);
		}
		/* write texture */
		{
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(program);

		glBindImageTexture(2, texture[0], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);
		glBindImageTexture(3, texture[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);

		glDispatchCompute(1, 1, 1);

		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		GLuint		   c_program = CreateComputeProgram(GenC(expected_value));
		std::vector<T> out_data(kSize * kSize);
		GLuint		   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		T* map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(map_data[i], expected_value, internalformat))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[i]).c_str()
					<< ". Value should be: " << ToString(expected_value).c_str()
					<< ". Format is: " << FormatEnumToString(internalformat).c_str() << "." << tcu::TestLog::EndMessage;
				glDeleteTextures(2, texture);
				glUseProgram(0);
				glDeleteProgram(program);
				glDeleteProgram(c_program);
				glDeleteBuffers(1, &m_buffer);
				return false;
			}
		}
		glDeleteTextures(2, texture);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(c_program);
		glDeleteBuffers(1, &m_buffer);

		return true;
	}

	template <typename T>
	std::string GenCS(GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 8" NL "layout(local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 2) readonly uniform " << TypePrefix<T>()
		   << "image2D g_image_read;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 3) writeonly uniform " << TypePrefix<T>()
		   << "image2D g_image_write;" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL "  "
		   << TypePrefix<T>() << "vec4 v = imageLoad(g_image_read, coord);" NL
								 "  imageStore(g_image_write, coord, v+v);" NL "  //imageStore(g_image_write, coord, "
		   << TypePrefix<T>() << "vec4" << expected_value << ");" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenC(const T& value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 8" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "uniform "
		   << TypePrefix<T>() << "sampler2D g_sampler;" NL "layout(std430) buffer OutputBuffer {" NL "  "
		   << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL
			  "  data[gl_LocalInvocationIndex] = texelFetch(g_sampler, ivec2(gl_LocalInvocationID), 0);" NL
			  "  //data[gl_LocalInvocationIndex] = "
		   << TypePrefix<T>() << "vec4" << value << ";" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.1 BasicAllTargetsStore
//-----------------------------------------------------------------------------
class BasicAllTargetsStoreFS : public ShaderImageLoadStoreBase
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
		if (!IsVSFSAvailable(0, 4))
			return NOT_SUPPORTED;
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Write(T2D, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(T2D, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T2D, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		glActiveTexture(GL_TEXTURE0);
		return NO_ERROR;
	}

	template <typename T>
	bool Write(int target, GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, GenFS(target, internalformat, write_value).c_str());
		GLuint		 textures[8];
		glGenTextures(8, textures);

		const int	  kSize = 11;
		std::vector<T> data(kSize * kSize * 2);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, internalformat, kSize, kSize, 2);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat, kSize, kSize, 2);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, textures[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat); // 2D
		glBindImageTexture(1, textures[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 3D
		glBindImageTexture(2, textures[4], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // Cube
		glBindImageTexture(3, textures[7], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 2DArray

		glUseProgram(program);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		GLuint		   c_program = CreateComputeProgram(GenC(write_value));
		std::vector<T> out_data2D(kSize * kSize * 6);
		std::vector<T> out_data3D(kSize * kSize * 6);
		std::vector<T> out_dataCube(kSize * kSize * 6);
		std::vector<T> out_data2DArray(kSize * kSize * 6);
		GLuint		   m_buffer[4];
		glGenBuffers(4, m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);

		glUseProgram(c_program);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_2d"), 1);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_3d"), 2);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_cube"), 3);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_2darray"), 4);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[target]);
		T*  map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 6 * kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		int layers   = 2;
		if (target == T2D)
			layers = 1;
		if (target == TCM)
			layers = 6;
		status	 = CompareValues(map_data, kSize, expected_value, internalformat, layers);
		if (!status)
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << target << " target, " << FormatEnumToString(internalformat).c_str()
				<< " format failed." << tcu::TestLog::EndMessage;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glDeleteTextures(8, textures);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(c_program);
		glDeleteBuffers(4, m_buffer);

		return status;
	}

	template <typename T>
	std::string GenFS(int target, GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		switch (target)
		{
		case T2D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 0) writeonly uniform "
			   << TypePrefix<T>() << "image2D g_image_2d;";
			break;
		case T3D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) writeonly uniform "
			   << TypePrefix<T>() << "image3D g_image_3d;";
			break;
		case TCM:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) writeonly uniform "
			   << TypePrefix<T>() << "imageCube g_image_cube;";
			break;
		case T2DA:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) writeonly uniform "
			   << TypePrefix<T>() << "image2DArray g_image_2darray;";
			break;
		}
		os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);";

		switch (target)
		{
		case T2D:
			os << NL "  imageStore(g_image_2d, coord, " << TypePrefix<T>() << "vec4" << write_value << ");";
			break;
		case T3D:
			os << NL "  imageStore(g_image_3d, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_3d, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		case TCM:
			os << NL "  imageStore(g_image_cube, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 2), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 3), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 4), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 5), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		case T2DA:
			os << NL "  imageStore(g_image_2darray, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_2darray, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		}
		os << NL "  discard;" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenC(const T& write_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "uniform "
		   << TypePrefix<T>() << "sampler2D g_sampler_2d;" NL "uniform " << TypePrefix<T>()
		   << "sampler3D g_sampler_3d;" NL "uniform " << TypePrefix<T>() << "samplerCube g_sampler_cube;" NL "uniform "
		   << TypePrefix<T>()
		   << "sampler2DArray g_sampler_2darray;" NL "layout(std430, binding = 1) buffer OutputBuffer2D {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE];" NL "} g_buff_2d;" NL
								 "layout(std430, binding = 0) buffer OutputBuffer3D {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_3d;" NL
								 "layout(std430, binding = 3) buffer OutputBufferCube {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*6];" NL "} g_buff_cube;" NL
								 "layout(std430, binding = 2) buffer OutputBuffer2DArray {" NL "  "
		   << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_2darray;" NL "void main() {" NL
			  "  int cubemap_i = 2 * int(gl_LocalInvocationID.x) - KSIZE + 1;" NL
			  "  int cubemap_j = 2 * int(gl_LocalInvocationID.y) - KSIZE + 1;" NL
			  "  uint layer = uint(KSIZE * KSIZE);" NL
			  "  g_buff_2d.data[gl_LocalInvocationIndex] = texelFetch(g_sampler_2d, ivec2(gl_LocalInvocationID), 0);" NL
			  "  g_buff_3d.data[gl_LocalInvocationIndex] = texelFetch(g_sampler_3d, ivec3(gl_LocalInvocationID.xy, 0), "
			  "0);" NL "  g_buff_3d.data[gl_LocalInvocationIndex + layer] = texelFetch(g_sampler_3d, "
			  "ivec3(gl_LocalInvocationID.xy, 1), 0);" NL "  g_buff_2darray.data[gl_LocalInvocationIndex] = "
			  "texelFetch(g_sampler_2darray, "
			  "ivec3(gl_LocalInvocationID.xy, 0), 0);" NL
			  "  g_buff_2darray.data[gl_LocalInvocationIndex + layer] = texelFetch(g_sampler_2darray, "
			  "ivec3(gl_LocalInvocationID.xy, 1), 0);" NL "  g_buff_cube.data[gl_LocalInvocationIndex] = "
			  "texture(g_sampler_cube, vec3(KSIZE,cubemap_i,cubemap_j));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + layer] = texture(g_sampler_cube, "
			  "vec3(KSIZE,cubemap_i,cubemap_j));" NL "  g_buff_cube.data[gl_LocalInvocationIndex + 2u * layer] = "
			  "texture(g_sampler_cube, vec3(cubemap_i,KSIZE,cubemap_j));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + 3u * layer] = texture(g_sampler_cube, "
			  "vec3(cubemap_i,KSIZE,cubemap_j));" NL "  g_buff_cube.data[gl_LocalInvocationIndex + 4u * layer] = "
			  "texture(g_sampler_cube, vec3(cubemap_i,cubemap_j,KSIZE));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + 5u * layer] = texture(g_sampler_cube, "
			  "vec3(cubemap_i,cubemap_j,KSIZE));" NL "  //g_buff_2d.data[gl_LocalInvocationIndex] = "
		   << write_value << ";" NL "}";
		return os.str();
	}
};

class BasicAllTargetsStoreCS : public ShaderImageLoadStoreBase
{
	virtual long Setup()
	{
		return NO_ERROR;
	}

	virtual long Run()
	{

		if (!Write(T2D, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(T2D, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T2D, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(T3D, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(TCM, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32F, vec4(-1.0f, 2.0f, 3.0f, -4.0f), vec4(-1.0f, 2.0f, 3.0f, -4.0f)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(T2DA, GL_RGBA32UI, uvec4(1, 2, 3, 4), uvec4(1, 2, 3, 4)))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glActiveTexture(GL_TEXTURE0);
		return NO_ERROR;
	}

	template <typename T>
	bool Write(int target, GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const GLuint program = CreateComputeProgram(GenCS(target, internalformat, write_value));
		GLuint		 textures[8];
		glGenTextures(8, textures);

		const int	  kSize = 11;
		std::vector<T> data(kSize * kSize * 2);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, internalformat, kSize, kSize, 2);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, internalformat, kSize, kSize);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat, kSize, kSize, 2);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(0, textures[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat); // 2D
		glBindImageTexture(1, textures[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 3D
		glBindImageTexture(2, textures[4], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // Cube
		glBindImageTexture(3, textures[7], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 2DArray

		glUseProgram(program);
		glDispatchCompute(1, 1, 1);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		GLuint		   c_program = CreateComputeProgram(GenC(write_value));
		std::vector<T> out_data2D(kSize * kSize * 6);
		std::vector<T> out_data3D(kSize * kSize * 6);
		std::vector<T> out_dataCube(kSize * kSize * 6);
		std::vector<T> out_data2DArray(kSize * kSize * 6);
		GLuint		   m_buffer[4];
		glGenBuffers(4, m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);

		glUseProgram(c_program);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_2d"), 1);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_3d"), 2);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_cube"), 3);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler_2darray"), 4);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[target]);
		T*  map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 6 * kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		int layers   = 2;
		if (target == T2D)
			layers = 1;
		if (target == TCM)
			layers = 6;
		status	 = CompareValues(map_data, kSize, expected_value, internalformat, layers);
		if (!status)
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << target << " target, " << FormatEnumToString(internalformat).c_str()
				<< " format failed." << tcu::TestLog::EndMessage;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glDeleteTextures(8, textures);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteProgram(c_program);
		glDeleteBuffers(4, m_buffer);

		return status;
	}

	template <typename T>
	std::string GenCS(int target, GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;";
		switch (target)
		{
		case T2D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 0) writeonly uniform "
			   << TypePrefix<T>() << "image2D g_image_2d;";
			break;
		case T3D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) writeonly uniform "
			   << TypePrefix<T>() << "image3D g_image_3d;";
			break;
		case TCM:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) writeonly uniform "
			   << TypePrefix<T>() << "imageCube g_image_cube;";
			break;
		case T2DA:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) writeonly uniform "
			   << TypePrefix<T>() << "image2DArray g_image_2darray;";
			break;
		}
		os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);";
		switch (target)
		{
		case T2D:
			os << NL "  imageStore(g_image_2d, coord, " << TypePrefix<T>() << "vec4" << write_value << ");";
			break;
		case T3D:
			os << NL "  imageStore(g_image_3d, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_3d, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		case TCM:
			os << NL "  imageStore(g_image_cube, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 2), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 3), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 4), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_cube, ivec3(coord, 5), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		case T2DA:
			os << NL "  imageStore(g_image_2darray, ivec3(coord, 0), " << TypePrefix<T>() << "vec4" << write_value
			   << ");" NL "  imageStore(g_image_2darray, ivec3(coord, 1), " << TypePrefix<T>() << "vec4" << write_value
			   << ");";
			break;
		}
		os << NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenC(const T& write_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL "uniform "
		   << TypePrefix<T>() << "sampler2D g_sampler_2d;" NL "uniform " << TypePrefix<T>()
		   << "sampler3D g_sampler_3d;" NL "uniform " << TypePrefix<T>() << "samplerCube g_sampler_cube;" NL "uniform "
		   << TypePrefix<T>()
		   << "sampler2DArray g_sampler_2darray;" NL "layout(std430, binding = 0) buffer OutputBuffer2D {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE];" NL "} g_buff_2d;" NL
								 "layout(std430, binding = 1) buffer OutputBuffer3D {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_3d;" NL
								 "layout(std430, binding = 2) buffer OutputBufferCube {" NL "  "
		   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*6];" NL "} g_buff_cube;" NL
								 "layout(std430, binding = 3) buffer OutputBuffer2DArray {" NL "  "
		   << TypePrefix<T>()
		   << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_2darray;" NL "void main() {" NL
			  "  int cubemap_i = 2 * int(gl_LocalInvocationID.x) - KSIZE + 1;" NL
			  "  int cubemap_j = 2 * int(gl_LocalInvocationID.y) - KSIZE + 1;" NL
			  "  uint layer = uint(KSIZE * KSIZE);" NL
			  "  g_buff_2d.data[gl_LocalInvocationIndex] = texelFetch(g_sampler_2d, ivec2(gl_LocalInvocationID), 0);" NL
			  "  g_buff_3d.data[gl_LocalInvocationIndex] = texelFetch(g_sampler_3d, ivec3(gl_LocalInvocationID.xy, 0), "
			  "0);" NL "  g_buff_3d.data[gl_LocalInvocationIndex + layer] = texelFetch(g_sampler_3d, "
			  "ivec3(gl_LocalInvocationID.xy, 1), 0);" NL "  g_buff_2darray.data[gl_LocalInvocationIndex] = "
			  "texelFetch(g_sampler_2darray, "
			  "ivec3(gl_LocalInvocationID.xy, 0), 0);" NL
			  "  g_buff_2darray.data[gl_LocalInvocationIndex + layer] = texelFetch(g_sampler_2darray, "
			  "ivec3(gl_LocalInvocationID.xy, 1), 0);" NL "  g_buff_cube.data[gl_LocalInvocationIndex] = "
			  "texture(g_sampler_cube, vec3(KSIZE,cubemap_i,cubemap_j));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + layer] = texture(g_sampler_cube, "
			  "vec3(KSIZE,cubemap_i,cubemap_j));" NL "  g_buff_cube.data[gl_LocalInvocationIndex + 2u * layer] = "
			  "texture(g_sampler_cube, vec3(cubemap_i,KSIZE,cubemap_j));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + 3u * layer] = texture(g_sampler_cube, "
			  "vec3(cubemap_i,KSIZE,cubemap_j));" NL "  g_buff_cube.data[gl_LocalInvocationIndex + 4u * layer] = "
			  "texture(g_sampler_cube, vec3(cubemap_i,cubemap_j,KSIZE));" NL
			  "  g_buff_cube.data[gl_LocalInvocationIndex + 5u * layer] = texture(g_sampler_cube, "
			  "vec3(cubemap_i,cubemap_j,KSIZE));" NL "  //g_buff_2d.data[gl_LocalInvocationIndex] = "
		   << write_value << ";" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.2.1 BasicAllTargetsLoad
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadFS : public ShaderImageLoadStoreBase
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
		if (!IsVSFSAvailable(0, 4) || !IsSSBInVSFSAvailable(4))
			return NOT_SUPPORTED;
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Read(T2D, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T2D, GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T2D, GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32I, ivec4(-1, 10, -200, 3000), ivec4(-1, 10, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32UI, uvec4(1, 10, 200, 3000), uvec4(1, 10, 200, 3000), GL_RGBA_INTEGER,
				  GL_UNSIGNED_INT))
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
	bool Read(int target, GLenum internalformat, const T& value, const T& expected_value, GLenum format, GLenum type)
	{
		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, GenFS(target, internalformat, expected_value).c_str());
		GLuint		 textures[8];
		glGenTextures(8, textures);

		const int	  kSize = 11;
		std::vector<T> data(kSize * kSize * 2, value);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, internalformat, kSize, kSize, 2);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kSize, kSize, 2, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat, kSize, kSize, 2);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kSize, kSize, 2, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(2, textures[1], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat); // 2D
		glBindImageTexture(0, textures[2], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 3D
		glBindImageTexture(3, textures[4], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // Cube
		glBindImageTexture(1, textures[7], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 2DArray

		std::vector<T> out_data2D(kSize * kSize * 6);
		std::vector<T> out_data3D(kSize * kSize * 6);
		std::vector<T> out_dataCube(kSize * kSize * 6);
		std::vector<T> out_data2DArray(kSize * kSize * 6);
		GLuint		   m_buffer[4];
		glGenBuffers(4, m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);

		glUseProgram(program);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[target]);
		T*  map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 6 * kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		int layers   = 2;
		if (target == T2D)
			layers = 1;
		if (target == TCM)
			layers = 6;
		status	 = CompareValues(map_data, kSize, expected_value, internalformat, layers);
		if (!status)
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << target << " target, " << FormatEnumToString(internalformat).c_str()
				<< " format failed." << tcu::TestLog::EndMessage;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(8, textures);
		glDeleteBuffers(4, m_buffer);

		return status;
	}

	template <typename T>
	std::string GenFS(int target, GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11";
		switch (target)
		{
		case T2D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) readonly uniform "
			   << TypePrefix<T>()
			   << "image2D g_image_2d;" NL "layout(std430, binding = 1) buffer OutputBuffer2D {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE];" NL "} g_buff_2d;";
			break;
		case T3D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 0) readonly uniform "
			   << TypePrefix<T>()
			   << "image3D g_image_3d;" NL "layout(std430, binding = 0) buffer OutputBuffer3D {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_3d;";
			break;
		case TCM:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) readonly uniform "
			   << TypePrefix<T>()
			   << "imageCube g_image_cube;" NL "layout(std430, binding = 3) buffer OutputBufferCube {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*6];" NL "} g_buff_cube;";
			break;
		case T2DA:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) readonly uniform "
			   << TypePrefix<T>()
			   << "image2DArray g_image_2darray;" NL "layout(std430, binding = 2) buffer OutputBuffer2DArray {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_2darray;";
			break;
		}
		os << NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
				 "  int coordIndex = coord.x + KSIZE * coord.y;" NL "  int layer = int(KSIZE * KSIZE);" NL "  "
		   << TypePrefix<T>() << "vec4 v;";

		switch (target)
		{
		case T2D:
			os << NL "  v = imageLoad(g_image_2d, coord);" NL "  g_buff_2d.data[coordIndex] = v;";
			break;
		case T3D:
			os << NL "  v = imageLoad(g_image_3d, ivec3(coord.xy, 0));" NL "  g_buff_3d.data[coordIndex] = v;" NL
					 "  v = imageLoad(g_image_3d, ivec3(coord.xy, 1));" NL "  g_buff_3d.data[coordIndex + layer] = v;";
			break;
		case TCM:
			os << NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 0));" NL "  g_buff_cube.data[coordIndex] = v;" NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 1));" NL "  g_buff_cube.data[coordIndex + layer] = v;" NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 2));" NL
				"  g_buff_cube.data[coordIndex + 2 * layer] = v;" NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 3));" NL
				"  g_buff_cube.data[coordIndex + 3 * layer] = v;" NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 4));" NL
				"  g_buff_cube.data[coordIndex + 4 * layer] = v;" NL
				"  v = imageLoad(g_image_cube, ivec3(coord, 5));" NL "  g_buff_cube.data[coordIndex + 5 * layer] = v;";
			break;
		case T2DA:
			os << NL "  v = imageLoad(g_image_2darray, ivec3(coord, 0));" NL "  g_buff_2darray.data[coordIndex] = v;" NL
					 "  v = imageLoad(g_image_2darray, ivec3(coord, 1));" NL
					 "  g_buff_2darray.data[coordIndex + layer] = v;";
			break;
		}
		os << NL "  //g_buff_2d.data[coordIndex] = " << expected_value << ";" NL "}";
		return os.str();
	}
};

class BasicAllTargetsLoadCS : public ShaderImageLoadStoreBase
{
	virtual long Setup()
	{
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!Read(T2D, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T2D, GL_RGBA32I, ivec4(-0x7fffffff, 0x7fffffff, -200, 3000),
				  ivec4(-0x7fffffff, 0x7fffffff, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T2D, GL_RGBA32UI, uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000),
				  uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32I, ivec4(-0x7fffffff, 0x7fffffff, -200, 3000),
				  ivec4(-0x7fffffff, 0x7fffffff, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T3D, GL_RGBA32UI, uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000),
				  uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32I, ivec4(-0x7fffffff, 0x7fffffff, -200, 3000),
				  ivec4(-0x7fffffff, 0x7fffffff, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(TCM, GL_RGBA32UI, uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000),
				  uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32F, vec4(-1.0f, 10.0f, -200.0f, 3000.0f), vec4(-1.0f, 10.0f, -200.0f, 3000.0f), GL_RGBA,
				  GL_FLOAT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32I, ivec4(-0x7fffffff, 0x7fffffff, -200, 3000),
				  ivec4(-0x7fffffff, 0x7fffffff, -200, 3000), GL_RGBA_INTEGER, GL_INT))
			return ERROR;
		if (!Read(T2DA, GL_RGBA32UI, uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000),
				  uvec4(0xffffffffu, 0x7fffffff, 0x80000000, 3000), GL_RGBA_INTEGER, GL_UNSIGNED_INT))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		return NO_ERROR;
	}

	template <typename T>
	bool Read(int target, GLenum internalformat, const T& value, const T& expected_value, GLenum format, GLenum type)
	{
		const GLuint program = CreateComputeProgram(GenCS(target, internalformat, expected_value));
		GLuint		 textures[8];
		glGenTextures(8, textures);

		const int	  kSize = 11;
		std::vector<T> data(kSize * kSize * 2, value);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, internalformat, kSize, kSize, 2);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kSize, kSize, 2, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, kSize, kSize, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat, kSize, kSize, 2);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kSize, kSize, 2, format, type, &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(2, textures[1], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat); // 2D
		glBindImageTexture(0, textures[2], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 3D
		glBindImageTexture(3, textures[4], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // Cube
		glBindImageTexture(1, textures[7], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 2DArray

		std::vector<T> out_data2D(kSize * kSize * 6);
		std::vector<T> out_data3D(kSize * kSize * 6);
		std::vector<T> out_dataCube(kSize * kSize * 6);
		std::vector<T> out_data2DArray(kSize * kSize * 6);
		GLuint		   m_buffer[4];
		glGenBuffers(4, m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer[0]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_buffer[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_buffer[2]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_buffer[3]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * kSize * kSize * 4 * 4, &out_dataCube[0], GL_STATIC_DRAW);

		glUseProgram(program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer[target]);
		T*  map_data = (T*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 6 * kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		int layers   = 2;
		if (target == T2D)
			layers = 1;
		if (target == TCM)
			layers = 6;
		status	 = CompareValues(map_data, kSize, expected_value, internalformat, layers);
		if (!status)
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << target << " target, " << FormatEnumToString(internalformat).c_str()
				<< " format failed." << tcu::TestLog::EndMessage;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(8, textures);
		glDeleteBuffers(4, m_buffer);

		return status;
	}

	template <typename T>
	std::string GenCS(int target, GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;";
		switch (target)
		{
		case T2D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) readonly uniform "
			   << TypePrefix<T>()
			   << "image2D g_image_2d;" NL "layout(std430, binding = 0) buffer OutputBuffer2D {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE];" NL "} g_buff_2d;";
			break;
		case T3D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 0) readonly uniform "
			   << TypePrefix<T>()
			   << "image3D g_image_3d;" NL "layout(std430, binding = 1) buffer OutputBuffer3D {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_3d;";
			break;
		case TCM:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) readonly uniform "
			   << TypePrefix<T>()
			   << "imageCube g_image_cube;" NL "layout(std430, binding = 2) buffer OutputBufferCube {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*6];" NL "} g_buff_cube;";
			break;
		case T2DA:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) readonly uniform "
			   << TypePrefix<T>()
			   << "image2DArray g_image_2darray;" NL "layout(std430, binding = 3) buffer OutputBuffer2DArray {" NL "  "
			   << TypePrefix<T>() << "vec4 data[KSIZE*KSIZE*2];" NL "} g_buff_2darray;";
			break;
		}
		os << NL "void main() {" NL "  ivec3 coord = ivec3(gl_LocalInvocationID.xy, 0);" NL
				 "  uint layer = uint(KSIZE * KSIZE);" NL "  "
		   << TypePrefix<T>() << "vec4 v;";
		switch (target)
		{
		case T2D:
			os << NL "  v = imageLoad(g_image_2d, coord.xy);" NL "  g_buff_2d.data[gl_LocalInvocationIndex] = v;";
			break;
		case T3D:
			os << NL "  v = imageLoad(g_image_3d, coord);" NL "  g_buff_3d.data[gl_LocalInvocationIndex] = v;" NL
					 "  v = imageLoad(g_image_3d, ivec3(coord.xy, 1));" NL
					 "  g_buff_3d.data[gl_LocalInvocationIndex + layer] = v;";
			break;
		case TCM:
			os << NL "  v = imageLoad(g_image_cube, coord);" NL "  g_buff_cube.data[gl_LocalInvocationIndex] = v;" NL
					 "  v = imageLoad(g_image_cube, ivec3(coord.xy, 1));" NL
					 "  g_buff_cube.data[gl_LocalInvocationIndex + layer] = v;" NL
					 "  v = imageLoad(g_image_cube, ivec3(coord.xy, 2));" NL
					 "  g_buff_cube.data[gl_LocalInvocationIndex + 2u * layer] = v;" NL
					 "  v = imageLoad(g_image_cube, ivec3(coord.xy, 3));" NL
					 "  g_buff_cube.data[gl_LocalInvocationIndex + 3u * layer] = v;" NL
					 "  v = imageLoad(g_image_cube, ivec3(coord.xy, 4));" NL
					 "  g_buff_cube.data[gl_LocalInvocationIndex + 4u * layer] = v;" NL
					 "  v = imageLoad(g_image_cube, ivec3(coord.xy, 5));" NL
					 "  g_buff_cube.data[gl_LocalInvocationIndex + 5u * layer] = v;";
			break;
		case T2DA:
			os << NL "  v = imageLoad(g_image_2darray, coord);" NL
					 "  g_buff_2darray.data[gl_LocalInvocationIndex] = v;" NL
					 "  v = imageLoad(g_image_2darray, ivec3(coord.xy, 1));" NL
					 "  g_buff_2darray.data[gl_LocalInvocationIndex + layer] = v;";
			break;
		}
		os << NL "  //g_buff_2d.data[gl_LocalInvocationIndex] = " << expected_value << ";" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// 1.3.3 BasicAllTargetsAtomic
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicFS : public ShaderImageLoadStoreBase
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
		if (!IsImageAtomicSupported())
			return NOT_SUPPORTED;
		if (!IsVSFSAvailable(0, 4))
			return NOT_SUPPORTED;
		if (!AreOutputsAvailable(5))
			return NOT_SUPPORTED;
		if (!IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		if (!Atomic<GLint>(GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(GL_R32UI))
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
	bool Atomic(GLenum internalformat)
	{
		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const GLuint program = BuildProgram(src_vs, GenFS<T>(internalformat).c_str(), false, true);
		GLuint		 textures[8];
		GLuint		 buffer;
		glGenTextures(8, textures);
		glGenBuffers(1, &buffer);

		const int	  kSize = 11;
		std::vector<T> data(kSize * kSize * 3);

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_3D, textures[2]);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_3D, 1, internalformat, kSize, kSize, 3);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kSize, kSize, 3, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_3D, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, textures[4]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, internalformat, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, kSize, kSize, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textures[7]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalformat, kSize, kSize, 3);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kSize, kSize, 3, Format<T>(), Type<T>(), &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glBindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat); // 2D
		glBindImageTexture(2, textures[2], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // 3D
		glBindImageTexture(0, textures[4], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // Cube
		glBindImageTexture(3, textures[7], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // 2DArray

		std::vector<ivec4> o_data(kSize * kSize);
		GLuint			   m_buffer;
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &o_data[0], GL_STATIC_DRAW);

		glUseProgram(program);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;

		ivec4* out_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(out_data[i], ivec4(10, 10, 10, 10), 0))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i
					<< "] Atomic operation check failed. (operation/target coded: " << ToString(out_data[i]).c_str()
					<< ")" << tcu::TestLog::EndMessage;
			}
		}

		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(8, textures);
		glDeleteBuffers(1, &buffer);
		glDeleteBuffers(1, &m_buffer);

		return status;
	}

	template <typename T>
	std::string GenFS(GLenum internalformat)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 11" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 1) coherent uniform " << TypePrefix<T>() << "image2D g_image_2d;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 2) coherent uniform " << TypePrefix<T>()
		   << "image3D g_image_3d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 0) coherent uniform " << TypePrefix<T>() << "imageCube g_image_cube;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 3) coherent uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "layout(std430) buffer out_data {" NL
			  "  ivec4 o_color[KSIZE*KSIZE];" NL "};" NL
		   << TypePrefix<T>() << "vec2 t(int i) {" NL "  return " << TypePrefix<T>()
		   << "vec2(i);" NL "}" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			  "  int coordIndex = coord.x + KSIZE * coord.y;" NL "  o_color[coordIndex] = ivec4(coordIndex);" NL
			  "  if (imageAtomicAdd(g_image_2d, coord, t(2).x) != t(0).x) o_color[coordIndex].x = 1;" NL
			  "  else if (imageAtomicMin(g_image_2d, coord, t(3).x) != t(2).x) o_color[coordIndex].x = 2;" NL
			  "  else if (imageAtomicMax(g_image_2d, coord, t(4).x) != t(2).x) o_color[coordIndex].x = 3;" NL
			  "  else if (imageAtomicAnd(g_image_2d, coord, t(0).x) != t(4).x) o_color[coordIndex].x = 4;" NL
			  "  else if (imageAtomicOr(g_image_2d, coord, t(7).x) != t(0).x) o_color[coordIndex].x = 5;" NL
			  "  else if (imageAtomicXor(g_image_2d, coord, t(4).x) != t(7).x) o_color[coordIndex].x = 6;" NL
			  "  else if (imageAtomicExchange(g_image_2d, coord, t(1).x) != t(3).x) o_color[coordIndex].x = 7;" NL
			  "  else if (imageAtomicCompSwap(g_image_2d, coord, t(1).x, t(6).x) != t(1).x) o_color[coordIndex].x = "
			  "8;" NL "  else o_color[coordIndex].x = 10;" NL
			  "  if (imageAtomicAdd(g_image_3d, ivec3(coord, 2), t(2).x) != t(0).x) o_color[coordIndex].y = 1;" NL
			  "  else if (imageAtomicMin(g_image_3d, ivec3(coord, 2), t(3).x) != t(2).x) o_color[coordIndex].y = 2;" NL
			  "  else if (imageAtomicMax(g_image_3d, ivec3(coord, 2), t(4).x) != t(2).x) o_color[coordIndex].y = 3;" NL
			  "  else if (imageAtomicAnd(g_image_3d, ivec3(coord, 2), t(0).x) != t(4).x) o_color[coordIndex].y = 4;" NL
			  "  else if (imageAtomicOr(g_image_3d, ivec3(coord, 2), t(7).x) != t(0).x) o_color[coordIndex].y = 5;" NL
			  "  else if (imageAtomicXor(g_image_3d, ivec3(coord, 2), t(4).x) != t(7).x) o_color[coordIndex].y = 6;" NL
			  "  else if (imageAtomicExchange(g_image_3d, ivec3(coord, 2), t(1).x) != t(3).x) o_color[coordIndex].y = "
			  "7;" NL "  else if (imageAtomicCompSwap(g_image_3d, ivec3(coord, 2), t(1).x, t(6).x) != t(1).x) "
			  "o_color[coordIndex].y = 8;" NL "  else o_color[coordIndex].y = 10;" NL
			  "  if (imageAtomicAdd(g_image_cube, ivec3(coord, 3), t(2).x) != t(0).x) o_color[coordIndex].z = 1;" NL
			  "  else if (imageAtomicMin(g_image_cube, ivec3(coord, 3), t(3).x) != t(2).x) o_color[coordIndex].z = "
			  "2;" NL "  else if (imageAtomicMax(g_image_cube, ivec3(coord, 3), t(4).x) != t(2).x) "
			  "o_color[coordIndex].z = 3;" NL "  else if (imageAtomicAnd(g_image_cube, ivec3(coord, 3), "
			  "t(0).x) != t(4).x) o_color[coordIndex].z = 4;" NL
			  "  else if (imageAtomicOr(g_image_cube, ivec3(coord, 3), t(7).x) != t(0).x) o_color[coordIndex].z = 5;" NL
			  "  else if (imageAtomicXor(g_image_cube, ivec3(coord, 3), t(4).x) != t(7).x) o_color[coordIndex].z = "
			  "6;" NL "  else if (imageAtomicExchange(g_image_cube, ivec3(coord, 3), t(1).x) != t(3).x) "
			  "o_color[coordIndex].z = 7;" NL "  else if (imageAtomicCompSwap(g_image_cube, ivec3(coord, 3), "
			  "t(1).x, t(6).x) != t(1).x) o_color[coordIndex].z = 8;" NL "  else o_color[coordIndex].z = 10;" NL
			  "  if (imageAtomicAdd(g_image_2darray, ivec3(coord, 2), t(2).x) != t(0).x) o_color[coordIndex].w = 1;" NL
			  "  else if (imageAtomicMin(g_image_2darray, ivec3(coord, 2), t(3).x) != t(2).x) o_color[coordIndex].w = "
			  "2;" NL "  else if (imageAtomicMax(g_image_2darray, ivec3(coord, 2), t(4).x) != t(2).x) "
			  "o_color[coordIndex].w = 3;" NL "  else if (imageAtomicAnd(g_image_2darray, ivec3(coord, 2), "
			  "t(0).x) != t(4).x) o_color[coordIndex].w = 4;" NL
			  "  else if (imageAtomicOr(g_image_2darray, ivec3(coord, 2), t(7).x) != t(0).x) o_color[coordIndex].w = "
			  "5;" NL "  else if (imageAtomicXor(g_image_2darray, ivec3(coord, 2), t(4).x) != t(7).x) "
			  "o_color[coordIndex].w = 6;" NL "  else if (imageAtomicExchange(g_image_2darray, ivec3(coord, "
			  "2), t(1).x) != t(3).x) o_color[coordIndex].w = 7;" NL
			  "  else if (imageAtomicCompSwap(g_image_2darray, ivec3(coord, 2), t(1).x, t(6).x) != t(1).x) "
			  "o_color[coordIndex].w = 8;" NL "  else o_color[coordIndex].w = 10;" NL "  discard;" NL "}";
		return os.str();
	}
};
//-----------------------------------------------------------------------------
// LoadStoreMachine
//-----------------------------------------------------------------------------
class LoadStoreMachine : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_buffer;
	int	m_stage;

	virtual long Setup()
	{
		glEnable(GL_RASTERIZER_DISCARD);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	template <typename T>
	bool Write(GLenum internalformat, const T& write_value, const T& expected_value)
	{
		const GLenum targets[]	 = { GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY };
		const int	kTargets	  = sizeof(targets) / sizeof(targets[0]);
		const int	kSize		   = 100;
		GLuint		 program_store = 0;
		GLuint		 program_load  = 0;
		if (m_stage == 0)
		{ // VS
			const char* src_fs = NL "void main() {" NL "  discard;" NL "}";
			program_store	  = BuildProgram(GenStoreShader(m_stage, internalformat, write_value).c_str(), src_fs);
			program_load	   = BuildProgram(GenLoadShader(m_stage, internalformat, expected_value).c_str(), src_fs);
		}
		else if (m_stage == 4)
		{ // CS
			program_store = CreateComputeProgram(GenStoreShader(m_stage, internalformat, write_value));
			program_load  = CreateComputeProgram(GenLoadShader(m_stage, internalformat, expected_value));
		}
		GLuint textures[kTargets];
		glGenTextures(kTargets, textures);

		for (int i = 0; i < kTargets; ++i)
		{
			glBindTexture(targets[i], textures[i]);
			glTexParameteri(targets[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(targets[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			if (targets[i] == GL_TEXTURE_2D)
			{
				glTexStorage2D(targets[i], 1, internalformat, kSize, 1);
			}
			else if (targets[i] == GL_TEXTURE_3D || targets[i] == GL_TEXTURE_2D_ARRAY)
			{
				glTexStorage3D(targets[i], 1, internalformat, kSize, 1, 2);
			}
			else if (targets[i] == GL_TEXTURE_CUBE_MAP)
			{
				glTexStorage2D(targets[i], 1, internalformat, kSize, kSize);
			}
		}
		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, internalformat); // 2D
		glBindImageTexture(2, textures[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 3D
		glBindImageTexture(0, textures[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // Cube
		glBindImageTexture(3, textures[3], 0, GL_TRUE, 0, GL_WRITE_ONLY, internalformat);  // 2DArray

		std::vector<ivec4> b_data(kSize);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * 4 * 4, &b_data[0], GL_STATIC_DRAW);

		glUseProgram(program_store);
		glBindVertexArray(m_vao);
		if (m_stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		else if (m_stage == 0)
		{ // VS
			glDrawArrays(GL_POINTS, 0, kSize);
		}
		bool status = true;
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		glBindImageTexture(3, textures[0], 0, GL_FALSE, 0, GL_READ_ONLY, internalformat); // 2D
		glBindImageTexture(2, textures[1], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 3D
		glBindImageTexture(1, textures[2], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // Cube
		glBindImageTexture(0, textures[3], 0, GL_TRUE, 0, GL_READ_ONLY, internalformat);  // 2DArray

		glUseProgram(program_load);
		if (m_stage == 0)
		{ // VS
			glDrawArrays(GL_POINTS, 0, kSize);
		}
		else if (m_stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		ivec4* out_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize; ++i)
		{
			if (!Equal(out_data[i], ivec4(0, 1, 0, 1), 0))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] load/store operation check failed. ("
					<< ToString(out_data[i]).c_str() << ")" << tcu::TestLog::EndMessage;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glUseProgram(0);
		glDeleteProgram(program_store);
		glDeleteProgram(program_load);
		glDeleteTextures(kTargets, textures);
		return status;
	}

	template <typename T>
	std::string GenStoreShader(int stage, GLenum internalformat, const T& write_value)
	{
		std::ostringstream os;
		if (stage == 4)
		{ // CS
			os << NL "#define KSIZE 100" NL "layout(local_size_x = KSIZE) in;";
		}
		os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) writeonly uniform "
		   << TypePrefix<T>() << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 2) writeonly uniform " << TypePrefix<T>() << "image3D g_image_3d;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 0) writeonly uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 3) writeonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "layout(std430) buffer out_data {" NL "  ivec4 o_color;" NL "};" NL
			  "void main() {" NL "  "
		   << TypePrefix<T>() << "vec4 g_value = " << TypePrefix<T>() << "vec4(o_color) + " << TypePrefix<T>() << "vec4"
		   << write_value
		   << ";" NL "  int g_index[6] = int[](o_color.x, o_color.y, o_color.z, o_color.w, o_color.r, o_color.g);";
		if (stage == 0)
		{ // VS
			os << NL "  ivec2 coord = ivec2(gl_VertexID, g_index[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_index[0]);";
		}
		os << NL "  imageStore(g_image_2d, coord, g_value);" NL
				 "  imageStore(g_image_3d, ivec3(coord.xy, g_index[0]), g_value);" NL
				 "  imageStore(g_image_3d, ivec3(coord.xy, g_index[1]), g_value);" NL
				 "  for (int i = 0; i < 6; ++i) {" NL
				 "    imageStore(g_image_cube, ivec3(coord, g_index[i]), g_value);" NL "  }" NL
				 "  imageStore(g_image_2darray, ivec3(coord, g_index[0]), g_value);" NL
				 "  imageStore(g_image_2darray, ivec3(coord, g_index[1]), g_value);" NL "}";
		return os.str();
	}

	template <typename T>
	std::string GenLoadShader(int stage, GLenum internalformat, const T& expected_value)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 100";
		if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = KSIZE) in;";
		}
		os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) readonly uniform "
		   << TypePrefix<T>() << "image2D g_image_2d;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 2) readonly uniform " << TypePrefix<T>() << "image3D g_image_3d;" NL "layout("
		   << FormatEnumToString(internalformat) << ", binding = 1) readonly uniform " << TypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(" << FormatEnumToString(internalformat)
		   << ", binding = 0) readonly uniform " << TypePrefix<T>()
		   << "image2DArray g_image_2darray;" NL "layout(std430) buffer out_data {" NL "  ivec4 o_color[KSIZE];" NL
			  "};" NL "void main() {";

		if (stage == 0)
		{ // VS
			os << NL "  " << TypePrefix<T>() << "vec4 g_value = " << TypePrefix<T>() << "vec4(o_color[gl_VertexID]) + "
			   << TypePrefix<T>() << "vec4" << expected_value << ";";
		}
		else if (stage == 4)
		{ // CS
			os << NL "  " << TypePrefix<T>() << "vec4 g_value = " << TypePrefix<T>()
			   << "vec4(o_color[gl_GlobalInvocationID.x]) + " << TypePrefix<T>() << "vec4" << expected_value << ";";
		}

		os << NL "  int g_index[6] = int[](o_color[0].x, o_color[0].y, o_color[0].z, o_color[0].w, o_color[1].r, "
				 "o_color[1].g);";
		if (stage == 0)
		{ // VS
			os << NL "  ivec2 coord = ivec2(gl_VertexID, g_index[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_index[0]);";
		}
		os << NL "  vec4 r = vec4(0.0, 1.0, 0.0, 1.0);" NL "  " << TypePrefix<T>()
		   << "vec4 v;" NL "  v = imageLoad(g_image_2d, coord);" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, float(coord.x), 2.0);" NL
			  "  v = imageLoad(g_image_3d, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, float(coord.x), 3.0);" NL
			  "  v = imageLoad(g_image_cube, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, float(coord.x), 6.0);" NL
			  "  v = imageLoad(g_image_2darray, ivec3(coord, g_index[0]));" NL
			  "  if (v != g_value) r = vec4(1.0, 0.0, float(coord.x), 23.0);" NL "  o_color[coord.x] = ivec4(r);" NL
			  "}";
		return os.str();
	}

protected:
	long RunStage(int stage)
	{
		m_stage = stage;
		if (!AreOutputsAvailable(5))
			return NOT_SUPPORTED;

		if (!Write(GL_RGBA32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;
		if (!Write(GL_R32F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 0.0f, 0.0f, 1.0f)))
			return ERROR;
		if (!Write(GL_RGBA16F, vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f)))
			return ERROR;

		if (!Write(GL_RGBA32I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_R32I, ivec4(1, -2, 3, -4), ivec4(1, 0, 0, 1)))
			return ERROR;
		if (!Write(GL_RGBA16I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;
		if (!Write(GL_RGBA8I, ivec4(1, -2, 3, -4), ivec4(1, -2, 3, -4)))
			return ERROR;

		if (!Write(GL_RGBA32UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_R32UI, uvec4(7, 2, 3, 4), uvec4(7, 0, 0, 1)))
			return ERROR;
		if (!Write(GL_RGBA16UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;
		if (!Write(GL_RGBA8UI, uvec4(0, 1, 2, 3), uvec4(0, 1, 2, 3)))
			return ERROR;

		if (!Write(GL_RGBA8, vec4(1.0f), vec4(1.0f)))
			return ERROR;

		if (!Write(GL_RGBA8_SNORM, vec4(1.0f, -1.0f, 1.0f, -1.0f), vec4(1.0f, -1.0f, 1.0f, -1.0f)))
			return ERROR;

		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// AtomicMachine
//-----------------------------------------------------------------------------
class AtomicMachine : public ShaderImageLoadStoreBase
{
	GLuint m_vao;
	GLuint m_buffer;

	virtual long Setup()
	{
		glEnable(GL_RASTERIZER_DISCARD);
		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	template <typename T>
	bool Atomic(int target, int stage, GLenum internalformat)
	{
		GLuint program = 0;
		if (stage == 0)
		{ // VS
			const char* src_fs = NL "void main() {" NL "  discard;" NL "}";
			program			   = BuildProgram(GenShader<T>(target, stage, internalformat).c_str(), src_fs, true, false);
		}
		else if (stage == 4)
		{ // CS
			program = CreateComputeProgram(GenShader<T>(target, stage, internalformat), true);
		}

		const GLenum targets[] = { GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY };
		const int	kTargets  = sizeof(targets) / sizeof(targets[0]);
		const int	kSize	 = 100;

		GLuint textures[kTargets];
		glGenTextures(kTargets, textures);

		for (int i = 0; i < kTargets; ++i)
		{
			glBindTexture(targets[i], textures[i]);
			glTexParameteri(targets[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(targets[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			if (targets[i] == GL_TEXTURE_2D)
			{
				glTexStorage2D(targets[i], 1, internalformat, kSize, 1);
			}
			else if (targets[i] == GL_TEXTURE_3D || targets[i] == GL_TEXTURE_2D_ARRAY)
			{
				glTexStorage3D(targets[i], 1, internalformat, kSize, 1, 2);
			}
			else if (targets[i] == GL_TEXTURE_CUBE_MAP)
			{
				glTexStorage2D(targets[i], 1, internalformat, kSize, kSize);
			}
		}
		glBindImageTexture(1, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, internalformat); // 2D
		glBindImageTexture(2, textures[1], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // 3D
		glBindImageTexture(0, textures[2], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // Cube
		glBindImageTexture(3, textures[3], 0, GL_TRUE, 0, GL_READ_WRITE, internalformat);  // 2DArray

		std::vector<ivec4> b_data(kSize);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * 4 * 4, &b_data[0], GL_STATIC_DRAW);

		glUseProgram(program);
		glBindVertexArray(m_vao);
		if (stage == 0)
		{ // VS
			glDrawArrays(GL_POINTS, 0, kSize);
		}
		else if (stage == 4)
		{ // CS
			glDispatchCompute(1, 1, 1);
		}
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool   status   = true;
		ivec4* out_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize; ++i)
		{
			if (!Equal(out_data[i], ivec4(0, 1, 0, 1), 0))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i << "] Atomic operation check failed. ("
					<< ToString(out_data[i]).c_str() << ")" << tcu::TestLog::EndMessage;
			}
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glUseProgram(0);
		glDeleteProgram(program);
		glDeleteTextures(kTargets, textures);
		return status;
	}

	template <typename T>
	std::string GenShader(int target, int stage, GLenum internalformat)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 100";
		if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = KSIZE) in;";
		}
		switch (target)
		{
		case T2D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 1) coherent uniform "
			   << TypePrefix<T>() << "image2D g_image_2d;";
			break;
		case T3D:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 2) coherent uniform "
			   << TypePrefix<T>() << "image3D g_image_3d;";
			break;
		case TCM:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 0) coherent uniform "
			   << TypePrefix<T>() << "imageCube g_image_cube;";
			break;
		case T2DA:
			os << NL "layout(" << FormatEnumToString(internalformat) << ", binding = 3) coherent uniform "
			   << TypePrefix<T>() << "image2DArray g_image_2darray;";
			break;
		}
		os << NL "layout(std430) buffer out_data {" NL "  ivec4 o_color[KSIZE];" NL "} r;" NL << TypePrefix<T>()
		   << "vec2 t(int i) {" NL "  return " << TypePrefix<T>()
		   << "vec2(i);" NL "}" NL "void main() {" NL "  int g_value[6] = int[](r.o_color[0].x, r.o_color[0].y+1, "
			  "r.o_color[0].z+2, r.o_color[0].w+3, r.o_color[1].r+4, "
			  "r.o_color[1].g+5);";
		if (stage == 0)
		{ // VS
			os << NL "  ivec2 coord = ivec2(gl_VertexID, g_value[0]);";
		}
		else if (stage == 4)
		{ // CS
			os << NL "  ivec2 coord = ivec2(gl_GlobalInvocationID.x, g_value[0]);";
		}
		os << NL "  ivec4 o_color = ivec4(0, 1, 0, 1);";

		switch (target)
		{
		case T2D:
			os << NL "  ivec4 i = ivec4(1, 0, 0, 2);" NL "  imageAtomicExchange(g_image_2d, coord, t(0).x);" NL
					 "  if (imageAtomicAdd(g_image_2d, coord, t(2).x) != t(0).x) o_color = i;" NL
					 "  if (imageAtomicMin(g_image_2d, coord, t(3).x) != t(2).x) o_color = i;" NL
					 "  if (imageAtomicMax(g_image_2d, coord, t(4).x) != t(2).x) o_color = i;" NL
					 "  if (imageAtomicAnd(g_image_2d, coord, t(0).x) != t(4).x) o_color = i;" NL
					 "  if (imageAtomicOr(g_image_2d, coord, t(7).x) != t(0).x) o_color = i;" NL
					 "  if (imageAtomicXor(g_image_2d, coord, t(4).x) != t(7).x) o_color = i;" NL
					 "  if (imageAtomicExchange(g_image_2d, coord, t(1).x) != t(3).x) o_color = i;" NL
					 "  if (imageAtomicCompSwap(g_image_2d, coord, t(1).x, t(6).x) != t(1).x) o_color = i;" NL
					 "  if (imageAtomicExchange(g_image_2d, coord, t(0).x) != t(6).x) o_color = i;";
			break;
		case T3D:
			os << NL "  ivec4 i = ivec4(1, 0, 0, 3);" NL
					 "  imageAtomicExchange(g_image_3d, ivec3(coord, 0), t(0).x);" NL
					 "  if (imageAtomicAdd(g_image_3d, ivec3(coord, 0), t(2).x) != t(0).x) o_color = i;" NL
					 "  if (imageAtomicMin(g_image_3d, ivec3(coord, 0), t(3).x) != t(2).x) o_color = i;" NL
					 "  if (imageAtomicMax(g_image_3d, ivec3(coord, g_value[0]), t(4).x) != t(2).x) o_color = i;" NL
					 "  if (imageAtomicAnd(g_image_3d, ivec3(coord, 0), t(0).x) != t(4).x) o_color = i;" NL
					 "  if (imageAtomicOr(g_image_3d, ivec3(coord, 0), t(7).x) != t(0).x) o_color = i;" NL
					 "  if (imageAtomicXor(g_image_3d, ivec3(coord, 0), t(4).x) != t(7).x) o_color = i;" NL
					 "  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), t(1).x) != t(3).x) o_color = i;" NL
					 "  if (imageAtomicCompSwap(g_image_3d, ivec3(coord, 0), t(1).x, t(6).x) != t(1).x) o_color = i;" NL
					 "  if (imageAtomicExchange(g_image_3d, ivec3(coord, 0), t(0).x) != t(6).x) o_color = i;";
			break;
		case TCM:
			os << NL
				"  ivec4 i = ivec4(1, 0, 0, 6);" NL "  imageAtomicExchange(g_image_cube, ivec3(coord, 0), t(0).x);" NL
				"  if (imageAtomicAdd(g_image_cube, ivec3(coord, 0), t(g_value[2]).x) != t(0).x) o_color = i;" NL
				"  if (imageAtomicMin(g_image_cube, ivec3(coord, 0), t(3).x) != t(2).x) o_color = i;" NL
				"  if (imageAtomicMax(g_image_cube, ivec3(coord, 0), t(4).x) != t(2).x) o_color = i;" NL
				"  if (imageAtomicAnd(g_image_cube, ivec3(coord, 0), t(0).x) != t(4).x) o_color = i;" NL
				"  if (imageAtomicOr(g_image_cube, ivec3(coord, 0), t(7).x) != t(0).x) o_color = i;" NL
				"  if (imageAtomicXor(g_image_cube, ivec3(coord, 0), t(4).x) != t(7).x) o_color = i;" NL
				"  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), t(1).x) != t(3).x) o_color = i;" NL
				"  if (imageAtomicCompSwap(g_image_cube, ivec3(coord, g_value[0]), t(1).x, t(6).x) != t(1).x) o_color "
				"= i;" NL "  if (imageAtomicExchange(g_image_cube, ivec3(coord, 0), t(0).x) != t(6).x) o_color = i;";
			break;
		case T2DA:
			os << NL
				"  ivec4 i = ivec4(1, 0, 0, 23);" NL
				"  imageAtomicExchange(g_image_2darray, ivec3(coord, 0), t(0).x);" NL
				"  if (imageAtomicAdd(g_image_2darray, ivec3(coord, 0), t(2).x) != t(0).x) o_color = i;" NL
				"  if (imageAtomicMin(g_image_2darray, ivec3(coord, 0), t(3).x) != t(2).x) o_color = i;" NL
				"  if (imageAtomicMax(g_image_2darray, ivec3(coord, 0), t(4).x) != t(2).x) o_color = i;" NL
				"  if (imageAtomicAnd(g_image_2darray, ivec3(coord, 0), t(0).x) != t(4).x) o_color = i;" NL
				"  if (imageAtomicOr(g_image_2darray, ivec3(coord, 0), t(7).x) != t(0).x) o_color = i;" NL
				"  if (imageAtomicXor(g_image_2darray, ivec3(coord, 0), t(g_value[4]).x) != t(7).x) o_color = i;" NL
				"  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), t(1).x) != t(3).x) o_color = i;" NL
				"  if (imageAtomicCompSwap(g_image_2darray, ivec3(coord, 0), t(1).x, t(6).x) != t(1).x) o_color = i;" NL
				"  if (imageAtomicExchange(g_image_2darray, ivec3(coord, 0), t(0).x) != t(6).x) o_color = i;";
			break;
		}
		os << NL "  r.o_color[coord.x] = o_color;" NL "}";
		return os.str();
	}

protected:
	long RunStage(int stage)
	{
		if (!IsImageAtomicSupported())
			return NOT_SUPPORTED;
		if (!Atomic<GLint>(T2D, stage, GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(T2D, stage, GL_R32UI))
			return ERROR;
		if (!Atomic<GLint>(T3D, stage, GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(T3D, stage, GL_R32UI))
			return ERROR;
		if (!Atomic<GLint>(TCM, stage, GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(TCM, stage, GL_R32UI))
			return ERROR;
		if (!Atomic<GLint>(T2DA, stage, GL_R32I))
			return ERROR;
		if (!Atomic<GLuint>(T2DA, stage, GL_R32UI))
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
		if (!IsVSFSAvailable(4, 0) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;
		return RunStage(0);
	}
};

//-----------------------------------------------------------------------------
// 1.3.8 BasicAllTargetsLoadStoreCS
//-----------------------------------------------------------------------------
class BasicAllTargetsLoadStoreCS : public LoadStoreMachine
{
	virtual long Run()
	{
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
		if (!IsVSFSAvailable(4, 0) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;
		return RunStage(0);
	}
};

//-----------------------------------------------------------------------------
// 1.3.13 BasicAllTargetsAtomicCS
//-----------------------------------------------------------------------------
class BasicAllTargetsAtomicCS : public AtomicMachine
{
	virtual long Run()
	{
		return RunStage(4);
	}
};

//-----------------------------------------------------------------------------
// 1.4.1 BasicGLSLMisc
//-----------------------------------------------------------------------------
class BasicGLSLMiscFS : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_program;
	GLuint m_vao, m_vbo;
	GLuint m_buffer;

	virtual long Setup()
	{
		m_texture = 0;
		m_program = 0;
		m_vao = m_vbo = 0;
		m_buffer	  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 2) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;

		const int		   kSize = 32;
		std::vector<float> data(kSize * kSize * 4);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, kSize, kSize, 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kSize, kSize, 4, GL_RED, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		const char* src_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";

		const char* src_fs =
			NL "#define KSIZE 32" NL "layout(std430) buffer out_data {" NL "  ivec4 o_color[KSIZE*KSIZE];" NL "};" NL
			   "layout(r32f, binding = 0) coherent restrict uniform image2D g_image_layer0;" NL
			   "layout(r32f, binding = 1) volatile uniform image2D g_image_layer1;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image_layer0, coord, vec4(1.0));" NL
			   "  memoryBarrier();" NL "  imageStore(g_image_layer1, coord, vec4(2.0));" NL "  memoryBarrier();" NL
			   "  imageStore(g_image_layer0, coord, vec4(3.0));" NL "  memoryBarrier();" NL
			   "  int coordIndex = coord.x + KSIZE * coord.y;" NL
			   "  o_color[coordIndex] = ivec4(imageLoad(g_image_layer0, coord) + imageLoad(g_image_layer1, coord));" NL
			   "}";
		m_program = BuildProgram(src_vs, src_fs);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 1, GL_READ_WRITE, GL_R32F);

		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, kSize, kSize);

		std::vector<ivec4> o_data(kSize * kSize);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &o_data[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;

		ivec4* out_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(out_data[i], ivec4(5, 0, 0, 2), 0))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i
					<< "] Check failed. Received: " << ToString(out_data[i]).c_str()
					<< " instead of: " << ToString(ivec4(5, 0, 0, 2)).c_str() << tcu::TestLog::EndMessage;
			}
		}

		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glDeleteTextures(1, &m_texture);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_buffer);
		glUseProgram(0);
		glDeleteProgram(m_program);
		return NO_ERROR;
	}
};

class BasicGLSLMiscCS : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		m_texture = 0;
		m_program = 0;
		m_buffer  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int		   kSize = 10;
		std::vector<float> data(kSize * kSize * 4);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, kSize, kSize, 4);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kSize, kSize, 4, GL_RED, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		const char* src_cs =
			NL "#define KSIZE 10" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "layout(std430) buffer out_data {" NL "  ivec4 o_color[KSIZE*KSIZE];" NL "};" NL
			   "layout(r32f, binding = 0) coherent restrict uniform image2D g_image_layer0;" NL
			   "layout(r32f, binding = 1) volatile uniform image2D g_image_layer1;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_LocalInvocationID.xy);" NL "  imageStore(g_image_layer0, coord, vec4(1.0));" NL
			   "  memoryBarrier();" NL "  imageStore(g_image_layer1, coord, vec4(2.0));" NL "  memoryBarrier();" NL
			   "  imageStore(g_image_layer0, coord, vec4(3.0));" NL "  memoryBarrier();" NL
			   "  o_color[gl_LocalInvocationIndex] = ivec4(imageLoad(g_image_layer0, coord) + "
			   "imageLoad(g_image_layer1, coord));" NL "}";
		m_program = CreateComputeProgram(src_cs);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 1, GL_READ_WRITE, GL_R32F);

		std::vector<ivec4> o_data(kSize * kSize);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &o_data[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		bool status = true;

		ivec4* out_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize * kSize; ++i)
		{
			if (!Equal(out_data[i], ivec4(5, 0, 0, 2), 0))
			{
				status = false;
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "[" << i
					<< "] Check failed. Received: " << ToString(out_data[i]).c_str()
					<< " instead of: " << ToString(ivec4(5, 0, 0, 2)).c_str() << tcu::TestLog::EndMessage;
			}
		}

		if (status)
			return NO_ERROR;
		else
			return ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteTextures(1, &m_texture);
		glUseProgram(0);
		glDeleteProgram(m_program);
		glDeleteBuffers(1, &m_buffer);
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
	GLuint c_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		m_texture[0] = m_texture[1] = 0;
		m_program[0] = m_program[1] = 0;
		m_vao = m_vbo = 0;
		m_buffer	  = 0;
		c_program	 = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 1))
			return NOT_SUPPORTED;

		const int		  kSize = 8;
		std::vector<vec4> data(kSize * kSize);

		glGenTextures(2, m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		const char* glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* glsl_early_frag_tests_fs =
			NL "layout(early_fragment_tests) in;" NL "layout(location = 0) out vec4 o_color;" NL
			   "layout(rgba32f, binding = 0) writeonly coherent uniform image2D g_image;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image, coord, vec4(17.0));" NL
			   "  o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		const char* glsl_fs =
			NL "layout(location = 0) out vec4 o_color;" NL
			   "layout(rgba32f, binding = 1) writeonly coherent uniform image2D g_image;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageStore(g_image, coord, vec4(13.0));" NL
			   "  o_color = vec4(1.0, 0.0, 0.0, 1.0);" NL "}";
		m_program[0] = BuildProgram(glsl_vs, glsl_early_frag_tests_fs);
		m_program[1] = BuildProgram(glsl_vs, glsl_fs);

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

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(m_program[1]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		const char* check_cs =
			NL "#define KSIZE 8" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "uniform sampler2D g_sampler;" NL "layout(std430) buffer OutputBuffer {" NL
			   "  vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL
			   "  data[gl_LocalInvocationIndex] = texelFetch(g_sampler, ivec2(gl_LocalInvocationID), 0);" NL "}";

		c_program = CreateComputeProgram(check_cs);
		std::vector<vec4> out_data(kSize * kSize);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		glUseProgram(c_program);
		glUniform1i(glGetUniformLocation(c_program, "g_sampler"), 5);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		float					 expectedVal  = 0.0f;
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		if (renderTarget.getDepthBits() == 0)
		{
			expectedVal = 17.0f;
		}

		if (!CompareValues(map_data, kSize, vec4(expectedVal)))
			return ERROR;
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, vec4(13.0f)))
			return ERROR;

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
		glDeleteProgram(c_program);
		glDeleteBuffers(1, &m_buffer);
		glActiveTexture(GL_TEXTURE0);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 1.4.3 BasicGLSLConst
//-----------------------------------------------------------------------------
class BasicGLSLConst : public ShaderImageLoadStoreBase
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
		const char* src_cs =
			NL "layout (local_size_x = 1) in;" NL "layout(std430) buffer out_data {" NL "  ivec4 o_color;" NL "};" NL
			   "uniform int MaxImageUnits;" NL "uniform int MaxCombinedShaderOutputResources;" NL
			   "uniform int MaxVertexImageUniforms;" NL "uniform int MaxFragmentImageUniforms;" NL
			   "uniform int MaxComputeImageUniforms;" NL "uniform int MaxCombinedImageUniforms;" NL "void main() {" NL
			   "  o_color = ivec4(0, 1, 0, 1);" NL
			   "  if (gl_MaxImageUnits != MaxImageUnits) o_color = ivec4(1, 0, 0, 1);" NL
			   "  if (gl_MaxCombinedShaderOutputResources != MaxCombinedShaderOutputResources) o_color = ivec4(1, 0, "
			   "0, 2);" NL "  if (gl_MaxVertexImageUniforms != MaxVertexImageUniforms) o_color = ivec4(1, 0, 0, 4);" NL
			   "  if (gl_MaxFragmentImageUniforms != MaxFragmentImageUniforms) o_color = ivec4(1, 0, 0, 5);" NL
			   "  if (gl_MaxComputeImageUniforms != MaxComputeImageUniforms) o_color = ivec4(1, 0, 0, 6);" NL
			   "  if (gl_MaxCombinedImageUniforms != MaxCombinedImageUniforms) o_color = ivec4(1, 0, 0, 9);" NL "}";
		m_program = CreateComputeProgram(src_cs);
		glUseProgram(m_program);

		GLint i;
		glGetIntegerv(GL_MAX_IMAGE_UNITS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxImageUnits"), i);

		glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxCombinedShaderOutputResources"), i);

		glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxVertexImageUniforms"), i);

		glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxFragmentImageUniforms"), i);

		glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxComputeImageUniforms"), i);

		glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, &i);
		glUniform1i(glGetUniformLocation(m_program, "MaxCombinedImageUniforms"), i);

		std::vector<ivec4> out_data(1);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		ivec4* map_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4, GL_MAP_READ_BIT);

		if (!Equal(map_data[0], ivec4(0, 1, 0, 1), 0))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "[" << i << "] Value is: " << ToString(map_data[0]).c_str()
				<< ". Value should be: " << ToString(ivec4(0, 1, 0, 1)).c_str() << tcu::TestLog::EndMessage;
			return ERROR;
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

//-----------------------------------------------------------------------------
// 2.1.1 AdvancedSyncImageAccess
//-----------------------------------------------------------------------------
class AdvancedSyncImageAccess : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_texture;
	GLuint m_store_program;
	GLuint m_draw_program;
	GLuint m_attribless_vao;

	virtual long Setup()
	{
		m_buffer		 = 0;
		m_texture		 = 0;
		m_store_program  = 0;
		m_draw_program   = 0;
		m_attribless_vao = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(1, 0) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;

		const int		  kSize = 44;
		const char* const glsl_store_vs =
			NL "layout(rgba32f) writeonly uniform image2D g_output_data;" NL "void main() {" NL
			   "  vec2[4] data = vec2[4](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));" NL
			   "  imageStore(g_output_data, ivec2(gl_VertexID,0), vec4(data[gl_VertexID], 0.0, 1.0));" NL
			   "  gl_PointSize = 1.0;" NL "}";
		const char* const glsl_store_fs = NL "void main() {" NL "  discard;" NL "}";
		const char* const glsl_draw_vs =
			NL "out vec4 vs_color;" NL "layout(rgba32f) readonly uniform image2D g_image;" NL
			   "uniform sampler2D g_sampler;" NL "void main() {" NL
			   "  vec4 pi = imageLoad(g_image, ivec2(gl_VertexID, 0));" NL
			   "  vec4 ps = texelFetch(g_sampler, ivec2(gl_VertexID, 0), 0);" NL
			   "  if (pi != ps) vs_color = vec4(1.0, 0.0, 0.0, 1.0);" NL
			   "  else vs_color = vec4(0.0, 1.0, 0.0, 1.0);" NL "  gl_Position = pi;" NL "}";
		const char* const glsl_draw_fs =
			NL "#define KSIZE 44" NL "in vec4 vs_color;" NL "layout(std430) buffer OutputBuffer {" NL
			   "  vec4 o_color[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
			   "  int coordIndex = coord.x + KSIZE * coord.y;" NL "  o_color[coordIndex] = vs_color;" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, glsl_store_fs);
		m_draw_program  = BuildProgram(glsl_draw_vs, glsl_draw_fs);

		glGenVertexArrays(1, &m_attribless_vao);
		glBindVertexArray(m_attribless_vao);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		std::vector<ivec4> data(4);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 4, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 1, GL_RGBA, GL_FLOAT, &data[0]);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glUseProgram(m_store_program);
		glDrawArrays(GL_POINTS, 0, 4);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

		glViewport(0, 0, kSize, kSize);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);

		std::vector<vec4> out_data(kSize * kSize);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &out_data[0], GL_STATIC_DRAW);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, vec4(0, 1, 0, 1)))
			return ERROR;

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
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
	GLuint m_texture[3];
	GLuint m_store_program;
	GLuint m_copy_program;
	GLuint m_draw_program;
	GLuint m_attribless_vao;
	GLuint m_draw_vao;

	virtual long Setup()
	{
		m_position_buffer = 0;
		m_color_buffer	= 0;
		m_element_buffer  = 0;
		m_store_program   = 0;
		m_draw_program	= 0;
		m_copy_program	= 0;
		m_attribless_vao  = 0;
		m_draw_vao		  = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(3, 0))
			return NOT_SUPPORTED;
		const char* const glsl_store_vs =
			NL "layout(rgba32f, binding = 0) writeonly uniform image2D g_position_buffer;" NL
			   "layout(rgba32f, binding = 1) writeonly uniform image2D g_color_buffer;" NL
			   "layout(r32ui, binding = 2) writeonly uniform uimage2D g_element_buffer;" NL "uniform vec4 g_color;" NL
			   "void main() {" NL "  vec2[4] data = vec2[4](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));" NL
			   "  imageStore(g_position_buffer, ivec2(gl_VertexID,0), vec4(data[gl_VertexID], 0.0, 1.0));" NL
			   "  imageStore(g_color_buffer, ivec2(gl_VertexID,0), g_color);" NL
			   "  imageStore(g_element_buffer, ivec2(gl_VertexID,0), uvec4(gl_VertexID));" NL "}";
		const char* const glsl_store_fs = NL "void main() {" NL "  discard;" NL "}";
		const char*		  glsl_copy_cs =
			NL "#define KSIZE 4" NL "layout (local_size_x = KSIZE) in;" NL
			   "layout(rgba32f, binding = 0) readonly uniform image2D g_position_img;" NL
			   "layout(rgba32f, binding = 1) readonly uniform image2D g_color_img;" NL
			   "layout(r32ui, binding = 2) readonly uniform uimage2D g_element_img;" NL
			   "layout(std430, binding = 1) buffer g_position_buf {" NL "  vec2 g_pos[KSIZE];" NL "};" NL
			   "layout(std430, binding = 2) buffer g_color_buf {" NL "  vec4 g_col[KSIZE];" NL "};" NL
			   "layout(std430, binding = 3) buffer g_element_buf {" NL "  uint g_elem[KSIZE];" NL "};" NL
			   "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID.x, 0);" NL
			   "  g_pos[coord.x] = (imageLoad(g_position_img, coord)).xy;" NL
			   "  g_col[coord.x] = imageLoad(g_color_img, coord);" NL
			   "  g_elem[coord.x] = uint((imageLoad(g_element_img, coord)).x);" NL "}";
		const char* const glsl_draw_vs = NL
			"layout(location = 0) in vec4 i_position;" NL "layout(location = 1) in vec4 i_color;" NL
			"out vec4 vs_color;" NL "void main() {" NL "  gl_Position = i_position;" NL "  vs_color = i_color;" NL "}";
		const char* const glsl_draw_fs = NL "in vec4 vs_color;" NL "layout(location = 0) out vec4 o_color;" NL
											"void main() {" NL "  o_color = vs_color;" NL "}";
		m_store_program = BuildProgram(glsl_store_vs, glsl_store_fs);
		m_copy_program  = CreateComputeProgram(glsl_copy_cs);
		m_draw_program  = BuildProgram(glsl_draw_vs, glsl_draw_fs);

		glGenTextures(3, m_texture);
		std::vector<ivec4> data(4);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 4, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 4, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, m_texture[2]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 4, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glGenVertexArrays(1, &m_attribless_vao);
		glGenVertexArrays(1, &m_draw_vao);
		glBindVertexArray(m_draw_vao);
		glGenBuffers(1, &m_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, 4 * 4 * 4, 0, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glGenBuffers(1, &m_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, 4 * 4 * 4, 0, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glGenBuffers(1, &m_element_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * 4 * 4, 0, GL_STATIC_DRAW);
		glBindVertexArray(0);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture[2], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 1.0f, 0.0f, 1.0f);
		glBindVertexArray(m_attribless_vao);
		glDrawArrays(GL_POINTS, 0, 4);

		glUseProgram(m_copy_program);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_position_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_color_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_element_buffer);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(1, 1, 1);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_draw_vao);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);

		if (!CheckFB(vec4(0, 1, 0, 1)))
		{
			return ERROR;
		}

		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 0.0f, 1.0f, 1.0f);
		glBindVertexArray(m_attribless_vao);
		glDrawArrays(GL_POINTS, 0, 4);
		glUseProgram(m_copy_program);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(m_draw_program);
		glBindVertexArray(m_draw_vao);
		glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);

		if (!CheckFB(vec4(0, 0, 1, 1)))
		{
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDisable(GL_RASTERIZER_DISCARD);
		glUseProgram(0);
		glDeleteTextures(3, m_texture);
		glDeleteBuffers(1, &m_position_buffer);
		glDeleteBuffers(1, &m_color_buffer);
		glDeleteBuffers(1, &m_element_buffer);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_copy_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_attribless_vao);
		glDeleteVertexArrays(1, &m_draw_vao);
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
	GLuint m_buffer;

	virtual long Setup()
	{
		m_texture		= 0;
		m_store_program = 0;
		m_draw_program  = 0;
		m_vao			= 0;
		m_vbo			= 0;
		m_buffer		= 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 32;
		if (!IsVSFSAvailable(0, 1) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;
		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_store_fs =
			NL "layout(rgba32f) writeonly uniform image2D g_image;" NL "uniform vec4 g_color;" NL "void main() {" NL
			   "  imageStore(g_image, ivec2(gl_FragCoord.xy), g_color);" NL "  discard;" NL "}";
		const char* const glsl_draw_fs =
			NL "layout(location = 0) out vec4 o_color;" NL "uniform sampler2D g_sampler;" NL
			   "layout(std430) buffer OutputBuffer {" NL "  uvec4 counter;" NL "  vec4 data[];" NL "};" NL
			   "void main() {" NL "  uint idx = atomicAdd(counter[0], 1u);" NL
			   "  data[idx] = texelFetch(g_sampler, ivec2(gl_FragCoord.xy), 0);" NL "}";
		m_store_program = BuildProgram(glsl_vs, glsl_store_fs);
		m_draw_program  = BuildProgram(glsl_vs, glsl_draw_fs);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, getWindowWidth(), getWindowHeight());
		glBindTexture(GL_TEXTURE_2D, 0);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		glViewport(0, 0, kSize, kSize);
		std::vector<vec4> data_b(kSize * kSize + 1);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (kSize * kSize + 1) * 4 * 4, &data_b[0], GL_STATIC_DRAW);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glUseProgram(m_store_program);
		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(m_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUniform4f(glGetUniformLocation(m_store_program, "g_color"), 0.0f, 1.0f, 0.0f, 1.0f);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glUseProgram(m_draw_program);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data =
			(vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 4 * 4, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, vec4(0, 1, 0, 1)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_draw_program);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.2.1 AdvancedAllStagesOneImage
//-----------------------------------------------------------------------------
class AdvancedAllStagesOneImage : public ShaderImageLoadStoreBase
{
	GLuint m_program;
	GLuint c_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_buffer;
	GLuint m_texture;

	virtual long Setup()
	{
		m_program = 0;
		c_program = 0;
		m_vao	 = 0;
		m_vbo	 = 0;
		m_buffer  = 0;
		m_texture = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 64;
		if (!IsVSFSAvailable(1, 1) || !IsImageAtomicSupported())
			return NOT_SUPPORTED;
		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL
			   "layout(r32ui, binding = 3) coherent uniform uimage2D g_image;" NL "void main() {" NL
			   "  gl_Position = i_position;" NL "  imageAtomicAdd(g_image, ivec2(0, gl_VertexID), 100u);" NL "}";
		const char* const glsl_fs =
			NL "#define KSIZE 64" NL "layout(r32ui, binding = 3) coherent uniform uimage2D g_image;" NL
			   "void main() {" NL "  imageAtomicAdd(g_image, ivec2(0, int(gl_FragCoord.x) & 0x03), 0x1u);" NL "}";
		m_program = BuildProgram(glsl_vs, glsl_fs, true, true);
		const char* const glsl_cs =
			NL "#define KSIZE 4" NL "layout (local_size_x = KSIZE) in;" NL
			   "layout(r32ui, binding = 3) uniform uimage2D g_image;" NL "layout(std430) buffer out_data {" NL
			   "  uvec4 data[KSIZE];" NL "};" NL "void main() {" NL
			   "  uvec4 v = imageLoad(g_image, ivec2(0, gl_LocalInvocationID.x));" NL
			   "  data[gl_LocalInvocationIndex] = v;" NL "}";
		c_program = CreateComputeProgram(glsl_cs, true);
		glUseProgram(m_program);

		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		std::vector<uvec4> ui32(16);
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 4, 4);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RED_INTEGER, GL_UNSIGNED_INT, &ui32[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(3, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		glViewport(0, 0, kSize, kSize);
		glBindVertexArray(m_vao);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		std::vector<vec4> data_b(4);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * 4 * 4, &data_b[0], GL_STATIC_DRAW);
		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		uvec4* map_data = (uvec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, 2, uvec4(1024 + 100, 0, 0, 1)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteBuffers(1, &m_vbo);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_program);
		glDeleteProgram(c_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.3.2 AdvancedMemoryOrder
//-----------------------------------------------------------------------------
class AdvancedMemoryOrderVSFS : public ShaderImageLoadStoreBase
{
	GLuint m_buffer;
	GLuint m_texture[2];
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;

	virtual long Setup()
	{
		m_buffer  = 0;
		m_program = 0;
		m_vao	 = 0;
		m_vbo	 = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;
		if (!IsVSFSAvailable(1, 1) || !IsSSBInVSFSAvailable(1))
			return NOT_SUPPORTED;
		const char* const glsl_vs = NL
			"layout(location = 0) in vec4 i_position;" NL "out vec4 vs_color;" NL
			"layout(r32f, binding = 0) uniform image2D g_image_vs;" NL "void main() {" NL
			"  gl_Position = i_position;" NL "  vs_color = vec4(41, 42, 43, 44);" NL
			"  imageStore(g_image_vs, ivec2(gl_VertexID), vec4(1.0));" NL
			"  imageStore(g_image_vs, ivec2(gl_VertexID), vec4(2.0));" NL
			"  imageStore(g_image_vs, ivec2(gl_VertexID), vec4(3.0));" NL
			"  if (imageLoad(g_image_vs, ivec2(gl_VertexID)) != vec4(3,0,0,1)) vs_color = vec4(21, 22, 23, 24);" NL "}";
		const char* const glsl_fs =
			NL "#define KSIZE 11" NL "in vec4 vs_color;" NL "layout(r32f, binding = 1) uniform image2D g_image;" NL
			   "layout(std430) buffer out_data {" NL "  vec4 data[KSIZE*KSIZE*4];" NL "};" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord);" NL "  int coordIndex = coord.x + KSIZE * coord.y;" NL
			   "  for (int i = 0; i < 4; ++i) {" NL "    data[coordIndex + i * KSIZE*KSIZE] = vs_color;" NL "  }" NL
			   "  for (int i = 0; i < 4; ++i) {" NL "    imageStore(g_image, coord, vec4(i+50));" NL
			   "    vec4 v = imageLoad(g_image, coord);" NL "    if (v.x != float(i+50)) {" NL
			   "      data[coordIndex + i * KSIZE*KSIZE] = vec4(v.xyz, i+10);" NL "      break;" NL "    }" NL "  }" NL
			   "}";
		m_program = BuildProgram(glsl_vs, glsl_fs);
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);

		std::vector<float> data(kSize * kSize);
		glGenTextures(2, m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RED, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RED, GL_FLOAT, &data[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

		std::vector<vec4> data_b(kSize * kSize * 4);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * kSize * kSize * 4 * 4, &data_b[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glViewport(0, 0, kSize, kSize);
		glBindVertexArray(m_vao);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data =
			(vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 4 * kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize * 2, vec4(41, 42, 43, 44)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(2, m_texture);
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
	bool   pipeline;
	GLuint m_texture;
	GLuint m_pipeline[2];
	GLuint m_vsp, m_fsp0, m_fsp1;
	GLuint m_vao, m_vbo;
	GLuint m_program[2];
	GLuint c_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		c_program = 0;
		m_buffer  = 0;
		CreateFullViewportQuad(&m_vao, &m_vbo, NULL);
		glGenTextures(1, &m_texture);
		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs0 =
			NL "layout(rgba32f, binding = 2) writeonly uniform image2D g_image[2];" NL "void main() {" NL
			   "  int i = g_image.length();" NL "  imageStore(g_image[0], ivec2(gl_FragCoord), vec4(i+98));" NL
			   "  imageStore(g_image[1], ivec2(gl_FragCoord), vec4(i+99));" NL "  discard;" NL "}";
		const char* const glsl_fs1 =
			NL "layout(rgba32f, binding = 0) writeonly uniform image2D g_image[2];" NL "void main() {" NL
			   "  int i = g_image.length();" NL "  imageStore(g_image[0], ivec2(gl_FragCoord), vec4(i+8));" NL
			   "  imageStore(g_image[1], ivec2(gl_FragCoord), vec4(i+9));" NL "  discard;" NL "}";
		if (pipeline)
		{
			glGenProgramPipelines(2, m_pipeline);
			m_vsp  = BuildShaderProgram(GL_VERTEX_SHADER, glsl_vs);
			m_fsp0 = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs0);
			m_fsp1 = BuildShaderProgram(GL_FRAGMENT_SHADER, glsl_fs1);
		}
		else
		{
			m_program[0] = BuildProgram(glsl_vs, glsl_fs0);
			m_program[1] = BuildProgram(glsl_vs, glsl_fs1);
		}
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 2))
			return NOT_SUPPORTED;
		const int kSize = 4;

		if (pipeline)
		{
			glUseProgramStages(m_pipeline[0], GL_VERTEX_SHADER_BIT, m_vsp);
			glUseProgramStages(m_pipeline[0], GL_FRAGMENT_SHADER_BIT, m_fsp0);
			glUseProgramStages(m_pipeline[1], GL_VERTEX_SHADER_BIT, m_vsp);
			glUseProgramStages(m_pipeline[1], GL_FRAGMENT_SHADER_BIT, m_fsp1);
		}
		glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, getWindowWidth(), getWindowHeight(), 8);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 6, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 0, GL_FALSE, 4, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture, 0, GL_FALSE, 1, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(3, m_texture, 0, GL_FALSE, 3, GL_READ_WRITE, GL_RGBA32F);

		glBindVertexArray(m_vao);
		if (pipeline)
			glBindProgramPipeline(m_pipeline[0]);
		else
			glUseProgram(m_program[0]);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

		if (pipeline)
			glBindProgramPipeline(m_pipeline[1]);
		else
			glUseProgram(m_program[1]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		const char* const glsl_cs =
			NL "#define KSIZE 4" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "layout(rgba32f, binding = 0) readonly uniform image2D g_image[4];" NL
			   "layout(std430) buffer OutputBuffer {" NL "  uvec4 counter;" NL "  vec4 data[];" NL "};" NL
			   "void main() {" NL "  uint idx = atomicAdd(counter[0], 1u);" NL
			   "  data[idx][0] = (imageLoad(g_image[0], ivec2(gl_GlobalInvocationID))).z;" NL
			   "  data[idx][1] = (imageLoad(g_image[1], ivec2(gl_GlobalInvocationID))).z;" NL
			   "  data[idx][2] = (imageLoad(g_image[2], ivec2(gl_GlobalInvocationID))).z;" NL
			   "  data[idx][3] = (imageLoad(g_image[3], ivec2(gl_GlobalInvocationID))).z;" NL "}";
		c_program = CreateComputeProgram(glsl_cs);
		glUseProgram(c_program);
		int wsx   = (getWindowWidth() / kSize) * kSize;
		int wsy   = (getWindowHeight() / kSize) * kSize;
		int minor = wsx > wsy ? wsy : wsx;

		std::vector<vec4> data_b(wsx * wsy + 1);
		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, (wsx * wsy + 1) * 4 * 4, &data_b[0], GL_STATIC_DRAW);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(wsx / kSize, wsy / kSize, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 4 * 4, wsx * wsy * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, minor, vec4(10, 11, 100, 101)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteTextures(1, &m_texture);
		if (pipeline)
		{
			glDeleteProgram(m_vsp);
			glDeleteProgram(m_fsp0);
			glDeleteProgram(m_fsp1);
			glDeleteProgramPipelines(2, m_pipeline);
		}
		else
		{
			glDeleteProgram(m_program[0]);
			glDeleteProgram(m_program[1]);
		}
		glDeleteProgram(c_program);
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}

public:
	AdvancedSSOSimple() : pipeline(true)
	{
	}
};

//-----------------------------------------------------------------------------
// 2.5 AdvancedCopyImage
//-----------------------------------------------------------------------------
class AdvancedCopyImageFS : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint c_program;
	GLuint m_vao, m_vbo, m_ebo;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);
		glGenBuffers(1, &m_buffer);

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs =
			NL "layout(rgba32f, binding = 3) readonly uniform image2D g_input_image;" NL
			   "layout(rgba32f, binding = 1) writeonly uniform image2D g_output_image;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord);" NL
			   "  imageStore(g_output_image, coord, imageLoad(g_input_image, coord));" NL "  discard;" NL "}";
		m_program				  = BuildProgram(glsl_vs, glsl_fs);
		const char* const glsl_cs = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(rgba32f, binding = 2) readonly uniform image2D g_image;" NL "layout(std430) buffer out_data {" NL
			"  vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex] = imageLoad(g_image, coord);" NL "}";
		c_program = CreateComputeProgram(glsl_cs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;
		if (!IsVSFSAvailable(0, 2))
			return NOT_SUPPORTED;

		std::vector<vec4> data(kSize * kSize, vec4(7.0f));
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_FLOAT, &data[0]);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(3, m_texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(2, m_texture[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

		std::vector<vec4> data_b(kSize * kSize);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data_b[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, vec4(7.f)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteProgram(c_program);
		glDeleteVertexArrays(1, &m_vao);
		return NO_ERROR;
	}
};

class AdvancedCopyImageCS : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint c_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		glGenBuffers(1, &m_buffer);

		const char* const glsl_cs =
			NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "layout(rgba32f, binding = 3) readonly uniform image2D g_input_image;" NL
			   "layout(rgba32f, binding = 1) writeonly uniform image2D g_output_image;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			   "  imageStore(g_output_image, coord, imageLoad(g_input_image, coord));" NL "}";
		m_program					= CreateComputeProgram(glsl_cs);
		const char* const glsl_cs_c = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(rgba32f, binding = 2) readonly uniform image2D g_image;" NL "layout(std430) buffer out_data {" NL
			"  vec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex] = imageLoad(g_image, coord);" NL "}";
		c_program = CreateComputeProgram(glsl_cs_c);

		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;

		std::vector<vec4> data(kSize * kSize, vec4(7.0f));
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_FLOAT, &data[0]);

		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, kSize, kSize);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(3, m_texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(2, m_texture[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

		std::vector<vec4> data_b(kSize * kSize);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data_b[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		vec4* map_data = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, vec4(7.f)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteProgram(c_program);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.6 AdvancedAllMips
//-----------------------------------------------------------------------------
class AdvancedAllMipsFS : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_store_program, m_load_program;
	GLuint m_vao, m_vbo, m_ebo;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(1, &m_texture);
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);
		glGenBuffers(1, &m_buffer);

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_store_fs =
			NL "layout(rgba32f, binding = 0) writeonly uniform image2D g_image[4];" NL "void main() {" NL
			   "  imageStore(g_image[0], ivec2(gl_FragCoord), vec4(23));" NL
			   "  imageStore(g_image[1], ivec2(gl_FragCoord), vec4(24));" NL
			   "  imageStore(g_image[2], ivec2(gl_FragCoord), vec4(25));" NL
			   "  imageStore(g_image[3], ivec2(gl_FragCoord), vec4(26));" NL "  discard;" NL "}";
		const char* const glsl_load_cs = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(rgba32f, binding = 0) readonly uniform image2D g_image[4];" NL "layout(std430) buffer out_data {" NL
			"  ivec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex] = ivec4(2, 3, 4, 5);" NL "  vec4 c0 = imageLoad(g_image[0], coord);" NL
			"  vec4 c1 = imageLoad(g_image[1], coord);" NL "  vec4 c2 = imageLoad(g_image[2], coord);" NL
			"  vec4 c3 = imageLoad(g_image[3], coord);" NL
			"  if ((all(lessThan(coord, ivec2(2))) && c0 != vec4(23)) || (any(greaterThanEqual(coord, ivec2(2))) && "
			"c0.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][0] = int(c0.x);" NL
			"  if ((all(lessThan(coord, ivec2(4))) && c1 != vec4(24)) || (any(greaterThanEqual(coord, ivec2(4))) && "
			"c1.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][1] = int(c1.x);" NL
			"  if ((all(lessThan(coord, ivec2(8))) && c2 != vec4(25)) || (any(greaterThanEqual(coord, ivec2(8))) && "
			"c2.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][2] = int(c2.x);" NL
			"  if ((all(lessThan(coord, ivec2(16))) && c3 != vec4(26)) || (any(greaterThanEqual(coord, ivec2(16))) && "
			"c3.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][3] = int(c3.x);" NL "}";
		m_store_program = BuildProgram(glsl_vs, glsl_store_fs);
		m_load_program  = CreateComputeProgram(glsl_load_cs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;
		if (!IsVSFSAvailable(0, 4))
			return NOT_SUPPORTED;
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexStorage2D(GL_TEXTURE_2D, 8, GL_RGBA32F, 128, 128);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture, 6, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 5, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture, 4, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(3, m_texture, 3, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		std::vector<GLubyte> data(kSize * kSize * 4 * 4);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data[0], GL_STATIC_DRAW);

		glViewport(0, 0, kSize, kSize);
		glBindVertexArray(m_vao);

		glUseProgram(m_store_program);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(m_load_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		ivec4* map_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, ivec4(2, 3, 4, 5)))
			return ERROR;
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
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

class AdvancedAllMipsCS : public ShaderImageLoadStoreBase
{
	GLuint m_texture;
	GLuint m_store_program, m_load_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(1, &m_texture);
		glGenBuffers(1, &m_buffer);

		const char* const glsl_store_cs =
			NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "layout(rgba32f, binding = 0) writeonly uniform image2D g_image[4];" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL "  imageStore(g_image[0], coord, vec4(23));" NL
			   "  imageStore(g_image[1], coord, vec4(24));" NL "  imageStore(g_image[2], coord, vec4(25));" NL
			   "  imageStore(g_image[3], coord, vec4(26));" NL "}";
		const char* const glsl_load_cs = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(rgba32f, binding = 0) readonly uniform image2D g_image[4];" NL "layout(std430) buffer out_data {" NL
			"  ivec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex] = ivec4(2, 3, 4, 5);" NL "  vec4 c0 = imageLoad(g_image[0], coord);" NL
			"  vec4 c1 = imageLoad(g_image[1], coord);" NL "  vec4 c2 = imageLoad(g_image[2], coord);" NL
			"  vec4 c3 = imageLoad(g_image[3], coord);" NL
			"  if ((all(lessThan(coord, ivec2(2))) && c0 != vec4(23)) || (any(greaterThanEqual(coord, ivec2(2))) && "
			"c0.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][0] = int(c0.x);" NL
			"  if ((all(lessThan(coord, ivec2(4))) && c1 != vec4(24)) || (any(greaterThanEqual(coord, ivec2(4))) && "
			"c1.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][1] = int(c1.x);" NL
			"  if ((all(lessThan(coord, ivec2(8))) && c2 != vec4(25)) || (any(greaterThanEqual(coord, ivec2(8))) && "
			"c2.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][2] = int(c2.x);" NL
			"  if ((all(lessThan(coord, ivec2(16))) && c3 != vec4(26)) || (any(greaterThanEqual(coord, ivec2(16))) && "
			"c3.xyz != vec3(0.0)))" NL "      data[gl_LocalInvocationIndex][3] = int(c3.x);" NL "}";
		m_store_program = CreateComputeProgram(glsl_store_cs);
		m_load_program  = CreateComputeProgram(glsl_load_cs);

		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexStorage2D(GL_TEXTURE_2D, 8, GL_RGBA32F, 128, 128);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture, 6, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, m_texture, 5, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(2, m_texture, 4, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(3, m_texture, 3, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		std::vector<GLubyte> data(kSize * kSize * 4 * 4);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data[0], GL_STATIC_DRAW);

		glUseProgram(m_store_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(m_load_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		ivec4* map_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, ivec4(2, 3, 4, 5)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteTextures(1, &m_texture);
		glDeleteProgram(m_store_program);
		glDeleteProgram(m_load_program);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};

//-----------------------------------------------------------------------------
// 2.7 AdvancedCast
//-----------------------------------------------------------------------------
class AdvancedCastFS : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint c_program;
	GLuint m_vao, m_vbo, m_ebo;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		glGenBuffers(1, &m_buffer);
		m_program = 0;
		c_program = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!IsVSFSAvailable(0, 2) || !IsImageAtomicSupported())
			return NOT_SUPPORTED;
		const int kSize = 11;
		CreateFullViewportQuad(&m_vao, &m_vbo, &m_ebo);

		const char* const glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* const glsl_fs =
			NL "#define KSIZE 11" NL "layout(r32i, binding = 0) coherent uniform iimage2D g_image0;" NL
			   "layout(r32ui, binding = 1) coherent uniform uimage2D g_image1;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL "  imageAtomicAdd(g_image0, coord, 2);" NL
			   "  imageAtomicAdd(g_image0, coord, -1);" NL "  imageAtomicAdd(g_image1, coord, 1u);" NL
			   "  imageAtomicAdd(g_image1, coord, 2u);" NL "}";
		m_program = BuildProgram(glsl_vs, glsl_fs, false, true);

		const char* const glsl_cs = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(r32i, binding = 0) uniform iimage2D gi_image;" NL
			"layout(r32ui, binding = 1) uniform uimage2D gu_image;" NL "layout(std430) buffer out_data {" NL
			"  ivec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex].yx = imageLoad(gi_image, coord).xy;" NL
			"  data[gl_LocalInvocationIndex].wz = ivec2(imageLoad(gu_image, coord).xz);" NL "}";
		c_program = CreateComputeProgram(glsl_cs);

		std::vector<GLubyte> data(kSize * kSize * 4 * 4);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glBindVertexArray(m_vao);
		glViewport(0, 0, kSize, kSize);
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		ivec4* map_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, ivec4(0, 1, 0, 3)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, getWindowWidth(), getWindowHeight());
		glUseProgram(0);
		glDeleteBuffers(1, &m_vbo);
		glDeleteBuffers(1, &m_ebo);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteProgram(c_program);
		glDeleteVertexArrays(1, &m_vao);
		glActiveTexture(GL_TEXTURE0);
		return NO_ERROR;
	}
};

class AdvancedCastCS : public ShaderImageLoadStoreBase
{
	GLuint m_texture[2];
	GLuint m_program;
	GLuint c_program;
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(2, m_texture);
		glGenBuffers(1, &m_buffer);
		m_program = 0;
		c_program = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		const int kSize = 11;
		if (!IsImageAtomicSupported())
			return NO_ERROR;

		const char* const glsl_cs =
			NL "#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			   "layout(r32i, binding = 0) coherent uniform iimage2D g_image0;" NL
			   "layout(r32ui, binding = 1) coherent uniform uimage2D g_image1;" NL "void main() {" NL
			   "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL "  imageAtomicAdd(g_image0, coord, 222);" NL
			   "  imageAtomicAdd(g_image0, coord, -11);" NL "  imageAtomicAdd(g_image1, coord, 1u);" NL
			   "  imageAtomicAdd(g_image1, coord, 2u);" NL "}";
		m_program = CreateComputeProgram(glsl_cs, true);

		const char* const glsl_cs_c = NL
			"#define KSIZE 11" NL "layout (local_size_x = KSIZE, local_size_y = KSIZE) in;" NL
			"layout(r32i, binding = 0) uniform iimage2D gi_image;" NL
			"layout(r32ui, binding = 1) uniform uimage2D gu_image;" NL "layout(std430) buffer out_data {" NL
			"  ivec4 data[KSIZE*KSIZE];" NL "};" NL "void main() {" NL "  ivec2 coord = ivec2(gl_LocalInvocationID);" NL
			"  data[gl_LocalInvocationIndex].yz = imageLoad(gi_image, coord).xw;" NL
			"  data[gl_LocalInvocationIndex].wx = ivec2(imageLoad(gu_image, coord).xy);" NL "}";
		c_program = CreateComputeProgram(glsl_cs_c);

		std::vector<GLubyte> data(kSize * kSize * 4 * 4);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, m_texture[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, m_texture[1]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kSize, kSize);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindImageTexture(0, m_texture[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
		glBindImageTexture(1, m_texture[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, kSize * kSize * 4 * 4, &data[0], GL_STATIC_DRAW);

		glUseProgram(m_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(c_program);
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		ivec4* map_data = (ivec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kSize * kSize * 4 * 4, GL_MAP_READ_BIT);

		if (!CompareValues(map_data, kSize, ivec4(0, 211, 1, 3)))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glUseProgram(0);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTextures(2, m_texture);
		glDeleteProgram(m_program);
		glDeleteProgram(c_program);
		glActiveTexture(GL_TEXTURE0);
		return NO_ERROR;
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
		const char* glsl_vs =
			NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL "}";
		const char* glsl_fs = NL "layout(rgba32f) writeonly uniform image2D g_image;" NL "void main() {" NL
								 "  ivec2 coord = ivec2(gl_FragCoord.xy);" NL
								 "  imageStore(g_image, coord, vec4(0.0));" NL "  discard;" NL "}";
		m_program = BuildProgram(glsl_vs, glsl_fs);

		GLint max_image_units;
		glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_units);
		glUseProgram(m_program);
		bool  status = true;
		GLint i		 = 1;
		glUniform1i(glGetUniformLocation(m_program, "g_image"), 1);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glUniform1iv(glGetUniformLocation(m_program, "g_image"), 1, &i);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glUniform1ui(glGetUniformLocation(m_program, "g_image"), 0);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glUniform2i(glGetUniformLocation(m_program, "g_image"), 0, 0);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;

		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glUniform* should generate INVALID_OPERATION "
				<< "if the location refers to an image variable." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glUseProgram(0);
		glProgramUniform1i(m_program, glGetUniformLocation(m_program, "g_image"), 1);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glProgramUniform1iv(m_program, glGetUniformLocation(m_program, "g_image"), 1, &i);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glProgramUniform1ui(m_program, glGetUniformLocation(m_program, "g_image"), 0);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;
		glProgramUniform2i(m_program, glGetUniformLocation(m_program, "g_image"), 0, 0);
		if (glGetError() != GL_INVALID_OPERATION)
			status = false;

		if (!status)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glProgramUniform* should generate INVALID_OPERATION "
				<< "if the location refers to an image variable." << tcu::TestLog::EndMessage;
			return ERROR;
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
	GLuint m_texture, m_texture2;

	virtual long Setup()
	{
		m_texture  = 0;
		m_texture2 = 0;
		return NO_ERROR;
	}

	virtual long Run()
	{
		GLint max_image_units;
		glGetIntegerv(GL_MAX_IMAGE_UNITS, &max_image_units);
		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 64, 64);

		glBindImageTexture(max_image_units, m_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <unit> "
				<< "is greater than or equal to the value of MAX_IMAGE_UNITS." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(0, 123, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <texture> "
				<< "is not the name of an existing texture object." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(1, m_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <format> "
				<< "is not a legal format." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(1, m_texture, -1, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <level> "
				<< "is less than zero." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glBindImageTexture(1, m_texture, 0, GL_FALSE, -1, GL_READ_ONLY, GL_RGBA32F);
		if (glGetError() != GL_INVALID_VALUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <layer> "
				<< "is less than zero." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		glGenTextures(1, &m_texture2);
		glBindTexture(GL_TEXTURE_2D, m_texture2);
		glBindImageTexture(1, m_texture2, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		if (glGetError() != GL_INVALID_OPERATION)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "BindImageTexture should generate INVALID_VALUE if <texture> "
				<< "is a mutable texture object." << tcu::TestLog::EndMessage;
			return ERROR;
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteTextures(1, &m_texture);
		glDeleteTextures(1, &m_texture2);
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
		if (!Compile( // writeonly & readonly qualifiers
				NL "layout(rgba32f) writeonly readonly uniform image2D g_image;" NL "void main() {" NL
				   "  vec4 o_color;" NL "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile( // writeonly && reading
				NL "layout(rgba32f) writeonly uniform image2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile( //readonly && writing
				NL "uniform vec4 i_color;" NL "layout(rgba32f) readonly uniform image2D g_image;" NL "void main() {" NL
				   "  vec4 o_color;" NL "  imageStore(g_image, ivec2(0), i_color);" NL "  o_color = i_color;" NL "}"))
			return ERROR;

		if (!Compile( // no format layout && load
				NL "uniform image2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile( // no fromat layout && readonly && load
				NL "readonly uniform image2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = imageLoad(g_image, ivec2(0));" NL "}"))
			return ERROR;

		if (!Compile( // target type image1D not supported
				NL "layout(r32i) uniform image1D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // format layout not compatible with type
				NL "layout(rgba16) writeonly uniform iimage2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // imageAtomicAdd doesn't support r32f
				NL "#extension GL_OES_shader_image_atomic : require" NL
				   "layout(r32f) coherent uniform image2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  imageAtomicAdd(g_image, ivec2(1), 10);" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // imageAtomicAdd doesn't support rgba8i
				NL "#extension GL_OES_shader_image_atomic : require" NL
				   "layout(rgba8i) coherent uniform iimage2D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  imageAtomicAdd(g_image, ivec2(1), 1);" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // format layout not compatible with type
				NL "layout(r32ui) uniform iimage3D g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  imageStore(g_image, ivec3(1), ivec4(1));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // format layout not compatible with type
				NL "layout(r32f) uniform uimage2DArray g_image;" NL "void main() {" NL "  vec4 o_color;" NL
				   "  imageStore(g_image, ivec3(0), uvec4(1));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Compile( // wrong function argument type
				NL "layout(r32f) coherent uniform image2D g_image;" NL "vec4 Load(iimage2D image) {" NL
				   "  return imageLoad(image, vec2(0));" NL "}" NL "void main() {" NL "  vec4 o_color;" NL
				   "  o_color = Load(g_image);" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Compile(const std::string& source)
	{
		const char* const csVer  = "#version 310 es" NL "layout(local_size_x = 1) in;";
		const char* const src[3] = { csVer, kGLSLPrec, source.c_str() };
		const GLuint	  sh	 = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(sh, 3, src, NULL);
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
				<< tcu::TestLog::Message << "Compilation should fail [compute shader]." << tcu::TestLog::EndMessage;
			return false;
		}
		const char* const fsVer   = "#version 310 es" NL "precision highp float;";
		const char* const fsrc[3] = { fsVer, kGLSLPrec, source.c_str() };
		const GLuint	  fsh	 = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fsh, 3, fsrc, NULL);
		glCompileShader(fsh);

		glGetShaderInfoLog(fsh, sizeof(log), NULL, log);
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader Info Log:\n"
											<< log << tcu::TestLog::EndMessage;
		glGetShaderiv(fsh, GL_COMPILE_STATUS, &status);
		glDeleteShader(fsh);

		if (status == GL_TRUE)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Compilation should fail [fragment shader]." << tcu::TestLog::EndMessage;
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
		if (!IsVSFSAvailable(1, 1))
			return NOT_SUPPORTED;
		if (!Link(NL "layout(location = 0) in vec4 i_position;" NL
					 "layout(rgba32f) writeonly uniform highp image3D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec3(gl_VertexID), vec4(0));" NL "  gl_Position = i_position;" NL "}",

				  NL "precision highp float;" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(rgba32f) writeonly uniform highp image2D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec2(gl_FragCoord), vec4(1.0));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		if (!Link(NL "layout(location = 0) in vec4 i_position;" NL
					 "layout(rgba32f) writeonly uniform highp image2D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec2(gl_VertexID), vec4(0));" NL "  gl_Position = i_position;" NL "}",

				  NL "precision highp float;" NL "layout(location = 0) out vec4 o_color;" NL
					 "layout(r32f) writeonly uniform highp image2D g_image;" NL "void main() {" NL
					 "  imageStore(g_image, ivec2(gl_FragCoord), vec4(1.0));" NL "  o_color = vec4(1.0);" NL "}"))
			return ERROR;

		return NO_ERROR;
	}

	bool Link(const std::string& vs, const std::string& fs)
	{
		const char* const sVer = "#version 310 es";
		const GLuint	  p	= glCreateProgram();

		const GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
		glAttachShader(p, vsh);
		glDeleteShader(vsh);
		const char* const vssrc[2] = { sVer, vs.c_str() };
		glShaderSource(vsh, 2, vssrc, NULL);
		glCompileShader(vsh);

		const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
		glAttachShader(p, fsh);
		glDeleteShader(fsh);
		const char* const fssrc[2] = { sVer, fs.c_str() };
		glShaderSource(fsh, 2, fssrc, NULL);
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

} // anonymous namespace

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
	addChild(new TestSubcase(m_context, "basic-api-barrier-byRegion", TestSubcase::Create<BasicAPIBarrierByRegion>));
	addChild(new TestSubcase(m_context, "basic-api-texParam", TestSubcase::Create<BasicAPITexParam>));
	addChild(new TestSubcase(m_context, "basic-allFormats-store-fs", TestSubcase::Create<BasicAllFormatsStoreFS>));
	addChild(new TestSubcase(m_context, "basic-allFormats-store-cs", TestSubcase::Create<BasicAllFormatsStoreCS>));
	addChild(new TestSubcase(m_context, "basic-allFormats-load-fs", TestSubcase::Create<BasicAllFormatsLoadFS>));
	addChild(new TestSubcase(m_context, "basic-allFormats-load-cs", TestSubcase::Create<BasicAllFormatsLoadCS>));
	addChild(new TestSubcase(m_context, "basic-allFormats-loadStoreComputeStage",
							 TestSubcase::Create<BasicAllFormatsLoadStoreComputeStage>));
	addChild(new TestSubcase(m_context, "basic-allTargets-store-fs", TestSubcase::Create<BasicAllTargetsStoreFS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-store-cs", TestSubcase::Create<BasicAllTargetsStoreCS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-load-fs", TestSubcase::Create<BasicAllTargetsLoadFS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-load-cs", TestSubcase::Create<BasicAllTargetsLoadCS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicFS", TestSubcase::Create<BasicAllTargetsAtomicFS>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreVS", TestSubcase::Create<BasicAllTargetsLoadStoreVS>));
	addChild(
		new TestSubcase(m_context, "basic-allTargets-loadStoreCS", TestSubcase::Create<BasicAllTargetsLoadStoreCS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicVS", TestSubcase::Create<BasicAllTargetsAtomicVS>));
	addChild(new TestSubcase(m_context, "basic-allTargets-atomicCS", TestSubcase::Create<BasicAllTargetsAtomicCS>));
	addChild(new TestSubcase(m_context, "basic-glsl-misc-fs", TestSubcase::Create<BasicGLSLMiscFS>));
	addChild(new TestSubcase(m_context, "basic-glsl-misc-cs", TestSubcase::Create<BasicGLSLMiscCS>));
	addChild(new TestSubcase(m_context, "basic-glsl-earlyFragTests", TestSubcase::Create<BasicGLSLEarlyFragTests>));
	addChild(new TestSubcase(m_context, "basic-glsl-const", TestSubcase::Create<BasicGLSLConst>));
	addChild(new TestSubcase(m_context, "advanced-sync-imageAccess", TestSubcase::Create<AdvancedSyncImageAccess>));
	addChild(new TestSubcase(m_context, "advanced-sync-vertexArray", TestSubcase::Create<AdvancedSyncVertexArray>));
	addChild(new TestSubcase(m_context, "advanced-sync-imageAccess2", TestSubcase::Create<AdvancedSyncImageAccess2>));
	addChild(new TestSubcase(m_context, "advanced-allStages-oneImage", TestSubcase::Create<AdvancedAllStagesOneImage>));
	addChild(new TestSubcase(m_context, "advanced-memory-order-vsfs", TestSubcase::Create<AdvancedMemoryOrderVSFS>));
	addChild(new TestSubcase(m_context, "advanced-sso-simple", TestSubcase::Create<AdvancedSSOSimple>));
	addChild(new TestSubcase(m_context, "advanced-copyImage-fs", TestSubcase::Create<AdvancedCopyImageFS>));
	addChild(new TestSubcase(m_context, "advanced-copyImage-cs", TestSubcase::Create<AdvancedCopyImageCS>));
	addChild(new TestSubcase(m_context, "advanced-allMips-fs", TestSubcase::Create<AdvancedAllMipsFS>));
	addChild(new TestSubcase(m_context, "advanced-allMips-cs", TestSubcase::Create<AdvancedAllMipsCS>));
	addChild(new TestSubcase(m_context, "advanced-cast-fs", TestSubcase::Create<AdvancedCastFS>));
	addChild(new TestSubcase(m_context, "advanced-cast-cs", TestSubcase::Create<AdvancedCastCS>));
	addChild(new TestSubcase(m_context, "negative-uniform", TestSubcase::Create<NegativeUniform>));
	addChild(new TestSubcase(m_context, "negative-bind", TestSubcase::Create<NegativeBind>));
	addChild(new TestSubcase(m_context, "negative-compileErrors", TestSubcase::Create<NegativeCompileErrors>));
	addChild(new TestSubcase(m_context, "negative-linkErrors", TestSubcase::Create<NegativeLinkErrors>));
}

} // namespace es31compatibility
} // namespace gl4cts
