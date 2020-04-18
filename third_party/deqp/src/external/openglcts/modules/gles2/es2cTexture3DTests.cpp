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
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file es2cTexture3DTests.cpp
 * \brief GL_OES_texture_3D tests definition.
 */ /*-------------------------------------------------------------------*/

#include "es2cTexture3DTests.hpp"
#include "deDefs.hpp"
#include "deInt32.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "gluTexture.hpp"
#include "gluTextureTestUtil.hpp"
#include "gluTextureUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuPixelFormat.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTexLookupVerifier.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"

#include <map>

using namespace glw;
using namespace glu::TextureTestUtil;

namespace es2cts
{

enum
{
	VIEWPORT_WIDTH  = 64,
	VIEWPORT_HEIGHT = 64,
};

typedef std::pair<int, const char*> CompressedFormatName;
static CompressedFormatName compressedFormatNames[] = {
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ETC1_RGB8, "etc1_rgb8_oes"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA, "rgba_astc_4x4_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA, "rgba_astc_5x4_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA, "rgba_astc_5x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA, "rgba_astc_6x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA, "rgba_astc_6x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA, "rgba_astc_8x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA, "rgba_astc_8x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA, "rgba_astc_8x8_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA, "rgba_astc_10x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA, "rgba_astc_10x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA, "rgba_astc_10x8_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA, "rgba_astc_10x10_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA, "rgba_astc_12x10_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA, "rgba_astc_12x12_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8, "srgb8_alpha8_astc_4x4_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8, "srgb8_alpha8_astc_5x4_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8, "srgb8_alpha8_astc_5x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8, "srgb8_alpha8_astc_6x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8, "srgb8_alpha8_astc_6x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8, "srgb8_alpha8_astc_8x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8, "srgb8_alpha8_astc_8x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8, "srgb8_alpha8_astc_8x8_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8, "sgb8_alpha8_astc_10x5_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8, "srgb8_alpha8_astc_10x6_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8, "srgb8_alpha8_astc_10x8_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8, "srgb8_alpha8_astc_10x10_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8, "srgb8_alpha8_astc_12x10_khr"),
	CompressedFormatName(tcu::COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8, "srgb8_alpha8_astc_12x12_khr")
};

const char* getCompressedFormatName(tcu::CompressedTexFormat format)
{
	static std::map<int, const char*> formatMap(compressedFormatNames,
												compressedFormatNames + DE_LENGTH_OF_ARRAY(compressedFormatNames));
	return formatMap.at(format);
}

class Texture3DBase : public deqp::TestCase
{
public:
	Texture3DBase(deqp::Context& context, const char* name, const char* description);
	virtual ~Texture3DBase(void);

	bool isFeatureSupported() const;
	void getSupportedCompressedFormats(std::set<int>& validFormats) const;
	int calculateDataSize(deUint32 formats, int width, int height, int depth) const;

	template <typename TextureType>
	void verifyTestResult(const float* texCoords, const tcu::Surface& rendered, const TextureType& reference,
						  const ReferenceParams& refParams, bool isNearestOnly) const;

	void uploadTexture3D(const glu::Texture3D& texture) const;

	void renderQuad(glu::TextureTestUtil::TextureType textureType, const float* texCoords) const;

	void verifyError(GLenum expectedError, const char* missmatchMessage) const;
	void verifyError(GLenum expectedError1, GLenum expectedError2, const char* missmatchMessage) const;

	// New methods wrappers.
	void callTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
						GLint border, GLenum format, GLenum type, const void* pixels) const;

	void callTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width,
						   GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) const;

	void callCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x,
							   GLint y, GLsizei width, GLsizei height) const;

	void callCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
								  GLsizei depth, GLint border, GLsizei imageSize, const void* data) const;

	void callCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
									 GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize,
									 const void* data) const;

	void callFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level,
								  GLint zoffset) const;
};

Texture3DBase::Texture3DBase(deqp::Context& context, const char* name, const char* description)
	: deqp::TestCase(context, name, description)
{
}

Texture3DBase::~Texture3DBase(void)
{
}

bool Texture3DBase::isFeatureSupported() const
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_texture_3D") &&
		!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 0)) &&
		!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_OES_texture_3D");
		return false;
	}
	return true;
}

void Texture3DBase::getSupportedCompressedFormats(std::set<int>& formatsSet) const
{
	const glu::ContextInfo& contextInfo = m_context.getContextInfo();

	formatsSet.clear();
	for (int formatNdx = 0; formatNdx < tcu::COMPRESSEDTEXFORMAT_LAST; formatNdx++)
	{
		// ETC2/EAC texture compression algorithm supports only two-dimensional images
		tcu::CompressedTexFormat format = static_cast<tcu::CompressedTexFormat>(formatNdx);
		if (tcu::isEtcFormat(format))
			continue;

		int glFormat = glu::getGLFormat(format);
		if (contextInfo.isCompressedTextureFormatSupported(glFormat))
			formatsSet.insert(glFormat);
	}

	if (formatsSet.empty())
		TCU_FAIL("No supported compressed texture formats.");
}

int Texture3DBase::calculateDataSize(deUint32 formats, int width, int height, int depth) const
{
	tcu::CompressedTexFormat format			= glu::mapGLCompressedTexFormat(formats);
	const tcu::IVec3		 blockPixelSize = tcu::getBlockPixelSize(format);
	const int				 blockSize		= tcu::getBlockSize(format);
	return deDivRoundUp32(width, blockPixelSize.x()) * deDivRoundUp32(height, blockPixelSize.y()) *
		   deDivRoundUp32(depth, blockPixelSize.z()) * blockSize;
}

template <typename TextureType>
void Texture3DBase::verifyTestResult(const float* texCoords, const tcu::Surface& rendered, const TextureType& reference,
									 const ReferenceParams& refParams, bool isNearestOnly) const
{
	const tcu::PixelFormat pixelFormat = m_context.getRenderTarget().getPixelFormat();
	const tcu::IVec4	   colorBits   = max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2),
									 tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
	tcu::LodPrecision	lodPrecision(18, 6);
	tcu::LookupPrecision lookupPrecision;
	lookupPrecision.colorThreshold = tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
	lookupPrecision.coordBits	  = tcu::IVec3(20, 20, 20);
	lookupPrecision.uvwBits		   = tcu::IVec3(7, 7, 7);
	lookupPrecision.colorMask	  = getCompareMask(pixelFormat);

	if (verifyTextureResult(m_testCtx, rendered.getAccess(), reference, texCoords, refParams, lookupPrecision,
							lodPrecision, pixelFormat))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return;
	}

	// Evaluate against lower precision requirements.
	lodPrecision.lodBits	= 4;
	lookupPrecision.uvwBits = tcu::IVec3(4, 4, 4);

	tcu::TestLog& log = m_testCtx.getLog();
	log << tcu::TestLog::Message
		<< "Warning: Verification against high precision requirements failed, trying with lower requirements."
		<< tcu::TestLog::EndMessage;

	if (verifyTextureResult(m_testCtx, rendered.getAccess(), reference, texCoords, refParams, lookupPrecision,
							lodPrecision, pixelFormat))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Low-quality filtering result");
		return;
	}

	log << tcu::TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case."
		<< tcu::TestLog::EndMessage;
	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
}

