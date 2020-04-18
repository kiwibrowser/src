/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief Rendering tests for different config and api combinations.
 * \todo [2013-03-19 pyry] GLES1 and VG support.
 *//*--------------------------------------------------------------------*/

#include "teglRenderTests.hpp"
#include "teglRenderCase.hpp"

#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuSurface.hpp"

#include "egluDefs.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluShaderProgram.hpp"

#include "glwFunctions.hpp"
#include "glwEnums.hpp"

#include "deRandom.hpp"
#include "deSharedPtr.hpp"
#include "deSemaphore.hpp"
#include "deThread.hpp"
#include "deString.h"

#include "rrRenderer.hpp"
#include "rrFragmentOperations.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>

namespace deqp
{
namespace egl
{

using std::string;
using std::vector;
using std::set;

using tcu::Vec4;

using tcu::TestLog;

using namespace glw;
using namespace eglw;

static const tcu::Vec4	CLEAR_COLOR		= tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);
static const float		CLEAR_DEPTH		= 1.0f;
static const int		CLEAR_STENCIL	= 0;

namespace
{

enum PrimitiveType
{
	PRIMITIVETYPE_TRIANGLE = 0,	//!< Triangles, requires 3 coordinates per primitive
//	PRIMITIVETYPE_POINT,		//!< Points, requires 1 coordinate per primitive (w is used as size)
//	PRIMITIVETYPE_LINE,			//!< Lines, requires 2 coordinates per primitive

	PRIMITIVETYPE_LAST
};

enum BlendMode
{
	BLENDMODE_NONE = 0,			//!< No blending
	BLENDMODE_ADDITIVE,			//!< Blending with ONE, ONE
	BLENDMODE_SRC_OVER,			//!< Blending with SRC_ALPHA, ONE_MINUS_SRC_ALPHA

	BLENDMODE_LAST
};

enum DepthMode
{
	DEPTHMODE_NONE = 0,			//!< No depth test or depth writes
	DEPTHMODE_LESS,				//!< Depth test with less & depth write

	DEPTHMODE_LAST
};

enum StencilMode
{
	STENCILMODE_NONE = 0,		//!< No stencil test or write
	STENCILMODE_LEQUAL_INC,		//!< Stencil test with LEQUAL, increment on pass

	STENCILMODE_LAST
};

struct DrawPrimitiveOp
{
	PrimitiveType	type;
	int				count;
	vector<Vec4>	positions;
	vector<Vec4>	colors;
	BlendMode		blend;
	DepthMode		depth;
	StencilMode		stencil;
	int				stencilRef;
};

static bool isANarrowScreenSpaceTriangle (const tcu::Vec4& p0, const tcu::Vec4& p1, const tcu::Vec4& p2)
{
	// to clip space
	const tcu::Vec2	csp0				= p0.swizzle(0, 1) / p0.w();
	const tcu::Vec2	csp1				= p1.swizzle(0, 1) / p1.w();
	const tcu::Vec2	csp2				= p2.swizzle(0, 1) / p2.w();

	const tcu::Vec2	e01					= (csp1 - csp0);
	const tcu::Vec2	e02					= (csp2 - csp0);

	const float		minimumVisibleArea	= 0.4f; // must cover at least 10% of the surface
	const float		visibleArea			= de::abs(e01.x() * e02.y() - e02.x() * e01.y()) * 0.5f;

	return visibleArea < minimumVisibleArea;
}

void randomizeDrawOp (de::Random& rnd, DrawPrimitiveOp& drawOp)
{
	const int	minStencilRef	= 0;
	const int	maxStencilRef	= 8;
	const int	minPrimitives	= 2;
	const int	maxPrimitives	= 4;

	const float	maxTriOffset	= 1.0f;
	const float	minDepth		= -1.0f; // \todo [pyry] Reference doesn't support Z clipping yet
	const float	maxDepth		= 1.0f;

	const float	minRGB			= 0.2f;
	const float	maxRGB			= 0.9f;
	const float	minAlpha		= 0.3f;
	const float	maxAlpha		= 1.0f;

	drawOp.type			= (PrimitiveType)rnd.getInt(0, PRIMITIVETYPE_LAST-1);
	drawOp.count		= rnd.getInt(minPrimitives, maxPrimitives);
	drawOp.blend		= (BlendMode)rnd.getInt(0, BLENDMODE_LAST-1);
	drawOp.depth		= (DepthMode)rnd.getInt(0, DEPTHMODE_LAST-1);
	drawOp.stencil		= (StencilMode)rnd.getInt(0, STENCILMODE_LAST-1);
	drawOp.stencilRef	= rnd.getInt(minStencilRef, maxStencilRef);

	if (drawOp.type == PRIMITIVETYPE_TRIANGLE)
	{
		drawOp.positions.resize(drawOp.count*3);
		drawOp.colors.resize(drawOp.count*3);

		for (int triNdx = 0; triNdx < drawOp.count; triNdx++)
		{
			const float		cx		= rnd.getFloat(-1.0f, 1.0f);
			const float		cy		= rnd.getFloat(-1.0f, 1.0f);

			for (int coordNdx = 0; coordNdx < 3; coordNdx++)
			{
				tcu::Vec4&	position	= drawOp.positions[triNdx*3 + coordNdx];
				tcu::Vec4&	color		= drawOp.colors[triNdx*3 + coordNdx];

				position.x()	= cx + rnd.getFloat(-maxTriOffset, maxTriOffset);
				position.y()	= cy + rnd.getFloat(-maxTriOffset, maxTriOffset);
				position.z()	= rnd.getFloat(minDepth, maxDepth);
				position.w()	= 1.0f;

				color.x()		= rnd.getFloat(minRGB, maxRGB);
				color.y()		= rnd.getFloat(minRGB, maxRGB);
				color.z()		= rnd.getFloat(minRGB, maxRGB);
				color.w()		= rnd.getFloat(minAlpha, maxAlpha);
			}

			// avoid generating narrow triangles
			{
				const int	maxAttempts	= 40;
				int			numAttempts	= 0;
				tcu::Vec4&	p0			= drawOp.positions[triNdx*3 + 0];
				tcu::Vec4&	p1			= drawOp.positions[triNdx*3 + 1];
				tcu::Vec4&	p2			= drawOp.positions[triNdx*3 + 2];

				while (isANarrowScreenSpaceTriangle(p0, p1, p2))
				{
					p1.x()	= cx + rnd.getFloat(-maxTriOffset, maxTriOffset);
					p1.y()	= cy + rnd.getFloat(-maxTriOffset, maxTriOffset);
					p1.z()	= rnd.getFloat(minDepth, maxDepth);
					p1.w()	= 1.0f;

					p2.x()	= cx + rnd.getFloat(-maxTriOffset, maxTriOffset);
					p2.y()	= cy + rnd.getFloat(-maxTriOffset, maxTriOffset);
					p2.z()	= rnd.getFloat(minDepth, maxDepth);
					p2.w()	= 1.0f;

					if (++numAttempts > maxAttempts)
					{
						DE_ASSERT(false);
						break;
					}
				}
			}
		}
	}
	else
		DE_ASSERT(false);
}

// Reference rendering code

class ReferenceShader : public rr::VertexShader, public rr::FragmentShader
{
public:
	enum
	{
		VaryingLoc_Color = 0
	};

