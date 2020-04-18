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
 * \brief Stencil texturing tests.
 *//*--------------------------------------------------------------------*/

#include "es31fStencilTexturingTests.hpp"

#include "gluStrUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureUtil.hpp"
#include "gluContextInfo.hpp"

#include "glsTextureTestUtil.hpp"

#include "tcuVector.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexLookupVerifier.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "deStringUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

using std::vector;
using std::string;
using tcu::IVec4;
using tcu::Vec2;
using tcu::Vec4;
using tcu::TestLog;
using tcu::TextureLevel;
using tcu::TextureFormat;

namespace
{

static void genTestRects (vector<IVec4>& rects, int width, int height)
{
	int curWidth	= width;
	int curHeight	= height;
	int ndx			= 0;

	for (;;)
	{
		rects.push_back(IVec4(width-curWidth, height-curHeight, curWidth, curHeight));

		DE_ASSERT(curWidth >= 1 && curHeight >= 1);
		if (curWidth == 1 && curHeight == 1)
			break;
		else if (curHeight > 1 && ((ndx%2) == 0 || curWidth == 1))
			curHeight -= 1;
		else
			curWidth -= 1;

		ndx += 1;
	}
}

static void rectsToTriangles (const vector<IVec4>& rects, int width, int height, vector<Vec2>& positions, vector<deUint16>& indices)
{
	const float		w		= float(width);
	const float		h		= float(height);

	positions.resize(rects.size()*4);
	indices.resize(rects.size()*6);

	for (int rectNdx = 0; rectNdx < (int)rects.size(); rectNdx++)
	{
		const int		rx		= rects[rectNdx].x();
		const int		ry		= rects[rectNdx].y();
		const int		rw		= rects[rectNdx].z();
		const int		rh		= rects[rectNdx].w();

		const float		x0		= float(rx*2)/w - 1.0f;
		const float		x1		= float((rx+rw)*2)/w - 1.0f;
		const float		y0		= float(ry*2)/h - 1.0f;
		const float		y1		= float((ry+rh)*2)/h - 1.0f;

		positions[rectNdx*4 + 0] = Vec2(x0, y0);
		positions[rectNdx*4 + 1] = Vec2(x1, y0);
		positions[rectNdx*4 + 2] = Vec2(x0, y1);
		positions[rectNdx*4 + 3] = Vec2(x1, y1);

		indices[rectNdx*6 + 0] = (deUint16)(rectNdx*4 + 0);
		indices[rectNdx*6 + 1] = (deUint16)(rectNdx*4 + 1);
		indices[rectNdx*6 + 2] = (deUint16)(rectNdx*4 + 2);
		indices[rectNdx*6 + 3] = (deUint16)(rectNdx*4 + 2);
		indices[rectNdx*6 + 4] = (deUint16)(rectNdx*4 + 1);
		indices[rectNdx*6 + 5] = (deUint16)(rectNdx*4 + 3);
	}
}

static void drawTestPattern (const glu::RenderContext& renderCtx, int width, int height)
{
	const glu::ShaderProgram program(renderCtx, glu::ProgramSources()
		<< glu::VertexSource(
			"#version 300 es\n"
			"in highp vec4 a_position;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"}\n")
		<< glu::FragmentSource(
			"#version 300 es\n"
			"void main (void) {}\n"));

	const glw::Functions&	gl		= renderCtx.getFunctions();
	vector<IVec4>			rects;
	vector<Vec2>			positions;
	vector<deUint16>		indices;

	if (!program.isOk())
		throw tcu::TestError("Compile failed");

	gl.useProgram	(program.getProgram());
	gl.viewport		(0, 0, width, height);
	gl.clear		(GL_STENCIL_BUFFER_BIT);
	gl.enable		(GL_STENCIL_TEST);
	gl.stencilOp	(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
	gl.stencilFunc	(GL_ALWAYS, 0, ~0u);
	GLU_EXPECT_NO_ERROR(gl.getError(), "State setup failed");

	genTestRects	(rects, width, height);
	rectsToTriangles(rects, width, height, positions, indices);

	{
		const glu::VertexArrayBinding posBinding = glu::va::Float("a_position", 2, (int)positions.size(), 0, positions[0].getPtr());
		glu::draw(renderCtx, program.getProgram(), 1, &posBinding, glu::pr::Triangles((int)indices.size(), &indices[0]));
	}

	gl.disable(GL_STENCIL_TEST);
}

static void renderTestPatternReference (const tcu::PixelBufferAccess& dst)
{
	const int		stencilBits		= tcu::getTextureFormatBitDepth(tcu::getEffectiveDepthStencilAccess(dst, tcu::Sampler::MODE_STENCIL).getFormat()).x();
	const deUint32	stencilMask		= (1u<<stencilBits)-1u;
	vector<IVec4>	rects;

	DE_ASSERT(dst.getFormat().order == TextureFormat::S || dst.getFormat().order == TextureFormat::DS);

	// clear depth and stencil
	if (dst.getFormat().order == TextureFormat::DS)
		tcu::clearDepth(dst, 0.0f);
	tcu::clearStencil(dst, 0u);

	genTestRects(rects, dst.getWidth(), dst.getHeight());

	for (vector<IVec4>::const_iterator rectIter = rects.begin(); rectIter != rects.end(); ++rectIter)
	{
		const int	x0		= rectIter->x();
		const int	y0		= rectIter->y();
		const int	x1		= x0+rectIter->z();
		const int	y1		= y0+rectIter->w();

		for (int y = y0; y < y1; y++)
		{
			for (int x = x0; x < x1; x++)
			{
				const int oldVal	= dst.getPixStencil(x, y);
				const int newVal	= (oldVal+1)&stencilMask;

				dst.setPixStencil(newVal, x, y);
			}
		}
	}
}

static void blitStencilToColor2D (const glu::RenderContext& renderCtx, deUint32 srcTex, int width, int height)
{
	const glu::ShaderProgram program(renderCtx, glu::ProgramSources()
		<< glu::VertexSource(
			"#version 300 es\n"
			"in highp vec4 a_position;\n"
			"in highp vec2 a_texCoord;\n"
			"out highp vec2 v_texCoord;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_texCoord = a_texCoord;\n"
			"}\n")
		<< glu::FragmentSource(
			"#version 300 es\n"
			"uniform highp usampler2D u_sampler;\n"
			"in highp vec2 v_texCoord;\n"
			"layout(location = 0) out highp uint o_stencil;\n"
			"void main (void)\n"
			"{\n"
			"	o_stencil = texture(u_sampler, v_texCoord).x;\n"
			"}\n"));

	const float positions[] =
	{
		-1.0f, -1.0f,
		+1.0f, -1.0f,
		-1.0f, +1.0f,
		+1.0f, +1.0f
	};
	const float texCoord[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f
	};
	const glu::VertexArrayBinding vertexArrays[] =
	{
		glu::va::Float("a_position", 2, 4, 0, &positions[0]),
		glu::va::Float("a_texCoord", 2, 4, 0, &texCoord[0])
	};
	const deUint8 indices[] = { 0, 1, 2, 2, 1, 3 };

	const glw::Functions& gl = renderCtx.getFunctions();

	if (!program.isOk())
		throw tcu::TestError("Compile failed");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, srcTex);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture state setup failed");

	gl.useProgram(program.getProgram());
	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program setup failed");

	gl.viewport(0, 0, width, height);
	glu::draw(renderCtx, program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), &vertexArrays[0],
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
}

static void blitStencilToColor2DArray (const glu::RenderContext& renderCtx, deUint32 srcTex, int width, int height, int level)
{
	const glu::ShaderProgram program(renderCtx, glu::ProgramSources()
		<< glu::VertexSource(
			"#version 300 es\n"
			"in highp vec4 a_position;\n"
			"in highp vec3 a_texCoord;\n"
			"out highp vec3 v_texCoord;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_texCoord = a_texCoord;\n"
			"}\n")
		<< glu::FragmentSource(
			"#version 300 es\n"
			"uniform highp usampler2DArray u_sampler;\n"
			"in highp vec3 v_texCoord;\n"
			"layout(location = 0) out highp uint o_stencil;\n"
			"void main (void)\n"
			"{\n"
			"	o_stencil = texture(u_sampler, v_texCoord).x;\n"
			"}\n"));

	const float positions[] =
	{
		-1.0f, -1.0f,
		+1.0f, -1.0f,
		-1.0f, +1.0f,
		+1.0f, +1.0f
	};
	const float texCoord[] =
	{
		0.0f, 0.0f, float(level),
		1.0f, 0.0f, float(level),
		0.0f, 1.0f, float(level),
		1.0f, 1.0f, float(level)
	};
	const glu::VertexArrayBinding vertexArrays[] =
	{
		glu::va::Float("a_position", 2, 4, 0, &positions[0]),
		glu::va::Float("a_texCoord", 3, 4, 0, &texCoord[0])
	};
	const deUint8 indices[] = { 0, 1, 2, 2, 1, 3 };

	const glw::Functions& gl = renderCtx.getFunctions();

	if (!program.isOk())
		throw tcu::TestError("Compile failed");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, srcTex);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture state setup failed");

