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
 * \brief FBO colorbuffer tests.
 *//*--------------------------------------------------------------------*/

#include "es31fFboColorbufferTests.hpp"
#include "es31fFboTestCase.hpp"
#include "es31fFboTestUtil.hpp"

#include "gluTextureUtil.hpp"
#include "gluContextInfo.hpp"

#include "tcuCommandLine.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRGBA.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"

#include "sglrContextUtil.hpp"

#include "deRandom.hpp"
#include "deString.h"

#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::string;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::UVec4;
using tcu::TestLog;
using namespace FboTestUtil;

const tcu::RGBA MIN_THRESHOLD(12, 12, 12, 12);

static tcu::Vec4 generateRandomColor (de::Random& random)
{
	tcu::Vec4 retVal;

	retVal[0] = random.getFloat();
	retVal[1] = random.getFloat();
	retVal[2] = random.getFloat();
	retVal[3] = 1.0f;

	return retVal;
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

class FboColorbufferCase : public FboTestCase
{
public:
	FboColorbufferCase (Context& context, const char* name, const char* desc, const deUint32 format)
		: FboTestCase	(context, name, desc)
		, m_format		(format)
	{
	}

	bool compare (const tcu::Surface& reference, const tcu::Surface& result)
	{
		const tcu::RGBA threshold (tcu::max(getFormatThreshold(m_format), MIN_THRESHOLD));

		m_testCtx.getLog() << TestLog::Message << "Comparing images, threshold: " << threshold << TestLog::EndMessage;

		return tcu::bilinearCompare(m_testCtx.getLog(), "Result", "Image comparison result", reference.getAccess(), result.getAccess(), threshold, tcu::COMPARE_LOG_RESULT);
	}

protected:
	const deUint32	m_format;
};

class FboColorTexCubeArrayCase : public FboColorbufferCase
{
public:
	FboColorTexCubeArrayCase (Context& context, const char* name, const char* description, deUint32 texFmt, const IVec3& texSize)
		: FboColorbufferCase	(context, name, description, texFmt)
		, m_texSize				(texSize)
	{
		DE_ASSERT(texSize.z() % 6 == 0);
	}

protected:
	void preCheck (void)
	{
		if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_cube_map_array"))
			TCU_THROW(NotSupportedError, "Test requires extension GL_EXT_texture_cube_map_array or a context version equal or higher than 3.2");

		checkFormatSupport(m_format);
	}

	void render (tcu::Surface& dst)
	{
		TestLog&				log					= m_testCtx.getLog();
		de::Random				rnd					(deStringHash(getName()) ^ 0xed607a89 ^ m_testCtx.getCommandLine().getBaseSeed());
		tcu::TextureFormat		texFmt				= glu::mapGLInternalFormat(m_format);
		tcu::TextureFormatInfo	fmtInfo				= tcu::getTextureFormatInfo(texFmt);

		Texture2DShader			texToFboShader		(DataTypes() << glu::TYPE_SAMPLER_2D, getFragmentOutputType(texFmt), fmtInfo.valueMax-fmtInfo.valueMin, fmtInfo.valueMin);
		TextureCubeArrayShader	arrayTexShader		(glu::getSamplerCubeArrayType(texFmt), glu::TYPE_FLOAT_VEC4, glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()));

		deUint32				texToFboShaderID	= getCurrentContext()->createProgram(&texToFboShader);
		deUint32				arrayTexShaderID	= getCurrentContext()->createProgram(&arrayTexShader);

		// Setup textures
		texToFboShader.setUniforms(*getCurrentContext(), texToFboShaderID);
		arrayTexShader.setTexScaleBias(fmtInfo.lookupScale, fmtInfo.lookupBias);

		// Framebuffers.
		std::vector<deUint32>	fbos;
		deUint32				tex;

		{
			glu::TransferFormat	transferFmt		= glu::getTransferFormat(texFmt);
			bool				isFilterable	= glu::isGLInternalColorFormatFilterable(m_format);
			const IVec3&		size			= m_texSize;

			log << TestLog::Message
				<< "Creating a cube map array texture ("
				<< size.x() << "x" << size.y()
				<< ", depth: "
				<< size.z() << ")"
				<< TestLog::EndMessage;

			glGenTextures(1, &tex);

			glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY,	GL_TEXTURE_WRAP_S,		GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY,	GL_TEXTURE_WRAP_T,		GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY,	GL_TEXTURE_WRAP_R,		GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY,	GL_TEXTURE_MIN_FILTER,	isFilterable ? GL_LINEAR : GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY,	GL_TEXTURE_MAG_FILTER,	isFilterable ? GL_LINEAR : GL_NEAREST);
			glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, m_format, size.x(), size.y(), size.z(), 0, transferFmt.format, transferFmt.dataType, DE_NULL);

			log << TestLog::Message << "Creating a framebuffer object for each layer-face" << TestLog::EndMessage;

			// Generate an FBO for each layer-face
			for (int ndx = 0; ndx < m_texSize.z(); ndx++)
			{
				deUint32 layerFbo;

				glGenFramebuffers(1, &layerFbo);
				glBindFramebuffer(GL_FRAMEBUFFER, layerFbo);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0, ndx);
				checkError();
				checkFramebufferStatus(GL_FRAMEBUFFER);

				fbos.push_back(layerFbo);
			}
		}

		log << TestLog::Message << "Rendering test images to layer-faces in randomized order" << TestLog::EndMessage;

		{
			std::vector<int> order(fbos.size());

			for (size_t n = 0; n < order.size(); n++)
				order[n] = (int)n;
			rnd.shuffle(order.begin(), order.end());

			for (size_t ndx = 0; ndx < order.size(); ndx++)
			{
				const int			layerFace	= order[ndx];
				const deUint32		format		= GL_RGBA;
				const deUint32		dataType	= GL_UNSIGNED_BYTE;
				const int			texW		= 128;
				const int			texH		= 128;
				deUint32			tmpTex		= 0;
				const deUint32		fbo			= fbos[layerFace];
				const IVec3&		viewport	= m_texSize;
				tcu::TextureLevel	data		(glu::mapGLTransferFormat(format, dataType), texW, texH, 1);

				tcu::fillWithGrid(data.getAccess(), 8, generateRandomColor(rnd), Vec4(0.0f));

				glGenTextures(1, &tmpTex);
				glBindTexture(GL_TEXTURE_2D, tmpTex);
				glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,		GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,		GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER,	GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER,	GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, format, texW, texH, 0, format, dataType, data.getAccess().getDataPtr());

				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glViewport(0, 0, viewport.x(), viewport.y());
				sglr::drawQuad(*getCurrentContext(), texToFboShaderID, Vec3(-1.0f, -1.0f, 0.0f), Vec3(1.0f, 1.0f, 0.0f));
				checkError();

				// Render to framebuffer
				{
					const Vec3			p0		= Vec3(float(ndx % 2) - 1.0f, float(ndx / 2) - 1.0f, 0.0f);
					const Vec3			p1		= p0 + Vec3(1.0f, 1.0f, 0.0f);
					const int			layer	= layerFace / 6;
					const tcu::CubeFace	face	= getCubeFaceFromNdx(layerFace % 6);

					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glViewport(0, 0, getWidth(), getHeight());

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, tex);

					arrayTexShader.setLayer(layer);
					arrayTexShader.setFace(face);
					arrayTexShader.setUniforms(*getCurrentContext(), arrayTexShaderID);

					sglr::drawQuad(*getCurrentContext(), arrayTexShaderID, p0, p1);
					checkError();
				}
			}
		}

		readPixels(dst, 0, 0, getWidth(), getHeight());
	}

