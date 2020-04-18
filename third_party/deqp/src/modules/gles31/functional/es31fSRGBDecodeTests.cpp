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
 * \brief Texture format tests.
 *//*--------------------------------------------------------------------*/

#include "es31fSRGBDecodeTests.hpp"
#include "gluContextInfo.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluTexture.hpp"
#include "glsTextureTestUtil.hpp"
#include "tcuPixelFormat.hpp"
#include "tcuTestContext.hpp"
#include "tcuRenderTarget.hpp"
#include "gluTextureUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "glwFunctions.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "deUniquePtr.hpp"
#include "gluPixelTransfer.hpp"
#include "tcuDefs.hpp"
#include "tcuVectorUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using glu::TextureTestUtil::TEXTURETYPE_2D;

enum SRGBDecode
{
	SRGBDECODE_SKIP_DECODE		= 0,
	SRGBDECODE_DECODE,
	SRGBDECODE_DECODE_DEFAULT
};

enum ShaderOutputs
{
	SHADEROUTPUTS_ONE	= 1,
	SHADEROUTPUTS_TWO,
};

enum ShaderUniforms
{
	SHADERUNIFORMS_ONE	= 1,
	SHADERUNIFORMS_TWO,
};

enum ShaderSamplingGroup
{
	SHADERSAMPLINGGROUP_TEXTURE		= 0,
	SHADERSAMPLINGGROUP_TEXEL_FETCH
};

enum ShaderSamplingType
{
	TEXTURESAMPLING_TEXTURE													= 0,
	TEXTURESAMPLING_TEXTURE_LOD,
	TEXTURESAMPLING_TEXTURE_GRAD,
	TEXTURESAMPLING_TEXTURE_OFFSET,
	TEXTURESAMPLING_TEXTURE_PROJ,
	TEXTURESAMPLING_TEXELFETCH,
	TEXTURESAMPLING_TEXELFETCH_OFFSET,

	// ranges required for looping mechanism in a case nodes iteration function
	TEXTURESAMPLING_TEXTURE_START		= TEXTURESAMPLING_TEXTURE,
	TEXTURESAMPLING_TEXTURE_END			= TEXTURESAMPLING_TEXTURE_PROJ		+ 1,
	TEXTURESAMPLING_TEXELFETCH_START	= TEXTURESAMPLING_TEXELFETCH,
	TEXTURESAMPLING_TEXELFETCH_END		= TEXTURESAMPLING_TEXELFETCH_OFFSET	+ 1
};

enum FunctionParameters
{
	FUNCTIONPARAMETERS_ONE = 1,
	FUNCTIONPARAMETERS_TWO
};

enum Blending
{
	BLENDING_REQUIRED		= 0,
	BLENDING_NOT_REQUIRED
};

enum Toggling
{
	TOGGLING_REQUIRED		= 0,
	TOGGLING_NOT_REQUIRED
};

tcu::Vec4 getColorReferenceLinear (void)
{
	return tcu::Vec4(0.2f, 0.3f, 0.4f, 1.0f);
}

tcu::Vec4 getColorReferenceSRGB (void)
{
	return tcu::linearToSRGB(tcu::Vec4(0.2f, 0.3f, 0.4f, 1.0f));
}

tcu::Vec4 getColorGreenPass (void)
{
	return tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
}

namespace TestDimensions
{
	const int WIDTH		= 128;
	const int HEIGHT	= 128;
} // global test texture dimensions

namespace TestSamplingPositions
{
	const int X_POS = 0;
	const int Y_POS = 0;
} // global test sampling positions

const char* getFunctionDefinitionSRGBToLinearCheck (void)
{
	static const char* functionDefinition =
			"mediump vec4 srgbToLinearCheck(in mediump vec4 texelSRGBA, in mediump vec4 texelLinear) \n"
			"{ \n"
			"	const int NUM_CHANNELS = 4;"
			"	mediump vec4 texelSRGBAConverted; \n"
			"	mediump vec4 epsilonErr = vec4(0.005); \n"
			"	mediump vec4 testResult; \n"
			"	for (int idx = 0; idx < NUM_CHANNELS; idx++) \n"
			"	{ \n"
			"		texelSRGBAConverted[idx] = pow( (texelSRGBA[idx] + 0.055) / 1.055, 1.0 / 0.4116); \n"
			"	} \n"
			"	if ( all(lessThan(abs(texelSRGBAConverted - texelLinear), epsilonErr)) || all(equal(texelSRGBAConverted, texelLinear)) ) \n"
			"	{ \n"
			"		return testResult = vec4(0.0, 1.0, 0.0, 1.0); \n"
			"	} \n"
			"	else \n"
			"	{ \n"
			"		return testResult = vec4(1.0, 0.0, 0.0, 1.0); \n"
			"	} \n"
			"} \n";

	return functionDefinition;
}

const char* getFunctionDefinitionEqualCheck (void)
{
	static const char* functionDefinition =
			"mediump vec4 colorsEqualCheck(in mediump vec4 colorA, in mediump vec4 colorB) \n"
			"{ \n"
			"	mediump vec4 epsilonErr = vec4(0.005); \n"
			"	mediump vec4 testResult; \n"
			"	if ( all(lessThan(abs(colorA - colorB), epsilonErr)) || all(equal(colorA, colorB)) ) \n"
			"	{ \n"
			"		return testResult = vec4(0.0, 1.0, 0.0, 1.0); \n"
			"	} \n"
			"	else \n"
			"	{ \n"
			"		return testResult = vec4(1.0, 0.0, 0.0, 1.0); \n"
			"	} \n"
			"} \n";

	return functionDefinition;
}

namespace EpsilonError
{
	const float CPU = 0.005f;
}

struct TestGroupConfig
{
	TestGroupConfig			(const char* groupName, const char* groupDescription, const tcu::TextureFormat groupInternalFormat)
		: name				(groupName)
		, description		(groupDescription)
		, internalFormat	(groupInternalFormat) {}

	~TestGroupConfig		(void) {};

	const char*					name;
	const char*					description;
	const tcu::TextureFormat	internalFormat;
};

struct UniformData
{
	UniformData			(glw::GLuint uniformLocation, const std::string& uniformName)
		: location		(uniformLocation)
		, name			(uniformName)
		, toggleDecode	(false) {}

	~UniformData		(void) {}

	glw::GLuint	location;
	std::string	name;
	bool		toggleDecode;
};

struct UniformToToggle
{
	UniformToToggle		(const int uniformProgramIdx, const std::string& uniformName)
		: programIdx	(uniformProgramIdx)
		, name			(uniformName) {}

	~UniformToToggle	(void) {}

	int			programIdx;
	std::string	name;
};

struct ComparisonFunction
{
	ComparisonFunction		(const std::string& funcName, const FunctionParameters funcParameters, const std::string& funcImplementation)
		: name				(funcName)
		, parameters		(funcParameters)
		, implementation	(funcImplementation) {}

	~ComparisonFunction		(void) {}

	std::string			name;
	FunctionParameters	parameters;
	std::string			implementation;
};

struct FragmentShaderParameters
{
	FragmentShaderParameters	(const ShaderOutputs	outputTotal,
								 const ShaderUniforms	uniformTotal,
								 ComparisonFunction*	comparisonFunction,
								 Blending				blendRequired,
								 Toggling				toggleRequired);

	~FragmentShaderParameters	(void);

	ShaderOutputs				outputTotal;
	ShaderUniforms				uniformTotal;
	ShaderSamplingType			samplingType;
	std::string					functionName;
	FunctionParameters			functionParameters;
	std::string					functionImplementation;
	bool						hasFunction;
	Blending					blendRequired;
	Toggling					toggleRequired;
	std::vector<std::string>	uniformsToToggle;
};

