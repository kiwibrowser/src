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

#include "gl4cTextureGatherTests.hpp"
#include "glcTestSubcase.hpp"
#include "gluContextInfo.hpp"
#include "gluPixelTransfer.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include <cstdarg>
#include <math.h>
#include <string>
#include <vector>

namespace gl4cts
{

using namespace glw;
using tcu::Vec4;
using tcu::Vec3;
using tcu::Vec2;
using tcu::IVec4;
using tcu::UVec4;

namespace
{

class TGBase : public deqp::SubcaseBase
{
public:
	virtual ~TGBase()
	{
	}

	TGBase() : renderTarget(m_context.getRenderContext().getRenderTarget()), pixelFormat(renderTarget.getPixelFormat())
	{
		g_color_eps = Vec4(1.f / (float)(1 << pixelFormat.redBits), 1.f / (float)(1 << pixelFormat.greenBits),
						   1.f / (float)(1 << pixelFormat.blueBits), 1.f / (float)(1 << pixelFormat.alphaBits));
	}

	const tcu::RenderTarget& renderTarget;
	const tcu::PixelFormat&  pixelFormat;
	Vec4					 g_color_eps;

	int GetWindowWidth()
	{
		return renderTarget.getWidth();
	}

	int GetWindowHeight()
	{
		return renderTarget.getHeight();
	}

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

	GLuint CreateProgram(const char* src_vs, const char* src_tcs, const char* src_tes, const char* src_gs,
						 const char* src_fs)
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
		return p;
	}

	GLuint CreateComputeProgram(const std::string& cs)
	{
		const GLuint p = glCreateProgram();

		if (!cs.empty())
		{
			const GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
			glAttachShader(p, sh);
			glDeleteShader(sh);
			const char* const src[1] = { cs.c_str() };
			glShaderSource(sh, 1, src, NULL);
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

	GLfloat distance(GLfloat p0, GLfloat p1)
	{
		return de::abs(p0 - p1);
	}

	inline bool ColorEqual(const Vec4& c0, const Vec4& c1, const Vec4& epsilon)
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

	virtual long Setup()
	{
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		return NO_ERROR;
	}
};

class GatherEnumsTest : public TGBase
{
	virtual std::string Title()
	{
		return "Basic Enum Test";
	}

	virtual std::string Purpose()
	{
		return "Verify that gather related enums are correct.";
	}

	virtual std::string Method()
	{
		return "Query GL_*_TEXTURE_GATHER_OFFSET enums.";
	}

	virtual std::string PassCriteria()
	{
		return "Values of enums meet GL spec requirements.";
	}

	virtual long Run()
	{
		GLint res;
		glGetIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &res);
		if (res > -8)
		{
			return ERROR;
		}
		glGetIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &res);
		if (res < 7)
		{
			return ERROR;
		}
		return NO_ERROR;
	}
};

class GatherGLSLCompile : public TGBase
{
	GLuint program;

	virtual std::string Title()
	{
		return "GLSL Compile Test";
	}

	virtual std::string Purpose()
	{
		return "Verify that gather functions are visible in the shaders.";
	}

	virtual std::string Method()
	{
		return "Create shaders which use all types of gather functions.";
	}

	virtual std::string PassCriteria()
	{
		return "Programs compile and link successfuly.";
	}

	virtual std::string Uniforms()
	{
		return "uniform sampler2D tex_2d;                  \n"
			   "uniform isamplerCube itex_cube;            \n"
			   "uniform usampler2DArray utex_2da;          \n"
			   "uniform isampler2DRect itex_2dr;           \n"
			   ""
			   "uniform sampler2DRectShadow tex_2drs;      \n"
			   "uniform sampler2DShadow tex_2ds;           \n"
			   "uniform samplerCubeShadow tex_cubes;       \n"
			   "uniform sampler2DArrayShadow tex_2das;     \n";
	}

	virtual std::string Sampling()
	{
		return "    textureGather(tex_2d,vec2(1));          \n"
			   "    textureGather(itex_cube,vec3(1));       \n"
			   "    textureGather(utex_2da,vec3(1));        \n"
			   "    textureGather(itex_2dr,vec2(1));        \n"
			   ""
			   "    textureGather(tex_2drs,vec2(1), 0.5);   \n"
			   "    textureGather(tex_2ds,vec2(1), 0.5);    \n"
			   "    textureGather(tex_cubes,vec3(1), 0.5);  \n"
			   "    textureGather(tex_2das,vec3(1), 0.5);   \n"
			   ""
			   "    textureGatherOffset(tex_2d,vec2(1), ivec2(0));          \n"
			   "    textureGatherOffset(utex_2da,vec3(1), ivec2(0));        \n"
			   "    textureGatherOffset(itex_2dr,vec2(1), ivec2(0));        \n"
			   ""
			   "    textureGatherOffset(tex_2drs,vec2(1), 0.5, ivec2(0));   \n"
			   "    textureGatherOffset(tex_2ds,vec2(1), 0.5, ivec2(0));    \n"
			   "    textureGatherOffset(tex_2das,vec3(1), 0.5, ivec2(0));   \n"
			   ""
			   "    const ivec2 offsets[4] = ivec2[](ivec2(0), ivec2(0), ivec2(0), ivec2(0)); \n"
			   "    textureGatherOffsets(tex_2d,vec2(1), offsets);          \n"
			   "    textureGatherOffsets(utex_2da,vec3(1), offsets);        \n"
			   "    textureGatherOffsets(itex_2dr,vec2(1), offsets);        \n"
			   ""
			   "    textureGatherOffsets(tex_2drs,vec2(1), 0.5, offsets);   \n"
			   "    textureGatherOffsets(tex_2ds,vec2(1), 0.5, offsets);    \n"
			   "    textureGatherOffsets(tex_2das,vec3(1), 0.5, offsets);   \n";
	}

	virtual std::string VertexShader()
	{
		return "#version 400                               \n" + Uniforms() +
			   "  void main() {                            \n" + Sampling() +
			   "    gl_Position = vec4(1);                 \n"
			   "  }                                        \n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 400                               \n"
			   "out vec4 color;                            \n" +
			   Uniforms() + "  void main() {                            \n" + Sampling() +
			   "    color = vec4(1);                       \n"
			   "  }                                        \n";
	}

	virtual long Run()
	{
		program = CreateProgram(VertexShader().c_str(), NULL, NULL, NULL, FragmentShader().c_str());
		glLinkProgram(program);
		if (!CheckProgram(program))
			return ERROR;
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glDeleteProgram(program);
		return NO_ERROR;
	}
};

class GatherBase : public TGBase
{
public:
	GLuint tex, fbo, rbo, program, vao, vbo;

	virtual GLvoid CreateTexture2DRgb(bool base_level = false)
	{
		GLenum		internal_format = GL_RGB32F;
		GLenum		format			= GL_RGB;
		const GLint csize			= base_level ? 64 : 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= GL_FLOAT;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, 0);
		}
		std::vector<Vec3> pixels(csize * csize, Vec3(1.0));
		glTexSubImage2D(target, 0, 0, 0, csize, csize, format, tex_type, &pixels[0]);
		glGenerateMipmap(target);

