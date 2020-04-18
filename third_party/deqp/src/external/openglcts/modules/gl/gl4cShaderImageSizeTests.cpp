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

#include "gl4cShaderImageSizeTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include <assert.h>
#include <cstdarg>

namespace gl4cts
{
using namespace glw;

namespace
{
typedef tcu::Vec3  vec3;
typedef tcu::Vec4  vec4;
typedef tcu::IVec4 ivec4;
typedef tcu::UVec4 uvec4;

class ShaderImageSizeBase : public deqp::SubcaseBase
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

	bool CheckProgram(GLuint program)
	{
		if (program == 0)
			return true;
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
};

template <typename T>
std::string ImageTypePrefix();

template <>
std::string ImageTypePrefix<vec4>()
{
	return "";
}

template <>
std::string ImageTypePrefix<ivec4>()
{
	return "i";
}

template <>
std::string ImageTypePrefix<uvec4>()
{
	return "u";
}

template <typename T>
std::string ImageFormatPostfix();

template <>
std::string ImageFormatPostfix<vec4>()
{
	return "f";
}

template <>
std::string ImageFormatPostfix<ivec4>()
{
	return "i";
}

template <>
std::string ImageFormatPostfix<uvec4>()
{
	return "ui";
}

template <typename T>
GLenum TexInternalFormat();

template <>
GLenum TexInternalFormat<vec4>()
{
	return GL_RGBA32F;
}

template <>
GLenum TexInternalFormat<ivec4>()
{
	return GL_RGBA32I;
}

template <>
GLenum TexInternalFormat<uvec4>()
{
	return GL_RGBA32UI;
}

template <typename T>
GLenum TexType();

template <>
GLenum TexType<vec4>()
{
	return GL_FLOAT;
}

template <>
GLenum TexType<ivec4>()
{
	return GL_INT;
}

template <>
GLenum TexType<uvec4>()
{
	return GL_UNSIGNED_INT;
}

template <typename T>
GLenum TexFormat();

template <>
GLenum TexFormat<vec4>()
{
	return GL_RGBA;
}

template <>
GLenum TexFormat<ivec4>()
{
	return GL_RGBA_INTEGER;
}

template <>
GLenum TexFormat<uvec4>()
{
	return GL_RGBA_INTEGER;
}
//=============================================================================
// ImageSizeMachine
//-----------------------------------------------------------------------------
class ImageSizeMachine : public deqp::GLWrapper
{
	GLuint m_pipeline;
	GLuint m_program[3];
	GLuint m_vertex_array;
	GLuint m_texture;