FragmentShaderParameters::FragmentShaderParameters	(const ShaderOutputs	paramsOutputTotal,
													 const ShaderUniforms	paramsUniformTotal,
													 ComparisonFunction*	paramsComparisonFunction,
													 Blending				paramsBlendRequired,
													 Toggling				paramsToggleRequired)
	: outputTotal									(paramsOutputTotal)
	, uniformTotal									(paramsUniformTotal)
	, blendRequired									(paramsBlendRequired)
	, toggleRequired								(paramsToggleRequired)
{
	if (paramsComparisonFunction != DE_NULL)
	{
		functionName			= paramsComparisonFunction->name;
		functionParameters		= paramsComparisonFunction->parameters;
		functionImplementation	= paramsComparisonFunction->implementation;

		hasFunction = true;
	}
	else
	{
		hasFunction = false;
	}
}

FragmentShaderParameters::~FragmentShaderParameters (void)
{
}

class SRGBTestSampler
{
public:
				SRGBTestSampler		(Context&						context,
									 const tcu::Sampler::WrapMode	wrapS,
									 const tcu::Sampler::WrapMode	wrapT,
									 const tcu::Sampler::FilterMode	minFilter,
									 const tcu::Sampler::FilterMode	magFilter,
									 const SRGBDecode				decoding);
				~SRGBTestSampler	(void);

	void		setDecode			(const SRGBDecode decoding);
	void		setTextureUnit		(const deUint32 textureUnit);
	void		setIsActive			(const bool isActive);

	bool		getIsActive			(void) const;

	void		bindToTexture		(void);

private:
	const glw::Functions*		m_gl;
	deUint32					m_samplerHandle;
	tcu::Sampler::WrapMode		m_wrapS;
	tcu::Sampler::WrapMode		m_wrapT;
	tcu::Sampler::FilterMode	m_minFilter;
	tcu::Sampler::FilterMode	m_magFilter;
	SRGBDecode					m_decoding;
	deUint32					m_textureUnit;
	bool						m_isActive;
};

SRGBTestSampler::SRGBTestSampler	(Context&						context,
									 const tcu::Sampler::WrapMode	wrapS,
									 const tcu::Sampler::WrapMode	wrapT,
									 const tcu::Sampler::FilterMode	minFilter,
									 const tcu::Sampler::FilterMode	magFilter,
									 const SRGBDecode				decoding)
	: m_gl							(&context.getRenderContext().getFunctions())
	, m_wrapS						(wrapS)
	, m_wrapT						(wrapT)
	, m_minFilter					(minFilter)
	, m_magFilter					(magFilter)
	, m_isActive					(false)
{
	m_gl->genSamplers(1, &m_samplerHandle);

	m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_WRAP_S, glu::getGLWrapMode(m_wrapS));
	m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_WRAP_T, glu::getGLWrapMode(m_wrapT));
	m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_MIN_FILTER, glu::getGLFilterMode(m_minFilter));
	m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_MAG_FILTER, glu::getGLFilterMode(m_magFilter));

	this->setDecode(decoding);
}

SRGBTestSampler::~SRGBTestSampler (void)
{
	m_gl->deleteSamplers(1, &m_samplerHandle);
}

void SRGBTestSampler::setDecode (const SRGBDecode decoding)
{
	if (decoding == SRGBDECODE_SKIP_DECODE)
	{
		m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "samplerParameteri(m_samplerID, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT)");
	}
	else if (decoding == SRGBDECODE_DECODE)
	{
		m_gl->samplerParameteri(m_samplerHandle, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "samplerParameteri(m_samplerID, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT)");
	}
	else
	{
		DE_FATAL("sRGB texture sampler must have either GL_SKIP_DECODE_EXT or GL_DECODE_EXT settings");
	}

	m_decoding = decoding;
}

void SRGBTestSampler::setTextureUnit (const deUint32 textureUnit)
{
	m_textureUnit = textureUnit;
}

void SRGBTestSampler::setIsActive (const bool isActive)
{
	m_isActive = isActive;
}

bool SRGBTestSampler::getIsActive (void) const
{
	return m_isActive;
}

void SRGBTestSampler::bindToTexture (void)
{
	m_gl->bindSampler(m_textureUnit, m_samplerHandle);
}

class SRGBTestTexture
{
public:
				SRGBTestTexture		(Context&									context,
									 const glu::TextureTestUtil::TextureType	targetType,
									 const tcu::TextureFormat					internalFormat,
									 const int									width,
									 const int									height,
									 const tcu::Vec4							color,
									 const tcu::Sampler::WrapMode				wrapS,
									 const tcu::Sampler::WrapMode				wrapT,
									 const tcu::Sampler::FilterMode				minFilter,
									 const tcu::Sampler::FilterMode				magFilter,
									 const SRGBDecode							decoding);
				~SRGBTestTexture	(void);

	void		setParameters		(void);
	void		setDecode			(const SRGBDecode decoding);
	void		setHasSampler		(const bool hasSampler);

	deUint32	getHandle			(void) const;
	deUint32	getGLTargetType		(void) const;
	SRGBDecode	getDecode			(void) const;

	void		upload				(void);

private:
	void		setColor			(void);

	Context&							m_context;
	glu::Texture2D						m_source;
	glu::TextureTestUtil::TextureType	m_targetType;
	const tcu::TextureFormat			m_internalFormat;
	const int							m_width;
	const int							m_height;
	tcu::Vec4							m_color;
	tcu::Sampler::WrapMode				m_wrapS;
	tcu::Sampler::WrapMode				m_wrapT;
	tcu::Sampler::FilterMode			m_minFilter;
	tcu::Sampler::FilterMode			m_magFilter;
	SRGBDecode							m_decoding;
	bool								m_hasSampler;
};

SRGBTestTexture::SRGBTestTexture	(Context&									context,
									 const glu::TextureTestUtil::TextureType	targetType,
									 const tcu::TextureFormat					internalFormat,
									 const int									width,
									 const int									height,
									 const tcu::Vec4							color,
									 const tcu::Sampler::WrapMode				wrapS,
									 const tcu::Sampler::WrapMode				wrapT,
									 const tcu::Sampler::FilterMode				minFilter,
									 const tcu::Sampler::FilterMode				magFilter,
									 SRGBDecode									decoding)
	: m_context						(context)
	, m_source						(context.getRenderContext(), glu::getInternalFormat(internalFormat), width, height)
	, m_targetType					(targetType)
	, m_internalFormat				(internalFormat)
	, m_width						(width)
	, m_height						(height)
	, m_color						(color)
	, m_wrapS						(wrapS)
	, m_wrapT						(wrapT)
	, m_minFilter					(minFilter)
	, m_magFilter					(magFilter)
	, m_decoding					(decoding)
	, m_hasSampler					(false)
{
	this->setColor();
}

SRGBTestTexture::~SRGBTestTexture (void)
{
}

void SRGBTestTexture::setParameters (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(this->getGLTargetType(), this->getHandle());

	gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_WRAP_S, glu::getGLWrapMode(m_wrapS));
	gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_WRAP_T, glu::getGLWrapMode(m_wrapT));
	gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_MIN_FILTER, glu::getGLFilterMode(m_minFilter));
	gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_MAG_FILTER, glu::getGLFilterMode(m_magFilter));

	gl.bindTexture(this->getGLTargetType(), 0);

	setDecode(m_decoding);
}

void SRGBTestTexture::setDecode (const SRGBDecode decoding)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(this->getGLTargetType(), this->getHandle());

	switch (decoding)
	{
		case SRGBDECODE_SKIP_DECODE:
		{
			gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri(this->getGLTargetType(), GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT)");
			break;
		}
		case SRGBDECODE_DECODE:
		{
			gl.texParameteri(this->getGLTargetType(), GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri(this->getGLTargetType(), GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT)");
			break;
		}
		case SRGBDECODE_DECODE_DEFAULT:
		{
			// do not use srgb decode options. Set to default
			break;
		}
		default:
			DE_FATAL("Error: Decoding option not recognised");
	}

	gl.bindTexture(this->getGLTargetType(), 0);

	m_decoding = decoding;
}

void SRGBTestTexture::setHasSampler (const bool hasSampler)
{
	m_hasSampler = hasSampler;
}

deUint32 SRGBTestTexture::getHandle (void) const
{
	return m_source.getGLTexture();
}