		Vec3 data[4] = { Vec3(12. / 16, 13. / 16, 14. / 16), Vec3(8. / 16, 9. / 16, 10. / 16),
						 Vec3(0. / 16, 1. / 16, 2. / 16), Vec3(4. / 16, 5. / 16, 6. / 16) };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, tex_type, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, tex_type, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, tex_type, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTexture2DRg(bool base_level = false)
	{
		GLenum		internal_format = GL_RG32F;
		GLenum		format			= GL_RG;
		const GLint csize			= base_level ? 64 : 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= GL_FLOAT;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, 0);
		}
		std::vector<Vec2> pixels(csize * csize, Vec2(1.0));
		glTexSubImage2D(target, 0, 0, 0, csize, csize, format, tex_type, &pixels[0]);
		glGenerateMipmap(target);

		Vec2 data[4] = { Vec2(12. / 16, 13. / 16), Vec2(8. / 16, 9. / 16), Vec2(0. / 16, 1. / 16),
						 Vec2(4. / 16, 5. / 16) };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, tex_type, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, tex_type, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, tex_type, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTexture2DR(bool base_level = false)
	{
		GLenum		internal_format = GL_R32F;
		GLenum		format			= GL_RED;
		const GLint csize			= base_level ? 64 : 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= GL_FLOAT;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, 0);
		}
		std::vector<GLfloat> pixels(csize * csize, 1.0);
		glTexSubImage2D(target, 0, 0, 0, csize, csize, format, tex_type, &pixels[0]);
		glGenerateMipmap(target);

		GLfloat data[4] = { 12. / 16., 8. / 16., 0. / 16., 4. / 16. };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, tex_type, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, tex_type, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, tex_type, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTexture2DInt(bool rect = false)
	{
		GLenum		internal_format = InternalFormat();
		const GLint csize			= 32;
		GLint		size			= csize;
		GLenum		target			= rect ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		if (!rect)
		{
			for (int i = 0; size > 0; ++i, size /= 2)
			{
				glTexImage2D(target, i, internal_format, size, size, 0, GL_RGBA_INTEGER, GL_INT, 0);
			}
		}
		else
		{
			glTexImage2D(target, 0, internal_format, size, size, 0, GL_RGBA_INTEGER, GL_INT, 0);
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		glTexSubImage2D(target, 0, 0, 0, csize, csize, GL_RGBA_INTEGER, GL_INT, &pixels[0]);

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		glTexSubImage2D(target, 0, 22, 25, 2, 2, GL_RGBA_INTEGER, GL_INT, data);
		glTexSubImage2D(target, 0, 16, 10, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 0);
		glTexSubImage2D(target, 0, 11, 2, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 1);
		glTexSubImage2D(target, 0, 24, 13, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 2);
		glTexSubImage2D(target, 0, 9, 14, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	virtual GLvoid CreateTexture2DArrayInt(int slices, int data_slice)
	{
		GLenum		internal_format = InternalFormat();
		const GLint csize			= 32;
		GLint		size			= csize;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, size, size, slices, 0, GL_RGBA_INTEGER, GL_INT, 0);
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		for (int i = 0; i < slices; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, csize, csize, 1, GL_RGBA_INTEGER, GL_INT, &pixels[0]);
		}

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 22, 25, data_slice, 2, 2, 1, GL_RGBA_INTEGER, GL_INT, data);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 16, 10, data_slice, 1, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 11, 2, data_slice, 1, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 1);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 24, 13, data_slice, 1, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 2);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 9, 14, data_slice, 1, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 3);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	virtual GLvoid CreateTextureCubeArray(int slices, int data_slice)
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= 32;
		GLint		size			= csize;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, i, internal_format, size, size, 6 * slices, 0, format, GL_FLOAT, 0);
		}
		std::vector<Vec4> pixels(csize * csize, Vec4(1.0));
		for (int j = 0; j < 6 * slices; ++j)
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, j, csize, csize, 1, format, GL_FLOAT, &pixels[0]);
		}

		if (format != GL_DEPTH_COMPONENT)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
		}

		Vec4 data[4] = { Vec4(12. / 16, 13. / 16, 14. / 16, 15. / 16), Vec4(8. / 16, 9. / 16, 10. / 16, 11. / 16),
						 Vec4(0. / 16, 1. / 16, 2. / 16, 3. / 16), Vec4(4. / 16, 5. / 16, 6. / 16, 7. / 16) };

		Vec4  depthData(data[0][0], data[1][0], data[2][0], data[3][0]);
		Vec4* packedData = (format == GL_DEPTH_COMPONENT) ? &depthData : data;

		for (int i = 0; i < 6; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 22, 25, (6 * data_slice) + i, 2, 2, 1, format, GL_FLOAT,
							packedData);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 16, 10, (6 * data_slice) + i, 1, 1, 1, format, GL_FLOAT,
							data + 0);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 11, 2, (6 * data_slice) + i, 1, 1, 1, format, GL_FLOAT,
							data + 1);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 24, 13, (6 * data_slice) + i, 1, 1, 1, format, GL_FLOAT,
							data + 2);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 9, 14, (6 * data_slice) + i, 1, 1, 1, format, GL_FLOAT,
							data + 3);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTextureCubeArrayInt(int slices, int data_slice)
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= GL_RGBA_INTEGER;
		const GLint csize			= 32;
		GLint		size			= csize;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, i, internal_format, size, size, 6 * slices, 0, format, GL_INT, 0);
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		for (int j = 0; j < 6 * slices; ++j)
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, j, csize, csize, 1, format, GL_INT, &pixels[0]);
		}

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		for (int i = 0; i < 6; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 22, 25, (6 * data_slice) + i, 2, 2, 1, format, GL_INT, data);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 9, 14, (6 * data_slice) + i, 1, 1, 1, format, GL_INT,
							data + 3);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 16, 10, (6 * data_slice) + i, 1, 1, 1, format, GL_INT,
							data + 0);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 11, 2, (6 * data_slice) + i, 1, 1, 1, format, GL_INT,
							data + 1);
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 24, 13, (6 * data_slice) + i, 1, 1, 1, format, GL_INT,
							data + 2);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	virtual GLvoid CreateTexture2DArray(int slices, int data_slice)
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= 32;
		GLint		size			= csize;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, size, size, slices, 0, format, GL_FLOAT, 0);
		}
		std::vector<Vec4> pixels(csize * csize, Vec4(1.0));
		for (int i = 0; i < slices; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, csize, csize, 1, format, GL_FLOAT, &pixels[0]);
		}

		if (format != GL_DEPTH_COMPONENT)
		{
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
		}

		Vec4 data[4] = { Vec4(12. / 16, 13. / 16, 14. / 16, 15. / 16), Vec4(8. / 16, 9. / 16, 10. / 16, 11. / 16),
						 Vec4(0. / 16, 1. / 16, 2. / 16, 3. / 16), Vec4(4. / 16, 5. / 16, 6. / 16, 7. / 16) };

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 22, 25, data_slice, 2, 2, 1, format, GL_FLOAT, data);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 16, 10, data_slice, 1, 1, 1, format, GL_FLOAT, data + 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 11, 2, data_slice, 1, 1, 1, format, GL_FLOAT, data + 1);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 24, 13, data_slice, 1, 1, 1, format, GL_FLOAT, data + 2);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 9, 14, data_slice, 1, 1, 1, format, GL_FLOAT, data + 3);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTextureCubeInt()
	{
		GLenum		internal_format = InternalFormat();
		const GLint csize			= 32;
		GLint		size			= csize;

		const GLenum faces[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			for (int j = 0; j < 6; ++j)
			{
				glTexImage2D(faces[j], i, internal_format, size, size, 0, GL_RGBA_INTEGER, GL_INT, 0);
			}
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 0, 0, csize, csize, GL_RGBA_INTEGER, GL_INT, &pixels[0]);
		}

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 22, 25, 2, 2, GL_RGBA_INTEGER, GL_INT, data);
			glTexSubImage2D(faces[j], 0, 16, 10, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 0);
			glTexSubImage2D(faces[j], 0, 11, 2, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 1);
			glTexSubImage2D(faces[j], 0, 24, 13, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 2);
			glTexSubImage2D(faces[j], 0, 9, 14, 1, 1, GL_RGBA_INTEGER, GL_INT, data + 3);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	virtual GLvoid CreateTextureCube()
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= 32;
		GLint		size			= csize;

		const GLenum faces[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			for (int j = 0; j < 6; ++j)
			{
				glTexImage2D(faces[j], i, internal_format, size, size, 0, format, GL_FLOAT, 0);
			}
		}
		std::vector<Vec4> pixels(csize * csize, Vec4(1.0));
		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 0, 0, csize, csize, format, GL_FLOAT, &pixels[0]);
		}

		if (format != GL_DEPTH_COMPONENT)
		{
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		}

		Vec4 data[4] = { Vec4(12. / 16, 13. / 16, 14. / 16, 15. / 16), Vec4(8. / 16, 9. / 16, 10. / 16, 11. / 16),
						 Vec4(0. / 16, 1. / 16, 2. / 16, 3. / 16), Vec4(4. / 16, 5. / 16, 6. / 16, 7. / 16) };

		Vec4  depthData(data[0][0], data[1][0], data[2][0], data[3][0]);
		Vec4* packedData = (format == GL_DEPTH_COMPONENT) ? &depthData : data;

		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 22, 25, 2, 2, format, GL_FLOAT, packedData);
			glTexSubImage2D(faces[j], 0, 16, 10, 1, 1, format, GL_FLOAT, data + 0);
			glTexSubImage2D(faces[j], 0, 11, 2, 1, 1, format, GL_FLOAT, data + 1);
			glTexSubImage2D(faces[j], 0, 24, 13, 1, 1, format, GL_FLOAT, data + 2);
			glTexSubImage2D(faces[j], 0, 9, 14, 1, 1, format, GL_FLOAT, data + 3);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTexture2D(bool rect = false, bool base_level = false)
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= base_level ? 64 : 32;
		GLint		size			= csize;
		GLenum		target			= rect ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		if (!rect)
		{
			for (int i = 0; size > 0; ++i, size /= 2)
			{
				glTexImage2D(target, i, internal_format, size, size, 0, format, GL_FLOAT, 0);
			}
		}
		else
		{
			glTexImage2D(target, 0, internal_format, size, size, 0, format, GL_FLOAT, 0);
		}
		std::vector<Vec4> pixels(csize * csize, Vec4(1.0));
		glTexSubImage2D(target, 0, 0, 0, csize, csize, format, GL_FLOAT, &pixels[0]);

		if (!rect && format != GL_DEPTH_COMPONENT)
		{
			glGenerateMipmap(target);
		}

		Vec4 data[4] = { Vec4(12. / 16, 13. / 16, 14. / 16, 15. / 16), Vec4(8. / 16, 9. / 16, 10. / 16, 11. / 16),
						 Vec4(0. / 16, 1. / 16, 2. / 16, 3. / 16), Vec4(4. / 16, 5. / 16, 6. / 16, 7. / 16) };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, GL_FLOAT, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, GL_FLOAT, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, GL_FLOAT, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, GL_FLOAT, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, GL_FLOAT, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		if (base_level)
			glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 1);
	}

	virtual std::string FallthroughVertexShader()
	{
		return "#version 400                     \n"
			   "in vec4 v_in_0;                  \n"
			   "flat out vec4 v_out_0;           \n"
			   "void main() {                    \n"
			   "    gl_Position = vec4(0,0,0,1);                \n"
			   "    v_out_0 = v_in_0;                           \n"
			   "}";
	}

	virtual std::string FallthroughFragmentShader()
	{
		return "#version 400                     \n"
			   "out " +
			   Type() + " f_out_0;                \n"
						"flat in " +
			   Type() + " v_out_0;            \n"
						"void main() {                    \n"
						"    f_out_0 = v_out_0;           \n"
						"}";
	}

	virtual std::string TestFunction()
	{
		return Sampler() + TextBody();
	}

	virtual std::string Type()
	{
		return "vec4";
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2D my_sampler;                      \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                       \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y));           \n"
						"}\n";
	}

	virtual std::string VertexShader()
	{
		return "#version 400                                       \n"
			   "#extension GL_ARB_texture_cube_map_array : enable  \n"
			   ""
			   "in vec4 v_in_0;                                    \n"
			   "flat out " +
			   Type() + " v_out_0;                   \n" + TestFunction() +
			   "void main() {                                      \n"
			   "    gl_Position = vec4(0,0,0,1);                   \n"
			   "    v_out_0 = test_function(v_in_0);               \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 400                                       \n"
			   "#extension GL_ARB_texture_cube_map_array : enable  \n"
			   ""
			   "flat in vec4 v_out_0;                              \n"
			   "out " +
			   Type() + " f_out_0;                        \n" + TestFunction() +
			   "void main() {                                      \n"
			   "    f_out_0 = test_function(v_out_0);              \n"
			   "}";
	}

	virtual std::string ComputeShader()
	{
		return "#version 400 core                                           \n"
			   "#extension GL_ARB_shader_storage_buffer_object : require    \n"
			   "#extension GL_ARB_compute_shader : require            \n"
			   "#extension GL_ARB_texture_cube_map_array : enable     \n"
			   "layout(local_size_x = 1, local_size_y = 1) in;        \n"
			   "layout(std430) buffer Output {                        \n"
			   "  " +
			   Type() + " data;                                \n"
						"} g_out;                                              \n"
						"uniform vec4 cs_in;                                   \n" +
			   TestFunction() + "void main() {                                         \n"
								"  g_out.data = test_function(cs_in);                  \n"
								"}                                                     \n";
	}

	virtual void Init()
	{
		CreateTexture2D();
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 1, 1);
	}

	virtual long Verify()
	{
		Vec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, &data);
		if (!ColorEqual(data, Expected(), g_color_eps))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << ", epsilon: " << g_color_eps.x() << ", " << g_color_eps.y() << ", "
				<< g_color_eps.z() << ", " << g_color_eps.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual Vec4 Expected()
	{
		return Vec4(0. / 16, 4. / 16, 8. / 16, 12. / 16);
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32F;
	}

	virtual GLenum Format()
	{
		return GL_RGBA;
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 5, 3);
	}

	virtual bool Supported()
	{
		return true;
	}

	virtual long Run()
	{

		if (!Supported())
			return NO_ERROR;
		Init();

		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		SetRbo();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		GLfloat colorf[4] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, colorf);
		glViewport(0, 0, 1, 1);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);
		Vec4 buffData = BufferData();
		glBufferData(GL_ARRAY_BUFFER, 16, &buffData, GL_STATIC_DRAW);

		for (int i = 0; i < 2; ++i)
		{
			if (i == 0)
				program = CreateProgram(VertexShader().c_str(), NULL, NULL, NULL, FallthroughFragmentShader().c_str());
			else
				program = CreateProgram(FallthroughVertexShader().c_str(), NULL, NULL, NULL, FragmentShader().c_str());
			glBindAttribLocation(program, 0, "v_in_0");
			glBindFragDataLocation(program, 0, "f_out_0");
			glLinkProgram(program);
			if (!CheckProgram(program))
				return ERROR;
			glUseProgram(program);

			glDrawArrays(GL_POINTS, 0, 1);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			glDeleteProgram(program);

			if (Verify() == ERROR)
				return ERROR;
		}

		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader") ||
			!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected "
				<< "GL_ARB_compute_shader or GL_ARB_compute_shader not supported, skipping compute stage"
				<< tcu::TestLog::EndMessage;
			return NO_ERROR;
		}
		else
		{
			return TestCompute();
		}
	}

	virtual long TestCompute()
	{
		GLuint m_buffer;

		program = CreateComputeProgram(ComputeShader());
		glLinkProgram(program);
		if (!CheckProgram(program))
			return ERROR;
		glUseProgram(program);

		glUniform4f(glGetUniformLocation(program, "cs_in"), BufferData().x(), BufferData().y(), BufferData().z(),
					BufferData().w());

		glGenBuffers(1, &m_buffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vec4), NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glDispatchCompute(1, 1, 1);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		long error = VerifyCompute();
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &m_buffer);

		return error;
	}

	virtual long VerifyCompute()
	{
		Vec4* data;
		data = static_cast<Vec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Vec4), GL_MAP_READ_BIT));
		if (!ColorEqual(data[0], Expected(), g_color_eps))
		{ // for unorms
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << " got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glViewport(0, 0, GetWindowWidth(), GetWindowHeight());
		glDisableVertexAttribArray(0);
		glDeleteTextures(1, &tex);
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteRenderbuffers(1, &rbo);
		glDeleteFramebuffers(1, &fbo);
		glDeleteProgram(program);
		return NO_ERROR;
	}
};

