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
 * \brief EXT Shader Framebuffer Fetch Tests.
 *//*--------------------------------------------------------------------*/

#include "es31fShaderFramebufferFetchTests.hpp"
#include "es31fFboTestUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuVectorUtil.hpp"

#include "gluShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "deStringUtil.hpp"

#include <vector>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using std::vector;
using std::string;
using tcu::TestLog;

using namespace glw;
using namespace FboTestUtil;

static void checkExtensionSupport (Context& context, const char* extName)
{
	if (!context.getContextInfo().isExtensionSupported(extName))
		throw tcu::NotSupportedError(string(extName) + " not supported");
}

static void checkFramebufferFetchSupport (Context& context)
{
	checkExtensionSupport(context, "GL_EXT_shader_framebuffer_fetch");
}

static bool isRequiredFormat (deUint32 format, glu::RenderContext& renderContext)
{
	const bool isES32 = glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	switch (format)
	{
		// Color-renderable formats
		case GL_RGBA32I:
		case GL_RGBA32UI:
		case GL_RGBA16I:
		case GL_RGBA16UI:
		case GL_RGBA8:
		case GL_RGBA8I:
		case GL_RGBA8UI:
		case GL_SRGB8_ALPHA8:
		case GL_RGB10_A2:
		case GL_RGB10_A2UI:
		case GL_RGBA4:
		case GL_RGB5_A1:
		case GL_RGB8:
		case GL_RGB565:
		case GL_RG32I:
		case GL_RG32UI:
		case GL_RG16I:
		case GL_RG16UI:
		case GL_RG8:
		case GL_RG8I:
		case GL_RG8UI:
		case GL_R32I:
		case GL_R32UI:
		case GL_R16I:
		case GL_R16UI:
		case GL_R8:
		case GL_R8I:
		case GL_R8UI:
			return true;

		// Float format
		case GL_RGBA32F:
		case GL_RGB32F:
		case GL_R11F_G11F_B10F:
		case GL_RG32F:
		case GL_R32F:
			return isES32;

		default:
			return false;
	}
}

tcu::TextureFormat getReadPixelFormat (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32);

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT32);

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);

		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT);

		default:
			DE_ASSERT(false);
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);
	}
}

tcu::Vec4 getFixedPointFormatThreshold (const tcu::TextureFormat& sourceFormat, const tcu::TextureFormat& readPixelsFormat)
{
	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_FLOATING_POINT);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_FLOATING_POINT);

	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER);

	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER);

	const tcu::IVec4	srcBits		= tcu::getTextureFormatBitDepth(sourceFormat);
	const tcu::IVec4	readBits	= tcu::getTextureFormatBitDepth(readPixelsFormat);

	return tcu::Vec4(3.0f) / ((tcu::Vector<deUint64, 4>(1) << (tcu::min(srcBits, readBits).cast<deUint64>())) - tcu::Vector<deUint64, 4>(1)).cast<float>();
}

tcu::UVec4 getFloatULPThreshold (const tcu::TextureFormat& sourceFormat, const tcu::TextureFormat& readPixelsFormat)
{
	const tcu::IVec4	srcMantissaBits		= tcu::getTextureFormatMantissaBitDepth(sourceFormat);
	const tcu::IVec4	readMantissaBits	= tcu::getTextureFormatMantissaBitDepth(readPixelsFormat);
	tcu::IVec4			ULPDiff(0);

	for (int i = 0; i < 4; i++)
		if (readMantissaBits[i] >= srcMantissaBits[i])
			ULPDiff[i] = readMantissaBits[i] - srcMantissaBits[i];

	return tcu::UVec4(4) * (tcu::UVec4(1) << (ULPDiff.cast<deUint32>()));
}

static bool isAnyExtensionSupported (Context& context, const std::vector<std::string>& requiredExts)
{
	for (std::vector<std::string>::const_iterator iter = requiredExts.begin(); iter != requiredExts.end(); iter++)
	{
		const std::string& extension = *iter;

		if (context.getContextInfo().isExtensionSupported(extension.c_str()))
			return true;
	}

	return false;
}

static std::string getColorOutputType(tcu::TextureFormat format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:		return "uvec4";
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:		return "ivec4";
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:		return "vec4";
		default:
			DE_FATAL("Unsupported TEXTURECHANNELCLASS");
			return "";
	}
}

static std::vector<std::string> getEnablingExtensions (deUint32 format, glu::RenderContext& renderContext)
{
	const bool					isES32 = glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	std::vector<std::string>	out;

	DE_ASSERT(!isRequiredFormat(format, renderContext));

	switch (format)
	{
		case GL_RGB16F:
			out.push_back("GL_EXT_color_buffer_half_float");
			break;

		case GL_RGBA16F:
		case GL_RG16F:
		case GL_R16F:
			out.push_back("GL_EXT_color_buffer_half_float");

		case GL_RGBA32F:
		case GL_RGB32F:
		case GL_R11F_G11F_B10F:
		case GL_RG32F:
		case GL_R32F:
			if (!isES32)
				out.push_back("GL_EXT_color_buffer_float");
			break;

		default:
			break;
	}

	return out;
}

void checkFormatSupport (Context& context, deUint32 sizedFormat)
{
	const bool						isCoreFormat	= isRequiredFormat(sizedFormat, context.getRenderContext());
	const std::vector<std::string>	requiredExts	= (!isCoreFormat) ? getEnablingExtensions(sizedFormat, context.getRenderContext()) : std::vector<std::string>();

	// Check that we don't try to use invalid formats.
	DE_ASSERT(isCoreFormat || !requiredExts.empty());

	if (!requiredExts.empty() && !isAnyExtensionSupported(context, requiredExts))
		throw tcu::NotSupportedError("Format not supported");
}