deUint32 SRGBTestTexture::getGLTargetType (void) const
{
	switch (m_targetType)
	{
		case TEXTURETYPE_2D:
		{
			return GL_TEXTURE_2D;
		}
		default:
		{
			DE_FATAL("Error: Target type not recognised");
			return -1;
		}
	}
}

SRGBDecode SRGBTestTexture::getDecode (void) const
{
	return m_decoding;
}

void SRGBTestTexture::upload (void)
{
	m_source.upload();
}

void SRGBTestTexture::setColor (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindTexture(this->getGLTargetType(), this->getHandle());

	m_source.getRefTexture().allocLevel(0);

	for (int py = 0; py < m_height; py++)
	{
		for (int px = 0; px < m_width; px++)
		{
			m_source.getRefTexture().getLevel(0).setPixel(m_color, px, py);
		}
	}

	gl.bindTexture(this->getGLTargetType(), 0);
}

class SRGBTestProgram
{
public:
									SRGBTestProgram			(Context& context, const FragmentShaderParameters& shaderParameters);
									~SRGBTestProgram		(void);

	void							setBlendRequired		(bool blendRequired);
	void							setToggleRequired		(bool toggleRequired);
	void							setUniformToggle		(int location, bool toggleDecodeValue);

	const std::vector<UniformData>&	getUniformDataList		(void) const;
	int								getUniformLocation		(const std::string& name);
	deUint32						getHandle				(void) const;
	bool							getBlendRequired		(void) const;

private:
	std::string						genFunctionCall			(ShaderSamplingType samplingType, const int uniformIdx);
	void							genFragmentShader		(void);

	Context&						m_context;
	de::MovePtr<glu::ShaderProgram>	m_program;
	FragmentShaderParameters		m_shaderFragmentParameters;
	std::string						m_shaderVertex;
	std::string						m_shaderFragment;
	std::vector<UniformData>		m_uniformDataList;
	bool							m_blendRequired;
	bool							m_toggleRequired;
};

