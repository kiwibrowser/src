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
 * \file  glcPackedDepthStencilTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcPackedDepthStencilTests.hpp"
#include "deMath.h"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>
#include <cstring>
#include <stdio.h>

using namespace glw;
using namespace glu;

namespace glcts
{

#define TEX_SIZE 256
#define TOLERANCE_LOW 0.48
#define TOLERANCE_HIGH 0.52
#define EPSILON 0.01

enum DrawMode
{
	DEFAULT,
	DEPTH_SPAN1,
	DEPTH_SPAN2,
};

struct D32F_S8
{
	GLfloat d;
	GLuint  s;
};

// Reference texture names for the described 5 textures and framebuffers
// and also for identifying other cases' reference textures
enum TextureNames
{
	packedTexImage,
	packedTexRender,
	packedTexRenderInitStencil,
	packedTexRenderDepthStep,
	packedTexRenderStencilStep,
	NUM_TEXTURES,
	verifyCopyTexImage,
	verifyPartialAttachments,
	verifyMixedAttachments,
	verifyClearBufferDepth,
	verifyClearBufferStencil,
	verifyClearBufferDepthStencil,
	verifyBlit,
};

struct TypeFormat
{
	GLenum		type;
	GLenum		format;
	const char* formatName;
	int			size;
	int			d;
	int			s;
};

#define NUM_TEXTURE_TYPES 2

static const TypeFormat TextureTypes[NUM_TEXTURE_TYPES] = {
	{ GL_UNSIGNED_INT_24_8, GL_DEPTH24_STENCIL8, "depth24_stencil8", sizeof(GLuint), 24, 8 },
	{ GL_FLOAT_32_UNSIGNED_INT_24_8_REV, GL_DEPTH32F_STENCIL8, "depth32f_stencil8", sizeof(GLuint) + sizeof(GLfloat),
	  32, 8 },
};

// Texture targets for initial state checking
static const GLenum coreTexTargets[] = {
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_1D,
	GL_TEXTURE_1D_ARRAY,
	GL_TEXTURE_CUBE_MAP_ARRAY,
	GL_TEXTURE_RECTANGLE,
	GL_TEXTURE_2D_MULTISAMPLE,
	GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
	GL_PROXY_TEXTURE_1D,
	GL_PROXY_TEXTURE_2D,
	GL_PROXY_TEXTURE_3D,
	GL_PROXY_TEXTURE_1D_ARRAY,
	GL_PROXY_TEXTURE_2D_ARRAY,
	GL_PROXY_TEXTURE_CUBE_MAP_ARRAY,
	GL_PROXY_TEXTURE_RECTANGLE,
	GL_PROXY_TEXTURE_CUBE_MAP,
	GL_PROXY_TEXTURE_2D_MULTISAMPLE,
	GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY,
};
static const GLenum esTexTargets[] = {
	GL_TEXTURE_2D,
	GL_TEXTURE_3D,
	GL_TEXTURE_2D_ARRAY,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

// Listing of non-depth_stencil types for error tests
static const GLenum coreNonDepthStencilTypes[] = {
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT,
	GL_HALF_FLOAT,
	GL_FLOAT,
	GL_UNSIGNED_SHORT_5_6_5,
	GL_UNSIGNED_SHORT_4_4_4_4,
	GL_UNSIGNED_SHORT_5_5_5_1,
	GL_UNSIGNED_INT_2_10_10_10_REV,
	GL_UNSIGNED_INT_10F_11F_11F_REV,
	GL_UNSIGNED_INT_5_9_9_9_REV,
	GL_UNSIGNED_BYTE_3_3_2,
	GL_UNSIGNED_BYTE_2_3_3_REV,
	GL_UNSIGNED_SHORT_5_6_5_REV,
	GL_UNSIGNED_SHORT_4_4_4_4_REV,
	GL_UNSIGNED_SHORT_1_5_5_5_REV,
	GL_UNSIGNED_INT_8_8_8_8,
	GL_UNSIGNED_INT_8_8_8_8_REV,
	GL_UNSIGNED_INT_10_10_10_2,
};
static const GLenum esNonDepthStencilTypes[] = {
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT,
	GL_HALF_FLOAT,
	GL_FLOAT,
	GL_UNSIGNED_SHORT_5_6_5,
	GL_UNSIGNED_SHORT_4_4_4_4,
	GL_UNSIGNED_SHORT_5_5_5_1,
	GL_UNSIGNED_INT_2_10_10_10_REV,
	GL_UNSIGNED_INT_10F_11F_11F_REV,
	GL_UNSIGNED_INT_5_9_9_9_REV,
};

// Listing of non-depth_stencil formats for error tests
static const GLenum coreNonDepthStencilFormats[] = {
	GL_STENCIL_INDEX, GL_RED,		  GL_GREEN,		   GL_BLUE,			 GL_RG,			  GL_RGB,		 GL_RGBA,
	GL_BGR,			  GL_BGRA,		  GL_RED_INTEGER,  GL_GREEN_INTEGER, GL_BLUE_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER,
	GL_RGBA_INTEGER,  GL_BGR_INTEGER, GL_BGRA_INTEGER,
};
static const GLenum esNonDepthStencilFormats[] = {
	GL_RED,
	GL_RG,
	GL_RGB,
	GL_RGBA,
	GL_LUMINANCE,		// for es3+
	GL_ALPHA,			// for es3+
	GL_LUMINANCE_ALPHA, // for es3+
	GL_RED_INTEGER,
	GL_RG_INTEGER,
	GL_RGB_INTEGER,
	GL_RGBA_INTEGER,
};

// Listing of non-depth_stencil base formats for error tests
static const GLenum coreOtherBaseFormats[] = {
	GL_RED, GL_RG, GL_RGB, GL_RGBA,
};
static const GLenum esOtherBaseFormats[] = {
	GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_LUMINANCE, GL_ALPHA, GL_LUMINANCE_ALPHA,
};

struct AttachmentParam
{
	GLenum pname;
	GLint  value;
};

#define NUM_ATTACHMENT_PARAMS_CORE 13
#define NUM_ATTACHMENT_PARAMS_ES 12

static const AttachmentParam coreAttachmentParams[NUM_TEXTURE_TYPES][NUM_ATTACHMENT_PARAMS_CORE] = {
	{
		{ GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, 24 },
		{ GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, 8 },
		{ GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_UNSIGNED_NORMALIZED },
		{ GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, GL_LINEAR },
		{ GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, -1 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_LAYERED, 0 },
	},
	{
		{ GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, 32 },
		{ GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, 8 },
		{ GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_FLOAT },
		{ GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, GL_LINEAR },
		{ GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, -1 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_LAYERED, 0 },
	},
};
static const AttachmentParam esAttachmentParams[NUM_TEXTURE_TYPES][NUM_ATTACHMENT_PARAMS_ES] = {
	{
		{ GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, 24 },
		{ GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, 8 },
		{ GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_UNSIGNED_NORMALIZED },
		{ GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, GL_LINEAR },
		{ GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, -1 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER, 0 },
	},
	{
		{ GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, 32 },
		{ GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, 8 },
		{ GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, GL_FLOAT },
		{ GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, GL_LINEAR },
		{ GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, -1 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, 0 },
		{ GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER, 0 },
	},
};

enum ColorFunction
{
	COLOR_CHECK_DEFAULT,
	COLOR_CHECK_DEPTH,
	COLOR_CHECK_STENCIL,
};

class BaseTest : public deqp::TestCase
{
public:
	BaseTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~BaseTest();

	void								 init(void);
	virtual tcu::TestNode::IterateResult iterate(void);

	const AttachmentParam* getAttachmentParams() const;
	void createGradient(std::vector<GLbyte>& data);

	void setDrawReadBuffer(GLenum draw, GLenum read);
	void restoreDrawReadBuffer();

	void createTextures();
	void setupTexture();
	void destroyTextures();

	GLuint createProgram(const char* vsCode, const char* fsCode);
	void setupColorProgram(GLint& uColor);
	bool setupTextureProgram();
	bool setupStencilProgram();
	bool setTextureUniform(GLuint programId);

	void drawQuad(DrawMode drawMode, GLuint program);
	void renderToTextures();
	bool verifyDepthStencilGradient(GLvoid* data, unsigned int texIndex, int width, int height);
	bool verifyColorGradient(GLvoid* data, unsigned int texIndex, int function, int width, int height);
	bool doReadPixels(GLuint texture, int function);

protected:
	GLuint m_defaultFBO;
	GLuint m_drawBuffer;
	GLuint m_readBuffer;

	const GLenum* m_textureTargets;
	GLuint		  m_textureTargetsCount;
	const GLenum* m_nonDepthStencilTypes;
	GLuint		  m_nonDepthStencilTypesCount;
	const GLenum* m_nonDepthStencilFormats;
	GLuint		  m_nonDepthStencilFormatsCount;
	const GLenum* m_otherBaseFormats;
	GLuint		  m_otherBaseFormatsCount;

	const AttachmentParam* m_attachmentParams[NUM_TEXTURE_TYPES];
	GLuint				   m_attachmentParamsCount;

	const TypeFormat& m_typeFormat;

	GLuint m_textureProgram;
	GLuint m_colorProgram;
	GLuint m_stencilProgram;

	GLuint m_textures[NUM_TEXTURES];
	GLuint m_framebuffers[NUM_TEXTURES];
};

BaseTest::BaseTest(deqp::Context& context, const TypeFormat& tf)
	: deqp::TestCase(context, tf.formatName, "")
	, m_defaultFBO(0)
	, m_drawBuffer(GL_COLOR_ATTACHMENT0)
	, m_readBuffer(GL_COLOR_ATTACHMENT0)
	, m_textureTargets(coreTexTargets)
	, m_textureTargetsCount(DE_LENGTH_OF_ARRAY(coreTexTargets))
	, m_nonDepthStencilTypes(coreNonDepthStencilTypes)
	, m_nonDepthStencilTypesCount(DE_LENGTH_OF_ARRAY(coreNonDepthStencilTypes))
	, m_nonDepthStencilFormats(coreNonDepthStencilFormats)
	, m_nonDepthStencilFormatsCount(DE_LENGTH_OF_ARRAY(coreNonDepthStencilFormats))
	, m_otherBaseFormats(coreOtherBaseFormats)
	, m_otherBaseFormatsCount(DE_LENGTH_OF_ARRAY(coreOtherBaseFormats))
	, m_typeFormat(tf)
	, m_textureProgram(0)
	, m_colorProgram(0)
	, m_stencilProgram(0)

{
}

BaseTest::~BaseTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	if (m_textureProgram)
		gl.deleteProgram(m_textureProgram);
	if (m_colorProgram)
		gl.deleteProgram(m_colorProgram);
	if (m_stencilProgram)
		gl.deleteProgram(m_stencilProgram);
}

void BaseTest::init(void)
{
	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_textureTargets			  = esTexTargets;
		m_textureTargetsCount		  = DE_LENGTH_OF_ARRAY(esTexTargets);
		m_nonDepthStencilTypes		  = esNonDepthStencilTypes;
		m_nonDepthStencilTypesCount   = DE_LENGTH_OF_ARRAY(esNonDepthStencilTypes);
		m_nonDepthStencilFormats	  = esNonDepthStencilFormats;
		m_nonDepthStencilFormatsCount = DE_LENGTH_OF_ARRAY(esNonDepthStencilFormats);
		m_otherBaseFormats			  = esOtherBaseFormats;
		m_otherBaseFormatsCount		  = DE_LENGTH_OF_ARRAY(esOtherBaseFormats);

		for (int i				  = 0; i < NUM_TEXTURE_TYPES; i++)
			m_attachmentParams[i] = esAttachmentParams[i];
		m_attachmentParamsCount   = NUM_ATTACHMENT_PARAMS_ES;
	}
	else
	{
		for (int i				  = 0; i < NUM_TEXTURE_TYPES; i++)
			m_attachmentParams[i] = coreAttachmentParams[i];
		m_attachmentParamsCount   = NUM_ATTACHMENT_PARAMS_CORE;
	}
}

