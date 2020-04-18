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

#include "es31cShaderImageSizeTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuVectorUtil.hpp"
#include <assert.h>
#include <cstdarg>

namespace glcts
{
using namespace glw;

namespace
{
typedef tcu::Vec2  vec2;
typedef tcu::Vec3  vec3;
typedef tcu::Vec4  vec4;
typedef tcu::IVec4 ivec4;
typedef tcu::UVec4 uvec4;

const char* const kGLSLVer =
	"#version 310 es\n" NL "precision highp float;" NL "precision highp int;" NL "precision highp image2D;" NL
	"precision highp image3D;" NL "precision highp imageCube;" NL "precision highp image2DArray;" NL
	"precision highp iimage2D;" NL "precision highp iimage3D;" NL "precision highp iimageCube;" NL
	"precision highp iimage2DArray;" NL "precision highp uimage2D;" NL "precision highp uimage3D;" NL
	"precision highp uimageCube;" NL "precision highp uimage2DArray;";

class ShaderImageSizeBase : public glcts::SubcaseBase
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
			reason << "Required " << requiredVS << " VS storage blocks but only " << imagesVS << " available."
				   << std::endl
				   << "Required " << requiredFS << " FS storage blocks but only " << imagesFS << " available."
				   << std::endl;
			OutputNotSupported(reason.str());
			return false;
		}
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

template <typename T>
GLenum TexFormat();

//=============================================================================
// ImageSizeMachine
//-----------------------------------------------------------------------------
class ImageSizeMachine : public glcts::GLWrapper
{
	GLuint m_pipeline;
	GLuint m_program[3];
	GLuint m_vertex_array;
	GLuint m_buffer;
	bool   pipeline;
	GLuint m_xfb_id;

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
					<< tcu::TestLog::Message << kGLSLVer << cs << tcu::TestLog::EndMessage;
			return p;
		}