void Texture3DBase::uploadTexture3D(const glu::Texture3D& texture) const
{
	// note: this function is modified version of glu::Texture3D::upload()
	// this was needed to support methods added by GL_OES_texture_3D extension

	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	deUint32			  textureName	  = texture.getGLTexture();
	const tcu::Texture3D& referenceTexture = texture.getRefTexture();

	TCU_CHECK(textureName);
	gl.bindTexture(GL_TEXTURE_3D, textureName);

	GLint pixelStorageMode = 1;
	int   pixelSize		   = referenceTexture.getFormat().getPixelSize();
	if (deIsPowerOfTwo32(pixelSize))
		pixelStorageMode = de::min(pixelSize, 8);

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, pixelStorageMode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture upload failed");

	glu::TransferFormat transferFormat = glu::getTransferFormat(referenceTexture.getFormat());

	for (int levelNdx = 0; levelNdx < referenceTexture.getNumLevels(); levelNdx++)
	{
		if (referenceTexture.isLevelEmpty(levelNdx))
			continue; // Don't upload.

		tcu::ConstPixelBufferAccess access = referenceTexture.getLevel(levelNdx);
		DE_ASSERT(access.getRowPitch() == access.getFormat().getPixelSize() * access.getWidth());
		DE_ASSERT(access.getSlicePitch() == access.getFormat().getPixelSize() * access.getWidth() * access.getHeight());
		callTexImage3D(GL_TEXTURE_3D, levelNdx, transferFormat.format, access.getWidth(), access.getHeight(),
					   access.getDepth(), 0 /* border */, transferFormat.format, transferFormat.dataType,
					   access.getDataPtr());
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture upload failed");
}

void Texture3DBase::renderQuad(glu::TextureTestUtil::TextureType textureType, const float* texCoords) const
{
	glu::RenderContext&   renderContext = m_context.getRenderContext();
	glu::GLSLVersion	  glslVersion   = glu::getContextTypeGLSLVersion(renderContext.getType());
	const glw::Functions& gl			= renderContext.getFunctions();

	// Prepare data for rendering
	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
	static const float	position[]	= { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	static const char* vsTemplate = "${VERSION}\n"
									"attribute highp vec4 a_position;\n"
									"attribute highp ${TEXCOORD_TYPE} a_texCoord;\n"
									"varying highp ${TEXCOORD_TYPE} v_texCoord;\n"
									"void main (void) {\n"
									"  gl_Position = a_position;\n"
									"  v_texCoord = a_texCoord;\n"
									"}\n";
	static const char* fsTemplate = "${VERSION}\n"
									"${HEADER}\n"
									"varying highp ${TEXCOORD_TYPE} v_texCoord;\n"
									"uniform highp ${SAMPLER_TYPE} u_sampler;\n"
									"void main (void) {\n"
									"  gl_FragColor = ${LOOKUP}(u_sampler, v_texCoord);\n"
									"}\n";

	int numComponents = 3;

	std::map<std::string, std::string> specializationMap;
	specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (textureType == TEXTURETYPE_3D)
	{
		specializationMap["HEADER"]		   = "#extension GL_OES_texture_3D : enable";
		specializationMap["TEXCOORD_TYPE"] = "vec3";
		specializationMap["SAMPLER_TYPE"]  = "sampler3D";
		specializationMap["LOOKUP"]		   = "texture3D";
	}
	else if (textureType == TEXTURETYPE_2D)
	{
		numComponents					   = 2;
		specializationMap["HEADER"]		   = "";
		specializationMap["TEXCOORD_TYPE"] = "vec2";
		specializationMap["SAMPLER_TYPE"]  = "sampler2D";
		specializationMap["LOOKUP"]		   = "texture2D";
	}
	else
		TCU_FAIL("Unsuported texture type.");

	// Specialize shaders
	std::string			vs = tcu::StringTemplate(vsTemplate).specialize(specializationMap);
	std::string			fs = tcu::StringTemplate(fsTemplate).specialize(specializationMap);
	glu::ProgramSources programSources(glu::makeVtxFragSources(vs, fs));

	// Create program
	glu::ShaderProgram testProgram(renderContext, programSources);
	if (!testProgram.isOk())
	{
		m_testCtx.getLog() << testProgram;
		TCU_FAIL("Compile failed");
	}

	// Set uniforms
	deUint32 programId = testProgram.getProgram();
	gl.useProgram(programId);
	gl.uniform1i(gl.getUniformLocation(programId, "u_sampler"), 0);

	// Define vertex attributes
	const glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("a_position", 2, 4, 0, position),
													 glu::va::Float("a_texCoord", numComponents, 4, 0, texCoords) };

	// Draw quad
	glu::draw(m_context.getRenderContext(), programId, DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));
}

void Texture3DBase::verifyError(GLenum expectedError, const char* missmatchMessage) const
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	GLenum				  currentError = gl.getError();
	if (currentError == expectedError)
		return;

	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Incorrect error was reported");
	m_testCtx.getLog() << tcu::TestLog::Message << glu::getErrorStr(static_cast<int>(expectedError))
					   << " was expected but got " << glu::getErrorStr(static_cast<int>(currentError)) << ". "
					   << missmatchMessage << tcu::TestLog::EndMessage;
}

void Texture3DBase::verifyError(GLenum expectedError1, GLenum expectedError2, const char* missmatchMessage) const
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	GLenum				  currentError = gl.getError();
	if ((currentError == expectedError1) || (currentError == expectedError2))
		return;

	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Incorrect error was reported");
	m_testCtx.getLog() << tcu::TestLog::Message << glu::getErrorStr(static_cast<int>(expectedError1)) << " or "
					   << glu::getErrorStr(static_cast<int>(expectedError1)) << " was expected but got "
					   << glu::getErrorStr(static_cast<int>(currentError)) << ". " << missmatchMessage
					   << tcu::TestLog::EndMessage;
}

void Texture3DBase::callTexImage3D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height,
								   GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.texImage3DOES)
		gl.texImage3DOES(target, level, internalFormat, width, height, depth, border, format, type, pixels);
	else if (gl.texImage3D)
		gl.texImage3D(target, level, static_cast<GLint>(internalFormat), width, height, depth, border, format, type,
					  pixels);
	else
		TCU_FAIL("glTexImage3D not supported");
}

void Texture3DBase::callTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
									  GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
									  const void* pixels) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.texSubImage3DOES)
		gl.texSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	else if (gl.texSubImage3D)
		gl.texSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	else
		TCU_FAIL("glTexSubImage3D not supported");
}

void Texture3DBase::callCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
										  GLint x, GLint y, GLsizei width, GLsizei height) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.copyTexSubImage3DOES)
		gl.copyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	else if (gl.copyTexSubImage3D)
		gl.copyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	else
		TCU_FAIL("glCopyTexSubImage3D not supported");
}

void Texture3DBase::callCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width,
											 GLsizei height, GLsizei depth, GLint border, GLsizei imageSize,
											 const void* data) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.compressedTexImage3DOES)
		gl.compressedTexImage3DOES(target, level, internalformat, width, height, depth, border, imageSize, data);
	else if (gl.compressedTexImage3D)
		gl.compressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
	else
		TCU_FAIL("gl.compressedTexImage3D not supported");
}

void Texture3DBase::callCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
												GLsizei width, GLsizei height, GLsizei depth, GLenum format,
												GLsizei imageSize, const void* data) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.compressedTexSubImage3DOES)
		gl.compressedTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize,
									  data);
	else if (gl.compressedTexSubImage3D)
		gl.compressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize,
								   data);
	else
		TCU_FAIL("gl.compressedTexSubImage3D not supported");
}

void Texture3DBase::callFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
											 GLint level, GLint zoffset) const
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (gl.framebufferTexture3DOES)
		gl.framebufferTexture3DOES(target, attachment, textarget, texture, level, zoffset);
	else if (gl.framebufferTexture3D)
		gl.framebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
	else
		TCU_FAIL("glFramebufferTexture3D not supported");
}

struct FilteringData
{
	GLint minFilter;
	GLint magFilter;
	GLint wrapS;
	GLint wrapT;
	GLint wrapR;

	deUint32 internalFormat;
	int		 width;
	int		 height;
	int		 depth;
};

class Texture3DFilteringCase : public Texture3DBase
{
public:
	Texture3DFilteringCase(deqp::Context& context, const char* name, const char* desc, const FilteringData& data);
	~Texture3DFilteringCase(void);

	void		  init(void);
	void		  deinit(void);
	IterateResult iterate(void);

private:
	struct FilterCase
	{
		const glu::Texture3D* texture;
		tcu::Vec3			  lod;
		tcu::Vec3			  offset;

		FilterCase(void) : texture(DE_NULL)
		{
		}

		FilterCase(const glu::Texture3D* tex_, const tcu::Vec3& lod_, const tcu::Vec3& offset_)
			: texture(tex_), lod(lod_), offset(offset_)
		{
		}
	};

private:
	Texture3DFilteringCase(const Texture3DFilteringCase& other);
	Texture3DFilteringCase& operator=(const Texture3DFilteringCase& other);

	const FilteringData m_filteringData;

	glu::Texture3D* m_gradientTex;
	glu::Texture3D* m_gridTex;

	std::vector<FilterCase> m_cases;
	int						m_caseNdx;
};

Texture3DFilteringCase::Texture3DFilteringCase(deqp::Context& context, const char* name, const char* desc,
											   const FilteringData& data)
	: Texture3DBase(context, name, desc)
	, m_filteringData(data)
	, m_gradientTex(DE_NULL)
	, m_gridTex(DE_NULL)
	, m_caseNdx(0)
{
}

