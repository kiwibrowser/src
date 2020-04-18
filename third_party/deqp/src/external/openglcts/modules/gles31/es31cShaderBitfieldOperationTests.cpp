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

#include "es31cShaderBitfieldOperationTests.hpp"
#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glw.h"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

using tcu::TestLog;
using std::string;
using std::vector;
using glcts::Context;

static std::string specializeVersion(const std::string& source, glu::GLSLVersion version, const char* testStatement)
{
	DE_ASSERT(version == glu::GLSL_VERSION_310_ES || version >= glu::GLSL_VERSION_430);
	std::map<std::string, std::string> args;
	args["VERSION_DECL"]   = glu::getGLSLVersionDeclaration(version);
	args["TEST_STATEMENT"] = testStatement;
	return tcu::StringTemplate(source.c_str()).specialize(args);
}

struct Data
{
	Data()
	{
		memset(this, 0, sizeof *this);
	}
	Data(Data const& init)
	{
		memcpy(this, &init, sizeof *this);
	}

	GLuint  inUvec4[4];
	GLint   inIvec4[4];
	GLfloat inVec4[4];

	GLuint  in2Uvec4[4];
	GLint   in2Ivec4[4];
	GLfloat in2Vec4[4];

	GLint offset;
	GLint bits;
	GLint padding[2];

	GLuint  outUvec4[4];
	GLint   outIvec4[4];
	GLfloat outVec4[4];

	GLuint  out2Uvec4[4];
	GLint   out2Ivec4[4];
	GLfloat out2Vec4[4];
};

struct Uvec4 : public Data
{
	Uvec4(GLuint x = 0, GLuint y = 0, GLuint z = 0, GLuint w = 0)
	{
		inUvec4[0] = x;
		inUvec4[1] = y;
		inUvec4[2] = z;
		inUvec4[3] = w;
	}
};

struct Ivec4 : public Data
{
	Ivec4(GLint x = 0, GLint y = 0, GLint z = 0, GLint w = 0)
	{
		inIvec4[0] = x;
		inIvec4[1] = y;
		inIvec4[2] = z;
		inIvec4[3] = w;
	}
};

struct Vec4 : public Data
{
	Vec4(GLfloat x = 0.0f, GLfloat y = 0.0f, GLfloat z = 0.0f, GLfloat w = 0.0f)
	{
		inVec4[0] = x;
		inVec4[1] = y;
		inVec4[2] = z;
		inVec4[3] = w;
	}
};

class ShaderBitfieldOperationCase : public TestCase
{
public:
	ShaderBitfieldOperationCase(Context& context, const char* name, const char* description,
								glu::GLSLVersion glslVersion, Data const& data, char const* testStatement);
	~ShaderBitfieldOperationCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion m_glslVersion;
	Data			 m_data;
	std::string		 m_testStatement;

	virtual bool test(Data const* data) = 0;
};

ShaderBitfieldOperationCase::ShaderBitfieldOperationCase(Context& context, const char* name, const char* description,
														 glu::GLSLVersion glslVersion, Data const& data,
														 char const* testStatement)
	: TestCase(context, name, description), m_glslVersion(glslVersion), m_data(data), m_testStatement(testStatement)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_430);
}

ShaderBitfieldOperationCase::~ShaderBitfieldOperationCase()
{
}

ShaderBitfieldOperationCase::IterateResult ShaderBitfieldOperationCase::iterate()
{
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	GLuint data;
	gl.genBuffers(1, &data);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, data);
	gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Data), &m_data, GL_STATIC_DRAW);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, data);

	char const* css = "${VERSION_DECL}\n"
					  "\n"
					  "layout(local_size_x = 1) in;\n"
					  "\n"
					  "layout(binding = 0, std430) buffer Data {\n"
					  "    uvec4 inUvec4;\n"
					  "    ivec4 inIvec4;\n"
					  "    vec4 inVec4;\n"
					  "\n"
					  "    uvec4 in2Uvec4;\n"
					  "    ivec4 in2Ivec4;\n"
					  "    vec4 in2Vec4;\n"
					  "\n"
					  "    int offset;\n"
					  "    int bits;\n"
					  "\n"
					  "    uvec4 outUvec4;\n"
					  "    ivec4 outIvec4;\n"
					  "    vec4 outVec4;\n"
					  "\n"
					  "    uvec4 out2Uvec4;\n"
					  "    ivec4 out2Ivec4;\n"
					  "    vec4 out2Vec4;\n"
					  "};\n"
					  "\n"
					  "void main()\n"
					  "{\n"
					  "    ${TEST_STATEMENT};\n"
					  "}\n";

	GLuint		cs		   = gl.createShader(GL_COMPUTE_SHADER);
	std::string csString   = specializeVersion(css, m_glslVersion, m_testStatement.c_str());
	char const* strings[1] = { csString.c_str() };
	gl.shaderSource(cs, 1, strings, 0);
	gl.compileShader(cs);
	GLint compileSuccess = 0;
	gl.getShaderiv(cs, GL_COMPILE_STATUS, &compileSuccess);
	if (!compileSuccess)
	{
		TCU_FAIL("Compile failed");
	}

	GLuint pgm = gl.createProgram();
	gl.attachShader(pgm, cs);
	gl.linkProgram(pgm);
	GLint linkSuccess = 0;
	gl.getProgramiv(pgm, GL_LINK_STATUS, &linkSuccess);
	if (!linkSuccess)
	{
		gl.deleteShader(cs);
		TCU_FAIL("Link failed");
	}

	gl.useProgram(pgm);

	gl.dispatchCompute(1, 1, 1);

	Data const* results = (Data const*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Data), GL_MAP_READ_BIT);
	isOk				= test(results);
	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);

	gl.useProgram(0);
	gl.deleteProgram(pgm);
	gl.deleteShader(cs);

	gl.deleteBuffers(1, &data);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

