/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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

#include "es31fTextureFormatTests.hpp"
#include "gluContextInfo.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"
#include "glsTextureTestUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

using std::vector;
using std::string;
using tcu::TestLog;

namespace deqp
{
namespace gles31
{
namespace Functional
{

using namespace deqp::gls;
using namespace deqp::gls::TextureTestUtil;
using namespace glu::TextureTestUtil;

using tcu::Sampler;

struct SupportedExtensions
{
	bool cubeMapArray;
	bool sRGBR8;
};

static tcu::CubeFace getCubeFaceFromNdx (int ndx)
{
	switch (ndx)
	{
		case 0:	return tcu::CUBEFACE_POSITIVE_X;
		case 1:	return tcu::CUBEFACE_NEGATIVE_X;
		case 2:	return tcu::CUBEFACE_POSITIVE_Y;
		case 3:	return tcu::CUBEFACE_NEGATIVE_Y;
		case 4:	return tcu::CUBEFACE_POSITIVE_Z;
		case 5:	return tcu::CUBEFACE_NEGATIVE_Z;
		default:
			DE_ASSERT(false);
			return tcu::CUBEFACE_LAST;
	}
}

namespace
{

SupportedExtensions checkSupport (const glu::ContextInfo& renderCtxInfoid)
{
	SupportedExtensions supportedExtensions;

	supportedExtensions.cubeMapArray = renderCtxInfoid.isExtensionSupported("GL_EXT_texture_cube_map_array");
	supportedExtensions.sRGBR8 = renderCtxInfoid.isExtensionSupported("GL_EXT_texture_sRGB_R8");

	return supportedExtensions;
}

} // anonymous

// TextureCubeArrayFormatCase

class TextureCubeArrayFormatCase : public tcu::TestCase
{
public:
										TextureCubeArrayFormatCase	(tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const glu::ContextInfo& renderCtxInfo, const char* name, const char* description, deUint32 format, deUint32 dataType, int size, int depth);
										TextureCubeArrayFormatCase	(tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const glu::ContextInfo& renderCtxInfo, const char* name, const char* description, deUint32 internalFormat, int size, int depth);
										~TextureCubeArrayFormatCase	(void);

	void								init						(void);
	void								deinit						(void);
	IterateResult						iterate						(void);

private:
										TextureCubeArrayFormatCase	(const TextureCubeArrayFormatCase& other);
	TextureCubeArrayFormatCase&			operator=					(const TextureCubeArrayFormatCase& other);

	bool								testLayerFace				(int layerNdx);

	glu::RenderContext&					m_renderCtx;
	const glu::ContextInfo&				m_renderCtxInfo;

	const deUint32						m_format;
	const deUint32						m_dataType;
	const int							m_size;
	const int							m_depth;

	glu::TextureCubeArray*				m_texture;
	TextureTestUtil::TextureRenderer	m_renderer;