	template <typename T>
	std::string GenShader(int stage, bool ms_and_1d, bool subroutine)
	{
		std::ostringstream os;
		os << "#version 430 core";
		if (stage == 4)
		{ // CS
			os << NL "#extension GL_ARB_compute_shader : require";
		}
		os << NL "layout(binding = 0, rgba32i) writeonly uniform iimage2D g_result;";
		if (ms_and_1d == false)
		{
			os << NL "layout(binding = 1, rgba32" << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>()
			   << "image2D g_image_2d;" NL "layout(binding = 2, rgba32" << ImageFormatPostfix<T>() << ") uniform "
			   << ImageTypePrefix<T>() << "image3D g_image_3d;" NL "layout(binding = 3, rgba32"
			   << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>()
			   << "imageCube g_image_cube;" NL "layout(binding = 4, rgba32" << ImageFormatPostfix<T>() << ") uniform "
			   << ImageTypePrefix<T>() << "imageCubeArray g_image_cube_array;" NL "layout(binding = 5, rgba32"
			   << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>()
			   << "image2DRect g_image_rect;" NL "layout(binding = 6, rgba32" << ImageFormatPostfix<T>() << ") uniform "
			   << ImageTypePrefix<T>() << "image2DArray g_image_2d_array;" NL "layout(binding = 7, rgba32"
			   << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>() << "imageBuffer g_image_buffer;";
		}
		else
		{
			os << NL "layout(binding = 1, rgba32" << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>()
			   << "image1D g_image_1d;" NL "layout(binding = 2, rgba32" << ImageFormatPostfix<T>() << ") uniform "
			   << ImageTypePrefix<T>() << "image1DArray g_image_1d_array;" NL "layout(binding = 3, rgba32"
			   << ImageFormatPostfix<T>() << ") uniform " << ImageTypePrefix<T>()
			   << "image2DMS g_image_2dms;" NL "layout(binding = 4, rgba32" << ImageFormatPostfix<T>() << ") uniform "
			   << ImageTypePrefix<T>() << "image2DMSArray g_image_2dms_array;";
		}
		if (subroutine)
		{
			os << NL "subroutine void FuncType(int coord);" NL "subroutine uniform FuncType g_func;";
		}
		if (stage == 0)
		{ // VS
			os << NL "void main() {" NL "  int coord = gl_VertexID;";
		}
		else if (stage == 1)
		{ // TCS
			os << NL "layout(vertices = 1) out;" NL "void main() {" NL "  gl_TessLevelInner[0] = 1;" NL
					 "  gl_TessLevelInner[1] = 1;" NL "  gl_TessLevelOuter[0] = 1;" NL "  gl_TessLevelOuter[1] = 1;" NL
					 "  gl_TessLevelOuter[2] = 1;" NL "  gl_TessLevelOuter[3] = 1;" NL "  int coord = gl_PrimitiveID;";
		}
		else if (stage == 2)
		{ // TES
			os << NL "layout(quads, point_mode) in;" NL "void main() {" NL "  int coord = gl_PrimitiveID;";
		}
		else if (stage == 3)
		{ // GS
			os << NL "layout(points) in;" NL "layout(points, max_vertices = 1) out;" NL "void main() {" NL
					 "  int coord = gl_PrimitiveIDIn;";
		}
		else if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = 1) in;" NL "void main() {" NL "  int coord = int(gl_GlobalInvocationID.x);";
		}
		else if (stage == 5)
		{ // FS
			os << NL "void main() {" NL "  int coord = gl_PrimitiveID;";
		}
		if (subroutine)
		{
			os << NL "  g_func(coord);" NL "}" NL "subroutine(FuncType) void Func0(int coord) {";
		}
		if (ms_and_1d == false)
		{
			os << NL "  imageStore(g_result, ivec2(coord, 0), ivec4(imageSize(g_image_2d), 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 1), ivec4(imageSize(g_image_3d), 0));" NL
					 "  imageStore(g_result, ivec2(coord, 2), ivec4(imageSize(g_image_cube), 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 3), ivec4(imageSize(g_image_cube_array), 0));" NL
					 "  imageStore(g_result, ivec2(coord, 4), ivec4(imageSize(g_image_rect), 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 5), ivec4(imageSize(g_image_2d_array), 0));" NL
					 "  imageStore(g_result, ivec2(coord, 6), ivec4(imageSize(g_image_buffer), 0, 0, 0));" NL "}";
		}
		else
		{
			os << NL "  imageStore(g_result, ivec2(coord, 0), ivec4(imageSize(g_image_1d), 0, 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 1), ivec4(imageSize(g_image_1d_array), 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 2), ivec4(imageSize(g_image_2dms), 0, 0));" NL
					 "  imageStore(g_result, ivec2(coord, 3), ivec4(imageSize(g_image_2dms_array), 0));" NL
					 "  imageStore(g_result, ivec2(coord, 4), ivec4(0));" NL
					 "  imageStore(g_result, ivec2(coord, 5), ivec4(0));" NL
					 "  imageStore(g_result, ivec2(coord, 6), ivec4(0));" NL "}";
		}
		return os.str();
	}