class ShaderBitfieldOperationCaseFrexp : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseFrexp(Context& context, const char* name, glu::GLSLVersion glslVersion, Data const& data,
									 int components, char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			if (data->inVec4[i] == 0.0)
			{
				if (data->outVec4[i] != 0.0 || data->outIvec4[i] != 0)
				{
					return false;
				}
			}
			else if (deFloatAbs(data->outVec4[i]) < 0.5 || deFloatAbs(data->outVec4[i]) >= 1.0)
			{
				return false;
			}

			float result = data->outVec4[i] * deFloatPow(2.0, (float)data->outIvec4[i]);
			if (deFloatAbs(result - data->inVec4[i]) > 0.0001f)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseFrexpFloat : public ShaderBitfieldOperationCaseFrexp
{
public:
	ShaderBitfieldOperationCaseFrexpFloat(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  Data const& data)
		: ShaderBitfieldOperationCaseFrexp(context, name, glslVersion, data, 1,
										   "outVec4.x = frexp(inVec4.x, outIvec4.x)")
	{
	}
};

class ShaderBitfieldOperationCaseFrexpVec2 : public ShaderBitfieldOperationCaseFrexp
{
public:
	ShaderBitfieldOperationCaseFrexpVec2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data)
		: ShaderBitfieldOperationCaseFrexp(context, name, glslVersion, data, 2,
										   "outVec4.xy = frexp(inVec4.xy, outIvec4.xy)")
	{
	}
};

class ShaderBitfieldOperationCaseFrexpVec3 : public ShaderBitfieldOperationCaseFrexp
{
public:
	ShaderBitfieldOperationCaseFrexpVec3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data)
		: ShaderBitfieldOperationCaseFrexp(context, name, glslVersion, data, 3,
										   "outVec4.xyz = frexp(inVec4.xyz, outIvec4.xyz)")
	{
	}
};

class ShaderBitfieldOperationCaseFrexpVec4 : public ShaderBitfieldOperationCaseFrexp
{
public:
	ShaderBitfieldOperationCaseFrexpVec4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data)
		: ShaderBitfieldOperationCaseFrexp(context, name, glslVersion, data, 4, "outVec4 = frexp(inVec4, outIvec4)")
	{
	}
};

class ShaderBitfieldOperationCaseLdexp : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseLdexp(Context& context, const char* name, glu::GLSLVersion glslVersion, Data const& data,
									 int components, char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			float expected = deFloatLdExp(data->inVec4[i], data->inIvec4[i]);
			if (deFloatAbs(expected - data->outVec4[i]) > 0.0001f)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseLdexpFloat : public ShaderBitfieldOperationCaseLdexp
{
public:
	ShaderBitfieldOperationCaseLdexpFloat(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  Data const& data, Data const& exp)
		: ShaderBitfieldOperationCaseLdexp(context, name, glslVersion, data, 1,
										   "outVec4.x = ldexp(inVec4.x, inIvec4.x)")
	{
		m_data.inIvec4[0] = exp.inIvec4[0];
	}
};

class ShaderBitfieldOperationCaseLdexpVec2 : public ShaderBitfieldOperationCaseLdexp
{
public:
	ShaderBitfieldOperationCaseLdexpVec2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data, Data const& exp)
		: ShaderBitfieldOperationCaseLdexp(context, name, glslVersion, data, 2,
										   "outVec4.xy = ldexp(inVec4.xy, inIvec4.xy)")
	{
		m_data.inIvec4[0] = exp.inIvec4[0];
		m_data.inIvec4[1] = exp.inIvec4[1];
	}
};

class ShaderBitfieldOperationCaseLdexpVec3 : public ShaderBitfieldOperationCaseLdexp
{
public:
	ShaderBitfieldOperationCaseLdexpVec3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data, Data const& exp)
		: ShaderBitfieldOperationCaseLdexp(context, name, glslVersion, data, 3,
										   "outVec4.xyz = ldexp(inVec4.xyz, inIvec4.xyz)")
	{
		m_data.inIvec4[0] = exp.inIvec4[0];
		m_data.inIvec4[1] = exp.inIvec4[1];
		m_data.inIvec4[2] = exp.inIvec4[2];
	}
};

class ShaderBitfieldOperationCaseLdexpVec4 : public ShaderBitfieldOperationCaseLdexp
{
public:
	ShaderBitfieldOperationCaseLdexpVec4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data, Data const& exp)
		: ShaderBitfieldOperationCaseLdexp(context, name, glslVersion, data, 4, "outVec4 = ldexp(inVec4, inIvec4)")
	{
		m_data.inIvec4[0] = exp.inIvec4[0];
		m_data.inIvec4[1] = exp.inIvec4[1];
		m_data.inIvec4[2] = exp.inIvec4[2];
		m_data.inIvec4[3] = exp.inIvec4[3];
	}
};

class ShaderBitfieldOperationCasePackUnorm : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCasePackUnorm(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, "outUvec4.x = packUnorm4x8(inVec4)")
	{
	}

private:
	virtual bool test(Data const* data)
	{
		GLuint expected =
			((int(data->inVec4[0] * 255.0 + 0.5) & 0xFF) << 0) | ((int(data->inVec4[1] * 255.0 + 0.5) & 0xFF) << 8) |
			((int(data->inVec4[2] * 255.0 + 0.5) & 0xFF) << 16) | ((int(data->inVec4[3] * 255.0 + 0.5) & 0xFF) << 24);
		if (expected != data->outUvec4[0])
		{
			return false;
		}
		return true;
	}
};

class ShaderBitfieldOperationCasePackSnorm : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCasePackSnorm(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 Data const& data)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, "outUvec4.x = packSnorm4x8(inVec4)")
	{
	}

private:
	virtual bool test(Data const* data)
	{
		GLuint expected = ((int(deFloatFloor(data->inVec4[0] * 127.0f + 0.5f)) & 0xFF) << 0) |
						  ((int(deFloatFloor(data->inVec4[1] * 127.0f + 0.5f)) & 0xFF) << 8) |
						  ((int(deFloatFloor(data->inVec4[2] * 127.0f + 0.5f)) & 0xFF) << 16) |
						  ((int(deFloatFloor(data->inVec4[3] * 127.0f + 0.5f)) & 0xFF) << 24);
		if (expected != data->outUvec4[0])
		{
			return false;
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseUnpackUnorm : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseUnpackUnorm(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   Data const& data)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, "outVec4 = unpackUnorm4x8(inUvec4.x)")
	{
	}

private:
	virtual bool test(Data const* data)
	{
		GLfloat x = float((data->inUvec4[0] >> 0) & 0xFF) / 255.0f;
		if (deFloatAbs(data->outVec4[0] - x) > 0.0001f)
		{
			return false;
		}
		GLfloat y = float((data->inUvec4[0] >> 8) & 0xFF) / 255.0f;
		if (deFloatAbs(data->outVec4[1] - y) > 0.0001f)
		{
			return false;
		}
		GLfloat z = float((data->inUvec4[0] >> 16) & 0xFF) / 255.0f;
		if (deFloatAbs(data->outVec4[2] - z) > 0.0001f)
		{
			return false;
		}
		GLfloat w = float((data->inUvec4[0] >> 24) & 0xFF) / 255.0f;
		if (deFloatAbs(data->outVec4[3] - w) > 0.0001f)
		{
			return false;
		}

		return true;
	}
};