	ReferenceShader ()
		: rr::VertexShader	(2, 1)		// color and pos in => color out
		, rr::FragmentShader(1, 1)		// color in => color out
	{
		this->rr::VertexShader::m_inputs[0].type		= rr::GENERICVECTYPE_FLOAT;
		this->rr::VertexShader::m_inputs[1].type		= rr::GENERICVECTYPE_FLOAT;

		this->rr::VertexShader::m_outputs[0].type		= rr::GENERICVECTYPE_FLOAT;
		this->rr::VertexShader::m_outputs[0].flatshade	= false;

		this->rr::FragmentShader::m_inputs[0].type		= rr::GENERICVECTYPE_FLOAT;
		this->rr::FragmentShader::m_inputs[0].flatshade	= false;

		this->rr::FragmentShader::m_outputs[0].type		= rr::GENERICVECTYPE_FLOAT;
	}

	void shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			const int positionAttrLoc = 0;
			const int colorAttrLoc = 1;

			rr::VertexPacket& packet = *packets[packetNdx];

			// Transform to position
			packet.position = rr::readVertexAttribFloat(inputs[positionAttrLoc], packet.instanceNdx, packet.vertexNdx);

			// Pass color to FS
			packet.outputs[VaryingLoc_Color] = rr::readVertexAttribFloat(inputs[colorAttrLoc], packet.instanceNdx, packet.vertexNdx);
		}
	}

	void shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			rr::FragmentPacket& packet = packets[packetNdx];

			for (int fragNdx = 0; fragNdx < 4; ++fragNdx)
				rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, rr::readVarying<float>(packet, context, VaryingLoc_Color, fragNdx));
		}
	}
};

void toReferenceRenderState (rr::RenderState& state, const DrawPrimitiveOp& drawOp)
{
	state.cullMode	= rr::CULLMODE_NONE;

	if (drawOp.blend != BLENDMODE_NONE)
	{
		state.fragOps.blendMode = rr::BLENDMODE_STANDARD;

		switch (drawOp.blend)
		{
			case BLENDMODE_ADDITIVE:
				state.fragOps.blendRGBState.srcFunc		= rr::BLENDFUNC_ONE;
				state.fragOps.blendRGBState.dstFunc		= rr::BLENDFUNC_ONE;
				state.fragOps.blendRGBState.equation	= rr::BLENDEQUATION_ADD;
				state.fragOps.blendAState				= state.fragOps.blendRGBState;
				break;

			case BLENDMODE_SRC_OVER:
				state.fragOps.blendRGBState.srcFunc		= rr::BLENDFUNC_SRC_ALPHA;
				state.fragOps.blendRGBState.dstFunc		= rr::BLENDFUNC_ONE_MINUS_SRC_ALPHA;
				state.fragOps.blendRGBState.equation	= rr::BLENDEQUATION_ADD;
				state.fragOps.blendAState				= state.fragOps.blendRGBState;
				break;

			default:
				DE_ASSERT(false);
		}
	}

	if (drawOp.depth != DEPTHMODE_NONE)
	{
		state.fragOps.depthTestEnabled = true;

		DE_ASSERT(drawOp.depth == DEPTHMODE_LESS);
		state.fragOps.depthFunc = rr::TESTFUNC_LESS;
	}

	if (drawOp.stencil != STENCILMODE_NONE)
	{
		state.fragOps.stencilTestEnabled = true;

		DE_ASSERT(drawOp.stencil == STENCILMODE_LEQUAL_INC);
		state.fragOps.stencilStates[0].func		= rr::TESTFUNC_LEQUAL;
		state.fragOps.stencilStates[0].sFail	= rr::STENCILOP_KEEP;
		state.fragOps.stencilStates[0].dpFail	= rr::STENCILOP_INCR;
		state.fragOps.stencilStates[0].dpPass	= rr::STENCILOP_INCR;
		state.fragOps.stencilStates[0].ref		= drawOp.stencilRef;
		state.fragOps.stencilStates[1]			= state.fragOps.stencilStates[0];
	}
}

