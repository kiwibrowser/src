/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file  InternalformatTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcInternalformatTests.hpp"
#include "deMath.h"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"

#include <algorithm>
#include <map>

using namespace glw;

namespace glcts
{

// all extension names required by the tests
static const char* EXT_texture_type_2_10_10_10_REV = "GL_EXT_texture_type_2_10_10_10_REV";
static const char* EXT_texture_shared_exponent	 = "GL_EXT_texture_shared_exponent";
static const char* EXT_texture_integer			   = "GL_EXT_texture_integer";
static const char* ARB_texture_rgb10_a2ui		   = "GL_ARB_texture_rgb10_a2ui";
static const char* ARB_depth_texture			   = "GL_ARB_depth_texture";
static const char* ARB_texture_float			   = "GL_ARB_texture_float";
static const char* OES_texture_float			   = "GL_OES_texture_float";
static const char* OES_texture_float_linear		   = "GL_OES_texture_float_linear";
static const char* OES_texture_half_float		   = "GL_OES_texture_half_float";
static const char* OES_texture_half_float_linear   = "GL_OES_texture_half_float_linear";
static const char* OES_rgb8_rgba8				   = "GL_OES_rgb8_rgba8";
static const char* OES_depth_texture			   = "GL_OES_depth_texture";
static const char* OES_depth24					   = "GL_OES_depth24";
static const char* OES_depth32					   = "GL_OES_depth32";
static const char* OES_packed_depth_stencil		   = "GL_OES_packed_depth_stencil";
static const char* OES_stencil1					   = "GL_OES_stencil1";
static const char* OES_stencil4					   = "GL_OES_stencil4";
static const char* OES_stencil8					   = "GL_OES_stencil8";
static const char* OES_required_internalformat	 = "GL_OES_required_internalformat";

struct TextureFormat
{
	GLenum		format;
	GLenum		type;
	GLint		internalFormat;
	const char* requiredExtension;
	const char* secondReqiredExtension;
	GLint		minFilter;
	GLint		magFilter;

	TextureFormat()
	{
	}

	TextureFormat(GLenum aFormat, GLenum aType, GLint aInternalFormat, const char* aRequiredExtension = DE_NULL,
				  const char* aSecondReqiredExtension = DE_NULL, GLint aMinFilter = GL_NEAREST,
				  GLint aMagFilter = GL_NEAREST)
		: format(aFormat)
		, type(aType)
		, internalFormat(aInternalFormat)
		, requiredExtension(aRequiredExtension)
		, secondReqiredExtension(aSecondReqiredExtension)
		, minFilter(aMinFilter)
		, magFilter(aMagFilter)
	{
	}
};

struct CopyTexImageFormat
{
	GLint		internalFormat;
	const char* requiredExtension;
	const char* secondReqiredExtension;
	GLint		minFilter;
	GLint		magFilter;

	CopyTexImageFormat(GLenum aInternalFormat, const char* aRequiredExtension = DE_NULL,
					   const char* aSecondReqiredExtension = DE_NULL, GLint aMinFilter = GL_NEAREST,
					   GLint aMagFilter = GL_NEAREST)
		: internalFormat(aInternalFormat)
		, requiredExtension(aRequiredExtension)
		, secondReqiredExtension(aSecondReqiredExtension)
		, minFilter(aMinFilter)
		, magFilter(aMagFilter)
	{
	}
};

enum RenderBufferType
{
	RENDERBUFFER_COLOR,
	RENDERBUFFER_STENCIL,
	RENDERBUFFER_DEPTH,
	RENDERBUFFER_DEPTH_STENCIL
};

struct RenderbufferFormat
{
	GLenum			 format;
	RenderBufferType type;
	const char*		 requiredExtension;
	const char*		 secondReqiredExtension;

	RenderbufferFormat(GLenum aFormat, RenderBufferType aType, const char* aRequiredExtension = DE_NULL,
					   const char* aSecondReqiredExtension = DE_NULL)
		: format(aFormat)
		, type(aType)
		, requiredExtension(aRequiredExtension)
		, secondReqiredExtension(aSecondReqiredExtension)
	{
	}
};

class InternalformatCaseBase : public deqp::TestCase
{
public:
	InternalformatCaseBase(deqp::Context& context, const std::string& name);
	virtual ~InternalformatCaseBase()
	{
	}

protected:
	bool requiredExtensionsSupported(const char* extension1, const char* extension2);
	GLuint createTexture(GLint internalFormat, GLenum format, GLenum type, GLint minFilter, GLint magFilter,
						 bool generateData = true) const;
	glu::ProgramSources prepareTexturingProgramSources(GLint internalFormat, GLenum format, GLenum type) const;
	void renderTexturedQuad(GLuint programId) const;

private:
	void generateTextureData(GLuint width, GLuint height, GLenum type, unsigned int pixelSize, unsigned int components,
							 std::vector<unsigned char>& result) const;