	gl.useProgram(program.getProgram());
	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program setup failed");

	gl.viewport(0, 0, width, height);
	glu::draw(renderCtx, program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), &vertexArrays[0],
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
}

static void blitStencilToColorCube (const glu::RenderContext& renderCtx, deUint32 srcTex, const float* texCoord, int width, int height)
{
	const glu::ShaderProgram program(renderCtx, glu::ProgramSources()
		<< glu::VertexSource(
			"#version 300 es\n"
			"in highp vec4 a_position;\n"
			"in highp vec3 a_texCoord;\n"
			"out highp vec3 v_texCoord;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_texCoord = a_texCoord;\n"
			"}\n")
		<< glu::FragmentSource(
			"#version 300 es\n"
			"uniform highp usamplerCube u_sampler;\n"
			"in highp vec3 v_texCoord;\n"
			"layout(location = 0) out highp vec4 o_color;\n"
			"void main (void)\n"
			"{\n"
			"	o_color.x = float(texture(u_sampler, v_texCoord).x) / 255.0;\n"
			"	o_color.yzw = vec3(0.0, 0.0, 1.0);\n"
			"}\n"));

	const float positions[] =
	{
		-1.0f, -1.0f,
		-1.0f, +1.0f,
		+1.0f, -1.0f,
		+1.0f, +1.0f
	};
	const glu::VertexArrayBinding vertexArrays[] =
	{
		glu::va::Float("a_position", 2, 4, 0, &positions[0]),
		glu::va::Float("a_texCoord", 3, 4, 0, texCoord)
	};
	const deUint8 indices[] = { 0, 1, 2, 2, 1, 3 };

	const glw::Functions& gl = renderCtx.getFunctions();

	if (!program.isOk())
		throw tcu::TestError("Compile failed");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP, srcTex);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Texture state setup failed");

	gl.useProgram(program.getProgram());
	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program setup failed");

	gl.viewport(0, 0, width, height);
	glu::draw(renderCtx, program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), &vertexArrays[0],
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
}

