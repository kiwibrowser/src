/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file  esextcDrawBuffersIndexedColorMasks.hpp
 * \brief Draw Buffers Indexed tests 4. Color masks
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedColorMasks.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "tcuTestLog.hpp"
#include <cmath>

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
DrawBuffersIndexedColorMasks::DrawBuffersIndexedColorMasks(Context& context, const ExtParameters& extParams,
														   const char* name, const char* description)
	: DrawBuffersIndexedBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

void DrawBuffersIndexedColorMasks::prepareFramebuffer()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint maxDrawBuffers = 0;
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	if (maxDrawBuffers < 4)
	{
		throw tcu::ResourceError("Minimum number of draw buffers too low");
	}

	gl.genFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	std::vector<glw::GLenum> bufs(maxDrawBuffers);
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		bufs[i] = GL_COLOR_ATTACHMENT0 + i;
	}
	gl.drawBuffers(maxDrawBuffers, &bufs[0]);

	gl.disable(GL_DITHER);
}

void DrawBuffersIndexedColorMasks::releaseFramebuffer()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint maxDrawBuffers = 0;
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	if (maxDrawBuffers < 4)
	{
		throw tcu::ResourceError("Minimum number of draw buffers too low");
	}

	BlendMaskStateMachine state(m_context, m_testCtx.getLog(), maxDrawBuffers);
	state.SetDefaults();
	gl.deleteFramebuffers(1, &m_fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	glw::GLenum bufs[1] = { GL_BACK };
	gl.drawBuffers(1, bufs);
	gl.readBuffer(GL_BACK);
}