	// color converting methods
	static void convertByte(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertUByte(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertHFloat(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertFloat(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertShort(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertUShort(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertInt(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertUInt(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertUInt_24_8(tcu::Vec4 inColor, unsigned char* dst, int components);
	static void convertUShort_4_4_4_4(tcu::Vec4 inColor, unsigned char* dst, int);
	static void convertUShort_5_5_5_1(tcu::Vec4 inColor, unsigned char* dst, int);
	static void convertUShort_5_6_5(tcu::Vec4 inColor, unsigned char* dst, int);
	static void convertUInt_2_10_10_10_rev(tcu::Vec4 inColor, unsigned char* dst, int);

	static GLhalf floatToHalf(float f);

protected:
	GLsizei m_renderWidth;
	GLsizei m_renderHeight;
};

InternalformatCaseBase::InternalformatCaseBase(deqp::Context& context, const std::string& name)
	: deqp::TestCase(context, name.c_str(), ""), m_renderWidth(64), m_renderHeight(64)
{
}

bool InternalformatCaseBase::requiredExtensionsSupported(const char* extension1, const char* extension2)
{
	const glu::ContextInfo& contextInfo = m_context.getContextInfo();
	if (extension1)
	{
		if (extension2)
		{
			if (!contextInfo.isExtensionSupported(extension1) || !contextInfo.isExtensionSupported(extension2))
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "One of required extensions is not supported");
				return false;
			}
		}
		else if (!contextInfo.isExtensionSupported(extension1))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Required extension is not supported");
			return false;
		}
	}
	return true;
}

GLuint InternalformatCaseBase::createTexture(GLint internalFormat, GLenum format, GLenum type, GLint minFilter,
											 GLint magFilter, bool generateData) const
{
	const Functions&		   gl = m_context.getRenderContext().getFunctions();
	GLuint					   textureName;
	std::vector<unsigned char> textureData;
	GLvoid*					   textureDataPtr = DE_NULL;

	if (generateData)
	{
		tcu::TextureFormat tcuTextureFormat = glu::mapGLTransferFormat(format, type);
		unsigned int	   components		= tcu::getNumUsedChannels(tcuTextureFormat.order);
		unsigned int	   pixelSize		= 4;

		// note: getPixelSize hits assertion for GL_UNSIGNED_INT_2_10_10_10_REV when format is RGB
		if (type != GL_UNSIGNED_INT_2_10_10_10_REV)
			pixelSize = tcu::getPixelSize(tcuTextureFormat);

		generateTextureData(m_renderWidth, m_renderHeight, type, pixelSize, components, textureData);
		textureDataPtr = &textureData[0];
	}

	gl.genTextures(1, &textureName);
	gl.bindTexture(GL_TEXTURE_2D, textureName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");

	gl.texImage2D(GL_TEXTURE_2D, 0, internalFormat, m_renderWidth, m_renderHeight, 0, format, type, textureDataPtr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");

	return textureName;
}

glu::ProgramSources InternalformatCaseBase::prepareTexturingProgramSources(GLint internalFormat, GLenum format,
																		   GLenum type) const
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	glu::ContextType	contextType   = renderContext.getType();
	glu::GLSLVersion	glslVersion   = glu::getContextTypeGLSLVersion(contextType);

	std::string vs;
	std::string fs;

	std::map<std::string, std::string> specializationMap;
	specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (glu::contextSupports(contextType, glu::ApiType::es(3, 0)) || glu::isContextTypeGLCore(contextType))
	{
		vs = "${VERSION}\n"
			 "precision highp float;\n"
			 "in vec2 position;\n"
			 "in vec2 inTexcoord;\n"
			 "out vec2 texcoord;\n"
			 "void main()\n"
			 "{\n"
			 "  texcoord = inTexcoord;\n"
			 "  gl_Position = vec4(position, 0.0, 1.0);\n"
			 "}\n";
		fs = "${VERSION}\n"
			 "precision highp float;\n"
			 "uniform ${SAMPLER} sampler;\n"
			 "in vec2 texcoord;\n"
			 "out highp vec4 color;\n"
			 "void main()\n"
			 "{\n"
			 "  ${SAMPLED_TYPE} v = texture(sampler, texcoord);\n"
			 "  color = ${CALCULATE_COLOR};\n"
			 "  ${PROCESS_COLOR}\n"
			 "}\n";

		specializationMap["PROCESS_COLOR"] = "";
		if ((format == GL_RGB_INTEGER) || (format == GL_RGBA_INTEGER))
		{
			specializationMap["SAMPLED_TYPE"] = "uvec4";
			specializationMap["SAMPLER"]	  = "usampler2D";
			if (type == GL_BYTE)
			{
				specializationMap["SAMPLED_TYPE"]	= "ivec4";
				specializationMap["SAMPLER"]		 = "isampler2D";
				specializationMap["CALCULATE_COLOR"] = "vec4(v) / 127.0";
			}
			else if (type == GL_UNSIGNED_BYTE)
			{
				specializationMap["CALCULATE_COLOR"] = "vec4(v) / 255.0";
			}
			else if (type == GL_SHORT)
			{
				specializationMap["SAMPLED_TYPE"]	= "ivec4";
				specializationMap["SAMPLER"]		 = "isampler2D";
				specializationMap["CALCULATE_COLOR"] = "vec4(v / 128) / 256.0";
			}
			else if (type == GL_UNSIGNED_SHORT)
			{
				specializationMap["CALCULATE_COLOR"] = "vec4(v / 256) / 256.0";
			}
			else if (type == GL_INT)
			{
				specializationMap["SAMPLED_TYPE"]	= "ivec4";
				specializationMap["SAMPLER"]		 = "isampler2D";
				specializationMap["CALCULATE_COLOR"] = "vec4(v / 2097152u) / 1024.0";
			}
			else // GL_UNSIGNED_INT
			{
				if (internalFormat == GL_RGB10_A2UI)
					specializationMap["CALCULATE_COLOR"] = "vec4(vec3(v.rgb) / 1023.0, float(v.a) / 3.0)";
				else
					specializationMap["CALCULATE_COLOR"] = "vec4(v / 4194304u) / 1024.0";
			}

			if (format == GL_RGB_INTEGER)
				specializationMap["PROCESS_COLOR"] = "color.a = 1.0;\n";
		}
		else
		{
			specializationMap["SAMPLED_TYPE"]	= "vec4";
			specializationMap["SAMPLER"]		 = "sampler2D";
			specializationMap["CALCULATE_COLOR"] = "v";
		}
	}
	else
	{
		vs = "${VERSION}\n"
			 "attribute highp vec2 position;\n"
			 "attribute highp vec2 inTexcoord;\n"
			 "varying highp vec2 texcoord;\n"
			 "void main()\n"
			 "{\n"
			 "  texcoord = inTexcoord;\n"
			 "  gl_Position = vec4(position, 0.0, 1.0);\n"
			 "}\n";
		fs = "${VERSION}\n"
			 "uniform highp sampler2D sampler;\n"
			 "varying highp vec2 texcoord;\n"
			 "void main()\n"
			 "{\n"
			 "  gl_FragColor = texture2D(sampler, texcoord);\n"
			 "}\n";
	}

	vs = tcu::StringTemplate(vs).specialize(specializationMap);
	fs = tcu::StringTemplate(fs).specialize(specializationMap);
	return glu::makeVtxFragSources(vs.c_str(), fs.c_str());
}

void InternalformatCaseBase::renderTexturedQuad(GLuint programId) const
{
	// Prepare data for rendering
	static const deUint16				 quadIndices[]  = { 0, 1, 2, 2, 1, 3 };
	static const float					 position[]		= { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
	static const float					 texCoord[]		= { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
	static const glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 2, 4, 0, position),
															glu::va::Float("inTexcoord", 2, 4, 0, texCoord) };

	glu::draw(m_context.getRenderContext(), programId, DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));
}

void InternalformatCaseBase::generateTextureData(GLuint width, GLuint height, GLenum type, unsigned int pixelSize,
												 unsigned int components, std::vector<unsigned char>& result) const
{
	// colors are the 4 corner colors specified ( lower left, lower right, upper left, upper right )
	static tcu::Vec4 colors[4] = { tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f),
								   tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f), tcu::Vec4(0.0f, 1.0f, 1.0f, 1.0f) };

	typedef void (*ColorConversionFunc)(tcu::Vec4, unsigned char*, int);
	typedef std::map<GLenum, ColorConversionFunc> ColorConversionMap;
	static ColorConversionMap colorConversionMap;
	if (colorConversionMap.empty())
	{
		colorConversionMap[GL_BYTE]						   = &convertByte;
		colorConversionMap[GL_UNSIGNED_BYTE]			   = &convertUByte;
		colorConversionMap[GL_HALF_FLOAT]				   = &convertHFloat;
		colorConversionMap[GL_FLOAT]					   = &convertFloat;
		colorConversionMap[GL_SHORT]					   = &convertShort;
		colorConversionMap[GL_UNSIGNED_SHORT]			   = &convertUShort;
		colorConversionMap[GL_INT]						   = &convertInt;
		colorConversionMap[GL_UNSIGNED_INT]				   = &convertUInt;
		colorConversionMap[GL_UNSIGNED_INT_24_8]		   = &convertUInt_24_8;
		colorConversionMap[GL_UNSIGNED_SHORT_4_4_4_4]	  = &convertUShort_4_4_4_4;
		colorConversionMap[GL_UNSIGNED_SHORT_5_5_5_1]	  = &convertUShort_5_5_5_1;
		colorConversionMap[GL_UNSIGNED_SHORT_5_6_5]		   = &convertUShort_5_6_5;
		colorConversionMap[GL_UNSIGNED_INT_2_10_10_10_REV] = &convertUInt_2_10_10_10_rev;
	}

	ColorConversionFunc convertColor = colorConversionMap.at(type);

	float lwidth  = static_cast<float>(width - 1);
	float lheight = static_cast<float>(height - 1);

	result.resize(width * height * pixelSize);
	unsigned char* dataPtr = &result[0];

	for (GLuint y = 0; y < height; ++y)
	{
		for (GLuint x = 0; x < width; ++x)
		{
			float	 posX  = (lwidth - static_cast<float>(x)) / lwidth;
			float	 posY  = (lheight - static_cast<float>(y)) / lheight;
			float	 rposX = 1.f - posX;
			float	 rposY = 1.f - posY;
			tcu::Vec4 c		= colors[0] * (posX * posY) + colors[1] * (rposX * posY) + colors[2] * (posX * rposY) +
						  colors[3] * (rposX * rposY);
			convertColor(c, dataPtr, static_cast<int>(components));
			dataPtr += pixelSize;
		}
	}
}

void InternalformatCaseBase::convertByte(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	char* dstChar = reinterpret_cast<char*>(dst);
	for (int i	 = 0; i < components; ++i)
		dstChar[i] = static_cast<char>(inColor[i] * 127.0f);
}

void InternalformatCaseBase::convertUByte(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	for (int i = 0; i < components; ++i)
		dst[i] = static_cast<unsigned char>(inColor[i] * 255.f);
}

void InternalformatCaseBase::convertHFloat(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	GLhalf* dstHalf = reinterpret_cast<GLhalf*>(dst);
	for (int i	 = 0; i < components; ++i)
		dstHalf[i] = floatToHalf(inColor[i]);
}

void InternalformatCaseBase::convertFloat(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	float* dstFloat = reinterpret_cast<float*>(dst);
	for (int i		= 0; i < components; ++i)
		dstFloat[i] = inColor[i];
}

void InternalformatCaseBase::convertShort(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	short* dstUShort = reinterpret_cast<short*>(dst);
	for (int i = 0; i < components; ++i)
	{
		double c	 = static_cast<double>(inColor[i]);
		dstUShort[i] = static_cast<short>(c * 32768 - 1);
	}
}

void InternalformatCaseBase::convertUShort(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	unsigned short* dstUShort = reinterpret_cast<unsigned short*>(dst);
	for (int i = 0; i < components; ++i)
	{
		double c	 = static_cast<double>(inColor[i]);
		dstUShort[i] = static_cast<unsigned short>(c * 65535u);
	}
}

void InternalformatCaseBase::convertInt(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	int* dstUInt = reinterpret_cast<int*>(dst);
	for (int i	 = 0; i < components; ++i)
		dstUInt[i] = static_cast<int>(inColor[i] * 2147483648u - 1);
}

void InternalformatCaseBase::convertUInt(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	unsigned int* dstUInt = reinterpret_cast<unsigned int*>(dst);
	for (int i = 0; i < components; ++i)
	{
		double c   = static_cast<double>(inColor[i]);
		dstUInt[i] = static_cast<unsigned int>(c * 4294967295u);
	}
}

void InternalformatCaseBase::convertUInt_24_8(tcu::Vec4 inColor, unsigned char* dst, int components)
{
	unsigned int* dstUint = reinterpret_cast<unsigned int*>(dst);
	for (int i = 0; i < components; ++i)
	{
		dstUint[i] = static_cast<unsigned int>(inColor[i] * 4294967295u);

		// Last 8 bits for stencil so clear those
		dstUint[i] = dstUint[i] & 0xFFFFFF00;
	}
}

void InternalformatCaseBase::convertUShort_4_4_4_4(tcu::Vec4 inColor, unsigned char* dst, int)
{
	unsigned short* dstUShort = reinterpret_cast<unsigned short*>(dst);

	unsigned int r = static_cast<unsigned int>(inColor[0] * 15) << 12;
	unsigned int g = static_cast<unsigned int>(inColor[1] * 15) << 8;
	unsigned int b = static_cast<unsigned int>(inColor[2] * 15) << 4;
	unsigned int a = static_cast<unsigned int>(inColor[3] * 15) << 0;

	dstUShort[0] = (r & 0xF000) | (g & 0x0F00) | (b & 0x00F0) | (a & 0x000F);
}

void InternalformatCaseBase::convertUShort_5_5_5_1(tcu::Vec4 inColor, unsigned char* dst, int)
{
	unsigned short* dstUShort = reinterpret_cast<unsigned short*>(dst);

	unsigned int r = static_cast<unsigned int>(inColor[0] * 31) << 11;
	unsigned int g = static_cast<unsigned int>(inColor[1] * 31) << 6;
	unsigned int b = static_cast<unsigned int>(inColor[2] * 31) << 1;
	unsigned int a = static_cast<unsigned int>(inColor[3] * 1) << 0;

	dstUShort[0] = (r & 0xF800) | (g & 0x07c0) | (b & 0x003e) | (a & 0x0001);
}

void InternalformatCaseBase::convertUShort_5_6_5(tcu::Vec4 inColor, unsigned char* dst, int)
{
	unsigned short* dstUShort = reinterpret_cast<unsigned short*>(dst);

	unsigned int r = static_cast<unsigned int>(inColor[0] * 31) << 11;
	unsigned int g = static_cast<unsigned int>(inColor[1] * 63) << 5;
	unsigned int b = static_cast<unsigned int>(inColor[2] * 31) << 0;

	dstUShort[0] = (r & 0xF800) | (g & 0x07e0) | (b & 0x001f);
}

void InternalformatCaseBase::convertUInt_2_10_10_10_rev(tcu::Vec4 inColor, unsigned char* dst, int)
{
	unsigned int* dstUint = reinterpret_cast<unsigned int*>(dst);

	// Alpha value is rounded to eliminate small precision errors that
	// may result in big errors after converting value to just 4 bits
	unsigned int a = static_cast<unsigned int>(deFloatRound(inColor[3] * 3)) << 30;
	unsigned int b = static_cast<unsigned int>(inColor[2] * 1023) << 20;
	unsigned int g = static_cast<unsigned int>(inColor[1] * 1023) << 10;
	unsigned int r = static_cast<unsigned int>(inColor[0] * 1023) << 0;

	dstUint[0] = (a & 0xC0000000) | (b & 0x3FF00000) | (g & 0x000FFC00) | (r & 0x000003FF);
}

GLhalf InternalformatCaseBase::floatToHalf(float f)
{
	const unsigned int HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
	// Max exponent value in single precision that will be converted
	// to Inf or Nan when stored as a half-float
	const unsigned int HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
	// 255 is the max exponent biased value
	const unsigned int FLOAT_MAX_BIASED_EXP		 = (0xFF << 23);
	const unsigned int HALF_FLOAT_MAX_BIASED_EXP = (0x1F << 10);

	char*		 c	= reinterpret_cast<char*>(&f);
	unsigned int x	= *reinterpret_cast<unsigned int*>(c);
	unsigned int sign = static_cast<GLhalf>(x >> 31);

	// Get mantissa
	unsigned int mantissa = x & ((1 << 23) - 1);
	// Get exponent bits
	unsigned int exp = x & FLOAT_MAX_BIASED_EXP;

	if (exp >= HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		// Check if the original single precision float number is a NaN
		if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
		{
			// We have a single precision NaN
			mantissa = (1 << 23) - 1;
		}
		else
		{
			// 16-bit half-float representation stores number as Inf
			mantissa = 0;
		}
		return (GLhalf)((((GLhalf)sign) << 15) | (GLhalf)(HALF_FLOAT_MAX_BIASED_EXP) | (GLhalf)(mantissa >> 13));
	}
	// Check if exponent is <= -15
	else if (exp <= HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP)
	{
		// Store a denorm half-float value or zero
		exp = (HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23;
		mantissa |= (1 << 23);
		mantissa >>= (14 + exp);
		return (GLhalf)((((GLhalf)sign) << 15) | (GLhalf)(mantissa));
	}

	return (GLhalf)((((GLhalf)sign) << 15) | (GLhalf)((exp - HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 13) |
					(GLhalf)(mantissa >> 13));
}

class Texture2DCase : public InternalformatCaseBase
{
public:
	Texture2DCase(deqp::Context& context, const std::string& name, const TextureFormat& textureFormat);
	virtual ~Texture2DCase()
	{
	}

	virtual tcu::TestNode::IterateResult iterate(void);

private:
	TextureFormat m_testFormat;
};

Texture2DCase::Texture2DCase(deqp::Context& context, const std::string& name, const TextureFormat& testFormat)
	: InternalformatCaseBase(context, name.c_str()), m_testFormat(testFormat)
{
}

tcu::TestNode::IterateResult Texture2DCase::iterate(void)
{
	if (!requiredExtensionsSupported(m_testFormat.requiredExtension, m_testFormat.secondReqiredExtension))
		return STOP;

	typedef std::map<GLenum, TextureFormat> ReferenceFormatMap;
	static ReferenceFormatMap formatMap;
	if (formatMap.empty())
	{
		formatMap[GL_RED]			  = TextureFormat(GL_RED, GL_UNSIGNED_BYTE, GL_RED);
		formatMap[GL_RG]			  = TextureFormat(GL_RG, GL_UNSIGNED_BYTE, GL_RG);
		formatMap[GL_RGB]			  = TextureFormat(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB);
		formatMap[GL_RGBA]			  = TextureFormat(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB);
		formatMap[GL_RGBA_INTEGER]	= TextureFormat(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB);
		formatMap[GL_RGB_INTEGER]	 = TextureFormat(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB);
		formatMap[GL_ALPHA]			  = TextureFormat(GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA);
		formatMap[GL_LUMINANCE]		  = TextureFormat(GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE);
		formatMap[GL_LUMINANCE_ALPHA] = TextureFormat(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE_ALPHA);
		formatMap[GL_DEPTH_COMPONENT] = TextureFormat(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT);
		formatMap[GL_DEPTH_STENCIL]   = TextureFormat(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8);
	}

	ReferenceFormatMap::iterator formatIterator = formatMap.find(m_testFormat.format);
	if (formatIterator == formatMap.end())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error: Unknown 2D texture format "
						   << glu::getTextureFormatStr(m_testFormat.format).toString() << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	const TextureFormat& referenceFormat = formatIterator->second;
	glu::RenderContext&  renderContext   = m_context.getRenderContext();
	const Functions&	 gl				 = renderContext.getFunctions();

	// Setup viewport
	gl.viewport(0, 0, m_renderWidth, m_renderHeight);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Create test and reference texture
	GLuint testTextureName = createTexture(m_testFormat.internalFormat, m_testFormat.format, m_testFormat.type,
										   m_testFormat.minFilter, m_testFormat.magFilter);
	GLuint referenceTextureName = createTexture(referenceFormat.internalFormat, referenceFormat.format,
												referenceFormat.type, m_testFormat.minFilter, m_testFormat.magFilter);

	// Create program that will render tested texture to screen
	glu::ShaderProgram testProgram(
		renderContext,
		prepareTexturingProgramSources(m_testFormat.internalFormat, m_testFormat.format, m_testFormat.type));
	if (!testProgram.isOk())
	{
		m_testCtx.getLog() << testProgram;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(testProgram.getProgram());
	gl.uniform1i(gl.getUniformLocation(testProgram.getProgram(), "sampler"), 0);

	// Render textured quad with tested texture
	gl.bindTexture(GL_TEXTURE_2D, testTextureName);
	renderTexturedQuad(testProgram.getProgram());
	tcu::Surface testSurface(m_renderWidth, m_renderHeight);
	glu::readPixels(renderContext, 0, 0, testSurface.getAccess());

	// Create program that will render reference texture to screen
	glu::ProgramSources referenceSources =
		prepareTexturingProgramSources(referenceFormat.internalFormat, referenceFormat.format, referenceFormat.type);
	glu::ShaderProgram referenceProgram(renderContext, referenceSources);
	if (!referenceProgram.isOk())
	{
		m_testCtx.getLog() << referenceProgram;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(referenceProgram.getProgram());
	gl.uniform1i(gl.getUniformLocation(referenceProgram.getProgram(), "sampler"), 0);

	// Render textured quad with reference texture
	gl.bindTexture(GL_TEXTURE_2D, referenceTextureName);
	renderTexturedQuad(referenceProgram.getProgram());
	tcu::Surface referenceSurface(m_renderWidth, m_renderHeight);
	glu::readPixels(renderContext, 0, 0, referenceSurface.getAccess());

	// Compare surfaces
	if (tcu::fuzzyCompare(m_testCtx.getLog(), "Result", "Image comparison result", referenceSurface, testSurface, 0.05f,
						  tcu::COMPARE_LOG_RESULT))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	gl.deleteTextures(1, &testTextureName);
	gl.deleteTextures(1, &referenceTextureName);

	return STOP;
}

class CopyTexImageCase : public InternalformatCaseBase
{
public:
	CopyTexImageCase(deqp::Context& context, const std::string& name, const CopyTexImageFormat& copyTexImageFormat);
	virtual ~CopyTexImageCase()
	{
	}

	virtual tcu::TestNode::IterateResult iterate(void);

private:
	GLenum getUnsizedFormatFromInternalFormat(GLint internalFormat) const;
	GLenum getTypeFromInternalFormat(GLint internalFormat) const;

private:
	CopyTexImageFormat m_testFormat;
};

CopyTexImageCase::CopyTexImageCase(deqp::Context& context, const std::string& name,
								   const CopyTexImageFormat& copyTexImageFormat)
	: InternalformatCaseBase(context, name.c_str()), m_testFormat(copyTexImageFormat)
{
}

tcu::TestNode::IterateResult CopyTexImageCase::iterate(void)
{
	if (!requiredExtensionsSupported(m_testFormat.requiredExtension, m_testFormat.secondReqiredExtension))
		return STOP;

	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	// Determine texture format and type
	GLint  textureInternalFormat = m_testFormat.internalFormat;
	GLuint textureType			 = getTypeFromInternalFormat(textureInternalFormat);
	GLuint textureFormat		 = getUnsizedFormatFromInternalFormat(textureInternalFormat);

	// Create program that will render texture to screen
	glu::ShaderProgram program(renderContext,
							   prepareTexturingProgramSources(textureInternalFormat, textureFormat, textureType));
	if (!program.isOk())
	{
		m_testCtx.getLog() << program;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(program.getProgram());
	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "sampler"), 0);
	gl.viewport(0, 0, m_renderWidth, m_renderHeight);

	// Create required textures
	GLuint referenceTextureId = createTexture(textureInternalFormat, textureFormat, textureType, m_testFormat.minFilter,
											  m_testFormat.magFilter);
	GLuint copiedTextureId = createTexture(textureInternalFormat, textureFormat, textureType, m_testFormat.minFilter,
										   m_testFormat.magFilter, false);

	// Create main RGBA framebuffer - this is needed because some default framebuffer may be RGB
	GLuint mainFboId = 0;
	gl.genFramebuffers(1, &mainFboId);
	gl.bindFramebuffer(GL_FRAMEBUFFER, mainFboId);
	GLuint mainFboColorTextureId = createTexture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, false);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainFboColorTextureId, 0);

	// Render reference texture to main FBO and grab it
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.bindTexture(GL_TEXTURE_2D, referenceTextureId);
	renderTexturedQuad(program.getProgram());
	tcu::Surface referenceSurface(m_renderWidth, m_renderHeight);
	glu::readPixels(renderContext, 0, 0, referenceSurface.getAccess());

	GLuint copyFboId				  = 0;
	GLuint copyFboColorTextureId	  = 0;
	bool   internalFormatIsRenderable = (textureInternalFormat != GL_RGB9_E5) && (textureInternalFormat != GL_ALPHA) &&
									  (textureInternalFormat != GL_LUMINANCE) &&
									  (textureInternalFormat != GL_LUMINANCE_ALPHA);

	// When possible use separate FBO for copy operation
	if (internalFormatIsRenderable)
	{
		// Create copy FBO and attach reference texture to color or depth attachment
		gl.genFramebuffers(1, &copyFboId);
		gl.bindFramebuffer(GL_FRAMEBUFFER, copyFboId);

		if (textureFormat == GL_DEPTH_COMPONENT)
		{
			copyFboColorTextureId = createTexture(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, false);
			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyFboColorTextureId, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");
			gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, referenceTextureId, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");
		}
		else
		{
			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, referenceTextureId, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D");
		}

		// Check if FBO is compleate
		GLenum bufferStatus = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (bufferStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			if (bufferStatus == GL_FRAMEBUFFER_UNSUPPORTED)
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unsuported framebuffer");
			else
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Framebuffer not complete");
			return STOP;
		}
	}

	// Copy attachment from copy FBO to tested texture (if copy FBO culdn't be created
	// then copying will be done from main FBO color attachment)
	gl.bindTexture(GL_TEXTURE_2D, copiedTextureId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");
	gl.copyTexImage2D(GL_TEXTURE_2D, 0, textureInternalFormat, 0, 0, m_renderWidth, m_renderHeight, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyTexImage2D");

	// Make sure that main FBO is bound
	gl.bindFramebuffer(GL_FRAMEBUFFER, mainFboId);

	// Render and grab tested texture
	gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.bindTexture(GL_TEXTURE_2D, copiedTextureId);
	renderTexturedQuad(program.getProgram());
	tcu::Surface resultSurface(m_renderWidth, m_renderHeight);
	glu::readPixels(renderContext, 0, 0, resultSurface.getAccess());

	// Compare surfaces
	if (tcu::fuzzyCompare(m_testCtx.getLog(), "Result", "Image comparison result", referenceSurface, resultSurface,
						  0.05f, tcu::COMPARE_LOG_RESULT))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	// Cleanup
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.deleteFramebuffers(1, &mainFboId);
	gl.deleteTextures(1, &mainFboColorTextureId);
	if (copyFboId)
	{
		gl.deleteFramebuffers(1, &copyFboId);
		if (copyFboColorTextureId)
			gl.deleteTextures(1, &copyFboColorTextureId);
	}
	gl.deleteTextures(1, &copiedTextureId);
	gl.deleteTextures(1, &referenceTextureId);

	return STOP;
}

GLenum CopyTexImageCase::getUnsizedFormatFromInternalFormat(GLint internalFormat) const
{
	switch (internalFormat)
	{
	case GL_RGBA:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGB10_A2:
		return GL_RGBA;
	case GL_RGB10_A2UI:
		return GL_RGBA_INTEGER;
	case GL_RGB:
	case GL_RGB565:
	case GL_RGB8:
	case GL_RGB10:
	case GL_RGB9_E5:
		return GL_RGB;
	case GL_LUMINANCE_ALPHA:
	case GL_LUMINANCE4_ALPHA4_OES:
	case GL_LUMINANCE8_ALPHA8_OES:
		return GL_LUMINANCE_ALPHA;
	case GL_LUMINANCE:
	case GL_LUMINANCE8_OES:
		return GL_LUMINANCE;
	case GL_ALPHA:
	case GL_ALPHA8_OES:
		return GL_ALPHA;
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
		return GL_DEPTH_COMPONENT;
	default:
		TCU_FAIL("Unrecognized internal format");
	}
	return GL_NONE;
}

GLenum CopyTexImageCase::getTypeFromInternalFormat(GLint internalFormat) const
{
	switch (internalFormat)
	{
	case GL_RGB10:
	case GL_RGB10_A2:
	case GL_RGB10_A2UI:
		return GL_UNSIGNED_INT_2_10_10_10_REV;
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
		return GL_UNSIGNED_SHORT;
	case GL_DEPTH_COMPONENT32:
		return GL_UNSIGNED_INT;
	}

	return GL_UNSIGNED_BYTE;
}

class RenderbufferCase : public InternalformatCaseBase
{
public:
	RenderbufferCase(deqp::Context& context, const std::string& name, const RenderbufferFormat& renderbufferFormat);
	virtual ~RenderbufferCase();

	virtual tcu::TestNode::IterateResult iterate(void);

private:
	void constructOrthoProjMatrix(GLfloat* mat4, GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n,
								  GLfloat f) const;
	bool   createFramebuffer();
	void   deleteFramebuffer();
	GLuint createAndAttachRenderBuffer(GLenum rbFormat, GLenum fbAttachment);
	void renderColoredQuad(GLuint programId, const float* positions) const;
	glu::ProgramSources prepareColoringProgramSources() const;

private:
	GLuint			   m_fbo;
	GLuint			   m_rbColor;
	GLuint			   m_rbDepth;
	GLuint			   m_rbStencil;
	RenderbufferFormat m_testFormat;
};

RenderbufferCase::RenderbufferCase(deqp::Context& context, const std::string& name,
								   const RenderbufferFormat& renderbufferFormat)
	: InternalformatCaseBase(context, name.c_str())
	, m_fbo(0)
	, m_rbColor(0)
	, m_rbDepth(0)
	, m_rbStencil(0)
	, m_testFormat(renderbufferFormat)
{
}

RenderbufferCase::~RenderbufferCase()
{
}

tcu::TestNode::IterateResult RenderbufferCase::iterate(void)
{
	if (!requiredExtensionsSupported(m_testFormat.requiredExtension, m_testFormat.secondReqiredExtension))
		return STOP;

	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	int maxRenderbufferSize;
	gl.getIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
	int windowWidth  = m_context.getRenderTarget().getWidth();
	int windowHeight = m_context.getRenderTarget().getHeight();
	m_renderWidth	= (windowWidth > maxRenderbufferSize) ? maxRenderbufferSize : windowWidth;
	m_renderHeight   = (windowHeight > maxRenderbufferSize) ? maxRenderbufferSize : windowHeight;

	float			   w					   = static_cast<float>(m_renderWidth);
	float			   h					   = static_cast<float>(m_renderHeight);
	static const float bigQuadPositionsSet[]   = { 0, 0, 0, w, 0, 0, 0, h, 0, w, h, 0 };
	static const float smallQuadPositionsSet[] = { 5.0f, 5.0f,  0.5f, w / 2, 5.0f,  0.5f,
												   5.0f, h / 2, 0.5f, w / 2, h / 2, 0.5f };

	tcu::Surface testSurface[2];
	testSurface[0].setSize(m_renderWidth, m_renderHeight);
	testSurface[1].setSize(m_renderWidth, m_renderHeight);

	GLint defaultFramebufferDepthBits   = 16;
	GLint defaultFramebufferStencilBits = 8;
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.getIntegerv(GL_DEPTH_BITS, &defaultFramebufferDepthBits);
		gl.getIntegerv(GL_STENCIL_BITS, &defaultFramebufferStencilBits);
	}
	else
	{
		gl.getNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
													&defaultFramebufferDepthBits);
		gl.getNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
													&defaultFramebufferStencilBits);
	}

	// Create program that will render texture to screen
	glu::ShaderProgram program(renderContext, prepareColoringProgramSources());
	if (!program.isOk())
	{
		m_testCtx.getLog() << program;
		TCU_FAIL("Compile failed");
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	float mvpMatrix[16];
	constructOrthoProjMatrix(mvpMatrix, 0.0, m_renderWidth, 0.0f, m_renderHeight, 1.0f, -1.0f);
	GLint mvpUniformLocation = gl.getUniformLocation(program.getProgram(), "mvpMatrix");
	gl.uniformMatrix4fv(mvpUniformLocation, 1, 0, mvpMatrix);

	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.viewport(0, 0, m_renderWidth, m_renderHeight);

	if (m_testFormat.type != RENDERBUFFER_STENCIL)
	{
		if (!createFramebuffer())
			return STOP;

		if (defaultFramebufferDepthBits)
		{
			gl.enable(GL_DEPTH_TEST);
			gl.depthFunc(GL_LESS);
		}

		for (GLuint loop = 0; loop < 2; loop++)
		{
			gl.bindFramebuffer(GL_FRAMEBUFFER, loop ? m_fbo : 0);
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			if (defaultFramebufferDepthBits)
			{
				// Draw a small quad just in the z buffer
				gl.colorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				renderColoredQuad(program.getProgram(), smallQuadPositionsSet);

				// Large quad should be drawn on top small one to verify that the depth test is working
				gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}

			// Draws large quad
			renderColoredQuad(program.getProgram(), bigQuadPositionsSet);

			glu::readPixels(renderContext, 0, 0, testSurface[loop].getAccess());
		}

		deleteFramebuffer();

		// Compare surfaces
		if (!tcu::fuzzyCompare(m_testCtx.getLog(), "Result", "Image comparison result", testSurface[0], testSurface[1],
							   0.05f, tcu::COMPARE_LOG_RESULT))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Depth subtest failed");
			return STOP;
		}
	}

	bool stencilRenderbufferAvailable =
		(m_testFormat.type == RENDERBUFFER_STENCIL) || (m_testFormat.type == RENDERBUFFER_DEPTH_STENCIL);
	if (defaultFramebufferStencilBits && stencilRenderbufferAvailable)
	{
		gl.disable(GL_DEPTH_TEST);
		gl.enable(GL_STENCIL_TEST);

		if (!createFramebuffer())
			return STOP;

		for (GLuint loop = 0; loop < 2; loop++)
		{
			gl.bindFramebuffer(GL_FRAMEBUFFER, loop ? m_fbo : 0);
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Draw a rect scissored to half the screen height, incrementing the stencil buffer.
			gl.enable(GL_SCISSOR_TEST);
			gl.scissor(0, 0, m_renderWidth, m_renderHeight / 2);
			gl.stencilFunc(GL_ALWAYS, 0x0, 0xFF);
			gl.stencilOp(GL_ZERO, GL_INCR, GL_INCR);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilOp");
			renderColoredQuad(program.getProgram(), bigQuadPositionsSet);
			gl.disable(GL_SCISSOR_TEST);

			// Only draw where stencil is equal to 1
			gl.stencilFunc(GL_EQUAL, 0x01, 0xFF);
			gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			gl.clear(GL_COLOR_BUFFER_BIT);
			renderColoredQuad(program.getProgram(), bigQuadPositionsSet);

			glu::readPixels(renderContext, 0, 0, testSurface[loop].getAccess());
		}

		gl.disable(GL_STENCIL_TEST);
		deleteFramebuffer();

		// Compare surfaces
		if (!tcu::fuzzyCompare(m_testCtx.getLog(), "Result", "Image comparison result", testSurface[0], testSurface[1],
							   0.05f, tcu::COMPARE_LOG_RESULT))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Stencil subtest failed");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

void RenderbufferCase::constructOrthoProjMatrix(GLfloat* mat4, GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n,
												GLfloat f) const
{
	GLfloat inv_width  = 1.0f / (r - l);
	GLfloat inv_height = 1.0f / (t - b);
	GLfloat inv_depth  = 1.0f / (f - n);

	memset(mat4, 0, sizeof(GLfloat) * 16);
	/*
        0    4    8    12
        1    5    9    13
        2    6    10    14
        3    7    11    15
    */

	mat4[0]  = 2.0f * inv_width;
	mat4[5]  = 2.0f * inv_height;
	mat4[10] = 2.0f * inv_depth;

	mat4[12] = -(r + l) * inv_width;
	mat4[13] = -(t + b) * inv_height;
	mat4[14] = -(f + n) * inv_depth;
	mat4[15] = 1.0f;
}

bool RenderbufferCase::createFramebuffer()
{
	glu::RenderContext& renderContext = m_context.getRenderContext();
	const Functions&	gl			  = renderContext.getFunctions();

	gl.genFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	if (m_testFormat.type == RENDERBUFFER_COLOR)
	{
		m_rbColor = createAndAttachRenderBuffer(m_testFormat.format, GL_COLOR_ATTACHMENT0);
		m_rbDepth = createAndAttachRenderBuffer(GL_DEPTH_COMPONENT16, GL_DEPTH_ATTACHMENT);
	}
	else
	{
		m_rbColor = createAndAttachRenderBuffer(GL_RGBA8, GL_COLOR_ATTACHMENT0);
		if (m_testFormat.type == RENDERBUFFER_DEPTH)
			m_rbDepth = createAndAttachRenderBuffer(m_testFormat.format, GL_DEPTH_ATTACHMENT);
		else if (m_testFormat.type == RENDERBUFFER_STENCIL)
			m_rbStencil = createAndAttachRenderBuffer(m_testFormat.format, GL_STENCIL_ATTACHMENT);
		else if (m_testFormat.type == RENDERBUFFER_DEPTH_STENCIL)
		{
			if (glu::contextSupports(renderContext.getType(), glu::ApiType::es(2, 0)))
			{
				m_rbDepth = createAndAttachRenderBuffer(m_testFormat.format, GL_DEPTH_ATTACHMENT);
				gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbDepth);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer");
			}
			else
				m_rbDepth = createAndAttachRenderBuffer(m_testFormat.format, GL_DEPTH_STENCIL_ATTACHMENT);
		}
	}

	GLenum bufferStatus = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (bufferStatus == GL_FRAMEBUFFER_UNSUPPORTED)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Unsuported framebuffer");
		return false;
	}
	else if (bufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Framebuffer not complete");
		return false;
	}

	return true;
}

void RenderbufferCase::deleteFramebuffer()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	if (m_fbo)
		gl.deleteFramebuffers(1, &m_fbo);
	if (m_rbColor)
		gl.deleteRenderbuffers(1, &m_rbColor);
	if (m_rbDepth)
		gl.deleteRenderbuffers(1, &m_rbDepth);
	if (m_rbStencil)
		gl.deleteRenderbuffers(1, &m_rbStencil);
}

GLuint RenderbufferCase::createAndAttachRenderBuffer(GLenum rbFormat, GLenum fbAttachment)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint rbName;

	gl.genRenderbuffers(1, &rbName);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbName);
	gl.renderbufferStorage(GL_RENDERBUFFER, rbFormat, m_renderWidth, m_renderHeight);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage");
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, fbAttachment, GL_RENDERBUFFER, rbName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer");

