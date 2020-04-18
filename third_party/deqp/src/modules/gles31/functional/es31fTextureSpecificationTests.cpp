/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.0 Module
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
 * \brief Texture specification tests.
 *
 * \todo [pyry] Following tests are missing:
 *  - Specify mipmap incomplete texture, use without mipmaps, re-specify
 *    as complete and render.
 *  - Randomly re-specify levels to eventually reach mipmap-complete texture.
 *//*--------------------------------------------------------------------*/

#include "es31fTextureSpecificationTests.hpp"
#include "tcuTestLog.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"
#include "sglrContextUtil.hpp"
#include "sglrContextWrapper.hpp"
#include "sglrGLContext.hpp"
#include "sglrReferenceContext.hpp"
#include "glsTextureTestUtil.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"

// \todo [2012-04-29 pyry] Should be named SglrUtil
#include "es31fFboTestUtil.hpp"

#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using std::vector;
using std::pair;
using tcu::TestLog;
using tcu::Vec4;
using tcu::IVec4;
using tcu::UVec4;
using namespace FboTestUtil;

enum
{
	VIEWPORT_WIDTH	= 256,
	VIEWPORT_HEIGHT	= 256
};

static inline int maxLevelCount (int size)
{
	return (int)deLog2Floor32(size)+1;
}

template <int Size>
static tcu::Vector<float, Size> randomVector (de::Random& rnd, const tcu::Vector<float, Size>& minVal = tcu::Vector<float, Size>(0.0f), const tcu::Vector<float, Size>& maxVal = tcu::Vector<float, Size>(1.0f))
{
	tcu::Vector<float, Size> res;
	for (int ndx = 0; ndx < Size; ndx++)
		res[ndx] = rnd.getFloat(minVal[ndx], maxVal[ndx]);
	return res;
}

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

class TextureSpecCase : public TestCase, public sglr::ContextWrapper
{
public:
						TextureSpecCase			(Context& context, const char* name, const char* desc);
						~TextureSpecCase		(void);

	IterateResult		iterate					(void);

protected:
	virtual bool		checkExtensionSupport	(void)	{ return true; }

	virtual void		createTexture			(void)																= DE_NULL;
	virtual void		verifyTexture			(sglr::GLContext& gles3Context, sglr::ReferenceContext& refContext)	= DE_NULL;

	// Utilities.
	void				renderTex				(tcu::Surface& dst, deUint32 program, int width, int height);
	void				readPixels				(tcu::Surface& dst, int x, int y, int width, int height);

private:
						TextureSpecCase			(const TextureSpecCase& other);
	TextureSpecCase&	operator=				(const TextureSpecCase& other);
};

TextureSpecCase::TextureSpecCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

TextureSpecCase::~TextureSpecCase (void)
{
}

TextureSpecCase::IterateResult TextureSpecCase::iterate (void)
{
	glu::RenderContext&			renderCtx				= TestCase::m_context.getRenderContext();
	const tcu::RenderTarget&	renderTarget			= renderCtx.getRenderTarget();
	tcu::TestLog&				log						= m_testCtx.getLog();

	if (renderTarget.getWidth() < VIEWPORT_WIDTH || renderTarget.getHeight() < VIEWPORT_HEIGHT)
		throw tcu::NotSupportedError("Too small viewport", "", __FILE__, __LINE__);

	if (!checkExtensionSupport())
		throw tcu::NotSupportedError("Extension not supported", "", __FILE__, __LINE__);

	// Context size, and viewport for GLES3.1
	de::Random	rnd			(deStringHash(getName()));
	int			width		= deMin32(renderTarget.getWidth(),	VIEWPORT_WIDTH);
	int			height		= deMin32(renderTarget.getHeight(),	VIEWPORT_HEIGHT);
	int			x			= rnd.getInt(0, renderTarget.getWidth()		- width);
	int			y			= rnd.getInt(0, renderTarget.getHeight()	- height);

	// Contexts.
	sglr::GLContext					gles31Context	(renderCtx, log, sglr::GLCONTEXT_LOG_CALLS, tcu::IVec4(x, y, width, height));
	sglr::ReferenceContextBuffers	refBuffers		(tcu::PixelFormat(8,8,8,renderTarget.getPixelFormat().alphaBits?8:0), 0 /* depth */, 0 /* stencil */, width, height);
	sglr::ReferenceContext			refContext		(sglr::ReferenceContextLimits(renderCtx), refBuffers.getColorbuffer(), refBuffers.getDepthbuffer(), refBuffers.getStencilbuffer());

	// Clear color buffer.
	for (int ndx = 0; ndx < 2; ndx++)
	{
		setContext(ndx ? (sglr::Context*)&refContext : (sglr::Context*)&gles31Context);
		glClearColor(0.125f, 0.25f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	}

	// Construct texture using both GLES3.1 and reference contexts.
	for (int ndx = 0; ndx < 2; ndx++)
	{
		setContext(ndx ? (sglr::Context*)&refContext : (sglr::Context*)&gles31Context);
		createTexture();
		TCU_CHECK(glGetError() == GL_NO_ERROR);
	}

	// Initialize case result to pass.
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// Disable logging.
	gles31Context.enableLogging(0);

	// Verify results.
	verifyTexture(gles31Context, refContext);

	return STOP;
}

void TextureSpecCase::renderTex (tcu::Surface& dst, deUint32 program, int width, int height)
{
	int		targetW		= getWidth();
	int		targetH		= getHeight();

	float	w			= (float)width	/ (float)targetW;
	float	h			= (float)height	/ (float)targetH;

	sglr::drawQuad(*getCurrentContext(), program, tcu::Vec3(-1.0f, -1.0f, 0.0f), tcu::Vec3(-1.0f + w*2.0f, -1.0f + h*2.0f, 0.0f));

	// Read pixels back.
	readPixels(dst, 0, 0, width, height);
}

void TextureSpecCase::readPixels (tcu::Surface& dst, int x, int y, int width, int height)
{
	dst.setSize(width, height);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dst.getAccess().getDataPtr());
}