tcu::TextureFormat getColorFormat (const tcu::PixelFormat& colorBits)
{
	using tcu::TextureFormat;

	DE_ASSERT(de::inBounds(colorBits.redBits,	0, 0xff) &&
			  de::inBounds(colorBits.greenBits,	0, 0xff) &&
			  de::inBounds(colorBits.blueBits,	0, 0xff) &&
			  de::inBounds(colorBits.alphaBits,	0, 0xff));

#define PACK_FMT(R, G, B, A) (((R) << 24) | ((G) << 16) | ((B) << 8) | (A))

	// \note [pyry] This may not hold true on some implementations - best effort guess only.
	switch (PACK_FMT(colorBits.redBits, colorBits.greenBits, colorBits.blueBits, colorBits.alphaBits))
	{
		case PACK_FMT(8,8,8,8):		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
		case PACK_FMT(8,8,8,0):		return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_INT8);
		case PACK_FMT(4,4,4,4):		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_4444);
		case PACK_FMT(5,5,5,1):		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_SHORT_5551);
		case PACK_FMT(5,6,5,0):		return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_SHORT_565);

		// \note Defaults to RGBA8
		default:					return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
	}

#undef PACK_FMT
}

/*
The getColorThreshold function is used to obtain a
threshold usable for the fuzzyCompare function.

For 8bit color depths a value of 0.02 should provide
a good metric for rejecting images above this level.
For other bit depths other thresholds should be selected.
Ideally this function would take advantage of the
getColorThreshold function provided by the PixelFormat class
as this would also allow setting per channel thresholds.
However using the PixelFormat provided function can result
in too strict thresholds for 8bit bit depths (compared to
the current default of 0.02) or too relaxed for lower bit
depths if scaled proportionally to the 8bit default.
*/

float getColorThreshold (const tcu::PixelFormat& colorBits)
{
	if ((colorBits.redBits > 0 && colorBits.redBits < 8) ||
		(colorBits.greenBits > 0 && colorBits.greenBits < 8) ||
		(colorBits.blueBits > 0 && colorBits.blueBits < 8) ||
		(colorBits.alphaBits > 0 && colorBits.alphaBits < 8))
	{
		return 0.05f;
	}
	else
	{
		return 0.02f;
	}
}

tcu::TextureFormat getDepthFormat (const int depthBits)
{
	switch (depthBits)
	{
		case 0:		return tcu::TextureFormat();
		case 8:		return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::UNORM_INT8);
		case 16:	return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::UNORM_INT16);
		case 24:	return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::UNORM_INT24);
		case 32:
		default:	return tcu::TextureFormat(tcu::TextureFormat::D, tcu::TextureFormat::FLOAT);
	}
}

tcu::TextureFormat getStencilFormat (int stencilBits)
{
	switch (stencilBits)
	{
		case 0:		return tcu::TextureFormat();
		case 8:
		default:	return tcu::TextureFormat(tcu::TextureFormat::S, tcu::TextureFormat::UNSIGNED_INT8);
	}
}