	int									m_curLayerFace;
};

TextureCubeArrayFormatCase::TextureCubeArrayFormatCase (tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const glu::ContextInfo& renderCtxInfo, const char* name, const char* description, deUint32 format, deUint32 dataType, int size, int depth)
	: TestCase			(testCtx, name, description)
	, m_renderCtx		(renderCtx)
	, m_renderCtxInfo	(renderCtxInfo)
	, m_format			(format)
	, m_dataType		(dataType)
	, m_size			(size)
	, m_depth			(depth)
	, m_texture			(DE_NULL)
	, m_renderer		(renderCtx, testCtx.getLog(), glu::getContextTypeGLSLVersion(renderCtx.getType()), glu::PRECISION_HIGHP)
	, m_curLayerFace	(0)
{
}

TextureCubeArrayFormatCase::TextureCubeArrayFormatCase (tcu::TestContext& testCtx, glu::RenderContext& renderCtx, const glu::ContextInfo& renderCtxInfo, const char* name, const char* description, deUint32 internalFormat, int size, int depth)
	: TestCase			(testCtx, name, description)
	, m_renderCtx		(renderCtx)
	, m_renderCtxInfo	(renderCtxInfo)
	, m_format			(internalFormat)
	, m_dataType		(GL_NONE)
	, m_size			(size)
	, m_depth			(depth)
	, m_texture			(DE_NULL)
	, m_renderer		(renderCtx, testCtx.getLog(), glu::getContextTypeGLSLVersion(renderCtx.getType()), glu::PRECISION_HIGHP)
	, m_curLayerFace	(0)
{
}

TextureCubeArrayFormatCase::~TextureCubeArrayFormatCase (void)
{
	deinit();
}

void TextureCubeArrayFormatCase::init (void)
{
	const SupportedExtensions supportedExtensions = checkSupport(m_renderCtxInfo);

	if ((supportedExtensions.cubeMapArray && m_format != GL_SR8_EXT) ||
		(supportedExtensions.cubeMapArray && m_format == GL_SR8_EXT && supportedExtensions.sRGBR8))
	{
		m_texture = m_dataType != GL_NONE
				  ? new glu::TextureCubeArray(m_renderCtx, m_format, m_dataType, m_size, m_depth)	// Implicit internal format.
				  : new glu::TextureCubeArray(m_renderCtx, m_format, m_size, m_depth);				// Explicit internal format.

		tcu::TextureFormatInfo spec = tcu::getTextureFormatInfo(m_texture->getRefTexture().getFormat());

		// Fill level 0.
		m_texture->getRefTexture().allocLevel(0);
		tcu::fillWithComponentGradients(m_texture->getRefTexture().getLevel(0), spec.valueMin, spec.valueMax);

		// Initialize state.
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		m_curLayerFace = 0;
	}
	else
	{
		if (supportedExtensions.cubeMapArray == false)
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Cube map arrays not supported");

		if (supportedExtensions.sRGBR8 == false)
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "texture srgb r8 not supported");
	}
}

void TextureCubeArrayFormatCase::deinit (void)
{
	delete m_texture;
	m_texture = DE_NULL;

	m_renderer.clear();
}

bool TextureCubeArrayFormatCase::testLayerFace (int layerFaceNdx)
{
	const glw::Functions&	gl				= m_renderCtx.getFunctions();
	TestLog&				log				= m_testCtx.getLog();
	RandomViewport			viewport		(m_renderCtx.getRenderTarget(), m_size, m_size, deStringHash(getName()));
	tcu::Surface			renderedFrame	(viewport.width, viewport.height);
	tcu::Surface			referenceFrame	(viewport.width, viewport.height);
	tcu::RGBA				threshold		= m_renderCtx.getRenderTarget().getPixelFormat().getColorThreshold() + tcu::RGBA(1,1,1,1);
	vector<float>			texCoord;
	ReferenceParams			renderParams	(TEXTURETYPE_CUBE_ARRAY);
	tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(m_texture->getRefTexture().getFormat());
	const int				layerNdx		= layerFaceNdx / 6;
	const tcu::CubeFace		face			= getCubeFaceFromNdx(layerFaceNdx % 6);

	renderParams.samplerType				= getSamplerType(m_texture->getRefTexture().getFormat());
	renderParams.sampler					= Sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::NEAREST, Sampler::NEAREST);
	renderParams.sampler.seamlessCubeMap	= true;
	renderParams.colorScale					= spec.lookupScale;
	renderParams.colorBias					= spec.lookupBias;

	// Layer here specifies the cube slice
	computeQuadTexCoordCubeArray(texCoord, face, tcu::Vec2(0.0f, 0.0f), tcu::Vec2(1.0f, 1.0f), tcu::Vec2((float)layerNdx));

	// Setup base viewport.
	gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

	// Upload texture data to GL.
	m_texture->upload();

	// Bind to unit 0.
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_texture->getGLTexture());

	// Setup nearest neighbor filtering and clamp-to-edge.
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Set texturing state");

	// Draw.
	m_renderer.renderQuad(0, &texCoord[0], renderParams);
	glu::readPixels(m_renderCtx, viewport.x, viewport.y, renderedFrame.getAccess());

	// Compute reference.
	sampleTexture(tcu::SurfaceAccess(referenceFrame, m_renderCtx.getRenderTarget().getPixelFormat()), m_texture->getRefTexture(), &texCoord[0], renderParams);