class PlainGatherFloat2DRgba : public GatherBase
{
};

class PlainGatherFloat2DRg : public GatherBase
{
public:
	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                    \n"
			   "    return textureGather(my_sampler, vec2(p.x, p.y), 1);        \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(1. / 16, 5. / 16, 9. / 16, 13. / 16);
	}

	virtual GLenum InternalFormat()
	{
		return GL_RG32F;
	}
};

class PlainGatherUnorm2D : public GatherBase
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_RGBA16;
	}

	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                    \n"
			   "    return textureGather(my_sampler, vec2(p.x, p.y), 0);        \n"
			   "}\n";
	}
};

class PlainGatherInt2DRgba : public GatherBase
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual void Init()
	{
		CreateTexture2DInt();
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Sampler()
	{
		return "uniform isampler2D my_sampler;                     \n";
	}

	virtual std::string Type()
	{
		return "ivec4";
	}
};

class PlainGatherInt2DRg : public PlainGatherInt2DRgba
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_RG32I;
	}
};

class PlainGatherUint2D : public GatherBase
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual void Init()
	{
		CreateTexture2DInt();
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data);
		if (data != IVec4(2, 6, 10, 14))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 2, 6, 10, 14, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(2, 6, 10, 14))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Sampler()
	{
		return "uniform usampler2D my_sampler;                     \n";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(22.9f / 32, 25.9f / 32, 2, 2);
	}

	virtual std::string Type()
	{
		return "uvec4";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                   \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y), 2);            \n"
						"}\n";
	}
};