SRGBTestProgram::SRGBTestProgram	(Context& context, const FragmentShaderParameters& shaderParameters)
	: m_context						(context)
	, m_shaderFragmentParameters	(shaderParameters)
	, m_blendRequired				(false)
	, m_toggleRequired				(false)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log					= m_context.getTestContext().getLog();
	glu::ShaderProgramInfo	buildInfo;
	const int				totalShaderStages	= 2;

	// default vertex shader used in all tests
	m_shaderVertex =	"#version 310 es \n"
						"layout (location = 0) in mediump vec3 aPosition; \n"
						"layout (location = 1) in mediump vec2 aTexCoord; \n"
						"out mediump vec2 vs_aTexCoord; \n"
						"void main () \n"
						"{ \n"
						"	gl_Position = vec4(aPosition, 1.0); \n"
						"	vs_aTexCoord = aTexCoord; \n"
						"} \n";

	this->genFragmentShader();

	m_program = de::MovePtr<glu::ShaderProgram>(new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(m_shaderVertex, m_shaderFragment)));

	if (!m_program->isOk())
	{
		TCU_FAIL("Failed to compile shaders and link program");
	}

	glw::GLint activeUniforms, maxLen;
	glw::GLint size, location;
	glw::GLenum type;

	gl.getProgramiv(this->getHandle(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
	gl.getProgramiv(this->getHandle(), GL_ACTIVE_UNIFORMS, &activeUniforms);

	std::vector<glw::GLchar> uniformName(static_cast<int>(maxLen));
	for (int idx = 0; idx < activeUniforms; idx++)
	{
		gl.getActiveUniform(this->getHandle(), idx, maxLen, NULL, &size, &type, &uniformName[0]);
		location = gl.getUniformLocation(this->getHandle(), &uniformName[0]);

		UniformData uniformData(location, std::string(&uniformName[0], strlen(&uniformName[0])));
		m_uniformDataList.push_back(uniformData);
	}

	// log shader program info. Only vertex and fragment shaders included
	buildInfo.program = m_program->getProgramInfo();
	for (int shaderIdx = 0; shaderIdx < totalShaderStages; shaderIdx++)
	{
		glu::ShaderInfo shaderInfo = m_program->getShaderInfo(static_cast<glu::ShaderType>(static_cast<int>(glu::SHADERTYPE_VERTEX) + static_cast<int>(shaderIdx)), 0);
		buildInfo.shaders.push_back(shaderInfo);
	}

	log << buildInfo;
}

SRGBTestProgram::~SRGBTestProgram (void)
{
	m_program	= de::MovePtr<glu::ShaderProgram>(DE_NULL);
}

void SRGBTestProgram::setBlendRequired (bool blendRequired)
{
	m_blendRequired = blendRequired;
}

void SRGBTestProgram::setToggleRequired (bool toggleRequired)
{
	m_toggleRequired = toggleRequired;
}

void SRGBTestProgram::setUniformToggle (int location, bool toggleDecodeValue)
{
	if ( (m_uniformDataList.empty() == false) && (location >= 0) && (location <= (int)m_uniformDataList.size()) )
	{
		m_uniformDataList[location].toggleDecode = toggleDecodeValue;
	}
	else
	{
		TCU_THROW(TestError, "Error: Uniform location not found. glGetActiveUniforms returned uniforms incorrectly ");
	}
}

const std::vector<UniformData>& SRGBTestProgram::getUniformDataList (void) const
{
	return m_uniformDataList;
}

int SRGBTestProgram::getUniformLocation (const std::string& name)
{
	for (std::size_t idx = 0; idx < m_uniformDataList.size(); idx++)
	{
		if (m_uniformDataList[idx].name == name)
		{
			return m_uniformDataList[idx].location;
		}
	}

	TCU_THROW(TestError, "Error: If name correctly requested then glGetActiveUniforms() returned active uniform data incorrectly");
	return -1;
}

glw::GLuint SRGBTestProgram::getHandle (void) const
{
	return m_program->getProgram();
}

bool SRGBTestProgram::getBlendRequired (void) const
{
	return m_blendRequired;
}

std::string SRGBTestProgram::genFunctionCall (ShaderSamplingType samplingType, const int uniformIdx)
{
	std::ostringstream functionCall;

	functionCall << "	mediump vec4 texelColor" << uniformIdx << " = ";

	switch (samplingType)
		{
			case TEXTURESAMPLING_TEXTURE:
			{
				functionCall << "texture(uTexture" << uniformIdx << ", vs_aTexCoord); \n";
				break;
			}
			case TEXTURESAMPLING_TEXTURE_LOD:
			{
				functionCall << "textureLod(uTexture" << uniformIdx << ", vs_aTexCoord, 0.0); \n";
				break;
			}
			case TEXTURESAMPLING_TEXTURE_GRAD:
			{
				functionCall << "textureGrad(uTexture" << uniformIdx << ", vs_aTexCoord, vec2(0.0, 0.0), vec2(0.0, 0.0)); \n";
				break;
			}
			case TEXTURESAMPLING_TEXTURE_OFFSET:
			{
				functionCall << "textureOffset(uTexture" << uniformIdx << ", vs_aTexCoord, ivec2(0.0, 0.0)); \n";
				break;
			}
			case TEXTURESAMPLING_TEXTURE_PROJ:
			{
				functionCall << "textureProj(uTexture" << uniformIdx << ", vec3(vs_aTexCoord, 1.0)); \n";
				break;
			}
			case TEXTURESAMPLING_TEXELFETCH:
			{
				functionCall << "texelFetch(uTexture" << uniformIdx << ", ivec2(vs_aTexCoord), 0); \n";
				break;
			}
			case TEXTURESAMPLING_TEXELFETCH_OFFSET:
			{
				functionCall << "texelFetchOffset(uTexture" << uniformIdx << ", ivec2(vs_aTexCoord), 0, ivec2(0.0, 0.0)); \n";
				break;
			}
			default:
			{
				DE_FATAL("Error: Sampling type not recognised");
			}
		}

	return functionCall.str();
}

void SRGBTestProgram::genFragmentShader (void)
{
	std::ostringstream source;
	std::ostringstream sampleTexture;
	std::ostringstream functionParameters;
	std::ostringstream shaderOutputs;

	// if comparison function is present resulting shader requires precisely one output
	DE_ASSERT( !(m_shaderFragmentParameters.hasFunction && (static_cast<int>(m_shaderFragmentParameters.outputTotal) != static_cast<int>(SHADEROUTPUTS_ONE))) );

	// function parameters must equal the number of uniforms i.e. textures passed into the function
	DE_ASSERT( !(m_shaderFragmentParameters.hasFunction && (static_cast<int>(m_shaderFragmentParameters.uniformTotal) != static_cast<int>(m_shaderFragmentParameters.functionParameters))) );

	// fragment shader cannot contain more outputs than the number of texture uniforms
	DE_ASSERT( !(static_cast<int>(m_shaderFragmentParameters.outputTotal) > static_cast<int>(m_shaderFragmentParameters.uniformTotal)) ) ;

	source << "#version 310 es \n"
		<< "in mediump vec2 vs_aTexCoord; \n";

	for (int output = 0; output < m_shaderFragmentParameters.outputTotal; output++)
	{
		source << "layout (location = " << output << ") out mediump vec4 fs_aColor" << output << "; \n";
	}

	for (int uniform = 0; uniform < m_shaderFragmentParameters.uniformTotal; uniform++)
	{
		source << "uniform sampler2D uTexture" << uniform << "; \n";
	}

	if (m_shaderFragmentParameters.hasFunction == true)
	{
		source << m_shaderFragmentParameters.functionImplementation;
	}

	source << "void main () \n"
		<< "{ \n";

	for (int uniformIdx = 0; uniformIdx < m_shaderFragmentParameters.uniformTotal; uniformIdx++)
	{
		source << this->genFunctionCall(m_shaderFragmentParameters.samplingType, uniformIdx);
	}

	if (m_shaderFragmentParameters.hasFunction == true)
	{
		switch ( static_cast<FunctionParameters>(m_shaderFragmentParameters.functionParameters) )
		{
			case FUNCTIONPARAMETERS_ONE:
			{
				functionParameters << "(texelColor0)";
				break;
			}
			case FUNCTIONPARAMETERS_TWO:
			{
				functionParameters << "(texelColor0, texelColor1)";
				break;
			}
			default:
			{
				DE_FATAL("Error: Number of comparison function parameters invalid");
			}
		}

		shaderOutputs << "	fs_aColor0 = " << m_shaderFragmentParameters.functionName << functionParameters.str() << "; \n";
	}
	else
	{
		for (int output = 0; output < m_shaderFragmentParameters.outputTotal; output++)
		{
			shaderOutputs << "	fs_aColor" << output << " = texelColor" << output << "; \n";
		}
	}

	source << shaderOutputs.str();
	source << "} \n";

	m_shaderFragment = source.str();
}

class SRGBTestCase : public TestCase
{
public:
							SRGBTestCase					(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat);
							~SRGBTestCase					(void);

	void					init							(void);
	void					deinit							(void);
	virtual IterateResult	iterate							(void);

	void					setSamplingGroup				(const ShaderSamplingGroup samplingGroup);
	void					setSamplingLocations			(const int px, const int py);
	void					setUniformToggle				(const int programIdx, const std::string& uniformName, bool toggleDecode);

	void					addTexture						(const glu::TextureTestUtil::TextureType	targetType,
															 const int									width,
															 const int									height,
															 const tcu::Vec4							color,
															 const tcu::Sampler::WrapMode				wrapS,
															 const tcu::Sampler::WrapMode				wrapT,
															 const tcu::Sampler::FilterMode				minFilter,
															 const tcu::Sampler::FilterMode				magFilter,
															 const SRGBDecode							decoding);
	void					addSampler						(const tcu::Sampler::WrapMode	wrapS,
															 const tcu::Sampler::WrapMode	wrapT,
															 const tcu::Sampler::FilterMode	minFilter,
															 const tcu::Sampler::FilterMode	magFilter,
															 const SRGBDecode				decoding);
	void					addShaderProgram				(const FragmentShaderParameters& shaderParameters);

	void					genShaderPrograms				(ShaderSamplingType samplingType);
	void					deleteShaderPrograms			(void);

	void					readResultTextures				(void);
	void					storeResultPixels				(std::vector<tcu::Vec4>& resultPixelData);

	void					toggleDecode					(const std::vector<UniformData>& uniformDataList);
	void					bindSamplerToTexture			(const int samplerIdx, const int textureIdx, const deUint32 textureUnit);
	void					activateSampler					(const int samplerIdx, const bool active);
	void					logColor						(const std::string& colorLogMessage, int colorIdx, tcu::Vec4 color) const;
	tcu::Vec4				formatReferenceColor			(tcu::Vec4 referenceColor);

	// render function has a default implentation. Can be overriden for special cases
	virtual void			render							(void);

	// following functions must be overidden to perform individual test cases
	virtual void			setupTest						(void) = 0;
	virtual bool			verifyResult					(void) = 0;

protected:
	de::MovePtr<glu::Framebuffer>			m_framebuffer;
	std::vector<SRGBTestTexture*>			m_textureSourceList;
	std::vector<SRGBTestSampler*>			m_samplerList;
	std::vector<glw::GLuint>				m_renderBufferList;
	const tcu::Vec4							m_epsilonError;
	std::vector<tcu::TextureLevel>			m_textureResultList;
	int										m_resultOutputTotal;
	tcu::TextureFormat						m_resultTextureFormat;
	glw::GLuint								m_vaoID;
	glw::GLuint								m_vertexDataID;
	std::vector<FragmentShaderParameters>	m_shaderParametersList;
	std::vector<SRGBTestProgram*>			m_shaderProgramList;
	ShaderSamplingGroup						m_samplingGroup;
	int										m_px;
	int										m_py;
	const tcu::TextureFormat				m_internalFormat;

private:
	void			uploadTextures	(void);
	void			initFrameBuffer	(void);
	void			initVertexData	(void);

					SRGBTestCase	(const SRGBTestCase&);
	SRGBTestCase&	operator=		(const SRGBTestCase&);
};

SRGBTestCase::SRGBTestCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
	: TestCase				(context, name, description)
	, m_epsilonError		(EpsilonError::CPU)
	, m_resultTextureFormat	(tcu::TextureFormat(tcu::TextureFormat::sRGBA, tcu::TextureFormat::UNORM_INT8))
	, m_vaoID				(0)
	, m_vertexDataID		(0)
	, m_samplingGroup		(SHADERSAMPLINGGROUP_TEXTURE)
	, m_internalFormat		(internalFormat)
{
}

SRGBTestCase::~SRGBTestCase (void)
{
	deinit();
}

void SRGBTestCase::init (void)
{
	// extension requirements for test
	if ( (glu::getInternalFormat(m_internalFormat) == GL_SRGB8_ALPHA8) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_sRGB_decode") )
	{
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_sRGB_decode extension");
	}

	if ( (glu::getInternalFormat(m_internalFormat) == GL_SR8_EXT) && !(m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_sRGB_R8")) )
	{
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_sRGB_R8 extension");
	}

	m_framebuffer = de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(m_context.getRenderContext()));
}

void SRGBTestCase::deinit (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_framebuffer	= de::MovePtr<glu::Framebuffer>(DE_NULL);

	for (std::size_t renderBufferIdx = 0; renderBufferIdx < m_renderBufferList.size(); renderBufferIdx++)
	{
		gl.deleteRenderbuffers(1, &m_renderBufferList[renderBufferIdx]);
	}
	m_renderBufferList.clear();

	for (std::size_t textureSourceIdx = 0; textureSourceIdx < m_textureSourceList.size(); textureSourceIdx++)
	{
		delete m_textureSourceList[textureSourceIdx];
	}
	m_textureSourceList.clear();

	for (std::size_t samplerIdx = 0; samplerIdx < m_samplerList.size(); samplerIdx++)
	{
		delete m_samplerList[samplerIdx];
	}
	m_samplerList.clear();

	if (m_vaoID != 0)
	{
		gl.deleteVertexArrays(1, &m_vaoID);
		m_vaoID = 0;
	}

	if (m_vertexDataID != 0)
	{
		gl.deleteBuffers(1, &m_vertexDataID);
		m_vertexDataID = 0;
	}
}

SRGBTestCase::IterateResult SRGBTestCase::iterate (void)
{
	bool	result;
	int		startIdx	= -1;
	int		endIdx		= -1;

	this->setupTest();

	if (m_samplingGroup == SHADERSAMPLINGGROUP_TEXTURE)
	{
		startIdx	= static_cast<int>(TEXTURESAMPLING_TEXTURE_START);
		endIdx		= static_cast<int>(TEXTURESAMPLING_TEXTURE_END);
	}
	else if (m_samplingGroup == SHADERSAMPLINGGROUP_TEXEL_FETCH)
	{
		startIdx	= static_cast<int>(TEXTURESAMPLING_TEXELFETCH_START);
		endIdx		= static_cast<int>(TEXTURESAMPLING_TEXELFETCH_END);
	}
	else
	{
		DE_FATAL("Error: Sampling group not defined");
	}

	this->initVertexData();
	this->initFrameBuffer();

	// loop through all sampling types in the required sampling group, performing individual tests for each
	for (int samplingTypeIdx = startIdx; samplingTypeIdx < endIdx; samplingTypeIdx++)
	{
		this->genShaderPrograms(static_cast<ShaderSamplingType>(samplingTypeIdx));
		this->uploadTextures();
		this->render();

		result = this->verifyResult();

		this->deleteShaderPrograms();

		if (result == true)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result verification failed");
			return STOP;
		}
	}

	return STOP;
}

void SRGBTestCase::setSamplingGroup (const ShaderSamplingGroup samplingGroup)
{
	m_samplingGroup = samplingGroup;
}

void SRGBTestCase::setSamplingLocations (const int px, const int py)
{
	m_px = px;
	m_py = py;
}

void SRGBTestCase::addTexture (	const glu::TextureTestUtil::TextureType	targetType,
								const int								width,
								const int								height,
								const tcu::Vec4							color,
								const tcu::Sampler::WrapMode			wrapS,
								const tcu::Sampler::WrapMode			wrapT,
								const tcu::Sampler::FilterMode			minFilter,
								const tcu::Sampler::FilterMode			magFilter,
								const SRGBDecode						decoding)
{
	SRGBTestTexture* texture = new SRGBTestTexture(m_context, targetType, m_internalFormat, width, height, color, wrapS, wrapT, minFilter, magFilter, decoding);
	m_textureSourceList.push_back(texture);
}

void SRGBTestCase::addSampler (	const tcu::Sampler::WrapMode	wrapS,
								const tcu::Sampler::WrapMode	wrapT,
								const tcu::Sampler::FilterMode	minFilter,
								const tcu::Sampler::FilterMode	magFilter,
								const SRGBDecode				decoding)
{
	SRGBTestSampler *sampler = new SRGBTestSampler(m_context, wrapS, wrapT, minFilter, magFilter, decoding);
	m_samplerList.push_back(sampler);
}

void SRGBTestCase::addShaderProgram (const FragmentShaderParameters& shaderParameters)
{
	m_shaderParametersList.push_back(shaderParameters);
	m_resultOutputTotal = shaderParameters.outputTotal;
}

void SRGBTestCase::genShaderPrograms (ShaderSamplingType samplingType)
{
	for (int shaderParamsIdx = 0; shaderParamsIdx < (int)m_shaderParametersList.size(); shaderParamsIdx++)
	{
		m_shaderParametersList[shaderParamsIdx].samplingType = samplingType;
		SRGBTestProgram* shaderProgram = new SRGBTestProgram(m_context, m_shaderParametersList[shaderParamsIdx]);

		if (m_shaderParametersList[shaderParamsIdx].blendRequired == BLENDING_REQUIRED)
		{
			shaderProgram->setBlendRequired(true);
		}

		if (m_shaderParametersList[shaderParamsIdx].toggleRequired == TOGGLING_REQUIRED)
		{
			shaderProgram->setToggleRequired(true);
			std::vector<std::string> uniformsToToggle = m_shaderParametersList[shaderParamsIdx].uniformsToToggle;

			for (int uniformNameIdx = 0; uniformNameIdx < (int)uniformsToToggle.size(); uniformNameIdx++)
			{
				shaderProgram->setUniformToggle(shaderProgram->getUniformLocation(uniformsToToggle[uniformNameIdx]), true);
			}
		}

		m_shaderProgramList.push_back(shaderProgram);
	}
}

void SRGBTestCase::deleteShaderPrograms (void)
{
	for (std::size_t idx = 0; idx < m_shaderProgramList.size(); idx++)
	{
		delete m_shaderProgramList[idx];
	}
	m_shaderProgramList.clear();
}

void SRGBTestCase::readResultTextures (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	int						width	= m_context.getRenderContext().getRenderTarget().getWidth();
	int						height	= m_context.getRenderContext().getRenderTarget().getHeight();

	gl.bindFramebuffer(GL_FRAMEBUFFER, **m_framebuffer);

	m_textureResultList.resize(m_renderBufferList.size());

	for (std::size_t renderBufferIdx = 0; renderBufferIdx < m_renderBufferList.size(); renderBufferIdx++)
	{
		gl.readBuffer(GL_COLOR_ATTACHMENT0 + (glw::GLenum)renderBufferIdx);
		m_textureResultList[renderBufferIdx].setStorage(m_resultTextureFormat, width, height);
		glu::readPixels(m_context.getRenderContext(), 0, 0, m_textureResultList[renderBufferIdx].getAccess());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels()");
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SRGBTestCase::storeResultPixels (std::vector<tcu::Vec4>& resultPixelData)
{
	tcu::TestLog&		log			= m_context.getTestContext().getLog();
	std::ostringstream	message;
	int					width		= m_context.getRenderContext().getRenderTarget().getWidth();
	int					height		= m_context.getRenderContext().getRenderTarget().getHeight();

	// ensure result sampling coordinates are within range of the result color attachment
	DE_ASSERT((m_px >= 0) && (m_px < width));
	DE_ASSERT((m_py >= 0) && (m_py < height));
	DE_UNREF(width && height);

	for (int idx = 0; idx < (int)m_textureResultList.size(); idx++)
	{
		resultPixelData.push_back(m_textureResultList[idx].getAccess().getPixel(m_px, m_py));
		this->logColor(std::string("Result color: "), idx, resultPixelData[idx]);
	}

	// log error rate (threshold)
	message << m_epsilonError;
	log << tcu::TestLog::Message << std::string("Epsilon error: ") << message.str() << tcu::TestLog::EndMessage;
}

void SRGBTestCase::toggleDecode (const std::vector<UniformData>& uniformDataList)
{
	DE_ASSERT( uniformDataList.size() <= m_textureSourceList.size() );

	for (int uniformIdx = 0; uniformIdx < (int)uniformDataList.size(); uniformIdx++)
	{
		if (uniformDataList[uniformIdx].toggleDecode == true)
		{
			if (m_textureSourceList[uniformIdx]->getDecode() == SRGBDECODE_DECODE_DEFAULT)
			{
				// cannot toggle default
				continue;
			}

			// toggle sRGB decode values (ignoring value if set to default)
			m_textureSourceList[uniformIdx]->setDecode((SRGBDecode)((m_textureSourceList[uniformIdx]->getDecode() + 1) % SRGBDECODE_DECODE_DEFAULT));
		}
	}
}

void SRGBTestCase::bindSamplerToTexture (const int samplerIdx, const int textureIdx, const deUint32 textureUnit)
{
	deUint32 enumConversion = textureUnit - GL_TEXTURE0;
	m_textureSourceList[textureIdx]->setHasSampler(true);
	m_samplerList[samplerIdx]->setTextureUnit(enumConversion);
}

void SRGBTestCase::activateSampler (const int samplerIdx, const bool active)
{
	m_samplerList[samplerIdx]->setIsActive(active);
}

void SRGBTestCase::logColor (const std::string& colorLogMessage, int colorIdx, tcu::Vec4 color) const
{
	tcu::TestLog&			log		= m_context.getTestContext().getLog();
	std::ostringstream		message;

	message << colorLogMessage << colorIdx << " = (" << color.x() << ", " << color.y() << ", " << color.z() << ", " << color.w() << ")";
	log << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;
}

tcu::Vec4 SRGBTestCase::formatReferenceColor (tcu::Vec4 referenceColor)
{
	switch (glu::getInternalFormat(m_internalFormat))
	{
		case GL_SRGB8_ALPHA8:
		{
			return referenceColor;
		}
		case GL_SR8_EXT:
		{
			// zero unwanted color channels
			referenceColor.y() = 0;
			referenceColor.z() = 0;
			return referenceColor;
		}
		default:
		{
			DE_FATAL("Error: Internal format not recognised");
			return referenceColor;
		}
	}
}

void SRGBTestCase::render (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// default rendering only uses one program
	gl.bindFramebuffer(GL_FRAMEBUFFER, **m_framebuffer);
	gl.bindVertexArray(m_vaoID);

	gl.useProgram(m_shaderProgramList[0]->getHandle());

	for (int textureSourceIdx = 0; textureSourceIdx < (int)m_textureSourceList.size(); textureSourceIdx++)
	{
		gl.activeTexture(GL_TEXTURE0 + (glw::GLenum)textureSourceIdx);
		gl.bindTexture(m_textureSourceList[textureSourceIdx]->getGLTargetType(), m_textureSourceList[textureSourceIdx]->getHandle());
		glw::GLuint samplerUniformLocationID = gl.getUniformLocation(m_shaderProgramList[0]->getHandle(), (std::string("uTexture") + de::toString(textureSourceIdx)).c_str());
		TCU_CHECK(samplerUniformLocationID != (glw::GLuint)-1);
		gl.uniform1i(samplerUniformLocationID, (glw::GLenum)textureSourceIdx);
	}

	for (int samplerIdx = 0; samplerIdx < (int)m_samplerList.size(); samplerIdx++)
	{
		if (m_samplerList[samplerIdx]->getIsActive() == true)
		{
			m_samplerList[samplerIdx]->bindToTexture();
		}
	}

	gl.drawArrays(GL_TRIANGLES, 0, 6);

	for (std::size_t textureSourceIdx = 0; textureSourceIdx < m_textureSourceList.size(); textureSourceIdx++)
	{
		gl.bindTexture(m_textureSourceList[textureSourceIdx]->getGLTargetType(), 0);
	}
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
}

void SRGBTestCase::uploadTextures (void)
{
	for (std::size_t idx = 0; idx < m_textureSourceList.size(); idx++)
	{
		m_textureSourceList[idx]->upload();
		m_textureSourceList[idx]->setParameters();
	}
}

void SRGBTestCase::initFrameBuffer (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	int						width	= m_context.getRenderContext().getRenderTarget().getWidth();
	int						height	= m_context.getRenderContext().getRenderTarget().getHeight();

	if (m_resultOutputTotal == 0)
	{
		throw std::invalid_argument("SRGBTestExecutor must have at least 1 rendered result");
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, **m_framebuffer);

	DE_ASSERT(m_renderBufferList.empty());
	for (int outputIdx = 0; outputIdx < m_resultOutputTotal; outputIdx++)
	{
		glw::GLuint renderBuffer = -1;
		m_renderBufferList.push_back(renderBuffer);
	}

	for (std::size_t renderBufferIdx = 0; renderBufferIdx < m_renderBufferList.size(); renderBufferIdx++)
	{
		gl.genRenderbuffers(1, &m_renderBufferList[renderBufferIdx]);
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderBufferList[renderBufferIdx]);
		gl.renderbufferStorage(GL_RENDERBUFFER, glu::getInternalFormat(m_resultTextureFormat), width, height);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (glw::GLenum)renderBufferIdx, GL_RENDERBUFFER, m_renderBufferList[renderBufferIdx]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Create and setup renderbuffer object");
	}
	TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	std::vector<glw::GLenum> renderBufferTargets(m_renderBufferList.size());
	for (std::size_t renderBufferIdx = 0; renderBufferIdx < m_renderBufferList.size(); renderBufferIdx++)
	{
		renderBufferTargets[renderBufferIdx] = GL_COLOR_ATTACHMENT0 + (glw::GLenum)renderBufferIdx;
	}
	gl.drawBuffers((glw::GLsizei)renderBufferTargets.size(), &renderBufferTargets[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawBuffer()");

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SRGBTestCase::initVertexData (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();

	static const glw::GLfloat squareVertexData[] =
	{
		// position				// texcoord
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f, // bottom left corner
		 1.0f, -1.0f, 0.0f,		1.0f, 0.0f, // bottom right corner
		 1.0f,  1.0f, 0.0f,		1.0f, 1.0f, // Top right corner

		-1.0f,  1.0f, 0.0f,		0.0f, 1.0f, // top left corner
		 1.0f,  1.0f, 0.0f,		1.0f, 1.0f, // Top right corner
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f  // bottom left corner
	};

	DE_ASSERT(m_vaoID == 0);
	gl.genVertexArrays(1, &m_vaoID);
	gl.bindVertexArray(m_vaoID);

	gl.genBuffers(1, &m_vertexDataID);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertexDataID);
	gl.bufferData(GL_ARRAY_BUFFER, (glw::GLsizei)sizeof(squareVertexData), squareVertexData, GL_STATIC_DRAW);

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * (glw::GLsizei)sizeof(GL_FLOAT), (glw::GLvoid *)0);
	gl.enableVertexAttribArray(0);
	gl.vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * (glw::GLsizei)sizeof(GL_FLOAT), (glw::GLvoid *)(3 * sizeof(GL_FLOAT)));
	gl.enableVertexAttribArray(1);

	gl.bindVertexArray(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
}

class TextureDecodeSkippedCase : public SRGBTestCase
{
public:
			TextureDecodeSkippedCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase			(context, name, description, internalFormat) {}

			~TextureDecodeSkippedCase	(void) {}

	void	setupTest					(void);
	bool	verifyResult				(void);
};

void TextureDecodeSkippedCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture to DECODE_SKIP_EXT
	//	- store texture on GPU
	//	- in fragment shader, sample the texture using texture*() and render texel values to a color attachment in the FBO
	//	- on the host, read back the pixel values into a tcu::TextureLevel
	//	- analyse the texel values, expecting them in sRGB format i.e. linear space decoding was skipped

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_ONE, SHADERUNIFORMS_ONE, NULL, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addShaderProgram(shaderParameters);
	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);

	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool TextureDecodeSkippedCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;
	tcu::Vec4				pixelConverted;
	tcu::Vec4				pixelReference;
	tcu::Vec4				pixelExpected;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);

	pixelConverted = tcu::sRGBToLinear(pixelResultList[resultColorIdx]);
	pixelReference = this->formatReferenceColor(getColorReferenceLinear());
	pixelExpected = this->formatReferenceColor(getColorReferenceSRGB());

	this->formatReferenceColor(pixelReference);
	this->logColor(std::string("Expected color: "), resultColorIdx, pixelExpected);

	// result color 0 should be sRGB. Compare with linear reference color
	if ( (tcu::boolAll(tcu::lessThan(tcu::abs(pixelConverted - pixelReference), m_epsilonError))) || (tcu::boolAll(tcu::equal(pixelConverted, pixelReference))) )
	{
		log << tcu::TestLog::Message << std::string("sRGB as expected") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("not sRGB as expected") << tcu::TestLog::EndMessage;
		return false;
	}
}

class TextureDecodeEnabledCase : public SRGBTestCase
{
public:
		TextureDecodeEnabledCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
			: SRGBTestCase			(context, name, description, internalFormat) {}

		~TextureDecodeEnabledCase	(void) {}

		void	setupTest			(void);
		bool	verifyResult		(void);
};

void TextureDecodeEnabledCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture to DECODE_EXT
	//	- store texture on GPU
	//	- in fragment shader, sample the texture using texture*() and render texel values to a color attachment in the FBO
	//	- on the host, read back the pixel values into a tcu::TextureLevel
	//	- analyse the texel values, expecting them in lRGB format i.e. linear space decoding was enabled

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_ONE, SHADERUNIFORMS_ONE, NULL, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_DECODE);

	this->addShaderProgram(shaderParameters);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);

	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool TextureDecodeEnabledCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;
	tcu::Vec4				pixelConverted;
	tcu::Vec4				pixelReference;
	tcu::Vec4				pixelExpected;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);

	pixelConverted = tcu::linearToSRGB(pixelResultList[resultColorIdx]);
	pixelReference = this->formatReferenceColor(getColorReferenceSRGB());
	pixelExpected = this->formatReferenceColor(getColorReferenceLinear());

	this->logColor(std::string("Expected color: "), resultColorIdx, pixelExpected);

	// result color 0 should be SRGB. Compare with sRGB reference color
	if ( (tcu::boolAll(tcu::lessThan(tcu::abs(pixelConverted - pixelReference), m_epsilonError))) || (tcu::boolAll(tcu::equal(pixelConverted, pixelReference))) )
	{
		log << tcu::TestLog::Message << std::string("linear as expected") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("not linear as expected") << tcu::TestLog::EndMessage;
		return false;
	}
}