static inline tcu::ConstPixelBufferAccess stencilToRedAccess (const tcu::ConstPixelBufferAccess& access)
{
	DE_ASSERT(access.getFormat() == TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8));
	return tcu::ConstPixelBufferAccess(TextureFormat(TextureFormat::R, TextureFormat::UNSIGNED_INT8), access.getSize(), access.getPitch(), access.getDataPtr());
}

static bool compareStencilToRed (tcu::TestLog& log, const tcu::ConstPixelBufferAccess& stencilRef, const tcu::ConstPixelBufferAccess& result)
{
	const int		maxPrints		= 10;
	int				numFailed		= 0;

	DE_ASSERT(stencilRef.getFormat().order == TextureFormat::S);
	DE_ASSERT(stencilRef.getWidth() == result.getWidth() && stencilRef.getHeight() == result.getHeight());

	for (int y = 0; y < stencilRef.getHeight(); y++)
	{
		for (int x = 0; x < stencilRef.getWidth(); x++)
		{
			const int		ref		= stencilRef.getPixStencil(x, y);
			const int		res		= result.getPixelInt(x, y).x();

			if (ref != res)
			{
				if (numFailed < maxPrints)
					log << TestLog::Message << "ERROR: Expected " << ref << ", got " << res << " at (" << x << ", " << y << ")" << TestLog::EndMessage;
				else if (numFailed == maxPrints)
					log << TestLog::Message << "..." << TestLog::EndMessage;

				numFailed += 1;
			}
		}
	}

	log << TestLog::Message << "Found " << numFailed << " faulty pixels, comparison " << (numFailed == 0 ? "passed." : "FAILED!") << TestLog::EndMessage;

	log << TestLog::ImageSet("ComparisonResult", "Image comparison result")
		<< TestLog::Image("Result", "Result stencil buffer", result);

	if (numFailed > 0)
		log << TestLog::Image("Reference", "Reference stencil buffer", stencilToRedAccess(stencilRef));

	log << TestLog::EndImageSet;

	return numFailed == 0;
}