class PlainGatherDepth2D : public GatherBase
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_DEPTH_COMPONENT32F;
	}

	virtual GLenum Format()
	{
		return GL_DEPTH_COMPONENT;
	}

	virtual void Init()
	{
		CreateTexture2D();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 Expected()
	{
		return Vec4(1, 1, 0, 0);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 13.5 / 16, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DShadow my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y), p.z);       \n"
						"}\n";
	}
};

class PlainGatherFloat2DArray : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DArray(9, 5);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z));       \n"
						"}\n";
	}
};

class PlainGatherUnorm2DArray : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DArray(7, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.w));       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA16;
	}
};

class PlainGatherInt2DArray : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DArrayInt(20, 11);
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(3, 7, 11, 15))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 3, 7, 11, 15, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(3, 7, 11, 15))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "ivec4";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 11, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform isampler2DArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), 3);    \n"
						"}\n";
	}
};

class PlainGatherUint2DArray : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DArrayInt(3, 1);
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "uvec4";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 1, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform usampler2DArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z));       \n"
						"}\n";
	}
};

class PlainGatherDepth2DArray : public GatherBase
{
public:
	virtual GLenum InternalFormat()
	{
		return GL_DEPTH_COMPONENT32F;
	}

	virtual GLenum Format()
	{
		return GL_DEPTH_COMPONENT;
	}

	virtual void Init()
	{
		CreateTexture2DArray(9, 5);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 Expected()
	{
		return Vec4(1, 1, 0, 0);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 5, 13.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DArrayShadow my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), p.w);       \n"
						"}\n";
	}
};

class PlainGatherFloatCubeRgba : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCube();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z));       \n"
						"}\n";
	}
};

class PlainGatherFloatCubeRg : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCube();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), 1);    \n"
						"}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(1. / 16, 5. / 16, 9. / 16, 13. / 16);
	}

	virtual GLenum InternalFormat()
	{
		return GL_RG32F;
	}
};

class PlainGatherUnormCube : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCube();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z));       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA16;
	}
};

class PlainGatherIntCubeRgba : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCubeInt();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform isamplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z));       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "ivec4";
	}
};

class PlainGatherIntCubeRg : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCubeInt();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform isamplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), 1);    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RG32I;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(1, 5, 9, 13))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 1, 5, 9, 13, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(1, 5, 9, 13))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "ivec4";
	}
};

class PlainGatherUintCube : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCubeInt();
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform usamplerCube my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), 0);    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "uvec4";
	}
};

class PlainGatherDepthCube : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTextureCube();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual GLenum InternalFormat()
	{
		return GL_DEPTH_COMPONENT32F;
	}

	virtual GLenum Format()
	{
		return GL_DEPTH_COMPONENT;
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 7.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCubeShadow my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                \n"
						"    return textureGather(my_sampler, vec3(p.x, p.y, p.z), p.w);  \n"
						"}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(0, 0, 1, 1);
	}
};

class PlainGatherFloatCubeArray : public GatherBase
{
public:
	virtual bool Supported()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_cube_map_array"))
		{
			OutputNotSupported("GL_ARB_texture_cube_map_array not supported");
			return false;
		}
		return true;
	}

	virtual void Init()
	{
		CreateTextureCubeArray(9, 5);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 5);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCubeArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {              \n"
						"    return textureGather(my_sampler, p);       \n"
						"}\n";
	}
};

class PlainGatherUnormCubeArray : public GatherBase
{
public:
	virtual bool Supported()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_cube_map_array"))
		{
			OutputNotSupported("GL_ARB_texture_cube_map_array not supported");
			return false;
		}
		return true;
	}

	virtual void Init()
	{
		CreateTextureCubeArray(7, 3);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCubeArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {              \n"
						"    return textureGather(my_sampler, p);       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA16;
	}
};

class PlainGatherIntCubeArray : public GatherBase
{
public:
	virtual bool Supported()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_cube_map_array"))
		{
			OutputNotSupported("GL_ARB_texture_cube_map_array not supported");
			return false;
		}
		return true;
	}

	virtual void Init()
	{
		CreateTextureCubeArrayInt(20, 11);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 11);
	}

	virtual std::string Sampler()
	{
		return "uniform isamplerCubeArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {              \n"
						"    return textureGather(my_sampler, p);       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "ivec4";
	}
};

class PlainGatherUintCubeArray : public GatherBase
{
public:
	virtual bool Supported()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_cube_map_array"))
		{
			OutputNotSupported("GL_ARB_texture_cube_map_array not supported");
			return false;
		}
		return true;
	}

	virtual void Init()
	{
		CreateTextureCubeArrayInt(3, 1);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 1);
	}

	virtual std::string Sampler()
	{
		return "uniform usamplerCubeArray my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {              \n"
						"    return textureGather(my_sampler, p);       \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 1, 1);
	}

	virtual long Verify()
	{
		UVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data);
		if (data != UVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "uvec4";
	}
};

class PlainGatherDepthCubeArray : public GatherBase
{
public:
	virtual bool Supported()
	{
		if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_cube_map_array"))
		{
			OutputNotSupported("GL_ARB_texture_cube_map_array not supported");
			return false;
		}
		return true;
	}