	return rbName;
}

void RenderbufferCase::renderColoredQuad(GLuint programId, const float* positions) const
{
	// Prepare data for rendering
	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
	static const float	colors[]		= {
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};
	const glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 3, 4, 0, positions),
													 glu::va::Float("color", 4, 4, 0, colors) };

	glu::draw(m_context.getRenderContext(), programId, DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));
}

glu::ProgramSources RenderbufferCase::prepareColoringProgramSources() const
{
	glu::RenderContext& renderContext	  = m_context.getRenderContext();
	glu::ContextType	contextType		   = renderContext.getType();
	glu::GLSLVersion	glslVersion		   = glu::getContextTypeGLSLVersion(contextType);
	std::string			versionDeclaration = glu::getGLSLVersionDeclaration(glslVersion);

	versionDeclaration += "\n";
	std::string vs = versionDeclaration;
	std::string fs = versionDeclaration;
	if (glu::contextSupports(contextType, glu::ApiType::es(3, 0)) || glu::isContextTypeGLCore(contextType))
	{
		vs += "in highp vec3 position;\n"
			  "in highp vec4 color;\n"
			  "out highp vec4 fColor;\n"
			  "uniform mat4 mvpMatrix;"
			  "void main()\n"
			  "{\n"
			  "  fColor = color;\n"
			  "  gl_Position = mvpMatrix * vec4(position, 1.0);\n"
			  "}\n";
		fs += "in highp vec4 fColor;\n"
			  "out highp vec4 color;\n"
			  "void main()\n"
			  "{\n"
			  "  color = fColor;\n"
			  "}\n";
	}
	else
	{
		vs += "attribute highp vec3 position;\n"
			  "attribute highp vec4 color;\n"
			  "varying highp vec4 fColor;\n"
			  "uniform mat4 mvpMatrix;"
			  "void main()\n"
			  "{\n"
			  "  fColor = color;\n"
			  "  gl_Position = mvpMatrix * vec4(position, 1.0);\n"
			  "}\n";
		fs += "varying highp vec4 fColor;\n"
			  "void main()\n"
			  "{\n"
			  "  gl_FragColor = fColor;\n"
			  "}\n";
	}

	return glu::makeVtxFragSources(vs.c_str(), fs.c_str());
}