		return p;
	}

	GLuint BuildProgram(const char* src_vs, const char* src_fs, bool use_xfb, bool* result = NULL)
	{
		const GLuint p = glCreateProgram();

		if (src_vs)
		{
			GLuint sh = glCreateShader(GL_VERTEX_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[2] = { kGLSLVer, src_vs };
			glShaderSource(sh, 2, src, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << src[0] << src[1] << tcu::TestLog::EndMessage;
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
			const char* const src[2] = { kGLSLVer, src_fs };
			glShaderSource(sh, 2, src, NULL);
			if (!CompileShader(sh))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << src[0] << src[1] << tcu::TestLog::EndMessage;
				if (result)
					*result = false;
				return p;
			}
		}
		if (use_xfb)
			SetupTransformFeedback(p);
		if (!LinkProgram(p))
		{
			if (src_vs)
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << kGLSLVer << src_vs << tcu::TestLog::EndMessage;
			if (src_fs)
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << kGLSLVer << src_fs << tcu::TestLog::EndMessage;
			if (result)
				*result = false;
			return p;
		}

		return p;
	}

	void SetupTransformFeedback(GLuint program)
	{
		const char* const varying_name[] = { "count[0]", "count[1]", "count[2]", "count[3]" };
		glTransformFeedbackVaryings(program, 4, varying_name, GL_INTERLEAVED_ATTRIBS);
	}

	inline bool Equal(const ivec4& result, const ivec4& expected)
	{
		if (expected[0] != result[0])
			return false;
		if (expected[1] != result[1])
			return false;
		if (expected[2] != result[2])
			return false;
		if (expected[3] != result[3])
			return false;
		return true;
	}

	template <typename T>
	std::string GenShader(int stage)
	{
		std::ostringstream os;
		os << NL "#define KSIZE 4";
		if (stage == 0)
		{ // VS uses transform feedback
			os << NL "flat out ivec4 count[KSIZE];";
		}
		else
		{ // CS + FS use SSBO
			os << NL "layout(std430) buffer OutputBuffer {" NL "  ivec4 count[KSIZE];" NL "};";
		}
		os << NL "layout(binding = 0, rgba32" << ImageFormatPostfix<T>() << ") readonly writeonly uniform highp "
		   << ImageTypePrefix<T>() << "image2D g_image_2d;" NL "layout(binding = 1, rgba32" << ImageFormatPostfix<T>()
		   << ") readonly writeonly uniform highp " << ImageTypePrefix<T>()
		   << "image3D g_image_3d;" NL "layout(binding = 2, rgba32" << ImageFormatPostfix<T>()
		   << ") readonly writeonly uniform highp " << ImageTypePrefix<T>()
		   << "imageCube g_image_cube;" NL "layout(binding = 3, rgba32" << ImageFormatPostfix<T>()
		   << ") readonly writeonly uniform highp " << ImageTypePrefix<T>() << "image2DArray g_image_2d_array;";
		if (stage == 0)
		{ // VS
			os << NL "void main() {" NL "  int coord = gl_VertexID;" NL "#ifdef GL_ES" NL "  gl_PointSize = 1.0;" NL
					 "#endif";
		}
		else if (stage == 4)
		{ // CS
			os << NL "layout(local_size_x = 1) in;" NL "void main() {" NL "  int coord = int(gl_GlobalInvocationID.x);";
		}
		else if (stage == 5)
		{ // FS
			os << NL "uniform int fakePrimitiveID;" NL "void main() {" NL "  int coord = fakePrimitiveID;";
		}
		os << NL "  count[coord + 0] = ivec4(imageSize(g_image_2d), 0, 0);" NL
				 "  count[coord + 1] = ivec4(imageSize(g_image_3d), 0);" NL
				 "  count[coord + 2] = ivec4(imageSize(g_image_cube), 0, 0);" NL
				 "  count[coord + 3] = ivec4(imageSize(g_image_2d_array), 0);" NL "}";
		return os.str();
	}

public:
	ImageSizeMachine() : pipeline(false)
	{
		if (pipeline)
			glGenProgramPipelines(1, &m_pipeline);
		memset(m_program, 0, sizeof(m_program));
		glGenVertexArrays(1, &m_vertex_array);
		glGenBuffers(1, &m_buffer);
		glGenTransformFeedbacks(1, &m_xfb_id);
	}

	~ImageSizeMachine()
	{
		if (pipeline)
		{
			glDeleteProgramPipelines(1, &m_pipeline);
			for (int i = 0; i < 3; ++i)
				glDeleteProgram(m_program[i]);
		}
		else
		{
			glDeleteProgram(m_program[0]);
		}
		glDeleteVertexArrays(1, &m_vertex_array);
		glDeleteBuffers(1, &m_buffer);
		glDeleteTransformFeedbacks(1, &m_xfb_id);
	}

	template <typename T>
	long Run(int stage, ivec4 expected_result[4])
	{
		const int kSize = 4;
		if (stage == 0)
		{ // VS
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_id);
			const char* const glsl_fs = NL "void main() {" NL "  discard;" NL "}";
			std::string		  vs	  = GenShader<T>(stage);
			const char* const glsl_vs = vs.c_str();
			if (pipeline)
			{
				m_program[0] = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
				m_program[1] = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
				glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
				glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_program[1]);
			}
			else
			{
				m_program[0] = BuildProgram(glsl_vs, glsl_fs, true);
			}
		}
		else if (stage == 4)
		{ // CS
			std::string		  cs	  = GenShader<T>(stage);
			const char* const glsl_cs = cs.c_str();
			if (pipeline)
			{
				m_program[0] = glCreateShaderProgramv(GL_COMPUTE_SHADER, 1, &glsl_cs);
				glUseProgramStages(m_pipeline, GL_COMPUTE_SHADER_BIT, m_program[0]);
			}
			else
			{
				m_program[0] = CreateComputeProgram(glsl_cs);
			}
		}
		else if (stage == 5)
		{ // FS
			const char* const glsl_vs =
				NL "layout(location = 0) in vec4 i_position;" NL "void main() {" NL "  gl_Position = i_position;" NL
				   "#ifdef GL_ES" NL "  gl_PointSize = 1.0;" NL "#endif" NL "}";
			std::string		  fs	  = GenShader<T>(stage);
			const char* const glsl_fs = fs.c_str();
			if (pipeline)
			{
				m_program[0] = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
				m_program[1] = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
				glUseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, m_program[0]);
				glUseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, m_program[1]);
			}
			else
			{
				m_program[0] = BuildProgram(glsl_vs, glsl_fs, false);
			}
		}
		if (!CheckProgram(m_program[0]))
			return ERROR;
		if (pipeline)
			if (!CheckProgram(m_program[1]))
				return ERROR;

		ivec4 data[kSize];
		for (int i  = 0; i < kSize; ++i)
			data[i] = ivec4(100000);

		GLenum output_buffer_type = (stage == 0) ? GL_TRANSFORM_FEEDBACK_BUFFER : GL_SHADER_STORAGE_BUFFER;

		glBindBufferBase(output_buffer_type, 0, m_buffer);
		glBufferData(output_buffer_type, kSize * 4 * 4, &data[0], GL_STATIC_DRAW);

		if (pipeline)
			glBindProgramPipeline(m_pipeline);
		else
			glUseProgram(m_program[0]);
		glBindVertexArray(m_vertex_array);

		if (stage == 0)
			glBeginTransformFeedback(GL_POINTS);

		if (stage == 4)
			glDispatchCompute(1, 1, 1);
		else
			glDrawArrays(GL_POINTS, 0, 1);

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		if (stage == 0)
			glEndTransformFeedback();

		ivec4* map_data = (ivec4*)glMapBufferRange(output_buffer_type, 0, kSize * 4 * 4, GL_MAP_READ_BIT);
		for (int i = 0; i < kSize; ++i)
		{
			if (!Equal(map_data[i], expected_result[i]))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Returned value is: (" << map_data[i][0] << " " << map_data[i][1] << " "
					<< map_data[i][2] << " " << map_data[i][3] << "). Expected value is: (" << expected_result[i][0]
					<< " " << expected_result[i][1] << " " << expected_result[i][2] << " " << expected_result[i][3]
					<< "). Image unit is: " << i << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}
		glUnmapBuffer(output_buffer_type);

		if (stage == 0)
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

		return NO_ERROR;
	}
};

