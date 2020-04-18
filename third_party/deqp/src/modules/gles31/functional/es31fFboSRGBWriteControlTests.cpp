/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2017 The Android Open Source Project
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
 *//*!
 * \file
 * \brief FBO sRGB tests.
*//*--------------------------------------------------------------------*/

#include "es31fFboSRGBWriteControlTests.hpp"
#include "es31fFboTestUtil.hpp"
#include "gluTextureUtil.hpp"
#include "gluContextInfo.hpp"
#include "tcuTestLog.hpp"
#include "glwEnums.hpp"
#include "sglrContextUtil.hpp"
#include "glwFunctions.hpp"
#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "glsTextureTestUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "gluStrUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

tcu::Vec4 getTestColorLinear (void)
{
	return tcu::Vec4(0.2f, 0.3f, 0.4f, 1.0f);
}

tcu::Vec4 getTestColorSRGB (void)
{
	return linearToSRGB(tcu::Vec4(0.2f, 0.3f, 0.4f, 1.0f));
}

tcu::Vec4 getTestColorBlank (void)
{
	return tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

tcu::Vec4 getEpsilonError (void)
{
	return tcu::Vec4(0.005f);
}

enum QueryType
{
	QUERYTYPE_ISENABLED = 0,
	QUERYTYPE_BOOLEAN,
	QUERYTYPE_FLOAT,
	QUERYTYPE_INT,
	QUERYTYPE_INT64,
	QUERYTYPE_LAST
};

enum DataType
{
	DATATYPE_BOOLEAN = 0,
	DATATYPE_FLOAT,
	DATATYPE_INT,
	DATATYPE_INT64,
};

enum FramebufferSRGB
{
	FRAMEBUFFERSRGB_ENABLED = 0,
	FRAMEBUFFERSRGB_DISABLED
};

enum FramebufferBlend
{
	FRAMEBUFFERBLEND_ENABLED = 0,
	FRAMEBUFFERBLEND_DISABLED
};

enum TextureSourcesType
{
	TEXTURESOURCESTYPE_RGBA		= 0,
	TEXTURESOURCESTYPE_SRGBA,
	TEXTURESOURCESTYPE_BOTH,
	TEXTURESOURCESTYPE_NONE
};

enum FboType
{
	FBOTYPE_SOURCE			= 0,
	FBOTYPE_DESTINATION
};

enum RendererTask
{
	RENDERERTASK_DRAW = 0,
	RENDERERTASK_COPY
};

enum SamplingType
{
	SAMPLINGTYPE_TEXTURE			= 0,
	SAMPLINGTYPE_TEXTURE_LOD,
	SAMPLINGTYPE_TEXTURE_GRAD,
	SAMPLINGTYPE_TEXTURE_OFFSET,
	SAMPLINGTYPE_TEXTURE_PROJ,
};

namespace TestTextureSizes
{
	const int WIDTH = 128;
	const int HEIGHT = 128;
} // global test texture sizes

namespace SampligTypeCount
{
	const int MAX = 5;
} // global max number of sampling types

std::string buildSamplingPassType (const int samplerTotal)
{
	std::ostringstream	shaderFragment;

	const SamplingType	samplingTypeList [] =
	{
		SAMPLINGTYPE_TEXTURE, SAMPLINGTYPE_TEXTURE_LOD, SAMPLINGTYPE_TEXTURE_GRAD, SAMPLINGTYPE_TEXTURE_OFFSET, SAMPLINGTYPE_TEXTURE_PROJ
	} ;

	for (int samplerTypeIdx = 0; samplerTypeIdx < DE_LENGTH_OF_ARRAY(samplingTypeList); samplerTypeIdx++)
	{
		shaderFragment
			<< "	if (uFunctionType == " << samplerTypeIdx << ") \n"
			<< "	{ \n";

		for (int samplerIdx = 0; samplerIdx < samplerTotal; samplerIdx++)
		{
			switch (static_cast<SamplingType>(samplerTypeIdx))
			{
				case SAMPLINGTYPE_TEXTURE:
				{
					shaderFragment
						<< "		texelColor" << samplerIdx << " = texture(uTexture" << samplerIdx << ", vs_aTexCoord); \n";
					break;
				}
				case SAMPLINGTYPE_TEXTURE_LOD:
				{
					shaderFragment
						<< "		texelColor" << samplerIdx << " = textureLod(uTexture" << samplerIdx << ", vs_aTexCoord, 0.0f); \n";
					break;
				}
				case SAMPLINGTYPE_TEXTURE_GRAD:
				{
					shaderFragment
						<< "		texelColor" << samplerIdx << " = textureGrad(uTexture" << samplerIdx << ", vs_aTexCoord, vec2(0.0f, 0.0f), vec2(0.0f, 0.0f)); \n";
					break;
				}
				case SAMPLINGTYPE_TEXTURE_OFFSET:
				{
					shaderFragment
						<< "		texelColor" << samplerIdx << " = textureOffset(uTexture" << samplerIdx << ", vs_aTexCoord, ivec2(0.0f, 0.0f)); \n";
					break;
				}
				case SAMPLINGTYPE_TEXTURE_PROJ:
				{
					shaderFragment
						<< "		texelColor" << samplerIdx << " = textureProj(uTexture" << samplerIdx << ", vec3(vs_aTexCoord, 1.0f)); \n";
					break;
				}
				default:
					DE_FATAL("Error: sampling type unrecognised");
			}
		}

		shaderFragment
			<< "	} \n";
	}

	return shaderFragment.str();
}

void logColor (Context& context, const std::string& colorLogMessage, const tcu::Vec4 resultColor)
{
	tcu::TestLog&			log		= context.getTestContext().getLog();
	std::ostringstream		message;

	message << colorLogMessage << " = (" << resultColor.x() << ", " << resultColor.y() << ", " << resultColor.z() << ", " << resultColor.w() << ")";
		log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;
}

struct TestFunction
{
	explicit TestFunction	(const bool hasFunctionValue)
		: hasFunction		(hasFunctionValue) {}
	TestFunction			(const char* const functionNameValue, const char* const functionDefinition)
		: hasFunction		(true)
		, functionName		(functionNameValue)
		, functionDefintion	(functionDefinition) {}
	~TestFunction			(void) {}

	bool			hasFunction;
	const char*		functionName;
	const char*		functionDefintion;
};

TestFunction getFunctionBlendLinearToSRGBCheck (void)
{
	const char* const functionName = "blendPlusLinearToSRGB";

	const char* const functionDefinition =
		"mediump vec4 blendPlusLinearToSRGB(in mediump vec4 colorSrc, in mediump vec4 colorDst) \n"
		"{ \n"
		"	const int MAX_VECTOR_SIZE = 4; \n"
		"	mediump vec4 colorConverted; \n"
		"	mediump vec4 colorBlended; \n"
		"	for (int idx = 0; idx < MAX_VECTOR_SIZE; idx++) \n"
		"	{ \n"
		"		if (uBlendFunctionType == 0) \n"
		"		{ \n"
		"			colorBlended[idx] = (colorSrc[idx] * uFactorSrc) + colorDst[idx] * uFactorDst; \n"
		"		} \n"
		"		if (uBlendFunctionType == 1) \n"
		"		{ \n"
		"			colorBlended[idx] = (colorSrc[idx] * uFactorSrc) - (colorDst[idx] * uFactorDst); \n"
		"		} \n"
				"if (uBlendFunctionType == 2) \n"
		"		{ \n"
		"			colorBlended[idx] = (colorDst[idx] * uFactorDst) - (colorSrc[idx] * uFactorSrc); \n"
		"		} \n"
		"		if (colorBlended[idx] < 0.0031308f) \n"
		"		{ \n"
		"			colorConverted[idx] = 12.92f * colorBlended[idx]; \n"
		"		} \n"
		"		else \n"
		"		{ \n"
		"			colorConverted[idx] = 1.055f * pow(colorBlended[idx], 0.41666f) - 0.055f; \n"
		"		} \n"
		"	} \n"
		"	return colorConverted; \n"
		"} \n";

	TestFunction testFunction(functionName, functionDefinition);

	return testFunction;
}

struct FBOConfig
{
	FBOConfig					(const deUint32 textureInternalFormatValue,
								 const tcu::Vec4 textureColorValue,
								 const deUint32 fboTargetTypeValue,
								 const deUint32 fboColorAttachmentValue,
								 const FboType fboTypeValue)
		: textureInternalFormat	(textureInternalFormatValue)
		, textureColor			(textureColorValue)
		, fboTargetType			(fboTargetTypeValue)
		, fboColorAttachment	(fboColorAttachmentValue)
		, fboType				(fboTypeValue) {}
	~FBOConfig					(void) {}

	deUint32	textureInternalFormat;
	tcu::Vec4	textureColor;
	deUint32	fboTargetType;
	deUint32	fboColorAttachment;
	FboType		fboType;
};

struct BlendConfig
{
	deUint32	equation;
	deUint32	funcSrc;
	deUint32	funcDst;
};

std::vector<BlendConfig> getBlendingConfigList (void)
{
	BlendConfig blendConfigs[12];

	// add function permutations
	blendConfigs[0].equation = GL_FUNC_ADD;
	blendConfigs[1].equation = GL_FUNC_ADD;
	blendConfigs[2].equation = GL_FUNC_ADD;
	blendConfigs[3].equation = GL_FUNC_ADD;

	blendConfigs[0].funcSrc = GL_ONE;
	blendConfigs[0].funcDst = GL_ONE;
	blendConfigs[1].funcSrc = GL_ONE;
	blendConfigs[1].funcDst = GL_ZERO;
	blendConfigs[2].funcSrc = GL_ZERO;
	blendConfigs[2].funcDst = GL_ONE;
	blendConfigs[3].funcSrc = GL_ZERO;
	blendConfigs[3].funcDst = GL_ZERO;

	// subtract function permutations
	blendConfigs[4].equation = GL_FUNC_SUBTRACT;
	blendConfigs[5].equation = GL_FUNC_SUBTRACT;
	blendConfigs[6].equation = GL_FUNC_SUBTRACT;
	blendConfigs[7].equation = GL_FUNC_SUBTRACT;

	blendConfigs[4].funcSrc = GL_ONE;
	blendConfigs[4].funcDst = GL_ONE;
	blendConfigs[5].funcSrc = GL_ONE;
	blendConfigs[5].funcDst = GL_ZERO;
	blendConfigs[6].funcSrc = GL_ZERO;
	blendConfigs[6].funcDst = GL_ONE;
	blendConfigs[7].funcSrc = GL_ZERO;
	blendConfigs[7].funcDst = GL_ZERO;

	// reverse subtract function permutations
	blendConfigs[8].equation = GL_FUNC_REVERSE_SUBTRACT;
	blendConfigs[9].equation = GL_FUNC_REVERSE_SUBTRACT;
	blendConfigs[10].equation = GL_FUNC_REVERSE_SUBTRACT;
	blendConfigs[11].equation = GL_FUNC_REVERSE_SUBTRACT;

	blendConfigs[8].funcSrc = GL_ONE;
	blendConfigs[8].funcDst = GL_ONE;
	blendConfigs[9].funcSrc = GL_ONE;
	blendConfigs[9].funcDst = GL_ZERO;
	blendConfigs[10].funcSrc = GL_ZERO;
	blendConfigs[10].funcDst = GL_ONE;
	blendConfigs[11].funcSrc = GL_ZERO;
	blendConfigs[11].funcDst = GL_ZERO;

	std::vector<BlendConfig> configList(blendConfigs, blendConfigs + DE_LENGTH_OF_ARRAY(blendConfigs));

	return configList;
}

struct TestRenderPassConfig
{
	TestRenderPassConfig		(void)
		: testFunction			(false) {}

	TestRenderPassConfig		(const TextureSourcesType textureSourcesTypeValue,
								FBOConfig fboConfigListValue,
								const FramebufferSRGB framebufferSRGBValue,
								const FramebufferBlend framebufferBendValue,
								const RendererTask rendererTaskValue)
		: textureSourcesType	(textureSourcesTypeValue)
		, framebufferSRGB		(framebufferSRGBValue)
		, frameBufferBlend		(framebufferBendValue)
		, testFunction			(false)
		, rendererTask			(rendererTaskValue) {fboConfigList.push_back(fboConfigListValue);}

	TestRenderPassConfig		(const TextureSourcesType textureSourcesTypeValue,
								FBOConfig fboConfigListValue,
								const FramebufferSRGB framebufferSRGBValue,
								const FramebufferBlend framebufferBendValue,
								TestFunction testFunctionValue,
								const RendererTask rendererTaskValue)
		: textureSourcesType	(textureSourcesTypeValue)
		, framebufferSRGB		(framebufferSRGBValue)
		, frameBufferBlend		(framebufferBendValue)
		, testFunction			(testFunctionValue)
		, rendererTask			(rendererTaskValue) {fboConfigList.push_back(fboConfigListValue);}

	TestRenderPassConfig		(const TextureSourcesType textureSourcesTypeValue,
								std::vector<FBOConfig> fboConfigListValue,
								const FramebufferSRGB framebufferSRGBValue,
								const FramebufferBlend framebufferBendValue,
								TestFunction testFunctionValue,
								const RendererTask rendererTaskValue)
		: textureSourcesType	(textureSourcesTypeValue)
		, fboConfigList			(fboConfigListValue)
		, framebufferSRGB		(framebufferSRGBValue)
		, frameBufferBlend		(framebufferBendValue)
		, testFunction			(testFunctionValue)
		, rendererTask			(rendererTaskValue) {}

	~TestRenderPassConfig		(void) {}

	TextureSourcesType		textureSourcesType;
	std::vector<FBOConfig>	fboConfigList;
	FramebufferSRGB			framebufferSRGB;
	FramebufferBlend		frameBufferBlend;
	TestFunction			testFunction;
	RendererTask			rendererTask;
};

class TestVertexData
{
public:
							TestVertexData		(Context& context);
							~TestVertexData		(void);

	void					init				(void);

	void					bind				(void) const;
	void					unbind				(void) const;

private:
	const glw::Functions*	m_gl;
	std::vector<float>		m_data;
	glw::GLuint				m_vboHandle;
	glw::GLuint				m_vaoHandle;
};

TestVertexData::TestVertexData	(Context& context)
	: m_gl						(&context.getRenderContext().getFunctions())
{
	const glw::GLfloat		vertexData[]	=
	{
		// position				// texcoord
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f, // bottom left corner
		 1.0f, -1.0f, 0.0f,		1.0f, 0.0f, // bottom right corner
		 1.0f,  1.0f, 0.0f,		1.0f, 1.0f, // Top right corner

		-1.0f,  1.0f, 0.0f,		0.0f, 1.0f, // top left corner
		 1.0f,  1.0f, 0.0f,		1.0f, 1.0f, // Top right corner
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f  // bottom left corner
	};

	m_data.resize(DE_LENGTH_OF_ARRAY(vertexData));
	for (int idx = 0; idx < (int)m_data.size(); idx++)
		m_data[idx] = vertexData[idx];

	m_gl->genVertexArrays(1, &m_vaoHandle);
	m_gl->bindVertexArray(m_vaoHandle);

	m_gl->genBuffers(1, &m_vboHandle);
	m_gl->bindBuffer(GL_ARRAY_BUFFER, m_vboHandle);

	m_gl->bufferData(GL_ARRAY_BUFFER, (glw::GLsizei)(m_data.size() * sizeof(glw::GLfloat)), &m_data[0], GL_STATIC_DRAW);

	m_gl->enableVertexAttribArray(0);
	m_gl->vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * (glw::GLsizei)sizeof(GL_FLOAT), (glw::GLvoid *)0);
	m_gl->enableVertexAttribArray(1);
	m_gl->vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * (glw::GLsizei)sizeof(GL_FLOAT), (glw::GLvoid *)(3 * sizeof(GL_FLOAT)));

	m_gl->bindVertexArray(0);
	m_gl->bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "gl error during vertex data setup");
}

