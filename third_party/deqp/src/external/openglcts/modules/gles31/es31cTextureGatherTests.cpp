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

#include "es31cTextureGatherTests.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include "tcuMatrixUtil.hpp"
#include "tcuRenderTarget.hpp"
#include <cstdarg>
#include <sstream>
#include <string>
#include <vector>

namespace glcts
{

using namespace glw;
using tcu::Vec4;
using tcu::Vec3;
using tcu::Vec2;
using tcu::IVec4;
using tcu::UVec4;

namespace
{

class TGBase : public glcts::SubcaseBase
{
public:
	virtual ~TGBase()
	{
	}

	TGBase() : renderTarget(m_context.getRenderContext().getRenderTarget()), pixelFormat(renderTarget.getPixelFormat())
	{
	}

	const tcu::RenderTarget& renderTarget;
	const tcu::PixelFormat&  pixelFormat;

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

	GLuint CreateProgram(const char* src_vs, const char* src_fs)
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
		return "uniform mediump sampler2D tex_2d;                  \n"
			   "uniform mediump isamplerCube itex_cube;            \n"
			   "uniform mediump usampler2DArray utex_2da;          \n"
			   ""
			   "uniform mediump sampler2DShadow tex_2ds;           \n"
			   "uniform mediump samplerCubeShadow tex_cubes;       \n"
			   "uniform mediump sampler2DArrayShadow tex_2das;     \n";
	}

	virtual std::string Sampling()
	{
		return "    textureGather(tex_2d,vec2(1));          \n"
			   "    textureGather(itex_cube,vec3(1));       \n"
			   "    textureGather(utex_2da,vec3(1));        \n"
			   ""
			   "    textureGather(tex_2ds,vec2(1), 0.5);    \n"
			   "    textureGather(tex_cubes,vec3(1), 0.5);  \n"
			   "    textureGather(tex_2das,vec3(1), 0.5);   \n"
			   ""
			   "    textureGatherOffset(tex_2d,vec2(1), ivec2(0));          \n"
			   "    textureGatherOffset(utex_2da,vec3(1), ivec2(0));        \n"
			   ""
			   "    textureGatherOffset(tex_2ds,vec2(1), 0.5, ivec2(0));    \n"
			   "    textureGatherOffset(tex_2das,vec3(1), 0.5, ivec2(0));   \n";
	}

	virtual std::string VertexShader()
	{
		return "#version 310 es                               \n" + Uniforms() +
			   "  void main() {                            \n" + Sampling() +
			   "    gl_Position = vec4(1);                 \n"
			   "  }                                        \n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 310 es                            \n"
			   "precision highp float;                     \n"
			   "out mediump vec4 color;                    \n" +
			   Uniforms() + "  void main() {                            \n" + Sampling() +
			   "    color = vec4(1);                       \n"
			   "  }                                        \n";
	}

	virtual long Run()
	{
		program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str());
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