Texture3DFilteringCase::~Texture3DFilteringCase(void)
{
	Texture3DFilteringCase::deinit();
}

void Texture3DFilteringCase::init(void)
{
	if (!isFeatureSupported())
		return;

	const deUint32 internalFormat = m_filteringData.internalFormat;
	const int	  width		  = m_filteringData.width;
	const int	  height		  = m_filteringData.height;
	const int	  depth		  = m_filteringData.depth;

	const tcu::TextureFormat	 texFmt	= glu::mapGLInternalFormat(internalFormat);
	const tcu::TextureFormatInfo fmtInfo   = tcu::getTextureFormatInfo(texFmt);
	const tcu::Vec4				 cScale	= fmtInfo.valueMax - fmtInfo.valueMin;
	const tcu::Vec4				 cBias	 = fmtInfo.valueMin;
	const int					 numLevels = deLog2Floor32(de::max(de::max(width, height), depth)) + 1;

	// Create textures.
	m_gradientTex = new glu::Texture3D(m_context.getRenderContext(), internalFormat, width, height, depth);
	m_gridTex	 = new glu::Texture3D(m_context.getRenderContext(), internalFormat, width, height, depth);

	// Fill first gradient texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		tcu::Vec4 gMin = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f) * cScale + cBias;
		tcu::Vec4 gMax = tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) * cScale + cBias;

		m_gradientTex->getRefTexture().allocLevel(levelNdx);
		tcu::fillWithComponentGradients(m_gradientTex->getRefTexture().getLevel(levelNdx), gMin, gMax);
	}

	// Fill second with grid texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		deUint32 step   = 0x00ffffff / numLevels;
		deUint32 rgb	= step * levelNdx;
		deUint32 colorA = 0xff000000 | rgb;
		deUint32 colorB = 0xff000000 | ~rgb;

		m_gridTex->getRefTexture().allocLevel(levelNdx);
		tcu::fillWithGrid(m_gridTex->getRefTexture().getLevel(levelNdx), 4, tcu::RGBA(colorA).toVec() * cScale + cBias,
						  tcu::RGBA(colorB).toVec() * cScale + cBias);
	}

	// Upload.
	uploadTexture3D(*m_gradientTex);
	uploadTexture3D(*m_gridTex);

	// Test cases
	m_cases.push_back(FilterCase(m_gradientTex, tcu::Vec3(1.5f, 2.8f, 1.0f), tcu::Vec3(-1.0f, -2.7f, -2.275f)));
	m_cases.push_back(FilterCase(m_gradientTex, tcu::Vec3(-2.0f, -1.5f, -1.8f), tcu::Vec3(-0.1f, 0.9f, -0.25f)));
	m_cases.push_back(FilterCase(m_gridTex, tcu::Vec3(0.2f, 0.175f, 0.3f), tcu::Vec3(-2.0f, -3.7f, -1.825f)));
	m_cases.push_back(FilterCase(m_gridTex, tcu::Vec3(-0.8f, -2.3f, -2.5f), tcu::Vec3(0.2f, -0.1f, 1.325f)));

	m_caseNdx = 0;
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void Texture3DFilteringCase::deinit(void)
{
	delete m_gradientTex;
	delete m_gridTex;

	m_gradientTex = DE_NULL;
	m_gridTex	 = DE_NULL;

	m_cases.clear();
}

Texture3DFilteringCase::IterateResult Texture3DFilteringCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions&		 gl		 = m_context.getRenderContext().getFunctions();
	const FilterCase&			 curCase = m_cases[m_caseNdx];
	const tcu::TextureFormat	 texFmt  = curCase.texture->getRefTexture().getFormat();
	const tcu::TextureFormatInfo fmtInfo = tcu::getTextureFormatInfo(texFmt);
	const tcu::ScopedLogSection  section(m_testCtx.getLog(), std::string("Test") + de::toString(m_caseNdx),
										std::string("Test ") + de::toString(m_caseNdx));
	tcu::TestLog&   log = m_testCtx.getLog();
	ReferenceParams refParams(TEXTURETYPE_3D);
	tcu::Surface	rendered(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	tcu::Vec3		texCoord[4];

	// Setup params for reference.
	refParams.sampler = glu::mapGLSampler(m_filteringData.wrapS, m_filteringData.wrapT, m_filteringData.wrapR,
										  m_filteringData.minFilter, m_filteringData.magFilter);
	refParams.samplerType = getSamplerType(texFmt);
	refParams.lodMode	 = LODMODE_EXACT;
	refParams.colorBias   = fmtInfo.lookupBias;
	refParams.colorScale  = fmtInfo.lookupScale;

	// Compute texture coordinates.
	log << tcu::TestLog::Message << "Approximate lod per axis = " << curCase.lod << ", offset = " << curCase.offset
		<< tcu::TestLog::EndMessage;

	{
		const float lodX = curCase.lod.x();
		const float lodY = curCase.lod.y();
		const float lodZ = curCase.lod.z();
		const float oX   = curCase.offset.x();
		const float oY   = curCase.offset.y();
		const float oZ   = curCase.offset.z();
		const float sX   = deFloatExp2(lodX) * float(VIEWPORT_WIDTH) / float(m_gradientTex->getRefTexture().getWidth());
		const float sY = deFloatExp2(lodY) * float(VIEWPORT_HEIGHT) / float(m_gradientTex->getRefTexture().getHeight());
		const float sZ = deFloatExp2(lodZ) * float(VIEWPORT_WIDTH) / float(m_gradientTex->getRefTexture().getDepth());

		texCoord[0] = tcu::Vec3(oX, oY, oZ);
		texCoord[1] = tcu::Vec3(oX, oY + sY, oZ + sZ * 0.5f);
		texCoord[2] = tcu::Vec3(oX + sX, oY, oZ + sZ * 0.5f);
		texCoord[3] = tcu::Vec3(oX + sX, oY + sY, oZ + sZ);
	}

	gl.bindTexture(GL_TEXTURE_3D, curCase.texture->getGLTexture());
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, m_filteringData.minFilter);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, m_filteringData.magFilter);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, m_filteringData.wrapS);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, m_filteringData.wrapT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, m_filteringData.wrapR);

	// Verify bound 3D texture.
	GLint resultName;
	gl.getIntegerv(GL_TEXTURE_BINDING_3D, &resultName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");
	if (curCase.texture->getGLTexture() == static_cast<deUint32>(resultName))
	{
		// Render.
		gl.viewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		renderQuad(TEXTURETYPE_3D, texCoord->getPtr());
		glu::readPixels(m_context.getRenderContext(), 0, 0, rendered.getAccess());

		// Compare rendered image to reference.
		const bool isNearestOnly =
			(m_filteringData.minFilter == GL_NEAREST) && (m_filteringData.magFilter == GL_NEAREST);
		verifyTestResult(texCoord[0].getPtr(), rendered, curCase.texture->getRefTexture(), refParams, isNearestOnly);
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid texture name");
	}

	++m_caseNdx;
	return (m_caseNdx < static_cast<int>(m_cases.size())) ? CONTINUE : STOP;
}

class TexSubImage3DCase : public Texture3DBase
{
public:
	TexSubImage3DCase(deqp::Context& context, const char* name, const char* desc, deUint32 internalFormat, int width,
					  int height, int depth);

	IterateResult iterate(void);

private:
	deUint32 m_internalFormat;
	int		 m_width;
	int		 m_height;
	int		 m_depth;
	int		 m_numLevels;
};

TexSubImage3DCase::TexSubImage3DCase(deqp::Context& context, const char* name, const char* desc,
									 deUint32 internalFormat, int width, int height, int depth)
	: Texture3DBase(context, name, desc)
	, m_internalFormat(internalFormat)
	, m_width(width)
	, m_height(height)
	, m_depth(depth)
	, m_numLevels(static_cast<int>(deLog2Floor32(de::max(width, de::max(height, depth))) + 1))
{
}