TestVertexData::~TestVertexData (void)
{
	m_gl->deleteBuffers(1, &m_vboHandle);
	m_gl->deleteVertexArrays(1, &m_vaoHandle);
}

void TestVertexData::bind (void) const
{
	m_gl->bindVertexArray(m_vaoHandle);
}

void TestVertexData::unbind (void) const
{
	m_gl->bindVertexArray(0);
}

class TestTexture2D
{
public:
								TestTexture2D		(Context& context, const deUint32 internalFormatValue, const deUint32 transferFormatValue, const deUint32 transferTypeValue, const tcu::Vec4 imageColorValue);
								~TestTexture2D		(void);

	int							getTextureUnit		(void) const;
	deUint32					getHandle			(void) const;

	void						bind				(const int textureUnit);
	void						unbind				(void) const;

private:
	const glw::Functions*		m_gl;
	glw::GLuint					m_handle;
	const deUint32				m_internalFormat;
	tcu::TextureFormat			m_transferFormat;
	int							m_width;
	int							m_height;
	tcu::TextureLevel			m_imageData;
	int							m_textureUnit;
};

TestTexture2D::TestTexture2D	(Context& context, const deUint32 internalFormat, const deUint32 transferFormat, const deUint32 transferType, const tcu::Vec4 imageColor)
	: m_gl						(&context.getRenderContext().getFunctions())
	, m_internalFormat			(internalFormat)
	, m_transferFormat			(tcu::TextureFormat(glu::mapGLTransferFormat(transferFormat, transferType)))
	, m_width					(TestTextureSizes::WIDTH)
	, m_height					(TestTextureSizes::HEIGHT)
	, m_imageData				(tcu::TextureLevel(glu::mapGLInternalFormat(internalFormat), m_width, m_height, 1))
{
	// fill image data with a solid test color
	tcu::clear(m_imageData.getAccess(), tcu::Vec4(0.0f));
	for (int py = 0; py < m_imageData.getHeight(); py++)
	{
		for (int px = 0; px < m_imageData.getWidth(); px++)
			m_imageData.getAccess().setPixel(imageColor, px, py);
	}

	m_gl->genTextures(1, &m_handle);

	m_gl->bindTexture(GL_TEXTURE_2D, m_handle);
	m_gl->texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,		GL_MIRRORED_REPEAT);
	m_gl->texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,		GL_MIRRORED_REPEAT);
	m_gl->texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER,	GL_NEAREST);
	m_gl->texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER,	GL_NEAREST);

	m_gl->texImage2D(GL_TEXTURE_2D, 0, m_internalFormat, m_width, m_height, 0, transferFormat, transferType, m_imageData.getAccess().getDataPtr());

	m_gl->bindTexture(GL_TEXTURE_2D, 0);
}

