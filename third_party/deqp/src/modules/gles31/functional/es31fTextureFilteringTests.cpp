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
 * \brief Texture filtering tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTextureFilteringTests.hpp"

#include "glsTextureTestUtil.hpp"

#include "gluPixelTransfer.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"

#include "tcuCommandLine.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTexLookupVerifier.hpp"
#include "tcuVectorUtil.hpp"

#include "deStringUtil.hpp"
#include "deString.h"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::vector;
using std::string;
using tcu::TestLog;
using namespace gls::TextureTestUtil;
using namespace glu::TextureTestUtil;

static const char* getFaceDesc (const tcu::CubeFace face)
{
	switch (face)
	{
		case tcu::CUBEFACE_NEGATIVE_X:	return "-X";
		case tcu::CUBEFACE_POSITIVE_X:	return "+X";
		case tcu::CUBEFACE_NEGATIVE_Y:	return "-Y";
		case tcu::CUBEFACE_POSITIVE_Y:	return "+Y";
		case tcu::CUBEFACE_NEGATIVE_Z:	return "-Z";
		case tcu::CUBEFACE_POSITIVE_Z:	return "+Z";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

static void logCubeArrayTexCoords(TestLog& log, vector<float>& texCoord)
{
	const size_t numVerts = texCoord.size() / 4;

	DE_ASSERT(texCoord.size() % 4 == 0);

	for (size_t vertNdx = 0; vertNdx < numVerts; vertNdx++)
	{
		const size_t	coordNdx	= vertNdx * 4;

		const float		u			= texCoord[coordNdx + 0];
		const float		v			= texCoord[coordNdx + 1];
		const float		w			= texCoord[coordNdx + 2];
		const float		q			= texCoord[coordNdx + 3];

		log << TestLog::Message
			<< vertNdx << ": ("
			<< u << ", "
			<< v << ", "
			<< w << ", "
			<< q << ")"
			<< TestLog::EndMessage;
	}
}

// Cube map array filtering

class TextureCubeArrayFilteringCase : public TestCase
{
public:
									TextureCubeArrayFilteringCase	(Context& context,
																	 const char* name,
																	 const char* desc,
																	 deUint32 minFilter,
																	 deUint32 magFilter,
																	 deUint32 wrapS,
																	 deUint32 wrapT,
																	 deUint32 internalFormat,
																	 int size,
																	 int depth,
																	 bool onlySampleFaceInterior = false);

									~TextureCubeArrayFilteringCase	(void);

	void							init							(void);
	void							deinit							(void);
	IterateResult					iterate							(void);

private:
									TextureCubeArrayFilteringCase	(const TextureCubeArrayFilteringCase&);
	TextureCubeArrayFilteringCase&	operator=						(const TextureCubeArrayFilteringCase&);

	const deUint32					m_minFilter;
	const deUint32					m_magFilter;
	const deUint32					m_wrapS;
	const deUint32					m_wrapT;

	const deUint32					m_internalFormat;
	const int						m_size;
	const int						m_depth;

	const bool						m_onlySampleFaceInterior; //!< If true, we avoid sampling anywhere near a face's edges.

	struct FilterCase
	{
		const glu::TextureCubeArray*	texture;
		tcu::Vec2						bottomLeft;
		tcu::Vec2						topRight;
		tcu::Vec2						layerRange;

		FilterCase (void)
			: texture(DE_NULL)
		{
		}

		FilterCase (const glu::TextureCubeArray* tex_, const tcu::Vec2& bottomLeft_, const tcu::Vec2& topRight_, const tcu::Vec2& layerRange_)
			: texture		(tex_)
			, bottomLeft	(bottomLeft_)
			, topRight		(topRight_)
			, layerRange	(layerRange_)
		{
		}
	};

	glu::TextureCubeArray*	m_gradientTex;
	glu::TextureCubeArray*	m_gridTex;

	TextureRenderer			m_renderer;

	std::vector<FilterCase>	m_cases;
	int						m_caseNdx;
};

TextureCubeArrayFilteringCase::TextureCubeArrayFilteringCase (Context& context,
															  const char* name,
															  const char* desc,
															  deUint32 minFilter,
															  deUint32 magFilter,
															  deUint32 wrapS,
															  deUint32 wrapT,
															  deUint32 internalFormat,
															  int size,
															  int depth,
															  bool onlySampleFaceInterior)
	: TestCase					(context, name, desc)
	, m_minFilter				(minFilter)
	, m_magFilter				(magFilter)
	, m_wrapS					(wrapS)
	, m_wrapT					(wrapT)
	, m_internalFormat			(internalFormat)
	, m_size					(size)
	, m_depth					(depth)
	, m_onlySampleFaceInterior	(onlySampleFaceInterior)
	, m_gradientTex				(DE_NULL)
	, m_gridTex					(DE_NULL)
	, m_renderer				(context.getRenderContext(), context.getTestContext().getLog(), glu::GLSL_VERSION_310_ES, glu::PRECISION_HIGHP)
	, m_caseNdx					(0)
{
}

TextureCubeArrayFilteringCase::~TextureCubeArrayFilteringCase (void)
{
	TextureCubeArrayFilteringCase::deinit();
}

void TextureCubeArrayFilteringCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_cube_map_array"))
		throw tcu::NotSupportedError("GL_EXT_texture_cube_map_array not supported");

	if (m_internalFormat == GL_SR8_EXT && !(m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_sRGB_R8")))
		TCU_THROW(NotSupportedError, "GL_EXT_texture_sRGB_R8 not supported");

	try
	{
		const tcu::TextureFormat		texFmt		= glu::mapGLInternalFormat(m_internalFormat);
		const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(texFmt);
		const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;
		const tcu::Vec4					cBias		= fmtInfo.valueMin;
		const int						numLevels	= deLog2Floor32(m_size) + 1;
		const int						numLayers	= m_depth / 6;

		// Create textures.
		m_gradientTex	= new glu::TextureCubeArray(m_context.getRenderContext(), m_internalFormat, m_size, m_depth);
		m_gridTex		= new glu::TextureCubeArray(m_context.getRenderContext(), m_internalFormat, m_size, m_depth);

		const tcu::IVec4 levelSwz[] =
		{
			tcu::IVec4(0,1,2,3),
			tcu::IVec4(2,1,3,0),
			tcu::IVec4(3,0,1,2),
			tcu::IVec4(1,3,2,0),
		};

		// Fill first gradient texture (gradient direction varies between layers).
		for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
		{
			m_gradientTex->getRefTexture().allocLevel(levelNdx);

			const tcu::PixelBufferAccess levelBuf = m_gradientTex->getRefTexture().getLevel(levelNdx);

			for (int layerFaceNdx = 0; layerFaceNdx < m_depth; layerFaceNdx++)
			{
				const tcu::IVec4	swz		= levelSwz[layerFaceNdx % DE_LENGTH_OF_ARRAY(levelSwz)];
				const tcu::Vec4		gMin	= tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f).swizzle(swz[0],swz[1],swz[2],swz[3])*cScale + cBias;
				const tcu::Vec4		gMax	= tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f).swizzle(swz[0],swz[1],swz[2],swz[3])*cScale + cBias;

				tcu::fillWithComponentGradients(tcu::getSubregion(levelBuf, 0, 0, layerFaceNdx, levelBuf.getWidth(), levelBuf.getHeight(), 1), gMin, gMax);
			}
		}

		// Fill second with grid texture (each layer has unique colors).
		for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
		{
			m_gridTex->getRefTexture().allocLevel(levelNdx);

			const tcu::PixelBufferAccess levelBuf = m_gridTex->getRefTexture().getLevel(levelNdx);

			for (int layerFaceNdx = 0; layerFaceNdx < m_depth; layerFaceNdx++)
			{
				const deUint32	step	= 0x00ffffff / (numLevels*m_depth - 1);
				const deUint32	rgb		= step * (levelNdx + layerFaceNdx*numLevels);
				const deUint32	colorA	= 0xff000000 | rgb;
				const deUint32	colorB	= 0xff000000 | ~rgb;

				tcu::fillWithGrid(tcu::getSubregion(levelBuf, 0, 0, layerFaceNdx, levelBuf.getWidth(), levelBuf.getHeight(), 1),
								  4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
			}
		}

		// Upload.
		m_gradientTex->upload();
		m_gridTex->upload();

		// Test cases
		{
			const glu::TextureCubeArray* const	tex0	= m_gradientTex;
			const glu::TextureCubeArray* const	tex1	= m_gridTex;

			if (m_onlySampleFaceInterior)
			{
				m_cases.push_back(FilterCase(tex0, tcu::Vec2(-0.8f, -0.8f),	tcu::Vec2(0.8f,  0.8f),	tcu::Vec2(-0.5f, float(numLayers)+0.5f)));	// minification
				m_cases.push_back(FilterCase(tex0, tcu::Vec2(0.5f, 0.65f),	tcu::Vec2(0.8f,  0.8f),	tcu::Vec2(-0.5f, float(numLayers)+0.5f)));	// magnification
				m_cases.push_back(FilterCase(tex1, tcu::Vec2(-0.8f, -0.8f),	tcu::Vec2(0.8f,  0.8f),	tcu::Vec2(float(numLayers)+0.5f, -0.5f)));	// minification
				m_cases.push_back(FilterCase(tex1, tcu::Vec2(0.2f, 0.2f),	tcu::Vec2(0.6f,  0.5f),	tcu::Vec2(float(numLayers)+0.5f, -0.5f)));	// magnification
			}
			else
			{
				const bool isSingleSample = (m_context.getRenderTarget().getNumSamples() == 0);

				// minification - w/ tweak to avoid hitting triangle edges with a face switchpoint in multisample configs
				if (isSingleSample)
					m_cases.push_back(FilterCase(tex0, tcu::Vec2(-1.25f, -1.2f), tcu::Vec2(1.2f, 1.25f), tcu::Vec2(-0.5f, float(numLayers)+0.5f)));
				else
					m_cases.push_back(FilterCase(tex0, tcu::Vec2(-1.19f, -1.3f), tcu::Vec2(1.1f, 1.35f), tcu::Vec2(-0.5f, float(numLayers)+0.5f)));

				m_cases.push_back(FilterCase(tex0, tcu::Vec2(0.8f, 0.8f),		tcu::Vec2(1.25f, 1.20f),	tcu::Vec2(-0.5f, float(numLayers)+0.5f)));	// magnification
				m_cases.push_back(FilterCase(tex1, tcu::Vec2(-1.19f, -1.3f),	tcu::Vec2(1.1f, 1.35f),		tcu::Vec2(float(numLayers)+0.5f, -0.5f)));	// minification
				m_cases.push_back(FilterCase(tex1, tcu::Vec2(-1.2f, -1.1f),		tcu::Vec2(-0.8f, -0.8f),	tcu::Vec2(float(numLayers)+0.5f, -0.5f)));	// magnification

				// Layer rounding - only in single-sample configs as multisample configs may produce smooth transition at the middle.
				if (isSingleSample && (numLayers > 1))
					m_cases.push_back(FilterCase(tex0,	tcu::Vec2(-2.0f, -1.5f  ),	tcu::Vec2(-0.1f,  0.9f), tcu::Vec2(1.50001f, 1.49999f)));
			}
		}

		m_caseNdx = 0;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	catch (...)
	{
		// Clean up to save memory.
		TextureCubeArrayFilteringCase::deinit();
		throw;
	}
}

void TextureCubeArrayFilteringCase::deinit (void)
{
	delete m_gradientTex;
	delete m_gridTex;

	m_gradientTex	= DE_NULL;
	m_gridTex		= DE_NULL;

	m_renderer.clear();
	m_cases.clear();
}

TextureCubeArrayFilteringCase::IterateResult TextureCubeArrayFilteringCase::iterate (void)
{
	TestLog&						log				= m_testCtx.getLog();
	const glu::RenderContext&		renderCtx		= m_context.getRenderContext();
	const glw::Functions&			gl				= renderCtx.getFunctions();
	const int						viewportSize	= 28;
	const deUint32					randomSeed		= deStringHash(getName()) ^ deInt32Hash(m_caseNdx) ^ m_testCtx.getCommandLine().getBaseSeed();
	const RandomViewport			viewport		(m_context.getRenderTarget(), viewportSize, viewportSize, randomSeed);
	const FilterCase&				curCase			= m_cases[m_caseNdx];
	const tcu::TextureFormat		texFmt			= curCase.texture->getRefTexture().getFormat();
	const tcu::TextureFormatInfo	fmtInfo			= tcu::getTextureFormatInfo(texFmt);
	const tcu::ScopedLogSection		section			(m_testCtx.getLog(), string("Test") + de::toString(m_caseNdx), string("Test ") + de::toString(m_caseNdx));
	ReferenceParams					refParams		(TEXTURETYPE_CUBE_ARRAY);

	if (viewport.width < viewportSize || viewport.height < viewportSize)
		throw tcu::NotSupportedError("Render target too small", "", __FILE__, __LINE__);

	// Params for reference computation.
	refParams.sampler					= glu::mapGLSampler(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, m_minFilter, m_magFilter);
	refParams.sampler.seamlessCubeMap	= true;
	refParams.samplerType				= getSamplerType(texFmt);
	refParams.colorBias					= fmtInfo.lookupBias;
	refParams.colorScale				= fmtInfo.lookupScale;
	refParams.lodMode					= LODMODE_EXACT;

	gl.bindTexture	(GL_TEXTURE_CUBE_MAP_ARRAY, curCase.texture->getGLTexture());
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER,	m_minFilter);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER,	m_magFilter);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S,		m_wrapS);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T,		m_wrapT);

	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);

	m_testCtx.getLog() << TestLog::Message << "Coordinates: " << curCase.bottomLeft << " -> " << curCase.topRight << TestLog::EndMessage;

	for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
	{
		const tcu::CubeFace		face		= tcu::CubeFace(faceNdx);
		tcu::Surface			result		(viewport.width, viewport.height);
		vector<float>			texCoord;

		computeQuadTexCoordCubeArray(texCoord, face, curCase.bottomLeft, curCase.topRight, curCase.layerRange);

		log << TestLog::Message << "Face " << getFaceDesc(face) << TestLog::EndMessage;

		log << TestLog::Message << "Texture coordinates:" << TestLog::EndMessage;

		logCubeArrayTexCoords(log, texCoord);

		m_renderer.renderQuad(0, &texCoord[0], refParams);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw");

		glu::readPixels(renderCtx, viewport.x, viewport.y, result.getAccess());
		GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels");

		{
			const bool				isNearestOnly	= m_minFilter == GL_NEAREST && m_magFilter == GL_NEAREST;
			const tcu::PixelFormat	pixelFormat		= renderCtx.getRenderTarget().getPixelFormat();
			const tcu::IVec4		coordBits		= tcu::IVec4(10);
			const tcu::IVec4		colorBits		= max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2), tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
			tcu::LodPrecision		lodPrecision;
			tcu::LookupPrecision	lookupPrecision;

			lodPrecision.derivateBits		= 10;
			lodPrecision.lodBits			= 5;
			lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
			lookupPrecision.coordBits		= coordBits.toWidth<3>();
			lookupPrecision.uvwBits			= tcu::IVec3(6);
			lookupPrecision.colorMask		= getCompareMask(pixelFormat);

			const bool isHighQuality = verifyTextureResult(m_testCtx, result.getAccess(), curCase.texture->getRefTexture(),
														   &texCoord[0], refParams, lookupPrecision, coordBits, lodPrecision, pixelFormat);

			if (!isHighQuality)
			{
				// Evaluate against lower precision requirements.
				lodPrecision.lodBits	= 4;
				lookupPrecision.uvwBits	= tcu::IVec3(4);

				m_testCtx.getLog() << TestLog::Message << "Warning: Verification against high precision requirements failed, trying with lower requirements." << TestLog::EndMessage;

				const bool isOk = verifyTextureResult(m_testCtx, result.getAccess(), curCase.texture->getRefTexture(),
													  &texCoord[0], refParams, lookupPrecision, coordBits, lodPrecision, pixelFormat);

				if (!isOk)
				{
					m_testCtx.getLog() << TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case." << TestLog::EndMessage;
					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
				}
				else if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
					m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Low-quality filtering result");
			}
		}
	}

	m_caseNdx += 1;
	return m_caseNdx < (int)m_cases.size() ? CONTINUE : STOP;
}