tcu::TestNode::IterateResult BaseTest::iterate(void)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

const AttachmentParam* BaseTest::getAttachmentParams() const
{
	// find type index
	int index = 0;
	for (; index < NUM_TEXTURE_TYPES; index++)
	{
		if (TextureTypes[index].format == m_typeFormat.format)
			break;
	}

	if (index >= NUM_TEXTURE_TYPES)
		TCU_FAIL("Missing attachment definition");

	return m_attachmentParams[index];
}

// Creates a gradient texture data in the given type parameter format
void BaseTest::createGradient(std::vector<GLbyte>& data)
{
	switch (m_typeFormat.type)
	{
	case GL_UNSIGNED_INT_24_8:
	{
		data.resize(TEX_SIZE * TEX_SIZE * sizeof(GLuint));
		GLuint* dataPtr = reinterpret_cast<GLuint*>(&data[0]);
		for (int j = 0; j < TEX_SIZE; j++)
		{
			for (int i = 0; i < TEX_SIZE; i++)
			{
				GLuint  d = static_cast<GLuint>(static_cast<float>(i) / (TEX_SIZE - 1) * 0x00ffffff);
				GLubyte s = i & 0xff;

				dataPtr[TEX_SIZE * j + i] = (d << 8) + s;
			}
		}
		return;
	}
	case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
	{
		data.resize(TEX_SIZE * TEX_SIZE * sizeof(D32F_S8));
		D32F_S8* dataPtr = reinterpret_cast<D32F_S8*>(&data[0]);
		for (int j = 0; j < TEX_SIZE; j++)
		{
			for (int i = 0; i < TEX_SIZE; i++)
			{
				D32F_S8 v				  = { static_cast<float>(i) / (TEX_SIZE - 1), static_cast<GLuint>(i & 0xff) };
				dataPtr[TEX_SIZE * j + i] = v;
			}
		}
		return;
	}
	default:
		TCU_FAIL("Unsuported type");
	}
}

void BaseTest::setDrawReadBuffer(GLenum draw, GLenum read)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint drawBuffer;
	gl.getIntegerv(GL_DRAW_BUFFER0, &drawBuffer);
	m_drawBuffer = static_cast<GLuint>(drawBuffer);

	GLint readBuffer;
	gl.getIntegerv(GL_READ_BUFFER, &readBuffer);
	m_readBuffer = static_cast<GLuint>(readBuffer);

	gl.drawBuffers(1, &draw);
	gl.readBuffer(read);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer");
}

void BaseTest::restoreDrawReadBuffer()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.drawBuffers(1, &m_drawBuffer);
	gl.readBuffer(m_readBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer");
}

void BaseTest::createTextures()
{
	// Creates all the textures and framebuffers
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(NUM_TEXTURES, m_textures);
	gl.genFramebuffers(NUM_TEXTURES, m_framebuffers);

	// packedTexImage
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexImage]);
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexImage]);
	setupTexture();
	std::vector<GLbyte> data;
	createGradient(data);
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  &data[0]);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexImage], 0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexImage], 0);

	// packedTexRender
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRender]);
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexRender]);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  NULL);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRender], 0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRender], 0);

	// packedTexRenderInitStencil
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderInitStencil]);
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexRenderInitStencil]);
	setupTexture();
	createGradient(data);
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  &data[0]);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRenderInitStencil],
							0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
							m_textures[packedTexRenderInitStencil], 0);

	// packedTexRenderDepthStep
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderDepthStep]);
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexRenderDepthStep]);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  NULL);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRenderDepthStep],
							0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRenderDepthStep],
							0);

	// packedTexRenderStencilStep
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderStencilStep]);
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexRenderStencilStep]);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  NULL);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexRenderStencilStep],
							0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
							m_textures[packedTexRenderStencilStep], 0);

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
}

void BaseTest::setupTexture()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
}

// Destroys all the textures and framebuffers
void BaseTest::destroyTextures()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.deleteFramebuffers(NUM_TEXTURES, m_framebuffers);
	gl.deleteTextures(NUM_TEXTURES, m_textures);
}

GLuint BaseTest::createProgram(const char* vsCode, const char* fsCode)
{
	glu::RenderContext&   renderContext = m_context.getRenderContext();
	glu::ContextType	  contextType   = renderContext.getType();
	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	glu::GLSLVersion	  glslVersion   = glu::getContextTypeGLSLVersion(contextType);
	const char*			  version		= glu::getGLSLVersionDeclaration(glslVersion);

	glu::Shader vs(gl, glu::SHADERTYPE_VERTEX);
	const char* vSources[] = { version, vsCode };
	const int   vLengths[] = { int(strlen(version)), int(strlen(vsCode)) };
	vs.setSources(2, vSources, vLengths);
	vs.compile();
	if (!vs.getCompileStatus())
		TCU_FAIL("Vertex shader compilation failed");

	glu::Shader fs(gl, glu::SHADERTYPE_FRAGMENT);
	const char* fSources[] = { version, fsCode };
	const int   fLengths[] = { int(strlen(version)), int(strlen(fsCode)) };
	fs.setSources(2, fSources, fLengths);
	fs.compile();
	if (!fs.getCompileStatus())
		TCU_FAIL("Fragment shader compilation failed");

	GLuint p = gl.createProgram();
	gl.attachShader(p, vs.getShader());
	gl.attachShader(p, fs.getShader());
	gl.linkProgram(p);
	return p;
}

void BaseTest::setupColorProgram(GLint& uColor)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	const char* vs = "\n"
					 "precision highp float;\n"
					 "in vec4 pos;\n"
					 "void main() {\n"
					 "  gl_Position = pos;\n"
					 "}\n";

	const char* fs = "\n"
					 "precision highp float;\n"
					 "out vec4 color;\n"
					 "uniform vec4 uColor;\n"
					 "void main() {\n"
					 "  color = uColor;\n"
					 "}\n";

	// setup shader program
	if (!m_colorProgram)
		m_colorProgram = createProgram(vs, fs);
	if (!m_colorProgram)
		TCU_FAIL("Error while loading shader program");

	gl.useProgram(m_colorProgram);

	// Setup program uniforms
	uColor = gl.getUniformLocation(m_colorProgram, "uColor");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation");

	if (uColor == -1)
		TCU_FAIL("Error getting uniform uColor");

	gl.uniform4f(uColor, 1.0f, 1.0f, 1.0f, 1.0f);
}