static bool compareRedChannel (tcu::TestLog& log, const tcu::ConstPixelBufferAccess& result, int reference)
{
	const int		maxPrints		= 10;
	int				numFailed		= 0;

	for (int y = 0; y < result.getHeight(); y++)
	{
		for (int x = 0; x < result.getWidth(); x++)
		{
			const int res = result.getPixelInt(x, y).x();

			if (reference != res)
			{
				if (numFailed < maxPrints)
					log << TestLog::Message << "ERROR: Expected " << reference << ", got " << res << " at (" << x << ", " << y << ")" << TestLog::EndMessage;
				else if (numFailed == maxPrints)
					log << TestLog::Message << "..." << TestLog::EndMessage;

				numFailed += 1;
			}
		}
	}

	log << TestLog::Message << "Found " << numFailed << " faulty pixels, comparison " << (numFailed == 0 ? "passed." : "FAILED!") << TestLog::EndMessage;

	log << TestLog::ImageSet("ComparisonResult", "Image comparison result")
		<< TestLog::Image("Result", "Result stencil buffer", result);

	log << TestLog::EndImageSet;

	return numFailed == 0;
}

static void stencilToUnorm8 (const tcu::TextureCube& src, tcu::TextureCube& dst)
{
	for (int levelNdx = 0; levelNdx < src.getNumLevels(); levelNdx++)
	{
		for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
		{
			const tcu::CubeFace face = tcu::CubeFace(faceNdx);

			if (!src.isLevelEmpty(face, levelNdx))
			{
				dst.allocLevel(face, levelNdx);

				const tcu::ConstPixelBufferAccess	srcLevel	= src.getLevelFace(levelNdx, face);
				const tcu::PixelBufferAccess		dstLevel	= dst.getLevelFace(levelNdx, face);

				for (int y = 0; y < src.getSize(); y++)
				for (int x = 0; x < src.getSize(); x++)
					dstLevel.setPixel(Vec4(float(srcLevel.getPixStencil(x, y)) / 255.f, 0.f, 0.f, 1.f), x, y);
			}
		}
	}
}

static void checkDepthStencilFormatSupport (Context& context, deUint32 format)
{
	if (format == GL_STENCIL_INDEX8)
	{
		const char* reqExt = "GL_OES_texture_stencil8";
		if ((context.getRenderContext().getType().getAPI() != glu::ApiType::core(3,2)) && !context.getContextInfo().isExtensionSupported(reqExt))
			throw tcu::NotSupportedError(glu::getTextureFormatStr(format).toString() + " requires " + reqExt);
	}
	else
	{
		DE_ASSERT(format == GL_DEPTH32F_STENCIL8 || format == GL_DEPTH24_STENCIL8);
	}
}

static void checkFramebufferStatus (const glw::Functions& gl)
{
	const deUint32 status = gl.checkFramebufferStatus(GL_FRAMEBUFFER);

	if (status == GL_FRAMEBUFFER_UNSUPPORTED)
		throw tcu::NotSupportedError("Unsupported framebuffer configuration");
	else if (status != GL_FRAMEBUFFER_COMPLETE)
		throw tcu::TestError("Incomplete framebuffer: " + glu::getFramebufferStatusStr(status).toString());
}