tcu::Vec4 scaleColorValue (tcu::TextureFormat format, const tcu::Vec4& color)
{
	const tcu::TextureFormatInfo	fmtInfo			= tcu::getTextureFormatInfo(format);
	const tcu::Vec4					cScale			= fmtInfo.valueMax-fmtInfo.valueMin;
	const tcu::Vec4					cBias			= fmtInfo.valueMin;

	return tcu::RGBA(color).toVec() * cScale + cBias;
}

// Base class for framebuffer fetch test cases

class FramebufferFetchTestCase : public TestCase
{
public:
									FramebufferFetchTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
									~FramebufferFetchTestCase		(void);

	void							init							(void);
	void							deinit							(void);

protected:
	string							genPassThroughVertSource		(void);
	virtual glu::ProgramSources		genShaderSources				(void);

	void							genFramebufferWithTexture		(const tcu::Vec4& color);
	void							genAttachementTexture			(const tcu::Vec4& color);
	void							genUniformColor					(const tcu::Vec4& color);

	void							render							(void);
	void							verifyRenderbuffer				(TestLog& log, const tcu::TextureFormat& format, const tcu::TextureLevel& reference, const tcu::TextureLevel& result);

	const glw::Functions&			m_gl;
	const deUint32					m_format;

	glu::ShaderProgram*				m_program;
	GLuint							m_framebuffer;
	GLuint							m_texColorBuffer;

	tcu::TextureFormat				m_texFmt;
	glu::TransferFormat				m_transferFmt;
	bool							m_isFilterable;

	enum
	{
		VIEWPORT_WIDTH	= 64,
		VIEWPORT_HEIGHT = 64,
	};
};

FramebufferFetchTestCase::FramebufferFetchTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: TestCase (context, name, desc)
	, m_gl					(m_context.getRenderContext().getFunctions())
	, m_format				(format)
	, m_program				(DE_NULL)
	, m_framebuffer			(0)
	, m_texColorBuffer		(0)
	, m_texFmt				(glu::mapGLInternalFormat(m_format))
	, m_transferFmt			(glu::getTransferFormat(m_texFmt))
	, m_isFilterable		(glu::isGLInternalColorFormatFilterable(m_format))
{
}

FramebufferFetchTestCase::~FramebufferFetchTestCase (void)
{
	FramebufferFetchTestCase::deinit();
}

void FramebufferFetchTestCase::init (void)
{
	checkFramebufferFetchSupport (m_context);
	checkFormatSupport(m_context, m_format);

	DE_ASSERT(!m_program);
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), genShaderSources());

	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		TCU_FAIL("Failed to compile shader program");
	}

	m_gl.useProgram(m_program->getProgram());
}

void FramebufferFetchTestCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;

	if (m_framebuffer)
	{
		m_gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		m_gl.deleteFramebuffers(1, &m_framebuffer);
		m_framebuffer = 0;
	}

	if (m_texColorBuffer)
	{
		m_gl.deleteTextures(1, &m_texColorBuffer);
		m_texColorBuffer = 0;
	}
}

string FramebufferFetchTestCase::genPassThroughVertSource (void)
{
	std::ostringstream vertShaderSource;

	vertShaderSource	<< "#version 310 es\n"
						<< "in highp vec4 a_position;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	gl_Position = a_position;\n"
						<< "}\n";

	return vertShaderSource.str();
}

glu::ProgramSources FramebufferFetchTestCase::genShaderSources (void)
{
	const string		vecType	= getColorOutputType(m_texFmt);
	std::ostringstream	fragShaderSource;

	fragShaderSource	<< "#version 310 es\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "layout(location = 0) inout highp " << vecType << " o_color;\n"
						<< "uniform highp " << vecType << " u_color;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	o_color += u_color;\n"
						<< "}\n";

	return glu::makeVtxFragSources(genPassThroughVertSource(), fragShaderSource.str());
}