TexSubImage3DCase::IterateResult TexSubImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	glu::RenderContext&   renderCtx = m_context.getRenderContext();
	const glw::Functions& gl		= renderCtx.getFunctions();

	tcu::Vec4					 firstColor(0.0f, 1.0f, 0.0f, 1.0f);
	tcu::Vec4					 secondColor(1.0f, 0.0f, 1.0f, 1.0f);
	glu::Texture3D				 texture(m_context.getRenderContext(), m_internalFormat, m_width, m_height, m_depth);
	const tcu::TextureFormat	 textureFormat = texture.getRefTexture().getFormat();
	const tcu::TextureFormatInfo formatInfo	= tcu::getTextureFormatInfo(textureFormat);

	// Fill texture.
	for (int levelNdx = 0; levelNdx < m_numLevels; levelNdx++)
	{
		texture.getRefTexture().allocLevel(levelNdx);
		const tcu::PixelBufferAccess& pba = texture.getRefTexture().getLevel(levelNdx);
		tcu::fillWithComponentGradients(pba, firstColor, secondColor);
	}

	// Upload texture
	uploadTexture3D(texture);

	gl.bindTexture(GL_TEXTURE_3D, texture.getGLTexture());
	GLU_EXPECT_NO_ERROR(gl.getError(), "gl.bindTexture");
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri for GL_TEXTURE_WRAP_R");
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri for GL_TEXTURE_WRAP_T");
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri for GL_TEXTURE_WRAP_R");
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri for GL_TEXTURE_MIN_FILTER");
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri for GL_TEXTURE_MAG_FILTER");

	// Re-specify parts of each level in uploaded texture and in reference texture
	tcu::TextureLevel data(textureFormat);
	for (int levelNdx = 0; levelNdx < m_numLevels - 2; levelNdx++)
	{
		int scale = levelNdx + 1;
		int w	 = de::max(1, m_width >> scale);
		int h	 = de::max(1, m_height >> scale);
		int d	 = de::max(1, m_depth >> levelNdx);

		data.setSize(w, h, d);
		tcu::clear(data.getAccess(), secondColor);

		glu::TransferFormat transferFormat = glu::getTransferFormat(textureFormat);
		callTexSubImage3D(GL_TEXTURE_3D, levelNdx, w, h, 0, w, h, d, transferFormat.format, transferFormat.dataType,
						  data.getAccess().getDataPtr());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D");

		const tcu::PixelBufferAccess& pba = texture.getRefTexture().getLevel(levelNdx);
		tcu::clear(getSubregion(pba, w, h, 0, w, h, d), secondColor);
	}

	// Setup params for reference.
	ReferenceParams refParams(TEXTURETYPE_3D);
	refParams.sampler	 = glu::mapGLSampler(GL_REPEAT, GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
	refParams.samplerType = getSamplerType(textureFormat);
	refParams.lodMode	 = LODMODE_EXACT;
	refParams.colorBias   = formatInfo.lookupBias;
	refParams.colorScale  = formatInfo.lookupScale;

	tcu::Vec3 texCoord[4] = { tcu::Vec3(0.0f, 0.0f, 0.5f), tcu::Vec3(0.0f, 1.0f, 0.5f), tcu::Vec3(1.0f, 0.0f, 0.5f),
							  tcu::Vec3(1.0f, 1.0f, 0.5f) };

	// Render.
	gl.viewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	renderQuad(TEXTURETYPE_3D, texCoord[0].getPtr());

	tcu::Surface rendered(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	glu::readPixels(m_context.getRenderContext(), 0, 0, rendered.getAccess());

	// Compare rendered image to reference.
	verifyTestResult(texCoord[0].getPtr(), rendered, texture.getRefTexture(), refParams, false);
	return STOP;
}

class CopyTexSubImage3DCase : public Texture3DBase
{
public:
	CopyTexSubImage3DCase(deqp::Context& context, const char* name, const char* desc, deUint32 internalFormat,
						  int width, int height, int depth);

	IterateResult iterate(void);

private:
	deUint32 m_internalFormat;
	int		 m_width;
	int		 m_height;
	int		 m_depth;
	int		 m_numLevels;
};

CopyTexSubImage3DCase::CopyTexSubImage3DCase(deqp::Context& context, const char* name, const char* desc,
											 deUint32 internalFormat, int width, int height, int depth)
	: Texture3DBase(context, name, desc)
	, m_internalFormat(internalFormat)
	, m_width(width)
	, m_height(height)
	, m_depth(depth)
	, m_numLevels(static_cast<int>(deLog2Floor32(de::max(width, de::max(height, depth))) + 1))
{
}

CopyTexSubImage3DCase::IterateResult CopyTexSubImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	glu::RenderContext&   renderCtx = m_context.getRenderContext();
	const glw::Functions& gl		= renderCtx.getFunctions();

	ReferenceParams				 refParams(TEXTURETYPE_3D);
	tcu::Surface				 rendered(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	tcu::Vec4					 firstColor(0.0f, 1.0f, 0.0f, 1.0f);
	tcu::Vec4					 secondColor(1.0f, 0.0f, 1.0f, 1.0f);
	glu::Texture3D				 texture(m_context.getRenderContext(), m_internalFormat, m_width, m_height, m_depth);
	const tcu::TextureFormat	 textureFormat = texture.getRefTexture().getFormat();
	const tcu::TextureFormatInfo formatInfo	= tcu::getTextureFormatInfo(textureFormat);

	// Fill texture.
	for (int levelNdx = 0; levelNdx < m_numLevels; levelNdx++)
	{
		texture.getRefTexture().allocLevel(levelNdx);
		const tcu::PixelBufferAccess& pba = texture.getRefTexture().getLevel(levelNdx);
		tcu::fillWithComponentGradients(pba, firstColor, secondColor);
	}

	// Upload texture.
	uploadTexture3D(texture);

	gl.clearColor(secondColor[0], secondColor[1], secondColor[2], secondColor[3]);
	gl.clear(GL_COLOR_BUFFER_BIT);
	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);

	gl.bindTexture(GL_TEXTURE_3D, texture.getGLTexture());
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Re-specify parts of each level in uploaded texture and in reference texture
	tcu::TextureLevel data(textureFormat);
	for (int levelNdx = 0; levelNdx < m_numLevels - 2; levelNdx++)
	{
		int scale = levelNdx + 1;
		int w	 = de::max(1, m_width >> scale);
		int h	 = de::max(1, m_height >> scale);
		int d	 = de::max(1, m_depth >> levelNdx);

		data.setSize(w, h, d);
		tcu::clear(data.getAccess(), secondColor);

		for (int depthNdx = 0; depthNdx < d; depthNdx++)
		{
			callCopyTexSubImage3D(GL_TEXTURE_3D, levelNdx, w, h, depthNdx, 0, 0, w, h);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyTexSubImage3D");
		}
		const tcu::PixelBufferAccess& pba = texture.getRefTexture().getLevel(levelNdx);
		tcu::clear(getSubregion(pba, w, h, 0, w, h, d), secondColor);
	}

	// Setup params for reference.
	refParams.sampler	 = glu::mapGLSampler(GL_REPEAT, GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
	refParams.samplerType = getSamplerType(textureFormat);
	refParams.lodMode	 = LODMODE_EXACT;
	refParams.colorBias   = formatInfo.lookupBias;
	refParams.colorScale  = formatInfo.lookupScale;

	tcu::Vec3 texCoord[4] = { tcu::Vec3(0.0f, 0.0f, 0.5f), tcu::Vec3(0.0f, 1.0f, 0.5f), tcu::Vec3(1.0f, 0.0f, 0.5f),
							  tcu::Vec3(1.0f, 1.0f, 0.5f) };

	// Render.
	gl.viewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	renderQuad(TEXTURETYPE_3D, texCoord[0].getPtr());
	glu::readPixels(m_context.getRenderContext(), 0, 0, rendered.getAccess());

	// Compare rendered image to reference.
	verifyTestResult(texCoord[0].getPtr(), rendered, texture.getRefTexture(), refParams, false);
	return STOP;
}

class FramebufferTexture3DCase : public Texture3DBase
{
public:
	FramebufferTexture3DCase(deqp::Context& context, const char* name, const char* desc, deUint32 internalFormat,
							 int width, int height, int depth);

	IterateResult iterate(void);

private:
	deUint32 m_internalFormat;
	int		 m_width;
	int		 m_height;
	int		 m_depth;
	int		 m_numLevels;
};

FramebufferTexture3DCase::FramebufferTexture3DCase(deqp::Context& context, const char* name, const char* desc,
												   deUint32 internalFormat, int width, int height, int depth)
	: Texture3DBase(context, name, desc)
	, m_internalFormat(internalFormat)
	, m_width(width)
	, m_height(height)
	, m_depth(depth)
	, m_numLevels(static_cast<int>(deLog2Floor32(de::max(width, de::max(height, depth))) + 1))
{
}

FramebufferTexture3DCase::IterateResult FramebufferTexture3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	glu::RenderContext&   renderCtx = m_context.getRenderContext();
	const glw::Functions& gl		= renderCtx.getFunctions();

	tcu::Vec4	  firstColor(0.0f, 1.0f, 0.0f, 1.0f);
	tcu::Vec4	  secondColor(1.0f, 0.0f, 1.0f, 1.0f);
	glu::Texture3D texture3D(m_context.getRenderContext(), m_internalFormat, m_width, m_height, m_depth);
	glu::Texture2D texture2D(m_context.getRenderContext(), m_internalFormat, m_width, m_height);

	// Fill textures.
	texture3D.getRefTexture().allocLevel(0);
	const tcu::PixelBufferAccess& pba3D = texture3D.getRefTexture().getLevel(0);
	tcu::clear(pba3D, secondColor);

	for (int levelNdx = 0; levelNdx < m_numLevels; levelNdx++)
	{
		texture2D.getRefTexture().allocLevel(levelNdx);
		const tcu::PixelBufferAccess& pba2D = texture2D.getRefTexture().getLevel(levelNdx);
		tcu::fillWithGrid(pba2D, 4, firstColor, secondColor);
	}

	// Upload textures.
	uploadTexture3D(texture3D);
	texture2D.upload();

	gl.bindTexture(GL_TEXTURE_3D, texture3D.getGLTexture());
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Create framebuffer.
	glw::GLuint fbo = 0;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffers");
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texture3D.getGLTexture(), 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture3D");

	deUint32 status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		TCU_FAIL("Framebuffer is not complete");

	gl.bindTexture(GL_TEXTURE_2D, texture2D.getGLTexture());
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const tcu::TextureFormat	 textureFormat = texture2D.getRefTexture().getFormat();
	const tcu::TextureFormatInfo formatInfo	= tcu::getTextureFormatInfo(textureFormat);

	tcu::Vec2 texCoord[4] = { tcu::Vec2(0.0f, 0.0f), tcu::Vec2(0.0f, 1.0f), tcu::Vec2(1.0f, 0.0f),
							  tcu::Vec2(1.0f, 1.0f) };

	// Render to fbo.
	gl.viewport(0, 0, m_width, m_height);
	renderQuad(TEXTURETYPE_2D, texCoord[0].getPtr());

	// Setup params for reference.
	ReferenceParams refParams(TEXTURETYPE_2D);
	refParams.sampler	 = glu::mapGLSampler(GL_REPEAT, GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST);
	refParams.samplerType = getSamplerType(textureFormat);
	refParams.lodMode	 = LODMODE_EXACT;
	refParams.colorBias   = formatInfo.lookupBias;
	refParams.colorScale  = formatInfo.lookupScale;

	// Compare image rendered to selected layer of 3d texture to reference.
	tcu::Surface rendered(m_width, m_height);
	glu::readPixels(m_context.getRenderContext(), 0, 0, rendered.getAccess());
	verifyTestResult(texCoord[0].getPtr(), rendered, texture2D.getRefTexture(), refParams, false);

	// Cleanup.
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
	gl.deleteFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");

	return STOP;
}