class TextureCubeArraySpecCase : public TextureSpecCase
{
public:
							TextureCubeArraySpecCase	(Context& context, const char* name, const char* desc, const tcu::TextureFormat& format, int size, int depth, int numLevels);
							~TextureCubeArraySpecCase	(void);

protected:
	virtual bool			checkExtensionSupport		(void);
	virtual void			verifyTexture				(sglr::GLContext& gles3Context, sglr::ReferenceContext& refContext);

	tcu::TextureFormat		m_texFormat;
	tcu::TextureFormatInfo	m_texFormatInfo;
	int						m_size;
	int						m_depth;
	int						m_numLevels;
};

TextureCubeArraySpecCase::TextureCubeArraySpecCase (Context& context, const char* name, const char* desc, const tcu::TextureFormat& format, int size, int depth, int numLevels)
	: TextureSpecCase		(context, name, desc)
	, m_texFormat			(format)
	, m_texFormatInfo		(tcu::getTextureFormatInfo(format))
	, m_size				(size)
	, m_depth				(depth)
	, m_numLevels			(numLevels)
{
}

TextureCubeArraySpecCase::~TextureCubeArraySpecCase (void)
{
}

bool TextureCubeArraySpecCase::checkExtensionSupport (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	return supportsES32 || m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_cube_map_array");
}

void TextureCubeArraySpecCase::verifyTexture (sglr::GLContext& gles3Context, sglr::ReferenceContext& refContext)
{
	const glu::GLSLVersion	glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());
	TextureCubeArrayShader	shader			(glu::getSamplerCubeArrayType(m_texFormat), glu::TYPE_FLOAT_VEC4, glslVersion);
	deUint32				shaderIDgles	= gles3Context.createProgram(&shader);
	deUint32				shaderIDRef		= refContext.createProgram(&shader);

	shader.setTexScaleBias(m_texFormatInfo.lookupScale, m_texFormatInfo.lookupBias);

	// Set state.
	for (int ndx = 0; ndx < 2; ndx++)
	{
		sglr::Context* ctx = ndx ? static_cast<sglr::Context*>(&refContext) : static_cast<sglr::Context*>(&gles3Context);

		setContext(ctx);

		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER,	GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER,	GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S,		GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T,		GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R,		GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL,	m_numLevels-1);
	}

	for (int layerFaceNdx = 0; layerFaceNdx < m_depth; layerFaceNdx++)
	{
		const int			layerNdx	= layerFaceNdx / 6;
		const tcu::CubeFace	face		= getCubeFaceFromNdx(layerFaceNdx % 6);
		bool				layerOk		= true;

		shader.setLayer(layerNdx);
		shader.setFace(face);

		for (int levelNdx = 0; levelNdx < m_numLevels; levelNdx++)
		{
			int				levelSize	= de::max(1, m_size	>> levelNdx);
			tcu::Surface	reference;
			tcu::Surface	result;

			if (levelSize <= 2)
				continue; // Fuzzy compare doesn't work for images this small.

			for (int ndx = 0; ndx < 2; ndx++)
			{
				tcu::Surface&	dst			= ndx ? reference									: result;
				sglr::Context*	ctx			= ndx ? static_cast<sglr::Context*>(&refContext)	: static_cast<sglr::Context*>(&gles3Context);
				deUint32		shaderID	= ndx ? shaderIDRef									: shaderIDgles;

				setContext(ctx);
				shader.setUniforms(*ctx, shaderID);
				renderTex(dst, shaderID, levelSize, levelSize);
			}

			const float		threshold		= 0.02f;
			string			levelStr		= de::toString(levelNdx);
			string			layerFaceStr	= de::toString(layerFaceNdx);
			string			name			= string("LayerFace") + layerFaceStr + "Level" + levelStr;
			string			desc			= string("Layer-face ") + layerFaceStr + ", Level " + levelStr;
			bool			isFaceOk		= tcu::fuzzyCompare(m_testCtx.getLog(), name.c_str(), desc.c_str(), reference, result, threshold,
																(levelNdx == 0 && layerFaceNdx == 0) == 0 ? tcu::COMPARE_LOG_RESULT : tcu::COMPARE_LOG_ON_ERROR);

			if (!isFaceOk)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
				layerOk = false;
				break;
			}
		}

		if (!layerOk)
			break;
	}
}