	bool IsFloatingPointTexture(GLenum internal_format)
	{
		switch (internal_format)
		{
		case GL_R32F:
		case GL_RG32F:
		case GL_RGB32F:
		case GL_RGBA32F:
		case GL_DEPTH_COMPONENT32F:
			return true;
		}

		return false;
	}

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
			std::vector<Vec3> pixels(size * size, Vec3(1.0));
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, &pixels[0]);
		}

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

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
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
			std::vector<Vec2> pixels(size * size, Vec2(1.0));
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, &pixels[0]);
		}

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

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
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
			std::vector<GLfloat> pixels(size * size, 1.0);
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, &pixels[0]);
		}

		GLfloat data[4] = { 12. / 16., 8. / 16., 0. / 16., 4. / 16. };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, tex_type, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, tex_type, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, tex_type, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
	}

	virtual GLvoid CreateTexture2DInt()
	{
		GLenum		internal_format = InternalFormat();
		const GLint csize			= 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= Type().find('u') != std::string::npos ? GL_UNSIGNED_INT : GL_INT;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage2D(target, i, internal_format, size, size, 0, GL_RGBA_INTEGER, tex_type, 0);
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		glTexSubImage2D(target, 0, 0, 0, csize, csize, GL_RGBA_INTEGER, tex_type, &pixels[0]);

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		glTexSubImage2D(target, 0, 22, 25, 2, 2, GL_RGBA_INTEGER, tex_type, data);
		glTexSubImage2D(target, 0, 16, 10, 1, 1, GL_RGBA_INTEGER, tex_type, data + 0);
		glTexSubImage2D(target, 0, 11, 2, 1, 1, GL_RGBA_INTEGER, tex_type, data + 1);
		glTexSubImage2D(target, 0, 24, 13, 1, 1, GL_RGBA_INTEGER, tex_type, data + 2);
		glTexSubImage2D(target, 0, 9, 14, 1, 1, GL_RGBA_INTEGER, tex_type, data + 3);

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
		GLenum		tex_type		= Type().find('u') != std::string::npos ? GL_UNSIGNED_INT : GL_INT;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, size, size, slices, 0, GL_RGBA_INTEGER, tex_type, 0);
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		for (int i = 0; i < slices; ++i)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, csize, csize, 1, GL_RGBA_INTEGER, tex_type, &pixels[0]);
		}

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 22, 25, data_slice, 2, 2, 1, GL_RGBA_INTEGER, tex_type, data);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 16, 10, data_slice, 1, 1, 1, GL_RGBA_INTEGER, tex_type, data + 0);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 11, 2, data_slice, 1, 1, 1, GL_RGBA_INTEGER, tex_type, data + 1);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 24, 13, data_slice, 1, 1, 1, GL_RGBA_INTEGER, tex_type, data + 2);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 9, 14, data_slice, 1, 1, 1, GL_RGBA_INTEGER, tex_type, data + 3);

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
			std::vector<Vec4> pixels(size * size * slices, Vec4(1.0));
			glTexImage3D(GL_TEXTURE_2D_ARRAY, i, internal_format, size, size, slices, 0, format, GL_FLOAT, &pixels[0]);
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

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
	}

	virtual GLvoid CreateTextureCubeInt()
	{
		GLenum		internal_format = InternalFormat();
		const GLint csize			= 32;
		GLint		size			= csize;
		GLenum		tex_type		= Type().find('u') != std::string::npos ? GL_UNSIGNED_INT : GL_INT;

		const GLenum faces[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
								  GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			for (int j = 0; j < 6; ++j)
			{
				glTexImage2D(faces[j], i, internal_format, size, size, 0, GL_RGBA_INTEGER, tex_type, 0);
			}
		}
		std::vector<IVec4> pixels(csize * csize, IVec4(999));
		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 0, 0, csize, csize, GL_RGBA_INTEGER, tex_type, &pixels[0]);
		}

		IVec4 data[4] = { IVec4(12, 13, 14, 15), IVec4(8, 9, 10, 11), IVec4(0, 1, 2, 3), IVec4(4, 5, 6, 7) };

		for (int j = 0; j < 6; ++j)
		{
			glTexSubImage2D(faces[j], 0, 22, 25, 2, 2, GL_RGBA_INTEGER, tex_type, data);
			glTexSubImage2D(faces[j], 0, 16, 10, 1, 1, GL_RGBA_INTEGER, tex_type, data + 0);
			glTexSubImage2D(faces[j], 0, 11, 2, 1, 1, GL_RGBA_INTEGER, tex_type, data + 1);
			glTexSubImage2D(faces[j], 0, 24, 13, 1, 1, GL_RGBA_INTEGER, tex_type, data + 2);
			glTexSubImage2D(faces[j], 0, 9, 14, 1, 1, GL_RGBA_INTEGER, tex_type, data + 3);
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
			std::vector<Vec4> pixels(size * size, Vec4(1.0));
			for (int j = 0; j < 6; ++j)
			{
				glTexImage2D(faces[j], i, internal_format, size, size, 0, format, GL_FLOAT, &pixels[0]);
			}
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

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
	}

	virtual GLvoid CreateTextureSRGB()
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= GL_UNSIGNED_BYTE;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, 0);
		}
		std::vector<GLubyte> pixels(csize * csize * 4, 255);
		glTexSubImage2D(target, 0, 0, 0, csize, csize, format, tex_type, &pixels[0]);

		if (format != GL_DEPTH_COMPONENT)
		{
			glGenerateMipmap(target);
		}

		GLubyte data[16] = { 240, 13, 14, 15, 160, 9, 10, 11, 0, 1, 2, 3, 80, 5, 6, 7 };

		glTexSubImage2D(target, 0, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, 0, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, 0, 11, 2, 1, 1, format, tex_type, data + 4);
		glTexSubImage2D(target, 0, 24, 13, 1, 1, format, tex_type, data + 8);
		glTexSubImage2D(target, 0, 9, 14, 1, 1, format, tex_type, data + 12);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}

	virtual GLvoid CreateTexture2D(bool base_level = false)
	{
		GLenum		internal_format = InternalFormat();
		GLenum		format			= Format();
		const GLint csize			= base_level ? 64 : 32;
		GLint		size			= csize;
		GLenum		target			= GL_TEXTURE_2D;
		GLenum		tex_type		= InternalFormat() == GL_SRGB8_ALPHA8 ? GL_UNSIGNED_BYTE : GL_FLOAT;

		glGenTextures(1, &tex);
		glBindTexture(target, tex);
		for (int i = 0; size > 0; ++i, size /= 2)
		{
			std::vector<Vec4> pixels(size * size, Vec4(1.0));
			glTexImage2D(target, i, internal_format, size, size, 0, format, tex_type, &pixels[0]);
		}

		Vec4 data[4] = { Vec4(12. / 16, 13. / 16, 14. / 16, 15. / 16), Vec4(8. / 16, 9. / 16, 10. / 16, 11. / 16),
						 Vec4(0. / 16, 1. / 16, 2. / 16, 3. / 16), Vec4(4. / 16, 5. / 16, 6. / 16, 7. / 16) };

		glTexSubImage2D(target, base_level, 22, 25, 2, 2, format, tex_type, data);
		glTexSubImage2D(target, base_level, 16, 10, 1, 1, format, tex_type, data + 0);
		glTexSubImage2D(target, base_level, 11, 2, 1, 1, format, tex_type, data + 1);
		glTexSubImage2D(target, base_level, 24, 13, 1, 1, format, tex_type, data + 2);
		glTexSubImage2D(target, base_level, 9, 14, 1, 1, format, tex_type, data + 3);

		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		if (IsFloatingPointTexture(internal_format))
		{
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}

		if (base_level)
			glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 1);
	}

	virtual std::string FallthroughVertexShader()
	{
		return "#version 310 es                       \n"
			   "in vec4 v_in_0;                    \n"
			   "flat out vec4 v_out_0;             \n"
			   "void main() {                      \n"
			   "    gl_Position = vec4(0,0,0,1);   \n"
			   "#ifdef GL_ES                       \n"
			   "    gl_PointSize = 1.0f;           \n"
			   "#endif                             \n"
			   "    v_out_0 = v_in_0;              \n"
			   "}";
	}

	virtual std::string FallthroughFragmentShader()
	{
		return "#version 310 es                          \n"
			   "precision highp float;                   \n"
			   "out mediump vec4 f_out_0;                \n"
			   "flat in  mediump vec4 v_out_0;           \n"
			   "void main() {                            \n"
			   "    f_out_0 = v_out_0;                   \n"
			   "}";
	}

	virtual std::string TestFunction()
	{
		return Sampler() + TextBody();
	}

	virtual std::string Sampler()
	{
		return "uniform mediump sampler2D my_sampler;                  \n";
	}

	virtual std::string Type()
	{
		return "vec4";
	}

	virtual std::string TextBody()
	{
		std::string str_if = "    if (all(lessThanEqual(abs(tmp - " + Expected() + "), vec4(0.039)))) {           \n";
		if (Type().find('u') != std::string::npos || Type().find('i') != std::string::npos)
		{
			str_if = "    if (tmp == " + Expected() + ") {           \n";
		}
		return "vec4 test_function(vec4 p) {                                                                         "
			   "\n" +
			   Offset() + "    mediump " + Type() + " tmp = " + Gather() +
			   ";                                                   \n" + str_if +
			   "        return vec4(0.0, 1.0, 0.0, 1.0);                                                             \n"
			   "    } else {                                                                                         \n"
			   "        return vec4(float(tmp.x), float(tmp.y), float(tmp.z), float(tmp.w));                         \n"
			   "    }                                                                                                \n"
			   "}\n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y))";
	}

	virtual std::string Offset()
	{
		return "";
	}

	virtual std::string VertexShader()
	{
		return "#version 310 es                                       \n"
			   ""
			   "in mediump vec4 v_in_0;                            \n"
			   "flat out mediump vec4 v_out_0;                     \n" +
			   TestFunction() + "void main() {                                      \n"
								"    gl_Position = vec4(0, 0, 0, 1);                \n"
								"#ifdef GL_ES                                       \n"
								"    gl_PointSize = 1.0f;                           \n"
								"#endif                                             \n"
								"    v_out_0 = test_function(v_in_0);               \n"
								"}";
	}

	virtual std::string FragmentShader()
	{
		return "#version 310 es                                       \n"
			   ""
			   "precision highp float;                             \n"
			   "flat in mediump vec4 v_out_0;                      \n"
			   "out mediump vec4 f_out_0;                          \n" +
			   TestFunction() + "void main() {                                      \n"
								"    f_out_0 = test_function(v_out_0);              \n"
								"}";
	}

	virtual std::string ComputeShader()
	{
		return "#version 310 es                                       \n"
			   "layout(local_size_x = 1, local_size_y = 1) in;        \n"
			   "layout(std430) buffer Output {                        \n"
			   "  mediump vec4 data;                                  \n"
			   "} g_out;                                              \n"
			   "uniform mediump vec4 cs_in;                           \n" +
			   TestFunction() + "void main() {                                         \n"
								"  g_out.data = test_function(cs_in);                  \n"
								"}                                                     \n";
	}

	virtual void Init()
	{
		CreateTexture2D();
	}

	virtual long Verify()
	{
		std::vector<GLubyte> data(4);
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		if (data[0] != 0 || data[1] != 255 || data[2] != 0 || data[3] != 255)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected Vec4(0, 255, 0, 255), got: " << data[0] << ", " << data[1] << ", "
				<< data[2] << ", " << data[3] << tcu::TestLog::EndMessage;
			return ERROR;
		}
		return NO_ERROR;
	}

	virtual std::string Expected()
	{
		return "vec4(0./16., 4./16., 8./16., 12./16.)";
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
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1, 1);
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
				program = CreateProgram(VertexShader().c_str(), FallthroughFragmentShader().c_str());
			else
				program = CreateProgram(FallthroughVertexShader().c_str(), FragmentShader().c_str());
			glBindAttribLocation(program, 0, "v_in_0");
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

		return TestCompute();
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
		if (data[0] != Vec4(0, 1, 0, 1))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Expected Vec4(0, 1, 0, 1), got: " << data[0].x() << ", " << data[0].y()
				<< ", " << data[0].z() << ", " << data[0].w() << tcu::TestLog::EndMessage;
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