class CompressedTexture3DCase : public Texture3DBase
{
public:
	CompressedTexture3DCase(deqp::Context& context, const char* name, deUint32 format);

	IterateResult iterate(void);

private:
	int m_compressedFormat;
};

CompressedTexture3DCase::CompressedTexture3DCase(deqp::Context& context, const char* name, deUint32 format)
	: Texture3DBase(context, name, ""), m_compressedFormat(static_cast<int>(format))
{
}

CompressedTexture3DCase::IterateResult CompressedTexture3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions&	gl			 = m_context.getRenderContext().getFunctions();
	const glu::ContextInfo&  contextInfo = m_context.getContextInfo();
	tcu::CompressedTexFormat format		 = glu::mapGLCompressedTexFormat(m_compressedFormat);

	if (!contextInfo.isCompressedTextureFormatSupported(m_compressedFormat))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Compressed format not supported by implementation");
		return STOP;
	}

	const deInt32 width		  = 64;
	const deInt32 height	  = 64;
	const deInt32 depth		  = 4;
	const deInt32 levelsCount = 4;

	GLuint textureName;
	gl.genTextures(1, &textureName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gl.genTextures");
	gl.bindTexture(GL_TEXTURE_3D, textureName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gl.bindTexture");

	// Create 3D texture with random data.
	for (deInt32 levelIndex = 0; levelIndex < levelsCount; ++levelIndex)
	{
		deInt32 levelWidth  = de::max(width >> levelIndex, 1);
		deInt32 levelHeight = de::max(height >> levelIndex, 1);
		deInt32 levelDepth  = de::max(depth >> levelIndex, 1);

		tcu::CompressedTexture level(format, levelWidth, levelHeight, levelDepth);
		const int			   dataSize = level.getDataSize();
		deUint8* const		   data		= static_cast<deUint8*>(level.getData());
		de::Random			   rnd(deStringHash(getName()) + levelIndex);

		for (int i  = 0; i < dataSize; i++)
			data[i] = rnd.getUint32() & 0xff;

		callCompressedTexImage3D(GL_TEXTURE_3D, levelIndex, m_compressedFormat, level.getWidth(), level.getHeight(),
								 level.getDepth(), 0 /* border */, level.getDataSize(), level.getData());
		GLU_EXPECT_NO_ERROR(gl.getError(), "callCompressedTexImage3D");
	}

	// Replace whole texture data.
	for (deInt32 levelIndex = levelsCount - 2; levelIndex >= 0; --levelIndex)
	{
		deInt32 partWidth  = de::max(width >> levelIndex, 1);
		deInt32 partHeight = de::max(height >> levelIndex, 1);
		deInt32 partDepth  = de::max(depth >> levelIndex, 1);
		;

		tcu::CompressedTexture dataPart(format, partWidth, partHeight, partDepth);
		const int			   dataSize = dataPart.getDataSize();
		deUint8* const		   data		= static_cast<deUint8*>(dataPart.getData());
		de::Random			   rnd(deStringHash(getName()) + levelIndex);

		for (int i  = 0; i < dataSize; i++)
			data[i] = rnd.getUint32() & 0xff;

		callCompressedTexSubImage3D(GL_TEXTURE_3D, levelIndex, 0, 0, 0, dataPart.getWidth(), dataPart.getHeight(),
									dataPart.getDepth(), m_compressedFormat, dataPart.getDataSize(),
									dataPart.getData());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexSubImage3D");
	}

	gl.deleteTextures(1, &textureName);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gl.deleteTextures");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

class NegativeTexImage3DCase : public Texture3DBase
{
public:
	NegativeTexImage3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeTexImage3DCase::NegativeTexImage3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

NegativeTexImage3DCase::IterateResult NegativeTexImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// negative usage
	{
		const char* message1 = "GL_INVALID_ENUM is generated if target is invalid.";
		callTexImage3D(0, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, message1);
		callTexImage3D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, message1);

		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGBA, 0, 0);
		verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if type is not a type constant.");

		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, 0, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if format is not an accepted format constant.");

		callTexImage3D(GL_TEXTURE_3D, 0, 0, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if internalFormat is not one of the accepted "
									  "resolution and format symbolic constants.");

		const char* message2 = "GL_INVALID_OPERATION is generated if target is GL_TEXTURE_3D and format is "
							   "GL_DEPTH_COMPONENT, or GL_DEPTH_STENCIL.";
		callTexImage3D(GL_TEXTURE_3D, 0, GL_DEPTH_STENCIL, 1, 1, 1, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		verifyError(GL_INVALID_OPERATION, message2);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_DEPTH_COMPONENT, 1, 1, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_OPERATION, message2);

		const char* message3 =
			"GL_INVALID_OPERATION is generated if the combination of internalFormat, format and type is invalid.";
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_OPERATION, message3);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
		verifyError(GL_INVALID_OPERATION, message3);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGB5_A1, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		verifyError(GL_INVALID_OPERATION, message3);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGB10_A2, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_INT_2_10_10_10_REV, 0);
		verifyError(GL_INVALID_OPERATION, message3);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32UI, 1, 1, 1, 0, GL_RGBA_INTEGER, GL_INT, 0);
		verifyError(GL_INVALID_OPERATION, message3);
	}

	// invalid leve
	{
		const char* message = "GL_INVALID_VALUE is generated if level is less than 0.";
		callTexImage3D(GL_TEXTURE_3D, -1, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// maximal level
	{
		int max3DTexSize;
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTexSize);
		GLint log2Max3DTextureSize = deLog2Floor32(max3DTexSize) + 1;
		callTexImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, GL_RGB, 1, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE,
					"GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");
	}

	// negative dimensions
	{
		const char* message = "GL_INVALID_VALUE is generated if width or height is less than 0.";
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, -1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, -1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, -1, -1, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// maximal dimensions
	{
		int aboveMax3DTextureSize;
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &aboveMax3DTextureSize);
		int aboveMaxTextureSize;
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &aboveMaxTextureSize);
		++aboveMax3DTextureSize;
		++aboveMaxTextureSize;

		const char* message =
			"GL_INVALID_VALUE is generated if width, height or depth is greater than GL_MAX_3D_TEXTURE_SIZE.";
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, aboveMax3DTextureSize, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, aboveMax3DTextureSize, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 1, 1, aboveMax3DTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, aboveMax3DTextureSize, aboveMax3DTextureSize, aboveMax3DTextureSize,
					   0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// invalid border
	{
		const char* message = "GL_INVALID_VALUE is generated if border is not 0 or 1.";
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, -1, GL_RGB, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, 1, 1, 1, 2, GL_RGB, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	return STOP;
}