	virtual void Init()
	{
		CreateTextureCubeArray(7, 3);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(7. / 16, -10. / 16, 1, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform samplerCubeArrayShadow my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                   \n"
						"    return textureGather(my_sampler, p, 7.5/16);    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_DEPTH_COMPONENT32F;
	}

	virtual GLenum Format()
	{
		return GL_DEPTH_COMPONENT;
	}

	virtual Vec4 Expected()
	{
		return Vec4(0, 0, 1, 1);
	}
};

class PlainGatherFloat2DRect : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2D(true);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23, 26, 0, 0);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DRect my_sampler;                     \n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(1. / 16, 5. / 16, 9. / 16, 13. / 16);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                          \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y), 1);   \n"
						"}\n";
	}
};

class PlainGatherUnorm2DRect : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2D(true);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23, 26, 0, 0);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DRect my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                       \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y));   \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA16;
	}
};

class PlainGatherInt2DRect : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DInt(true);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(22.9f, 25.9f, 0, 0);
	}

	virtual std::string Sampler()
	{
		return "uniform isampler2DRect my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                        \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y));    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32I, 1, 1);
	}

	virtual long Verify()
	{
		IVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_INT, &data);
		if (data != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "ivec4";
	}
};

class PlainGatherUint2DRect : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DInt(true);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(22.9f, 25.9f, 0, 0);
	}

	virtual std::string Sampler()
	{
		return "uniform usampler2DRect my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                        \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y));    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual void SetRbo()
	{
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 1, 1);
	}

	virtual long Verify()
	{
		UVec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data);
		if (data != UVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected 0, 4, 8, 12, got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual long VerifyCompute()
	{
		IVec4* data;
		data = static_cast<IVec4*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
		if (data[0] != IVec4(0, 4, 8, 12))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected " << Expected().x() << ", " << Expected().y() << ", "
				<< Expected().z() << ", " << Expected().w() << ", got: " << data[0].x() << ", " << data[0].y() << ", "
				<< data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Type()
	{
		return "uvec4";
	}
};

class PlainGatherDepth2DRect : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2D(true);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23, 26, 13.5 / 16, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform sampler2DRectShadow my_sampler;                     \n";
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                             \n"
						"    return textureGather(my_sampler, vec2(p.x, p.y), p.z);    \n"
						"}\n";
	}

	virtual GLenum InternalFormat()
	{
		return GL_DEPTH_COMPONENT32F;
	}

	virtual GLenum Format()
	{
		return GL_DEPTH_COMPONENT;
	}

	virtual Vec4 Expected()
	{
		return Vec4(1, 1, 0, 0);
	}
};

class OffsetGatherFloat2D : public GatherBase
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 4, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                               \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.z)));    \n"
						"}\n";
	}
};

class OffsetGatherUnorm2D : public PlainGatherUnorm2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 4, 4);
	}

	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                                    \n"
			   "    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.z)));    \n"
			   "}\n";
	}
};

class OffsetGatherInt2D : public PlainGatherInt2DRgba
{
	virtual Vec4 BufferData()
	{
		return Vec4(18.9f / 32.f, 21.9f / 32.f, 4, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                               \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.z)));    \n"
						"}\n";
	}
};

class OffsetGatherUint2D : public PlainGatherUint2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18.9f / 32.f, 21.9f / 32.f, 4, 2);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                          \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.z)), 2);            \n"
						"}\n";
	}
};

class OffsetGatherDepth2D : public PlainGatherDepth2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 4, 13.5 / 16);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                               \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), p.w, ivec2(int(p.z)));    \n"
						"}\n";
	}
};

class OffsetGatherFloat2DArray : public PlainGatherFloat2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 5, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                    \n"
						"    return textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherUnorm2DArray : public PlainGatherUnorm2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 3, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                    \n"
						"    return textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherInt2DArray : public PlainGatherInt2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 11, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                    \n"
						"    return textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), ivec2(int(p.w)), 3); \n"
						"}\n";
	}
};

class OffsetGatherUint2DArray : public PlainGatherUint2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 1, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                    \n"
						"    return textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherDepth2DArray : public PlainGatherDepth2DArray
{
	virtual void Init()
	{
		CreateTexture2DArray(7, 3);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 3, 4);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                                   \n"
			   "    return textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), p.y + (5./32), ivec2(int(p.w)));    \n"
			   "}\n";
	}
};

class OffsetGatherFloat2DRect : public PlainGatherFloat2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(19, 22, 0, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                  \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.w)), 1);    \n"
						"}\n";
	}
};

class OffsetGatherUnorm2DRect : public PlainGatherUnorm2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(30.9f, 18.9f, -8.f, 7.f);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                         \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.z), int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherInt2DRect : public PlainGatherInt2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(22.9f, 25.9f, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                               \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherUint2DRect : public PlainGatherUint2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(26.9f, 29.9f, 0, -4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                               \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetGatherDepth2DRect : public PlainGatherDepth2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(19, 22, 13.5 / 16, 4);
	}

	virtual std::string TextBody()
	{
		return Type() + " test_function(vec4 p) {                                                    \n"
						"    return textureGatherOffset(my_sampler, vec2(p.x, p.y), p.z, ivec2(int(p.w)));    \n"
						"}\n";
	}
};

class OffsetsGatherFloat2D : public OffsetGatherFloat2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherUnorm2D : public OffsetGatherUnorm2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 0, 0);
	}

	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4  test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherInt2D : public OffsetGatherInt2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherUint2D : public OffsetGatherUint2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 2, 2);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets, 2);                      \n"
			   "}\n";
	}
};

class OffsetsGatherDepth2D : public OffsetGatherDepth2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 0.49f, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), p.z, offsets);                    \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(0, 0, 1, 1);
	}
};

class OffsetsGatherFloat2DArray : public PlainGatherFloat2DArray
{
	virtual void Init()
	{
		CreateTexture2DArray(3, 1);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 1, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec3(p.x, p.y, p.z), offsets);                    \n"
			   "}\n";
	}
};

class OffsetsGatherUnorm2DArray : public PlainGatherUnorm2DArray
{
	virtual void Init()
	{
		CreateTexture2DArray(3, 1);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 1, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec3(p.x, p.y, p.z), offsets);                    \n"
			   "}\n";
	}
};

class OffsetsGatherInt2DArray : public PlainGatherInt2DArray
{
	virtual void Init()
	{
		CreateTexture2DArrayInt(3, 1);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 1, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec3(p.x, p.y, p.z), offsets, 3);                 \n"
			   "}\n";
	}
};

class OffsetsGatherUint2DArray : public PlainGatherUint2DArray
{
	virtual void Init()
	{
		CreateTexture2DArrayInt(3, 1);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 1, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec3(p.x, p.y, p.z), offsets);                    \n"
			   "}\n";
	}
};

class OffsetsGatherDepth2DArray : public PlainGatherDepth2DArray
{
	virtual void Init()
	{
		CreateTexture2DArray(3, 1);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	}

	virtual Vec4 BufferData()
	{
		return Vec4(18. / 32, 8. / 32, 1, 0.49f);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec3(p.x, p.y, p.z), p.w, offsets);               \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(0, 0, 1, 1);
	}
};

class OffsetsGatherFloat2DRect : public PlainGatherFloat2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(18, 8, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets, 1);                      \n"
			   "}\n";
	}
};

class OffsetsGatherUnorm2DRect : public PlainGatherUnorm2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(18, 8, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherInt2DRect : public PlainGatherUint2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(17.9f, 7.9f, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherUint2DRect : public PlainGatherUint2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(17.9f, 7.9f, 0, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), offsets);                         \n"
			   "}\n";
	}
};

class OffsetsGatherDepth2DRect : public PlainGatherDepth2DRect
{
	virtual Vec4 BufferData()
	{
		return Vec4(17.9f, 7.9f, 0.49f, 0);
	}