typedef TextureFormat	  TF;
typedef CopyTexImageFormat CF;
typedef RenderbufferFormat RF;

struct TestData
{
	std::vector<TextureFormat>		texture2DFormats;
	std::vector<CopyTexImageFormat> copyTexImageFormats;
	std::vector<RenderbufferFormat> renderbufferFormats;
};

/** Constructor.
 *
 *  @param context Rendering context.
 */
InternalformatTests::InternalformatTests(deqp::Context& context)
	: TestCaseGroup(context, "internalformat", "Texture internalformat tests")
{
}

template <typename Data, unsigned int Size>
void InternalformatTests::append(std::vector<Data>& dataVector, const Data (&dataArray)[Size])
{
	dataVector.insert(dataVector.end(), dataArray, dataArray + Size);
}

void InternalformatTests::getESTestData(TestData& testData, glu::ContextType& contextType)
{
	TextureFormat commonTexture2DFormats[] = {
		TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA),
		TF(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB),
		TF(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA),
		TF(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE_ALPHA),
		TF(GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE),
		TF(GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA),
		TF(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8, OES_rgb8_rgba8),
		TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8, OES_rgb8_rgba8),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGBA, EXT_texture_type_2_10_10_10_REV),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB10_A2, EXT_texture_type_2_10_10_10_REV),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB5_A1, EXT_texture_type_2_10_10_10_REV),
		TF(GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB, EXT_texture_type_2_10_10_10_REV),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, GL_DEPTH_COMPONENT, OES_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT, OES_depth_texture),
		TF(GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8, OES_packed_depth_stencil),
		TF(GL_RGB, GL_HALF_FLOAT, GL_RGB16F, OES_texture_half_float),
		TF(GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F, OES_texture_half_float),
		TF(GL_RGB, GL_HALF_FLOAT, GL_RGB16F, OES_texture_half_float_linear, DE_NULL, GL_LINEAR, GL_LINEAR),
		TF(GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F, OES_texture_half_float_linear, DE_NULL, GL_LINEAR, GL_LINEAR),
		TF(GL_RGB, GL_FLOAT, GL_RGB32F, OES_texture_float),
		TF(GL_RGBA, GL_FLOAT, GL_RGBA32F, OES_texture_float),
		TF(GL_RGB, GL_FLOAT, GL_RGB32F, OES_texture_float_linear, DE_NULL, GL_LINEAR, GL_LINEAR),
		TF(GL_RGBA, GL_FLOAT, GL_RGBA32F, OES_texture_float_linear, DE_NULL, GL_LINEAR, GL_LINEAR),
	};

	CopyTexImageFormat commonCopyTexImageFormats[] = {
		CF(GL_RGB),
		CF(GL_RGBA),
		CF(GL_ALPHA),
		CF(GL_LUMINANCE),
		CF(GL_LUMINANCE_ALPHA),
		CF(GL_RGBA8, OES_rgb8_rgba8),
		CF(GL_RGB8, OES_rgb8_rgba8),
	};

	RenderbufferFormat commonRenderbufferFormats[] = {
		RF(GL_RGBA8, RENDERBUFFER_COLOR, OES_rgb8_rgba8),
		RF(GL_RGB8, RENDERBUFFER_COLOR, OES_rgb8_rgba8),
	};

	append(testData.texture2DFormats, commonTexture2DFormats);
	append(testData.copyTexImageFormats, commonCopyTexImageFormats);
	append(testData.renderbufferFormats, commonRenderbufferFormats);

	if (glu::contextSupports(contextType, glu::ApiType::es(3, 0)))
	{
		TextureFormat es3Texture2DFormats[] = {
			TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA4),
			TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGB5_A1),
			TF(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB565),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1),
			TF(GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB),
			TF(GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB565),
		};

		CopyTexImageFormat es3CopyTexImageFormats[] = {
			CF(GL_RGBA4),
			CF(GL_RGB5_A1),
			CF(GL_RGB565)
		};

		RenderbufferFormat es3RenderbufferFormats[] = {
			RF(GL_RGB5_A1, RENDERBUFFER_COLOR),
		};

		append(testData.texture2DFormats, es3Texture2DFormats);
		append(testData.copyTexImageFormats, es3CopyTexImageFormats);
		append(testData.renderbufferFormats, es3RenderbufferFormats);
	}
	else if (glu::contextSupports(contextType, glu::ApiType::es(2, 0)))
	{
		TextureFormat es2Texture2DFormats[] = {
			TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGB5_A1, OES_required_internalformat),
			TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA4, OES_required_internalformat),
			TF(GL_RGB, GL_UNSIGNED_BYTE, GL_RGB565, OES_required_internalformat),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, OES_required_internalformat),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA, OES_required_internalformat),
			TF(GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, OES_required_internalformat),
			TF(GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB, OES_required_internalformat),
			TF(GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB565, OES_required_internalformat),
			TF(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE8_ALPHA8_OES, OES_required_internalformat),
			TF(GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, GL_LUMINANCE4_ALPHA4_OES, OES_required_internalformat),
			TF(GL_LUMINANCE, GL_UNSIGNED_BYTE, GL_LUMINANCE8_OES, OES_required_internalformat),
			TF(GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA8_OES, OES_required_internalformat),
			TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT16, OES_required_internalformat,
			   OES_depth_texture),
			TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, GL_DEPTH_COMPONENT16, OES_required_internalformat,
			   OES_depth_texture),
			TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT24, OES_required_internalformat, OES_depth24),
			TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT32, OES_required_internalformat, OES_depth32),
		};

		CopyTexImageFormat es2CopyTexImageFormats[] = {
			CF(GL_RGB5_A1, OES_required_internalformat),
			CF(GL_RGB565, OES_required_internalformat),
			CF(GL_RGBA4, OES_required_internalformat),
			CF(GL_LUMINANCE4_ALPHA4_OES, OES_required_internalformat),
			CF(GL_LUMINANCE8_ALPHA8_OES, OES_required_internalformat),
			CF(GL_LUMINANCE8_OES, OES_required_internalformat),
			CF(GL_ALPHA8_OES, OES_required_internalformat),
			CF(GL_RGB10_A2, EXT_texture_type_2_10_10_10_REV, OES_required_internalformat),
			CF(GL_RGB10, EXT_texture_type_2_10_10_10_REV, OES_required_internalformat)
		};

		RenderbufferFormat es2RenderbufferFormats[] = {
			RF(GL_STENCIL_INDEX1, RENDERBUFFER_STENCIL, OES_stencil1),
			RF(GL_STENCIL_INDEX4, RENDERBUFFER_STENCIL, OES_stencil4),
			RF(GL_STENCIL_INDEX8, RENDERBUFFER_STENCIL, OES_stencil8),
			RF(GL_DEPTH_COMPONENT16, RENDERBUFFER_DEPTH, OES_depth_texture),
			RF(GL_DEPTH_COMPONENT24, RENDERBUFFER_DEPTH, OES_depth24),
			RF(GL_DEPTH_COMPONENT32, RENDERBUFFER_DEPTH, OES_depth32),
			RF(GL_DEPTH24_STENCIL8, RENDERBUFFER_DEPTH_STENCIL, OES_packed_depth_stencil),
			RF(GL_RGB5_A1, RENDERBUFFER_COLOR, OES_required_internalformat),
		};

		append(testData.texture2DFormats, es2Texture2DFormats);
		append(testData.copyTexImageFormats, es2CopyTexImageFormats);
		append(testData.renderbufferFormats, es2RenderbufferFormats);
	}
}