// Basic TexImage3D() with cube map array texture usage
class BasicTexImageCubeArrayCase : public TextureCubeArraySpecCase
{
public:
	BasicTexImageCubeArrayCase (Context& context, const char* name, const char* desc, deUint32 internalFormat, int size, int numLayers)
		: TextureCubeArraySpecCase	(context, name, desc, glu::mapGLInternalFormat(internalFormat), size, numLayers, maxLevelCount(size))
		, m_internalFormat			(internalFormat)
	{
	}

protected:
	void createTexture (void)
	{
		deUint32				tex			= 0;
		de::Random				rnd			(deStringHash(getName()));
		glu::TransferFormat		transferFmt	= glu::getTransferFormat(m_texFormat);
		tcu::TextureLevel		levelData	(glu::mapGLTransferFormat(transferFmt.format, transferFmt.dataType));

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		for (int ndx = 0; ndx < m_numLevels; ndx++)
		{
			int		levelW		= de::max(1, m_size	>> ndx);
			Vec4	gMin		= randomVector<4>(rnd, m_texFormatInfo.valueMin, m_texFormatInfo.valueMax);
			Vec4	gMax		= randomVector<4>(rnd, m_texFormatInfo.valueMin, m_texFormatInfo.valueMax);

			levelData.setSize(levelW, levelW, m_depth);
			tcu::fillWithComponentGradients(levelData.getAccess(), gMin, gMax);

			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, ndx, m_internalFormat, levelW, levelW, m_depth, 0, transferFmt.format, transferFmt.dataType, levelData.getAccess().getDataPtr());
		}
	}

	deUint32 m_internalFormat;
};

// Basic glTexStorage3D() with cube map array texture usage
class BasicTexStorageCubeArrayCase : public TextureCubeArraySpecCase
{
public:
	BasicTexStorageCubeArrayCase (Context& context, const char* name, const char* desc, deUint32 internalFormat, int size, int numLayers, int numLevels)
		: TextureCubeArraySpecCase	(context, name, desc, glu::mapGLInternalFormat(internalFormat), size, numLayers, numLevels)
		, m_internalFormat			(internalFormat)
	{
	}

protected:
	void createTexture (void)
	{
		deUint32				tex			= 0;
		de::Random				rnd			(deStringHash(getName()));
		glu::TransferFormat		transferFmt	= glu::getTransferFormat(m_texFormat);
		tcu::TextureLevel		levelData	(glu::mapGLTransferFormat(transferFmt.format, transferFmt.dataType));

		glGenTextures	(1, &tex);
		glBindTexture	(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glTexStorage3D	(GL_TEXTURE_CUBE_MAP_ARRAY, m_numLevels, m_internalFormat, m_size, m_size, m_depth);

		glPixelStorei	(GL_UNPACK_ALIGNMENT, 1);

		for (int ndx = 0; ndx < m_numLevels; ndx++)
		{
			int		levelW		= de::max(1, m_size	>> ndx);
			Vec4	gMin		= randomVector<4>(rnd, m_texFormatInfo.valueMin, m_texFormatInfo.valueMax);
			Vec4	gMax		= randomVector<4>(rnd, m_texFormatInfo.valueMin, m_texFormatInfo.valueMax);

			levelData.setSize(levelW, levelW, m_depth);
			tcu::fillWithComponentGradients(levelData.getAccess(), gMin, gMax);

			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, ndx, 0, 0, 0, levelW, levelW, m_depth, transferFmt.format, transferFmt.dataType, levelData.getAccess().getDataPtr());
		}
	}

	deUint32 m_internalFormat;
};

// Pixel buffer object cases.

// TexImage3D() cube map array from pixel buffer object.
class TexImageCubeArrayBufferCase : public TextureCubeArraySpecCase
{
public:
	TexImageCubeArrayBufferCase (Context&		context,
							   const char*	name,
							   const char*	desc,
							   deUint32		internalFormat,
							   int			size,
							   int			depth,
							   int			imageHeight,
							   int			rowLength,
							   int			skipImages,
							   int			skipRows,
							   int			skipPixels,
							   int			alignment,
							   int			offset)
		: TextureCubeArraySpecCase	(context, name, desc, glu::mapGLInternalFormat(internalFormat), size, depth, 1)
		, m_internalFormat			(internalFormat)
		, m_imageHeight				(imageHeight)
		, m_rowLength				(rowLength)
		, m_skipImages				(skipImages)
		, m_skipRows				(skipRows)
		, m_skipPixels				(skipPixels)
		, m_alignment				(alignment)
		, m_offset					(offset)
	{
	}

protected:
	void createTexture (void)
	{
		glu::TransferFormat		transferFmt		= glu::getTransferFormat(m_texFormat);
		int						pixelSize		= m_texFormat.getPixelSize();
		int						rowLength		= m_rowLength > 0 ? m_rowLength : m_size;
		int						rowPitch		= deAlign32(rowLength*pixelSize, m_alignment);
		int						imageHeight		= m_imageHeight > 0 ? m_imageHeight : m_size;
		int						slicePitch		= imageHeight*rowPitch;
		deUint32				tex				= 0;
		deUint32				buf				= 0;
		vector<deUint8>			data;

		DE_ASSERT(m_numLevels == 1);

		// Fill data with grid.
		data.resize(slicePitch*(m_depth+m_skipImages) + m_offset);
		{
			Vec4	cScale		= m_texFormatInfo.valueMax-m_texFormatInfo.valueMin;
			Vec4	cBias		= m_texFormatInfo.valueMin;
			Vec4	colorA		= Vec4(1.0f, 0.0f, 0.0f, 1.0f)*cScale + cBias;
			Vec4	colorB		= Vec4(0.0f, 1.0f, 0.0f, 1.0f)*cScale + cBias;

			tcu::fillWithGrid(tcu::PixelBufferAccess(m_texFormat, m_size, m_size, m_depth, rowPitch, slicePitch, &data[0] + m_skipImages*slicePitch + m_skipRows*rowPitch + m_skipPixels*pixelSize + m_offset), 4, colorA, colorB);
		}

		glGenBuffers(1, &buf);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, (int)data.size(), &data[0], GL_STATIC_DRAW);

		glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,	m_imageHeight);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,		m_rowLength);
		glPixelStorei(GL_UNPACK_SKIP_IMAGES,	m_skipImages);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,		m_skipRows);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,	m_skipPixels);
		glPixelStorei(GL_UNPACK_ALIGNMENT,		m_alignment);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, m_internalFormat, m_size, m_size, m_depth, 0, transferFmt.format, transferFmt.dataType, (const void*)(deUintptr)m_offset);
	}

	deUint32	m_internalFormat;
	int			m_imageHeight;
	int			m_rowLength;
	int			m_skipImages;
	int			m_skipRows;
	int			m_skipPixels;
	int			m_alignment;
	int			m_offset;
};

// TexSubImage3D() cube map array PBO case.
class TexSubImageCubeArrayBufferCase : public TextureCubeArraySpecCase
{
public:
	TexSubImageCubeArrayBufferCase (Context&		context,
								 const char*	name,
								 const char*	desc,
								 deUint32		internalFormat,
								 int			size,
								 int			depth,
								 int			subX,
								 int			subY,
								 int			subZ,
								 int			subW,
								 int			subH,
								 int			subD,
								 int			imageHeight,
								 int			rowLength,
								 int			skipImages,
								 int			skipRows,
								 int			skipPixels,
								 int			alignment,
								 int			offset)
		: TextureCubeArraySpecCase	(context, name, desc, glu::mapGLInternalFormat(internalFormat), size, depth, 1)
		, m_internalFormat			(internalFormat)
		, m_subX					(subX)
		, m_subY					(subY)
		, m_subZ					(subZ)
		, m_subW					(subW)
		, m_subH					(subH)
		, m_subD					(subD)
		, m_imageHeight				(imageHeight)
		, m_rowLength				(rowLength)
		, m_skipImages				(skipImages)
		, m_skipRows				(skipRows)
		, m_skipPixels				(skipPixels)
		, m_alignment				(alignment)
		, m_offset					(offset)
	{
	}

protected:
	void createTexture (void)
	{
		glu::TransferFormat		transferFmt		= glu::getTransferFormat(m_texFormat);
		int						pixelSize		= m_texFormat.getPixelSize();
		deUint32				tex				= 0;
		deUint32				buf				= 0;
		vector<deUint8>			data;

		DE_ASSERT(m_numLevels == 1);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);

		// Fill with gradient.
		{
			int		rowPitch		= deAlign32(pixelSize*m_size,  4);
			int		slicePitch		= rowPitch*m_size;

			data.resize(slicePitch*m_depth);
			tcu::fillWithComponentGradients(tcu::PixelBufferAccess(m_texFormat, m_size, m_size, m_depth, rowPitch, slicePitch, &data[0]), m_texFormatInfo.valueMin, m_texFormatInfo.valueMax);
		}

		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, m_internalFormat, m_size, m_size, m_depth, 0, transferFmt.format, transferFmt.dataType, &data[0]);

		// Fill data with grid.
		{
			int		rowLength		= m_rowLength > 0 ? m_rowLength : m_subW;
			int		rowPitch		= deAlign32(rowLength*pixelSize, m_alignment);
			int		imageHeight		= m_imageHeight > 0 ? m_imageHeight : m_subH;
			int		slicePitch		= imageHeight*rowPitch;
			Vec4	cScale			= m_texFormatInfo.valueMax-m_texFormatInfo.valueMin;
			Vec4	cBias			= m_texFormatInfo.valueMin;
			Vec4	colorA			= Vec4(1.0f, 0.0f, 0.0f, 1.0f)*cScale + cBias;
			Vec4	colorB			= Vec4(0.0f, 1.0f, 0.0f, 1.0f)*cScale + cBias;

			data.resize(slicePitch*(m_depth+m_skipImages) + m_offset);
			tcu::fillWithGrid(tcu::PixelBufferAccess(m_texFormat, m_subW, m_subH, m_subD, rowPitch, slicePitch, &data[0] + m_skipImages*slicePitch + m_skipRows*rowPitch + m_skipPixels*pixelSize + m_offset), 4, colorA, colorB);
		}

		glGenBuffers(1, &buf);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER,	buf);
		glBufferData(GL_PIXEL_UNPACK_BUFFER,	(int)data.size(), &data[0], GL_STATIC_DRAW);

		glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,	m_imageHeight);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,		m_rowLength);
		glPixelStorei(GL_UNPACK_SKIP_IMAGES,	m_skipImages);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,		m_skipRows);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,	m_skipPixels);
		glPixelStorei(GL_UNPACK_ALIGNMENT,		m_alignment);
		glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, m_subX, m_subY, m_subZ, m_subW, m_subH, m_subD, transferFmt.format, transferFmt.dataType, (const void*)(deIntptr)m_offset);
	}

	deUint32	m_internalFormat;
	int			m_subX;
	int			m_subY;
	int			m_subZ;
	int			m_subW;
	int			m_subH;
	int			m_subD;
	int			m_imageHeight;
	int			m_rowLength;
	int			m_skipImages;
	int			m_skipRows;
	int			m_skipPixels;
	int			m_alignment;
	int			m_offset;
};