	// Compare and log.
	return compareImages(log, (string("LayerFace" + de::toString(layerFaceNdx))).c_str(), (string("Layer-face " + de::toString(layerFaceNdx))).c_str(), referenceFrame, renderedFrame, threshold);
}

TextureCubeArrayFormatCase::IterateResult TextureCubeArrayFormatCase::iterate (void)
{
	if (m_testCtx.getTestResult() == QP_TEST_RESULT_NOT_SUPPORTED)
		return STOP;

	// Execute test for all layers.
	bool isOk = testLayerFace(m_curLayerFace);

	if (!isOk && m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");

	m_curLayerFace += 1;

	return m_curLayerFace < m_texture->getRefTexture().getDepth() ? CONTINUE : STOP;
}

// TextureBufferFormatCase

class TextureBufferFormatCase : public TestCase
{
public:
								TextureBufferFormatCase		(Context& ctx, glu::RenderContext& renderCtx, const char* name, const char* description, deUint32 internalFormat, int width);
								~TextureBufferFormatCase	(void);

	void						init						(void);
	void						deinit						(void);
	IterateResult				iterate						(void);

private:
								TextureBufferFormatCase		(const TextureBufferFormatCase& other);
	TextureBufferFormatCase&	operator=					(const TextureBufferFormatCase& other);

	glu::RenderContext&			m_renderCtx;

	deUint32					m_format;
	int							m_width;
	int							m_maxTextureBufferSize;

	glu::TextureBuffer*			m_texture;
	TextureRenderer				m_renderer;
};

TextureBufferFormatCase::TextureBufferFormatCase (Context& ctx, glu::RenderContext& renderCtx, const char* name, const char* description, deUint32 internalFormat, int width)
	: TestCase					(ctx, name, description)
	, m_renderCtx				(renderCtx)
	, m_format					(internalFormat)
	, m_width					(width)
	, m_maxTextureBufferSize	(0)
	, m_texture					(DE_NULL)
	, m_renderer				(renderCtx, ctx.getTestContext().getLog(), glu::getContextTypeGLSLVersion(renderCtx.getType()), glu::PRECISION_HIGHP)
{
}

TextureBufferFormatCase::~TextureBufferFormatCase (void)
{
	deinit();
}

void TextureBufferFormatCase::init (void)
{
	TestLog&				log				= m_testCtx.getLog();
	tcu::TextureFormat		fmt				= glu::mapGLInternalFormat(m_format);
	tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(fmt);
	tcu::Vec4				colorA			(spec.valueMin.x(), spec.valueMax.y(), spec.valueMin.z(), spec.valueMax.w());
	tcu::Vec4				colorB			(spec.valueMax.x(), spec.valueMin.y(), spec.valueMax.z(), spec.valueMin.w());
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32
		&& !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_buffer")
		&& !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_buffer"))
	{
		TCU_THROW(NotSupportedError, "Texture buffers not supported");
	}

	m_maxTextureBufferSize = m_context.getContextInfo().getInt(GL_MAX_TEXTURE_BUFFER_SIZE);

	if (m_maxTextureBufferSize <= 0)
		TCU_THROW(NotSupportedError, "GL_MAX_TEXTURE_BUFFER_SIZE > 0 required");

	log << TestLog::Message << "Buffer texture, " << glu::getTextureFormatStr(m_format) << ", " << m_width
							<< ",\n  fill with " << formatGradient(&colorA, &colorB) << " gradient"
		<< TestLog::EndMessage;

	m_texture = new glu::TextureBuffer(m_renderCtx, m_format, m_width * fmt.getPixelSize());

	// Fill level 0.
	tcu::fillWithComponentGradients(m_texture->getFullRefTexture(), colorA, colorB);
}

void TextureBufferFormatCase::deinit (void)
{
	delete m_texture;
	m_texture = DE_NULL;

	m_renderer.clear();
}

TextureBufferFormatCase::IterateResult TextureBufferFormatCase::iterate (void)
{
	TestLog&							log						= m_testCtx.getLog();
	const glw::Functions&				gl						= m_renderCtx.getFunctions();
	RandomViewport						viewport				(m_renderCtx.getRenderTarget(), m_width, 1, deStringHash(getName()));
	tcu::Surface						renderedFrame			(viewport.width, viewport.height);
	tcu::Surface						referenceFrame			(viewport.width, viewport.height);
	tcu::RGBA							threshold				= m_renderCtx.getRenderTarget().getPixelFormat().getColorThreshold() + tcu::RGBA(1,1,1,1);
	vector<float>						texCoord;
	RenderParams						renderParams			(TEXTURETYPE_BUFFER);
	const tcu::ConstPixelBufferAccess	effectiveRefTexture		= glu::getTextureBufferEffectiveRefTexture(*m_texture, m_maxTextureBufferSize);
	tcu::TextureFormatInfo				spec					= tcu::getTextureFormatInfo(effectiveRefTexture.getFormat());

	renderParams.flags			|= RenderParams::LOG_ALL;
	renderParams.samplerType	= getFetchSamplerType(effectiveRefTexture.getFormat());
	renderParams.colorScale		= spec.lookupScale;
	renderParams.colorBias		= spec.lookupBias;

	computeQuadTexCoord1D(texCoord, 0.0f, (float)(effectiveRefTexture.getWidth()));

	gl.clearColor(0.125f, 0.25f, 0.5f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	// Setup base viewport.
	gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

	// Upload texture data to GL.
	m_texture->upload();

	// Bind to unit 0.
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_BUFFER, m_texture->getGLTexture());

	GLU_EXPECT_NO_ERROR(gl.getError(), "Set texturing state");

	// Draw.
	m_renderer.renderQuad(0, &texCoord[0], renderParams);
	glu::readPixels(m_renderCtx, viewport.x, viewport.y, renderedFrame.getAccess());

	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels()");

	// Compute reference.
	fetchTexture(tcu::SurfaceAccess(referenceFrame, m_renderCtx.getRenderTarget().getPixelFormat()), effectiveRefTexture, &texCoord[0], spec.lookupScale, spec.lookupBias);

	// Compare and log.
	bool isOk = compareImages(log, referenceFrame, renderedFrame, threshold);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
							isOk ? "Pass"				: "Image comparison failed");

	return STOP;
}