class NegativeCompressedTexImage3DCase : public Texture3DBase
{
public:
	NegativeCompressedTexImage3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeCompressedTexImage3DCase::NegativeCompressedTexImage3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

class NegativeTexSubImage3DCase : public Texture3DBase
{
public:
	NegativeTexSubImage3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeTexSubImage3DCase::NegativeTexSubImage3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

NegativeTexSubImage3DCase::IterateResult NegativeTexSubImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// negative usage
	{
		deUint32 texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		const char* message1 = "GL_INVALID_ENUM is generated if target is invalid.";
		callTexSubImage3D(0, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, message1);
		callTexSubImage3D(GL_TEXTURE_2D, 0, 0, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, message1);

		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 4, 4, 4, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if format is not an accepted format constant.");

		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, 0, 0);
		verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if type is not a type constant.");

		const char* message2 = "GL_INVALID_OPERATION is generated if the combination of internalFormat of "
							   "the previously specified texture array, format and type is not valid.";
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_4_4_4_4, 0);
		verifyError(GL_INVALID_OPERATION, message2);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		verifyError(GL_INVALID_OPERATION, message2);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_RGB, GL_UNSIGNED_SHORT_5_5_5_1, 0);
		verifyError(GL_INVALID_OPERATION, message2);

		gl.deleteTextures(1, &texture);
	}

	// negative level
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		const char* message = "GL_INVALID_VALUE is generated if level is less than 0.";
		callTexSubImage3D(GL_TEXTURE_3D, -1, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);

		gl.deleteTextures(1, &texture);
	}

	// maximal level
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		int maxSize;
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxSize);
		GLint log2Max3DTextureSize = deLog2Floor32(maxSize) + 1;

		callTexSubImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, 0, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE,
					"GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");

		gl.deleteTextures(1, &texture);
	}

	// negative offset
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		const char* message = "GL_INVALID_VALUE is generated if xoffset, yoffset or zoffset are negative.";
		callTexSubImage3D(GL_TEXTURE_3D, 0, -1, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, -1, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, -1, -1, -1, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);

		gl.deleteTextures(1, &texture);
	}

	// invalid offset
	{
		deUint32 texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		callTexSubImage3D(GL_TEXTURE_3D, 0, 2, 0, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if xoffset + width > texture_width.");
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 2, 0, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if yoffset + height > texture_height.");
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 2, 4, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if zoffset + depth > texture_depth.");

		gl.deleteTextures(1, &texture);
	}

	// negative dimensions
	{
		const char* message = "GL_INVALID_VALUE is generated if width, height or depth is less than 0.";
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, -1, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, -1, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
		callTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, -1, -1, -1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	return STOP;
}

class NegativeCopyTexSubImage3DCase : public Texture3DBase
{
public:
	NegativeCopyTexSubImage3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeCopyTexSubImage3DCase::NegativeCopyTexSubImage3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

NegativeCopyTexSubImage3DCase::IterateResult NegativeCopyTexSubImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// invalid usage
	{
		GLuint texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		callCopyTexSubImage3D(0, 0, 0, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if target is invalid.");

		gl.deleteTextures(1, &texture);
	}

	// negative level
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		const char* message = "GL_INVALID_VALUE is generated if level is less than 0.";
		callCopyTexSubImage3D(GL_TEXTURE_3D, -1, 0, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, message);

		gl.deleteTextures(1, &texture);
	}

	// maximal level
	{
		int maxSize;
		int max3DSize;
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DSize);
		deUint32 log2Max3DTextureSize = deLog2Floor32(max3DSize) + 1;

		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		callCopyTexSubImage3D(GL_TEXTURE_3D, log2Max3DTextureSize, 0, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE,
					"GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");

		gl.deleteTextures(1, &texture);
	}

	// negative offset
	{
		GLuint texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		const char* message = "GL_INVALID_VALUE is generated if xoffset, yoffset or zoffset is negative.";
		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, -1, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, message);
		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, -1, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, message);
		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, -1, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, message);
		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, -1, -1, -1, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, message);

		gl.deleteTextures(1, &texture);
	}

	// invalid offset
	{
		GLuint texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 1, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if xoffset + width > texture_width.");

		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 1, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if yoffset + height > texture_height.");

		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 4, 0, 0, 4, 4);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if zoffset + 1 > texture_depth.");

		gl.deleteTextures(1, &texture);
	}

	// negative dimensions
	{
		GLuint texture = 0x1234;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.getError(); // reset error

		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, -4, 4);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if width < 0.");

		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, 4, -4);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if height < 0.");

		gl.deleteTextures(1, &texture);
	}

	// incomplete_framebuffer
	{
		GLuint fbo = 0x1234;
		GLuint texture;

		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.genFramebuffers(1, &fbo);
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER);

		const char* message = "GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently "
							  "bound framebuffer is not framebuffer complete.";
		callCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, 4, 4);
		verifyError(GL_INVALID_FRAMEBUFFER_OPERATION, message);

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		gl.deleteFramebuffers(1, &fbo);
		gl.deleteTextures(1, &texture);
	}

	return STOP;
}

class NegativeFramebufferTexture3DCase : public Texture3DBase
{
public:
	NegativeFramebufferTexture3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeFramebufferTexture3DCase::NegativeFramebufferTexture3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

NegativeFramebufferTexture3DCase::IterateResult NegativeFramebufferTexture3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	GLuint fbo = 0x1234;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint tex3D = 0x1234;
	gl.genTextures(1, &tex3D);
	gl.bindTexture(GL_TEXTURE_3D, tex3D);

	GLint maxTexSize = 0x1234;
	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	gl.getError(); // reset error

	callFramebufferTexture3D(-1, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, tex3D, 0, 0);
	verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if target is not one of the accepted tokens.");

	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex3D, 0, 0);
	verifyError(GL_INVALID_OPERATION,
				"GL_INVALID_OPERATION is generated if textarget is not an accepted texture target.");

	callFramebufferTexture3D(GL_FRAMEBUFFER, -1, GL_TEXTURE_3D, tex3D, 0, 0);
	verifyError(GL_INVALID_ENUM, "GL_INVALID_ENUM is generated if attachment is not an accepted token.");

	const char* message1 =
		"GL_INVALID_VALUE is generated if level is less than 0 or larger than log_2 of maximum texture size.";
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, tex3D, -1, 0);
	verifyError(GL_INVALID_VALUE, message1);
	GLint maxSize = deLog2Floor32(maxTexSize) + 1;
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, tex3D, maxSize, 0);
	verifyError(GL_INVALID_VALUE, message1);

	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, -1, 0, 0);
	verifyError(
		GL_INVALID_OPERATION,
		"GL_INVALID_OPERATION is generated if texture is neither 0 nor the name of an existing texture object.");

	const char* message2 = "GL_INVALID_OPERATION is generated if textarget and texture are not compatible.";
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, tex3D, 0, 0);
	verifyError(GL_INVALID_OPERATION, message2);
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex3D, 0, 0);
	verifyError(GL_INVALID_OPERATION, message2);
	gl.deleteTextures(1, &tex3D);

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	callFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, 0, 0, 0);
	verifyError(GL_INVALID_OPERATION, "GL_INVALID_OPERATION is generated if zero is bound to target.");

	gl.deleteFramebuffers(1, &fbo);
	return STOP;
}