void renderReference (const tcu::PixelBufferAccess& dst, const vector<DrawPrimitiveOp>& drawOps, const tcu::PixelFormat& colorBits, const int depthBits, const int stencilBits, const int numSamples)
{
	const int						width			= dst.getWidth();
	const int						height			= dst.getHeight();

	tcu::TextureLevel				colorBuffer;
	tcu::TextureLevel				depthBuffer;
	tcu::TextureLevel				stencilBuffer;

	rr::Renderer					referenceRenderer;
	rr::VertexAttrib				attributes[2];
	const ReferenceShader			shader;

	attributes[0].type				= rr::VERTEXATTRIBTYPE_FLOAT;
	attributes[0].size				= 4;
	attributes[0].stride			= 0;
	attributes[0].instanceDivisor	= 0;

	attributes[1].type				= rr::VERTEXATTRIBTYPE_FLOAT;
	attributes[1].size				= 4;
	attributes[1].stride			= 0;
	attributes[1].instanceDivisor	= 0;

	// Initialize buffers.
	colorBuffer.setStorage(getColorFormat(colorBits), numSamples, width, height);
	rr::clearMultisampleColorBuffer(colorBuffer, CLEAR_COLOR, rr::WindowRectangle(0, 0, width, height));

	if (depthBits > 0)
	{
		depthBuffer.setStorage(getDepthFormat(depthBits), numSamples, width, height);
		rr::clearMultisampleDepthBuffer(depthBuffer, CLEAR_DEPTH, rr::WindowRectangle(0, 0, width, height));
	}

	if (stencilBits > 0)
	{
		stencilBuffer.setStorage(getStencilFormat(stencilBits), numSamples, width, height);
		rr::clearMultisampleStencilBuffer(stencilBuffer, CLEAR_STENCIL, rr::WindowRectangle(0, 0, width, height));
	}

	const rr::RenderTarget renderTarget(rr::MultisamplePixelBufferAccess::fromMultisampleAccess(colorBuffer.getAccess()),
										rr::MultisamplePixelBufferAccess::fromMultisampleAccess(depthBuffer.getAccess()),
										rr::MultisamplePixelBufferAccess::fromMultisampleAccess(stencilBuffer.getAccess()));

	for (vector<DrawPrimitiveOp>::const_iterator drawOp = drawOps.begin(); drawOp != drawOps.end(); drawOp++)
	{
		// Translate state
		rr::RenderState renderState((rr::ViewportState)(rr::MultisamplePixelBufferAccess::fromMultisampleAccess(colorBuffer.getAccess())));
		toReferenceRenderState(renderState, *drawOp);

		DE_ASSERT(drawOp->type == PRIMITIVETYPE_TRIANGLE);

		attributes[0].pointer = &drawOp->positions[0];
		attributes[1].pointer = &drawOp->colors[0];

		referenceRenderer.draw(
			rr::DrawCommand(
				renderState,
				renderTarget,
				rr::Program(static_cast<const rr::VertexShader*>(&shader), static_cast<const rr::FragmentShader*>(&shader)),
				2,
				attributes,
				rr::PrimitiveList(rr::PRIMITIVETYPE_TRIANGLES, drawOp->count * 3, 0)));
	}

	rr::resolveMultisampleColorBuffer(dst, rr::MultisamplePixelBufferAccess::fromMultisampleAccess(colorBuffer.getAccess()));
}

// API rendering code

class Program
{
public:
					Program				(void) {}
	virtual			~Program			(void) {}

	virtual void	setup				(void) const = DE_NULL;
};

typedef de::SharedPtr<Program> ProgramSp;

static glu::ProgramSources getProgramSourcesES2 (void)
{
	static const char* s_vertexSrc =
		"attribute highp vec4 a_position;\n"
		"attribute mediump vec4 a_color;\n"
		"varying mediump vec4 v_color;\n"
		"void main (void)\n"
		"{\n"
		"	gl_Position = a_position;\n"
		"	v_color = a_color;\n"
		"}\n";

	static const char* s_fragmentSrc =
		"varying mediump vec4 v_color;\n"
		"void main (void)\n"
		"{\n"
		"	gl_FragColor = v_color;\n"
		"}\n";

	return glu::ProgramSources() << glu::VertexSource(s_vertexSrc) << glu::FragmentSource(s_fragmentSrc);
}

class GLES2Program : public Program
{
public:
	GLES2Program (const glw::Functions& gl)
		: m_gl				(gl)
		, m_program			(gl, getProgramSourcesES2())
		, m_positionLoc		(0)
		, m_colorLoc		(0)
	{

		m_positionLoc	= m_gl.getAttribLocation(m_program.getProgram(), "a_position");
		m_colorLoc		= m_gl.getAttribLocation(m_program.getProgram(), "a_color");
	}

	~GLES2Program (void)
	{
	}

	void setup (void) const
	{
		m_gl.useProgram(m_program.getProgram());
		m_gl.enableVertexAttribArray(m_positionLoc);
		m_gl.enableVertexAttribArray(m_colorLoc);
		GLU_CHECK_GLW_MSG(m_gl, "Program setup failed");
	}

	int						getPositionLoc		(void) const { return m_positionLoc;	}
	int						getColorLoc			(void) const { return m_colorLoc;		}

private:
	const glw::Functions&	m_gl;
	glu::ShaderProgram		m_program;
	int						m_positionLoc;
	int						m_colorLoc;
};