	virtual std::string TextBody()
	{
		return Type() +
			   " test_function(vec4 p) {                                                             \n"
			   "    const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3)); \n"
			   "    return textureGatherOffsets(my_sampler, vec2(p.x, p.y), p.z, offsets);                    \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(0, 0, 1, 1);
	}
};

class Swizzle : public PlainGatherFloat2DRgba
{
	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                    \n"
			   "    return textureGather(my_sampler, vec2(p.x, p.y), 1).yzww;   \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(5. / 16, 9. / 16, 13. / 16, 13. / 16);
	}
};

class BaseLevel : public PlainGatherFloat2DRgba
{
	virtual void Init()
	{
		CreateTexture2D(false, true);
	}
};

class IncompleteTexture : public PlainGatherFloat2DRgba
{
	virtual void Init()
	{
		CreateTexture2D();
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, 0);
	}

	virtual Vec4 Expected()
	{
		return Vec4(0);
	}
};

class IncompleteTextureLastComp : public PlainGatherFloat2DRgba
{
	virtual void Init()
	{
		CreateTexture2D();
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, 0);
	}

	virtual Vec4 Expected()
	{
		return Vec4(1);
	}

	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                    \n"
			   "    return textureGather(my_sampler, vec2(p.x, p.y), 3);        \n"
			   "}\n";
	}
};

class TriangleDraw : public GatherBase
{
	GLuint program, rbo, fbo, vao, vbo;

	virtual std::string VertexShader()
	{
		return "#version 400                                  \n"
			   "out vec2 texcoords;                           \n"
			   "in vec4 Vertex;                               \n"
			   "void main() {                                 \n"
			   "   gl_Position = Vertex;                      \n"
			   "   texcoords = (Vertex.xy + vec2(1.0)) / 2.0; \n"
			   "}\n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 400                                      \n"
			   "in vec2 texcoords;                                \n"
			   "out vec4 FragColor;                               \n"
			   "uniform sampler2D tex;                            \n"
			   "void main() {                                     \n"
			   "   FragColor = textureGather(tex, texcoords, 2);  \n"
			   "}\n";
	}

	virtual long Run()
	{
		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		GLfloat colorf[4] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, colorf);
		glViewport(0, 0, 100, 100);

		program = CreateProgram(VertexShader().c_str(), NULL, NULL, NULL, FragmentShader().c_str());
		glBindAttribLocation(program, 0, "Vertex");
		glBindFragDataLocation(program, 0, "FragColor");
		glLinkProgram(program);
		if (!CheckProgram(program))
			return ERROR;
		glUseProgram(program);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		std::vector<Vec4> data(100 * 100, Vec4(0.25, 0.5, 0.75, 1));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 100, 100, 0, GL_RGBA, GL_FLOAT, &data[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		GLuint l_vao;
		glGenVertexArrays(1, &l_vao);
		glBindVertexArray(l_vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		GLfloat buffData[16] = { -1, 1, 0, 1, -1, -1, 0, 1, 1, 1, 0, 1, 1, -1, 0, 1 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(buffData), buffData, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);
		glDeleteVertexArrays(1, &l_vao);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		std::vector<Vec4> read(100 * 100, Vec4(0));
		glReadPixels(0, 0, 100, 100, GL_RGBA, GL_FLOAT, &read[0]);
		for (unsigned int i = 0; i < read.size(); ++i)
		{
			if (read[i] != Vec4(0.75))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Got: " << read[i].x() << " " << read[i].y() << " " << read[i].z()
					<< " " << read[i].w() << ", expected vec4(0.25)" << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glViewport(0, 0, GetWindowWidth(), GetWindowHeight());
		glDeleteTextures(1, &tex);
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteProgram(program);
		return NO_ERROR;
	}
};

class PlainGatherFloat2DSrgb : public GatherBase
{
public:
	virtual Vec4 Expected()
	{
		return Vec4(0.f, 0.0512695f, 0.21582f, 0.519531f);
	}

	virtual GLenum InternalFormat()
	{
		return GL_SRGB8_ALPHA8;
	}
};

class PlainGatherFloat2DSrgbAlpha : public GatherBase
{
public:
	virtual std::string TestFunction()
	{
		return "uniform sampler2D my_sampler;                                   \n"
			   ""
			   "vec4 test_function(vec4 p) {                                    \n"
			   "    return textureGather(my_sampler, vec2(p.x, p.y), 3);        \n"
			   "}\n";
	}

	virtual Vec4 Expected()
	{
		return Vec4(3. / 16, 7. / 16, 11. / 16, 15. / 16);
	}

	virtual GLenum InternalFormat()
	{
		return GL_SRGB8_ALPHA8;
	}
};

class GatherGeometryShader : public GatherBase
{
	virtual std::string VertexShader()
	{
		return "#version 400                       \n"
			   "in vec4 v_in_0;                    \n"
			   "out vec4 d;                        \n"
			   "void main() {                      \n"
			   "   gl_Position = vec4(0, 0, 0, 1); \n"
			   "   d = v_in_0;                     \n"
			   "}";
	};

	virtual std::string GeometryShader()
	{
		return "#version 400                                                  \n"
			   "layout(points) in;                                            \n"
			   "layout(points, max_vertices = 1) out;                         \n"
			   "out vec4 col;                                                 \n"
			   "in vec4 d[1];                                                 \n"
			   "uniform sampler2D tex;                                        \n"
			   ""
			   "void main() {                                                 \n"
			   "   vec4 v1 = textureGather(tex, vec2(d[0].x, d[0].y));                                               \n"
			   "   vec4 v2 = textureGatherOffset(tex, vec2(d[0].x, d[0].y) - vec2(4./32), ivec2(4));                 \n"
			   "   const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3));         \n"
			   "   vec4 v3 = textureGatherOffsets(tex, vec2(d[0].x, d[0].y) - vec2(5./32, 18./32), offsets);         \n"
			   "   vec4 expected = vec4(0./16, 4./16, 8./16, 12./16);                                                \n"
			   "   float error = 0.0;                                         \n"
			   ""
			   "   if (v1 != expected)    \n"
			   "       error += 0.01;     \n"
			   "   if (v2 != expected)    \n"
			   "       error += 0.02;     \n"
			   "   if (v3 != expected)    \n"
			   "       error += 0.04;     \n"
			   ""
			   "   if (error != 0.0)                         \n"
			   "       col = vec4(v1.x, v2.y, v3.z, d[0].x); \n"
			   "   else                                      \n"
			   "       col = vec4(0, 1, 0, 1);               \n"
			   ""
			   "   gl_Position = vec4(0, 0, 0, 1);         \n"
			   "   EmitVertex();                           \n"
			   "   EndPrimitive();                         \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 400               \n"
			   "in vec4 col;               \n"
			   "out vec4 f_out_0;          \n"
			   "void main() {              \n"
			   "   f_out_0 = col;          \n"
			   "}";
	}

	virtual long Run()
	{
		if (!Supported())
			return NO_ERROR;
		Init();

		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		SetRbo();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		GLfloat colorf[4] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, colorf);
		glViewport(0, 0, 1, 1);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);
		Vec4 buffData = BufferData();
		glBufferData(GL_ARRAY_BUFFER, 16, &buffData, GL_STATIC_DRAW);

		program = CreateProgram(VertexShader().c_str(), NULL, NULL, GeometryShader().c_str(), FragmentShader().c_str());
		glBindAttribLocation(program, 0, "v_in_0");
		glBindFragDataLocation(program, 0, "f_out_0");
		glLinkProgram(program);
		if (!CheckProgram(program))
			return ERROR;
		glUseProgram(program);

		glDrawArrays(GL_POINTS, 0, 1);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glDeleteProgram(program);
		Vec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, &data);
		if (!ColorEqual(data, Vec4(0, 1, 0, 1), g_color_eps))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected Vec4(0, 1, 0, 1), got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << ", epsilon: " << g_color_eps.x() << ", " << g_color_eps.y() << ", "
				<< g_color_eps.z() << ", " << g_color_eps.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};