	bool CheckProgram(GLuint program)
	{
		if (program == 0)
			return true;
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

public:
	ImageSizeMachine()
	{
		glGenProgramPipelines(1, &m_pipeline);
		memset(m_program, 0, sizeof(m_program));
		glGenVertexArrays(1, &m_vertex_array);
		glGenTextures(1, &m_texture);
	}

	~ImageSizeMachine()
	{
		glDeleteProgramPipelines(1, &m_pipeline);
		for (int i = 0; i < 3; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteTextures(1, &m_texture);
	}

	template <typename T>
	long Run(int stage, bool ms_and_1d, ivec4 expected_result[7], bool subroutine = false)
	{
		if (stage == 0)
		{ // VS
			std::string		  vs	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_vs = vs.c_str();
			m_program[0]			  = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
			glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
		}
		else if (stage == 1)
		{ // TCS
			const char* const glsl_vs = "#version 430 core" NL "out gl_PerVertex { vec4 gl_Position; };" NL
										"void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0);}";
			const char* const glsl_tes = "#version 430 core" NL "layout(quads, point_mode) in;" NL "void main() {}";
			std::string		  tcs	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_tcs = tcs.c_str();
			m_program[0]			   = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
			m_program[1]			   = glCreateShaderProgramv(GL_TESS_CONTROL_SHADER, 1, &glsl_tcs);
			m_program[2]			   = glCreateShaderProgramv(GL_TESS_EVALUATION_SHADER, 1, &glsl_tes);
			glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
			glUseProgramStages(m_pipeline, GL_TESS_CONTROL_SHADER_BIT, m_program[1]);
			glUseProgramStages(m_pipeline, GL_TESS_EVALUATION_SHADER_BIT, m_program[2]);
		}
		else if (stage == 2)
		{ // TES
			const char* const glsl_vs = "#version 430 core" NL "out gl_PerVertex { vec4 gl_Position; };" NL
										"void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0);}";
			std::string		  tes	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_tes = tes.c_str();
			m_program[0]			   = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
			m_program[1]			   = glCreateShaderProgramv(GL_TESS_EVALUATION_SHADER, 1, &glsl_tes);
			glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
			glUseProgramStages(m_pipeline, GL_TESS_EVALUATION_SHADER_BIT, m_program[1]);
		}
		else if (stage == 3)
		{ // GS
			const char* const glsl_vs = "#version 430 core" NL "out gl_PerVertex { vec4 gl_Position; };" NL
										"void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0);}";
			std::string		  gs	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_gs = gs.c_str();
			m_program[0]			  = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
			m_program[1]			  = glCreateShaderProgramv(GL_GEOMETRY_SHADER, 1, &glsl_gs);
			glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
			glUseProgramStages(m_pipeline, GL_GEOMETRY_SHADER_BIT, m_program[1]);
		}
		else if (stage == 4)
		{ // CS
			std::string		  cs	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_cs = cs.c_str();
			m_program[0]			  = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &glsl_cs);
			glUseProgramStages(m_pipeline, GL_COMPUTE_SHADER_BIT, m_program[0]);
		}
		else if (stage == 5)
		{ // FS
			const char* const glsl_vs = "#version 430 core" NL "out gl_PerVertex { vec4 gl_Position; };" NL
										"void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0);}";
			std::string		  fs	  = GenShader<T>(stage, ms_and_1d, subroutine);
			const char* const glsl_fs = fs.c_str();
			m_program[0]			  = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
			m_program[1]			  = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
			glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
			glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_program[1]);
		}
		for (int i = 0; i < 3; ++i)
		{
			if (!CheckProgram(m_program[i]))
				return ERROR;
		}

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		{
			ivec4 data[7];
			for (int i  = 0; i < 7; ++i)
				data[i] = ivec4(100000);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32I, 1, 7, 0, GL_RGBA_INTEGER, GL_INT, &data[0]);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32I);
		glBindProgramPipeline(m_pipeline);
		glBindVertexArray(m_vertex_array);
		if (stage != 5)
		{
			glEnable(GL_RASTERIZER_DISCARD);
		}
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
		glDisable(GL_RASTERIZER_DISCARD);

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
		{
			ivec4 data[7];
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_INT, &data[0]);
			for (int i = 0; i < 7; ++i)
			{
				if (data[i] != expected_result[i])
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Returned value is: (" << data[i][0] << " " << data[i][1] << " "
						<< data[i][2] << " " << data[i][3] << "). Expected value is: (" << expected_result[i][0] << " "
						<< expected_result[i][1] << " " << expected_result[i][2] << " " << expected_result[i][3]
						<< "). Image unit is: " << (i + 1) << tcu::TestLog::EndMessage;
					return ERROR;
				}
			}
		}
		return NO_ERROR;
	}
};
//=============================================================================
// 1.1.x.y BasicNonMS
//-----------------------------------------------------------------------------