void InternalformatTests::getGLTestData(TestData& testData, glu::ContextType&)
{
	TextureFormat commonTexture2DFormats[] = {
		TF(GL_RED, GL_BYTE, GL_R8_SNORM),
		TF(GL_RED, GL_SHORT, GL_R16_SNORM),
		TF(GL_RG, GL_BYTE, GL_RG8_SNORM),
		TF(GL_RG, GL_SHORT, GL_RG16_SNORM),
		TF(GL_RGB, GL_BYTE, GL_RGB8_SNORM),
		TF(GL_RGB, GL_SHORT, GL_RGB16_SNORM),
		TF(GL_RGBA, GL_BYTE, GL_RGBA8_SNORM),
		TF(GL_RGBA, GL_SHORT, GL_RGBA16_SNORM),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGBA),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB10_A2),
		TF(GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB5_A1),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, GL_DEPTH_COMPONENT, ARB_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, GL_DEPTH_COMPONENT16, ARB_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT, ARB_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT24, ARB_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT32, ARB_depth_texture),
		TF(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_DEPTH_COMPONENT16, ARB_depth_texture),
		TF(GL_RGBA, GL_UNSIGNED_BYTE, GL_RGB9_E5, EXT_texture_shared_exponent),
		TF(GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV, GL_RGB10_A2UI, ARB_texture_rgb10_a2ui),
		TF(GL_RGBA_INTEGER, GL_UNSIGNED_INT, GL_RGBA32UI, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_UNSIGNED_INT, GL_RGB32UI, EXT_texture_integer),
		TF(GL_RGBA_INTEGER, GL_UNSIGNED_SHORT, GL_RGBA16UI, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_UNSIGNED_SHORT, GL_RGB16UI, EXT_texture_integer),
		TF(GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, GL_RGBA8UI, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_UNSIGNED_BYTE, GL_RGB8UI, EXT_texture_integer),
		TF(GL_RGBA_INTEGER, GL_INT, GL_RGBA32I, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_INT, GL_RGB32I, EXT_texture_integer),
		TF(GL_RGBA_INTEGER, GL_SHORT, GL_RGBA16I, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_SHORT, GL_RGB16I, EXT_texture_integer),
		TF(GL_RGBA_INTEGER, GL_BYTE, GL_RGBA8I, EXT_texture_integer),
		TF(GL_RGB_INTEGER, GL_BYTE, GL_RGB8I, EXT_texture_integer),
		TF(GL_RED, GL_HALF_FLOAT, GL_R16F, ARB_texture_float),
		TF(GL_RG, GL_HALF_FLOAT, GL_RG16F, ARB_texture_float),
		TF(GL_RGB, GL_HALF_FLOAT, GL_RGB16F, ARB_texture_float),
		TF(GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F, ARB_texture_float),
		TF(GL_RED, GL_FLOAT, GL_R32F, ARB_texture_float),
		TF(GL_RG, GL_FLOAT, GL_RG32F, ARB_texture_float),
		TF(GL_RGB, GL_FLOAT, GL_RGB32F, ARB_texture_float),
		TF(GL_RGBA, GL_FLOAT, GL_RGBA32F, ARB_texture_float),
	};

	CopyTexImageFormat commonCopyTexImageFormats[] = {
		CF(GL_DEPTH_COMPONENT16, ARB_depth_texture),
		CF(GL_DEPTH_COMPONENT24, ARB_depth_texture),
		CF(GL_DEPTH_COMPONENT32, ARB_depth_texture),
		CF(GL_RGB9_E5, EXT_texture_shared_exponent),
		CF(GL_RGB10_A2UI, ARB_texture_rgb10_a2ui),
		CF(GL_RGB10_A2),
	};

	RenderbufferFormat commonRenderbufferFormats[] = {
		RF(GL_RGBA8, RENDERBUFFER_COLOR),
		RF(GL_RGB9_E5, RENDERBUFFER_COLOR, EXT_texture_shared_exponent),
		RF(GL_RGB10_A2UI, RENDERBUFFER_COLOR, ARB_texture_rgb10_a2ui),
		RF(GL_DEPTH24_STENCIL8, RENDERBUFFER_DEPTH_STENCIL),
		RF(GL_DEPTH_COMPONENT16, RENDERBUFFER_DEPTH, ARB_depth_texture),
		RF(GL_DEPTH_COMPONENT24, RENDERBUFFER_DEPTH, ARB_depth_texture),
		RF(GL_DEPTH_COMPONENT32, RENDERBUFFER_DEPTH, ARB_depth_texture),
	};

	append(testData.texture2DFormats, commonTexture2DFormats);
	append(testData.copyTexImageFormats, commonCopyTexImageFormats);
	append(testData.renderbufferFormats, commonRenderbufferFormats);
}