class TexelFetchDecodeSkippedcase : public SRGBTestCase
{
public:
			TexelFetchDecodeSkippedcase		(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase				(context, name, description, internalFormat) {}

			~TexelFetchDecodeSkippedcase	(void) {}

	void	setupTest						(void);
	bool	verifyResult					(void);
};

void TexelFetchDecodeSkippedcase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture to DECODE_SKIP_EXT
	//	- store texture on GPU
	//	- in fragment shader, sample the texture using texelFetch*() and render texel values to a color attachment in the FBO
	//	- on the host, read back the pixel values into a tcu::TextureLevel
	//	- analyse the texel values, expecting them in lRGB format i.e. linear space decoding is always enabled with texelFetch*()

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_ONE, SHADERUNIFORMS_ONE, NULL, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addShaderProgram(shaderParameters);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);

	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXEL_FETCH);
}

bool TexelFetchDecodeSkippedcase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;
	tcu::Vec4				pixelReference;
	tcu::Vec4				pixelExpected;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);

	pixelReference = pixelExpected = this->formatReferenceColor(getColorReferenceLinear());

	this->logColor(std::string("Expected color: "), resultColorIdx, pixelExpected);

	// result color 0 should be linear due to automatic conversion via texelFetch*(). Compare with linear reference color
	if ( (tcu::boolAll(tcu::lessThan(tcu::abs(pixelResultList[0] - pixelReference), m_epsilonError))) || (tcu::boolAll(tcu::equal(pixelResultList[0], pixelReference))) )
	{
		log << tcu::TestLog::Message << std::string("linear as expected") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("not linear as expected") << tcu::TestLog::EndMessage;
		return false;
	}
}