class GatherTesselationShader : public GatherBase
{
	virtual std::string VertexShader()
	{
		return "#version 400               \n"
			   "in vec4 v_in_0;            \n"
			   "out vec4 d;                \n"
			   "void main() {              \n"
			   "   gl_Position = vec4(0, 0, 0, 1); \n"
			   "   d = v_in_0;             \n"
			   "}";
	}

	virtual std::string ControlShader()
	{
		return "#version 400                                                  \n"
			   "layout(vertices = 1) out;                                     \n"
			   "out vec4 ccol[];                                              \n"
			   "in vec4 d[];                                                  \n"
			   "out vec4 d_con[];                                             \n"
			   "uniform sampler2D tex;                                        \n"
			   ""
			   "void main() {                                        \n"
			   "   vec4 v1 = textureGather(tex, vec2(d[0].x, d[0].y));                                               \n"
			   "   vec4 v2 = textureGatherOffset(tex, vec2(d[0].x, d[0].y) - vec2(4./32), ivec2(4));                 \n"
			   "   const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3));         \n"
			   "   vec4 v3 = textureGatherOffsets(tex, vec2(d[0].x, d[0].y) - vec2(5./32, 18./32), offsets);         \n"
			   "   vec4 expected = vec4(0./16, 4./16, 8./16, 12./16);                                                \n"
			   "   float error = 0.0;                                         \n"
			   ""
			   "   if (v1 != expected)    \n"
			   "       error += 0.01;     \n"
			   "   if (v2 != expected)    \n"
			   "       error += 0.02;     \n"
			   "   if (v3 != expected)    \n"
			   "       error += 0.04;     \n"
			   ""
			   "   if (error != 0.0)                                            \n"
			   "       ccol[gl_InvocationID] = vec4(v1.x, v2.y, d[0].x, 1);      \n"
			   "   else                                                         \n"
			   "       ccol[gl_InvocationID] = vec4(0, 1, 0, 1);                \n"
			   ""
			   "   d_con[gl_InvocationID] = d[gl_InvocationID];                                 \n"
			   "   gl_out[gl_InvocationID].gl_Position = vec4(0, 0, 0, 1);                      \n"
			   "   gl_TessLevelInner[0] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[0] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[1] = 1.0;                                                  \n"
			   "   gl_TessLevelOuter[2] = 1.0;                                                  \n"
			   "}";
	}

	virtual std::string EvalShader()
	{
		return "#version 400                                                  \n"
			   "layout(triangles, point_mode) in;                             \n"
			   "in vec4 ccol[];                                               \n"
			   "in vec4 d_con[];                                              \n"
			   "out vec4 ecol;                                                \n"
			   "uniform sampler2D tex;                                        \n"
			   ""
			   "void main() {                                        \n"
			   "   vec4 v1 = textureGather(tex, vec2(d_con[0].x, d_con[0].y));                                         "
			   "      \n"
			   "   vec4 v2 = textureGatherOffset(tex, vec2(d_con[0].x, d_con[0].y) - vec2(4./32), ivec2(4));           "
			   "      \n"
			   "   const ivec2 offsets[4] = ivec2[](ivec2(7, 6), ivec2(-8, 7), ivec2(-6, -5), ivec2(-1, 3));           "
			   "      \n"
			   "   vec4 v3 = textureGatherOffsets(tex, vec2(d_con[0].x, d_con[0].y) - vec2(5./32, 18./32), offsets);   "
			   "      \n"
			   "   vec4 expected = vec4(0./16, 4./16, 8./16, 12./16);                                                  "
			   "      \n"
			   "   float error = 0.0;                                         \n"
			   ""
			   "   if (v1 != expected)    \n"
			   "       error += 0.01;     \n"
			   "   if (v2 != expected)    \n"
			   "       error += 0.02;     \n"
			   "   if (v3 != expected)    \n"
			   "       error += 0.04;     \n"
			   ""
			   "   if (error != 0.0)                                          \n"
			   "       ecol = vec4(v1.x, v2.y, d_con[0].x, -1);                      \n"
			   "   else if (ccol[0] != vec4(0, 1, 0, 1))     \n"
			   "       ecol = ccol[0];                       \n"
			   "   else                                      \n"
			   "       ecol = vec4(0, 1, 0, 1);              \n"
			   ""
			   "   gl_Position = vec4(0, 0, 0, 1);   \n"
			   "}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 400               \n"
			   "in vec4 ecol;              \n"
			   "out vec4 f_out_0;          \n"
			   "void main() {              \n"
			   "   f_out_0 = ecol;         \n"
			   "}";
	}

	virtual long Run()
	{

		if (!Supported())
			return NO_ERROR;
		Init();

		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		SetRbo();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		GLfloat colorf[4] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, colorf);
		glViewport(0, 0, 1, 1);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(0);
		Vec4 buffData = BufferData();
		glBufferData(GL_ARRAY_BUFFER, 16, &buffData, GL_STATIC_DRAW);

		program = CreateProgram(VertexShader().c_str(), ControlShader().c_str(), EvalShader().c_str(), NULL,
								FragmentShader().c_str());
		glBindAttribLocation(program, 0, "v_in_0");
		glBindFragDataLocation(program, 0, "f_out_0");
		glLinkProgram(program);
		if (!CheckProgram(program))
			return ERROR;
		glUseProgram(program);

		glPatchParameteri(GL_PATCH_VERTICES, 1);
		glDrawArrays(GL_PATCHES, 0, 1);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glDeleteProgram(program);
		Vec4 data;
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, &data);
		if (!ColorEqual(data, Vec4(0, 1, 0, 1), g_color_eps))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected Vec4(0, 1, 0, 1), got: " << data.x() << ", " << data.y() << ", "
				<< data.z() << ", " << data.w() << ", epsilon: " << g_color_eps.x() << ", " << g_color_eps.y() << ", "
				<< g_color_eps.z() << ", " << g_color_eps.w() << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}
};

class PlainGatherFloat2DRgb : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DRgb();
	}
};

class PlainGatherFloat2DR : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DR();
	}
};

class OffsetGatherFloat2DRgb : public OffsetGatherFloat2D
{
public:
	virtual void Init()
	{
		CreateTexture2DRgb();
	}
};

class OffsetGatherFloat2DRg : public OffsetGatherFloat2D
{
public:
	virtual void Init()
	{
		CreateTexture2DRg();
	}
};

class OffsetGatherFloat2DR : public OffsetGatherFloat2D
{
public:
	virtual void Init()
	{
		CreateTexture2DR();
	}
};

} // anonymous namespace

TextureGatherTests::TextureGatherTests(deqp::Context& context) : TestCaseGroup(context, "texture_gather", "")
{
}

TextureGatherTests::~TextureGatherTests(void)
{
}