void FramebufferFetchTestCase::genFramebufferWithTexture (const tcu::Vec4& color)
{
	m_gl.genFramebuffers(1, &m_framebuffer);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	genAttachementTexture(color);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "genAttachementTexture()");

	m_gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texColorBuffer, 0);
	TCU_CHECK(m_gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

void FramebufferFetchTestCase::genAttachementTexture (const tcu::Vec4& color)
{
	tcu::TextureLevel			data					(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::TextureChannelClass	textureChannelClass =	tcu::getTextureChannelClass(m_texFmt.type);

	m_gl.genTextures(1, &m_texColorBuffer);
	m_gl.bindTexture(GL_TEXTURE_2D, m_texColorBuffer);

	m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,		GL_CLAMP_TO_EDGE);
	m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,		GL_CLAMP_TO_EDGE);
	m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_R,		GL_CLAMP_TO_EDGE);
	m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER,	m_isFilterable ? GL_LINEAR : GL_NEAREST);
	m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER,	m_isFilterable ? GL_LINEAR : GL_NEAREST);

	if (textureChannelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
		tcu::clear(data.getAccess(), color.asUint());
	else if (textureChannelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
		tcu::clear(data.getAccess(), color.asInt());
	else
		tcu::clear(data.getAccess(), color);

	m_gl.texImage2D(GL_TEXTURE_2D, 0, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, m_transferFmt.format, m_transferFmt.dataType, data.getAccess().getDataPtr());
	m_gl.bindTexture(GL_TEXTURE_2D, 0);
}

void FramebufferFetchTestCase::verifyRenderbuffer (TestLog&	log, const tcu::TextureFormat& format, const tcu::TextureLevel&	reference, const tcu::TextureLevel&	result)
{
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		{
			const string		name		= "Renderbuffer";
			const string		desc		= "Compare renderbuffer (floating_point)";
			const tcu::UVec4	threshold	= getFloatULPThreshold(format, result.getFormat());

			if (!tcu::floatUlpThresholdCompare(log, name.c_str(), desc.c_str(), reference, result, threshold, tcu::COMPARE_LOG_RESULT))
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
		{
			const string		name		= "Renderbuffer";
			const string		desc		= "Compare renderbuffer (integer)";
			const tcu::UVec4	threshold	(1, 1, 1, 1);

			if (!tcu::intThresholdCompare(log, name.c_str(), desc.c_str(), reference, result, threshold, tcu::COMPARE_LOG_RESULT))
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		{
			const string		name		= "Renderbuffer";
			const string		desc		= "Compare renderbuffer (fixed point)";
			const tcu::Vec4		threshold	= getFixedPointFormatThreshold(format, result.getFormat());

			if (!tcu::floatThresholdCompare(log, name.c_str(), desc.c_str(), reference, result, threshold, tcu::COMPARE_LOG_RESULT))
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

			break;
		}

		default:
		{
			DE_ASSERT(DE_FALSE);
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}
}

void FramebufferFetchTestCase::genUniformColor (const tcu::Vec4& color)
{
	const GLuint colorLocation	= m_gl.getUniformLocation(m_program->getProgram(), "u_color");

	switch (tcu::getTextureChannelClass(m_texFmt.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
		{
			m_gl.uniform4uiv(colorLocation, 1, color.asUint().getPtr());
			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
		{
			m_gl.uniform4iv(colorLocation, 1, color.asInt().getPtr());
			break;
		}
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		{
			m_gl.uniform4fv(colorLocation, 1, color.asFloat().getPtr());
			break;
		}
		default:
			DE_ASSERT(DE_FALSE);
	}

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "genUniformColor()");
}

void FramebufferFetchTestCase::render (void)
{
	const GLfloat coords[] =
	{
		-1.0f, -1.0f,
		+1.0f, -1.0f,
		+1.0f, +1.0f,
		-1.0f, +1.0f,
	};

	const GLushort indices[] =
	{
		0, 1, 2, 2, 3, 0,
	};

	const GLuint	coordLocation	= m_gl.getAttribLocation(m_program->getProgram(), "a_position");

	m_gl.viewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	glu::Buffer coordinatesBuffer(m_context.getRenderContext());
	glu::Buffer elementsBuffer(m_context.getRenderContext());

	m_gl.bindBuffer(GL_ARRAY_BUFFER, *coordinatesBuffer);
	m_gl.bufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(coords), coords, GL_STATIC_DRAW);
	m_gl.enableVertexAttribArray(coordLocation);
	m_gl.vertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, DE_NULL);

	m_gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, *elementsBuffer);
	m_gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sizeof(indices), &indices[0], GL_STATIC_DRAW);

	m_gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, DE_NULL);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "render()");
}

// Test description:
// - Attach texture containing solid color to framebuffer.
// - Draw full quad covering the entire viewport.
// - Sum framebuffer read color with passed in uniform color.
// - Compare resulting surface with reference.

class TextureFormatTestCase : public FramebufferFetchTestCase
{
public:
						TextureFormatTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~TextureFormatTestCase		(void) {};

	IterateResult		iterate						(void);

private:
	tcu::TextureLevel	genReferenceTexture			(const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor);
};

TextureFormatTestCase::TextureFormatTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

tcu::TextureLevel TextureFormatTestCase::genReferenceTexture (const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel			reference			(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::TextureChannelClass	textureChannelClass = tcu::getTextureChannelClass(m_texFmt.type);

	if (textureChannelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
	{
		tcu::clear(reference.getAccess(), fbColor.asUint() + uniformColor.asUint());
	}
	else if (textureChannelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
	{
		tcu::clear(reference.getAccess(), fbColor.asInt() + uniformColor.asInt());
	}
	else
	{
		if (tcu::isSRGB(m_texFmt))
		{
			const tcu::Vec4	fragmentColor = tcu::sRGBToLinear(fbColor) + uniformColor;
			tcu::clear(reference.getAccess(), tcu::linearToSRGB(fragmentColor));
		}
		else
		{
			tcu::clear(reference.getAccess(), fbColor + uniformColor);
		}
	}

	return reference;
}

TextureFormatTestCase::IterateResult TextureFormatTestCase::iterate (void)
{
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
	const tcu::Vec4		fbColor			= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f));

	tcu::TextureLevel	reference		= genReferenceTexture(fbColor, uniformColor);
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	genFramebufferWithTexture(fbColor);
	genUniformColor(uniformColor);
	render();

	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

	return STOP;
}

// Test description:
// - Attach multiple textures containing solid colors to framebuffer.
// - Draw full quad covering the entire viewport.
// - For each render target sum framebuffer read color with passed in uniform color.
// - Compare resulting surfaces with references.

class MultipleRenderTargetsTestCase : public FramebufferFetchTestCase
{
public:
						MultipleRenderTargetsTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~MultipleRenderTargetsTestCase		(void);

	IterateResult		iterate								(void);
	void				deinit								(void);

private:
	void				genFramebufferWithTextures			(const vector<tcu::Vec4>& colors);
	void				genAttachmentTextures				(const vector<tcu::Vec4>& colors);
	tcu::TextureLevel	genReferenceTexture					(const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor);
	glu::ProgramSources genShaderSources					(void);

	enum
	{
		MAX_COLOR_BUFFERS = 4
	};

	GLuint				m_texColorBuffers					[MAX_COLOR_BUFFERS];
	GLenum				m_colorBuffers						[MAX_COLOR_BUFFERS];
};

MultipleRenderTargetsTestCase::MultipleRenderTargetsTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
	, m_texColorBuffers ()
{
	m_colorBuffers[0] = GL_COLOR_ATTACHMENT0;
	m_colorBuffers[1] = GL_COLOR_ATTACHMENT1;
	m_colorBuffers[2] = GL_COLOR_ATTACHMENT2;
	m_colorBuffers[3] = GL_COLOR_ATTACHMENT3;
}

MultipleRenderTargetsTestCase::~MultipleRenderTargetsTestCase (void)
{
	MultipleRenderTargetsTestCase::deinit();
}

void MultipleRenderTargetsTestCase::deinit (void)
{
	// Clean up texture data
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(m_texColorBuffers); ++i)
	{
		if (m_texColorBuffers[i])
			m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texColorBuffers[i]);
	}

	FramebufferFetchTestCase::deinit();
}