class GPUConversionDecodeEnabledCase : public SRGBTestCase
{
public:
			GPUConversionDecodeEnabledCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase				(context, name, description, internalFormat) {}

			~GPUConversionDecodeEnabledCase	(void) {}

	void	setupTest						(void);
	bool	verifyResult					(void);
};

void GPUConversionDecodeEnabledCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture_a to DECODE_SKIP_EXT and texture_b to default
	//	- store textures on GPU
	//	- in fragment shader, sample both textures using texture*() and manually perform sRGB to lRGB conversion on texture_b
	//	- in fragment shader, compare converted texture_b with texture_a
	//	- render green image for pass or red for fail

	ComparisonFunction comparisonFunction("srgbToLinearCheck", FUNCTIONPARAMETERS_TWO, getFunctionDefinitionSRGBToLinearCheck());

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_ONE, SHADERUNIFORMS_TWO, &comparisonFunction, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_DECODE_DEFAULT);

	this->addShaderProgram(shaderParameters);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);

	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool GPUConversionDecodeEnabledCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);
	this->logColor(std::string("Expected color: "), resultColorIdx, getColorGreenPass());

	// result color returned from GPU is either green (pass) or fail (red)
	if ( tcu::boolAll(tcu::equal(pixelResultList[resultColorIdx], getColorGreenPass())) )
	{
		log << tcu::TestLog::Message << std::string("returned pass color from GPU") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("returned fail color from GPU") << tcu::TestLog::EndMessage;
		return false;
	}
}