class PlainGatherFloat2D : public GatherBase
{
};

class PlainGatherInt2D : public GatherBase
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

	virtual std::string Expected()
	{
		return "ivec4(0, 4, 8, 12)";
	}

	virtual std::string Sampler()
	{
		return "uniform mediump isampler2D my_sampler;  \n";
	}

	virtual std::string Type()
	{
		return "ivec4";
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

	virtual std::string Expected()
	{
		return "uvec4(2u, 6u, 10u, 14u)";
	}

	virtual std::string Sampler()
	{
		return "uniform  mediump usampler2D my_sampler;                     \n";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(22.9f / 32, 25.9f / 32, 2, 2);
	}

	virtual std::string Type()
	{
		return "uvec4";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y), 2)";
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

	virtual std::string Expected()
	{
		return "vec4(1.0, 1.0, 0.0, 0.0)";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 13.5 / 16, 3);
	}

	virtual std::string Sampler()
	{
		return "uniform mediump sampler2DShadow my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y), p.z)";
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
		return "uniform mediump sampler2DArray my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z))";
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

	virtual std::string Expected()
	{
		return "ivec4(3, 7, 11, 15)";
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
		return "uniform mediump isampler2DArray my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z), 3)";
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

	virtual std::string Expected()
	{
		return "uvec4(0u, 4u, 8u, 12u)";
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
		return "uniform  mediump usampler2DArray my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z))";
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

	virtual std::string Expected()
	{
		return "vec4(1.0, 1.0, 0.0, 0.0)";
	}

	virtual Vec4 BufferData()
	{
		return Vec4(23. / 32, 26. / 32, 5, 13.5 / 16);
	}

	virtual std::string Sampler()
	{
		return "uniform mediump sampler2DArrayShadow my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z), p.w)";
	}
};

class PlainGatherFloatCube : public GatherBase
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
		return "uniform mediump samplerCube my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z))";
	}
};