TestTexture2D::~TestTexture2D (void)
{
	m_gl->deleteTextures(1, &m_handle);
}

int TestTexture2D::getTextureUnit (void) const
{
	return m_textureUnit;
}

deUint32 TestTexture2D::getHandle (void) const
{
	return m_handle;
}

void TestTexture2D::bind (const int textureUnit)
{
	m_textureUnit = textureUnit;
	m_gl->activeTexture(GL_TEXTURE0 + m_textureUnit);
	m_gl->bindTexture(GL_TEXTURE_2D, m_handle);
}

void TestTexture2D::unbind (void) const
{
	m_gl->bindTexture(GL_TEXTURE_2D, 0);
}

class TestFramebuffer
{
public:
												TestFramebuffer			(void);
												TestFramebuffer			(Context& context, const deUint32 targetType, const deUint32 colorAttachment, glw::GLuint textureAttachmentHandle, const bool isSRGB, const FboType fboType, const int idx);
												~TestFramebuffer		(void);

	void										setTargetType			(const deUint32 targetType);

	FboType										getType					(void) const;
	deUint32									getColorAttachment		(void) const;
	int											getIdx					(void) const;

	void										bind					(void);
	void										unbind					(void);

	typedef de::UniquePtr<glu::Framebuffer>		fboUniquePtr;

private:
	const glw::Functions*						m_gl;
	fboUniquePtr								m_referenceSource;
	deUint32									m_targetType;
	bool										m_bound;
	bool										m_isSRGB;
	FboType										m_type;
	const int									m_idx;
	deUint32									m_colorAttachment;
};

TestFramebuffer::TestFramebuffer	(Context& context, const deUint32 targetType, const deUint32 colorAttachment, glw::GLuint textureAttachmentHandle, const bool isSRGB, const FboType fboType, const int idx)
	: m_gl							(&context.getRenderContext().getFunctions())
	, m_referenceSource				(new glu::Framebuffer(context.getRenderContext()))
	, m_targetType					(targetType)
	, m_bound						(false)
	, m_isSRGB						(isSRGB)
	, m_type						(fboType)
	, m_idx							(idx)
	, m_colorAttachment				(colorAttachment)
{
	m_gl->bindFramebuffer(m_targetType, **m_referenceSource);

	m_gl->framebufferTexture2D(m_targetType, m_colorAttachment, GL_TEXTURE_2D, textureAttachmentHandle, 0);

	TCU_CHECK(m_gl->checkFramebufferStatus(m_targetType) == GL_FRAMEBUFFER_COMPLETE);

	if (targetType == GL_DRAW_BUFFER)
	{
		glw::GLuint textureAttachments[] = {m_colorAttachment};
		m_gl->drawBuffers(DE_LENGTH_OF_ARRAY(textureAttachments), textureAttachments);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "glDrawBuffer()");
	}

	if (targetType == GL_READ_BUFFER)
	{
		m_gl->readBuffer(m_colorAttachment);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "glReadBuffer()");
	}

	m_gl->bindFramebuffer(m_targetType, 0);
}

TestFramebuffer::~TestFramebuffer (void)
{
}

void TestFramebuffer::setTargetType (const deUint32 targetType)
{
	m_targetType = targetType;
}

FboType TestFramebuffer::getType (void) const
{
	return m_type;
}

deUint32 TestFramebuffer::getColorAttachment (void) const
{
	return m_colorAttachment;
}

int TestFramebuffer::getIdx (void) const
{
	return m_idx;
}

void TestFramebuffer::bind (void)
{
	if (!m_bound)
	{
		m_gl->bindFramebuffer(m_targetType, **m_referenceSource);
		m_bound = true;
	}
}

void TestFramebuffer::unbind (void)
{
	if (m_bound)
	{
		m_gl->bindFramebuffer(m_targetType, 0);
		m_bound = false;
	}
}

class TestShaderProgram
{
public:
										TestShaderProgram		(Context& context, const int samplerTotal, TestFunction testFunction);
										~TestShaderProgram		(void);

	glw::GLuint							getHandle				(void) const;

	void								use						(void) const;
	void								unuse					(void) const;

	glu::ShaderProgramInfo				getLogInfo				(void);

private:
	const glw::Functions*				m_gl;
	de::MovePtr<glu::ShaderProgram>		m_referenceSource;
	const int							m_samplerTotal;
	const int							m_shaderStagesTotal;
};

TestShaderProgram::TestShaderProgram	(Context& context, const int samplerTotal, TestFunction testFunction)
	: m_gl								(&context.getRenderContext().getFunctions())
	, m_samplerTotal					(samplerTotal)
	, m_shaderStagesTotal				(2)
{
	std::ostringstream		shaderFragment;

	const char* const shaderVertex =
		"#version 310 es \n"
		"layout (location = 0) in mediump vec3 aPosition; \n"
		"layout (location = 1) in mediump vec2 aTexCoord; \n"
		"out mediump vec2 vs_aTexCoord; \n"
		"void main () \n"
		"{ \n"
		"	gl_Position = vec4(aPosition, 1.0f); \n"
		"	vs_aTexCoord = aTexCoord; \n"
		"} \n";

	shaderFragment
		<< "#version 310 es \n"
		<< "in mediump vec2 vs_aTexCoord; \n"
		<< "layout (location = 0) out mediump vec4 fs_aColor0; \n";

	for (int samplerIdx = 0; samplerIdx < m_samplerTotal; samplerIdx++)
		shaderFragment
			<< "uniform sampler2D uTexture" << samplerIdx << "; \n";

	shaderFragment
		<< "uniform int uFunctionType; \n";

	if (testFunction.hasFunction)
		shaderFragment
		<< "uniform int uBlendFunctionType; \n"
		<< "uniform mediump float uFactorSrc; \n"
		<< "uniform mediump float uFactorDst; \n"
			<< testFunction.functionDefintion;

	shaderFragment
		<< "void main () \n"
		<< "{ \n";

	for (int samplerIdx = 0; samplerIdx < m_samplerTotal; samplerIdx++)
		shaderFragment
			<<"	mediump vec4 texelColor" << samplerIdx << " = vec4(0.0f, 0.0f, 0.0f, 1.0f); \n";

	shaderFragment
		<< buildSamplingPassType(m_samplerTotal);

	if (testFunction.hasFunction)
		shaderFragment
			<< "	fs_aColor0 = " << testFunction.functionName << "(texelColor0, texelColor1); \n";
	else
		shaderFragment
			<< "	fs_aColor0 = texelColor0; \n";

	shaderFragment
		<< "} \n";

	m_referenceSource = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(context.getRenderContext(), glu::makeVtxFragSources(shaderVertex, shaderFragment.str())));
	if (!m_referenceSource->isOk())
	{
		tcu::TestLog& log = context.getTestContext().getLog();
		log << this->getLogInfo();
		TCU_FAIL("Failed to compile shaders and link program");
	}
}

TestShaderProgram::~TestShaderProgram (void)
{
	m_referenceSource = de::MovePtr<glu::ShaderProgram>(DE_NULL);
	m_referenceSource.clear();
}

deUint32 TestShaderProgram::getHandle (void) const
{
	return m_referenceSource->getProgram();
}