void MultipleRenderTargetsTestCase::genFramebufferWithTextures (const vector<tcu::Vec4>& colors)
{
	m_gl.genFramebuffers(1, &m_framebuffer);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	genAttachmentTextures(colors);

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(m_texColorBuffers); ++i)
		m_gl.framebufferTexture2D(GL_FRAMEBUFFER, m_colorBuffers[i], GL_TEXTURE_2D, m_texColorBuffers[i], 0);

	TCU_CHECK(m_gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	m_gl.drawBuffers((glw::GLsizei)MAX_COLOR_BUFFERS, &m_colorBuffers[0]);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "genFramebufferWithTextures()");
}

void MultipleRenderTargetsTestCase::genAttachmentTextures (const vector<tcu::Vec4>& colors)
{
	tcu::TextureLevel	data	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);

	m_gl.genTextures(MAX_COLOR_BUFFERS, m_texColorBuffers);

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(m_texColorBuffers); ++i)
	{
		m_gl.bindTexture(GL_TEXTURE_2D, m_texColorBuffers[i]);

		m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,		GL_CLAMP_TO_EDGE);
		m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,		GL_CLAMP_TO_EDGE);
		m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_R,		GL_CLAMP_TO_EDGE);
		m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER,	m_isFilterable ? GL_LINEAR : GL_NEAREST);
		m_gl.texParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER,	m_isFilterable ? GL_LINEAR : GL_NEAREST);

		clear(data.getAccess(), colors[i]);
		m_gl.texImage2D(GL_TEXTURE_2D, 0, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, m_transferFmt.format, m_transferFmt.dataType, data.getAccess().getDataPtr());
	}

	m_gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "genAttachmentTextures()");
}

tcu::TextureLevel MultipleRenderTargetsTestCase::genReferenceTexture (const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::clear(reference.getAccess(), fbColor + uniformColor);

	return reference;
}

glu::ProgramSources MultipleRenderTargetsTestCase::genShaderSources (void)
{
	const string		vecType	= getColorOutputType(m_texFmt);
	std::ostringstream	fragShaderSource;

	fragShaderSource	<< "#version 310 es\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "layout(location = 0) inout highp " << vecType << " o_color0;\n"
						<< "layout(location = 1) inout highp " << vecType << " o_color1;\n"
						<< "layout(location = 2) inout highp " << vecType << " o_color2;\n"
						<< "layout(location = 3) inout highp " << vecType << " o_color3;\n"
						<< "uniform highp " << vecType << " u_color;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	o_color0 += u_color;\n"
						<< "	o_color1 += u_color;\n"
						<< "	o_color2 += u_color;\n"
						<< "	o_color3 += u_color;\n"
						<< "}\n";

	return glu::makeVtxFragSources(genPassThroughVertSource(), fragShaderSource.str());
}

MultipleRenderTargetsTestCase::IterateResult MultipleRenderTargetsTestCase::iterate (void)
{
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	vector<tcu::Vec4> colors;
	colors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.9f, 0.0f, 0.0f, 1.0f)));
	colors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 0.9f, 0.0f, 1.0f)));
	colors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 0.0f, 0.9f, 1.0f)));
	colors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 0.9f, 0.9f, 1.0f)));

	genFramebufferWithTextures(colors);
	genUniformColor(uniformColor);
	render();

	for (int i = 0; i < DE_LENGTH_OF_ARRAY(m_colorBuffers); ++i)
	{
		tcu::TextureLevel	reference		= genReferenceTexture(colors[i], uniformColor);

		m_gl.readBuffer(m_colorBuffers[i]);
		glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
		verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);
	}

	return STOP;
}

// Test description:
// - Same as TextureFormatTestCase except uses built-in fragment output of ES 2.0

class LastFragDataTestCase : public FramebufferFetchTestCase
{
public:
						LastFragDataTestCase			(Context& context, const char* name, const char* desc, deUint32 format);
						~LastFragDataTestCase			(void) {};

	IterateResult		iterate							(void);

private:
	glu::ProgramSources genShaderSources				(void);
	tcu::TextureLevel	genReferenceTexture				(const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor);
};

LastFragDataTestCase::LastFragDataTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

