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
 * \file  esextcDrawBuffersIndexedBlending.hpp
 * \brief Draw Buffers Indexed tests 5. Blending
 */ /*-------------------------------------------------------------------*/

#include "esextcDrawBuffersIndexedBlending.hpp"
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
DrawBuffersIndexedBlending::DrawBuffersIndexedBlending(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description)
	: DrawBuffersIndexedBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

void DrawBuffersIndexedBlending::prepareFramebuffer()
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
}

void DrawBuffersIndexedBlending::releaseFramebuffer()
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

tcu::TestNode::IterateResult DrawBuffersIndexedBlending::iterate()
{
	static const glw::GLenum BlendFormats[] = {
		GL_R8, GL_RG8, GL_RGB8, GL_RGB565, GL_RGBA4, GL_RGBA8,
	};
	static const int	kSize	= 32;
	static unsigned int formatId = 0;

	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	glw::GLenum			  format = BlendFormats[formatId];

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

	// Clear background color
	tcu::Vec4 background(0.5f, 0.5f, 0.5f, 0.5f);
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		gl.clearBufferfv(GL_COLOR, i, &background[0]);
	}

	// Prepare expected, blended color values
	tcu::Vec4 colors[] = { tcu::Vec4(0.86f, 0.22f, 0.31f, 0.45f), tcu::Vec4(0.12f, 0.83f, 0.34f, 0.42f),
						   tcu::Vec4(0.56f, 0.63f, 0.76f, 0.99f), tcu::Vec4(0.14f, 0.34f, 0.34f, 0.22f) };

	int		  numComponents = NumComponents(format);
	tcu::RGBA expected[]	= {
		// GL_MIN
		tcu::RGBA(static_cast<unsigned int>(background.x() * 255),
				  static_cast<unsigned int>((numComponents >= 2 ? colors[0].y() : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents >= 3 ? colors[0].z() : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents == 4 ? background.w() : 1.0f) * 255)),
		// GL_FUNC_ADD
		tcu::RGBA(static_cast<unsigned int>(background.x() * 255),
				  static_cast<unsigned int>((numComponents >= 2 ? background.y() : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents >= 3 ? background.z() : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents == 4 ? colors[1].w() : 1.0f) * 255)),
		// GL_FUNC_SUBTRACT
		tcu::RGBA(
			static_cast<unsigned int>((colors[2].x() * (numComponents == 4 ? colors[2].w() : 1.0f) -
									   background.x() * (numComponents == 4 ? background.w() : 1.0f)) *
									  255),
			static_cast<unsigned int>((numComponents >= 2 ?
										   (colors[2].y() * (numComponents == 4 ? colors[2].w() : 1.0f) -
											background.y() * (numComponents == 4 ? background.w() : 1.0f)) :
										   0.0f) *
									  255),
			static_cast<unsigned int>((numComponents >= 3 ?
										   (colors[2].z() * (numComponents == 4 ? colors[2].w() : 1.0f) -
											background.z() * (numComponents == 4 ? background.w() : 1.0f)) :
										   0.0f) *
									  255),
			static_cast<unsigned int>(
				(numComponents == 4 ? (colors[2].w() * colors[2].w() - background.w() * background.w()) : 1.0f) * 255)),
		// GL_FUNC_REVERSE_SUBTRACT
		tcu::RGBA(static_cast<unsigned int>((background.x() - colors[3].x()) * 255),
				  static_cast<unsigned int>((numComponents >= 2 ? (background.y() - colors[3].y()) : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents >= 3 ? (background.z() - colors[3].z()) : 0.0f) * 255),
				  static_cast<unsigned int>((numComponents == 4 ? (background.w() - colors[3].w()) : 1.0f) * 255))
	};

	// Setup blending operations
	BlendMaskStateMachine state(m_context, m_testCtx.getLog(), maxDrawBuffers);
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		switch (i % 4)
		{
		case 0:
			// GL_MIN
			state.SetEnablei(i);
			state.SetBlendEquationSeparatei(i, GL_MIN, GL_MAX);
			state.SetBlendFunci(i, GL_ONE, GL_ONE);
			break;
		case 1:
			// GL_FUNC_ADD
			state.SetEnablei(i);
			state.SetBlendEquationi(i, GL_FUNC_ADD);
			state.SetBlendFuncSeparatei(i, GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);
			break;
		case 2:
			// GL_FUNC_SUBTRACT
			state.SetEnablei(i);
			state.SetBlendEquationi(i, GL_FUNC_SUBTRACT);
			state.SetBlendFunci(i, GL_SRC_ALPHA, GL_DST_ALPHA);
			break;
		case 3:
			// GL_FUNC_REVERSE_SUBTRACT
			state.SetEnablei(i);
			state.SetBlendEquationi(i, GL_FUNC_REVERSE_SUBTRACT);
			state.SetBlendFunci(i, GL_ONE, GL_ONE);
			break;
		}
	}

	// Prepare shader programs and draw fullscreen quad
	glu::ShaderProgram program(m_context.getRenderContext(),
							   glu::makeVtxFragSources(GenVS().c_str(), GenFS(maxDrawBuffers).c_str()));
	if (!program.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Could not create shader program");
		return STOP;
	}
	gl.useProgram(program.getProgram());

	glw::GLuint positionLocation = gl.getAttribLocation(program.getProgram(), "position");
	tcu::Vec3   vertices[]		 = {
		tcu::Vec3(-1.0f, -1.0f, 0.0f), tcu::Vec3(1.0f, -1.0f, 0.0f), tcu::Vec3(-1.0f, 1.0f, 0.0f),
		tcu::Vec3(1.0f, 1.0f, 0.0f),   tcu::Vec3(-1.0f, 1.0f, 0.0f), tcu::Vec3(1.0f, -1.0f, 0.0f)
	};

	gl.vertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	gl.enableVertexAttribArray(positionLocation);

	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		std::ostringstream os;
		os << "c" << i;
		// i.e.: glUniform4fv(glGetUniformLocation(m_program, "c0"), 1, &colors[i].r);
		gl.uniform4fv(gl.getUniformLocation(program.getProgram(), os.str().c_str()), 1, &colors[i % 4][0]);
	}

	gl.drawArrays(GL_TRIANGLES, 0, 6);

	// Read buffer colors and validate proper blending behaviour
	bool	  success = true;
	tcu::RGBA epsilon = GetEpsilon();
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		gl.readBuffer(GL_COLOR_ATTACHMENT0 + i);

		tcu::TextureLevel textureLevel(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
									   kSize, kSize);
		glu::readPixels(m_context.getRenderContext(), 0, 0, textureLevel.getAccess());

		if (!VerifyImg(textureLevel, expected[i % 4], epsilon))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Blending error in texture format " << format
							   << " occurred for draw buffer #" << i << "\n"
							   << tcu::TestLog::EndMessage;
			m_testCtx.getLog() << tcu::TestLog::Image("Result", "Rendered result image", textureLevel.getAccess());
			success = false;
		}
	}

	gl.disable(GL_BLEND);
	gl.useProgram(0);
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
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Blending error occurred");
		formatId = 0;
		return STOP;
	}
	else
	{
		++formatId;
		if (formatId < (sizeof(BlendFormats) / sizeof(BlendFormats[0])))
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

std::string DrawBuffersIndexedBlending::GenVS()
{
	std::ostringstream os;
	os << "#version 300 es                        \n"
		  "precision highp float;                 \n"
		  "precision highp int;                   \n"
		  "layout(location = 0) in vec4 position; \n"
		  "void main() {                          \n"
		  "    gl_Position = position;            \n"
		  "}";
	return os.str();
}
std::string DrawBuffersIndexedBlending::GenFS(int maxDrawBuffers)
{
	std::ostringstream os;
	os << "#version 300 es                        \n"
		  "precision highp float;                 \n"
		  "precision highp int;                   \n";

	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		os << "\nlayout(location = " << i << ") out vec4 color" << i << ";";
	}
	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		os << "\nuniform vec4 c" << i << ";";
	}

	os << "\nvoid main() {";

	for (int i = 0; i < maxDrawBuffers; ++i)
	{
		os << "\n    color" << i << " = c" << i << ";";
	}

	os << "\n}";
	return os.str();
}

unsigned int DrawBuffersIndexedBlending::NumComponents(glw::GLenum format)
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

tcu::RGBA DrawBuffersIndexedBlending::GetEpsilon()
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

bool DrawBuffersIndexedBlending::VerifyImg(const tcu::TextureLevel& textureLevel, tcu::RGBA expectedColor,
										   tcu::RGBA epsilon)
{
	for (int y = 0; y < textureLevel.getHeight(); ++y)
	{
		for (int x = 0; x < textureLevel.getWidth(); ++x)
		{
			tcu::RGBA pixel(textureLevel.getAccess().getPixel(x, y));
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