void TestShaderProgram::use (void) const
{
	m_gl->useProgram(this->getHandle());
}

void TestShaderProgram::unuse (void) const
{
	m_gl->useProgram(0);
}

glu::ShaderProgramInfo TestShaderProgram::getLogInfo (void)
{
	glu::ShaderProgramInfo	buildInfo;

	// log shader program info. Only vertex and fragment shaders included
	buildInfo.program = m_referenceSource->getProgramInfo();
	for (int shaderIdx = 0; shaderIdx < m_shaderStagesTotal; shaderIdx++)
	{
		glu::ShaderInfo shaderInfo = m_referenceSource->getShaderInfo(static_cast<glu::ShaderType>(static_cast<int>(glu::SHADERTYPE_VERTEX) + static_cast<int>(shaderIdx)), 0);
		buildInfo.shaders.push_back(shaderInfo);
	}
	return buildInfo;
}

class Renderer
{
public:
											Renderer						(Context& context);
											~Renderer						(void);

	void									init							(const TestRenderPassConfig& renderPassConfig, const int renderpass);
	void									deinit							(void);

	void									setSamplingType					(const SamplingType samplerIdx);
	void									setBlendIteration				(const int blendIteration);
	void									setFramebufferBlend				(const bool blend);
	void									setFramebufferSRGB				(const bool sRGB);

	std::vector<tcu::Vec4>					getResultsPreDraw				(void) const;
	std::vector<tcu::Vec4>					getResultsPostDraw				(void) const;
	int										getBlendConfigCount				(void) const;
	glu::ShaderProgramInfo					getShaderProgramInfo			(void);

	void									copyFrameBufferTexture			(const int srcPx, const int srcPy, const int dstPx, const int dstPy);
	void									draw							(void);
	void									storeShaderProgramInfo			(void);
	void									logShaderProgramInfo			(void);

	typedef de::SharedPtr<TestTexture2D>	TextureSp;
	typedef de::SharedPtr<TestFramebuffer>	FboSp;

private:
	void									createFBOwithColorAttachment	(const std::vector<FBOConfig> fboConfigList);
	void									setShaderProgramSamplingType	(const int samplerIdx);
	void									setShaderBlendFunctionType		(void);
	void									setShaderBlendSrcDstValues		(void);
	void									bindActiveTexturesSamplers		(void);
	void									bindAllRequiredSourceTextures	(const TextureSourcesType texturesRequired);
	void									unbindAllSourceTextures			(void);
	void									bindFramebuffer					(const int framebufferIdx);
	void									unbindFramebuffer				(const int framebufferIdx);
	void									enableFramebufferSRGB			(void);
	void									enableFramebufferBlend			(void);
	bool									isFramebufferAttachmentSRGB		(const deUint32 targetType, const deUint32 attachment) const;
	void									readTexels						(const int px, const int py, const deUint32 attachment, tcu::Vec4& texelData);
	void									logState						(const deUint32 targetType, const deUint32 attachment, const SamplingType samplingType) const;

	// renderer specific constants initialized during constructor
	Context&								m_context;
	const TestVertexData					m_vertexData;
	const int								m_textureSourceTotal;

	// additional resources monitored by the renderer
	std::vector<BlendConfig>				m_blendConfigList;
	std::vector<TextureSp>					m_textureSourceList;
	TestRenderPassConfig					m_renderPassConfig;
	std::vector<TextureSp>					m_fboTextureList;
	TestShaderProgram*						m_shaderProgram;
	std::vector<FboSp>						m_framebufferList;
	std::vector<tcu::Vec4>					m_resultsListPreDraw;
	std::vector<tcu::Vec4>					m_resultsListPostDraw;

	// mutable state variables (internal access only)
	bool									m_hasShaderProgramInfo;
	int										m_renderPass;
	int										m_samplersRequired;
	bool									m_hasFunction;
	bool									m_blittingEnabled;
	glu::ShaderProgramInfo					m_shaderProgramInfo;

	// mutable state variables (external access via setters)
	SamplingType							m_samplingType;
	int										m_blendIteration;
	bool									m_framebufferBlendEnabled;
	bool									m_framebufferSRGBEnabled;
};

Renderer::Renderer				(Context& context)
	: m_context					(context)
	, m_vertexData				(context)
	, m_textureSourceTotal		(2)
	, m_blendConfigList			(getBlendingConfigList())
	, m_hasShaderProgramInfo	(false)
{
	TextureSp textureLinear(new TestTexture2D(m_context, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, getTestColorLinear()));
	m_textureSourceList.push_back(textureLinear);

	TextureSp textureSRGB(new TestTexture2D(m_context, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE, getTestColorLinear()));
	m_textureSourceList.push_back(textureSRGB);
}

Renderer::~Renderer (void)
{
	m_textureSourceList.clear();
	this->deinit();
}

void Renderer::init (const TestRenderPassConfig& renderPassConfig, const int renderpass)
{
	m_renderPassConfig = renderPassConfig;
	m_renderPass = renderpass;

	this->createFBOwithColorAttachment(m_renderPassConfig.fboConfigList);

	if (m_renderPassConfig.textureSourcesType != TEXTURESOURCESTYPE_NONE)
	{
		if (m_renderPassConfig.textureSourcesType == TEXTURESOURCESTYPE_RGBA || m_renderPassConfig.textureSourcesType == TEXTURESOURCESTYPE_SRGBA)
			m_samplersRequired = 1;
		else if (m_renderPassConfig.textureSourcesType ==TEXTURESOURCESTYPE_BOTH )
			m_samplersRequired = 2;
		else
			DE_FATAL("Error: Texture source required not recognised");

		m_shaderProgram = new TestShaderProgram(m_context, m_samplersRequired, m_renderPassConfig.testFunction);
		m_hasFunction = m_renderPassConfig.testFunction.hasFunction;
	}
	else
		m_shaderProgram = DE_NULL;
}

void Renderer::deinit (void)
{
	if (m_shaderProgram != DE_NULL)
	{
		delete m_shaderProgram;
		m_shaderProgram = DE_NULL;
	}

	m_fboTextureList.clear();
	m_framebufferList.clear();
}

void Renderer::setSamplingType (const SamplingType samplingType)
{
	m_samplingType = samplingType;
}

void Renderer::setBlendIteration (const int blendIteration)
{
	m_blendIteration = blendIteration;
}

void Renderer::setFramebufferBlend (const bool blend)
{
	m_framebufferBlendEnabled = blend;
}

void Renderer::setFramebufferSRGB (const bool sRGB)
{
	m_framebufferSRGBEnabled = sRGB;
}

std::vector<tcu::Vec4> Renderer::getResultsPreDraw (void) const
{
	return m_resultsListPreDraw;
}

std::vector<tcu::Vec4> Renderer::getResultsPostDraw (void) const
{
	return m_resultsListPostDraw;
}

int Renderer::getBlendConfigCount (void) const
{
	return (int)m_blendConfigList.size();
}

void Renderer::copyFrameBufferTexture (const int srcPx, const int srcPy, const int dstPx, const int dstPy)
{
	const glw::Functions&	gl						= m_context.getRenderContext().getFunctions();
	int						fboSrcIdx				= -1;
	int						fboDstIdx				= -1;
	deUint32				fboSrcColAttachment		= GL_NONE;
	deUint32				fboDstColAttachment		= GL_NONE;

	for (int idx = 0; idx < (int)m_framebufferList.size(); idx++)
		this->bindFramebuffer(idx);

	// cache fbo attachments and idx locations
	for (int idx = 0; idx < (int)m_framebufferList.size(); idx++)
	{
		if (m_framebufferList[idx]->getType() == FBOTYPE_SOURCE)
		{
			fboSrcIdx = m_framebufferList[idx]->getIdx();
			fboSrcColAttachment = m_framebufferList[fboSrcIdx]->getColorAttachment();
		}
		if (m_framebufferList[idx]->getType() == FBOTYPE_DESTINATION)
		{
			fboDstIdx = m_framebufferList[idx]->getIdx();
			fboDstColAttachment = m_framebufferList[fboDstIdx]->getColorAttachment();
		}
	}

	for (int idx = 0; idx < (int)m_framebufferList.size(); idx++)
		m_framebufferList[idx]->unbind();

	// store texel data from both src and dst before performing the copy
	m_resultsListPreDraw.resize(2);
	m_framebufferList[fboSrcIdx]->bind();
	this->readTexels(0, 0, fboSrcColAttachment, m_resultsListPreDraw[0]);
	m_framebufferList[fboSrcIdx]->unbind();
	m_framebufferList[fboDstIdx]->setTargetType(GL_READ_FRAMEBUFFER);
	m_framebufferList[fboDstIdx]->bind();
	this->readTexels(0, 0, fboDstColAttachment, m_resultsListPreDraw[1]);
	m_framebufferList[fboDstIdx]->unbind();
	m_framebufferList[fboDstIdx]->setTargetType(GL_DRAW_FRAMEBUFFER);

	m_framebufferList[fboSrcIdx]->bind();
	m_framebufferList[fboDstIdx]->bind();

	this->enableFramebufferSRGB();
	this->enableFramebufferBlend();

	gl.blitFramebuffer(	srcPx, srcPy, TestTextureSizes::WIDTH, TestTextureSizes::HEIGHT,
						dstPx, dstPy, TestTextureSizes::WIDTH, TestTextureSizes::HEIGHT,
						GL_COLOR_BUFFER_BIT, GL_NEAREST);

	m_resultsListPostDraw.resize(2);
	this->readTexels(0, 0, fboSrcColAttachment, m_resultsListPostDraw[0]);
	m_framebufferList[fboSrcIdx]->unbind();
	m_framebufferList[fboDstIdx]->unbind();

	m_framebufferList[fboDstIdx]->setTargetType(GL_READ_FRAMEBUFFER);
	m_framebufferList[fboDstIdx]->bind();
	this->readTexels(0, 0, fboDstColAttachment, m_resultsListPostDraw[1]);
	m_framebufferList[fboDstIdx]->unbind();
}