NegativeCompressedTexImage3DCase::IterateResult NegativeCompressedTexImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::set<int> supportedFormats;
	getSupportedCompressedFormats(supportedFormats);
	GLenum supportedCompressedFormat = static_cast<GLenum>(*(supportedFormats.begin()));

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// negative usage
	{
		const char* message1 = "GL_INVALID_ENUM is generated if target is invalid.";
		callCompressedTexImage3D(0, 0, supportedCompressedFormat, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_ENUM, message1);
		callCompressedTexImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, supportedCompressedFormat, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_ENUM, message1);

		const char* message2 =
			"GL_INVALID_ENUM is generated if internalformat is not one of the specific compressed internal formats.";
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_ENUM, message2);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_ENUM, message2);

		const char* message3 = "INVALID_OPERATION is generated if internalformat is an ETC2/EAC format.";
		for (int formatNdx = 0; formatNdx < tcu::COMPRESSEDTEXFORMAT_LAST; formatNdx++)
		{
			tcu::CompressedTexFormat format = static_cast<tcu::CompressedTexFormat>(formatNdx);
			if (tcu::isEtcFormat(format) && (format != tcu::COMPRESSEDTEXFORMAT_ETC1_RGB8))
			{
				deUint32 compressedFormat = glu::getGLFormat(format);
				callCompressedTexImage3D(GL_TEXTURE_3D, 0, compressedFormat, 0, 0, 0, 0, 0, 0);
				verifyError(GL_INVALID_OPERATION, message3);
			}
		}
	}

	// negative level
	{
		callCompressedTexImage3D(GL_TEXTURE_3D, -1, supportedCompressedFormat, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if level is less than 0.");
	}

	// maximal level
	{
		int maxSize;
		gl.getIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxSize);
		GLint log2MaxTextureSize = deLog2Floor32(maxSize) + 1;
		callCompressedTexImage3D(GL_TEXTURE_3D, log2MaxTextureSize, supportedCompressedFormat, 0, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE,
					"GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_TEXTURE_SIZE).");
	}

	// negative dimensions
	{
		const char* message = "GL_INVALID_VALUE is generated if width, height or depth is less than 0.";
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, -1, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, -1, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, 0, -1, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, -1, -1, -1, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// maximal dimensions
	{
		int maxTextureSize;
		gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
		++maxTextureSize;

		const char* message =
			"GL_INVALID_VALUE is generated if width, height or depth is greater than GL_MAX_TEXTURE_SIZE.";
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, maxTextureSize, 0, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, maxTextureSize, 0, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, 0, maxTextureSize, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, maxTextureSize, maxTextureSize,
								 maxTextureSize, 0, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// invalid  border
	{
		const char* message = "GL_INVALID_VALUE is generated if border is not 0.";
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, 0, 0, -1, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, 0, 0, 1, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	// invalid size
	{
		const char* message = "GL_INVALID_VALUE is generated if imageSize is not consistent with the "
							  "format, dimensions, and contents of the specified compressed image data.";
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 0, 0, 0, 0, -1, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 16, 16, 1, 0, 9 * 4 * 8, 0);
		verifyError(GL_INVALID_VALUE, message);
	}

	return STOP;
}

class NegativeCompressedTexSubImage3DCase : public Texture3DBase
{
public:
	NegativeCompressedTexSubImage3DCase(deqp::Context& context, const char* name);

	IterateResult iterate(void);
};

NegativeCompressedTexSubImage3DCase::NegativeCompressedTexSubImage3DCase(deqp::Context& context, const char* name)
	: Texture3DBase(context, name, "")
{
}

NegativeCompressedTexSubImage3DCase::IterateResult NegativeCompressedTexSubImage3DCase::iterate(void)
{
	if (!isFeatureSupported())
		return STOP;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::set<int> supportedFormats;
	getSupportedCompressedFormats(supportedFormats);
	GLenum supportedCompressedFormat = static_cast<GLenum>(*(supportedFormats.begin()));
	int	textureSize				 = 16;
	int	dataSize					 = calculateDataSize(supportedCompressedFormat, textureSize, textureSize, 1);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// negative level
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, textureSize, textureSize, 1, 0, dataSize,
								 0);
		gl.getError(); // reset error

		callCompressedTexSubImage3D(GL_TEXTURE_3D, -1, 0, 0, 0, 0, 0, 0, supportedCompressedFormat, 0, 0);
		verifyError(GL_INVALID_VALUE, "GL_INVALID_VALUE is generated if level is less than 0.");

		gl.deleteTextures(1, &texture);
	}

	// invalid level
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, textureSize, textureSize, 1, 0, dataSize,
								 0);
		gl.getError(); // reset error

		GLint log2MaxTextureSize = deLog2Floor32(m_context.getContextInfo().getInt(GL_MAX_3D_TEXTURE_SIZE)) + 1;
		callCompressedTexSubImage3D(GL_TEXTURE_3D, log2MaxTextureSize, 0, 0, 0, 0, 0, 0, supportedCompressedFormat, 0,
									0);
		verifyError(GL_INVALID_VALUE,
					"GL_INVALID_VALUE is generated if level is greater than log_2(GL_MAX_3D_TEXTURE_SIZE).");

		gl.deleteTextures(1, &texture);
	}

	// negative offsets
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, textureSize, textureSize, 1, 0, dataSize,
								 0);
		gl.getError(); // reset error

		const char* message =
			"GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset, yoffset or zoffset are negative.";
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, -4, 0, 0, 0, 0, 0, supportedCompressedFormat, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, -4, 0, 0, 0, 0, supportedCompressedFormat, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, -4, 0, 0, 0, supportedCompressedFormat, 0, 0);
		verifyError(GL_INVALID_VALUE, message);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, -4, -4, -4, 0, 0, 0, supportedCompressedFormat, 0, 0);
		verifyError(GL_INVALID_VALUE, message);

		gl.deleteTextures(1, &texture);
	}

	// invalid offsets
	{
		deUint32 texture;
		gl.genTextures(1, &texture);
		gl.bindTexture(GL_TEXTURE_3D, texture);
		dataSize = calculateDataSize(supportedCompressedFormat, 4, 4, 1);
		callCompressedTexImage3D(GL_TEXTURE_3D, 0, supportedCompressedFormat, 4, 4, 1, 0, dataSize, 0);
		gl.getError(); // reset error

		const char* message = "GL_INVALID_VALUE or GL_INVALID_OPERATION is generated if xoffset + width > "
							  "texture_width or yoffset + height > texture_height.";
		dataSize = calculateDataSize(supportedCompressedFormat, 8, 4, 1);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 12, 0, 0, 8, 4, 1, supportedCompressedFormat, dataSize, 0);
		verifyError(GL_INVALID_VALUE, GL_INVALID_OPERATION, message);
		dataSize = calculateDataSize(supportedCompressedFormat, 4, 8, 1);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 12, 0, 4, 8, 1, supportedCompressedFormat, dataSize, 0);
		verifyError(GL_INVALID_VALUE, GL_INVALID_OPERATION, message);
		dataSize = calculateDataSize(supportedCompressedFormat, 4, 4, 1);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 12, 4, 4, 1, supportedCompressedFormat, dataSize, 0);
		verifyError(GL_INVALID_VALUE, GL_INVALID_OPERATION, message);
		dataSize = calculateDataSize(supportedCompressedFormat, 8, 8, 1);
		callCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 12, 12, 12, 8, 8, 1, supportedCompressedFormat, dataSize, 0);
		verifyError(GL_INVALID_VALUE, GL_INVALID_OPERATION, message);

		gl.deleteTextures(1, &texture);
	}

	return STOP;
}

Texture3DTests::Texture3DTests(deqp::Context& context) : TestCaseGroup(context, "texture_3d", "")
{
}

Texture3DTests::~Texture3DTests(void)
{
}