// Common code for default and stencil texture rendering shaders
bool BaseTest::setTextureUniform(GLuint programId)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(programId);

	GLint uniformTex = gl.getUniformLocation(programId, "tex");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation");

	if (uniformTex == -1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Error getting uniform tex" << tcu::TestLog::EndMessage;
		return false;
	}

	gl.uniform1i(uniformTex, 0);
	return true;
}

// Loads texture rendering shader
bool BaseTest::setupTextureProgram()
{
	const char* vs = "\n"
					 "precision highp float;\n"
					 "in vec4 pos;\n"
					 "in vec2 UV;\n"
					 "out vec2 vUV;\n"
					 "void main() {\n"
					 "  gl_Position = pos;\n"
					 "  vUV = UV;\n"
					 "}\n";

	const char* fs = "\n"
					 "precision highp float;\n"
					 "in vec2 vUV;\n"
					 "out vec4 color;\n"
					 "uniform sampler2D tex;\n"
					 "void main() {\n"
					 "  color = texture(tex, vUV).rrra;\n"
					 "}\n";

	if (!m_textureProgram)
		m_textureProgram = createProgram(vs, fs);
	if (!m_textureProgram)
		return false;

	return setTextureUniform(m_textureProgram);
}

// Loads texture stencil rendering shader
bool BaseTest::setupStencilProgram()
{
	const char* vs = "\n"
					 "precision highp float;\n"
					 "in vec4 pos;\n"
					 "in vec2 UV;\n"
					 "out vec2 vUV;\n"
					 "void main() {\n"
					 "  gl_Position = pos;\n"
					 "  vUV = UV;\n"
					 "}\n";

	const char* fs = "\n"
					 "precision highp float;\n"
					 "in vec2 vUV;\n"
					 "out vec4 color;\n"
					 "uniform highp usampler2D tex;\n"
					 "void main() {\n"
					 "  float s = float(texture(tex, vUV).r);\n"
					 "  s /= 255.0;\n"
					 "  color = vec4(s, s, s, 1);\n"
					 "}\n";

	if (!m_stencilProgram)
		m_stencilProgram = createProgram(vs, fs);
	if (!m_stencilProgram)
		return false;

	return setTextureUniform(m_stencilProgram);
}

void BaseTest::drawQuad(DrawMode drawMode, GLuint program)
{
	static const GLfloat verticesDefault[] = {
		-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
	};

	static const GLfloat verticesDepthSpan1[] = {
		-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
	};

	static const GLfloat verticesDepthSpan2[] = {
		-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
	};

	static const GLfloat texCoords[] = {
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
	};

	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
	static PrimitiveList  quadPrimitive = glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices);

	static const glu::VertexArrayBinding depthSpanVA1[] = { glu::va::Float("pos", 4, 4, 0, verticesDepthSpan1) };
	static const glu::VertexArrayBinding depthSpanVA2[] = { glu::va::Float("pos", 4, 4, 0, verticesDepthSpan2) };
	static const glu::VertexArrayBinding defaultVA[]	= { glu::va::Float("pos", 4, 4, 0, verticesDefault),
														 glu::va::Float("UV", 2, 4, 0, texCoords) };

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	if (drawMode == DEPTH_SPAN1)
		glu::draw(renderContext, program, 1, depthSpanVA1, quadPrimitive);
	else if (drawMode == DEPTH_SPAN2)
		glu::draw(renderContext, program, 1, depthSpanVA2, quadPrimitive);
	else
		glu::draw(renderContext, program, 2, defaultVA, quadPrimitive);
}

// Renders all non-trivial startup textures
void BaseTest::renderToTextures()
{
	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();

	GLint uColor;
	setupColorProgram(uColor);

	gl.enable(GL_DEPTH_TEST);
	// depth writing must be enabled as it is disabled in places like doReadPixels
	gl.depthMask(GL_TRUE);

	if (glu::isContextTypeES(renderContext.getType()))
		gl.clearDepthf(1.0f);
	else
		gl.clearDepth(1.0);

	gl.depthFunc(GL_LEQUAL);
	gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);

	drawQuad(DEPTH_SPAN1, m_colorProgram);

	// packedTexRender
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRender]);

	setDrawReadBuffer(GL_NONE, GL_NONE);

	gl.enable(GL_STENCIL_TEST);
	gl.stencilFunc(GL_ALWAYS, 0x0, 0xFF);
	gl.stencilOp(GL_ZERO, GL_INCR, GL_INCR);

	gl.clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	drawQuad(DEPTH_SPAN1, m_colorProgram);

	gl.disable(GL_STENCIL_TEST);

	restoreDrawReadBuffer();

	// packedTexRenderInitStencil
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderInitStencil]);

	setDrawReadBuffer(GL_NONE, GL_NONE);

	gl.clear(GL_DEPTH_BUFFER_BIT);
	drawQuad(DEPTH_SPAN1, m_colorProgram);

	restoreDrawReadBuffer();

	// packedTexRenderDepthStep
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderDepthStep]);

	setDrawReadBuffer(GL_NONE, GL_NONE);

	gl.clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	drawQuad(DEPTH_SPAN2, m_colorProgram);
	drawQuad(DEPTH_SPAN1, m_colorProgram);

	restoreDrawReadBuffer();

	// packedTexRenderStencilStep
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRenderStencilStep]);

	setDrawReadBuffer(GL_NONE, GL_NONE);

	gl.clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	gl.enable(GL_SCISSOR_TEST);
	gl.scissor(0, 0, TEX_SIZE, TEX_SIZE / 2);

	gl.enable(GL_STENCIL_TEST);
	gl.stencilFunc(GL_ALWAYS, 0x0, 0xFF);
	gl.stencilOp(GL_ZERO, GL_INCR, GL_INCR);
	for (int i = 0; i < 256; i++)
		drawQuad(DEPTH_SPAN2, m_colorProgram);
	gl.disable(GL_SCISSOR_TEST);

	gl.stencilFunc(GL_EQUAL, 0xFF, 0xFF);
	gl.clear(GL_DEPTH_BUFFER_BIT);
	drawQuad(DEPTH_SPAN1, m_colorProgram);
	gl.disable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable");

	restoreDrawReadBuffer();

	// end
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
}

// Verifies DepthStencil buffer data against reference values
bool BaseTest::verifyDepthStencilGradient(GLvoid* data, unsigned int texIndex, int width, int height)
{
	bool result = true;

	int index, skip;
	int countD, countS;

	index  = 0;
	countD = 0;
	countS = 0;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			float d, dref = 0.0;
			int   s, sref = 0;

			skip = 0;

			switch (m_typeFormat.type)
			{
			case GL_UNSIGNED_INT_24_8:
			{
				GLuint v = ((GLuint*)data)[index];
				d		 = ((float)(v >> 8)) / 0xffffff;
				s		 = v & 0xff;
				break;
			}
			case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
			{
				D32F_S8 v = ((D32F_S8*)data)[index];
				d		  = v.d;
				s		  = v.s & 0xff;
				break;
			}
			default:
				d = -1;
				s = -1;
				break;
			}

			switch (texIndex)
			{
			case packedTexImage:
				dref = ((float)i) / (width - 1);
				sref = (int)(dref * 255);
				break;
			case packedTexRender:
				dref = ((float)j) / (height - 1);
				sref = 1;
				break;
			case packedTexRenderInitStencil:
				dref = ((float)j) / (height - 1);
				sref = (int)(((float)i) / (width - 1) * 255);
				break;
			case packedTexRenderDepthStep:
				if (j < height * TOLERANCE_LOW)
				{
					dref = ((float)j) / (height - 1);
					sref = 0;
				}
				else if (j > height * TOLERANCE_HIGH)
				{
					dref = 1.0f - ((float)j) / (height - 1);
					sref = 0;
				}
				else
				{
					skip = 1; // give some tolerance to pixels in the middle
				}
				break;
			case packedTexRenderStencilStep:
				if (j < height * TOLERANCE_LOW)
				{
					dref = ((float)j) / (height - 1);
					sref = 255;
				}
				else if (j > height * TOLERANCE_HIGH)
				{
					dref = 1;
					sref = 0;
				}
				else
				{
					skip = 1; // give some tolerance to pixels in the middle
				}
				break;
			case verifyCopyTexImage:
				if (j < height * TOLERANCE_LOW)
				{
					dref = ((float)j) / (height - 1);
					sref = 1;
				}
				else if (j > height * TOLERANCE_HIGH)
				{
					dref = 0.5;
					sref = 1;
				}
				else
				{
					skip = 1; // give some tolerance to pixels in the middle
				}
				break;
			default:
				dref = -2;
				sref = -2;
				break;
			}

			if (!skip)
			{
				if (deFloatAbs(d - dref) > EPSILON)
				{
					result = false;
					countD++;
				}

				if (s != sref)
				{
					result = false;
					countS++;
				}
			}
			else
			{
				skip = 0;
			}

			index++;
		}
	}

	if (countD || countS)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "DEPTH_STENCIL comparison failed" << tcu::TestLog::EndMessage;
	}
	return result;
}