class UploadTex2DCase : public TestCase
{
public:
	UploadTex2DCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx			= m_context.getRenderContext();
		const glw::Functions&		gl					= renderCtx.getFunctions();
		const int					width				= 129;
		const int					height				= 113;
		glu::Framebuffer			fbo					(renderCtx);
		glu::Renderbuffer			colorBuf			(renderCtx);
		glu::Texture				depthStencilTex		(renderCtx);
		TextureLevel				uploadLevel			(glu::mapGLInternalFormat(m_format), width, height);
		TextureLevel				readLevel			(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), width, height);
		TextureLevel				stencilOnlyLevel	(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), width, height);

		checkDepthStencilFormatSupport(m_context, m_format);

		renderTestPatternReference(uploadLevel);
		renderTestPatternReference(stencilOnlyLevel);

		gl.bindTexture(GL_TEXTURE_2D, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, m_format, width, height);
		glu::texSubImage2D(renderCtx, GL_TEXTURE_2D, 0, 0, 0, uploadLevel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading texture data failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, width, height);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		checkFramebufferStatus(gl);

		blitStencilToColor2D(renderCtx, *depthStencilTex, width, height);
		glu::readPixels(renderCtx, 0, 0, readLevel);

		{
			const bool compareOk = compareStencilToRed(m_testCtx.getLog(), stencilOnlyLevel, readLevel);
			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class UploadTex2DArrayCase : public TestCase
{
public:
	UploadTex2DArrayCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx			= m_context.getRenderContext();
		const glw::Functions&		gl					= renderCtx.getFunctions();
		const int					width				= 41;
		const int					height				= 13;
		const int					levels				= 7;
		const int					ptrnLevel			= 3;
		glu::Framebuffer			fbo					(renderCtx);
		glu::Renderbuffer			colorBuf			(renderCtx);
		glu::Texture				depthStencilTex		(renderCtx);
		TextureLevel				uploadLevel			(glu::mapGLInternalFormat(m_format), width, height, levels);

		checkDepthStencilFormatSupport(m_context, m_format);

		for (int levelNdx = 0; levelNdx < levels; levelNdx++)
		{
			const tcu::PixelBufferAccess levelAccess = tcu::getSubregion(uploadLevel.getAccess(), 0, 0, levelNdx, width, height, 1);

			if (levelNdx == ptrnLevel)
				renderTestPatternReference(levelAccess);
			else
				tcu::clearStencil(levelAccess, levelNdx);
		}

		gl.bindTexture(GL_TEXTURE_2D_ARRAY, *depthStencilTex);
		gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, m_format, width, height, levels);
		glu::texSubImage3D(renderCtx, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, uploadLevel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading texture data failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, width, height);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		checkFramebufferStatus(gl);

		{
			TextureLevel	readLevel		(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), width, height);
			bool			allLevelsOk		= true;

			for (int levelNdx = 0; levelNdx < levels; levelNdx++)
			{
				tcu::ScopedLogSection section(m_testCtx.getLog(), "Level" + de::toString(levelNdx), "Level " + de::toString(levelNdx));
				bool levelOk;

				blitStencilToColor2DArray(renderCtx, *depthStencilTex, width, height, levelNdx);
				glu::readPixels(renderCtx, 0, 0, readLevel);

				if (levelNdx == ptrnLevel)
				{
					TextureLevel reference(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), width, height);
					renderTestPatternReference(reference);

					levelOk = compareStencilToRed(m_testCtx.getLog(), reference, readLevel);
				}
				else
					levelOk = compareRedChannel(m_testCtx.getLog(), readLevel, levelNdx);

				if (!levelOk)
				{
					allLevelsOk = false;
					break;
				}
			}

			m_testCtx.setTestResult(allLevelsOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
									allLevelsOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class UploadTexCubeCase : public TestCase
{
public:
	UploadTexCubeCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx			= m_context.getRenderContext();
		const glw::Functions&		gl					= renderCtx.getFunctions();
		const int					size				= 64;
		const int					renderWidth			= 128;
		const int					renderHeight		= 128;
		vector<float>				texCoord;
		glu::Framebuffer			fbo					(renderCtx);
		glu::Renderbuffer			colorBuf			(renderCtx);
		glu::Texture				depthStencilTex		(renderCtx);
		tcu::TextureCube			texData				(glu::mapGLInternalFormat(m_format), size);
		tcu::TextureLevel			result				(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), renderWidth, renderHeight);

		checkDepthStencilFormatSupport(m_context, m_format);

		for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
		{
			const tcu::CubeFace		face		= tcu::CubeFace(faceNdx);
			const int				stencilVal	= 42*faceNdx;

			texData.allocLevel(face, 0);
			tcu::clearStencil(texData.getLevelFace(0, face), stencilVal);
		}

		glu::TextureTestUtil::computeQuadTexCoordCube(texCoord, tcu::CUBEFACE_NEGATIVE_X, Vec2(-1.5f, -1.3f), Vec2(1.3f, 1.4f));

		gl.bindTexture(GL_TEXTURE_CUBE_MAP, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_CUBE_MAP, 1, m_format, size, size);

		for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
			glu::texSubImage2D(renderCtx, glu::getGLCubeFace(tcu::CubeFace(faceNdx)), 0, 0, 0, texData.getLevelFace(0, tcu::CubeFace(faceNdx)));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading texture data failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, renderWidth, renderHeight);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		checkFramebufferStatus(gl);

		blitStencilToColorCube(renderCtx, *depthStencilTex, &texCoord[0], renderWidth, renderHeight);
		glu::readPixels(renderCtx, 0, 0, result);

		{
			using namespace glu::TextureTestUtil;

			tcu::TextureCube		redTex			(TextureFormat(TextureFormat::R, TextureFormat::UNORM_INT8), size);
			const ReferenceParams	sampleParams	(TEXTURETYPE_CUBE, tcu::Sampler(tcu::Sampler::CLAMP_TO_EDGE,
																					tcu::Sampler::CLAMP_TO_EDGE,
																					tcu::Sampler::CLAMP_TO_EDGE,
																					tcu::Sampler::NEAREST,
																					tcu::Sampler::NEAREST));
			tcu::LookupPrecision	lookupPrec;
			tcu::LodPrecision		lodPrec;
			bool					compareOk;

			lookupPrec.colorMask		= tcu::BVec4(true, true, true, true);
			lookupPrec.colorThreshold	= tcu::computeFixedPointThreshold(IVec4(8, 8, 8, 8));
			lookupPrec.coordBits		= tcu::IVec3(22, 22, 22);
			lookupPrec.uvwBits			= tcu::IVec3(5, 5, 0);
			lodPrec.lodBits				= 7;
			lodPrec.derivateBits		= 16;

			stencilToUnorm8(texData, redTex);

			compareOk = verifyTextureResult(m_testCtx, result, redTex, &texCoord[0], sampleParams, lookupPrec, lodPrec, tcu::PixelFormat(8, 8, 8, 8));

			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class RenderTex2DCase : public TestCase
{
public:
	RenderTex2DCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx		= m_context.getRenderContext();
		const glw::Functions&		gl				= renderCtx.getFunctions();
		const int					width			= 117;
		const int					height			= 193;
		glu::Framebuffer			fbo				(renderCtx);
		glu::Renderbuffer			colorBuf		(renderCtx);
		glu::Texture				depthStencilTex	(renderCtx);
		TextureLevel				result			(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), width, height);
		TextureLevel				reference		(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), width, height);

		checkDepthStencilFormatSupport(m_context, m_format);

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, width, height);

		gl.bindTexture(GL_TEXTURE_2D, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, m_format, width, height);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, *depthStencilTex, 0);
		checkFramebufferStatus(gl);

		drawTestPattern(renderCtx, width, height);

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
		checkFramebufferStatus(gl);

		blitStencilToColor2D(renderCtx, *depthStencilTex, width, height);
		glu::readPixels(renderCtx, 0, 0, result.getAccess());

		renderTestPatternReference(reference);

		{
			const bool compareOk = compareStencilToRed(m_testCtx.getLog(), reference, result);
			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class ClearTex2DCase : public TestCase
{
public:
	ClearTex2DCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx		= m_context.getRenderContext();
		const glw::Functions&		gl				= renderCtx.getFunctions();
		const int					width			= 125;
		const int					height			= 117;
		const int					cellSize		= 8;
		glu::Framebuffer			fbo				(renderCtx);
		glu::Renderbuffer			colorBuf		(renderCtx);
		glu::Texture				depthStencilTex	(renderCtx);
		TextureLevel				result			(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), width, height);
		TextureLevel				reference		(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), width, height);

		checkDepthStencilFormatSupport(m_context, m_format);

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, width, height);

		gl.bindTexture(GL_TEXTURE_2D, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, m_format, width, height);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, *depthStencilTex, 0);
		checkFramebufferStatus(gl);

		gl.enable(GL_SCISSOR_TEST);

		for (int y = 0; y < height; y += cellSize)
		{
			for (int x = 0; x < width; x += cellSize)
			{
				const int		clearW		= de::min(cellSize, width-x);
				const int		clearH		= de::min(cellSize, height-y);
				const int		stencil		= int((deInt32Hash(x) ^ deInt32Hash(y)) & 0xff);

				gl.clearStencil(stencil);
				gl.scissor(x, y, clearW, clearH);
				gl.clear(GL_STENCIL_BUFFER_BIT);

				tcu::clearStencil(tcu::getSubregion(reference.getAccess(), x, y, clearW, clearH), stencil);
			}
		}

		gl.disable(GL_SCISSOR_TEST);

		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
		checkFramebufferStatus(gl);

		blitStencilToColor2D(renderCtx, *depthStencilTex, width, height);
		glu::readPixels(renderCtx, 0, 0, result.getAccess());

		{
			const bool compareOk = compareStencilToRed(m_testCtx.getLog(), reference, result);
			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class CompareModeCase : public TestCase
{
public:
	CompareModeCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx			= m_context.getRenderContext();
		const glw::Functions&		gl					= renderCtx.getFunctions();
		const int					width				= 64;
		const int					height				= 64;
		glu::Framebuffer			fbo					(renderCtx);
		glu::Renderbuffer			colorBuf			(renderCtx);
		glu::Texture				depthStencilTex		(renderCtx);
		TextureLevel				uploadLevel			(glu::mapGLInternalFormat(m_format), width, height);
		TextureLevel				readLevel			(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), width, height);
		TextureLevel				stencilOnlyLevel	(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), width, height);

		checkDepthStencilFormatSupport(m_context, m_format);

		m_testCtx.getLog() << TestLog::Message << "NOTE: Texture compare mode has no effect when reading stencil values." << TestLog::EndMessage;

		renderTestPatternReference(uploadLevel);
		renderTestPatternReference(stencilOnlyLevel);

		gl.bindTexture(GL_TEXTURE_2D, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, m_format, width, height);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
		glu::texSubImage2D(renderCtx, GL_TEXTURE_2D, 0, 0, 0, uploadLevel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading texture data failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, width, height);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		checkFramebufferStatus(gl);

		blitStencilToColor2D(renderCtx, *depthStencilTex, width, height);
		glu::readPixels(renderCtx, 0, 0, readLevel);

		{
			const bool compareOk = compareStencilToRed(m_testCtx.getLog(), stencilOnlyLevel, readLevel);
			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

class BaseLevelCase : public TestCase
{
public:
	BaseLevelCase (Context& context, const char* name, deUint32 format)
		: TestCase	(context, name, glu::getTextureFormatName(format))
		, m_format	(format)
	{
	}

	IterateResult iterate (void)
	{
		const glu::RenderContext&	renderCtx			= m_context.getRenderContext();
		const glw::Functions&		gl					= renderCtx.getFunctions();
		const int					width				= 128;
		const int					height				= 128;
		const int					levelNdx			= 2;
		const int					levelWidth			= width>>levelNdx;
		const int					levelHeight			= height>>levelNdx;
		glu::Framebuffer			fbo					(renderCtx);
		glu::Renderbuffer			colorBuf			(renderCtx);
		glu::Texture				depthStencilTex		(renderCtx);
		TextureLevel				uploadLevel			(glu::mapGLInternalFormat(m_format), levelWidth, levelHeight);
		TextureLevel				readLevel			(TextureFormat(TextureFormat::RGBA, TextureFormat::UNSIGNED_INT32), levelWidth, levelHeight);
		TextureLevel				stencilOnlyLevel	(TextureFormat(TextureFormat::S, TextureFormat::UNSIGNED_INT8), levelWidth, levelHeight);

		checkDepthStencilFormatSupport(m_context, m_format);

		m_testCtx.getLog() << TestLog::Message << "GL_TEXTURE_BASE_LEVEL = " << levelNdx << TestLog::EndMessage;

		renderTestPatternReference(uploadLevel);
		renderTestPatternReference(stencilOnlyLevel);

		gl.bindTexture(GL_TEXTURE_2D, *depthStencilTex);
		gl.texStorage2D(GL_TEXTURE_2D, deLog2Floor32(de::max(width, height))+1, m_format, width, height);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, levelNdx);
		glu::texSubImage2D(renderCtx, GL_TEXTURE_2D, levelNdx, 0, 0, uploadLevel);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading texture data failed");

		gl.bindRenderbuffer(GL_RENDERBUFFER, *colorBuf);
		gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32UI, levelWidth, levelHeight);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, *colorBuf);
		checkFramebufferStatus(gl);

		blitStencilToColor2D(renderCtx, *depthStencilTex, levelWidth, levelHeight);
		glu::readPixels(renderCtx, 0, 0, readLevel);

		{
			const bool compareOk = compareStencilToRed(m_testCtx.getLog(), stencilOnlyLevel, readLevel);
			m_testCtx.setTestResult(compareOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
									compareOk ? "Pass"				: "Image comparison failed");
		}

		return STOP;
	}