class DecodeToggledCase : public SRGBTestCase
{
public:
			DecodeToggledCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase	(context, name, description, internalFormat) {}

			~DecodeToggledCase	(void) {}

	void	render				(void);
	void	setupTest			(void);
	bool	verifyResult		(void);
};

void DecodeToggledCase::render (void)
{
	// override the base SRGBTestCase render function with the purpose of switching between shader programs,
	// toggling texture sRGB decode state between draw calls
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, **m_framebuffer);
	gl.bindVertexArray(m_vaoID);

	for (std::size_t programIdx = 0; programIdx < m_shaderProgramList.size(); programIdx++)
	{
		gl.useProgram(m_shaderProgramList[programIdx]->getHandle());

		this->toggleDecode(m_shaderProgramList[programIdx]->getUniformDataList());

		for (int textureSourceIdx = 0; textureSourceIdx < (int)m_textureSourceList.size(); textureSourceIdx++)
		{
			gl.activeTexture(GL_TEXTURE0 + (glw::GLenum)textureSourceIdx);
			gl.bindTexture(m_textureSourceList[textureSourceIdx]->getGLTargetType(), m_textureSourceList[textureSourceIdx]->getHandle());
			glw::GLuint samplerUniformLocationID = gl.getUniformLocation(m_shaderProgramList[programIdx]->getHandle(), (std::string("uTexture") + de::toString(textureSourceIdx)).c_str());
			TCU_CHECK(samplerUniformLocationID != (glw::GLuint) - 1);
			gl.uniform1i(samplerUniformLocationID, (glw::GLenum)textureSourceIdx);
		}

		for (int samplerIdx = 0; samplerIdx < (int)m_samplerList.size(); samplerIdx++)
		{
			if (m_samplerList[samplerIdx]->getIsActive() == true)
			{
				m_samplerList[samplerIdx]->bindToTexture();
			}
		}

		if (m_shaderProgramList[programIdx]->getBlendRequired() == true)
		{
			gl.enable(GL_BLEND);
			gl.blendEquation(GL_MAX);
			gl.blendFunc(GL_ONE, GL_ONE);
		}
		else
		{
			gl.disable(GL_BLEND);
		}

		gl.drawArrays(GL_TRIANGLES, 0, 6);

		// reset sRGB decode state on textures
		this->toggleDecode(m_shaderProgramList[programIdx]->getUniformDataList());
	}

	for (std::size_t textureSourceIdx = 0; textureSourceIdx < m_textureSourceList.size(); textureSourceIdx++)
	{
		gl.bindTexture(m_textureSourceList[textureSourceIdx]->getGLTargetType(), 0);
	}
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
}

void DecodeToggledCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture_a to DECODE_SKIP_EXT and texture_b to DECODE_EXT
	//	- create and use two seperate shader programs, program_a and program_b, each using different fragment shaders
	//	- store texture_a and texture_b on GPU
	// FIRST PASS:
	//	- use program_a
	//	- in fragment shader, sample both textures using texture*() and manually perform sRGB to lRGB conversion on texture_a
	//	- in fragment shader, test converted texture_a value with texture_b
	//	- render green image for pass or red for fail
	//	- store result in a color attachement 0
	// TOGGLE STAGE
	//	- during rendering, toggle texture_a from DECODE_SKIP_EXT to DECODE_EXT
	// SECOND PASS:
	//	- use program_b
	//	- in fragment shader, sample both textures using texture*() and manually perform equality check. Both should be linear
	//	- blend first pass result with second pass. Anything but a green result equals fail

	ComparisonFunction srgbToLinearFunction("srgbToLinearCheck", FUNCTIONPARAMETERS_TWO, getFunctionDefinitionSRGBToLinearCheck());
	ComparisonFunction colorsEqualFunction("colorsEqualCheck", FUNCTIONPARAMETERS_TWO, getFunctionDefinitionEqualCheck());

	FragmentShaderParameters shaderParametersA(SHADEROUTPUTS_ONE, SHADERUNIFORMS_TWO, &srgbToLinearFunction, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);
	FragmentShaderParameters shaderParametersB(SHADEROUTPUTS_ONE, SHADERUNIFORMS_TWO, &colorsEqualFunction, BLENDING_REQUIRED, TOGGLING_REQUIRED);

	// need to specify which texture uniform to toggle DECODE_EXT/SKIP_DECODE_EXT
	shaderParametersB.uniformsToToggle.push_back("uTexture0");

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_DECODE);

	this->addShaderProgram(shaderParametersA);
	this->addShaderProgram(shaderParametersB);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);
	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool DecodeToggledCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);
	this->logColor(std::string("Expected color: "), resultColorIdx, getColorGreenPass());

	//	result color is either green (pass) or fail (red)
	if ( tcu::boolAll(tcu::equal(pixelResultList[resultColorIdx], getColorGreenPass())) )
	{
		log << tcu::TestLog::Message << std::string("returned pass color from GPU") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("returned fail color from GPU") << tcu::TestLog::EndMessage;
		return false;
	}
}

class DecodeMultipleTexturesCase : public SRGBTestCase
{
public:
			DecodeMultipleTexturesCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase			(context, name, description, internalFormat) {}

			~DecodeMultipleTexturesCase	(void) {}

	void	setupTest					(void);
	bool	verifyResult				(void);
};

void DecodeMultipleTexturesCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture_a to DECODE_SKIP_EXT and texture_b to DECODE_EXT
	//	- upload textures to the GPU and bind to seperate uniform variables
	//	- sample both textures using texture*()
	//	- read texel values back to the CPU
	//	- compare the texel values, both should be different from each other

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_TWO, SHADERUNIFORMS_TWO, NULL,  BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_DECODE);

	this->addShaderProgram(shaderParameters);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);
	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool DecodeMultipleTexturesCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;
	tcu::Vec4				pixelExpected0;
	tcu::Vec4				pixelExpected1;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);

	pixelExpected0 = this->formatReferenceColor(getColorReferenceSRGB());
	pixelExpected1 = this->formatReferenceColor(getColorReferenceLinear());

	this->logColor(std::string("Expected color: "), resultColorIdx, pixelExpected0);
	this->logColor(std::string("Expected color: "), resultColorIdx +1, pixelExpected1);

	//	check if the two textures have different values i.e. uTexture0 = sRGB and uTexture1 = linear
	if ( !(tcu::boolAll(tcu::equal(pixelResultList[resultColorIdx], pixelResultList[resultColorIdx +1]))) )
	{
		log << tcu::TestLog::Message << std::string("texel values are different") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("texel values are equal") << tcu::TestLog::EndMessage;
		return false;
	}
}

class DecodeSamplerCase : public SRGBTestCase
{
public:
			DecodeSamplerCase	(Context& context, const char* name, const char* description, const tcu::TextureFormat internalFormat)
				: SRGBTestCase	(context, name, description, internalFormat) {}

			~DecodeSamplerCase	(void) {}

	void	setupTest			(void);
	bool	verifyResult		(void);
};

void DecodeSamplerCase::setupTest (void)
{
	// TEST STEPS:
	//	- create and set texture_a to DECODE_SKIP_EXT
	//	- upload texture to the GPU and bind to sampler
	//	- sample texture using texture*()
	//	- read texel values back to the CPU
	//	- compare the texel values, should be in sampler format (linear)

	FragmentShaderParameters shaderParameters(SHADEROUTPUTS_ONE, SHADERUNIFORMS_ONE, NULL, BLENDING_NOT_REQUIRED, TOGGLING_NOT_REQUIRED);

	this->addTexture(	TEXTURETYPE_2D,
						TestDimensions::WIDTH,
						TestDimensions::HEIGHT,
						getColorReferenceLinear(),
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_SKIP_DECODE);

	this->addSampler(	tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::MIRRORED_REPEAT_GL,
						tcu::Sampler::LINEAR,
						tcu::Sampler::LINEAR,
						SRGBDECODE_DECODE);

	this->addShaderProgram(shaderParameters);

	this->bindSamplerToTexture(0, 0, GL_TEXTURE0);
	this->activateSampler(0, true);

	this->setSamplingLocations(TestSamplingPositions::X_POS, TestSamplingPositions::Y_POS);
	this->setSamplingGroup(SHADERSAMPLINGGROUP_TEXTURE);
}

bool DecodeSamplerCase::verifyResult (void)
{
	tcu::TestLog&			log				= m_context.getTestContext().getLog();
	const int				resultColorIdx	= 0;
	std::vector<tcu::Vec4>	pixelResultList;
	tcu::Vec4				pixelConverted;
	tcu::Vec4				pixelReference;
	tcu::Vec4				pixelExpected;

	this->readResultTextures();
	this->storeResultPixels(pixelResultList);

	pixelConverted = tcu::linearToSRGB(pixelResultList[resultColorIdx]);
	pixelReference = this->formatReferenceColor(getColorReferenceSRGB());
	pixelExpected = this->formatReferenceColor(getColorReferenceLinear());

	this->logColor(std::string("Expected color: "), resultColorIdx, pixelExpected);

	//	texture was rendered using a sampler object with setting DECODE_EXT, therefore, results should be linear
	if ( (tcu::boolAll(tcu::lessThan(tcu::abs(pixelConverted - pixelReference), m_epsilonError))) || (tcu::boolAll(tcu::equal(pixelConverted, pixelReference))) )
	{
		log << tcu::TestLog::Message << std::string("linear as expected") << tcu::TestLog::EndMessage;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << std::string("not linear as expected") << tcu::TestLog::EndMessage;
		return false;
	}
}

} // anonymous

SRGBDecodeTests::SRGBDecodeTests	(Context& context)
	: TestCaseGroup					(context, "skip_decode", "sRGB skip decode tests")
{
}

SRGBDecodeTests::~SRGBDecodeTests (void)
{
}

void SRGBDecodeTests::init (void)
{
	const TestGroupConfig testGroupConfigList[] =
	{
		TestGroupConfig("srgba8",	"srgb decode tests using srgba internal format",	tcu::TextureFormat(tcu::TextureFormat::sRGBA, tcu::TextureFormat::UNORM_INT8)),
		TestGroupConfig("sr8",		"srgb decode tests using sr8 internal format",		tcu::TextureFormat(tcu::TextureFormat::sR, tcu::TextureFormat::UNORM_INT8))
	};

	// create groups for all desired internal formats, adding test cases to each
	for (std::size_t idx = 0; idx < DE_LENGTH_OF_ARRAY(testGroupConfigList); idx++)
	{
		tcu::TestCaseGroup* const testGroup = new tcu::TestCaseGroup(m_testCtx, testGroupConfigList[idx].name, testGroupConfigList[idx].description);
		tcu::TestNode::addChild(testGroup);

		testGroup->addChild(new TextureDecodeSkippedCase		(m_context, "skipped",			"testing for sRGB color values with sRGB texture decoding skipped",		testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new TextureDecodeEnabledCase		(m_context, "enabled",			"testing for linear color values with sRGB texture decoding enabled",	testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new TexelFetchDecodeSkippedcase		(m_context, "texel_fetch",		"testing for linear color values with sRGB texture decoding skipped",	testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new GPUConversionDecodeEnabledCase	(m_context, "conversion_gpu",	"sampling linear values and performing conversion on the gpu",			testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new DecodeToggledCase				(m_context, "toggled",			"toggle the sRGB decoding between draw calls",							testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new DecodeMultipleTexturesCase		(m_context, "multiple_textures","upload multiple textures with different sRGB decode values and sample",testGroupConfigList[idx].internalFormat));
		testGroup->addChild(new DecodeSamplerCase				(m_context, "using_sampler",	"testing that sampler object takes priority over texture state",		testGroupConfigList[idx].internalFormat));
	}
}

} // Functional
} // gles31
} // deqp