// Verifies Color buffer data against reference values
bool BaseTest::verifyColorGradient(GLvoid* data, unsigned int texIndex, int function, int width, int height)
{
	bool result = true;

	int index   = 0, skip;
	int channel = 0;
	int count   = 0;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			skip			= 0;
			GLuint color	= ((GLuint*)data)[index];
			GLuint colorref = 0;

			switch (texIndex)
			{
			case packedTexImage:
				channel  = (int)(((float)i) / (width - 1) * 255);
				colorref = 0xff000000 + channel * 0x00010101;
				break;
			case packedTexRender:
				if (function == COLOR_CHECK_DEPTH)
					channel = (int)(((float)j) / (height - 1) * 255);
				else
					channel = 1;
				colorref	= 0xff000000 + channel * 0x00010101;
				break;
			case packedTexRenderInitStencil:
				if (function == COLOR_CHECK_DEPTH)
					channel = (int)(((float)j) / (height - 1) * 255);
				else
					channel = (int)(((float)i) / (width - 1) * 255);
				colorref	= 0xff000000 + channel * 0x00010101;
				break;
			case packedTexRenderDepthStep:
				if (function == COLOR_CHECK_DEPTH)
				{
					if (j < height * TOLERANCE_LOW)
						channel = (int)(((float)j) / (height - 1) * 255);
					else if (j > height * TOLERANCE_HIGH)
						channel = 255 - (int)(((float)j) / (height - 1) * 255);
					else
						skip = 1; // give some tolerance to pixels in the middle
				}
				else
					channel = 0;
				colorref	= 0xff000000 + channel * 0x00010101;
				break;
			case packedTexRenderStencilStep:
				if (j < height * TOLERANCE_LOW)
				{
					if (function == COLOR_CHECK_DEPTH)
						channel = (int)(((float)j) / (height - 1) * 255);
					else
						channel = 255;
				}
				else if (j > height * TOLERANCE_HIGH)
					channel = (function == COLOR_CHECK_DEPTH) ? 255 : 0;
				else
					skip = 1; // give some tolerance to pixels in the middle
				colorref = 0xff000000 + channel * 0x00010101;
				break;
			case verifyCopyTexImage:
				if (j < height * TOLERANCE_LOW)
				{
					if (function == COLOR_CHECK_DEPTH)
						channel = (int)(((float)j) / (height - 1) * 255);
					else
						channel = 1;
				}
				else if (j > height * TOLERANCE_HIGH)
				{
					channel = (function == COLOR_CHECK_DEPTH) ? 127 : 1;
				}
				else
				{
					skip = 1; // give some tolerance to pixels in the middle
				}
				colorref = 0xff000000 + channel * 0x00010101;
				break;
			case verifyPartialAttachments:
				colorref = 0xffffffff;
				break;
			case verifyMixedAttachments:
				if (j > height * TOLERANCE_HIGH)
					colorref = 0xffffffff;
				else if (j < height * TOLERANCE_LOW)
					colorref = 0xcccccccc;
				else
					skip = 1;
				break;
			case verifyClearBufferDepth:
				if ((i & 0xff) == 0xff)
					colorref = 0xffffffff;
				else
					colorref = 0xcccccccc;
				break;
			case verifyClearBufferStencil:
				if (i > width * TOLERANCE_HIGH)
					colorref = 0xffffffff;
				else if (i < width * TOLERANCE_LOW)
					colorref = 0xcccccccc;
				else
					skip = 1;
				break;
			case verifyClearBufferDepthStencil:
				colorref = 0xffffffff;
				break;
			case verifyBlit:
				if (j > height * TOLERANCE_HIGH)
					colorref = 0xffffffff;
				else if (j < height * TOLERANCE_LOW)
					colorref = 0xcccccccc;
				else
					skip = 1;
				break;
			default:
				colorref = 0xdeadbeef;
				break;
			}

			if (skip)
				skip = 0;
			else if (color != colorref)
			{
				float d	= (float)(color & 0xff) / 0xff;
				float dref = (float)(colorref & 0xff) / 0xff;
				if (!((function == COLOR_CHECK_DEPTH) && (deFloatAbs(d - dref) < EPSILON)))
				{
					result = false;
					count++;
				}
			}

			index++;
		}
	}

	if (count)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "*** Color comparison failed" << tcu::TestLog::EndMessage;
		result = false;
	}
	return result;
}

// Verify DepthStencil texture by replicating it to color channels
// so it can be read using ReadPixels in Halti.
bool BaseTest::doReadPixels(GLuint texture, int function)
{
	bool				  result = true;
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint texColor;
	gl.genTextures(1, &texColor);
	gl.bindTexture(GL_TEXTURE_2D, texColor);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);
	setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);

	// Step 1: Verify depth values
	GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
						   << tcu::TestLog::EndMessage;
		result = false;
	}
	else
	{
		setupTextureProgram();

		gl.bindTexture(GL_TEXTURE_2D, texture);

		gl.disable(GL_DEPTH_TEST);
		gl.depthMask(GL_FALSE);
		gl.disable(GL_STENCIL_TEST);
		gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
		gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
		gl.clear(GL_COLOR_BUFFER_BIT);
		drawQuad(DEFAULT, m_textureProgram);

		std::vector<GLuint> dataColor(TEX_SIZE * TEX_SIZE, 0);
		gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &dataColor[0]);
		result &= verifyColorGradient(&dataColor[0], function, COLOR_CHECK_DEPTH, TEX_SIZE, TEX_SIZE);

		// Step 2: Verify stencil values
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

		status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
							   << tcu::TestLog::EndMessage;
			result = false;
		}
		else
		{
			GLint uColor;
			setupColorProgram(uColor);

			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT);

			gl.enable(GL_STENCIL_TEST);
			for (int i = 0; i < 256; i++)
			{
				float v = i / 255.0f;
				gl.uniform4f(uColor, v, v, v, 1.0f);
				gl.stencilFunc(GL_EQUAL, i, 0xFF);
				gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				drawQuad(DEFAULT, m_colorProgram);
			}

			gl.disable(GL_STENCIL_TEST);
			dataColor.assign(dataColor.size(), 0);
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &dataColor[0]);
			result &= verifyColorGradient(&dataColor[0], function, COLOR_CHECK_STENCIL, TEX_SIZE, TEX_SIZE);
		}
	}

	// clean up
	restoreDrawReadBuffer();
	gl.deleteFramebuffers(1, &fbo);
	gl.deleteTextures(1, &texColor);

	return result;
}

class InitialStateTest : public deqp::TestCase
{
public:
	InitialStateTest(deqp::Context& context);
	virtual ~InitialStateTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

InitialStateTest::InitialStateTest(deqp::Context& context)
	: deqp::TestCase(context, "initial_state", "TEXTURE_STENCIL_SIZE for the default texture objects should be 0")
{
}

InitialStateTest::~InitialStateTest()
{
}

tcu::TestNode::IterateResult InitialStateTest::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(coreTexTargets); i++)
	{
		GLenum target = coreTexTargets[i];

		GLfloat fp;
		gl.getTexLevelParameterfv(target, 0, GL_TEXTURE_STENCIL_SIZE, &fp);
		if (deFloatCmpNE(fp, 0.0f))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "gl.getTexLevelParameterfv: Parameter is not 0"
							   << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}

		GLint ip;
		gl.getTexLevelParameteriv(target, 0, GL_TEXTURE_STENCIL_SIZE, &ip);
		if (deFloatCmpNE(ip, 0.0f))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "gl.getTexLevelParameteriv: Parameter is not 0"
							   << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

class ValidateErrorsTest : public BaseTest
{
public:
	ValidateErrorsTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~ValidateErrorsTest();

	virtual tcu::TestNode::IterateResult iterate(void);

protected:
	bool checkErrors();
};

ValidateErrorsTest::ValidateErrorsTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

ValidateErrorsTest::~ValidateErrorsTest()
{
}

tcu::TestNode::IterateResult ValidateErrorsTest::iterate(void)
{
	createTextures();
	if (checkErrors())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	destroyTextures();
	return STOP;
}

//  Error tests [desktop only]:

//  - The error INVALID_ENUM is generated if ReadPixels is
//	called where format is DEPTH_STENCIL and type is not
//	UNSIGNED_INT_24_8, or FLOAT_32_UNSIGNED_INT_24_8_REV.

//  - The error INVALID_OPERATION is generated if ReadPixels
//	is called where type is UNSIGNED_INT_24_8 or
//	FLOAT_32_UNSIGNED_INT_24_8_REV and format is not DEPTH_STENCIL.

//  - The error INVALID_OPERATION is generated if ReadPixels
//	is called where format is DEPTH_STENCIL and there is not both a
//	depth buffer and a stencil buffer.

//  - Calling GetTexImage with a <format> of DEPTH_COMPONENT when the
//	base internal format of the texture image is not DEPTH_COMPONENT
//	or DEPTH_STENCIL causes the error INVALID_OPERATION.

//  - Calling GetTexImage with a <format> of DEPTH_STENCIL when
//	the base internal format of the texture image is not
//	DEPTH_STENCIL causes the error INVALID_OPERATION.

//  Error tests [Halti only]:

//  - The error INVALID_ENUM is generated if ReadPixels is
//	called where format is DEPTH_STENCIL.

//  Error tests [desktop and Halti]:

//  - TexImage generates INVALID_OPERATION if one of the base internal format
//	and format is DEPTH_COMPONENT or DEPTH_STENCIL, and the other is neither
//	of these values.