glu::ProgramSources LastFragDataTestCase::genShaderSources (void)
{
	const string		vecType	= getColorOutputType(m_texFmt);
	std::ostringstream	vertShaderSource;
	std::ostringstream	fragShaderSource;

	vertShaderSource	<< "#version 100\n"
						<< "attribute vec4 a_position;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	gl_Position = a_position;\n"
						<< "}\n";

	fragShaderSource	<< "#version 100\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "uniform highp " << vecType << " u_color;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	gl_FragColor = u_color + gl_LastFragData[0];\n"
						<< "}\n";

	return glu::makeVtxFragSources(vertShaderSource.str(), fragShaderSource.str());
}

tcu::TextureLevel LastFragDataTestCase::genReferenceTexture (const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::clear(reference.getAccess(), fbColor + uniformColor);

	return reference;
}

LastFragDataTestCase::IterateResult LastFragDataTestCase::iterate (void)
{
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
	const tcu::Vec4		fbColor			= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f));

	tcu::TextureLevel	reference		= genReferenceTexture(fbColor, uniformColor);
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	genFramebufferWithTexture(fbColor);
	genUniformColor(uniformColor);
	render();

	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

	return STOP;
}

// Test description:
// - Attach texture containing solid color to framebuffer.
// - Create one 2D texture for sampler with a grid pattern
// - Draw full screen quad covering the entire viewport.
// - Sum color values taken from framebuffer texture and sampled texture
// - Compare resulting surface with reference.

class TexelFetchTestCase : public FramebufferFetchTestCase
{
public:
						TexelFetchTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~TexelFetchTestCase		(void) {}

	IterateResult		iterate					(void);

private:
	glu::ProgramSources genShaderSources		(void);
	tcu::TextureLevel	genReferenceTexture		(const tcu::Vec4& colorEven, const tcu::Vec4& colorOdd, const tcu::Vec4& fbColor);
	void				genSamplerTexture		(const tcu::Vec4& colorEven, const tcu::Vec4& colorOdd);

	GLuint				m_samplerTexture;
};

TexelFetchTestCase::TexelFetchTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
	, m_samplerTexture(0)
{
}

void TexelFetchTestCase::genSamplerTexture (const tcu::Vec4& colorEven, const tcu::Vec4& colorOdd)
{
	tcu::TextureLevel	data	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);

	m_gl.activeTexture(GL_TEXTURE1);

	m_gl.genTextures(1, &m_samplerTexture);
	m_gl.bindTexture(GL_TEXTURE_2D, m_texColorBuffer);
	m_gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	m_gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	tcu::fillWithGrid(data.getAccess(), 8, colorEven, colorOdd);

	m_gl.texImage2D(GL_TEXTURE_2D, 0, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, m_transferFmt.format, m_transferFmt.dataType, data.getAccess().getDataPtr());
	m_gl.bindTexture(GL_TEXTURE_2D, 0);

	const GLuint samplerLocation = m_gl.getUniformLocation(m_program->getProgram(), "u_sampler");
	m_gl.uniform1i(samplerLocation, 1);

	GLU_EXPECT_NO_ERROR(m_gl.getError(), "genSamplerTexture()");
}

glu::ProgramSources TexelFetchTestCase::genShaderSources (void)
{
	const string		vecType	= getColorOutputType(m_texFmt);
	std::ostringstream	fragShaderSource;

	fragShaderSource	<< "#version 310 es\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "layout(location = 0) inout highp " << vecType << " o_color;\n"
						<< "\n"
						<< "uniform sampler2D u_sampler;\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	o_color += texelFetch(u_sampler, ivec2(gl_FragCoord), 0);\n"
						<< "}\n";

	return glu::makeVtxFragSources(genPassThroughVertSource(), fragShaderSource.str());
}

tcu::TextureLevel TexelFetchTestCase::genReferenceTexture (const tcu::Vec4& colorEven, const tcu::Vec4& colorOdd, const tcu::Vec4& fbColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::fillWithGrid(reference.getAccess(), 8, colorEven + fbColor, colorOdd + fbColor);

	return reference;
}

TexelFetchTestCase::IterateResult TexelFetchTestCase::iterate (void)
{
	const tcu::Vec4		fbColor			= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 0.5f, 1.0f));
	const tcu::Vec4		colorEven		= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.5f, 0.0f, 1.0f));
	const tcu::Vec4		colorOdd		= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 0.5f, 1.0f));

	genSamplerTexture(colorEven, colorOdd);
	tcu::TextureLevel	reference		= genReferenceTexture(colorEven, colorOdd, fbColor);
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	genFramebufferWithTexture(fbColor);
	render();

	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

	// cleanup
	m_gl.deleteTextures(1, &m_samplerTexture);

	return STOP;
}

// Test description:
// - Attach texture containing solid color to framebuffer.
// - Draw full screen quad covering the entire viewport.
// - Multiple assignments are made to the output color for fragments on the right vertical half of the screen.
// - A single assignment is made to the output color for fragments on the left vertical centre of the screen.
// - Values are calculated using the sum of the passed in uniform color and the previous framebuffer color.
// - Compare resulting surface with reference.

class MultipleAssignmentTestCase : public FramebufferFetchTestCase
{
public:
						MultipleAssignmentTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~MultipleAssignmentTestCase		(void) {}

	IterateResult		iterate							(void);

private:
	glu::ProgramSources genShaderSources				(void);
	tcu::TextureLevel	genReferenceTexture				(const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor);
};

MultipleAssignmentTestCase::MultipleAssignmentTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