tcu::TestNode::IterateResult DrawBuffersIndexedColorMasks::iterate()
{
	static const glw::GLenum WriteMasksFormats[] = { GL_R8,		 GL_RG8,	 GL_RGB8,	 GL_RGB565,  GL_RGBA4,
													 GL_RGB5_A1, GL_RGBA8,   GL_R8I,	  GL_R8UI,	GL_R16I,
													 GL_R16UI,   GL_R32I,	GL_R32UI,	GL_RG8I,	GL_RG8UI,
													 GL_RG16I,   GL_RG16UI,  GL_RG32I,	GL_RG32UI,  GL_RGBA8I,
													 GL_RGBA8UI, GL_RGBA16I, GL_RGBA16UI, GL_RGBA32I, GL_RGBA32UI };
	static const int	kSize	= 32;
	static unsigned int formatId = 0;

	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	glw::GLenum			  format = WriteMasksFormats[formatId];

	prepareFramebuffer();

	// Check number of available draw buffers
	glw::GLint maxDrawBuffers = 0;
	gl.getIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	if (maxDrawBuffers < 4)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Minimum number of draw buffers too low");
		return STOP;
	}

	// Prepare render targets
	glw::GLuint tex;
	gl.genTextures(1, &tex);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, tex);
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, format, kSize, kSize, maxDrawBuffers);
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex, 0, i);
	}

	// Clear all buffers
	switch (ReadableType(format))
	{
	case GL_UNSIGNED_BYTE:
	{
		tcu::Vec4 c0(0.15f, 0.3f, 0.45f, 0.6f);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferfv(GL_COLOR, i, &c0[0]);
		}
		break;
	}
	case GL_UNSIGNED_INT:
	{
		tcu::UVec4 c0(2, 3, 4, 5);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferuiv(GL_COLOR, i, &c0[0]);
		}
		break;
	}
	case GL_INT:
	{
		tcu::IVec4 c0(2, 3, 4, 5);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferiv(GL_COLOR, i, &c0[0]);
		}
		break;
	}
	}

	// Set color masks for each buffer
	BlendMaskStateMachine state(m_context, m_testCtx.getLog(), maxDrawBuffers);

	glw::GLboolean mask[] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		mask[i % 4] = GL_TRUE;
		state.SetColorMaski(i, mask[0], mask[1], mask[2], mask[3]);
		mask[i % 4] = GL_FALSE;
	}

	// Clear all buffers
	switch (ReadableType(format))
	{
	case GL_UNSIGNED_BYTE:
	{
		tcu::Vec4 c1(0.85f, 0.85f, 0.85f, 0.85f);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferfv(GL_COLOR, i, &c1[0]);
		}
		break;
	}
	case GL_UNSIGNED_INT:
	{
		tcu::UVec4 c1(23, 23, 23, 23);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferuiv(GL_COLOR, i, &c1[0]);
		}
		break;
	}
	case GL_INT:
	{
		tcu::IVec4 c1(23, 23, 23, 23);
		for (int i = 0; i < maxDrawBuffers; ++i)
		{
			gl.clearBufferiv(GL_COLOR, i, &c1[0]);
		}
		break;
	}
	}

	// Verify color
	int		  numComponents = NumComponents(format);
	tcu::RGBA epsilon		= GetEpsilon();
	bool	  success		= true;

	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		gl.readBuffer(GL_COLOR_ATTACHMENT0 + i);

		switch (ReadableType(format))
		{
		case GL_UNSIGNED_BYTE:
		{
			tcu::UVec4 e(static_cast<unsigned int>(0.15f * 255), static_cast<unsigned int>(0.30f * 255),
						 static_cast<unsigned int>(0.45f * 255), static_cast<unsigned int>(0.60f * 255));
			e[i % 4] = static_cast<unsigned int>(0.85f * 255);
			e		 = tcu::UVec4(e.x(), numComponents >= 2 ? e.y() : 0, numComponents >= 3 ? e.z() : 0,
						   numComponents == 4 ? e.w() : 255);
			tcu::RGBA expected(e.x(), e.y(), e.z(), e.w());

			std::vector<unsigned char> rendered(kSize * kSize * 4, 45);

			tcu::TextureLevel textureLevel(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
										   kSize, kSize);
			glu::readPixels(m_context.getRenderContext(), 0, 0, textureLevel.getAccess());

			if (!VerifyImg(textureLevel, expected, epsilon))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Write mask error in texture format " << format
								   << " occurred for buffer #" << i << "\n"
								   << tcu::TestLog::EndMessage;
				m_testCtx.getLog() << tcu::TestLog::Image("Result", "Rendered result image", textureLevel.getAccess());
				success = false;
			}
			break;
		}
		case GL_UNSIGNED_INT:
		{
			tcu::UVec4 e(2, 3, 4, 5);
			e[i % 4] = 23;
			e		 = tcu::UVec4(e.x(), numComponents >= 2 ? e.y() : 0, numComponents >= 3 ? e.z() : 0,
						   numComponents == 4 ? e.w() : 1);
			tcu::RGBA expected(e.x(), e.y(), e.z(), e.w());

			tcu::TextureLevel textureLevel(
				tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32), kSize, kSize);
			glu::readPixels(m_context.getRenderContext(), 0, 0, textureLevel.getAccess());

			if (!VerifyImg(textureLevel, expected, epsilon))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Write mask error in texture format " << format
								   << " occurred for buffer #" << i << "\n"
								   << tcu::TestLog::EndMessage;
				m_testCtx.getLog() << tcu::TestLog::Image("Result", "Rendered result image", textureLevel.getAccess());
				success = false;
			}
			break;
		}
		case GL_INT:
		{
			tcu::UVec4 e(2, 3, 4, 5);
			e[i % 4] = 23;
			e		 = tcu::UVec4(e.x(), numComponents >= 2 ? e.y() : 0, numComponents >= 3 ? e.z() : 0,
						   numComponents == 4 ? e.w() : 1);
			tcu::RGBA expected(e.x(), e.y(), e.z(), e.w());

			tcu::TextureLevel textureLevel(
				tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT32), kSize, kSize);
			glu::readPixels(m_context.getRenderContext(), 0, 0, textureLevel.getAccess());

			if (!VerifyImg(textureLevel, expected, epsilon))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Write mask error in texture format " << format
								   << " occurred for buffer #" << i << "\n"
								   << tcu::TestLog::EndMessage;
				m_testCtx.getLog() << tcu::TestLog::Image("Result", "Rendered result image", textureLevel.getAccess());
				success = false;
			}
			break;
		}
		}
	}

	gl.colorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	gl.deleteTextures(1, &tex);
	releaseFramebuffer();

	// Check for error
	glw::GLenum error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Some functions generated error");
		formatId = 0;
		return STOP;
	}

	if (!success)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Write mask error occurred");
		formatId = 0;
		return STOP;
	}
	else
	{
		++formatId;
		if (formatId < (sizeof(WriteMasksFormats) / sizeof(WriteMasksFormats[0])))
		{
			return CONTINUE;
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
			formatId = 0;
			return STOP;
		}
	}
}