TextureFilteringTests::TextureFilteringTests (Context& context)
	: TestCaseGroup(context, "filtering", "Texture Filtering Tests")
{
}

TextureFilteringTests::~TextureFilteringTests (void)
{
}

void TextureFilteringTests::init (void)
{
	static const struct
	{
		const char*		name;
		deUint32		mode;
	} wrapModes[] =
	{
		{ "clamp",		GL_CLAMP_TO_EDGE },
		{ "repeat",		GL_REPEAT },
		{ "mirror",		GL_MIRRORED_REPEAT }
	};

	static const struct
	{
		const char*		name;
		deUint32		mode;
	} minFilterModes[] =
	{
		{ "nearest",				GL_NEAREST					},
		{ "linear",					GL_LINEAR					},
		{ "nearest_mipmap_nearest",	GL_NEAREST_MIPMAP_NEAREST	},
		{ "linear_mipmap_nearest",	GL_LINEAR_MIPMAP_NEAREST	},
		{ "nearest_mipmap_linear",	GL_NEAREST_MIPMAP_LINEAR	},
		{ "linear_mipmap_linear",	GL_LINEAR_MIPMAP_LINEAR		}
	};

	static const struct
	{
		const char*		name;
		deUint32		mode;
	} magFilterModes[] =
	{
		{ "nearest",	GL_NEAREST },
		{ "linear",		GL_LINEAR }
	};

	static const struct
	{
		int size;
		int depth;
	} sizesCubeArray[] =
	{
		{   8,	 6 },
		{  64,	12 },
		{ 128,	12 },
		{   7,	12 },
		{  63,	18 }
	};

	static const struct
	{
		const char*		name;
		deUint32		format;
	} filterableFormatsByType[] =
	{
		{ "rgba16f",		GL_RGBA16F			},
		{ "r11f_g11f_b10f",	GL_R11F_G11F_B10F	},
		{ "rgb9_e5",		GL_RGB9_E5			},
		{ "rgba8",			GL_RGBA8			},
		{ "rgba8_snorm",	GL_RGBA8_SNORM		},
		{ "rgb565",			GL_RGB565			},
		{ "rgba4",			GL_RGBA4			},
		{ "rgb5_a1",		GL_RGB5_A1			},
		{ "sr8",			GL_SR8_EXT			},
		{ "srgb8_alpha8",	GL_SRGB8_ALPHA8		},
		{ "rgb10_a2",		GL_RGB10_A2			}
	};

	// Cube map array texture filtering.
	{
		tcu::TestCaseGroup* const groupCubeArray = new tcu::TestCaseGroup(m_testCtx, "cube_array", "Cube Map Array Texture Filtering");
		addChild(groupCubeArray);

		// Formats.
		{
			tcu::TestCaseGroup* const formatsGroup = new tcu::TestCaseGroup(m_testCtx, "formats", "Cube Map Array Texture Formats");
			groupCubeArray->addChild(formatsGroup);

			for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
			{
				for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
				{
					const deUint32	minFilter	= minFilterModes[filterNdx].mode;
					const char*		filterName	= minFilterModes[filterNdx].name;
					const deUint32	format		= filterableFormatsByType[fmtNdx].format;
					const char*		formatName	= filterableFormatsByType[fmtNdx].name;
					const bool		isMipmap	= minFilter != GL_NEAREST && minFilter != GL_LINEAR;
					const deUint32	magFilter	= isMipmap ? GL_LINEAR : minFilter;
					const string	name		= string(formatName) + "_" + filterName;
					const deUint32	wrapS		= GL_REPEAT;
					const deUint32	wrapT		= GL_REPEAT;
					const int		size		= 64;
					const int		depth		= 12;

					formatsGroup->addChild(new TextureCubeArrayFilteringCase(m_context,
																			 name.c_str(), "",
																			 minFilter, magFilter,
																			 wrapS, wrapT,
																			 format,
																			 size, depth));
				}
			}
		}

		// Sizes.
		{
			tcu::TestCaseGroup* const sizesGroup = new tcu::TestCaseGroup(m_testCtx, "sizes", "Texture Sizes");
			groupCubeArray->addChild(sizesGroup);

			for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizesCubeArray); sizeNdx++)
			{
				for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
				{
					const deUint32	minFilter	= minFilterModes[filterNdx].mode;
					const char*		filterName	= minFilterModes[filterNdx].name;
					const deUint32	format		= GL_RGBA8;
					const bool		isMipmap	= minFilter != GL_NEAREST && minFilter != GL_LINEAR;
					const deUint32	magFilter	= isMipmap ? GL_LINEAR : minFilter;
					const deUint32	wrapS		= GL_REPEAT;
					const deUint32	wrapT		= GL_REPEAT;
					const int		size		= sizesCubeArray[sizeNdx].size;
					const int		depth		= sizesCubeArray[sizeNdx].depth;
					const string	name		= de::toString(size) + "x" + de::toString(size) + "x" + de::toString(depth) + "_" + filterName;

					sizesGroup->addChild(new TextureCubeArrayFilteringCase(m_context,
																		   name.c_str(), "",
																		   minFilter, magFilter,
																		   wrapS, wrapT,
																		   format,
																		   size, depth));
				}
			}
		}

		// Wrap modes.
		{
			tcu::TestCaseGroup* const combinationsGroup = new tcu::TestCaseGroup(m_testCtx, "combinations", "Filter and wrap mode combinations");
			groupCubeArray->addChild(combinationsGroup);

			for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
			{
				for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
				{
					for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
					{
						for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
						{
							const deUint32	minFilter	= minFilterModes[minFilterNdx].mode;
							const deUint32	magFilter	= magFilterModes[magFilterNdx].mode;
							const deUint32	format		= GL_RGBA8;
							const deUint32	wrapS		= wrapModes[wrapSNdx].mode;
							const deUint32	wrapT		= wrapModes[wrapTNdx].mode;
							const int		size		= 63;
							const int		depth		= 12;
							const string	name		= string(minFilterModes[minFilterNdx].name) + "_" + magFilterModes[magFilterNdx].name + "_" + wrapModes[wrapSNdx].name + "_" + wrapModes[wrapTNdx].name;

							combinationsGroup->addChild(new TextureCubeArrayFilteringCase(m_context,
																						  name.c_str(), "",
																						  minFilter, magFilter,
																						  wrapS, wrapT,
																						  format,
																						  size, depth));
						}
					}
				}
			}
		}

		// Cases with no visible cube edges.
		{
			tcu::TestCaseGroup* const onlyFaceInteriorGroup = new tcu::TestCaseGroup(m_testCtx, "no_edges_visible", "Don't sample anywhere near a face's edges");
			groupCubeArray->addChild(onlyFaceInteriorGroup);

			for (int isLinearI = 0; isLinearI <= 1; isLinearI++)
			{
				const bool		isLinear	= isLinearI != 0;
				const deUint32	filter		= isLinear ? GL_LINEAR : GL_NEAREST;

				onlyFaceInteriorGroup->addChild(new TextureCubeArrayFilteringCase(m_context,
																				  isLinear ? "linear" : "nearest", "",
																				  filter, filter,
																				  GL_REPEAT, GL_REPEAT,
																				  GL_RGBA8,
																				  63, 12,
																				  true));
			}
		}
	}
}

} // Functional
} // gles31
} // deqp