glu::ProgramSources MultipleAssignmentTestCase::genShaderSources (void)
{
	const string		vecType = getColorOutputType(m_texFmt);
	std::ostringstream	vertShaderSource;
	std::ostringstream	fragShaderSource;

	vertShaderSource	<< "#version 310 es\n"
						<< "in highp vec4 a_position;\n"
						<< "out highp vec4 v_position;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	gl_Position = a_position;\n"
						<< "	v_position  = gl_Position;\n"
						<< "}\n";

	fragShaderSource	<< "#version 310 es\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "in highp vec4 v_position;\n"
						<< "layout(location = 0) inout highp " << vecType << " o_color;\n"
						<< "uniform highp " << vecType << " u_color;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	if (v_position.x > 0.0f)\n"
						<< "		o_color += u_color;\n"
						<< "\n"
						<< "	o_color += u_color;\n"
						<< "}\n";

	return glu::makeVtxFragSources(vertShaderSource.str(), fragShaderSource.str());
}

tcu::TextureLevel MultipleAssignmentTestCase::genReferenceTexture (const tcu::Vec4& fbColor, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);

	int	width	= reference.getAccess().getWidth();
	int	height	= reference.getAccess().getHeight();
	int	left	= width /2;
	int	top		= height/2;

	tcu::Vec4 compositeColor(uniformColor * 2.0f);

	tcu::clear(getSubregion(reference.getAccess(), left,		0,		0, width-left,	top,		1),	fbColor + compositeColor);
	tcu::clear(getSubregion(reference.getAccess(), 0,			top,	0, left,		height-top,	1), fbColor + uniformColor);
	tcu::clear(getSubregion(reference.getAccess(), left,		top,	0, width-left,	height-top, 1), fbColor + compositeColor);
	tcu::clear(getSubregion(reference.getAccess(), 0,			0,		0, left,		top,		1),	fbColor + uniformColor);

	return reference;
}

MultipleAssignmentTestCase::IterateResult MultipleAssignmentTestCase::iterate (void)
{
	const tcu::Vec4		fbColor			= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f));
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.25f, 0.0f, 0.0f, 1.0f));

	tcu::TextureLevel	reference		= genReferenceTexture(fbColor, uniformColor);
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	genFramebufferWithTexture(fbColor);
	genUniformColor(uniformColor);
	render();

	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

	return STOP;
}

// Test description:
// - Attach texture containing grid pattern to framebuffer.
// - Using framebuffer reads discard odd squares in the grid.
// - The even squares framebuffer color is added to the passed in uniform color.

class FragmentDiscardTestCase : public FramebufferFetchTestCase
{
public:
						FragmentDiscardTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~FragmentDiscardTestCase	(void) {}

	IterateResult		iterate						(void);

private:
	glu::ProgramSources genShaderSources			(void);
	void				genFramebufferWithGrid		(const tcu::Vec4& fbColorEven, const tcu::Vec4& fbColorOdd);
	tcu::TextureLevel	genReferenceTexture			(const tcu::Vec4& fbColorEven, const tcu::Vec4& fbColorOdd);
};

FragmentDiscardTestCase::FragmentDiscardTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

glu::ProgramSources FragmentDiscardTestCase::genShaderSources (void)
{
	const string		vecType	= getColorOutputType(m_texFmt);
	std::ostringstream	fragShaderSource;

	fragShaderSource	<< "#version 310 es\n"
						<< "#extension GL_EXT_shader_framebuffer_fetch : require\n"
						<< "layout(location = 0) inout highp " << vecType << " o_color;\n"
						<< "uniform highp " << vecType << " u_color;\n"
						<< "\n"
						<< "void main (void)\n"
						<< "{\n"
						<< "	const highp float threshold = 0.0005f;\n"
						<< "	bool valuesEqual = all(lessThan(abs(o_color - u_color), vec4(threshold)));\n\n"
						<< "	if (valuesEqual)\n"
						<< "		o_color += u_color;\n"
						<< "	else\n"
						<< "		discard;\n"
						<< "}\n";

	return glu::makeVtxFragSources(genPassThroughVertSource(), fragShaderSource.str());
}

void FragmentDiscardTestCase::genFramebufferWithGrid (const tcu::Vec4& fbColorEven, const tcu::Vec4& fbColorOdd)
{
	tcu::TextureLevel data	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);

	m_gl.genFramebuffers(1, &m_framebuffer);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	m_gl.genTextures(1, &m_texColorBuffer);
	m_gl.bindTexture(GL_TEXTURE_2D, m_texColorBuffer);

	tcu::fillWithGrid(data.getAccess(), 8, fbColorEven, fbColorOdd);

	m_gl.texImage2D(GL_TEXTURE_2D, 0, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0, m_transferFmt.format, m_transferFmt.dataType, data.getAccess().getDataPtr());
	m_gl.bindTexture(GL_TEXTURE_2D, 0);

	m_gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texColorBuffer, 0);
	TCU_CHECK(m_gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

tcu::TextureLevel FragmentDiscardTestCase::genReferenceTexture (const tcu::Vec4& fbColorEven, const tcu::Vec4& fbColorOdd)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	tcu::fillWithGrid(reference.getAccess(), 8, fbColorEven + fbColorEven, fbColorOdd);

	return reference;
}

FragmentDiscardTestCase::IterateResult FragmentDiscardTestCase::iterate (void)
{
	const tcu::Vec4		fbColorEven		= scaleColorValue(m_texFmt, tcu::Vec4(0.5f, 0.0f, 1.0f, 1.0f));
	const tcu::Vec4		fbColorOdd		= scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 1.0f, 1.0f, 1.0f));

	tcu::TextureLevel	reference		= genReferenceTexture(fbColorEven, fbColorOdd);
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	genFramebufferWithGrid(fbColorEven, fbColorOdd);

	genUniformColor(fbColorEven);
	render();

	glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
	verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

	return STOP;
}