void Renderer::draw (void)
{
	const glw::Functions&	gl = m_context.getRenderContext().getFunctions();

	if (m_renderPassConfig.textureSourcesType == TEXTURESOURCESTYPE_NONE)
		DE_FATAL("Error: Attempted to draw with no texture sources");

	// resize results storage with each render pass
	m_resultsListPreDraw.resize(m_renderPass + 1);
	m_resultsListPostDraw.resize(m_renderPass + 1);

	m_shaderProgram->use();
	m_vertexData.bind();

	for (int idx = 0; idx < (int)m_framebufferList.size(); idx++)
		this->bindFramebuffer(idx);

	this->bindAllRequiredSourceTextures(m_renderPassConfig.textureSourcesType);
	this->bindActiveTexturesSamplers();

	this->enableFramebufferSRGB();
	this->enableFramebufferBlend();

	this->readTexels(0, 0, GL_COLOR_ATTACHMENT0, m_resultsListPreDraw[m_renderPass]);
	this->setShaderProgramSamplingType(m_samplingType);
	if (m_hasFunction)
	{
		this->setShaderBlendFunctionType();
		this->setShaderBlendSrcDstValues();
	}

	gl.drawArrays(GL_TRIANGLES, 0, 6);

	this->readTexels(0, 0, GL_COLOR_ATTACHMENT0, m_resultsListPostDraw[m_renderPass]);
	this->logState(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_samplingType);

	this->unbindAllSourceTextures();
	for (int idx = 0; idx < (int)m_framebufferList.size(); idx++)
		this->unbindFramebuffer(idx);
	m_vertexData.unbind();
	m_shaderProgram->unuse();
}

void Renderer::storeShaderProgramInfo (void)
{
	m_shaderProgramInfo = m_shaderProgram->getLogInfo();
	m_hasShaderProgramInfo = true;
}

void Renderer::logShaderProgramInfo (void)
{
	tcu::TestLog& log = m_context.getTestContext().getLog();

	if (m_hasShaderProgramInfo)
		log << m_shaderProgramInfo;
}

void Renderer::createFBOwithColorAttachment (const std::vector<FBOConfig> fboConfigList)
{
	const int size = (int)fboConfigList.size();
	for (int idx = 0; idx < size; idx++)
	{
		TextureSp texture(new TestTexture2D(m_context, fboConfigList[idx].textureInternalFormat, GL_RGBA, GL_UNSIGNED_BYTE, fboConfigList[idx].textureColor));
		m_fboTextureList.push_back(texture);

		bool isSRGB;
		if (fboConfigList[idx].textureInternalFormat == GL_SRGB8_ALPHA8)
			isSRGB = true;
		else
			isSRGB = false;

		FboSp framebuffer(new TestFramebuffer(m_context, fboConfigList[idx].fboTargetType, fboConfigList[idx].fboColorAttachment, texture->getHandle(), isSRGB, fboConfigList[idx].fboType, idx));
		m_framebufferList.push_back(framebuffer);
	}
}

void Renderer::setShaderProgramSamplingType (const int samplerIdx)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint location = gl.getUniformLocation(m_shaderProgram->getHandle(), "uFunctionType");
	DE_ASSERT(location != (glw::GLuint)-1);
	gl.uniform1i(location, samplerIdx);
}

void Renderer::setShaderBlendFunctionType (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	int function = -1;
	if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_ADD)
		function = 0;
	else if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_SUBTRACT)
		function = 1;
	else if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_REVERSE_SUBTRACT)
		function = 2;
	else
		DE_FATAL("Error: Blend function not recognised");

	glw::GLuint location = gl.getUniformLocation(m_shaderProgram->getHandle(), "uBlendFunctionType");
	DE_ASSERT(location != (glw::GLuint)-1);
	gl.uniform1i(location, function);
}

void Renderer::setShaderBlendSrcDstValues (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	float funcSrc;
	if (m_blendConfigList[m_blendIteration].funcSrc == GL_ONE)
		funcSrc = 1.0f;
	else
		funcSrc = 0.0f;

	float funcDst;
		if (m_blendConfigList[m_blendIteration].funcDst == GL_ONE)
		funcDst = 1.0f;
	else
		funcDst = 0.0f;

	glw::GLuint locationSrc = gl.getUniformLocation(m_shaderProgram->getHandle(), "uFactorSrc");
	gl.uniform1f(locationSrc, funcSrc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f()");

	glw::GLuint locationDst = gl.getUniformLocation(m_shaderProgram->getHandle(), "uFactorDst");
	gl.uniform1f(locationDst, funcDst);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f()");
}

void Renderer::bindActiveTexturesSamplers (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (int idx = 0; idx < m_samplersRequired; idx++)
	{
		std::ostringstream stream;
		stream << "uTexture" << idx;
		std::string uniformName(stream.str());
		glw::GLint location = gl.getUniformLocation(m_shaderProgram->getHandle(), uniformName.c_str());
		DE_ASSERT(location != -1);
		gl.uniform1i(location, m_textureSourceList[idx]->getTextureUnit());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation()");
	}
}

void Renderer::bindAllRequiredSourceTextures (const TextureSourcesType texturesRequired)
{
	if (texturesRequired == TEXTURESOURCESTYPE_RGBA)
		m_textureSourceList[0]->bind(0);
	else if (texturesRequired == TEXTURESOURCESTYPE_SRGBA)
		m_textureSourceList[1]->bind(0);
	else if (texturesRequired == TEXTURESOURCESTYPE_BOTH)
	{
		m_textureSourceList[0]->bind(0);
		m_textureSourceList[1]->bind(1);
	}
	else
		DE_FATAL("Error: Invalid sources requested in bind all");
}

void Renderer::unbindAllSourceTextures (void)
{
	for (int idx = 0; idx < (int)m_textureSourceList.size(); idx++)
		m_textureSourceList[idx]->unbind();
}

void Renderer::bindFramebuffer (const int framebufferIdx)
{
	m_framebufferList[framebufferIdx]->bind();
}

void Renderer::unbindFramebuffer (const int framebufferIdx)
{
	m_framebufferList[framebufferIdx]->unbind();
}

void Renderer::enableFramebufferSRGB (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_framebufferSRGBEnabled)
		gl.enable(GL_FRAMEBUFFER_SRGB);
	else
		gl.disable(GL_FRAMEBUFFER_SRGB);
}

void Renderer::enableFramebufferBlend (void)
{
	const glw::Functions&	gl	= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log	= m_context.getTestContext().getLog();
	std::ostringstream		message;

	message << "Blend settings = ";

	if (m_framebufferBlendEnabled)
	{
		gl.enable(GL_BLEND);
		gl.blendEquation(m_blendConfigList[m_blendIteration].equation);
		gl.blendFunc(m_blendConfigList[m_blendIteration].funcSrc, m_blendConfigList[m_blendIteration].funcDst);

		std::string equation, src, dst;
		if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_ADD)
			equation = "GL_FUNC_ADD";
		if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_SUBTRACT)
			equation = "GL_FUNC_SUBTRACT";
		if (m_blendConfigList[m_blendIteration].equation == GL_FUNC_REVERSE_SUBTRACT)
			equation = "GL_FUNC_REVERSE_SUBTRACT";
		if (m_blendConfigList[m_blendIteration].funcSrc == GL_ONE)
			src = "GL_ONE";
		else
			src = "GL_ZERO";
		if (m_blendConfigList[m_blendIteration].funcDst == GL_ONE)
			dst = "GL_ONE";
		else
			dst = "GL_ZERO";

		message << "Enabled: equation = " << equation << ", func src = " << src << ", func dst = " << dst;
	}
	else
	{
		gl.disable(GL_BLEND);
		message << "Disabled";
	}

	log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;
}