// TexImage3D() depth case.
class TexImageCubeArrayDepthCase : public TextureCubeArraySpecCase
{
public:
	TexImageCubeArrayDepthCase (Context&	context,
							  const char*	name,
							  const char*	desc,
							  deUint32		internalFormat,
							  int			imageSize,
							  int			numLayers)
		: TextureCubeArraySpecCase(context, name, desc, glu::mapGLInternalFormat(internalFormat), imageSize, numLayers, maxLevelCount(imageSize))
		, m_internalFormat		(internalFormat)
	{
		// we are interested in the behavior near [-2, 2], map it to visible range [0, 1]
		m_texFormatInfo.lookupBias = Vec4(0.25f, 0.0f, 0.0f, 1.0f);
		m_texFormatInfo.lookupScale = Vec4(0.5f, 1.0f, 1.0f, 0.0f);
	}

	void createTexture (void)
	{
		glu::TransferFormat	fmt			= glu::getTransferFormat(m_texFormat);
		deUint32			tex			= 0;
		tcu::TextureLevel	levelData	(glu::mapGLTransferFormat(fmt.format, fmt.dataType));

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		GLU_CHECK();

		for (int ndx = 0; ndx < m_numLevels; ndx++)
		{
			const int   levelW		= de::max(1, m_size >> ndx);
			const Vec4  gMin		= Vec4(-1.5f, -2.0f, 1.7f, -1.5f);
			const Vec4  gMax		= Vec4(2.0f, 1.5f, -1.0f, 2.0f);

			levelData.setSize(levelW, levelW, m_depth);
			tcu::fillWithComponentGradients(levelData.getAccess(), gMin, gMax);

			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, ndx, m_internalFormat, levelW, levelW, m_depth, 0, fmt.format, fmt.dataType, levelData.getAccess().getDataPtr());
		}
	}

	const deUint32 m_internalFormat;
};

// TexSubImage3D() depth case.
class TexSubImageCubeArrayDepthCase : public TextureCubeArraySpecCase
{
public:
	TexSubImageCubeArrayDepthCase (Context&		context,
								 const char*	name,
								 const char*	desc,
								 deUint32		internalFormat,
								 int			imageSize,
								 int			numLayers)
		: TextureCubeArraySpecCase(context, name, desc, glu::mapGLInternalFormat(internalFormat), imageSize, numLayers, maxLevelCount(imageSize))
		, m_internalFormat		(internalFormat)
	{
		// we are interested in the behavior near [-2, 2], map it to visible range [0, 1]
		m_texFormatInfo.lookupBias = Vec4(0.25f, 0.0f, 0.0f, 1.0f);
		m_texFormatInfo.lookupScale = Vec4(0.5f, 1.0f, 1.0f, 0.0f);
	}

	void createTexture (void)
	{
		glu::TransferFormat	fmt			= glu::getTransferFormat(m_texFormat);
		de::Random			rnd			(deStringHash(getName()));
		deUint32			tex			= 0;
		tcu::TextureLevel	levelData	(glu::mapGLTransferFormat(fmt.format, fmt.dataType));

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		GLU_CHECK();

		// First specify full texture.
		for (int ndx = 0; ndx < m_numLevels; ndx++)
		{
			const int   levelW		= de::max(1, m_size >> ndx);
			const Vec4  gMin		= Vec4(-1.5f, -2.0f, 1.7f, -1.5f);
			const Vec4  gMax		= Vec4(2.0f, 1.5f, -1.0f, 2.0f);

			levelData.setSize(levelW, levelW, m_depth);
			tcu::fillWithComponentGradients(levelData.getAccess(), gMin, gMax);

			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, ndx, m_internalFormat, levelW, levelW, m_depth, 0, fmt.format, fmt.dataType, levelData.getAccess().getDataPtr());
		}