// Test description:
// - Create 2D texture array containing three mipmaps.
// - Each mipmap level is assigned a different color.
// - Attach single mipmap level to framebuffer and draw full screen quad.
// - Sum framebuffer read color with passed in uniform color.
// - Compare resulting surface with reference.
// - Repeat for subsequent mipmap levels.

class TextureLevelTestCase : public FramebufferFetchTestCase
{
public:
						TextureLevelTestCase			(Context& context, const char* name, const char* desc, deUint32 format);
						~TextureLevelTestCase			(void) {}

	IterateResult		iterate							(void);

private:
	void				create2DTextureArrayMipMaps		(const vector<tcu::Vec4>& colors);
	tcu::TextureLevel	genReferenceTexture				(int level, const vector<tcu::Vec4>& colors, const tcu::Vec4& uniformColor);
	void				genReferenceMipmap				(const tcu::Vec4& color, tcu::TextureLevel& reference);
};

TextureLevelTestCase::TextureLevelTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

void TextureLevelTestCase::create2DTextureArrayMipMaps (const vector<tcu::Vec4>& colors)
{
	int						numLevels	= (int)colors.size();
	tcu::TextureLevel		levelData	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType));

	m_gl.genTextures(1, &m_texColorBuffer);
	m_gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texColorBuffer);

	m_gl.texImage3D(GL_TEXTURE_2D_ARRAY, 0, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1, 0, m_transferFmt.format, m_transferFmt.dataType, DE_NULL);
	m_gl.generateMipmap(GL_TEXTURE_2D_ARRAY);

	for (int level = 0; level < numLevels; level++)
	{
		int		levelW		= de::max(1, VIEWPORT_WIDTH		>> level);
		int		levelH		= de::max(1, VIEWPORT_HEIGHT	>> level);

		levelData.setSize(levelW, levelH, 1);

		clear(levelData.getAccess(), colors[level]);
		m_gl.texImage3D(GL_TEXTURE_2D_ARRAY, level, m_format, levelW, levelH, 1, 0, m_transferFmt.format, m_transferFmt.dataType, levelData.getAccess().getDataPtr());
	}

	m_gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "create2DTextureArrayMipMaps()");
}

tcu::TextureLevel TextureLevelTestCase::genReferenceTexture (int level, const vector<tcu::Vec4>& colors, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH >> level, VIEWPORT_HEIGHT >> level, 1);

	genReferenceMipmap(colors[level] + uniformColor, reference);

	return reference;
}

void TextureLevelTestCase::genReferenceMipmap (const tcu::Vec4& color, tcu::TextureLevel& reference)
{
	const int	width	= reference.getAccess().getWidth();
	const int	height	= reference.getAccess().getHeight();
	const int	left	= width  / 2;
	const int	top		= height / 2;

	clear(getSubregion(reference.getAccess(), left,		0,		0, width-left,	top,		1),	color);
	clear(getSubregion(reference.getAccess(), 0,		top,	0, left,		height-top,	1), color);
	clear(getSubregion(reference.getAccess(), left,		top,	0, width-left,	height-top, 1), color);
	clear(getSubregion(reference.getAccess(), 0,		0,		0, left,		top,		1),	color);
}

TextureLevelTestCase::IterateResult TextureLevelTestCase::iterate (void)
{
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.1f, 0.0f, 0.0f, 1.0f));
	vector<tcu::Vec4>	levelColors;

	levelColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.4f, 0.0f, 0.0f, 1.0f)));
	levelColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.2f, 0.0f, 0.0f, 1.0f)));
	levelColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)));

	m_gl.genFramebuffers(1, &m_framebuffer);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	create2DTextureArrayMipMaps(levelColors);

	// attach successive mipmap layers to framebuffer and render
	for (int level = 0; level < (int)levelColors.size(); ++level)
	{
		std::ostringstream name, desc;
		name << "Level "		<< level;
		desc << "Mipmap level " << level;

		const tcu::ScopedLogSection	section			(m_testCtx.getLog(), name.str(), desc.str());
		tcu::TextureLevel			result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH >> level, VIEWPORT_HEIGHT >> level);
		tcu::TextureLevel			reference		= genReferenceTexture(level, levelColors, uniformColor);

		m_gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texColorBuffer, level, 0);

		genUniformColor(uniformColor);
		render();

		glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
		verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

		if (m_testCtx.getTestResult() != QP_TEST_RESULT_PASS)
			return STOP;
	}

	return STOP;
}

class TextureLayerTestCase : public FramebufferFetchTestCase
{
public:
						TextureLayerTestCase		(Context& context, const char* name, const char* desc, deUint32 format);
						~TextureLayerTestCase		(void) {}

	IterateResult		iterate						(void);

private:
	void				create2DTextureArrayLayers	(const vector<tcu::Vec4>&  colors);
	tcu::TextureLevel	genReferenceTexture			(int layer, const vector<tcu::Vec4>& colors, const tcu::Vec4& uniformColor);
};

TextureLayerTestCase::TextureLayerTestCase (Context& context, const char* name, const char* desc, deUint32 format)
	: FramebufferFetchTestCase(context, name, desc, format)
{
}