class PlainGatherIntCube : public GatherBase
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
		return "uniform mediump isamplerCube my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z))";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32I;
	}

	virtual std::string Expected()
	{
		return "ivec4(0, 4, 8, 12)";
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
		return "uniform  mediump usamplerCube my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z), 0)";
	}

	virtual GLenum InternalFormat()
	{
		return GL_RGBA32UI;
	}

	virtual std::string Expected()
	{
		return "uvec4(0u, 4u, 8u, 12u)";
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
		return "uniform mediump samplerCubeShadow my_sampler;                     \n";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec3(p.x, p.y, p.z), p.w)";
	}

	virtual std::string Expected()
	{
		return "vec4(0.0, 0.0, 1.0, 1.0)";
	}
};

class OffsetGatherFloat2D : public GatherBase
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 4, 4);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec2(p.x, p.y), offset)";
	}
};

class OffsetGatherInt2D : public PlainGatherInt2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18.9f / 32.f, 21.9f / 32.f, 4, 4);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec2(p.x, p.y), offset)";
	}
};

class OffsetGatherUint2D : public PlainGatherUint2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(18.9f / 32.f, 21.9f / 32.f, 4, 2);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec2(p.x, p.y), offset, 2)";
	}
};

class OffsetGatherDepth2D : public PlainGatherDepth2D
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 4, 13.5 / 16);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec2(p.x, p.y), p.w, offset)";
	}
};