unsigned int DrawBuffersIndexedColorMasks::NumComponents(glw::GLenum format)
{
	switch (format)
	{
	case GL_R8:
	case GL_R8I:
	case GL_R8UI:
	case GL_R16I:
	case GL_R16UI:
	case GL_R32I:
	case GL_R32UI:
		return 1;
	case GL_RG8:
	case GL_RG8I:
	case GL_RG8UI:
	case GL_RG16I:
	case GL_RG16UI:
	case GL_RG32I:
	case GL_RG32UI:
		return 2;
	case GL_RGB8:
	case GL_RGB565:
		return 3;
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGB10_A2:
	case GL_RGBA8I:
	case GL_RGBA8UI:
	case GL_RGBA16I:
	case GL_RGBA16UI:
	case GL_RGBA32I:
	case GL_RGBA32UI:
		return 4;
	default:
		return 0;
	}
}

glw::GLenum DrawBuffersIndexedColorMasks::ReadableType(glw::GLenum format)
{
	switch (format)
	{
	case GL_R8:
	case GL_RG8:
	case GL_RGB8:
	case GL_RGB565:
	case GL_RGBA4:
	case GL_RGB5_A1:
	case GL_RGBA8:
	case GL_RGB10_A2:
		return GL_UNSIGNED_BYTE;

	case GL_R8I:
	case GL_R16I:
	case GL_R32I:
	case GL_RG8I:
	case GL_RG16I:
	case GL_RG32I:
	case GL_RGBA8I:
	case GL_RGBA16I:
	case GL_RGBA32I:
		return GL_INT;

	case GL_R8UI:
	case GL_R16UI:
	case GL_R32UI:
	case GL_RG8UI:
	case GL_RG16UI:
	case GL_RG32UI:
	case GL_RGB10_A2UI:
	case GL_RGBA8UI:
	case GL_RGBA16UI:
	case GL_RGBA32UI:
		return GL_UNSIGNED_INT;

	default:
		return 0;
	}
}

tcu::RGBA DrawBuffersIndexedColorMasks::GetEpsilon()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	tcu::IVec4 bits;
	tcu::UVec4 epsilon;

	for (int i = 0; i < 4; ++i)
	{
		gl.getIntegerv(GL_RED_BITS + i, &bits[i]);
		epsilon[i] = de::min(
			255u, static_cast<unsigned int>(ceil(1.0 + 255.0 * (1.0 / pow(2.0, static_cast<double>(bits[i]))))));
	}

	return tcu::RGBA(epsilon.x(), epsilon.y(), epsilon.z(), epsilon.w());
}

bool DrawBuffersIndexedColorMasks::VerifyImg(const tcu::TextureLevel& textureLevel, tcu::RGBA expectedColor,
											 tcu::RGBA epsilon)
{
	for (int y = 0; y < textureLevel.getHeight(); ++y)
	{
		for (int x = 0; x < textureLevel.getWidth(); ++x)
		{
			tcu::IVec4 color(textureLevel.getAccess().getPixelInt(x, y));
			tcu::RGBA  pixel(color.x(), color.y(), color.z(), color.w());

			if (!tcu::compareThreshold(pixel, expectedColor, epsilon))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Expected value: " << expectedColor << "\n"
								   << "Read value:     " << pixel << "\n"
								   << "Epsilon:        " << epsilon << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}
	return true;
}

} // namespace glcts