std::string formatToString(GLenum format)
{
	// this function extends glu::getTextureFormatStr by formats used in thise tests

	typedef std::map<GLenum, std::string> FormatMap;
	static FormatMap formatMap;
	if (formatMap.empty())
	{
		// store in map formats that are not supported by glu::getTextureFormatStr
		formatMap[GL_LUMINANCE8_ALPHA8_OES] = "luminance8_alpha8_oes";
		formatMap[GL_LUMINANCE4_ALPHA4_OES] = "luminance4_alpha4_oes";
		formatMap[GL_STENCIL_INDEX1_OES]	= "stencil_index1_oes";
		formatMap[GL_STENCIL_INDEX4_OES]	= "stencil_index4_oes";
		formatMap[GL_LUMINANCE8_OES]		= "luminance8_oes";
		formatMap[GL_ALPHA8_OES]			= "alpha8_oes";
	}

	FormatMap::iterator it = formatMap.find(format);
	if (it == formatMap.end())
	{
		// if format is not in map try glu function
		std::string formatString = glu::getTextureFormatStr(format).toString();

		// cut out "GL_" from string
		formatString = formatString.substr(3, formatString.length());

		// make lower case
		std::transform(formatString.begin(), formatString.end(), formatString.begin(), tolower);

		return formatString;
	}
	return it->second;
}