// TextureFormatTests

TextureFormatTests::TextureFormatTests (Context& context)
	: TestCaseGroup(context, "format", "Texture Format Tests")
{
}

TextureFormatTests::~TextureFormatTests (void)
{
}

vector<string> toStringVector (const char* const* str, int numStr)
{
	vector<string> v;
	v.resize(numStr);
	for (int i = 0; i < numStr; i++)
		v[i] = str[i];
	return v;
}

void TextureFormatTests::init (void)
{
	tcu::TestCaseGroup* unsizedGroup	= DE_NULL;
	tcu::TestCaseGroup*	sizedGroup		= DE_NULL;
	tcu::TestCaseGroup*	sizedBufferGroup = DE_NULL;
	addChild((unsizedGroup		= new tcu::TestCaseGroup(m_testCtx,	"unsized",	"Unsized formats")));
	addChild((sizedGroup		= new tcu::TestCaseGroup(m_testCtx,	"sized",	"Sized formats")));
	addChild((sizedBufferGroup	= new tcu::TestCaseGroup(m_testCtx,	"buffer",	"Sized formats (Buffer)")));

	tcu::TestCaseGroup*	sizedCubeArrayGroup	= DE_NULL;
	sizedGroup->addChild((sizedCubeArrayGroup = new tcu::TestCaseGroup(m_testCtx, "cube_array", "Sized formats (2D Array)")));

	struct
	{
		const char*	name;
		deUint32	format;
		deUint32	dataType;
	} texFormats[] =
	{
		{ "alpha",							GL_ALPHA,			GL_UNSIGNED_BYTE },
		{ "luminance",						GL_LUMINANCE,		GL_UNSIGNED_BYTE },
		{ "luminance_alpha",				GL_LUMINANCE_ALPHA,	GL_UNSIGNED_BYTE },
		{ "rgb_unsigned_short_5_6_5",		GL_RGB,				GL_UNSIGNED_SHORT_5_6_5 },
		{ "rgb_unsigned_byte",				GL_RGB,				GL_UNSIGNED_BYTE },
		{ "rgba_unsigned_short_4_4_4_4",	GL_RGBA,			GL_UNSIGNED_SHORT_4_4_4_4 },
		{ "rgba_unsigned_short_5_5_5_1",	GL_RGBA,			GL_UNSIGNED_SHORT_5_5_5_1 },
		{ "rgba_unsigned_byte",				GL_RGBA,			GL_UNSIGNED_BYTE }
	};

	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(texFormats); formatNdx++)
	{
		deUint32	format		= texFormats[formatNdx].format;
		deUint32	dataType	= texFormats[formatNdx].dataType;
		string	nameBase		= texFormats[formatNdx].name;
		string	descriptionBase	= string(glu::getTextureFormatName(format)) + ", " + glu::getTypeName(dataType);

		unsizedGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_cube_array_pot").c_str(),		(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), format, dataType, 64, 12));
		unsizedGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_cube_array_npot").c_str(),	(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), format, dataType, 64, 12));
	}

	struct
	{
		const char*	name;
		deUint32	internalFormat;
	} sizedColorFormats[] =
	{
		{ "rgba32f",			GL_RGBA32F,			},
		{ "rgba32i",			GL_RGBA32I,			},
		{ "rgba32ui",			GL_RGBA32UI,		},
		{ "rgba16f",			GL_RGBA16F,			},
		{ "rgba16i",			GL_RGBA16I,			},
		{ "rgba16ui",			GL_RGBA16UI,		},
		{ "rgba8",				GL_RGBA8,			},
		{ "rgba8i",				GL_RGBA8I,			},
		{ "rgba8ui",			GL_RGBA8UI,			},
		{ "srgb_r8",			GL_SR8_EXT,			},
		{ "srgb8_alpha8",		GL_SRGB8_ALPHA8,	},
		{ "rgb10_a2",			GL_RGB10_A2,		},
		{ "rgb10_a2ui",			GL_RGB10_A2UI,		},
		{ "rgba4",				GL_RGBA4,			},
		{ "rgb5_a1",			GL_RGB5_A1,			},
		{ "rgba8_snorm",		GL_RGBA8_SNORM,		},
		{ "rgb8",				GL_RGB8,			},
		{ "rgb565",				GL_RGB565,			},
		{ "r11f_g11f_b10f",		GL_R11F_G11F_B10F,	},
		{ "rgb32f",				GL_RGB32F,			},
		{ "rgb32i",				GL_RGB32I,			},
		{ "rgb32ui",			GL_RGB32UI,			},
		{ "rgb16f",				GL_RGB16F,			},
		{ "rgb16i",				GL_RGB16I,			},
		{ "rgb16ui",			GL_RGB16UI,			},
		{ "rgb8_snorm",			GL_RGB8_SNORM,		},
		{ "rgb8i",				GL_RGB8I,			},
		{ "rgb8ui",				GL_RGB8UI,			},
		{ "srgb8",				GL_SRGB8,			},
		{ "rgb9_e5",			GL_RGB9_E5,			},
		{ "rg32f",				GL_RG32F,			},
		{ "rg32i",				GL_RG32I,			},
		{ "rg32ui",				GL_RG32UI,			},
		{ "rg16f",				GL_RG16F,			},
		{ "rg16i",				GL_RG16I,			},
		{ "rg16ui",				GL_RG16UI,			},
		{ "rg8",				GL_RG8,				},
		{ "rg8i",				GL_RG8I,			},
		{ "rg8ui",				GL_RG8UI,			},
		{ "rg8_snorm",			GL_RG8_SNORM,		},
		{ "r32f",				GL_R32F,			},
		{ "r32i",				GL_R32I,			},
		{ "r32ui",				GL_R32UI,			},
		{ "r16f",				GL_R16F,			},
		{ "r16i",				GL_R16I,			},
		{ "r16ui",				GL_R16UI,			},
		{ "r8",					GL_R8,				},
		{ "r8i",				GL_R8I,				},
		{ "r8ui",				GL_R8UI,			},
		{ "r8_snorm",			GL_R8_SNORM,		}
	};

	struct
	{
		const char*	name;
		deUint32	internalFormat;
	} sizedDepthStencilFormats[] =
	{
		// Depth and stencil formats
		{ "depth_component32f",	GL_DEPTH_COMPONENT32F	},
		{ "depth_component24",	GL_DEPTH_COMPONENT24	},
		{ "depth_component16",	GL_DEPTH_COMPONENT16	},
		{ "depth32f_stencil8",	GL_DEPTH32F_STENCIL8	},
		{ "depth24_stencil8",	GL_DEPTH24_STENCIL8		}
	};

	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(sizedColorFormats); formatNdx++)
	{
		deUint32	internalFormat	= sizedColorFormats[formatNdx].internalFormat;
		string		nameBase		= sizedColorFormats[formatNdx].name;
		string		descriptionBase	= glu::getTextureFormatName(internalFormat);

		sizedCubeArrayGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_pot").c_str(),		(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), internalFormat, 64, 12));
		sizedCubeArrayGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_npot").c_str(),	(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), internalFormat, 64, 12));
	}

	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(sizedDepthStencilFormats); formatNdx++)
	{
		deUint32	internalFormat	= sizedDepthStencilFormats[formatNdx].internalFormat;
		string		nameBase		= sizedDepthStencilFormats[formatNdx].name;
		string		descriptionBase	= glu::getTextureFormatName(internalFormat);

		sizedCubeArrayGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_pot").c_str(),		(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), internalFormat, 64, 12));
		sizedCubeArrayGroup->addChild(new TextureCubeArrayFormatCase (m_testCtx, m_context.getRenderContext(), m_context.getContextInfo(), (nameBase + "_npot").c_str(),	(descriptionBase + ", GL_TEXTURE_CUBE_MAP_ARRAY").c_str(), internalFormat, 64, 12));
	}

	// \todo Check
	struct
	{
		const char*	name;
		deUint32	internalFormat;
	} bufferColorFormats[] =
	{
		{ "r8",					GL_R8,				},
		{ "r16f",				GL_R16F,			},
		{ "r32f",				GL_R32F,			},
		{ "r8i",				GL_R8I,				},
		{ "r16i",				GL_R16I,			},
		{ "r32i",				GL_R32I,			},
		{ "r8ui",				GL_R8UI,			},
		{ "r16ui",				GL_R16UI,			},
		{ "r32ui",				GL_R32UI,			},
		{ "rg8",				GL_RG8,				},
		{ "rg16f",				GL_RG16F,			},
		{ "rg32f",				GL_RG32F,			},
		{ "rg8i",				GL_RG8I,			},
		{ "rg16i",				GL_RG16I,			},
		{ "rg32i",				GL_RG32I,			},
		{ "rg8ui",				GL_RG8UI,			},
		{ "rg16ui",				GL_RG16UI,			},
		{ "rg32ui",				GL_RG32UI,			},
		{ "rgba8",				GL_RGBA8,			},
		{ "rgba16f",			GL_RGBA16F,			},
		{ "rgba32f",			GL_RGBA32F,			},
		{ "rgba8i",				GL_RGBA8I,			},
		{ "rgba16i",			GL_RGBA16I,			},
		{ "rgba32i",			GL_RGBA32I,			},
		{ "rgba8ui",			GL_RGBA8UI,			},
		{ "rgba16ui",			GL_RGBA16UI,		},
		{ "rgba32ui",			GL_RGBA32UI,		}
	};

	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(bufferColorFormats); formatNdx++)
	{
		deUint32	internalFormat	= bufferColorFormats[formatNdx].internalFormat;
		string		nameBase		= bufferColorFormats[formatNdx].name;
		string		descriptionBase	= glu::getTextureFormatName(internalFormat);

		sizedBufferGroup->addChild	(new TextureBufferFormatCase	(m_context, m_context.getRenderContext(),	(nameBase + "_pot").c_str(),	(descriptionBase + ", GL_TEXTURE_BUFFER").c_str(),	internalFormat, 64));
		sizedBufferGroup->addChild	(new TextureBufferFormatCase	(m_context, m_context.getRenderContext(),	(nameBase + "_npot").c_str(),	(descriptionBase + ", GL_TEXTURE_BUFFER").c_str(),	internalFormat, 112));
	}
}

} // Functional
} // gles31
} // deqp