private:
	IVec3 m_texSize;
};

FboColorTests::FboColorTests (Context& context)
	: TestCaseGroup(context, "color", "Colorbuffer tests")
{
}

FboColorTests::~FboColorTests (void)
{
}

void FboColorTests::init (void)
{
	static const deUint32 colorFormats[] =
	{
		// RGBA formats
		GL_RGBA32I,
		GL_RGBA32UI,
		GL_RGBA16I,
		GL_RGBA16UI,
		GL_RGBA8,
		GL_RGBA8I,
		GL_RGBA8UI,
		GL_SRGB8_ALPHA8,
		GL_RGB10_A2,
		GL_RGB10_A2UI,
		GL_RGBA4,
		GL_RGB5_A1,

		// RGB formats
		GL_RGB8,
		GL_RGB565,

		// RG formats
		GL_RG32I,
		GL_RG32UI,
		GL_RG16I,
		GL_RG16UI,
		GL_RG8,
		GL_RG8I,
		GL_RG8UI,

		// R formats
		GL_R32I,
		GL_R32UI,
		GL_R16I,
		GL_R16UI,
		GL_R8,
		GL_R8I,
		GL_R8UI,

		// GL_EXT_color_buffer_float
		GL_RGBA32F,
		GL_RGBA16F,
		GL_R11F_G11F_B10F,
		GL_RG32F,
		GL_RG16F,
		GL_R32F,
		GL_R16F,

		// GL_EXT_color_buffer_half_float
		GL_RGB16F
	};

	// .texcubearray
	{
		tcu::TestCaseGroup* texCubeArrayGroup = new tcu::TestCaseGroup(m_testCtx, "texcubearray", "Cube map array texture tests");
		addChild(texCubeArrayGroup);

		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(colorFormats); fmtNdx++)
			texCubeArrayGroup->addChild(new FboColorTexCubeArrayCase(m_context, getFormatName(colorFormats[fmtNdx]), "",
																	 colorFormats[fmtNdx], IVec3(128, 128, 12)));
	}
}

} // Functional
} // gles31
} // deqp