class OffsetGatherFloat2DArray : public PlainGatherFloat2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 5, 4);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), offset)";
	}
};

class OffsetGatherInt2DArray : public PlainGatherInt2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 11, 4);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), offset, 3)";
	}
};

class OffsetGatherUint2DArray : public PlainGatherUint2DArray
{
	virtual Vec4 BufferData()
	{
		return Vec4(19. / 32, 22. / 32, 1, 4);
	}

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), offset, 0)";
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

	virtual std::string Offset()
	{
		return "const mediump ivec2 offset = ivec2(4);         \n";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec3(p.x, p.y, p.z), p.y + (5.0/32.0), offset)";
	}
};

class Swizzle : public PlainGatherFloat2D
{
	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y), 1).yzww";
	}

	virtual std::string Expected()
	{
		return "vec4(5./16., 9./16., 13./16., 13./16.)";
	}
};

class BaseLevel : public PlainGatherFloat2D
{
	virtual void Init()
	{
		CreateTexture2D(true);
	}
};

class IncompleteTexture : public PlainGatherFloat2D
{
	virtual void Init()
	{
		CreateTexture2D();
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, 0);
	}

	virtual std::string Expected()
	{
		return "vec4(0)";
	}

	virtual std::string Gather()
	{
		return "textureGatherOffset(my_sampler, vec2(p.x, p.y), ivec2(0), 1)";
	}
};