void clearGLES2 (const glw::Functions& gl, const tcu::Vec4& color, const float depth, const int stencil)
{
	gl.clearColor(color.x(), color.y(), color.z(), color.w());
	gl.clearDepthf(depth);
	gl.clearStencil(stencil);
	gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

void drawGLES2 (const glw::Functions& gl, const Program& program, const DrawPrimitiveOp& drawOp)
{
	const GLES2Program& gles2Program = dynamic_cast<const GLES2Program&>(program);

	switch (drawOp.blend)
	{
		case BLENDMODE_NONE:
			gl.disable(GL_BLEND);
			break;

		case BLENDMODE_ADDITIVE:
			gl.enable(GL_BLEND);
			gl.blendFunc(GL_ONE, GL_ONE);
			break;

		case BLENDMODE_SRC_OVER:
			gl.enable(GL_BLEND);
			gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		default:
			DE_ASSERT(false);
	}

	switch (drawOp.depth)
	{
		case DEPTHMODE_NONE:
			gl.disable(GL_DEPTH_TEST);
			break;

		case DEPTHMODE_LESS:
			gl.enable(GL_DEPTH_TEST);
			break;

		default:
			DE_ASSERT(false);
	}

	switch (drawOp.stencil)
	{
		case STENCILMODE_NONE:
			gl.disable(GL_STENCIL_TEST);
			break;

		case STENCILMODE_LEQUAL_INC:
			gl.enable(GL_STENCIL_TEST);
			gl.stencilFunc(GL_LEQUAL, drawOp.stencilRef, ~0u);
			gl.stencilOp(GL_KEEP, GL_INCR, GL_INCR);
			break;

		default:
			DE_ASSERT(false);
	}

	gl.disable(GL_DITHER);

	gl.vertexAttribPointer(gles2Program.getPositionLoc(), 4, GL_FLOAT, GL_FALSE, 0, &drawOp.positions[0]);
	gl.vertexAttribPointer(gles2Program.getColorLoc(), 4, GL_FLOAT, GL_FALSE, 0, &drawOp.colors[0]);

	DE_ASSERT(drawOp.type == PRIMITIVETYPE_TRIANGLE);
	gl.drawArrays(GL_TRIANGLES, 0, drawOp.count*3);
}

static void readPixelsGLES2 (const glw::Functions& gl, tcu::Surface& dst)
{
	gl.readPixels(0, 0, dst.getWidth(), dst.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, dst.getAccess().getDataPtr());
}

Program* createProgram (const glw::Functions& gl, EGLint api)
{
	switch (api)
	{
		case EGL_OPENGL_ES2_BIT:		return new GLES2Program(gl);
		case EGL_OPENGL_ES3_BIT_KHR:	return new GLES2Program(gl);
		default:
			throw tcu::NotSupportedError("Unsupported API");
	}
}

void draw (const glw::Functions& gl, EGLint api, const Program& program, const DrawPrimitiveOp& drawOp)
{
	switch (api)
	{
		case EGL_OPENGL_ES2_BIT:		drawGLES2(gl, program, drawOp);		break;
		case EGL_OPENGL_ES3_BIT_KHR:	drawGLES2(gl, program, drawOp);		break;
		default:
			throw tcu::NotSupportedError("Unsupported API");
	}
}

void clear (const glw::Functions& gl, EGLint api, const tcu::Vec4& color, const float depth, const int stencil)
{
	switch (api)
	{
		case EGL_OPENGL_ES2_BIT:		clearGLES2(gl, color, depth, stencil);		break;
		case EGL_OPENGL_ES3_BIT_KHR:	clearGLES2(gl, color, depth, stencil);		break;
		default:
			throw tcu::NotSupportedError("Unsupported API");
	}
}

static void readPixels (const glw::Functions& gl, EGLint api, tcu::Surface& dst)
{
	switch (api)
	{
		case EGL_OPENGL_ES2_BIT:		readPixelsGLES2(gl, dst);		break;
		case EGL_OPENGL_ES3_BIT_KHR:	readPixelsGLES2(gl, dst);		break;
		default:
			throw tcu::NotSupportedError("Unsupported API");
	}
}

static void finish (const glw::Functions& gl, EGLint api)
{
	switch (api)
	{
		case EGL_OPENGL_ES2_BIT:
		case EGL_OPENGL_ES3_BIT_KHR:
			gl.finish();
			break;

		default:
			throw tcu::NotSupportedError("Unsupported API");
	}
}

tcu::PixelFormat getPixelFormat (const Library& egl, EGLDisplay display, EGLConfig config)
{
	tcu::PixelFormat fmt;
	fmt.redBits		= eglu::getConfigAttribInt(egl, display, config, EGL_RED_SIZE);
	fmt.greenBits	= eglu::getConfigAttribInt(egl, display, config, EGL_GREEN_SIZE);
	fmt.blueBits	= eglu::getConfigAttribInt(egl, display, config, EGL_BLUE_SIZE);
	fmt.alphaBits	= eglu::getConfigAttribInt(egl, display, config, EGL_ALPHA_SIZE);
	return fmt;
}

} // anonymous

// SingleThreadRenderCase

class SingleThreadRenderCase : public MultiContextRenderCase
{
public:
						SingleThreadRenderCase		(EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi);

	void				init						(void);

private:
	virtual void		executeForContexts			(EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts);

	glw::Functions		m_gl;
};

// SingleThreadColorClearCase

SingleThreadRenderCase::SingleThreadRenderCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi)
	: MultiContextRenderCase(eglTestCtx, name, description, api, surfaceType, filters, numContextsPerApi)
{
}