//  - The error INVALID_OPERATION is generated if CopyTexImage
//	is called where format is DEPTH_STENCIL and there is not both a
//	depth buffer and a stencil buffer.
bool ValidateErrorsTest::checkErrors()
{
	bool					  result = true;
	GLuint					  fbo, fbo2;
	GLuint					  texColor;
	std::vector<GLfloat>	  data(4 * TEX_SIZE * TEX_SIZE, 0.0f);
	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();
	bool					  isContextES   = glu::isContextTypeES(renderContext.getType());

	if (isContextES)
	{
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexImage]);
		setDrawReadBuffer(GL_NONE, GL_NONE);
		gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);

		GLenum error = gl.getError();
		if (((GL_INVALID_OPERATION != error) && (GL_INVALID_ENUM != error)) &&
			!((GL_NO_ERROR == error) && m_context.getContextInfo().isExtensionSupported("GL_NV_read_depth_stencil")))
		{
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);
			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;
		}

		restoreDrawReadBuffer();
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
	}
	else
	{
		gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexImage]);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexImage]);
		setDrawReadBuffer(GL_NONE, GL_NONE);

		for (unsigned int i = 0; i < m_nonDepthStencilTypesCount; i++)
		{
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_DEPTH_STENCIL, m_nonDepthStencilTypes[i], &data[0]);
			if (gl.getError() != GL_INVALID_ENUM)
				result = false;
		}

		for (unsigned int i = 0; i < m_nonDepthStencilFormatsCount; i++)
		{
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, m_nonDepthStencilFormats[i], m_typeFormat.type, &data[0]);

			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;
		}

		for (int i = 0; i < 2; i++)
		{
			// setup texture/fbo
			gl.genTextures(1, &texColor);
			gl.genFramebuffers(1, &fbo);
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

			GLenum attachmentType = (i == 0) ? GL_DEPTH_ATTACHMENT : GL_STENCIL_ATTACHMENT;
			gl.framebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, m_textures[packedTexImage], 0);

			gl.bindTexture(GL_TEXTURE_2D, texColor);
			setupTexture();
			gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);

			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);
			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
			gl.bindTexture(GL_TEXTURE_2D, 0);
			gl.deleteFramebuffers(1, &fbo);
			gl.deleteTextures(1, &texColor);
		}

		for (unsigned int i = 0; i < m_otherBaseFormatsCount; i++)
		{
			GLenum format = m_otherBaseFormats[i];
			gl.genTextures(1, &texColor);
			gl.bindTexture(GL_TEXTURE_2D, texColor);
			setupTexture();
			gl.texImage2D(GL_TEXTURE_2D, 0, format, TEX_SIZE, TEX_SIZE, 0, format, GL_UNSIGNED_BYTE, 0);

			if (format != GL_DEPTH_COMPONENT)
			{
				gl.getTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, &data[0]);

				if (gl.getError() != GL_INVALID_OPERATION)
					result = false;
			}

			gl.getTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, &data[0]);
			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;

			gl.deleteTextures(1, &texColor);
		}
	}

	bool coreGL = !glu::isContextTypeES(m_context.getRenderContext().getType());
	for (int i = 0; i < 4; i++)
	{
		int limit;
		if (i < 2)
			limit = m_nonDepthStencilFormatsCount;
		else
			limit = m_otherBaseFormatsCount;

		for (int j = 0; j < limit; j++)
		{
			GLint internalFormat = 0;
			GLint format		 = 0;

			gl.genTextures(1, &texColor);
			gl.bindTexture(GL_TEXTURE_2D, texColor);
			setupTexture();

			switch (i)
			{
			case 0:
				internalFormat = GL_DEPTH_COMPONENT;
				format		   = m_nonDepthStencilFormats[j];
				break;
			case 1:
				internalFormat = GL_DEPTH_STENCIL;
				format		   = m_nonDepthStencilFormats[j];
				break;
			case 2:
				internalFormat = m_otherBaseFormats[j];
				format		   = GL_DEPTH_COMPONENT;
				break;
			case 3:
				internalFormat = m_otherBaseFormats[j];
				format		   = GL_DEPTH_STENCIL;
				break;
			}

			gl.texImage2D(GL_TEXTURE_2D, 0, internalFormat, TEX_SIZE, TEX_SIZE, 0, format,
						  (format == GL_DEPTH_STENCIL) ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE, 0);

			GLenum expectedError = GL_INVALID_OPERATION;
			if (coreGL && (format == GL_STENCIL_INDEX))
			{
				// The OpenGL 4.3 spec is imprecise about what error this should generate
				// see Bugzilla 10134: TexImage with a <format> of STENCIL_INDEX
				//	 4.3 core (Feb 14 2013) p. 174:
				//	 (describing TexImage3D)
				//		 The format STENCIL_INDEX is not allowed.
				// The new OpenGL 4.4 feature ARB_texture_stencil8 removes this error. So
				// the best we can do for OpenGL is to just allow any error, or no error,
				// for this specific case.
				gl.getError();
			}
			else if (gl.getError() != expectedError)
				result = false;

			gl.bindTexture(GL_TEXTURE_2D, 0);
			gl.deleteTextures(1, &texColor);
		}
	}

	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexImage], 0);

	gl.genFramebuffers(1, &fbo2);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo2);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_textures[packedTexImage], 0);

	GLuint tex;
	gl.genTextures(1, &tex);
	gl.bindTexture(GL_TEXTURE_2D, tex);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type, 0);

	for (int i = 0; i < 2; i++)
	{
		switch (i)
		{
		case 0:
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
			gl.bindTexture(GL_TEXTURE_2D, tex);
			break;
		case 1:
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo2);
			gl.bindTexture(GL_TEXTURE_2D, tex);
			break;
		}

		setDrawReadBuffer(GL_NONE, GL_NONE);

		gl.copyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, 0, 0, TEX_SIZE, TEX_SIZE, 0);

		GLenum error = gl.getError();
		if ((GL_INVALID_OPERATION != error) && (GL_INVALID_ENUM != error))
		{
			gl.copyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, 0, 0, TEX_SIZE, TEX_SIZE, 0);
			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;
		}

		restoreDrawReadBuffer();
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
	}

	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.deleteTextures(1, &tex);

	gl.deleteFramebuffers(1, &fbo);
	gl.deleteFramebuffers(1, &fbo2);

	return result;
}

class VerifyReadPixelsTest : public BaseTest
{
public:
	VerifyReadPixelsTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyReadPixelsTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyReadPixelsTest::VerifyReadPixelsTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

VerifyReadPixelsTest::~VerifyReadPixelsTest()
{
}

tcu::TestNode::IterateResult VerifyReadPixelsTest::iterate(void)
{
	//  Use readpixels to verify the results for the 5 textures above are correct.
	//	Note that in ES you can only use gl.readpixels on color buffers.
	//  Test method:
	//  - on desktop: ReadPixel DEPTH_STENCIL value to buffer. Verify gradient.
	//  - on desktop/Halti: Create FBO with color/depth/stencil attachment.
	//	Draw a quad with depth texture bound. Verify gradient.
	//	Draw 256 times using stencil test and gradient color. Verify gradient.

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();
	std::size_t				  dataSize		= static_cast<std::size_t>(TEX_SIZE * TEX_SIZE * m_typeFormat.size);
	std::vector<GLubyte>	  data(dataSize);

	createTextures();
	renderToTextures();

	bool result = true;
	for (int i = 0; i < NUM_TEXTURES; i++)
	{
		// Read DEPTH_STENCIL value, applies only to desktop
		if (!glu::isContextTypeES(renderContext.getType()))
		{
			gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[i]);

			setDrawReadBuffer(GL_NONE, GL_NONE);

			data.assign(TEX_SIZE * TEX_SIZE * m_typeFormat.size, 0);
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);
			result &= verifyDepthStencilGradient(&data[0], i, TEX_SIZE, TEX_SIZE);

			restoreDrawReadBuffer();
		}

		// On ES3.2 we have to render to color buffer to verify.
		// We can run this also on desktop.
		result &= doReadPixels(m_textures[i], i);
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class VerifyGetTexImageTest : public BaseTest
{
public:
	VerifyGetTexImageTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyGetTexImageTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyGetTexImageTest::VerifyGetTexImageTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

VerifyGetTexImageTest::~VerifyGetTexImageTest()
{
}

tcu::TestNode::IterateResult VerifyGetTexImageTest::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	std::vector<GLubyte>  data(TEX_SIZE * TEX_SIZE * m_typeFormat.size);

	createTextures();
	renderToTextures();

	bool result = true;
	for (int i = 0; i < NUM_TEXTURES; i++)
	{
		data.assign(TEX_SIZE * TEX_SIZE * m_typeFormat.size, 0);
		gl.bindTexture(GL_TEXTURE_2D, m_textures[i]);
		gl.getTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);
		result &= verifyDepthStencilGradient(&data[0], i, TEX_SIZE, TEX_SIZE);
	}

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class VerifyCopyTexImageTest : public BaseTest
{
public:
	VerifyCopyTexImageTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyCopyTexImageTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyCopyTexImageTest::VerifyCopyTexImageTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

VerifyCopyTexImageTest::~VerifyCopyTexImageTest()
{
}

tcu::TestNode::IterateResult VerifyCopyTexImageTest::iterate(void)
{
	// After rendering to depth and stencil, CopyTexImage the results to a new
	// DEPTH_STENCIL texture. Attach this texture to a new FBO. Verify that
	// depth and stencil tests work with the copied data.

	createTextures();
	renderToTextures();

	bool				  result = true;
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();

	// setup shader
	GLint uColor;
	setupColorProgram(uColor);

	// setup and copy texture/fbo
	GLuint tex;
	gl.genTextures(1, &tex);
	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexRender]);
	gl.bindTexture(GL_TEXTURE_2D, tex);
	setupTexture();