class IncompleteTextureLastComp : public PlainGatherFloat2D
{
	virtual void Init()
	{
		CreateTexture2D();
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, 0);
	}

	virtual std::string Expected()
	{
		return "vec4(1.0)";
	}

	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y), 3)";
	}
};

class TriangleDraw : public GatherBase
{
	GLuint program, rbo, fbo, vao, vbo;

	virtual std::string VertexShader()
	{
		return "#version 310 es                               \n"
			   "flat out mediump vec2 texcoords;              \n"
			   "in mediump vec4 Vertex;                       \n"
			   "void main() {                                 \n"
			   "   gl_Position = Vertex;                      \n"
			   "   texcoords = (Vertex.xy + vec2(1.0)) / 2.0; \n"
			   "}\n";
	}

	virtual std::string FragmentShader()
	{
		return "#version 310 es                                   \n"
			   "precision highp float;                            \n"
			   "flat in mediump vec2 texcoords;                   \n"
			   "out highp uvec4 FragColor;                        \n"
			   "uniform mediump sampler2D tex;                    \n"
			   "void main() {                                     \n"
			   "   vec4 data = textureGather(tex, texcoords, 2);  \n"
			   "   FragColor = floatBitsToUint(data);             \n"
			   "}\n";
	}

	virtual long Run()
	{
		glGenFramebuffers(1, &fbo);
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32UI, 100, 100);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		GLfloat colorf[4] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, colorf);
		glViewport(0, 0, 100, 100);

		program = CreateProgram(VertexShader().c_str(), FragmentShader().c_str());
		glBindAttribLocation(program, 0, "Vertex");
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

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		GLfloat buffData[16] = { -1, 1, 0, 1, -1, -1, 0, 1, 1, 1, 0, 1, 1, -1, 0, 1 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(buffData), buffData, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableVertexAttribArray(0);
		glDeleteVertexArrays(1, &vao);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		std::vector<unsigned int> read(100 * 100 * 4, 0);
		glReadPixels(0, 0, 100, 100, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &read[0]);
		for (unsigned int i = 0; i < read.size() / 4; i += 4)
		{
			const GLfloat* rdata = (const GLfloat*)&read[i];
			Vec4		   rvec(rdata[0], rdata[1], rdata[2], rdata[3]);
			if (rvec != Vec4(0.75))
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "Got: " << rvec.x() << " " << rvec.y() << " " << rvec.z() << " "
					<< rvec.w() << ", expected vec4(0.75)" << tcu::TestLog::EndMessage;
				return ERROR;
			}
		}

		return NO_ERROR;
	}

	virtual long Cleanup()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glDisableVertexAttribArray(0);
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
	virtual std::string Expected()
	{
		return "vec4(0, 20.0/255.0, 90.0/255.0, 222.0/255.0)";
	}

	virtual GLenum InternalFormat()
	{
		return GL_SRGB8_ALPHA8;
	}

	virtual void Init()
	{
		CreateTextureSRGB();
	}
};

class PlainGatherFloat2DSrgbAlpha : public GatherBase
{
public:
	virtual std::string Gather()
	{
		return "textureGather(my_sampler, vec2(p.x, p.y), 3)";
	}

	virtual std::string Expected()
	{
		return "vec4(3.0/255.0, 7.0/255.0, 11.0/255.0, 15.0/255.0)";
	}

	virtual GLenum InternalFormat()
	{
		return GL_SRGB8_ALPHA8;
	}