		// Re-specify parts of each level.
		for (int ndx = 0; ndx < m_numLevels; ndx++)
		{
			const int	levelW		= de::max(1, m_size >> ndx);

			const int	w			= rnd.getInt(1, levelW);
			const int	h			= rnd.getInt(1, levelW);
			const int	d			= rnd.getInt(1, m_depth);
			const int	x			= rnd.getInt(0, levelW-w);
			const int	y			= rnd.getInt(0, levelW-h);
			const int	z			= rnd.getInt(0, m_depth-d);

			const Vec4	colorA		= Vec4(2.0f, 1.5f, -1.0f, 2.0f);
			const Vec4	colorB		= Vec4(-1.5f, -2.0f, 1.7f, -1.5f);
			const int	cellSize	= rnd.getInt(2, 16);

			levelData.setSize(w, h, d);
			tcu::fillWithGrid(levelData.getAccess(), cellSize, colorA, colorB);

			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, ndx, x, y, z, w, h, d, fmt.format, fmt.dataType, levelData.getAccess().getDataPtr());
		}
	}

	const deUint32 m_internalFormat;
};

// TexImage3D() depth case with pbo.
class TexImageCubeArrayDepthBufferCase : public TextureCubeArraySpecCase
{
public:
	TexImageCubeArrayDepthBufferCase (Context&	context,
									const char*	name,
									const char*	desc,
									deUint32	internalFormat,
									int			imageSize,
									int			numLayers)
		: TextureCubeArraySpecCase(context, name, desc, glu::mapGLInternalFormat(internalFormat), imageSize, numLayers, 1)
		, m_internalFormat		(internalFormat)
	{
		// we are interested in the behavior near [-2, 2], map it to visible range [0, 1]
		m_texFormatInfo.lookupBias = Vec4(0.25f, 0.0f, 0.0f, 1.0f);
		m_texFormatInfo.lookupScale = Vec4(0.5f, 1.0f, 1.0f, 0.0f);
	}

	void createTexture (void)
	{
		glu::TransferFormat		transferFmt		= glu::getTransferFormat(m_texFormat);
		int						pixelSize		= m_texFormat.getPixelSize();
		int						rowLength		= m_size;
		int						alignment		= 4;
		int						rowPitch		= deAlign32(rowLength*pixelSize, alignment);
		int						imageHeight		= m_size;
		int						slicePitch		= imageHeight*rowPitch;
		deUint32				tex				= 0;
		deUint32				buf				= 0;
		vector<deUint8>			data;

		DE_ASSERT(m_numLevels == 1);

		// Fill data with grid.
		data.resize(slicePitch*m_depth);
		{
			const Vec4 gMin = Vec4(-1.5f, -2.0f, 1.7f, -1.5f);
			const Vec4 gMax = Vec4(2.0f, 1.5f, -1.0f, 2.0f);

			tcu::fillWithComponentGradients(tcu::PixelBufferAccess(m_texFormat, m_size, m_size, m_depth, rowPitch, slicePitch, &data[0]), gMin, gMax);
		}

		glGenBuffers(1, &buf);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, (int)data.size(), &data[0], GL_STATIC_DRAW);

		glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,	imageHeight);
		glPixelStorei(GL_UNPACK_ROW_LENGTH,		rowLength);
		glPixelStorei(GL_UNPACK_SKIP_IMAGES,	0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS,		0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS,	0);
		glPixelStorei(GL_UNPACK_ALIGNMENT,		alignment);

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
		glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, m_internalFormat, m_size, m_size, m_depth, 0, transferFmt.format, transferFmt.dataType, DE_NULL);
		glDeleteBuffers(1, &buf);
	}

	const deUint32 m_internalFormat;
};

TextureSpecificationTests::TextureSpecificationTests (Context& context)
	: TestCaseGroup(context, "specification", "Texture Specification Tests")
{
}

TextureSpecificationTests::~TextureSpecificationTests (void)
{
}