class ShaderBitfieldOperationCaseUnpackSnorm : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseUnpackSnorm(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   Data const& data)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, "outVec4 = unpackSnorm4x8(inUvec4.x)")
	{
	}

private:
	virtual bool test(Data const* data)
	{
		GLfloat x = float((signed char)((data->inUvec4[0] >> 0) & 0xFF)) / 127.0f;
		x		  = de::clamp<GLfloat>(x, -1.0f, 1.0f);
		if (deFloatAbs(data->outVec4[0] - x) > 0.0001f)
		{
			return false;
		}
		GLfloat y = float((signed char)((data->inUvec4[0] >> 8) & 0xFF)) / 127.0f;
		y		  = de::clamp<GLfloat>(y, -1.0f, 1.0f);
		if (deFloatAbs(data->outVec4[1] - y) > 0.0001f)
		{
			return false;
		}
		GLfloat z = float((signed char)((data->inUvec4[0] >> 16) & 0xFF)) / 127.0f;
		z		  = de::clamp<GLfloat>(z, -1.0f, 1.0f);
		if (deFloatAbs(data->outVec4[2] - z) > 0.0001f)
		{
			return false;
		}
		GLfloat w = float((signed char)((data->inUvec4[0] >> 24) & 0xFF)) / 127.0f;
		w		  = de::clamp<GLfloat>(w, -1.0f, 1.0f);
		if (deFloatAbs(data->outVec4[3] - w) > 0.0001f)
		{
			return false;
		}

		return true;
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractUint : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractUint(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, int offset, int bits, int components,
												   char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
		if (offset + bits > 32)
		{
			offset -= (offset + bits) - 32;
		}
		m_data.offset = offset;
		m_data.bits   = bits;
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLuint expected =
				(data->inUvec4[i] >> data->offset) & (data->bits == 32 ? 0xFFFFFFFF : ((1 << data->bits) - 1));
			if (data->outUvec4[i] != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractUint1 : public ShaderBitfieldOperationCaseBitfieldExtractUint
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractUint1(Context& context, const char* name, glu::GLSLVersion glslVersion,
													Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractUint(context, name, glslVersion, data, offset, bits, 1,
														 "outUvec4.x = bitfieldExtract(inUvec4.x, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractUint2 : public ShaderBitfieldOperationCaseBitfieldExtractUint
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractUint2(Context& context, const char* name, glu::GLSLVersion glslVersion,
													Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractUint(context, name, glslVersion, data, offset, bits, 2,
														 "outUvec4.xy = bitfieldExtract(inUvec4.xy, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractUint3 : public ShaderBitfieldOperationCaseBitfieldExtractUint
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractUint3(Context& context, const char* name, glu::GLSLVersion glslVersion,
													Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractUint(context, name, glslVersion, data, offset, bits, 3,
														 "outUvec4.xyz = bitfieldExtract(inUvec4.xyz, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractUint4 : public ShaderBitfieldOperationCaseBitfieldExtractUint
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractUint4(Context& context, const char* name, glu::GLSLVersion glslVersion,
													Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractUint(context, name, glslVersion, data, offset, bits, 4,
														 "outUvec4 = bitfieldExtract(inUvec4, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractInt : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractInt(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, int offset, int bits, int components,
												  char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
		if (offset + bits > 32)
		{
			offset -= (offset + bits) - 32;
		}
		m_data.offset = offset;
		m_data.bits   = bits;
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLint expected = data->inIvec4[i] << (32 - (data->offset + data->bits));
			expected >>= 32 - data->bits;
			if (data->outIvec4[i] != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractInt1 : public ShaderBitfieldOperationCaseBitfieldExtractInt
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractInt1(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractInt(context, name, glslVersion, data, offset, bits, 1,
														"outIvec4.x = bitfieldExtract(inIvec4.x, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractInt2 : public ShaderBitfieldOperationCaseBitfieldExtractInt
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractInt2(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractInt(context, name, glslVersion, data, offset, bits, 2,
														"outIvec4.xy = bitfieldExtract(inIvec4.xy, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractInt3 : public ShaderBitfieldOperationCaseBitfieldExtractInt
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractInt3(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractInt(context, name, glslVersion, data, offset, bits, 3,
														"outIvec4.xyz = bitfieldExtract(inIvec4.xyz, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldExtractInt4 : public ShaderBitfieldOperationCaseBitfieldExtractInt
{
public:
	ShaderBitfieldOperationCaseBitfieldExtractInt4(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldExtractInt(context, name, glslVersion, data, offset, bits, 4,
														"outIvec4 = bitfieldExtract(inIvec4, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertUint : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertUint(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, Data const& insert, int offset, int bits,
												  int components, char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
		for (int i = 0; i < components; ++i)
		{
			m_data.in2Uvec4[i] = insert.inUvec4[i];
		}
		if (offset + bits > 32)
		{
			offset -= (offset + bits) - 32;
		}
		m_data.offset = offset;
		m_data.bits   = bits;
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLuint mask = (data->bits == 32) ? ~0u : (1 << data->bits) - 1;
			GLuint expected =
				(data->inUvec4[i] & ~(mask << data->offset)) | ((data->in2Uvec4[i] & mask) << data->offset);
			if (data->outUvec4[i] != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertUint1 : public ShaderBitfieldOperationCaseBitfieldInsertUint
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertUint1(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertUint(
			  context, name, glslVersion, data, insert, offset, bits, 1,
			  "outUvec4.x = bitfieldInsert(inUvec4.x, in2Uvec4.x, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertUint2 : public ShaderBitfieldOperationCaseBitfieldInsertUint
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertUint2(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertUint(
			  context, name, glslVersion, data, insert, offset, bits, 2,
			  "outUvec4.xy = bitfieldInsert(inUvec4.xy, in2Uvec4.xy, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertUint3 : public ShaderBitfieldOperationCaseBitfieldInsertUint
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertUint3(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertUint(
			  context, name, glslVersion, data, insert, offset, bits, 3,
			  "outUvec4.xyz = bitfieldInsert(inUvec4.xyz, in2Uvec4.xyz, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertUint4 : public ShaderBitfieldOperationCaseBitfieldInsertUint
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertUint4(Context& context, const char* name, glu::GLSLVersion glslVersion,
												   Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertUint(context, name, glslVersion, data, insert, offset, bits, 4,
														"outUvec4 = bitfieldInsert(inUvec4, in2Uvec4, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertInt : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertInt(Context& context, const char* name, glu::GLSLVersion glslVersion,
												 Data const& data, Data const& insert, int offset, int bits,
												 int components, char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement), m_components(components)
	{
		for (int i = 0; i < components; ++i)
		{
			m_data.in2Ivec4[i] = insert.inIvec4[i];
		}
		if (offset + bits > 32)
		{
			offset -= (offset + bits) - 32;
		}
		m_data.offset = offset;
		m_data.bits   = bits;
	}

private:
	int m_components;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLuint mask = (data->bits == 32) ? ~0u : (1 << data->bits) - 1;
			GLint  expected =
				(data->inIvec4[i] & ~(mask << data->offset)) | ((data->in2Ivec4[i] & mask) << data->offset);
			if (data->outIvec4[i] != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertInt1 : public ShaderBitfieldOperationCaseBitfieldInsertInt
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertInt1(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertInt(
			  context, name, glslVersion, data, insert, offset, bits, 1,
			  "outIvec4.x = bitfieldInsert(inIvec4.x, in2Ivec4.x, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertInt2 : public ShaderBitfieldOperationCaseBitfieldInsertInt
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertInt2(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertInt(
			  context, name, glslVersion, data, insert, offset, bits, 2,
			  "outIvec4.xy = bitfieldInsert(inIvec4.xy, in2Ivec4.xy, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertInt3 : public ShaderBitfieldOperationCaseBitfieldInsertInt
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertInt3(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertInt(
			  context, name, glslVersion, data, insert, offset, bits, 3,
			  "outIvec4.xyz = bitfieldInsert(inIvec4.xyz, in2Ivec4.xyz, offset, bits)")
	{
	}
};

class ShaderBitfieldOperationCaseBitfieldInsertInt4 : public ShaderBitfieldOperationCaseBitfieldInsertInt
{
public:
	ShaderBitfieldOperationCaseBitfieldInsertInt4(Context& context, const char* name, glu::GLSLVersion glslVersion,
												  Data const& data, Data const& insert, int offset, int bits)
		: ShaderBitfieldOperationCaseBitfieldInsertInt(context, name, glslVersion, data, insert, offset, bits, 4,
													   "outIvec4 = bitfieldInsert(inIvec4, in2Ivec4, offset, bits)")
	{
	}
};

typedef GLuint (*UnaryUFunc)(GLuint input);
typedef GLint (*UnaryIFunc)(GLint input);

static GLuint bitfieldReverse(GLuint input)
{
	GLuint result = 0;
	for (int i = 0; i < 32; ++i)
	{
		result >>= 1;
		result |= (input & 0x80000000);
		input <<= 1;
	}
	return result;
}

static GLuint bitCount(GLuint input)
{
	GLuint result = 0;
	while (input)
	{
		if (input & 1)
		{
			result += 1;
		}
		input >>= 1;
	}
	return result;
}

static GLuint findLSB(GLuint input)
{
	if (!input)
	{
		return -1;
	}
	for (GLuint result = 0;; ++result)
	{
		if (input & 1)
		{
			return result;
		}
		input >>= 1;
	}
}

static GLuint findMSBU(GLuint input)
{
	if (!input)
	{
		return -1;
	}
	for (GLuint result = 31;; --result)
	{
		if (input & 0x80000000)
		{
			return result;
		}
		input <<= 1;
	}
}

static GLint findMSBI(GLint input)
{
	if (input == 0 || input == -1)
	{
		return -1;
	}
	else if (input > 0)
	{
		for (GLuint result = 31;; --result)
		{
			if (input & 0x80000000)
			{
				return result;
			}
			input <<= 1;
		}
	}
	else
	{
		for (GLuint result = 31;; --result)
		{
			if (!(input & 0x80000000))
			{
				return result;
			}
			input <<= 1;
		}
	}
}

class ShaderBitfieldOperationCaseUnaryUint : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseUnaryUint(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* funcName, UnaryUFunc func, Data const& data, int components,
										 char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement)
		, m_components(components)
		, m_func(func)
	{
		size_t pos = m_testStatement.find("func");
		m_testStatement.replace(pos, 4, funcName);
	}

private:
	int		   m_components;
	UnaryUFunc m_func;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLuint expected = m_func(data->inUvec4[i]);
			GLuint output   = data->outUvec4[i];
			if (m_func == (UnaryUFunc)glcts::bitCount || m_func == (UnaryUFunc)glcts::findLSB ||
				m_func == (UnaryUFunc)glcts::findMSBU || m_func == (UnaryUFunc)glcts::findMSBI)
			{
				/* The built-in bitCount, findLSB and findMSB functions
				 * return a lowp int, which can be encoded with as little
				 * as 9 bits. Since findLSB and findMSB can return negative
				 * values (namely, -1), we cannot compare the value directly
				 * against a (32-bit) GLuint.
				 */
				GLuint output_9bits = output & 0x1ff;
				GLuint sign_extend  = output_9bits & 0x100 ? 0xfffffe00 : 0;
				output				= output_9bits | sign_extend;
			}

			if (output != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseUnaryUint1 : public ShaderBitfieldOperationCaseUnaryUint
{
public:
	ShaderBitfieldOperationCaseUnaryUint1(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* funcName, UnaryUFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryUint(context, name, glslVersion, funcName, func, data, 1,
											   "outUvec4.x = uint(func(inUvec4.x))")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryUint2 : public ShaderBitfieldOperationCaseUnaryUint
{
public:
	ShaderBitfieldOperationCaseUnaryUint2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* funcName, UnaryUFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryUint(context, name, glslVersion, funcName, func, data, 2,
											   "outUvec4.xy = uvec2(func(inUvec4.xy))")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryUint3 : public ShaderBitfieldOperationCaseUnaryUint
{
public:
	ShaderBitfieldOperationCaseUnaryUint3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* funcName, UnaryUFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryUint(context, name, glslVersion, funcName, func, data, 3,
											   "outUvec4.xyz = uvec3(func(inUvec4.xyz))")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryUint4 : public ShaderBitfieldOperationCaseUnaryUint
{
public:
	ShaderBitfieldOperationCaseUnaryUint4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* funcName, UnaryUFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryUint(context, name, glslVersion, funcName, func, data, 4,
											   "outUvec4 = uvec4(func(inUvec4))")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryInt : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseUnaryInt(Context& context, const char* name, glu::GLSLVersion glslVersion,
										char const* funcName, UnaryIFunc func, Data const& data, int components,
										char const* testStatement)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, data, testStatement)
		, m_components(components)
		, m_func(func)
	{
		size_t pos = m_testStatement.find("func");
		m_testStatement.replace(pos, 4, funcName);
	}

private:
	int		   m_components;
	UnaryIFunc m_func;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLint expected = m_func(data->inIvec4[i]);
			if (data->outIvec4[i] != expected)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseUnaryInt1 : public ShaderBitfieldOperationCaseUnaryInt
{
public:
	ShaderBitfieldOperationCaseUnaryInt1(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* funcName, UnaryIFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryInt(context, name, glslVersion, funcName, func, data, 1,
											  "outIvec4.x = func(inIvec4.x)")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryInt2 : public ShaderBitfieldOperationCaseUnaryInt
{
public:
	ShaderBitfieldOperationCaseUnaryInt2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* funcName, UnaryIFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryInt(context, name, glslVersion, funcName, func, data, 2,
											  "outIvec4.xy = func(inIvec4.xy)")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryInt3 : public ShaderBitfieldOperationCaseUnaryInt
{
public:
	ShaderBitfieldOperationCaseUnaryInt3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* funcName, UnaryIFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryInt(context, name, glslVersion, funcName, func, data, 3,
											  "outIvec4.xyz = func(inIvec4.xyz)")
	{
	}
};

class ShaderBitfieldOperationCaseUnaryInt4 : public ShaderBitfieldOperationCaseUnaryInt
{
public:
	ShaderBitfieldOperationCaseUnaryInt4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* funcName, UnaryIFunc func, Data const& data)
		: ShaderBitfieldOperationCaseUnaryInt(context, name, glslVersion, funcName, func, data, 4,
											  "outIvec4 = func(inIvec4)")
	{
	}
};

typedef GLuint (*BinaryUFunc)(GLuint input, GLuint input2, GLuint& output2);
typedef GLint (*BinaryIFunc)(GLint input, GLint input2, GLint& output2);

static GLuint uaddCarry(GLuint input, GLuint input2, GLuint& output2)
{
	GLuint result = input + input2;
	output2		  = (input > result) ? 1 : 0;
	return result;
}

static GLuint usubBorrow(GLuint input, GLuint input2, GLuint& output2)
{
	output2 = (input2 > input) ? 1 : 0;
	return input - input2;
}

static GLuint umulExtended(GLuint input, GLuint input2, GLuint& output2)
{
	GLuint64 result = static_cast<GLuint64>(input) * static_cast<GLuint64>(input2);
	output2			= GLuint(result & 0xFFFFFFFF);
	return GLuint(result >> 32);
}

static GLint imulExtended(GLint input, GLint input2, GLint& output2)
{
	GLint64 result = static_cast<GLint64>(input) * static_cast<GLint64>(input2);
	output2		   = GLint(result & 0xFFFFFFFF);
	return GLint(result >> 32);
}

class ShaderBitfieldOperationCaseBinaryUint : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBinaryUint(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* testStatement, BinaryUFunc func, Data const& input,
										  Data const& input2, int components)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, input, testStatement)
		, m_components(components)
		, m_func(func)
	{
		for (int i = 0; i < components; ++i)
		{
			m_data.in2Uvec4[i] = input2.inUvec4[i];
		}
	}

private:
	int			m_components;
	BinaryUFunc m_func;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLuint expected2 = 0;
			GLuint expected  = m_func(data->inUvec4[i], data->in2Uvec4[i], expected2);
			if (data->outUvec4[i] != expected || data->out2Uvec4[i] != expected2)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBinaryUint1 : public ShaderBitfieldOperationCaseBinaryUint
{
public:
	ShaderBitfieldOperationCaseBinaryUint1(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   char const* testStatement, BinaryUFunc func, Data const& input,
										   Data const& input2)
		: ShaderBitfieldOperationCaseBinaryUint(context, name, glslVersion, testStatement, func, input, input2, 1)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryUint2 : public ShaderBitfieldOperationCaseBinaryUint
{
public:
	ShaderBitfieldOperationCaseBinaryUint2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   char const* testStatement, BinaryUFunc func, Data const& input,
										   Data const& input2)
		: ShaderBitfieldOperationCaseBinaryUint(context, name, glslVersion, testStatement, func, input, input2, 2)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryUint3 : public ShaderBitfieldOperationCaseBinaryUint
{
public:
	ShaderBitfieldOperationCaseBinaryUint3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   char const* testStatement, BinaryUFunc func, Data const& input,
										   Data const& input2)
		: ShaderBitfieldOperationCaseBinaryUint(context, name, glslVersion, testStatement, func, input, input2, 3)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryUint4 : public ShaderBitfieldOperationCaseBinaryUint
{
public:
	ShaderBitfieldOperationCaseBinaryUint4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										   char const* testStatement, BinaryUFunc func, Data const& input,
										   Data const& input2)
		: ShaderBitfieldOperationCaseBinaryUint(context, name, glslVersion, testStatement, func, input, input2, 4)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryInt : public ShaderBitfieldOperationCase
{
public:
	ShaderBitfieldOperationCaseBinaryInt(Context& context, const char* name, glu::GLSLVersion glslVersion,
										 char const* testStatement, BinaryIFunc func, Data const& input,
										 Data const& input2, int components)
		: ShaderBitfieldOperationCase(context, name, "", glslVersion, input, testStatement)
		, m_components(components)
		, m_func(func)
	{
		for (int i = 0; i < components; ++i)
		{
			m_data.in2Ivec4[i] = input2.inIvec4[i];
		}
	}

private:
	int			m_components;
	BinaryIFunc m_func;

	virtual bool test(Data const* data)
	{
		for (int i = 0; i < m_components; ++i)
		{
			GLint expected2 = 0;
			GLint expected  = m_func(data->inIvec4[i], data->in2Ivec4[i], expected2);
			if (data->outIvec4[i] != expected || data->out2Ivec4[i] != expected2)
			{
				return false;
			}
		}
		return true;
	}
};

class ShaderBitfieldOperationCaseBinaryInt1 : public ShaderBitfieldOperationCaseBinaryInt
{
public:
	ShaderBitfieldOperationCaseBinaryInt1(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* testStatement, BinaryIFunc func, Data const& input,
										  Data const& input2)
		: ShaderBitfieldOperationCaseBinaryInt(context, name, glslVersion, testStatement, func, input, input2, 1)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryInt2 : public ShaderBitfieldOperationCaseBinaryInt
{
public:
	ShaderBitfieldOperationCaseBinaryInt2(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* testStatement, BinaryIFunc func, Data const& input,
										  Data const& input2)
		: ShaderBitfieldOperationCaseBinaryInt(context, name, glslVersion, testStatement, func, input, input2, 2)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryInt3 : public ShaderBitfieldOperationCaseBinaryInt
{
public:
	ShaderBitfieldOperationCaseBinaryInt3(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* testStatement, BinaryIFunc func, Data const& input,
										  Data const& input2)
		: ShaderBitfieldOperationCaseBinaryInt(context, name, glslVersion, testStatement, func, input, input2, 3)
	{
	}
};

class ShaderBitfieldOperationCaseBinaryInt4 : public ShaderBitfieldOperationCaseBinaryInt
{
public:
	ShaderBitfieldOperationCaseBinaryInt4(Context& context, const char* name, glu::GLSLVersion glslVersion,
										  char const* testStatement, BinaryIFunc func, Data const& input,
										  Data const& input2)
		: ShaderBitfieldOperationCaseBinaryInt(context, name, glslVersion, testStatement, func, input, input2, 4)
	{
	}
};

ShaderBitfieldOperationTests::ShaderBitfieldOperationTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "shader_bitfield_operation", "Shader Bitfield Operation tests"), m_glslVersion(glslVersion)
{
}

ShaderBitfieldOperationTests::~ShaderBitfieldOperationTests(void)
{
}

void ShaderBitfieldOperationTests::init(void)
{
	de::Random rnd(m_context.getTestContext().getCommandLine().getBaseSeed());

	// shader_bitfield_operation.frexp
	tcu::TestCaseGroup* frexpGroup = new tcu::TestCaseGroup(m_testCtx, "frexp", "");
	addChild(frexpGroup);
	frexpGroup->addChild(new ShaderBitfieldOperationCaseFrexpFloat(m_context, "float_zero", m_glslVersion, Vec4(0.0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "float_" << i;
		frexpGroup->addChild(new ShaderBitfieldOperationCaseFrexpFloat(m_context, ss.str().c_str(), m_glslVersion,
																	   Vec4(rnd.getFloat())));
	}
	frexpGroup->addChild(
		new ShaderBitfieldOperationCaseFrexpVec2(m_context, "vec2_zero", m_glslVersion, Vec4(0.0, 0.0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec2_" << i;
		frexpGroup->addChild(new ShaderBitfieldOperationCaseFrexpVec2(m_context, ss.str().c_str(), m_glslVersion,
																	  Vec4(rnd.getFloat(), -rnd.getFloat())));
	}
	frexpGroup->addChild(
		new ShaderBitfieldOperationCaseFrexpVec3(m_context, "vec3_zero", m_glslVersion, Vec4(0.0, 0.0, 0.0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec3_" << i;
		frexpGroup->addChild(new ShaderBitfieldOperationCaseFrexpVec3(
			m_context, ss.str().c_str(), m_glslVersion, Vec4(rnd.getFloat(), -rnd.getFloat(), rnd.getFloat())));
	}
	frexpGroup->addChild(
		new ShaderBitfieldOperationCaseFrexpVec4(m_context, "vec4_zero", m_glslVersion, Vec4(0.0, 0.0, 0.0, 0.0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec4_" << i;
		frexpGroup->addChild(new ShaderBitfieldOperationCaseFrexpVec4(
			m_context, ss.str().c_str(), m_glslVersion,
			Vec4(rnd.getFloat(), -rnd.getFloat(), rnd.getFloat(), -rnd.getFloat())));
	}

	// shader_bitfield_operation.ldexp
	tcu::TestCaseGroup* ldexpGroup = new tcu::TestCaseGroup(m_testCtx, "ldexp", "");
	addChild(ldexpGroup);
	ldexpGroup->addChild(
		new ShaderBitfieldOperationCaseLdexpFloat(m_context, "float_zero", m_glslVersion, Vec4(0.0), Ivec4(0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "float_" << i;
		ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpFloat(m_context, ss.str().c_str(), m_glslVersion,
																	   Vec4(rnd.getFloat()), Ivec4(rnd.getInt(-8, 8))));
	}
	ldexpGroup->addChild(
		new ShaderBitfieldOperationCaseLdexpVec2(m_context, "vec2_zero", m_glslVersion, Vec4(0.0, 0.0), Ivec4(0, 0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec2_" << i;
		ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpVec2(m_context, ss.str().c_str(), m_glslVersion,
																	  Vec4(rnd.getFloat(), -rnd.getFloat()),
																	  Ivec4(rnd.getInt(-8, 8), rnd.getInt(-8, 8))));
	}
	ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpVec3(m_context, "vec3_zero", m_glslVersion,
																  Vec4(0.0, 0.0, 0.0), Ivec4(0, 0, 0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec3_" << i;
		ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpVec3(
			m_context, ss.str().c_str(), m_glslVersion, Vec4(rnd.getFloat(), -rnd.getFloat(), rnd.getFloat()),
			Ivec4(rnd.getInt(-8, 8), rnd.getInt(-8, 8), rnd.getInt(-8, 8))));
	}
	ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpVec4(m_context, "vec4_zero", m_glslVersion,
																  Vec4(0.0, 0.0, 0.0, 0.0), Ivec4(0, 0, 0, 0)));
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "vec4_" << i;
		ldexpGroup->addChild(new ShaderBitfieldOperationCaseLdexpVec4(
			m_context, ss.str().c_str(), m_glslVersion,
			Vec4(rnd.getFloat(), -rnd.getFloat(), rnd.getFloat(), -rnd.getFloat()),
			Ivec4(rnd.getInt(-8, 8), rnd.getInt(-8, 8), rnd.getInt(-8, 8), rnd.getInt(-8, 8))));
	}

	// shader_bitfield_operation.packUnorm4x8
	tcu::TestCaseGroup* packUnormGroup = new tcu::TestCaseGroup(m_testCtx, "packUnorm4x8", "");
	addChild(packUnormGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << i;
		packUnormGroup->addChild(new ShaderBitfieldOperationCasePackUnorm(
			m_context, ss.str().c_str(), m_glslVersion,
			Vec4(rnd.getFloat(), rnd.getFloat(), rnd.getFloat(), rnd.getFloat())));
	}

	// shader_bitfield_operation.packSnorm4x8
	tcu::TestCaseGroup* packSnormGroup = new tcu::TestCaseGroup(m_testCtx, "packSnorm4x8", "");
	addChild(packSnormGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << i;
		packSnormGroup->addChild(new ShaderBitfieldOperationCasePackSnorm(
			m_context, ss.str().c_str(), m_glslVersion,
			Vec4(rnd.getFloat(), -rnd.getFloat(), rnd.getFloat(), -rnd.getFloat())));
	}

	// shader_bitfield_operation.unpackUnorm4x8
	tcu::TestCaseGroup* unpackUnormGroup = new tcu::TestCaseGroup(m_testCtx, "unpackUnorm4x8", "");
	addChild(unpackUnormGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << i;
		unpackUnormGroup->addChild(new ShaderBitfieldOperationCaseUnpackUnorm(m_context, ss.str().c_str(),
																			  m_glslVersion, Uvec4(rnd.getUint32())));
	}

	// shader_bitfield_operation.unpackSnorm4x8
	tcu::TestCaseGroup* unpackSnormGroup = new tcu::TestCaseGroup(m_testCtx, "unpackSnorm4x8", "");
	addChild(unpackSnormGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << i;
		unpackSnormGroup->addChild(new ShaderBitfieldOperationCaseUnpackSnorm(m_context, ss.str().c_str(),
																			  m_glslVersion, Uvec4(rnd.getUint32())));
	}

	// shader_bitfield_operation.bitfieldExtract
	tcu::TestCaseGroup* bitfieldExtractGroup = new tcu::TestCaseGroup(m_testCtx, "bitfieldExtract", "");
	addChild(bitfieldExtractGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uint_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractUint1(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec2_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractUint2(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec3_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractUint3(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec4_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractUint4(
			m_context, ss.str().c_str(), m_glslVersion,
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}

	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "int_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractInt1(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec2_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractInt2(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec3_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractInt3(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec4_" << i;
		bitfieldExtractGroup->addChild(new ShaderBitfieldOperationCaseBitfieldExtractInt4(
			m_context, ss.str().c_str(), m_glslVersion,
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}

	// shader_bitfield_operation.bitfieldInsert
	tcu::TestCaseGroup* bitfieldInsertGroup = new tcu::TestCaseGroup(m_testCtx, "bitfieldInsert", "");
	addChild(bitfieldInsertGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uint_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertUint1(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32()), Uvec4(rnd.getUint32()),
			rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec2_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertUint2(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec3_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertUint3(
			m_context, ss.str().c_str(), m_glslVersion, Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec4_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertUint4(
			m_context, ss.str().c_str(), m_glslVersion,
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}

	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "int_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertInt1(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32()), Ivec4(rnd.getUint32()),
			rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec2_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertInt2(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32(), rnd.getUint32()),
			Ivec4(rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec3_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertInt3(
			m_context, ss.str().c_str(), m_glslVersion, Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31), rnd.getInt(1, 32)));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec4_" << i;
		bitfieldInsertGroup->addChild(new ShaderBitfieldOperationCaseBitfieldInsertInt4(
			m_context, ss.str().c_str(), m_glslVersion,
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()), rnd.getInt(0, 31),
			rnd.getInt(1, 32)));
	}

	struct UnaryTest
	{
		char const* funcName;
		UnaryUFunc  funcU;
		UnaryIFunc  funcI;
	} commonTests[] = {
		{ "bitfieldReverse", bitfieldReverse,
		  reinterpret_cast<UnaryIFunc>(bitfieldReverse) },				  // shader_bitfield_operation.bitfieldReverse
		{ "bitCount", bitCount, reinterpret_cast<UnaryIFunc>(bitCount) }, // shader_bitfield_operation.bitCount
		{ "findLSB", findLSB, reinterpret_cast<UnaryIFunc>(findLSB) },	// shader_bitfield_operation.findLSB
		{ "findMSB", findMSBU, findMSBI },								  // shader_bitfield_operation.findMSB
	};
	for (int test = 0; test < DE_LENGTH_OF_ARRAY(commonTests); ++test)
	{
		tcu::TestCaseGroup* commonGroup = new tcu::TestCaseGroup(m_testCtx, commonTests[test].funcName, "");
		addChild(commonGroup);
		commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryUint1(
			m_context, "uint_zero", m_glslVersion, commonTests[test].funcName, commonTests[test].funcU, Uvec4(0)));
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "uint_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryUint1(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcU,
				Uvec4(rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "uvec2_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryUint2(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcU,
				Uvec4(rnd.getUint32(), rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "uvec3_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryUint3(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcU,
				Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "uvec4_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryUint4(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcU,
				Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
		}

		commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt1(
			m_context, "int_zero", m_glslVersion, commonTests[test].funcName, commonTests[test].funcI, Ivec4(0)));
		commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt1(
			m_context, "int_minus_one", m_glslVersion, commonTests[test].funcName, commonTests[test].funcI, Ivec4(-1)));
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "int_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt1(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcI,
				Ivec4(rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "ivec2_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt2(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcI,
				Ivec4(rnd.getUint32(), rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "ivec3_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt3(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcI,
				Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
		}
		for (int i = 0; i < ITERATIONS; ++i)
		{
			std::stringstream ss;
			ss << "ivec4_" << i;
			commonGroup->addChild(new ShaderBitfieldOperationCaseUnaryInt4(
				m_context, ss.str().c_str(), m_glslVersion, commonTests[test].funcName, commonTests[test].funcI,
				Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
		}
	}

	// shader_bitfield_operation.uaddCarry
	tcu::TestCaseGroup* uaddCarryGroup = new tcu::TestCaseGroup(m_testCtx, "uaddCarry", "");
	addChild(uaddCarryGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uint_" << i;
		uaddCarryGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint1(
			m_context, ss.str().c_str(), m_glslVersion, "outUvec4.x = uaddCarry(inUvec4.x, in2Uvec4.x, out2Uvec4.x);",
			uaddCarry, Uvec4(rnd.getUint32()), Uvec4(rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec2_" << i;
		uaddCarryGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint2(
			m_context, ss.str().c_str(), m_glslVersion,
			"outUvec4.xy = uaddCarry(inUvec4.xy, in2Uvec4.xy, out2Uvec4.xy);", uaddCarry,
			Uvec4(rnd.getUint32(), rnd.getUint32()), Uvec4(rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec3_" << i;
		uaddCarryGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint3(
			m_context, ss.str().c_str(), m_glslVersion,
			"outUvec4.xyz = uaddCarry(inUvec4.xyz, in2Uvec4.xyz, out2Uvec4.xyz);", uaddCarry,
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec4_" << i;
		uaddCarryGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint4(
			m_context, ss.str().c_str(), m_glslVersion, "outUvec4 = uaddCarry(inUvec4, in2Uvec4, out2Uvec4);",
			uaddCarry, Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}

	// shader_bitfield_operation.usubBorrow
	tcu::TestCaseGroup* usubBorrowGroup = new tcu::TestCaseGroup(m_testCtx, "usubBorrow", "");
	addChild(usubBorrowGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uint_" << i;
		usubBorrowGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint1(
			m_context, ss.str().c_str(), m_glslVersion, "outUvec4.x = usubBorrow(inUvec4.x, in2Uvec4.x, out2Uvec4.x);",
			usubBorrow, Uvec4(rnd.getUint32()), Uvec4(rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec2_" << i;
		usubBorrowGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint2(
			m_context, ss.str().c_str(), m_glslVersion,
			"outUvec4.xy = usubBorrow(inUvec4.xy, in2Uvec4.xy, out2Uvec4.xy);", usubBorrow,
			Uvec4(rnd.getUint32(), rnd.getUint32()), Uvec4(rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec3_" << i;
		usubBorrowGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint3(
			m_context, ss.str().c_str(), m_glslVersion,
			"outUvec4.xyz = usubBorrow(inUvec4.xyz, in2Uvec4.xyz, out2Uvec4.xyz);", usubBorrow,
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec4_" << i;
		usubBorrowGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint4(
			m_context, ss.str().c_str(), m_glslVersion, "outUvec4 = usubBorrow(inUvec4, in2Uvec4, out2Uvec4);",
			usubBorrow, Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}

	// shader_bitfield_operation.umulExtended
	tcu::TestCaseGroup* umulExtendedGroup = new tcu::TestCaseGroup(m_testCtx, "umulExtended", "");
	addChild(umulExtendedGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uint_" << i;
		umulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint1(
			m_context, ss.str().c_str(), m_glslVersion, "umulExtended(inUvec4.x, in2Uvec4.x, outUvec4.x, out2Uvec4.x);",
			umulExtended, Uvec4(rnd.getUint32()), Uvec4(rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec2_" << i;
		umulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint2(
			m_context, ss.str().c_str(), m_glslVersion,
			"umulExtended(inUvec4.xy, in2Uvec4.xy, outUvec4.xy, out2Uvec4.xy);", umulExtended,
			Uvec4(rnd.getUint32(), rnd.getUint32()), Uvec4(rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec3_" << i;
		umulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint3(
			m_context, ss.str().c_str(), m_glslVersion,
			"umulExtended(inUvec4.xyz, in2Uvec4.xyz, outUvec4.xyz, out2Uvec4.xyz);", umulExtended,
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "uvec4_" << i;
		umulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryUint4(
			m_context, ss.str().c_str(), m_glslVersion, "umulExtended(inUvec4, in2Uvec4, outUvec4, out2Uvec4);",
			umulExtended, Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Uvec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}

	// shader_bitfield_operation.imulExtended
	tcu::TestCaseGroup* imulExtendedGroup = new tcu::TestCaseGroup(m_testCtx, "imulExtended", "");
	addChild(imulExtendedGroup);
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "int_" << i;
		imulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryInt1(
			m_context, ss.str().c_str(), m_glslVersion, "imulExtended(inIvec4.x, in2Ivec4.x, outIvec4.x, out2Ivec4.x);",
			imulExtended, Ivec4(rnd.getUint32()), Ivec4(rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec2_" << i;
		imulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryInt2(
			m_context, ss.str().c_str(), m_glslVersion,
			"imulExtended(inIvec4.xy, in2Ivec4.xy, outIvec4.xy, out2Ivec4.xy);", imulExtended,
			Ivec4(rnd.getUint32(), rnd.getUint32()), Ivec4(rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec3_" << i;
		imulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryInt3(
			m_context, ss.str().c_str(), m_glslVersion,
			"imulExtended(inIvec4.xyz, in2Ivec4.xyz, outIvec4.xyz, out2Ivec4.xyz);", imulExtended,
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}
	for (int i = 0; i < ITERATIONS; ++i)
	{
		std::stringstream ss;
		ss << "ivec4_" << i;
		imulExtendedGroup->addChild(new ShaderBitfieldOperationCaseBinaryInt4(
			m_context, ss.str().c_str(), m_glslVersion, "imulExtended(inIvec4, in2Ivec4, outIvec4, out2Ivec4);",
			imulExtended, Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32()),
			Ivec4(rnd.getUint32(), rnd.getUint32(), rnd.getUint32(), rnd.getUint32())));
	}
}

} // glcts