	setDrawReadBuffer(GL_NONE, GL_NONE);

	gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
				  NULL);
	gl.copyTexImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, 0, 0, TEX_SIZE, TEX_SIZE, 0);

	restoreDrawReadBuffer();

	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
	setDrawReadBuffer(GL_NONE, GL_NONE);

	int status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
						   << tcu::TestLog::EndMessage;
		result = false;
		restoreDrawReadBuffer();
	}
	else
	{
		// render
		gl.enable(GL_DEPTH_TEST);
		gl.depthFunc(GL_LEQUAL);
		gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
		gl.enable(GL_STENCIL_TEST);
		gl.stencilFunc(GL_EQUAL, 0x1, 0xFF);
		gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		drawQuad(DEFAULT, m_colorProgram);
		gl.disable(GL_STENCIL_TEST);

		// verify
		std::vector<GLubyte> data(TEX_SIZE * TEX_SIZE * m_typeFormat.size, 0);
		gl.getTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL, m_typeFormat.type, &data[0]);
		result &= verifyDepthStencilGradient(&data[0], verifyCopyTexImage, TEX_SIZE, TEX_SIZE);

		restoreDrawReadBuffer();

		result &= doReadPixels(tex, verifyCopyTexImage);
	}

	// clean up
	gl.deleteFramebuffers(1, &fbo);
	gl.deleteTextures(1, &tex);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class VerifyPartialAttachmentsTest : public BaseTest
{
public:
	VerifyPartialAttachmentsTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyPartialAttachmentsTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyPartialAttachmentsTest::VerifyPartialAttachmentsTest(deqp::Context& context, const TypeFormat& tf)
	: BaseTest(context, tf)
{
}

VerifyPartialAttachmentsTest::~VerifyPartialAttachmentsTest()
{
}

tcu::TestNode::IterateResult VerifyPartialAttachmentsTest::iterate(void)
{
	createTextures();
	renderToTextures();

	bool result = true;

	//  - Create an FBO with a packed depth stencil renderbuffer attached to
	//	DEPTH_ATTACHMENT only. If this FBO is complete, stencil test must act as
	//	if there is no stencil buffer (always pass.)

	//  - Create an FBO with a packed depth stencil renderbuffer attached to
	//	STENCIL_ATTACHMENT only. If this FBO is complete, depth test must act as
	//	if there is no depth buffer (always pass.)

	//  - Create an FBO with a packed depth stencil renderbuffer attached to
	//	STENCIL_ATTACHMENT only. If this FBO is complete, occlusion query must
	//	act as if there is no depth buffer (always pass.)

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();
	bool					  isContextES   = glu::isContextTypeES(renderContext.getType());

	// setup shader
	GLint uColor;
	setupColorProgram(uColor);

	GLuint occ;
	gl.genQueries(1, &occ);

	for (int i = 0; i < 3; i++)
	{
		// setup fbo
		GLuint fbo;
		gl.genFramebuffers(1, &fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

		GLuint rbo[2]; // color, D/S
		gl.genRenderbuffers(2, rbo);
		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, TEX_SIZE, TEX_SIZE);
		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
		gl.renderbufferStorage(GL_RENDERBUFFER, m_typeFormat.format, TEX_SIZE, TEX_SIZE);

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
		setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);

		switch (i)
		{
		case 0:
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
			break;
		case 1:
		case 2:
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
			break;
		default:
			result = false;
		}

		if (!result)
			break;

		int status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
							   << tcu::TestLog::EndMessage;
			result = false;
		}
		else
		{
			// render
			gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
			if (isContextES)
				gl.clearDepthf(1.0f);
			else
				gl.clearDepth(1.0);

			gl.clearStencil(0);
			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			switch (i)
			{
			case 0:
				gl.disable(GL_DEPTH_TEST);
				gl.enable(GL_STENCIL_TEST);
				gl.stencilFunc(GL_NEVER, 0xFF, 0xFF);
				gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				break;
			case 1:
				gl.enable(GL_DEPTH_TEST);
				gl.depthFunc(GL_NEVER);
				gl.disable(GL_STENCIL_TEST);
				break;
			case 2:
				gl.enable(GL_DEPTH_TEST);
				gl.depthFunc(GL_NEVER);
				gl.disable(GL_STENCIL_TEST);
				gl.beginQuery(GL_ANY_SAMPLES_PASSED, occ);
				break;
			default:
				break;
			}

			drawQuad(DEFAULT, m_colorProgram);

			if (i == 2)
			{
				GLuint n;

				gl.endQuery(GL_ANY_SAMPLES_PASSED);
				gl.getQueryObjectuiv(occ, GL_QUERY_RESULT, &n);

				if (n > 0)
				{
					drawQuad(DEFAULT, m_colorProgram);
				}
			}

			std::vector<GLuint> data(TEX_SIZE * TEX_SIZE, 0);
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
			result &= verifyColorGradient(&data[0], verifyPartialAttachments, COLOR_CHECK_DEFAULT, TEX_SIZE, TEX_SIZE);
		}

		restoreDrawReadBuffer();

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

		// clean up
		gl.deleteFramebuffers(1, &fbo);
		gl.deleteRenderbuffers(2, rbo);
	}

	gl.deleteQueries(1, &occ);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class VerifyMixedAttachmentsTest : public BaseTest
{
public:
	VerifyMixedAttachmentsTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyMixedAttachmentsTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyMixedAttachmentsTest::VerifyMixedAttachmentsTest(deqp::Context& context, const TypeFormat& tf)
	: BaseTest(context, tf)
{
}

VerifyMixedAttachmentsTest::~VerifyMixedAttachmentsTest()
{
}

tcu::TestNode::IterateResult VerifyMixedAttachmentsTest::iterate(void)
{
	// Create FBOs that mix DEPTH_STENCIL renderbuffers with DEPTH or STENCIL
	// renderbuffers. If these FBOs are complete, depth and stencil test
	// must work properly.

	// Create an FBO with two different packed depth stencil renderbuffers, one
	// attached to DEPTH_ATTACHMENT and the other attached to STENCIL_ATTACHMENT.
	// Querying DEPTH_STENCIL_ATTACHMENT must fail with INVALID_OPERATION. If
	// this FBO is complete, depth and stencil tests must work properly.

	createTextures();
	renderToTextures();

	bool result = true;

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();
	bool					  isContextES   = glu::isContextTypeES(renderContext.getType());

	GLint uColor;
	setupColorProgram(uColor);

	for (int i = 0; i < 3; i++)
	{
		// set up FBO/RBOs
		GLuint fbo;
		gl.genFramebuffers(1, &fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

		GLuint rbo[3]; // color, DEPTH_STENCIL, DEPTH/STENCIL
		gl.genRenderbuffers(3, rbo);

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, TEX_SIZE, TEX_SIZE);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
		gl.renderbufferStorage(GL_RENDERBUFFER, m_typeFormat.format, TEX_SIZE, TEX_SIZE);

		gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[2]);
		switch (i)
		{
		case 0:
			gl.renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[2]);
			break;
		case 1:
			gl.renderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[2]);
			break;
		case 2:
			gl.renderbufferStorage(GL_RENDERBUFFER, m_typeFormat.format, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[2]);

			GLint param;
			gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
												   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);
			if (gl.getError() != GL_INVALID_OPERATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Expected INVALID_OPERATION for DEPTH_STENCIL_ATTACHMENT query"
								   << tcu::TestLog::EndMessage;
				result = false;
			}

			break;
		}

		GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			if (status == GL_FRAMEBUFFER_UNSUPPORTED)
			{
				/* The spec only requires

					 "when both depth and stencil attachments are present, implementations are only
					  required to support framebuffer objects where both attachments refer to the same image."

				   Thus, it is accepatable for an implementation returning GL_FRAMEBUFFER_UNSUPPORTED.  And the
				   test can NOT be marked as fail.
				 */
			}
			else
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete" << tcu::TestLog::EndMessage;
				result = false;
			}
		}
		else
		{

			// render
			// step 1
			gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
			gl.enable(GL_DEPTH_TEST);
			gl.depthFunc(GL_LEQUAL);

			if (isContextES)
				gl.clearDepthf(1.0f);
			else
				gl.clearDepth(1.0);

			gl.enable(GL_STENCIL_TEST);
			gl.stencilFunc(GL_ALWAYS, 0x1, 0xFF);
			gl.stencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			gl.clearStencil(0);
			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			drawQuad(DEPTH_SPAN1, m_colorProgram);

			// step 2
			gl.stencilFunc(GL_EQUAL, 0x1, 0xFF);
			gl.stencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT);
			drawQuad(DEFAULT, m_colorProgram);

			std::vector<GLuint> data(TEX_SIZE * TEX_SIZE, 0);
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
			result &= verifyColorGradient(&data[0], verifyMixedAttachments, COLOR_CHECK_DEFAULT, TEX_SIZE, TEX_SIZE);
		}

		// clean up
		gl.deleteRenderbuffers(3, rbo);
		gl.deleteFramebuffers(1, &fbo);
	}

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class VerifyParametersTest : public BaseTest
{
public:
	VerifyParametersTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~VerifyParametersTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

VerifyParametersTest::VerifyParametersTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

VerifyParametersTest::~VerifyParametersTest()
{
}

tcu::TestNode::IterateResult VerifyParametersTest::iterate(void)
{
	//  Verify GetFramebufferAttachmentParameter queries of each <pname> on
	//	DEPTH_STENCIL_ATTACHMENT work correctly if both attachments are populated
	//	with the same object.

	createTextures();
	renderToTextures();

	bool result = true;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexImage]);

	GLint param;
	gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
										   GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);
	if (param != GL_TEXTURE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Invalid value for GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE: " << param
						   << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	const AttachmentParam* attachmentParams = getAttachmentParams();
	for (GLuint i = 0; i < m_attachmentParamsCount; i++)
	{
		int	ref;
		GLenum pname = attachmentParams[i].pname;
		if (GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE == pname)
		{
			gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, pname, &param);
			if (gl.getError() != GL_INVALID_OPERATION)
				result = false;
			continue;
		}

		gl.getFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, pname, &param);

		ref = attachmentParams[i].value;
		if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME)
			ref = m_textures[packedTexImage];

		if (param != ref)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value for pname " << attachmentParams[i].pname
							   << ": " << param << " ( expected " << ref << ")" << tcu::TestLog::EndMessage;
			result = false;
		}
	}

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class RenderbuffersTest : public BaseTest
{
public:
	RenderbuffersTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~RenderbuffersTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

RenderbuffersTest::RenderbuffersTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

RenderbuffersTest::~RenderbuffersTest()
{
}

tcu::TestNode::IterateResult RenderbuffersTest::iterate(void)
{
	createTextures();
	renderToTextures();

	bool result = true;

	// Verify RENDERBUFFER_DEPTH_SIZE and RENDERBUFFER_STENCIL_SIZE report
	// appropriate values for for DEPTH_STENCIL renderbuffers.

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffers[packedTexImage]);

	GLuint rbo;
	gl.genRenderbuffers(1, &rbo);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
	gl.renderbufferStorage(GL_RENDERBUFFER, m_typeFormat.format, TEX_SIZE, TEX_SIZE);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	GLint param;
	gl.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &param);
	if (param != m_typeFormat.d)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid depth: " << param << ", expected: " << m_typeFormat.d
						   << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.getRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &param);
	if (param != m_typeFormat.s)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid stencil: " << param << ", expected: " << m_typeFormat.s
						   << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.deleteRenderbuffers(1, &rbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class StencilSizeTest : public BaseTest
{
public:
	StencilSizeTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~StencilSizeTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

StencilSizeTest::StencilSizeTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

StencilSizeTest::~StencilSizeTest()
{
}

tcu::TestNode::IterateResult StencilSizeTest::iterate(void)
{
	// [desktop only] Verify TEXTURE_STENCIL_SIZE reports 8 for DEPTH_STENCIL
	// textures, and 0 for RGBA and DEPTH_COMPONENT textures.

	createTextures();
	renderToTextures();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool  result = true;
	GLint param;
	gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexImage]);
	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_STENCIL_SIZE, &param);
	if (param != 8)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value for DEPTH_STENCIL stencil size: " << param
						   << tcu::TestLog::EndMessage;
		result = false;
	}

	GLuint texRGBA;
	gl.genTextures(1, &texRGBA);
	gl.bindTexture(GL_TEXTURE_2D, texRGBA);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_STENCIL_SIZE, &param);
	if (param != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value for RGBA stencil size: " << param
						   << tcu::TestLog::EndMessage;
		result = false;
	}
	gl.deleteTextures(1, &texRGBA);

	GLuint texDepth;
	gl.genTextures(1, &texDepth);
	gl.bindTexture(GL_TEXTURE_2D, texDepth);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT,
				  0);
	gl.getTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_STENCIL_SIZE, &param);
	if (param != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value for DEPTH_COMPONENT stencil size: " << param
						   << tcu::TestLog::EndMessage;
		result = false;
	}
	gl.deleteTextures(1, &texDepth);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class ClearBufferTest : public BaseTest
{
public:
	ClearBufferTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~ClearBufferTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

ClearBufferTest::ClearBufferTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

ClearBufferTest::~ClearBufferTest()
{
}

tcu::TestNode::IterateResult ClearBufferTest::iterate(void)
{
	// Verify ClearBufferfv correctly clears the depth channel of a DEPTH_STENCIL
	// FBO attachment (and does not touch the stencil channel.)
	// Verify ClearBufferiv correctly clears the stencil channel of a
	// DEPTH_STENCIL FBO attachment (and does not touch the depth channel.)
	// Verify ClearBufferfi correctly clears the depth and stencil channels of a
	// DEPTH_STENCIL FBO attachment.

	createTextures();
	renderToTextures();

	bool	result = true;
	GLfloat valuef;
	GLint   valuei;
	GLenum  status;

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// setup shader
	GLint uColor;
	setupColorProgram(uColor);

	for (int i = 0; i < 3; i++)
	{
		// setup texture/fbo
		GLuint tex;
		GLuint texColor;
		GLuint fbo;
		gl.genTextures(1, &tex);
		gl.genTextures(1, &texColor);
		gl.genFramebuffers(1, &fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

		gl.bindTexture(GL_TEXTURE_2D, tex);
		setupTexture();
		std::vector<GLbyte> data;
		createGradient(data);
		gl.texImage2D(GL_TEXTURE_2D, 0, m_typeFormat.format, TEX_SIZE, TEX_SIZE, 0, GL_DEPTH_STENCIL, m_typeFormat.type,
					  &data[0]);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex, 0);

		gl.bindTexture(GL_TEXTURE_2D, texColor);
		setupTexture();
		gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);

		setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);

		status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
							   << tcu::TestLog::EndMessage;
			result = false;
		}
		else
		{
			// clear relevant buffers
			switch (i)
			{
			case 0:
				valuef = 1.0f;
				gl.clearBufferfv(GL_DEPTH, 0, &valuef);
				break;
			case 1:
				valuei = 0xff;
				gl.clearBufferiv(GL_STENCIL, 0, &valuei);
				break;
			case 2:
				valuef = 1.0f;
				valuei = 0xff;
				gl.clearBufferfi(GL_DEPTH_STENCIL, 0, valuef, valuei);
				break;
			}

			// render reference image
			gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
			gl.enable(GL_DEPTH_TEST);
			gl.depthFunc(GL_LEQUAL);
			gl.enable(GL_STENCIL_TEST);
			gl.stencilFunc(GL_EQUAL, 0xFF, 0xFF);
			gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT);
			drawQuad(DEFAULT, m_colorProgram);

			// verify
			std::vector<GLubyte> readData(TEX_SIZE * TEX_SIZE * 4, 0);
			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &readData[0]);
			result &=
				verifyColorGradient(&readData[0], verifyClearBufferDepth + i, COLOR_CHECK_DEFAULT, TEX_SIZE, TEX_SIZE);
		}

		// destroy texture/fbo
		restoreDrawReadBuffer();

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.deleteFramebuffers(1, &fbo);
		gl.deleteTextures(1, &tex);
		gl.deleteTextures(1, &texColor);
	}

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class BlitTest : public BaseTest
{
public:
	BlitTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~BlitTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

BlitTest::BlitTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

BlitTest::~BlitTest()
{
}

tcu::TestNode::IterateResult BlitTest::iterate(void)
{
	// Verify that NEAREST filtered blits of DEPTH and/or STENCIL between two
	// FBOs with the same format packed depth stencil attachment work. Test
	// non-multisample [1->1], multisample resolve [N->1], and multisample
	// replicate [1->N] blits, for each supported value of N up to MAX_SAMPLES.

	createTextures();
	renderToTextures();

	GLuint fbo[3]; // Framebuffers: source, dest, downsample
	GLuint rbo[4]; // Renderbuffers: source D/S, dest D/S, dest color, downsample color
	GLint  maxSamples;
	int	srcSamples, destSamples;
	bool   result = true;

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();
	bool					  isContextES   = glu::isContextTypeES(renderContext.getType());

	std::vector<GLuint> data(TEX_SIZE * TEX_SIZE);

	GLint uColor;
	setupColorProgram(uColor);

	gl.getIntegerv(GL_MAX_SAMPLES, &maxSamples);

	// ES does not allow SAMPLE_BUFFERS for the draw frame
	// buffer is greater than zero when doing a blit.
	int loopCount = isContextES ? 1 : 2;

	for (int j = 0; j < loopCount; j++)
	{
		for (int i = 0; i <= maxSamples; i++)
		{
			// Create FBO/RBO
			gl.genFramebuffers(3, fbo);
			gl.genRenderbuffers(4, rbo);

			if (j == 0)
			{
				srcSamples  = i;
				destSamples = 0;
			}
			else
			{
				srcSamples  = 0;
				destSamples = i;
			}

			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
			gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, srcSamples, m_typeFormat.format, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[0]);

			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[1]);
			gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, destSamples, m_typeFormat.format, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);

			gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[2]);
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, destSamples, GL_RGBA8, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[2]);

			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[2]);
			gl.bindRenderbuffer(GL_RENDERBUFFER, rbo[3]);
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[3]);

			// Render
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
			setDrawReadBuffer(GL_NONE, GL_NONE);

			gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
			gl.enable(GL_DEPTH_TEST);
			gl.depthFunc(GL_LEQUAL);

			if (isContextES)
				gl.clearDepthf(1.0f);
			else
				gl.clearDepth(1.0);

			gl.enable(GL_STENCIL_TEST);
			gl.stencilFunc(GL_ALWAYS, 0x1, 0xFF);
			gl.stencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			gl.clearStencil(0);
			gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
			gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			drawQuad(DEPTH_SPAN1, m_colorProgram);

			restoreDrawReadBuffer();

			// Blit
			gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);
			setDrawReadBuffer(GL_NONE, GL_NONE);
			gl.blitFramebuffer(0, 0, TEX_SIZE, TEX_SIZE, 0, 0, TEX_SIZE, TEX_SIZE,
							   GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
			restoreDrawReadBuffer();

			// Verify
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[1]);
			setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);

			gl.stencilFunc(GL_EQUAL, 0x1, 0xFF);
			gl.stencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			gl.clear(GL_COLOR_BUFFER_BIT);
			drawQuad(DEFAULT, m_colorProgram);

			restoreDrawReadBuffer();

			// Downsample blit
			gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fbo[1]);
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[2]);
			setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);
			gl.blitFramebuffer(0, 0, TEX_SIZE, TEX_SIZE, 0, 0, TEX_SIZE, TEX_SIZE, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			restoreDrawReadBuffer();

			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo[2]);

			gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

			result &= verifyColorGradient(&data[0], verifyBlit, COLOR_CHECK_DEFAULT, TEX_SIZE, TEX_SIZE);

			// Clean up
			gl.bindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
			gl.bindRenderbuffer(GL_RENDERBUFFER, 0);

			gl.deleteRenderbuffers(4, rbo);
			gl.deleteFramebuffers(3, fbo);
		}
	}

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