void TextureSpecificationTests::init (void)
{
	struct
	{
		const char*	name;
		deUint32	internalFormat;
	} colorFormats[] =
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

	static const struct
	{
		const char*	name;
		deUint32	internalFormat;
	} depthStencilFormats[] =
	{
		// Depth and stencil formats
		{ "depth_component32f",	GL_DEPTH_COMPONENT32F	},
		{ "depth_component24",	GL_DEPTH_COMPONENT24	},
		{ "depth_component16",	GL_DEPTH_COMPONENT16	},
		{ "depth32f_stencil8",	GL_DEPTH32F_STENCIL8	},
		{ "depth24_stencil8",	GL_DEPTH24_STENCIL8		}
	};

	// Basic TexImage3D usage.
	{
		tcu::TestCaseGroup* basicTexImageGroup = new tcu::TestCaseGroup(m_testCtx, "basic_teximage3d", "Basic glTexImage3D() usage");
		addChild(basicTexImageGroup);
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(colorFormats); formatNdx++)
		{
			const char*	fmtName				= colorFormats[formatNdx].name;
			deUint32	format				= colorFormats[formatNdx].internalFormat;
			const int	texCubeArraySize	= 64;
			const int	texCubeArrayLayers	= 6;

			basicTexImageGroup->addChild(new BasicTexImageCubeArrayCase	(m_context,	(string(fmtName) + "_cube_array").c_str(),	"",	format, texCubeArraySize, texCubeArrayLayers));
		}
	}

	// glTexImage3D() pbo cases.
	{
		tcu::TestCaseGroup* pboGroup = new tcu::TestCaseGroup(m_testCtx, "teximage3d_pbo", "glTexImage3D() from PBO");
		addChild(pboGroup);

		// Parameter cases
		static const struct
		{
			const char*	name;
			deUint32	format;
			int			size;
			int			depth;
			int			imageHeight;
			int			rowLength;
			int			skipImages;
			int			skipRows;
			int			skipPixels;
			int			alignment;
			int			offset;
		} parameterCases[] =
		{
			{ "rgb8_offset",		GL_RGB8,	23,	6,	0,	0,	0,	0,	0,	1,	67 },
			{ "rgb8_alignment",		GL_RGB8,	23,	6,	0,	0,	0,	0,	0,	2,	0 },
			{ "rgb8_image_height",	GL_RGB8,	23,	6,	26,	0,	0,	0,	0,	4,	0 },
			{ "rgb8_row_length",	GL_RGB8,	23,	6,	0,	27,	0,	0,	0,	4,	0 },
			{ "rgb8_skip_images",	GL_RGB8,	23,	6,	0,	0,	3,	0,	0,	4,	0 },
			{ "rgb8_skip_rows",		GL_RGB8,	23,	6,	26,	0,	0,	3,	0,	4,	0 },
			{ "rgb8_skip_pixels",	GL_RGB8,	23,	6,	0,	25,	0,	0,	2,	4,	0 }
		};

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(colorFormats); formatNdx++)
		{
			const string	fmtName				= colorFormats[formatNdx].name;
			const deUint32	format				= colorFormats[formatNdx].internalFormat;
			const int		texCubeArraySize	= 20;
			const int		texCubeDepth		= 6;

			pboGroup->addChild(new TexImageCubeArrayBufferCase	(m_context, (fmtName + "_cube_array").c_str(),	"", format, texCubeArraySize, texCubeDepth, 0, 0, 0, 0, 0, 4, 0));
		}

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(parameterCases); ndx++)
		{
			pboGroup->addChild(new TexImageCubeArrayBufferCase(m_context, (string(parameterCases[ndx].name) + "_cube_array").c_str(), "",
														parameterCases[ndx].format,
														parameterCases[ndx].size,
														parameterCases[ndx].depth,
														parameterCases[ndx].imageHeight,
														parameterCases[ndx].rowLength,
														parameterCases[ndx].skipImages,
														parameterCases[ndx].skipRows,
														parameterCases[ndx].skipPixels,
														parameterCases[ndx].alignment,
														parameterCases[ndx].offset));
		}
	}

	// glTexImage3D() depth cases.
	{
		tcu::TestCaseGroup* shadow3dGroup = new tcu::TestCaseGroup(m_testCtx, "teximage3d_depth", "glTexImage3D() with depth or depth/stencil format");
		addChild(shadow3dGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(depthStencilFormats); ndx++)
		{
			const int	texCubeArraySize	= 64;
			const int	texCubeArrayDepth	= 6;

			shadow3dGroup->addChild(new TexImageCubeArrayDepthCase(m_context, (std::string(depthStencilFormats[ndx].name) + "_cube_array").c_str(), "", depthStencilFormats[ndx].internalFormat, texCubeArraySize, texCubeArrayDepth));
		}
	}

	// glTexImage3D() depth cases with pbo.
	{
		tcu::TestCaseGroup* shadow3dGroup = new tcu::TestCaseGroup(m_testCtx, "teximage3d_depth_pbo", "glTexImage3D() with depth or depth/stencil format with pbo");
		addChild(shadow3dGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(depthStencilFormats); ndx++)
		{
			const int	texCubeArraySize	= 64;
			const int	texCubeArrayDepth	= 6;

			shadow3dGroup->addChild(new TexImageCubeArrayDepthBufferCase(m_context, (std::string(depthStencilFormats[ndx].name) + "_cube_array").c_str(), "", depthStencilFormats[ndx].internalFormat, texCubeArraySize, texCubeArrayDepth));
		}
	}

	// glTexSubImage3D() PBO cases.
	{
		tcu::TestCaseGroup* pboGroup = new tcu::TestCaseGroup(m_testCtx, "texsubimage3d_pbo", "glTexSubImage3D() pixel buffer object tests");
		addChild(pboGroup);

		static const struct
		{
			const char*	name;
			deUint32	format;
			int			size;
			int			depth;
			int			subX;
			int			subY;
			int			subZ;
			int			subW;
			int			subH;
			int			subD;
			int			imageHeight;
			int			rowLength;
			int			skipImages;
			int			skipRows;
			int			skipPixels;
			int			alignment;
			int			offset;
		} paramCases[] =
		{
			{ "rgb8_offset",		GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	0,	0,	0,	0,	0,	4,	67 },
			{ "rgb8_image_height",	GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	26,	0,	0,	0,	0,	4,	0 },
			{ "rgb8_row_length",	GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	0,	27,	0,	0,	0,	4,	0 },
			{ "rgb8_skip_images",	GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	0,	0,	3,	0,	0,	4,	0 },
			{ "rgb8_skip_rows",		GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	22,	0,	0,	3,	0,	4,	0 },
			{ "rgb8_skip_pixels",	GL_RGB8,	26, 12,	1,	2,	1,	23,	19,	8,	0,	25,	0,	0,	2,	4,	0 }
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(colorFormats); ndx++)
		{
			pboGroup->addChild(new TexSubImageCubeArrayBufferCase(m_context, (std::string(colorFormats[ndx].name) + "_cube_array").c_str(), "",
														   colorFormats[ndx].internalFormat,
														   26,	// Size
														   12,	// Depth
														   1,	// Sub X
														   2,	// Sub Y
														   0,	// Sub Z
														   23,	// Sub W
														   19,	// Sub H
														   8,	// Sub D
														   0,	// Image height
														   0,	// Row length
														   0,	// Skip images
														   0,	// Skip rows
														   0,	// Skip pixels
														   4,	// Alignment
														   0	/* offset */));
		}

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(paramCases); ndx++)
		{
			pboGroup->addChild(new TexSubImageCubeArrayBufferCase(m_context, (std::string(paramCases[ndx].name) + "_cube_array").c_str(), "",
														   paramCases[ndx].format,
														   paramCases[ndx].size,
														   paramCases[ndx].depth,
														   paramCases[ndx].subX,
														   paramCases[ndx].subY,
														   paramCases[ndx].subZ,
														   paramCases[ndx].subW,
														   paramCases[ndx].subH,
														   paramCases[ndx].subD,
														   paramCases[ndx].imageHeight,
														   paramCases[ndx].rowLength,
														   paramCases[ndx].skipImages,
														   paramCases[ndx].skipRows,
														   paramCases[ndx].skipPixels,
														   paramCases[ndx].alignment,
														   paramCases[ndx].offset));
		}
	}

	// glTexSubImage3D() depth cases.
	{
		tcu::TestCaseGroup* shadow3dGroup = new tcu::TestCaseGroup(m_testCtx, "texsubimage3d_depth", "glTexSubImage3D() with depth or depth/stencil format");
		addChild(shadow3dGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(depthStencilFormats); ndx++)
		{
			const int	texCubeArraySize	= 57;
			const int	texCubeArrayLayers	= 6;

			shadow3dGroup->addChild(new TexSubImageCubeArrayDepthCase(m_context, (std::string(depthStencilFormats[ndx].name) + "_cube_array").c_str(), "", depthStencilFormats[ndx].internalFormat, texCubeArraySize, texCubeArrayLayers));
		}
	}

	// glTexStorage3D() cases.
	{
		tcu::TestCaseGroup* texStorageGroup = new tcu::TestCaseGroup(m_testCtx, "texstorage3d", "Basic glTexStorage3D() usage");
		addChild(texStorageGroup);

		// All formats.
		tcu::TestCaseGroup* formatGroup = new tcu::TestCaseGroup(m_testCtx, "format", "glTexStorage3D() with all formats");
		texStorageGroup->addChild(formatGroup);

		// Color formats.
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(colorFormats); formatNdx++)
		{
			const char*	fmtName				= colorFormats[formatNdx].name;
			deUint32	internalFormat		= colorFormats[formatNdx].internalFormat;
			const int	texCubeArraySize	= 57;
			const int	texCubeArrayLayers	= 6;
			int			texCubeArrayLevels	= maxLevelCount(texCubeArraySize);

			formatGroup->addChild(new BasicTexStorageCubeArrayCase	(m_context, (string(fmtName) + "_cube_array").c_str(),	"", internalFormat, texCubeArraySize, texCubeArrayLayers, texCubeArrayLevels));
		}

		// Depth/stencil formats (only 2D texture array is supported).
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(depthStencilFormats); formatNdx++)
		{
			const char*	fmtName				= depthStencilFormats[formatNdx].name;
			deUint32	internalFormat		= depthStencilFormats[formatNdx].internalFormat;
			const int	texCubeArraySize	= 57;
			const int	texCubeArrayLayers	= 6;
			int			texCubeArrayLevels	= maxLevelCount(texCubeArraySize);

			formatGroup->addChild(new BasicTexStorageCubeArrayCase	(m_context, (string(fmtName) + "_cube_array").c_str(),	"", internalFormat, texCubeArraySize, texCubeArrayLayers, texCubeArrayLevels));
		}

		// Sizes.
		static const struct
		{
			int				size;
			int				layers;
			int				levels;
		} texCubeArraySizes[] =
		{
			//	Sz	La	Le
			{	1,	6,	1 },
			{	2,	6,	2 },
			{	32,	6,	3 },
			{	64,	6,	4 },
			{	57,	12,	1 },
			{	57,	12,	2 },
			{	57,	12,	6 }
		};

		tcu::TestCaseGroup* sizeGroup = new tcu::TestCaseGroup(m_testCtx, "size", "glTexStorage3D() with various sizes");
		texStorageGroup->addChild(sizeGroup);

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(texCubeArraySizes); ndx++)
		{
			const deUint32		format		= GL_RGBA8;
			int					size		= texCubeArraySizes[ndx].size;
			int					layers		= texCubeArraySizes[ndx].layers;
			int					levels		= texCubeArraySizes[ndx].levels;
			string				name		= string("cube_array_") + de::toString(size) + "x" + de::toString(size) + "x" + de::toString(layers) + "_" + de::toString(levels) + "_levels";

			sizeGroup->addChild(new BasicTexStorageCubeArrayCase(m_context, name.c_str(), "", format, size, layers, levels));
		}
	}
}

} // Functional
} // gles3
} // deqp