void SingleThreadRenderCase::init (void)
{
	MultiContextRenderCase::init();
	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

void SingleThreadRenderCase::executeForContexts (EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts)
{
	const Library&			egl			= m_eglTestCtx.getLibrary();
	const int				width		= eglu::querySurfaceInt(egl, display, surface, EGL_WIDTH);
	const int				height		= eglu::querySurfaceInt(egl, display, surface, EGL_HEIGHT);
	const int				numContexts	= (int)contexts.size();
	const int				drawsPerCtx	= 2;
	const int				numIters	= 2;
	const tcu::PixelFormat	pixelFmt	= getPixelFormat(egl, display, config.config);
	const float				threshold	= getColorThreshold(pixelFmt);

	const int				depthBits	= eglu::getConfigAttribInt(egl, display, config.config, EGL_DEPTH_SIZE);
	const int				stencilBits	= eglu::getConfigAttribInt(egl, display, config.config, EGL_STENCIL_SIZE);
	const int				numSamples	= eglu::getConfigAttribInt(egl, display, config.config, EGL_SAMPLES);

	TestLog&				log			= m_testCtx.getLog();

	tcu::Surface			refFrame	(width, height);
	tcu::Surface			frame		(width, height);

	de::Random				rnd			(deStringHash(getName()) ^ deInt32Hash(numContexts));
	vector<ProgramSp>		programs	(contexts.size());
	vector<DrawPrimitiveOp>	drawOps;

	// Log basic information about config.
	log << TestLog::Message << "EGL_RED_SIZE = "		<< pixelFmt.redBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_GREEN_SIZE = "		<< pixelFmt.greenBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_BLUE_SIZE = "		<< pixelFmt.blueBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_ALPHA_SIZE = "		<< pixelFmt.alphaBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_DEPTH_SIZE = "		<< depthBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_STENCIL_SIZE = "	<< stencilBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_SAMPLES = "			<< numSamples << TestLog::EndMessage;

	// Generate draw ops.
	drawOps.resize(numContexts*drawsPerCtx*numIters);
	for (vector<DrawPrimitiveOp>::iterator drawOp = drawOps.begin(); drawOp != drawOps.end(); ++drawOp)
		randomizeDrawOp(rnd, *drawOp);

	// Create and setup programs per context
	for (int ctxNdx = 0; ctxNdx < numContexts; ctxNdx++)
	{
		EGLint		api			= contexts[ctxNdx].first;
		EGLContext	context		= contexts[ctxNdx].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		programs[ctxNdx] = ProgramSp(createProgram(m_gl, api));
		programs[ctxNdx]->setup();
	}

	// Clear to black using first context.
	{
		EGLint		api			= contexts[0].first;
		EGLContext	context		= contexts[0].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		clear(m_gl, api, CLEAR_COLOR, CLEAR_DEPTH, CLEAR_STENCIL);
		finish(m_gl, api);
	}

	// Render.
	for (int iterNdx = 0; iterNdx < numIters; iterNdx++)
	{
		for (int ctxNdx = 0; ctxNdx < numContexts; ctxNdx++)
		{
			EGLint		api			= contexts[ctxNdx].first;
			EGLContext	context		= contexts[ctxNdx].second;

			EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

			for (int drawNdx = 0; drawNdx < drawsPerCtx; drawNdx++)
			{
				const DrawPrimitiveOp& drawOp = drawOps[iterNdx*numContexts*drawsPerCtx + ctxNdx*drawsPerCtx + drawNdx];
				draw(m_gl, api, *programs[ctxNdx], drawOp);
			}

			finish(m_gl, api);
		}
	}

	// Read pixels using first context. \todo [pyry] Randomize?
	{
		EGLint		api		= contexts[0].first;
		EGLContext	context	= contexts[0].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		readPixels(m_gl, api, frame);
	}

	EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

	// Render reference.
	// \note Reference image is always generated using single-sampling.
	renderReference(refFrame.getAccess(), drawOps, pixelFmt, depthBits, stencilBits, 1);

	// Compare images
	{
		bool imagesOk = tcu::fuzzyCompare(log, "ComparisonResult", "Image comparison result", refFrame, frame, threshold, tcu::COMPARE_LOG_RESULT);

		if (!imagesOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}
}

// MultiThreadRenderCase

class MultiThreadRenderCase : public MultiContextRenderCase
{
public:
						MultiThreadRenderCase		(EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi);

	void				init						(void);

private:
	virtual void		executeForContexts			(EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts);

	glw::Functions		m_gl;
};

class RenderTestThread;

typedef de::SharedPtr<RenderTestThread>	RenderTestThreadSp;
typedef de::SharedPtr<de::Semaphore>	SemaphoreSp;

struct DrawOpPacket
{
	DrawOpPacket (void)
		: drawOps	(DE_NULL)
		, numOps	(0)
	{
	}

	const DrawPrimitiveOp*	drawOps;
	int						numOps;
	SemaphoreSp				wait;
	SemaphoreSp				signal;
};

class RenderTestThread : public de::Thread
{
public:
	RenderTestThread (const Library& egl, EGLDisplay display, EGLSurface surface, EGLContext context, EGLint api, const glw::Functions& gl, const Program& program, const std::vector<DrawOpPacket>& packets)
		: m_egl		(egl)
		, m_display	(display)
		, m_surface	(surface)
		, m_context	(context)
		, m_api		(api)
		, m_gl		(gl)
		, m_program	(program)
		, m_packets	(packets)
	{
	}

	void run (void)
	{
		for (std::vector<DrawOpPacket>::const_iterator packetIter = m_packets.begin(); packetIter != m_packets.end(); packetIter++)
		{
			// Wait until it is our turn.
			packetIter->wait->decrement();

			// Acquire context.
			EGLU_CHECK_CALL(m_egl, makeCurrent(m_display, m_surface, m_surface, m_context));

			// Execute rendering.
			for (int ndx = 0; ndx < packetIter->numOps; ndx++)
				draw(m_gl, m_api, m_program, packetIter->drawOps[ndx]);

			finish(m_gl, m_api);

			// Release context.
			EGLU_CHECK_CALL(m_egl, makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

			// Signal completion.
			packetIter->signal->increment();
		}
		m_egl.releaseThread();
	}

private:
	const Library&						m_egl;
	EGLDisplay							m_display;
	EGLSurface							m_surface;
	EGLContext							m_context;
	EGLint								m_api;
	const glw::Functions&				m_gl;
	const Program&						m_program;
	const std::vector<DrawOpPacket>&	m_packets;
};

MultiThreadRenderCase::MultiThreadRenderCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLint api, EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi)
	: MultiContextRenderCase(eglTestCtx, name, description, api, surfaceType, filters, numContextsPerApi)
{
}

void MultiThreadRenderCase::init (void)
{
	MultiContextRenderCase::init();
	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

void MultiThreadRenderCase::executeForContexts (EGLDisplay display, EGLSurface surface, const Config& config, const std::vector<std::pair<EGLint, EGLContext> >& contexts)
{
	const Library&			egl					= m_eglTestCtx.getLibrary();
	const int				width				= eglu::querySurfaceInt(egl, display, surface, EGL_WIDTH);
	const int				height				= eglu::querySurfaceInt(egl, display, surface, EGL_HEIGHT);
	const int				numContexts			= (int)contexts.size();
	const int				opsPerPacket		= 2;
	const int				packetsPerThread	= 2;
	const int				numThreads			= numContexts;
	const int				numPackets			= numThreads * packetsPerThread;
	const tcu::PixelFormat	pixelFmt			= getPixelFormat(egl, display, config.config);
	const float				threshold			= getColorThreshold(pixelFmt);

	const int				depthBits			= eglu::getConfigAttribInt(egl, display, config.config, EGL_DEPTH_SIZE);
	const int				stencilBits			= eglu::getConfigAttribInt(egl, display, config.config, EGL_STENCIL_SIZE);
	const int				numSamples			= eglu::getConfigAttribInt(egl, display, config.config, EGL_SAMPLES);

	TestLog&				log					= m_testCtx.getLog();

	tcu::Surface			refFrame			(width, height);
	tcu::Surface			frame				(width, height);

	de::Random				rnd					(deStringHash(getName()) ^ deInt32Hash(numContexts));

	// Resources that need cleanup
	vector<ProgramSp>				programs	(numContexts);
	vector<SemaphoreSp>				semaphores	(numPackets+1);
	vector<DrawPrimitiveOp>			drawOps		(numPackets*opsPerPacket);
	vector<vector<DrawOpPacket> >	packets		(numThreads);
	vector<RenderTestThreadSp>		threads		(numThreads);

	// Log basic information about config.
	log << TestLog::Message << "EGL_RED_SIZE = "		<< pixelFmt.redBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_GREEN_SIZE = "		<< pixelFmt.greenBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_BLUE_SIZE = "		<< pixelFmt.blueBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_ALPHA_SIZE = "		<< pixelFmt.alphaBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_DEPTH_SIZE = "		<< depthBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_STENCIL_SIZE = "	<< stencilBits << TestLog::EndMessage;
	log << TestLog::Message << "EGL_SAMPLES = "			<< numSamples << TestLog::EndMessage;

	// Initialize semaphores.
	for (vector<SemaphoreSp>::iterator sem = semaphores.begin(); sem != semaphores.end(); ++sem)
		*sem = SemaphoreSp(new de::Semaphore(0));

	// Create draw ops.
	for (vector<DrawPrimitiveOp>::iterator drawOp = drawOps.begin(); drawOp != drawOps.end(); ++drawOp)
		randomizeDrawOp(rnd, *drawOp);

	// Create packets.
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		packets[threadNdx].resize(packetsPerThread);

		for (int packetNdx = 0; packetNdx < packetsPerThread; packetNdx++)
		{
			DrawOpPacket& packet = packets[threadNdx][packetNdx];

			// Threads take turns with packets.
			packet.wait		= semaphores[packetNdx*numThreads + threadNdx];
			packet.signal	= semaphores[packetNdx*numThreads + threadNdx + 1];
			packet.numOps	= opsPerPacket;
			packet.drawOps	= &drawOps[(packetNdx*numThreads + threadNdx)*opsPerPacket];
		}
	}

	// Create and setup programs per context
	for (int ctxNdx = 0; ctxNdx < numContexts; ctxNdx++)
	{
		EGLint		api			= contexts[ctxNdx].first;
		EGLContext	context		= contexts[ctxNdx].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		programs[ctxNdx] = ProgramSp(createProgram(m_gl, api));
		programs[ctxNdx]->setup();

		// Release context
		EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	}

	// Clear to black using first context.
	{
		EGLint		api			= contexts[0].first;
		EGLContext	context		= contexts[0].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		clear(m_gl, api, CLEAR_COLOR, CLEAR_DEPTH, CLEAR_STENCIL);
		finish(m_gl, api);

		// Release context
		EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	}

	// Create and launch threads (actual rendering starts once first semaphore is signaled).
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
	{
		threads[threadNdx] = RenderTestThreadSp(new RenderTestThread(egl, display, surface, contexts[threadNdx].second, contexts[threadNdx].first, m_gl, *programs[threadNdx], packets[threadNdx]));
		threads[threadNdx]->start();
	}

	// Signal start and wait until complete.
	semaphores.front()->increment();
	semaphores.back()->decrement();

	// Read pixels using first context. \todo [pyry] Randomize?
	{
		EGLint		api		= contexts[0].first;
		EGLContext	context	= contexts[0].second;

		EGLU_CHECK_CALL(egl, makeCurrent(display, surface, surface, context));

		readPixels(m_gl, api, frame);
	}

	EGLU_CHECK_CALL(egl, makeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));

	// Join threads.
	for (int threadNdx = 0; threadNdx < numThreads; threadNdx++)
		threads[threadNdx]->join();

	// Render reference.
	renderReference(refFrame.getAccess(), drawOps, pixelFmt, depthBits, stencilBits, 1);

	// Compare images
	{
		bool imagesOk = tcu::fuzzyCompare(log, "ComparisonResult", "Image comparison result", refFrame, frame, threshold, tcu::COMPARE_LOG_RESULT);

		if (!imagesOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}
}

RenderTests::RenderTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "render", "Basic rendering with different client APIs")
{
}

RenderTests::~RenderTests (void)
{
}

struct RenderGroupSpec
{
	const char*			name;
	const char*			desc;
	EGLint				apiBits;
	eglu::ConfigFilter	baseFilter;
	int					numContextsPerApi;
};

template <deUint32 Bits>
static bool renderable (const eglu::CandidateConfig& c)
{
	return (c.renderableType() & Bits) == Bits;
}

template <class RenderClass>
static void createRenderGroups (EglTestContext& eglTestCtx, tcu::TestCaseGroup* group, const RenderGroupSpec* first, const RenderGroupSpec* last)
{
	for (const RenderGroupSpec* groupIter = first; groupIter != last; groupIter++)
	{
		tcu::TestCaseGroup* configGroup = new tcu::TestCaseGroup(eglTestCtx.getTestContext(), groupIter->name, groupIter->desc);
		group->addChild(configGroup);

		vector<RenderFilterList>	filterLists;
		eglu::FilterList			baseFilters;
		baseFilters << groupIter->baseFilter;
		getDefaultRenderFilterLists(filterLists, baseFilters);

		for (vector<RenderFilterList>::const_iterator listIter = filterLists.begin(); listIter != filterLists.end(); listIter++)
			configGroup->addChild(new RenderClass(eglTestCtx, listIter->getName(), "", groupIter->apiBits, listIter->getSurfaceTypeMask(), *listIter, groupIter->numContextsPerApi));
	}
}

void RenderTests::init (void)
{
	static const RenderGroupSpec singleContextCases[] =
	{
		{
			"gles2",
			"Primitive rendering using GLES2",
			EGL_OPENGL_ES2_BIT,
			renderable<EGL_OPENGL_ES2_BIT>,
			1
		},
		{
			"gles3",
			"Primitive rendering using GLES3",
			EGL_OPENGL_ES3_BIT,
			renderable<EGL_OPENGL_ES3_BIT>,
			1
		},
	};

	static const RenderGroupSpec multiContextCases[] =
	{
		{
			"gles2",
			"Primitive rendering using multiple GLES2 contexts to shared surface",
			EGL_OPENGL_ES2_BIT,
			renderable<EGL_OPENGL_ES2_BIT>,
			3
		},
		{
			"gles3",
			"Primitive rendering using multiple GLES3 contexts to shared surface",
			EGL_OPENGL_ES3_BIT,
			renderable<EGL_OPENGL_ES3_BIT>,
			3
		},
		{
			"gles2_gles3",
			"Primitive rendering using multiple APIs to shared surface",
			EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT,
			renderable<EGL_OPENGL_ES2_BIT|EGL_OPENGL_ES3_BIT>,
			1
		},
	};

	tcu::TestCaseGroup* singleContextGroup = new tcu::TestCaseGroup(m_testCtx, "single_context", "Single-context rendering");
	addChild(singleContextGroup);
	createRenderGroups<SingleThreadRenderCase>(m_eglTestCtx, singleContextGroup, &singleContextCases[0], &singleContextCases[DE_LENGTH_OF_ARRAY(singleContextCases)]);

	tcu::TestCaseGroup* multiContextGroup = new tcu::TestCaseGroup(m_testCtx, "multi_context", "Multi-context rendering with shared surface");
	addChild(multiContextGroup);
	createRenderGroups<SingleThreadRenderCase>(m_eglTestCtx, multiContextGroup, &multiContextCases[0], &multiContextCases[DE_LENGTH_OF_ARRAY(multiContextCases)]);

	tcu::TestCaseGroup* multiThreadGroup = new tcu::TestCaseGroup(m_testCtx, "multi_thread", "Multi-thread rendering with shared surface");
	addChild(multiThreadGroup);
	createRenderGroups<MultiThreadRenderCase>(m_eglTestCtx, multiThreadGroup, &multiContextCases[0], &multiContextCases[DE_LENGTH_OF_ARRAY(multiContextCases)]);
}

} // egl
} // deqp