void TextureLayerTestCase::create2DTextureArrayLayers (const vector<tcu::Vec4>& colors)
{
	int						numLayers	= (int)colors.size();
	tcu::TextureLevel		layerData	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType));

	m_gl.genTextures(1, &m_texColorBuffer);
	m_gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texColorBuffer);
	m_gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, m_format, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, numLayers);
	m_gl.bindImageTexture(0, m_texColorBuffer, 0, GL_FALSE, 0, GL_READ_ONLY, m_format);

	layerData.setSize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, numLayers);

	for (int layer = 0; layer < numLayers; layer++)
	{
		clear(layerData.getAccess(), colors[layer]);
		m_gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1, m_transferFmt.format, m_transferFmt.dataType, layerData.getAccess().getDataPtr());
	}

	m_gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "create2DTextureArrayLayers()");
}

tcu::TextureLevel TextureLayerTestCase::genReferenceTexture (int layer, const vector<tcu::Vec4>& colors, const tcu::Vec4& uniformColor)
{
	tcu::TextureLevel	reference	(glu::mapGLTransferFormat(m_transferFmt.format, m_transferFmt.dataType), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 1);
	clear(reference.getAccess(), colors[layer] + uniformColor);

	return reference;
}

// Test description
// - Create 2D texture array containing three layers.
// - Each layer is assigned a different color.
// - Attach single layer to framebuffer and draw full screen quad.
// - Sum framebuffer read color with passed in uniform color.
// - Compare resulting surface with reference.
// - Repeat for subsequent texture layers.

TextureLayerTestCase::IterateResult TextureLayerTestCase::iterate (void)
{
	const tcu::Vec4		uniformColor	= scaleColorValue(m_texFmt, tcu::Vec4(0.1f, 0.1f, 0.1f, 1.0f));
	tcu::TextureLevel	result			(getReadPixelFormat(m_texFmt), VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	vector<tcu::Vec4>	layerColors;

	layerColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.4f, 0.0f, 0.0f, 1.0f)));
	layerColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.2f, 0.0f, 0.0f, 1.0f)));
	layerColors.push_back(scaleColorValue(m_texFmt, tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)));

	m_gl.genFramebuffers(1, &m_framebuffer);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

	create2DTextureArrayLayers(layerColors);

	for (int layer = 0; layer < (int)layerColors.size(); ++layer)
	{
		std::ostringstream name, desc;
		name << "Layer " << layer;
		desc << "Layer " << layer;

		const tcu::ScopedLogSection section		(m_testCtx.getLog(), name.str(), desc.str());
		tcu::TextureLevel			reference	= genReferenceTexture(layer, layerColors, uniformColor);

		m_gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texColorBuffer, 0, layer);

		genUniformColor(uniformColor);
		render();

		glu::readPixels(m_context.getRenderContext(), 0, 0, result.getAccess());
		verifyRenderbuffer(m_testCtx.getLog(), m_texFmt, reference, result);

		if (m_testCtx.getTestResult() != QP_TEST_RESULT_PASS)
			return STOP;
	}

	return STOP;
}

} // Anonymous

ShaderFramebufferFetchTests::ShaderFramebufferFetchTests (Context& context)
	: TestCaseGroup (context, "framebuffer_fetch", "GL_EXT_shader_framebuffer_fetch tests")
{
}

ShaderFramebufferFetchTests::~ShaderFramebufferFetchTests (void)
{
}

void ShaderFramebufferFetchTests::init (void)
{
	tcu::TestCaseGroup* const basicTestGroup				= new tcu::TestCaseGroup(m_testCtx, "basic",				"Basic framebuffer shader fetch tests");
	tcu::TestCaseGroup* const framebufferFormatTestGroup	= new tcu::TestCaseGroup(m_testCtx, "framebuffer_format",	"Texture render target formats tests");

	// basic
	{
		basicTestGroup->addChild(new TexelFetchTestCase				(m_context,		"texel_fetch",					"Framebuffer fetches in conjunction with shader texel fetches",			GL_RGBA8));
		basicTestGroup->addChild(new LastFragDataTestCase			(m_context,		"last_frag_data",				"Framebuffer fetches with built-in fragment output of ES 2.0",			GL_RGBA8));
		basicTestGroup->addChild(new FragmentDiscardTestCase		(m_context,		"fragment_discard",				"Framebuffer fetches in combination with fragment discards",			GL_RGBA8));
		basicTestGroup->addChild(new MultipleAssignmentTestCase		(m_context,		"multiple_assignment",			"Multiple assignments to fragment color inout",							GL_RGBA8));
		basicTestGroup->addChild(new MultipleRenderTargetsTestCase	(m_context,		"multiple_render_targets",		"Framebuffer fetches used in combination with multiple render targets",	GL_RGBA8));
		basicTestGroup->addChild(new TextureLevelTestCase			(m_context,		"framebuffer_texture_level",	"Framebuffer fetches with individual texture render target mipmaps",	GL_RGBA8));
		basicTestGroup->addChild(new TextureLayerTestCase			(m_context,		"framebuffer_texture_layer",	"Framebuffer fetches with individual texture render target layers",		GL_RGBA8));
	}

	// framebuffer formats
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
			GL_RGB10_A2UI, GL_RGBA4, GL_RGB5_A1,

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

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(colorFormats); ndx++)
			framebufferFormatTestGroup->addChild(new TextureFormatTestCase(m_context, getFormatName(colorFormats[ndx]), "Framebuffer fetches from texture attachments with varying formats", colorFormats[ndx]));
	}

	addChild(basicTestGroup);
	addChild(framebufferFormatTestGroup);
}

} // Functional
} // gles31
} // deqp