bool Renderer::isFramebufferAttachmentSRGB (const deUint32 targetType, const deUint32 attachment) const
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	glw::GLint				encodingType;

	gl.getFramebufferAttachmentParameteriv(targetType, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encodingType);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetNamedFramebufferAttachmentParameteriv()");

	switch (static_cast<glw::GLenum>(encodingType))
	{
		case GL_SRGB:
		{
			return true;
			break;
		}
		case GL_LINEAR:
		{
			return false;
			break;
		}
		default:
		{
			DE_FATAL("Error: Color attachment format not recognised");
			return false;
		}
	}
}

void Renderer::readTexels (const int px, const int py, const deUint32 mode, tcu::Vec4& texelData)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::TextureLevel		textureRead;

	// ensure result sampling coordinates are within range of the result color attachment
	DE_ASSERT((px >= 0) && (px < m_context.getRenderTarget().getWidth()));
	DE_ASSERT((py >= 0) && (py < m_context.getRenderTarget().getHeight()));

	gl.readBuffer(mode);
	textureRead.setStorage(glu::mapGLTransferFormat(GL_RGBA, GL_UNSIGNED_BYTE), TestTextureSizes::WIDTH, TestTextureSizes::HEIGHT);
	glu::readPixels(m_context.getRenderContext(), px, py, textureRead.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels()");
	texelData = textureRead.getAccess().getPixel(px, py);
}

void Renderer::logState (const deUint32 targetType, const deUint32 attachment, const SamplingType samplingType) const
{
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	std::ostringstream		message;

	bool fboAttachmentSRGB = this->isFramebufferAttachmentSRGB(targetType, attachment);
	message.str("");
	message << "getFramebufferAttachmentParameteriv() check = ";
	if (fboAttachmentSRGB)
		message << "GL_SRGB";
	else
		message << "GL_LINEAR";
	log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;

	message.str("");
	message << "Framebuffer color attachment value BEFORE draw call";
	logColor(m_context, message.str(), m_resultsListPreDraw[m_renderPass]);

	message.str("");
	message << "Framebuffer color attachment value AFTER draw call";
	logColor(m_context, message.str(), m_resultsListPostDraw[m_renderPass]);

	message.str("");
	message << "Sampling type = ";
	std::string type;
	if (samplingType == 0)
		type = "texture()";
	else if (samplingType == 1)
		type = "textureLOD()";
	else if (samplingType == 2)
		type = "textureGrad()";
	else if (samplingType == 3)
		type = "textureOffset()";
	else if (samplingType == 4)
		type = "textureProj()";
	else
		DE_FATAL("Error: Sampling type unregonised");
	message << type;
	log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;

	message.str("");
	if (m_framebufferSRGBEnabled)
		message << "Framebuffer SRGB = enabled";
	else
		message << "Framebuffer SRGB = disabled";
	log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;
}

class FboSRGBTestCase : public TestCase
{
public:
											FboSRGBTestCase				(Context& context, const char* const name, const char* const desc);
											~FboSRGBTestCase			(void);

	void									init						(void);
	void									deinit						(void);
	IterateResult							iterate						(void);

	void									setTestConfig				(std::vector<TestRenderPassConfig> renderPassConfigList);

	virtual void							setupTest					(void) = 0;
	virtual bool							verifyResult				(void) = 0;

protected:
	bool									m_hasTestConfig;
	std::vector<TestRenderPassConfig>		m_renderPassConfigList;
	bool									m_testcaseRequiresBlend;
	std::vector<tcu::Vec4>					m_resultsPreDraw;
	std::vector<tcu::Vec4>					m_resultsPostDraw;

private:
											FboSRGBTestCase				(const FboSRGBTestCase&);
	FboSRGBTestCase&						operator=					(const FboSRGBTestCase&);
};

FboSRGBTestCase::FboSRGBTestCase	(Context& context, const char* const name, const char* const desc)
	: TestCase						(context, name, desc)
	, m_hasTestConfig				(false)
{
}

FboSRGBTestCase::~FboSRGBTestCase (void)
{
	FboSRGBTestCase::deinit();
}

void FboSRGBTestCase::init (void)
{
	// extensions requirements for test
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires a context version equal or higher than 3.2");

	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_sRGB_write_control"))
		TCU_THROW(NotSupportedError, "Test requires extension GL_EXT_sRGB_write_control");

	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_sRGB_decode"))
		TCU_THROW(NotSupportedError, "Test requires GL_EXT_texture_sRGB_decode extension");
}

void FboSRGBTestCase::deinit (void)
{
}

FboSRGBTestCase::IterateResult FboSRGBTestCase::iterate (void)
{
	this->setupTest();

	DE_ASSERT(m_hasTestConfig && "Error: Renderer was not supplied a test config");

	Renderer renderer(m_context);

	// loop through each sampling type
	for (int samplingIdx = 0; samplingIdx < SampligTypeCount::MAX; samplingIdx++)
	{
		renderer.setSamplingType(static_cast<SamplingType>(samplingIdx));

		// loop through each blend configuration
		const int blendCount = renderer.getBlendConfigCount();
		for (int blendIdx = 0; blendIdx < blendCount; blendIdx++)
		{
			// loop through each render pass
			const int renderPassCount = (int)m_renderPassConfigList.size();
			for (int renderPassIdx = 0; renderPassIdx < renderPassCount; renderPassIdx++)
			{
				TestRenderPassConfig renderPassConfig = m_renderPassConfigList[renderPassIdx];

				renderer.init(renderPassConfig, renderPassIdx);

				if (blendIdx == 0 && renderPassConfig.rendererTask == RENDERERTASK_DRAW)
					renderer.storeShaderProgramInfo();

				if (renderPassConfig.frameBufferBlend == FRAMEBUFFERBLEND_ENABLED)
				{
					renderer.setBlendIteration(blendIdx);
					renderer.setFramebufferBlend(true);
				}
				else
					renderer.setFramebufferBlend(false);

				if (renderPassConfig.framebufferSRGB == FRAMEBUFFERSRGB_ENABLED)
					renderer.setFramebufferSRGB(true);
				else
					renderer.setFramebufferSRGB(false);

				if (renderPassConfig.rendererTask == RENDERERTASK_DRAW)
					renderer.draw();
				else if (renderPassConfig.rendererTask == RENDERERTASK_COPY)
					renderer.copyFrameBufferTexture(0, 0, 0, 0);
				else
					DE_FATAL("Error: render task not recognised");

				renderer.deinit();

			} // render passes

			m_resultsPreDraw = renderer.getResultsPreDraw();
			m_resultsPostDraw = renderer.getResultsPostDraw();

			bool testPassed = this->verifyResult();
			if (testPassed)
				m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
			else
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result verification failed");
				renderer.logShaderProgramInfo();
				return STOP;
			}

			if (!m_testcaseRequiresBlend)
				break;
		} // blend configs

		renderer.logShaderProgramInfo();
	} // sampling types

	return STOP;
}

void FboSRGBTestCase::setTestConfig (std::vector<TestRenderPassConfig> renderPassConfigList)
{
	m_renderPassConfigList = renderPassConfigList;
	m_hasTestConfig = true;

	for (int idx = 0; idx < (int)renderPassConfigList.size(); idx++)
	{
		if (renderPassConfigList[idx].frameBufferBlend == FRAMEBUFFERBLEND_ENABLED)
		{
			m_testcaseRequiresBlend = true;
			return;
		}
	}
	m_testcaseRequiresBlend = false;
}

class FboSRGBQueryCase : public TestCase
{
public:
					FboSRGBQueryCase	(Context& context, const char* const name, const char* const description);
					~FboSRGBQueryCase	(void);

	void			init				(void);
	void			deinit				(void);
	IterateResult	iterate				(void);
};

FboSRGBQueryCase::FboSRGBQueryCase	(Context& context, const char* const name, const char* const description)
	: TestCase						(context, name, description)
{
}

FboSRGBQueryCase::~FboSRGBQueryCase (void)
{
	FboSRGBQueryCase::deinit();
}

void FboSRGBQueryCase::init (void)
{
	// extension requirements for test
	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_sRGB_write_control"))
		TCU_THROW(NotSupportedError, "Test requires extension GL_EXT_sRGB_write_control");
}