private:
	const deUint32 m_format;
};

} // anonymous

StencilTexturingTests::StencilTexturingTests (Context& context)
	: TestCaseGroup(context, "stencil_texturing", "Stencil texturing tests")
{
}

StencilTexturingTests::~StencilTexturingTests (void)
{
}

void StencilTexturingTests::init (void)
{
	// .format
	{
		tcu::TestCaseGroup* const formatGroup = new tcu::TestCaseGroup(m_testCtx, "format", "Formats");
		addChild(formatGroup);

		formatGroup->addChild(new UploadTex2DCase		(m_context, "depth32f_stencil8_2d",			GL_DEPTH32F_STENCIL8));
		formatGroup->addChild(new UploadTex2DArrayCase	(m_context, "depth32f_stencil8_2d_array",	GL_DEPTH32F_STENCIL8));
		formatGroup->addChild(new UploadTexCubeCase		(m_context, "depth32f_stencil8_cube",		GL_DEPTH32F_STENCIL8));
		formatGroup->addChild(new UploadTex2DCase		(m_context, "depth24_stencil8_2d",			GL_DEPTH24_STENCIL8));
		formatGroup->addChild(new UploadTex2DArrayCase	(m_context, "depth24_stencil8_2d_array",	GL_DEPTH24_STENCIL8));
		formatGroup->addChild(new UploadTexCubeCase		(m_context, "depth24_stencil8_cube",		GL_DEPTH24_STENCIL8));

		// OES_texture_stencil8
		formatGroup->addChild(new UploadTex2DCase		(m_context, "stencil_index8_2d",			GL_STENCIL_INDEX8));
		formatGroup->addChild(new UploadTex2DArrayCase	(m_context, "stencil_index8_2d_array",		GL_STENCIL_INDEX8));
		formatGroup->addChild(new UploadTexCubeCase		(m_context, "stencil_index8_cube",			GL_STENCIL_INDEX8));
	}

	// .render
	{
		tcu::TestCaseGroup* const readRenderGroup = new tcu::TestCaseGroup(m_testCtx, "render", "Read rendered stencil values");
		addChild(readRenderGroup);

		readRenderGroup->addChild(new ClearTex2DCase	(m_context, "depth32f_stencil8_clear",	GL_DEPTH32F_STENCIL8));
		readRenderGroup->addChild(new RenderTex2DCase	(m_context, "depth32f_stencil8_draw",	GL_DEPTH32F_STENCIL8));
		readRenderGroup->addChild(new ClearTex2DCase	(m_context, "depth24_stencil8_clear",	GL_DEPTH24_STENCIL8));
		readRenderGroup->addChild(new RenderTex2DCase	(m_context, "depth24_stencil8_draw",	GL_DEPTH24_STENCIL8));
	}

	// .misc
	{
		tcu::TestCaseGroup* const miscGroup = new tcu::TestCaseGroup(m_testCtx, "misc", "Misc cases");
		addChild(miscGroup);

		miscGroup->addChild(new CompareModeCase	(m_context, "compare_mode_effect",	GL_DEPTH24_STENCIL8));
		miscGroup->addChild(new BaseLevelCase	(m_context, "base_level",			GL_DEPTH24_STENCIL8));
	}
}

} // Functional
} // gles31
} // deqp