/** Initializes the test group contents. */
void InternalformatTests::init()
{
	// Determine which data sets should be used for tests
	TestData		 testData;
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (glu::isContextTypeGLCore(contextType))
		getGLTestData(testData, contextType);
	else
		getESTestData(testData, contextType);

	// Construct texture2d tests
	TestCaseGroup* texture2DGroup = new deqp::TestCaseGroup(m_context, "texture2d", "");
	for (unsigned int i = 0; i < testData.texture2DFormats.size(); i++)
	{
		const TextureFormat& tf				= testData.texture2DFormats[i];
		std::string			 format			= formatToString(tf.format);
		std::string			 type			= glu::getTypeStr(tf.type).toString();
		std::string			 internalFormat = formatToString(tf.internalFormat);

		// cut out "GL_" from type and make it lowercase
		type = type.substr(3, type.length());
		std::transform(type.begin(), type.end(), type.begin(), tolower);

		std::string name = format + "_" + type + "_" + internalFormat;
		if (tf.minFilter == GL_LINEAR)
			name += "_linear";

		texture2DGroup->addChild(new Texture2DCase(m_context, name, tf));
	}
	addChild(texture2DGroup);

	// Construct copy_text_image tests
	TestCaseGroup* copyTexImageGroup = new deqp::TestCaseGroup(m_context, "copy_tex_image", "");
	for (unsigned int i = 0; i < testData.copyTexImageFormats.size(); i++)
	{
		const CopyTexImageFormat& ctif = testData.copyTexImageFormats[i];
		std::string				  name = formatToString(ctif.internalFormat);
		copyTexImageGroup->addChild(new CopyTexImageCase(m_context, name, ctif));
	}
	addChild(copyTexImageGroup);

	// Construct renderbuffer tests
	TestCaseGroup* renderbufferGroup = new deqp::TestCaseGroup(m_context, "renderbuffer", "");
	for (unsigned int i = 0; i < testData.renderbufferFormats.size(); i++)
	{
		const RenderbufferFormat& rbf  = testData.renderbufferFormats[i];
		std::string				  name = formatToString(rbf.format);
		renderbufferGroup->addChild(new RenderbufferCase(m_context, name, rbf));
	}
	addChild(renderbufferGroup);
}
} /* glcts namespace */