void FboSRGBQueryCase::deinit (void)
{
}

FboSRGBQueryCase::IterateResult FboSRGBQueryCase::iterate (void)
{
	// TEST INFO:
	// API tests which check when querying FRAMEBUFFER_SRGB_EXT capability returns the correct information when using glEnabled() or glDisabled()

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log		= m_context.getTestContext().getLog();
	const char*	const		msgPart	= ", after disabling = ";

	for (int idx = 0; idx < static_cast<int>(QUERYTYPE_LAST); idx++)
	{
		std::ostringstream	message;
		bool				pass		= false;

		message << std::string("Results: After Enabling = ");

		gl.enable(GL_FRAMEBUFFER_SRGB);

		switch (static_cast<QueryType>(idx))
		{
			case QUERYTYPE_ISENABLED:
			{
				glw::GLboolean enabled[2];
				enabled[0] = gl.isEnabled(GL_FRAMEBUFFER_SRGB);
				gl.disable(GL_FRAMEBUFFER_SRGB);
				enabled[1] = gl.isEnabled(GL_FRAMEBUFFER_SRGB);

				message << static_cast<float>(enabled[0]) << msgPart << static_cast<float>(enabled[1]);
				pass = (enabled[0] && !(enabled[1])) ? true : false;
				break;
			}
			case QUERYTYPE_BOOLEAN:
			{
				glw::GLboolean enabled[2];
				gl.getBooleanv(GL_FRAMEBUFFER_SRGB,&enabled[0]);
				gl.disable(GL_FRAMEBUFFER_SRGB);
				gl.getBooleanv(GL_FRAMEBUFFER_SRGB,&enabled[1]);

				message << static_cast<float>(enabled[0]) << msgPart << static_cast<float>(enabled[1]);
				pass = (enabled[0] && !(enabled[1])) ? true : false;
				break;
			}
			case QUERYTYPE_FLOAT:
			{
				glw::GLfloat enabled[2];
				gl.getFloatv(GL_FRAMEBUFFER_SRGB, &enabled[0]);
				gl.disable(GL_FRAMEBUFFER_SRGB);
				gl.getFloatv(GL_FRAMEBUFFER_SRGB, &enabled[1]);

				message << static_cast<float>(enabled[0]) << msgPart << static_cast<float>(enabled[1]);
				pass = ((int)enabled[0] && !((int)enabled[1])) ? true : false;
				break;
			}
			case QUERYTYPE_INT:
			{
				glw::GLint enabled[2];
				gl.getIntegerv(GL_FRAMEBUFFER_SRGB, &enabled[0]);
				gl.disable(GL_FRAMEBUFFER_SRGB);
				gl.getIntegerv(GL_FRAMEBUFFER_SRGB, &enabled[1]);

				message << static_cast<float>(enabled[0]) << msgPart << static_cast<float>(enabled[1]);
				pass = (enabled[0] && !(enabled[1])) ? true : false;
				break;
			}
			case QUERYTYPE_INT64:
			{
				glw::GLint64 enabled[2];
				gl.getInteger64v(GL_FRAMEBUFFER_SRGB, &enabled[0]);
				gl.disable(GL_FRAMEBUFFER_SRGB);
				gl.getInteger64v(GL_FRAMEBUFFER_SRGB, &enabled[1]);

				message << static_cast<float>(enabled[0]) << msgPart << static_cast<float>(enabled[1]);
				pass = (enabled[0] && !(enabled[1])) ? true : false;
				break;
			}
			default:
				DE_FATAL("Error: Datatype not recognised");
		}

		log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;

		if (pass)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result verification failed");
			return STOP;
		}
	}
	return STOP;
}

class FboSRGBColAttachCase : public FboSRGBTestCase
{
public:
			FboSRGBColAttachCase	(Context& context, const char* const name, const char* const description)
				: FboSRGBTestCase	(context, name, description) {}
			~FboSRGBColAttachCase	(void) {}

	void	setupTest				(void);
	bool	verifyResult			(void);
};

void FboSRGBColAttachCase::setupTest (void)
{
	// TEST INFO:
	// Check if FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING  set to SRGB and FRAMEBUFFER_SRGB_EXT enabled, destination colors are converted from SRGB to linear
	// before and after blending, finally the result is converted back to SRGB for storage

	// NOTE:
	// if fbo pre-draw color set to linaer, color values get linearlized "twice"
	// (0.2f, 0.3f, 0.4f, 1.0f) when sampled i.e. converted in shader = (0.0331048f, 0.073239f, 0.132868f)
	// resulting in the follolwing blending equation (0.2f, 0.3f, 0.4f 1.0f) + (0.0331048, 0.073239, 0.132868) = (0.521569f, 0.647059f, 0.756863f, 1.0f)

	FBOConfig fboConfig0 = FBOConfig(GL_SRGB8_ALPHA8, getTestColorLinear(), GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_SOURCE);
	FBOConfig fboConfig1 = FBOConfig(GL_RGBA8, getTestColorLinear(), GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_SOURCE);

	const TestRenderPassConfig renderPassConfigs[] =
	{
		TestRenderPassConfig(TEXTURESOURCESTYPE_RGBA, fboConfig0, FRAMEBUFFERSRGB_ENABLED, FRAMEBUFFERBLEND_ENABLED, RENDERERTASK_DRAW),
		TestRenderPassConfig(TEXTURESOURCESTYPE_BOTH, fboConfig1, FRAMEBUFFERSRGB_DISABLED, FRAMEBUFFERBLEND_DISABLED, getFunctionBlendLinearToSRGBCheck(), RENDERERTASK_DRAW)
	};
	std::vector<TestRenderPassConfig> renderPassConfigList(renderPassConfigs, renderPassConfigs + DE_LENGTH_OF_ARRAY(renderPassConfigs));

	this->setTestConfig(renderPassConfigList);
}

bool FboSRGBColAttachCase::verifyResult (void)
{
	if (tcu::boolAll(tcu::lessThan(tcu::abs(m_resultsPostDraw[0] - m_resultsPostDraw[1]), getEpsilonError())) || tcu::boolAll(tcu::equal(m_resultsPostDraw[0], m_resultsPostDraw[1])))
		return true;
	else
		return false;
}

class FboSRGBToggleBlendCase : public FboSRGBTestCase
{
public:
			FboSRGBToggleBlendCase		(Context& context, const char* const name, const char* const description)
				: FboSRGBTestCase		(context, name, description) {}
			~FboSRGBToggleBlendCase		(void) {}

	void	setupTest					(void);
	bool	verifyResult				(void);
};

void FboSRGBToggleBlendCase::setupTest (void)
{
	//	TEST INFO:
	//	Test to check if changing FRAMEBUFFER_SRGB_EXT from enabled to disabled. Enabled should produce SRGB color whilst disabled
	//	should produce linear color. Test conducted with blending disabled.

	FBOConfig fboConfig0 = FBOConfig(GL_SRGB8_ALPHA8, getTestColorLinear(), GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_DESTINATION);

	const TestRenderPassConfig renderPassConfigs[] =
	{
		TestRenderPassConfig(TEXTURESOURCESTYPE_RGBA, fboConfig0, FRAMEBUFFERSRGB_ENABLED,  FRAMEBUFFERBLEND_DISABLED, TestFunction(false), RENDERERTASK_DRAW),
		TestRenderPassConfig(TEXTURESOURCESTYPE_RGBA, fboConfig0, FRAMEBUFFERSRGB_DISABLED, FRAMEBUFFERBLEND_DISABLED, TestFunction(false), RENDERERTASK_DRAW)
	};
	std::vector<TestRenderPassConfig> renderPassConfigList(renderPassConfigs, renderPassConfigs + DE_LENGTH_OF_ARRAY(renderPassConfigs));

	this->setTestConfig(renderPassConfigList);
}

bool FboSRGBToggleBlendCase::verifyResult (void)
{
	if (tcu::boolAny(tcu::greaterThan(tcu::abs(m_resultsPostDraw[0] - m_resultsPostDraw[1]), getEpsilonError())))
		return true;
	else
		return false;
}

class FboSRGBRenderTargetIgnoreCase : public FboSRGBTestCase
{
public:
			FboSRGBRenderTargetIgnoreCase		(Context& context, const char* const name, const char* const description)
				: FboSRGBTestCase				(context, name, description) {}
			~FboSRGBRenderTargetIgnoreCase		(void) {}

	void	setupTest							(void);
	bool	verifyResult						(void);
};