void TextureGatherTests::init()
{
	using namespace deqp;
	addChild(new TestSubcase(m_context, "api-enums", TestSubcase::Create<GatherEnumsTest>));
	addChild(new TestSubcase(m_context, "gather-glsl-compile", TestSubcase::Create<GatherGLSLCompile>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-rgba", TestSubcase::Create<PlainGatherFloat2DRgba>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-rg", TestSubcase::Create<PlainGatherFloat2DRg>));
	addChild(new TestSubcase(m_context, "plain-gather-unorm-2d", TestSubcase::Create<PlainGatherUnorm2D>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2d-rgba", TestSubcase::Create<PlainGatherInt2DRgba>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2d-rg", TestSubcase::Create<PlainGatherInt2DRg>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-2d", TestSubcase::Create<PlainGatherUint2D>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-2d", TestSubcase::Create<PlainGatherDepth2D>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2darray", TestSubcase::Create<PlainGatherFloat2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-unorm-2darray", TestSubcase::Create<PlainGatherUnorm2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2darray", TestSubcase::Create<PlainGatherInt2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-2darray", TestSubcase::Create<PlainGatherUint2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-2darray", TestSubcase::Create<PlainGatherDepth2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-float-cube-rgba", TestSubcase::Create<PlainGatherFloatCubeRgba>));
	addChild(new TestSubcase(m_context, "plain-gather-float-cube-rg", TestSubcase::Create<PlainGatherFloatCubeRg>));
	addChild(new TestSubcase(m_context, "plain-gather-unorm-cube", TestSubcase::Create<PlainGatherUnormCube>));
	addChild(new TestSubcase(m_context, "plain-gather-int-cube-rgba", TestSubcase::Create<PlainGatherIntCubeRgba>));
	addChild(new TestSubcase(m_context, "plain-gather-int-cube-rg", TestSubcase::Create<PlainGatherIntCubeRg>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-cube", TestSubcase::Create<PlainGatherUintCube>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-cube", TestSubcase::Create<PlainGatherDepthCube>));
	addChild(
		new TestSubcase(m_context, "plain-gather-float-cube-array", TestSubcase::Create<PlainGatherFloatCubeArray>));
	addChild(
		new TestSubcase(m_context, "plain-gather-unorm-cube-array", TestSubcase::Create<PlainGatherUnormCubeArray>));
	addChild(new TestSubcase(m_context, "plain-gather-int-cube-array", TestSubcase::Create<PlainGatherIntCubeArray>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-cube-array", TestSubcase::Create<PlainGatherUintCubeArray>));
	addChild(
		new TestSubcase(m_context, "plain-gather-depth-cube-array", TestSubcase::Create<PlainGatherDepthCubeArray>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2drect", TestSubcase::Create<PlainGatherFloat2DRect>));
	addChild(new TestSubcase(m_context, "plain-gather-unorm-2drect", TestSubcase::Create<PlainGatherUnorm2DRect>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2drect", TestSubcase::Create<PlainGatherInt2DRect>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-2drect", TestSubcase::Create<PlainGatherUint2DRect>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-2drect", TestSubcase::Create<PlainGatherDepth2DRect>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d", TestSubcase::Create<OffsetGatherFloat2D>));
	addChild(new TestSubcase(m_context, "offset-gather-unorm-2d", TestSubcase::Create<OffsetGatherUnorm2D>));
	addChild(new TestSubcase(m_context, "offset-gather-int-2d", TestSubcase::Create<OffsetGatherInt2D>));
	addChild(new TestSubcase(m_context, "offset-gather-uint-2d", TestSubcase::Create<OffsetGatherUint2D>));
	addChild(new TestSubcase(m_context, "offset-gather-depth-2d", TestSubcase::Create<OffsetGatherDepth2D>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2darray", TestSubcase::Create<OffsetGatherFloat2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-unorm-2darray", TestSubcase::Create<OffsetGatherUnorm2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-int-2darray", TestSubcase::Create<OffsetGatherInt2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-uint-2darray", TestSubcase::Create<OffsetGatherUint2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-depth-2darray", TestSubcase::Create<OffsetGatherDepth2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2drect", TestSubcase::Create<OffsetGatherFloat2DRect>));
	addChild(new TestSubcase(m_context, "offset-gather-unorm-2drect", TestSubcase::Create<OffsetGatherUnorm2DRect>));
	addChild(new TestSubcase(m_context, "offset-gather-int-2drect", TestSubcase::Create<OffsetGatherInt2DRect>));
	addChild(new TestSubcase(m_context, "offset-gather-uint-2drect", TestSubcase::Create<OffsetGatherUint2DRect>));
	addChild(new TestSubcase(m_context, "offset-gather-depth-2drect", TestSubcase::Create<OffsetGatherDepth2DRect>));
	addChild(new TestSubcase(m_context, "offsets-gather-float-2d", TestSubcase::Create<OffsetsGatherFloat2D>));
	addChild(new TestSubcase(m_context, "offsets-gather-unorm-2d", TestSubcase::Create<OffsetsGatherUnorm2D>));
	addChild(new TestSubcase(m_context, "offsets-gather-int-2d", TestSubcase::Create<OffsetsGatherInt2D>));
	addChild(new TestSubcase(m_context, "offsets-gather-uint-2d", TestSubcase::Create<OffsetsGatherUint2D>));
	addChild(new TestSubcase(m_context, "offsets-gather-depth-2d", TestSubcase::Create<OffsetsGatherDepth2D>));
	addChild(
		new TestSubcase(m_context, "offsets-gather-float-2darray", TestSubcase::Create<OffsetsGatherFloat2DArray>));
	addChild(
		new TestSubcase(m_context, "offsets-gather-unorm-2darray", TestSubcase::Create<OffsetsGatherUnorm2DArray>));
	addChild(new TestSubcase(m_context, "offsets-gather-int-2darray", TestSubcase::Create<OffsetsGatherInt2DArray>));
	addChild(new TestSubcase(m_context, "offsets-gather-uint-2darray", TestSubcase::Create<OffsetsGatherUint2DArray>));
	addChild(
		new TestSubcase(m_context, "offsets-gather-depth-2darray", TestSubcase::Create<OffsetsGatherDepth2DArray>));
	addChild(new TestSubcase(m_context, "offsets-gather-float-2drect", TestSubcase::Create<OffsetsGatherFloat2DRect>));
	addChild(new TestSubcase(m_context, "offsets-gather-unorm-2drect", TestSubcase::Create<OffsetsGatherUnorm2DRect>));
	addChild(new TestSubcase(m_context, "offsets-gather-int-2drect", TestSubcase::Create<OffsetsGatherInt2DRect>));
	addChild(new TestSubcase(m_context, "offsets-gather-uint-2drect", TestSubcase::Create<OffsetsGatherUint2DRect>));
	addChild(new TestSubcase(m_context, "offsets-gather-depth-2drect", TestSubcase::Create<OffsetsGatherDepth2DRect>));
	addChild(new TestSubcase(m_context, "swizzle", TestSubcase::Create<Swizzle>));
	addChild(new TestSubcase(m_context, "base-level", TestSubcase::Create<BaseLevel>));
	addChild(new TestSubcase(m_context, "incomplete-texture", TestSubcase::Create<IncompleteTexture>));
	addChild(
		new TestSubcase(m_context, "incomplete-texture-last-comp", TestSubcase::Create<IncompleteTextureLastComp>));
	addChild(new TestSubcase(m_context, "triangle-draw", TestSubcase::Create<TriangleDraw>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-srgb", TestSubcase::Create<PlainGatherFloat2DSrgb>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-srgb-alpha",
							 TestSubcase::Create<PlainGatherFloat2DSrgbAlpha>));
	addChild(new TestSubcase(m_context, "gather-geometry-shader", TestSubcase::Create<GatherGeometryShader>));
	addChild(new TestSubcase(m_context, "gather-tesselation-shader", TestSubcase::Create<GatherTesselationShader>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-rgb", TestSubcase::Create<PlainGatherFloat2DRgb>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-r", TestSubcase::Create<PlainGatherFloat2DR>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-rgb", TestSubcase::Create<OffsetGatherFloat2DRgb>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-rg", TestSubcase::Create<OffsetGatherFloat2DRg>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-r", TestSubcase::Create<OffsetGatherFloat2DR>));
}

} // gl4cts namespace