class StencilTexturingTest : public BaseTest
{
public:
	StencilTexturingTest(deqp::Context& context, const TypeFormat& tf);
	virtual ~StencilTexturingTest();

	virtual tcu::TestNode::IterateResult iterate(void);
};

StencilTexturingTest::StencilTexturingTest(deqp::Context& context, const TypeFormat& tf) : BaseTest(context, tf)
{
}

StencilTexturingTest::~StencilTexturingTest()
{
}

tcu::TestNode::IterateResult StencilTexturingTest::iterate(void)
{
	// Verifies that either depth or stencil can be sampled depending on
	// GL_DEPTH_STENCIL_TEXTURE_MODE

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	glu::ContextType		  contextType   = renderContext.getType();
	const glw::Functions&	 gl			= renderContext.getFunctions();

	bool notSupported = false;
	if (glu::isContextTypeES(contextType))
		notSupported = !glu::contextSupports(contextType, glu::ApiType::es(3, 1));
	else
		notSupported = !glu::contextSupports(contextType, glu::ApiType::core(4, 3));

	if (notSupported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "stencil_texturing extension is not supported");
		return STOP;
	}

	createTextures();
	renderToTextures();

	bool   result = true;
	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint texColor;
	gl.genTextures(1, &texColor);
	gl.bindTexture(GL_TEXTURE_2D, texColor);
	setupTexture();
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColor, 0);
	setDrawReadBuffer(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0);

	// Step 1: Verify depth values
	GLenum status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer is incomplete: " << status
						   << tcu::TestLog::EndMessage;
		result = false;
	}
	else
	{
		gl.bindTexture(GL_TEXTURE_2D, m_textures[packedTexImage]);

		gl.disable(GL_DEPTH_TEST);
		gl.depthMask(GL_FALSE);
		gl.disable(GL_STENCIL_TEST);
		gl.viewport(0, 0, TEX_SIZE, TEX_SIZE);
		gl.clearColor(0.8f, 0.8f, 0.8f, 0.8f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
		setupTextureProgram();
		drawQuad(DEFAULT, m_textureProgram);

		std::vector<GLuint> dataColor(TEX_SIZE * TEX_SIZE, 0);
		gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &dataColor[0]);
		result &= verifyColorGradient(&dataColor[0], packedTexImage, COLOR_CHECK_DEPTH, TEX_SIZE, TEX_SIZE);

		// Step 2: Verify stencil values
		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		setupStencilProgram();
		drawQuad(DEFAULT, m_stencilProgram);

		dataColor.assign(TEX_SIZE * TEX_SIZE, 0);
		gl.readPixels(0, 0, TEX_SIZE, TEX_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, &dataColor[0]);
		result &= verifyColorGradient(&dataColor[0], packedTexImage, COLOR_CHECK_DEFAULT, TEX_SIZE, TEX_SIZE);
	}

	restoreDrawReadBuffer();
	gl.deleteFramebuffers(1, &fbo);
	gl.deleteTextures(1, &texColor);

	destroyTextures();
	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