void FboSRGBRenderTargetIgnoreCase::setupTest (void)
{
	// TEST INFO:
	// Check if render targets that are non-RGB ignore the state of GL_FRAMEBUFFER_SRGB_EXT. Rendering to an fbo with non-sRGB color
	// attachment should ignore color space conversion, producing linear color.

	FBOConfig fboConfig0 = FBOConfig(GL_RGBA8, getTestColorBlank(), GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_DESTINATION);

	const TestRenderPassConfig renderPassConfigs[] =
	{
		TestRenderPassConfig(TEXTURESOURCESTYPE_RGBA, fboConfig0, FRAMEBUFFERSRGB_ENABLED,  FRAMEBUFFERBLEND_DISABLED, TestFunction(false), RENDERERTASK_DRAW)

	};
	std::vector<TestRenderPassConfig> renderPassConfigList(renderPassConfigs, renderPassConfigs + DE_LENGTH_OF_ARRAY(renderPassConfigs));

	this->setTestConfig(renderPassConfigList);
}

bool FboSRGBRenderTargetIgnoreCase::verifyResult (void)
{
	if (tcu::boolAll(tcu::lessThan(tcu::abs(m_resultsPostDraw[0] - getTestColorLinear()), getEpsilonError())) || tcu::boolAll(tcu::equal(m_resultsPostDraw[0], getTestColorLinear())))
		return true;
	else
		return false;
}

class FboSRGBCopyToLinearCase : public FboSRGBTestCase
{
public:
			FboSRGBCopyToLinearCase		(Context& context, const char* const name, const char* const description)
				: FboSRGBTestCase		(context, name, description) {}
			~FboSRGBCopyToLinearCase	(void) {}

	void	setupTest					(void);
	bool	verifyResult				(void);
};

void FboSRGBCopyToLinearCase::setupTest (void)
{
	// TEST INFO:
	// Check if copying from an fbo with an sRGB color attachment to an fbo with a linear color attachment with FRAMEBUFFER_EXT enabled results in
	// an sRGB to linear conversion

	FBOConfig fboConfigs[] =
	{
		FBOConfig(GL_SRGB8_ALPHA8, getTestColorSRGB(), GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_SOURCE),
		FBOConfig(GL_RGBA8, getTestColorBlank(), GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, FBOTYPE_DESTINATION)
	};
	std::vector<FBOConfig> fboConfigList(fboConfigs, fboConfigs + DE_LENGTH_OF_ARRAY(fboConfigs));

	const TestRenderPassConfig renderPassConfigs[] =
	{
		TestRenderPassConfig(TEXTURESOURCESTYPE_NONE, fboConfigList, FRAMEBUFFERSRGB_ENABLED,  FRAMEBUFFERBLEND_DISABLED, TestFunction(false), RENDERERTASK_COPY)
	};
	std::vector<TestRenderPassConfig> renderPassConfigList(renderPassConfigs, renderPassConfigs + DE_LENGTH_OF_ARRAY(renderPassConfigs));

	this->setTestConfig(renderPassConfigList);
}

bool FboSRGBCopyToLinearCase::verifyResult (void)
{
	logColor(m_context, "pre-copy source fbo color values", m_resultsPreDraw[0]);
	logColor(m_context, "pre-copy destination fbo color values", m_resultsPreDraw[1]);
	logColor(m_context, "post-copy source fbo color values", m_resultsPostDraw[0]);
	logColor(m_context, "post-copy destination fbo color values", m_resultsPostDraw[1]);

	if (tcu::boolAll(tcu::lessThan(tcu::abs(m_resultsPostDraw[1] - getTestColorLinear()), getEpsilonError())) || tcu::boolAll(tcu::equal(m_resultsPostDraw[1], getTestColorLinear())))
		return true;
	else
		return false;
}

class FboSRGBUnsupportedEnumCase : public TestCase
{
public:
					FboSRGBUnsupportedEnumCase	(Context& context, const char* const name, const char* const description);
					~FboSRGBUnsupportedEnumCase	(void);

	void			init						(void);
	void			deinit						(void);
	bool			isInvalidEnum				(std::string functionName);
	IterateResult	iterate						(void);
};

FboSRGBUnsupportedEnumCase::FboSRGBUnsupportedEnumCase	(Context& context, const char* const name, const char* const description)
	: TestCase						(context, name, description)
{
}

FboSRGBUnsupportedEnumCase::~FboSRGBUnsupportedEnumCase (void)
{
	FboSRGBUnsupportedEnumCase::deinit();
}

void FboSRGBUnsupportedEnumCase::init (void)
{
	// extension requirements for test
	if (m_context.getContextInfo().isExtensionSupported("GL_EXT_sRGB_write_control"))
		TCU_THROW(NotSupportedError, "Test requires extension GL_EXT_sRGB_write_control to be unsupported");
}

void FboSRGBUnsupportedEnumCase::deinit (void)
{
}

bool FboSRGBUnsupportedEnumCase::isInvalidEnum (std::string functionName)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log			= m_context.getTestContext().getLog();
	bool					isOk		= true;
	glw::GLenum				error		= GL_NO_ERROR;

	log << tcu::TestLog::Message << "Checking call to " << functionName << tcu::TestLog::EndMessage;

	error = gl.getError();

	if (error != GL_INVALID_ENUM)
	{
		log << tcu::TestLog::Message << " returned wrong value [" << glu::getErrorStr(error) << ", expected " << glu::getErrorStr(GL_INVALID_ENUM) << "]" << tcu::TestLog::EndMessage;
		isOk = false;
	}

	return isOk;
}

FboSRGBUnsupportedEnumCase::IterateResult FboSRGBUnsupportedEnumCase::iterate (void)
{
	// TEST INFO:
	// API tests that check calls using enum GL_FRAMEBUFFER_SRGB return GL_INVALID_ENUM  when GL_EXT_sRGB_write_control is not supported

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	bool					allPass		= true;
	glw::GLboolean			bEnabled	= GL_FALSE;
	glw::GLfloat			fEnabled	= 0;
	glw::GLint				iEnabled	= 0;
	glw::GLint64			lEnabled	= 0;

	m_context.getTestContext().getLog() << tcu::TestLog::Message
										<< "Check calls using enum GL_FRAMEBUFFER_SRGB return GL_INVALID_ENUM  when GL_EXT_sRGB_write_control is not supported\n\n"
										<< tcu::TestLog::EndMessage;

	gl.enable(GL_FRAMEBUFFER_SRGB);
	allPass &= isInvalidEnum("glEnable()");

	gl.disable(GL_FRAMEBUFFER_SRGB);
	allPass &= isInvalidEnum("glDisable()");

	gl.isEnabled(GL_FRAMEBUFFER_SRGB);
	allPass &= isInvalidEnum("glIsEnabled()");

	gl.getBooleanv(GL_FRAMEBUFFER_SRGB, &bEnabled);
	allPass &= isInvalidEnum("glGetBooleanv()");

	gl.getFloatv(GL_FRAMEBUFFER_SRGB, &fEnabled);
	allPass &= isInvalidEnum("glGetFloatv()");

	gl.getIntegerv(GL_FRAMEBUFFER_SRGB, &iEnabled);
	allPass &= isInvalidEnum("glGetIntegerv()");

	gl.getInteger64v(GL_FRAMEBUFFER_SRGB, &lEnabled);
	allPass &= isInvalidEnum("glGetInteger64v()");

	if (allPass)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

} // anonymous

FboSRGBWriteControlTests::FboSRGBWriteControlTests	(Context& context)
	: TestCaseGroup			(context, "srgb_write_control", "Colorbuffer tests")
{
}

FboSRGBWriteControlTests::~FboSRGBWriteControlTests (void)
{
}

void FboSRGBWriteControlTests::init (void)
{
	this->addChild(new FboSRGBQueryCase					(m_context, "framebuffer_srgb_enabled",							"srgb enable framebuffer"));
	this->addChild(new FboSRGBColAttachCase				(m_context, "framebuffer_srgb_enabled_col_attach",				"srgb enable color attachment and framebuffer"));
	this->addChild(new FboSRGBToggleBlendCase			(m_context, "framebuffer_srgb_enabled_blend",					"toggle framebuffer srgb settings with blend disabled"));
	this->addChild(new FboSRGBRenderTargetIgnoreCase	(m_context, "framebuffer_srgb_enabled_render_target_ignore",	"enable framebuffer srgb, non-srgb render target should ignore"));
	this->addChild(new FboSRGBCopyToLinearCase			(m_context, "framebuffer_srgb_enabled_copy_to_linear",			"no conversion when blittering between framebuffer srgb and linear"));

	// negative
	this->addChild(new FboSRGBUnsupportedEnumCase		(m_context, "framebuffer_srgb_unsupported_enum",				"check error codes for query functions when extension is not supported"));
}

}
} // gles31
} // deqp