template <typename T, int STAGE>
class BasicNonMS : public ShaderImageSizeBase
{
	GLuint m_texture[7];
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(7, m_texture);
		glGenBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInStage(STAGE, 8))
			return NOT_SUPPORTED;

		const GLenum target[7] = { GL_TEXTURE_2D,		 GL_TEXTURE_3D,
								   GL_TEXTURE_CUBE_MAP,  GL_TEXTURE_CUBE_MAP_ARRAY,
								   GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_ARRAY,
								   GL_TEXTURE_BUFFER };
		for (int i = 0; i < 7; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			if (target[i] != GL_TEXTURE_BUFFER)
			{
				glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			if (i == 0)
			{
				glTexStorage2D(target[i], 10, TexInternalFormat<T>(), 512, 128);
				glBindImageTexture(1, m_texture[i], 1, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexStorage3D(target[i], 3, TexInternalFormat<T>(), 8, 8, 4);
				glBindImageTexture(2, m_texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexStorage2D(target[i], 4, TexInternalFormat<T>(), 16, 16);
				glBindImageTexture(3, m_texture[i], 0, GL_FALSE, 0, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexStorage3D(target[i], 2, TexInternalFormat<T>(), 4, 4, 12);
				glBindImageTexture(4, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 4)
			{
				glTexStorage2D(target[i], 1, TexInternalFormat<T>(), 16, 8);
				glBindImageTexture(5, m_texture[i], 0, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 5)
			{
				glTexStorage3D(target[i], 3, TexInternalFormat<T>(), 127, 39, 12);
				glBindImageTexture(6, m_texture[i], 2, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 6)
			{
				std::vector<GLubyte> data(256);
				glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
				glBufferData(GL_TEXTURE_BUFFER, 256, &data[0], GL_STATIC_DRAW);
				glTexBuffer(GL_TEXTURE_BUFFER, TexInternalFormat<T>(), m_buffer);
				glBindImageTexture(7, m_texture[i], 0, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4			 res[7] = { ivec4(256, 64, 0, 0), ivec4(8, 8, 4, 0),   ivec4(16, 16, 0, 0), ivec4(2, 2, 2, 0),
						 ivec4(16, 8, 0, 0),   ivec4(31, 9, 12, 0), ivec4(16, 0, 0, 0) };
		return machine.Run<T>(STAGE, false, res);
	}

	virtual long Cleanup()
	{
		glDeleteTextures(7, m_texture);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};
//=============================================================================
// 1.2.x.y BasicMS
//-----------------------------------------------------------------------------

template <typename T, int STAGE>
class BasicMS : public ShaderImageSizeBase
{
	GLuint m_texture[4];

	virtual long Setup()
	{
		glGenTextures(4, m_texture);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInStage(STAGE, 5))
			return NOT_SUPPORTED;
		if (!SupportedSamples(4))
			return NOT_SUPPORTED;

		const GLenum target[4] = { GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_MULTISAMPLE,
								   GL_TEXTURE_2D_MULTISAMPLE_ARRAY };
		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			if (target[i] == GL_TEXTURE_1D || target[i] == GL_TEXTURE_1D_ARRAY)
			{
				glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			if (i == 0)
			{
				glTexStorage1D(target[i], 10, TexInternalFormat<T>(), 512);
				glBindImageTexture(1, m_texture[i], 6, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexStorage2D(target[i], 3, TexInternalFormat<T>(), 15, 7);
				glBindImageTexture(2, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexImage2DMultisample(target[i], 4, TexInternalFormat<T>(), 17, 19, GL_FALSE);
				glBindImageTexture(3, m_texture[i], 0, GL_FALSE, 0, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexImage3DMultisample(target[i], 4, TexInternalFormat<T>(), 64, 32, 5, GL_FALSE);
				glBindImageTexture(4, m_texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4 res[7] = { ivec4(8, 0, 0, 0), ivec4(7, 7, 0, 0), ivec4(17, 19, 0, 0), ivec4(64, 32, 5, 0), ivec4(0),
						 ivec4(0),			ivec4(0) };
		return machine.Run<T>(STAGE, true, res);
	}

	virtual long Cleanup()
	{
		glDeleteTextures(4, m_texture);
		return NO_ERROR;
	}
};
//=============================================================================
// 2.1 AdvancedChangeSize
//-----------------------------------------------------------------------------
class AdvancedChangeSize : public ShaderImageSizeBase
{
	GLuint m_pipeline;
	GLuint m_program[2];
	GLuint m_vertex_array;
	GLuint m_texture[2];

	virtual long Setup()
	{
		glGenProgramPipelines(1, &m_pipeline);
		memset(m_program, 0, sizeof(m_program));
		glGenVertexArrays(1, &m_vertex_array);
		glGenTextures(2, m_texture);
		return NO_ERROR;
	}

	virtual long Run()
	{
		const char* const glsl_vs = "#version 430 core" NL "out gl_PerVertex { vec4 gl_Position; };" NL
									"const vec2 g_position[3] = { vec2(-1, -1), vec2(3, -1), vec2(-1, 3) };" NL
									"void main() { gl_Position = vec4(g_position[gl_VertexID], 0, 1); }";
		const char* const glsl_fs =
			"#version 430 core" NL "layout(location = 0) out vec4 g_color;" NL
			"layout(binding = 0, rgba8) uniform image2D g_image[2];" NL "uniform ivec2 g_expected_size[2];" NL
			"uniform int g_0 = 0, g_1 = 1;" NL "void main() {" NL "  vec4 c = vec4(0, 1, 0, 1);" NL
			"  if (imageSize(g_image[g_0]).xy != g_expected_size[g_0]) c = vec4(1, 0, 0, 1);" NL
			"  if (imageSize(g_image[g_1]).yx != g_expected_size[g_1]) c = vec4(1, 0, 0, 1);" NL "  g_color = c;" NL
			"}";
		m_program[0] = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
		m_program[1] = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
		for (int i = 0; i < 2; ++i)
			if (!CheckProgram(m_program[i]))
				return ERROR;

		glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
		glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_program[1]);

		glBindVertexArray(m_vertex_array);
		glBindProgramPipeline(m_pipeline);

		int size[2] = { 32, 128 };
		for (int i = 0; i < 2; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, m_texture[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[i], size[i], 0, GL_RGBA, GL_FLOAT, NULL);
			glBindImageTexture(i, m_texture[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
		}

		for (int i = 0; i < 3; ++i)
		{
			glProgramUniform2i(m_program[1], glGetUniformLocation(m_program[1], "g_expected_size[0]"), size[0],
							   size[0]);
			glProgramUniform2i(m_program[1], glGetUniformLocation(m_program[1], "g_expected_size[1]"), size[1],
							   size[1]);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			{
				bool			  status = true;
				std::vector<vec3> fb(getWindowWidth() * getWindowHeight());
				glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGB, GL_FLOAT, &fb[0][0]);
				if (!ValidateReadBuffer(0, 0, getWindowWidth(), getWindowHeight(), vec4(0, 1, 0, 1)))
					status = false;
				if (!status)
					return ERROR;
			}

			size[0] /= 2;
			size[1] /= 2;

			glBindTexture(GL_TEXTURE_2D, m_texture[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[0], size[0], 0, GL_RGBA, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, m_texture[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size[1], size[1], 0, GL_RGBA, GL_FLOAT, NULL);
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgramPipelines(1, &m_pipeline);
		for (int i = 0; i < 2; ++i)
			glDeleteProgram(m_program[i]);
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteTextures(2, m_texture);
		return NO_ERROR;
	}
};
//=============================================================================
// 2.2.x.y AdvancedNonMS
//-----------------------------------------------------------------------------

template <typename T, int STAGE>
class AdvancedNonMS : public ShaderImageSizeBase
{
	GLuint m_texture[7];
	GLuint m_buffer;

	virtual long Setup()
	{
		glGenTextures(7, m_texture);
		glGenBuffers(1, &m_buffer);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInStage(STAGE, 8))
			return NOT_SUPPORTED;

		const GLenum target[7] = { GL_TEXTURE_2D_ARRAY,		  GL_TEXTURE_3D,		GL_TEXTURE_CUBE_MAP_ARRAY,
								   GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_ARRAY,
								   GL_TEXTURE_BUFFER };
		for (int i = 0; i < 7; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			if (target[i] != GL_TEXTURE_BUFFER)
			{
				glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			if (i == 0)
			{
				glTexImage3D(target[i], 0, TexInternalFormat<T>(), 2, 2, 7, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 1, TexInternalFormat<T>(), 1, 1, 7, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(1, m_texture[i], 1, GL_FALSE, 3, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexImage3D(target[i], 0, TexInternalFormat<T>(), 4, 4, 2, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 1, TexInternalFormat<T>(), 2, 2, 1, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 2, TexInternalFormat<T>(), 1, 1, 1, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(2, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexImage3D(target[i], 0, TexInternalFormat<T>(), 2, 2, 12, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 1, TexInternalFormat<T>(), 1, 1, 12, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(3, m_texture[i], 0, GL_FALSE, 1, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexImage3D(target[i], 0, TexInternalFormat<T>(), 4, 4, 18, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 1, TexInternalFormat<T>(), 2, 2, 18, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 2, TexInternalFormat<T>(), 1, 1, 18, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(4, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 4)
			{
				glTexImage2D(target[i], 0, TexInternalFormat<T>(), 123, 11, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(5, m_texture[i], 0, GL_FALSE, 0, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 5)
			{
				glTexImage3D(target[i], 0, TexInternalFormat<T>(), 13, 7, 4, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 1, TexInternalFormat<T>(), 6, 3, 4, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 2, TexInternalFormat<T>(), 3, 1, 4, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage3D(target[i], 3, TexInternalFormat<T>(), 1, 1, 4, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(6, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 6)
			{
				glBindBuffer(GL_TEXTURE_BUFFER, m_buffer);
				glBufferData(GL_TEXTURE_BUFFER, 1024, NULL, GL_STATIC_DRAW);
				glTexBufferRange(GL_TEXTURE_BUFFER, TexInternalFormat<T>(), m_buffer, 256, 512);
				glBindImageTexture(7, m_texture[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4			 res[7] = { ivec4(1, 1, 0, 0),	ivec4(2, 2, 1, 0), ivec4(2, 2, 0, 0), ivec4(2, 2, 3, 0),
						 ivec4(123, 11, 0, 0), ivec4(6, 3, 4, 0), ivec4(32, 0, 0, 0) };
		return machine.Run<T>(STAGE, false, res, true);
	}

	virtual long Cleanup()
	{
		glDeleteTextures(7, m_texture);
		glDeleteBuffers(1, &m_buffer);
		return NO_ERROR;
	}
};
//=============================================================================
// 2.3.x.y AdvancedMS
//-----------------------------------------------------------------------------
template <typename T, int STAGE>
class AdvancedMS : public ShaderImageSizeBase
{
	GLuint m_texture[4];

	virtual long Setup()
	{
		glGenTextures(4, m_texture);
		return NO_ERROR;
	}

	virtual long Run()
	{
		if (!SupportedInStage(STAGE, 5))
			return NOT_SUPPORTED;
		if (!SupportedSamples(4))
			return NOT_SUPPORTED;

		const GLenum target[4] = { GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
								   GL_TEXTURE_2D_MULTISAMPLE_ARRAY };
		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			if (target[i] == GL_TEXTURE_1D || target[i] == GL_TEXTURE_1D_ARRAY)
			{
				glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			if (i == 0)
			{
				glTexImage1D(target[i], 0, TexInternalFormat<T>(), 7, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage1D(target[i], 1, TexInternalFormat<T>(), 3, 0, TexFormat<T>(), TexType<T>(), NULL);
				glTexImage1D(target[i], 2, TexInternalFormat<T>(), 1, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(1, m_texture[i], 1, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexImage2D(target[i], 0, TexInternalFormat<T>(), 7, 15, 0, TexFormat<T>(), TexType<T>(), NULL);
				glBindImageTexture(2, m_texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexImage3DMultisample(target[i], 4, TexInternalFormat<T>(), 7, 9, 3, GL_FALSE);
				glBindImageTexture(3, m_texture[i], 0, GL_FALSE, 1, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexImage3DMultisample(target[i], 4, TexInternalFormat<T>(), 64, 32, 5, GL_FALSE);
				glBindImageTexture(4, m_texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4			 res[7] = { ivec4(3, 0, 0, 0), ivec4(7, 15, 0, 0), ivec4(7, 9, 0, 0), ivec4(64, 32, 5, 0),
						 ivec4(0),			ivec4(0),			ivec4(0) };
		return machine.Run<T>(STAGE, true, res, true);
	}

	virtual long Cleanup()
	{
		glDeleteTextures(4, m_texture);
		return NO_ERROR;
	}
};
//=============================================================================
// 4.1 NegativeCompileTime
//-----------------------------------------------------------------------------
class NegativeCompileTime : public ShaderImageSizeBase
{
	virtual long Run()
	{
		// '#extension GL_ARB_shader_image_size : require' is missing
		if (!Compile("#version 420 core" NL "layout(location = 0) out vec4 g_color;" NL
					 "layout(binding = 0, rg16f) uniform image2D g_image;" NL "uniform ivec2 g_expected_size;" NL
					 "void main() {" NL "  if (imageSize(g_image) == g_expected_size) g_color = vec4(0, 1, 0, 1);" NL
					 "  else g_color = vec4(1, 0, 0, 1);" NL "}"))
			return ERROR;
		// imageSize(sampler)
		if (!Compile("#version 430 core" NL "layout(location = 0) out vec4 g_color;" NL
					 "layout(binding = 0) uniform sampler2D g_sampler;" NL "uniform ivec2 g_expected_size;" NL
					 "void main() {" NL "  if (imageSize(g_sampler) == g_expected_size) g_color = vec4(0, 1, 0, 1);" NL
					 "  else g_color = vec4(1, 0, 0, 1);" NL "}"))
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

} // anonymous namespace

ShaderImageSizeTests::ShaderImageSizeTests(deqp::Context& context) : TestCaseGroup(context, "shader_image_size", "")
{
}

ShaderImageSizeTests::~ShaderImageSizeTests(void)
{
}

void ShaderImageSizeTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-float", TestSubcase::Create<BasicNonMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-int", TestSubcase::Create<BasicNonMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-uint", TestSubcase::Create<BasicNonMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tcs-float", TestSubcase::Create<BasicNonMS<vec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tcs-int", TestSubcase::Create<BasicNonMS<ivec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tcs-uint", TestSubcase::Create<BasicNonMS<uvec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tes-float", TestSubcase::Create<BasicNonMS<vec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tes-int", TestSubcase::Create<BasicNonMS<ivec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-tes-uint", TestSubcase::Create<BasicNonMS<uvec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-gs-float", TestSubcase::Create<BasicNonMS<vec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-gs-int", TestSubcase::Create<BasicNonMS<ivec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-gs-uint", TestSubcase::Create<BasicNonMS<uvec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-float", TestSubcase::Create<BasicNonMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-int", TestSubcase::Create<BasicNonMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-uint", TestSubcase::Create<BasicNonMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-float", TestSubcase::Create<BasicNonMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-int", TestSubcase::Create<BasicNonMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-uint", TestSubcase::Create<BasicNonMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-ms-vs-float", TestSubcase::Create<BasicMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-ms-vs-int", TestSubcase::Create<BasicMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-ms-vs-uint", TestSubcase::Create<BasicMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-ms-tcs-float", TestSubcase::Create<BasicMS<vec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-ms-tcs-int", TestSubcase::Create<BasicMS<ivec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-ms-tcs-uint", TestSubcase::Create<BasicMS<uvec4, 1> >));
	addChild(new TestSubcase(m_context, "basic-ms-tes-float", TestSubcase::Create<BasicMS<vec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-ms-tes-int", TestSubcase::Create<BasicMS<ivec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-ms-tes-uint", TestSubcase::Create<BasicMS<uvec4, 2> >));
	addChild(new TestSubcase(m_context, "basic-ms-gs-float", TestSubcase::Create<BasicMS<vec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-ms-gs-int", TestSubcase::Create<BasicMS<ivec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-ms-gs-uint", TestSubcase::Create<BasicMS<uvec4, 3> >));
	addChild(new TestSubcase(m_context, "basic-ms-fs-float", TestSubcase::Create<BasicMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-ms-fs-int", TestSubcase::Create<BasicMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-ms-fs-uint", TestSubcase::Create<BasicMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-ms-cs-float", TestSubcase::Create<BasicMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-ms-cs-int", TestSubcase::Create<BasicMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-ms-cs-uint", TestSubcase::Create<BasicMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-changeSize", TestSubcase::Create<AdvancedChangeSize>));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-float", TestSubcase::Create<AdvancedNonMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tcs-float", TestSubcase::Create<AdvancedNonMS<vec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tcs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tcs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tes-float", TestSubcase::Create<AdvancedNonMS<vec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tes-int", TestSubcase::Create<AdvancedNonMS<ivec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-tes-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-gs-float", TestSubcase::Create<AdvancedNonMS<vec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-gs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-gs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-float", TestSubcase::Create<AdvancedNonMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-float", TestSubcase::Create<AdvancedNonMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-ms-vs-float", TestSubcase::Create<AdvancedMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-ms-vs-int", TestSubcase::Create<AdvancedMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-ms-vs-uint", TestSubcase::Create<AdvancedMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tcs-float", TestSubcase::Create<AdvancedMS<vec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tcs-int", TestSubcase::Create<AdvancedMS<ivec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tcs-uint", TestSubcase::Create<AdvancedMS<uvec4, 1> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tes-float", TestSubcase::Create<AdvancedMS<vec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tes-int", TestSubcase::Create<AdvancedMS<ivec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-ms-tes-uint", TestSubcase::Create<AdvancedMS<uvec4, 2> >));
	addChild(new TestSubcase(m_context, "advanced-ms-gs-float", TestSubcase::Create<AdvancedMS<vec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-ms-gs-int", TestSubcase::Create<AdvancedMS<ivec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-ms-gs-uint", TestSubcase::Create<AdvancedMS<uvec4, 3> >));
	addChild(new TestSubcase(m_context, "advanced-ms-fs-float", TestSubcase::Create<AdvancedMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-ms-fs-int", TestSubcase::Create<AdvancedMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-ms-fs-uint", TestSubcase::Create<AdvancedMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-ms-cs-float", TestSubcase::Create<AdvancedMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-ms-cs-int", TestSubcase::Create<AdvancedMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-ms-cs-uint", TestSubcase::Create<AdvancedMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "negative-compileTime", TestSubcase::Create<NegativeCompileTime>));
}

} // namespace gl4cts