PackedDepthStencilTests::PackedDepthStencilTests(deqp::Context& context)
	: TestCaseGroup(context, "packed_depth_stencil", "")
{
}

PackedDepthStencilTests::~PackedDepthStencilTests(void)
{
}

void PackedDepthStencilTests::init(void)
{
	TestCaseGroup* validateErrorsGroup			 = new deqp::TestCaseGroup(m_context, "validate_errors", "");
	TestCaseGroup* verifyReadPixelsGroup		 = new deqp::TestCaseGroup(m_context, "verify_read_pixels", "");
	TestCaseGroup* verifyGetTexImageGroup		 = new deqp::TestCaseGroup(m_context, "verify_get_tex_image", "");
	TestCaseGroup* verifyCopyTexImageGroup		 = new deqp::TestCaseGroup(m_context, "verify_copy_tex_image", "");
	TestCaseGroup* verifyPartialAttachmentsGroup = new deqp::TestCaseGroup(m_context, "verify_partial_attachments", "");
	TestCaseGroup* verifyMixedAttachmentsGroup   = new deqp::TestCaseGroup(m_context, "verify_mixed_attachments", "");
	TestCaseGroup* verifyParametersGroup		 = new deqp::TestCaseGroup(m_context, "verify_parameters", "");
	TestCaseGroup* renderbuffersGroup			 = new deqp::TestCaseGroup(m_context, "renderbuffers", "");
	TestCaseGroup* clearBufferGroup				 = new deqp::TestCaseGroup(m_context, "clear_buffer", "");
	TestCaseGroup* blitGroup					 = new deqp::TestCaseGroup(m_context, "blit", "");
	TestCaseGroup* stencilTexturingGroup		 = new deqp::TestCaseGroup(m_context, "stencil_texturing", "");
	TestCaseGroup* stencilSizeGroup				 = new deqp::TestCaseGroup(m_context, "stencil_size", "");

	bool isContextCoreGL = !glu::isContextTypeES(m_context.getRenderContext().getType());
	if (isContextCoreGL)
		validateErrorsGroup->addChild(new InitialStateTest(m_context));

	for (int i = 0; i < NUM_TEXTURE_TYPES; i++)
	{
		const TypeFormat& typeFormat = TextureTypes[i];
		validateErrorsGroup->addChild(new ValidateErrorsTest(m_context, typeFormat));
		verifyReadPixelsGroup->addChild(new VerifyReadPixelsTest(m_context, typeFormat));
		verifyPartialAttachmentsGroup->addChild(new VerifyPartialAttachmentsTest(m_context, typeFormat));
		verifyMixedAttachmentsGroup->addChild(new VerifyMixedAttachmentsTest(m_context, typeFormat));
		verifyParametersGroup->addChild(new VerifyParametersTest(m_context, typeFormat));
		renderbuffersGroup->addChild(new RenderbuffersTest(m_context, typeFormat));
		clearBufferGroup->addChild(new ClearBufferTest(m_context, typeFormat));
		blitGroup->addChild(new BlitTest(m_context, typeFormat));
		stencilTexturingGroup->addChild(new StencilTexturingTest(m_context, typeFormat));

		if (isContextCoreGL)
		{
			verifyGetTexImageGroup->addChild(new VerifyGetTexImageTest(m_context, typeFormat));
			verifyCopyTexImageGroup->addChild(new VerifyCopyTexImageTest(m_context, typeFormat));
			stencilSizeGroup->addChild(new StencilSizeTest(m_context, typeFormat));
		}
	}

	addChild(validateErrorsGroup);
	addChild(verifyReadPixelsGroup);
	addChild(verifyGetTexImageGroup);
	addChild(verifyCopyTexImageGroup);
	addChild(verifyPartialAttachmentsGroup);
	addChild(verifyMixedAttachmentsGroup);
	addChild(verifyParametersGroup);
	addChild(renderbuffersGroup);
	addChild(clearBufferGroup);
	addChild(blitGroup);
	addChild(stencilTexturingGroup);
	addChild(stencilSizeGroup);
}

} /* glcts namespace */