//=============================================================================
// 1.1.x.y BasicNonMS
//-----------------------------------------------------------------------------
template <typename T, int STAGE>
class BasicNonMS : public ShaderImageSizeBase
{
	GLuint m_texture[4];

	virtual long Setup()
	{
		glGenTextures(4, m_texture);
		return NO_ERROR;
	}
	virtual long Run()
	{
		if (STAGE == 0 && !IsVSFSAvailable(4, 0))
			return NOT_SUPPORTED;
		if (STAGE == 5 && !IsVSFSAvailable(0, 4))
			return NOT_SUPPORTED;

		const GLenum target[4] = { GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY };
		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			if (i == 0)
			{
				glTexStorage2D(target[i], 10, TexInternalFormat<T>(), 512, 128);
				glBindImageTexture(0, m_texture[i], 1, GL_FALSE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexStorage3D(target[i], 3, TexInternalFormat<T>(), 8, 8, 4);
				glBindImageTexture(1, m_texture[i], 0, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexStorage2D(target[i], 4, TexInternalFormat<T>(), 16, 16);
				glBindImageTexture(2, m_texture[i], 0, GL_TRUE, 0, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexStorage3D(target[i], 3, TexInternalFormat<T>(), 127, 39, 12);
				glBindImageTexture(3, m_texture[i], 2, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4			 res[4] = { ivec4(256, 64, 0, 0), ivec4(8, 8, 4, 0), ivec4(16, 16, 0, 0), ivec4(31, 9, 12, 0) };
		return machine.Run<T>(STAGE, res);
	}
	virtual long Cleanup()
	{
		glDeleteTextures(4, m_texture);
		return NO_ERROR;
	}
};
//=============================================================================
// 2.2.x.y AdvancedNonMS
//-----------------------------------------------------------------------------
template <typename T, int STAGE>
class AdvancedNonMS : public ShaderImageSizeBase
{
	GLuint m_texture[4];

	virtual long Setup()
	{
		glGenTextures(4, m_texture);
		return NO_ERROR;
	}
	virtual long Run()
	{
		if (STAGE == 0 && !IsVSFSAvailable(4, 0))
			return NOT_SUPPORTED;
		if (STAGE == 5 && !IsVSFSAvailable(0, 4))
			return NOT_SUPPORTED;

		const GLenum target[4] = { GL_TEXTURE_2D_ARRAY, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY };
		for (int i = 0; i < 4; ++i)
		{
			glBindTexture(target[i], m_texture[i]);
			glTexParameteri(target[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(target[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			if (i == 0)
			{
				glTexStorage3D(target[i], 2, TexInternalFormat<T>(), 2, 2, 7);
				glBindImageTexture(0, m_texture[i], 1, GL_FALSE, 3, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 1)
			{
				glTexStorage3D(target[i], 3, TexInternalFormat<T>(), 4, 4, 2);
				glBindImageTexture(1, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
			else if (i == 2)
			{
				glTexStorage2D(target[i], 2, TexInternalFormat<T>(), 2, 2);
				glBindImageTexture(2, m_texture[i], 0, GL_TRUE, 0, GL_READ_WRITE, TexInternalFormat<T>());
			}
			else if (i == 3)
			{
				glTexStorage3D(target[i], 4, TexInternalFormat<T>(), 13, 7, 4);
				glBindImageTexture(3, m_texture[i], 1, GL_TRUE, 0, GL_READ_ONLY, TexInternalFormat<T>());
			}
		}
		ImageSizeMachine machine;
		ivec4			 res[4] = { ivec4(1, 1, 0, 0), ivec4(2, 2, 1, 0), ivec4(2, 2, 0, 0), ivec4(6, 3, 4, 0) };
		return machine.Run<T>(STAGE, res);
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
		if (!Compile( // imagesize return type check
				"#version 310 es" NL "precision highp float;" NL "precision highp int;" NL
				"layout(local_size_x = 1) in;" NL "layout(r32f) uniform image2D g_image;" NL
				"layout(std430) buffer OutputBuffer { vec4 g_color; };" NL "void main() {" NL
				"  if (imageSize(g_image) == ivec3(5)) g_color = vec4(0, 1, 0, 1);" NL
				"  else g_color = vec4(1, 0, 0, 1);" NL "}"))
		{
			return ERROR;
		}
		if (!Compile( // imageSize(samplertype)
				"#version 310 es" NL "precision highp float;" NL "precision highp int;" NL
				"layout(local_size_x = 1) in;" NL "layout(r32f) uniform sampler2D g_image;" NL
				"layout(std430) buffer OutputBuffer { vec4 g_color; };" NL "void main() {" NL
				"  if (imageSize(g_image) == ivec2(5)) g_color = vec4(0, 1, 0, 1);" NL
				"  else g_color = vec4(1, 0, 0, 1);" NL "}"))
		{
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

} // anonymous namespace

ShaderImageSizeTests::ShaderImageSizeTests(glcts::Context& context) : TestCaseGroup(context, "shader_image_size", "")
{
}

ShaderImageSizeTests::~ShaderImageSizeTests(void)
{
}

void ShaderImageSizeTests::init()
{
	using namespace glcts;
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-float", TestSubcase::Create<BasicNonMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-int", TestSubcase::Create<BasicNonMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-vs-uint", TestSubcase::Create<BasicNonMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-float", TestSubcase::Create<BasicNonMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-int", TestSubcase::Create<BasicNonMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-fs-uint", TestSubcase::Create<BasicNonMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-float", TestSubcase::Create<BasicNonMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-int", TestSubcase::Create<BasicNonMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "basic-nonMS-cs-uint", TestSubcase::Create<BasicNonMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-float", TestSubcase::Create<AdvancedNonMS<vec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-vs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 0> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-float", TestSubcase::Create<AdvancedNonMS<vec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-fs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 5> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-float", TestSubcase::Create<AdvancedNonMS<vec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-int", TestSubcase::Create<AdvancedNonMS<ivec4, 4> >));
	addChild(new TestSubcase(m_context, "advanced-nonMS-cs-uint", TestSubcase::Create<AdvancedNonMS<uvec4, 4> >));
	addChild(new TestSubcase(m_context, "negative-compileTime", TestSubcase::Create<NegativeCompileTime>));
}
}