	virtual void Init()
	{
		CreateTextureSRGB();
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

class PlainGatherFloat2DRg : public GatherBase
{
public:
	virtual void Init()
	{
		CreateTexture2DRg();
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

TextureGatherTests::TextureGatherTests(glcts::Context& context) : TestCaseGroup(context, "texture_gather", "")
{
}

TextureGatherTests::~TextureGatherTests(void)
{
}

void TextureGatherTests::init()
{
	using namespace glcts;
	addChild(new TestSubcase(m_context, "api-enums", TestSubcase::Create<GatherEnumsTest>));
	addChild(new TestSubcase(m_context, "gather-glsl-compile", TestSubcase::Create<GatherGLSLCompile>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d", TestSubcase::Create<PlainGatherFloat2D>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2d", TestSubcase::Create<PlainGatherInt2D>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-2d", TestSubcase::Create<PlainGatherUint2D>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-2d", TestSubcase::Create<PlainGatherDepth2D>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2darray", TestSubcase::Create<PlainGatherFloat2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-int-2darray", TestSubcase::Create<PlainGatherInt2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-2darray", TestSubcase::Create<PlainGatherUint2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-2darray", TestSubcase::Create<PlainGatherDepth2DArray>));
	addChild(new TestSubcase(m_context, "plain-gather-float-cube-rgba", TestSubcase::Create<PlainGatherFloatCube>));
	addChild(new TestSubcase(m_context, "plain-gather-int-cube-rgba", TestSubcase::Create<PlainGatherIntCube>));
	addChild(new TestSubcase(m_context, "plain-gather-uint-cube", TestSubcase::Create<PlainGatherUintCube>));
	addChild(new TestSubcase(m_context, "plain-gather-depth-cube", TestSubcase::Create<PlainGatherDepthCube>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d", TestSubcase::Create<OffsetGatherFloat2D>));
	addChild(new TestSubcase(m_context, "offset-gather-int-2d", TestSubcase::Create<OffsetGatherInt2D>));
	addChild(new TestSubcase(m_context, "offset-gather-uint-2d", TestSubcase::Create<OffsetGatherUint2D>));
	addChild(new TestSubcase(m_context, "offset-gather-depth-2d", TestSubcase::Create<OffsetGatherDepth2D>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2darray", TestSubcase::Create<OffsetGatherFloat2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-int-2darray", TestSubcase::Create<OffsetGatherInt2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-uint-2darray", TestSubcase::Create<OffsetGatherUint2DArray>));
	addChild(new TestSubcase(m_context, "offset-gather-depth-2darray", TestSubcase::Create<OffsetGatherDepth2DArray>));
	addChild(new TestSubcase(m_context, "swizzle", TestSubcase::Create<Swizzle>));
	addChild(new TestSubcase(m_context, "base-level", TestSubcase::Create<BaseLevel>));
	addChild(new TestSubcase(m_context, "incomplete-texture", TestSubcase::Create<IncompleteTexture>));
	addChild(
		new TestSubcase(m_context, "incomplete-texture-last-comp", TestSubcase::Create<IncompleteTextureLastComp>));
	addChild(new TestSubcase(m_context, "triangle-draw", TestSubcase::Create<TriangleDraw>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-srgb", TestSubcase::Create<PlainGatherFloat2DSrgb>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-srgb-alpha",
							 TestSubcase::Create<PlainGatherFloat2DSrgbAlpha>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-rgb", TestSubcase::Create<PlainGatherFloat2DRgb>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-rg", TestSubcase::Create<PlainGatherFloat2DRg>));
	addChild(new TestSubcase(m_context, "plain-gather-float-2d-r", TestSubcase::Create<PlainGatherFloat2DR>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-rgb", TestSubcase::Create<OffsetGatherFloat2DRgb>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-rg", TestSubcase::Create<OffsetGatherFloat2DRg>));
	addChild(new TestSubcase(m_context, "offset-gather-float-2d-r", TestSubcase::Create<OffsetGatherFloat2DR>));
}
} // glcts namespace