void Texture3DTests::init()
{
	static const struct
	{
		const char* name;
		GLint		mode;
	} wrapModes[] = { { "clamp", GL_CLAMP_TO_EDGE }, { "repeat", GL_REPEAT }, { "mirror", GL_MIRRORED_REPEAT } };

	static const struct
	{
		const char* name;
		GLint		mode;
	} minFilterModes[] = { { "nearest", GL_NEAREST },
						   { "linear", GL_LINEAR },
						   { "nearest_mipmap_nearest", GL_NEAREST_MIPMAP_NEAREST },
						   { "linear_mipmap_nearest", GL_LINEAR_MIPMAP_NEAREST },
						   { "nearest_mipmap_linear", GL_NEAREST_MIPMAP_LINEAR },
						   { "linear_mipmap_linear", GL_LINEAR_MIPMAP_LINEAR } };

	static const struct
	{
		const char* name;
		GLint		mode;
	} magFilterModes[] = { { "nearest", GL_NEAREST }, { "linear", GL_LINEAR } };

	static const struct
	{
		int width;
		int height;
		int depth;
	} sizes[] = { { 4, 8, 8 }, { 32, 64, 16 }, { 128, 32, 64 }, { 3, 7, 5 }, { 63, 63, 63 } };

	static const struct
	{
		const char* name;
		deUint32	format;
	} filterableFormatsByType[] = {
		{ "rgba8", GL_RGBA8 },
	};

	// Texture3DFilteringCase
	{
		deqp::TestCaseGroup* texFilteringGroup =
			new deqp::TestCaseGroup(m_context, "filtering", "3D Texture Filtering");
		addChild(texFilteringGroup);

		// Formats.
		FilteringData		 data;
		deqp::TestCaseGroup* formatsGroup = new deqp::TestCaseGroup(m_context, "formats", "3D Texture Formats");
		texFilteringGroup->addChild(formatsGroup);
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
		{
			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				data.minFilter		= minFilterModes[filterNdx].mode;
				bool isMipmap		= data.minFilter != GL_NEAREST && data.minFilter != GL_LINEAR;
				data.magFilter		= isMipmap ? GL_LINEAR : data.minFilter;
				data.internalFormat = filterableFormatsByType[fmtNdx].format;
				data.wrapS			= GL_REPEAT;
				data.wrapT			= GL_REPEAT;
				data.wrapR			= GL_REPEAT;
				data.width			= 64;
				data.height			= 64;
				data.depth			= 64;

				const char* formatName = filterableFormatsByType[fmtNdx].name;
				const char* filterName = minFilterModes[filterNdx].name;
				std::string name	   = std::string(formatName) + "_" + filterName;

				formatsGroup->addChild(new Texture3DFilteringCase(m_context, name.c_str(), "", data));
			}
		}

		// Sizes.
		deqp::TestCaseGroup* sizesGroup = new deqp::TestCaseGroup(m_context, "sizes", "Texture Sizes");
		texFilteringGroup->addChild(sizesGroup);
		for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); sizeNdx++)
		{
			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				data.minFilter		= minFilterModes[filterNdx].mode;
				data.internalFormat = GL_RGBA8;
				bool isMipmap		= data.minFilter != GL_NEAREST && data.minFilter != GL_LINEAR;
				data.magFilter		= isMipmap ? GL_LINEAR : data.minFilter;
				data.wrapS			= GL_REPEAT;
				data.wrapT			= GL_REPEAT;
				data.wrapR			= GL_REPEAT;
				data.width			= sizes[sizeNdx].width;
				data.height			= sizes[sizeNdx].height;
				data.depth			= sizes[sizeNdx].depth;

				const char* filterName = minFilterModes[filterNdx].name;
				std::string name	   = de::toString(data.width) + "x" + de::toString(data.height) + "x" +
								   de::toString(data.depth) + "_" + filterName;

				sizesGroup->addChild(new Texture3DFilteringCase(m_context, name.c_str(), "", data));
			}
		}

		// Wrap modes.
		deqp::TestCaseGroup* combinationsGroup =
			new deqp::TestCaseGroup(m_context, "combinations", "Filter and wrap mode combinations");
		texFilteringGroup->addChild(combinationsGroup);
		for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
		{
			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
			{
				for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
				{
					for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
					{
						for (int wrapRNdx = 0; wrapRNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapRNdx++)
						{
							data.minFilter		= minFilterModes[minFilterNdx].mode;
							data.magFilter		= magFilterModes[magFilterNdx].mode;
							data.internalFormat = GL_RGBA8;
							data.wrapS			= wrapModes[wrapSNdx].mode;
							data.wrapT			= wrapModes[wrapTNdx].mode;
							data.wrapR			= wrapModes[wrapRNdx].mode;
							data.width			= 63;
							data.height			= 57;
							data.depth			= 67;
							std::string name	= std::string(minFilterModes[minFilterNdx].name) + "_" +
											   magFilterModes[magFilterNdx].name + "_" + wrapModes[wrapSNdx].name +
											   "_" + wrapModes[wrapTNdx].name + "_" + wrapModes[wrapRNdx].name;

							combinationsGroup->addChild(new Texture3DFilteringCase(m_context, name.c_str(), "", data));
						}
					}
				}
			}
		}

		// negative tests.
		combinationsGroup->addChild(new NegativeTexImage3DCase(m_context, "negative"));
	}

	// TexSubImage3DOES tests
	{
		tcu::TestCaseGroup* texSubImageGroup =
			new tcu::TestCaseGroup(m_testCtx, "sub_image", "Basic glTexSubImage3D() usage");
		addChild(texSubImageGroup);
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); formatNdx++)
		{
			const char* fmtName = filterableFormatsByType[formatNdx].name;
			deUint32	format  = filterableFormatsByType[formatNdx].format;
			texSubImageGroup->addChild(new TexSubImage3DCase(m_context, fmtName, "", format, 32, 64, 8));
		}
		texSubImageGroup->addChild(new NegativeTexSubImage3DCase(m_context, "negative"));
	}

	// CopyTexSubImage3DOES tests
	{
		tcu::TestCaseGroup* copyTexSubImageGroup =
			new tcu::TestCaseGroup(m_testCtx, "copy_sub_image", "Basic glCopyTexSubImage3D() usage");
		addChild(copyTexSubImageGroup);
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); formatNdx++)
		{
			const char* fmtName = filterableFormatsByType[formatNdx].name;
			deUint32	format  = filterableFormatsByType[formatNdx].format;
			copyTexSubImageGroup->addChild(new CopyTexSubImage3DCase(m_context, fmtName, "", format, 32, 64, 8));
		}
		copyTexSubImageGroup->addChild(new NegativeCopyTexSubImage3DCase(m_context, "negative"));
	}

	// FramebufferTexture3DOES tests
	{
		tcu::TestCaseGroup* framebufferTextureGroup =
			new tcu::TestCaseGroup(m_testCtx, "framebuffer_texture", "Basic glFramebufferTexture3D() usage");
		addChild(framebufferTextureGroup);
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); formatNdx++)
		{
			const char* fmtName = filterableFormatsByType[formatNdx].name;
			deUint32	format  = filterableFormatsByType[formatNdx].format;
			framebufferTextureGroup->addChild(new FramebufferTexture3DCase(m_context, fmtName, "", format, 64, 64, 3));
		}
		framebufferTextureGroup->addChild(new NegativeFramebufferTexture3DCase(m_context, "negative"));
	}

	// CompressedTexImage3DOES and CompressedTexSubImage3DOES tests
	{
		tcu::TestCaseGroup* compressedTexGroup =
			new tcu::TestCaseGroup(m_testCtx, "compressed_texture", "Basic gl.compressedTexImage3D() usage");
		for (int formatNdx = 0; formatNdx < tcu::COMPRESSEDTEXFORMAT_LAST; formatNdx++)
		{
			// ETC2/EAC texture compression algorithm supports only two-dimensional images
			tcu::CompressedTexFormat format = static_cast<tcu::CompressedTexFormat>(formatNdx);
			if (tcu::isEtcFormat(format))
				continue;

			deUint32	compressedFormat = glu::getGLFormat(format);
			const char* name			 = getCompressedFormatName(format);
			compressedTexGroup->addChild(new CompressedTexture3DCase(m_context, name, compressedFormat));
		}
		compressedTexGroup->addChild(new NegativeCompressedTexImage3DCase(m_context, "negative_compressed_tex_image"));
		compressedTexGroup->addChild(
			new NegativeCompressedTexSubImage3DCase(m_context, "negative_compressed_tex_sub_image"));
		addChild(compressedTexGroup);
	}
}
} // glcts namespace
