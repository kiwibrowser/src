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
 * \brief Advanced blending (GL_KHR_blend_equation_advanced) tests.
 *//*--------------------------------------------------------------------*/

#include "es31fAdvancedBlendTests.hpp"
#include "gluStrUtil.hpp"
#include "glsFragmentOpUtil.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluObjectWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"
#include "tcuPixelFormat.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "rrFragmentOperations.hpp"
#include "sglrReferenceUtils.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include <string>
#include <vector>

namespace deqp
{

using gls::FragmentOpUtil::IntegerQuad;
using gls::FragmentOpUtil::ReferenceQuadRenderer;
using tcu::TextureLevel;
using tcu::Vec2;
using tcu::Vec4;
using tcu::UVec4;
using tcu::TestLog;
using tcu::TextureFormat;
using std::string;
using std::vector;
using std::map;

namespace gles31
{
namespace Functional
{

namespace
{

enum
{
	MAX_VIEWPORT_WIDTH		= 128,
	MAX_VIEWPORT_HEIGHT		= 128
};

enum RenderTargetType
{
	RENDERTARGETTYPE_DEFAULT	= 0,	//!< Default framebuffer
	RENDERTARGETTYPE_SRGB_FBO,
	RENDERTARGETTYPE_MSAA_FBO,

	RENDERTARGETTYPE_LAST
};

static const char* getEquationName (glw::GLenum equation)
{
	switch (equation)
	{
		case GL_MULTIPLY:		return "multiply";
		case GL_SCREEN:			return "screen";
		case GL_OVERLAY:		return "overlay";
		case GL_DARKEN:			return "darken";
		case GL_LIGHTEN:		return "lighten";
		case GL_COLORDODGE:		return "colordodge";
		case GL_COLORBURN:		return "colorburn";
		case GL_HARDLIGHT:		return "hardlight";
		case GL_SOFTLIGHT:		return "softlight";
		case GL_DIFFERENCE:		return "difference";
		case GL_EXCLUSION:		return "exclusion";
		case GL_HSL_HUE:		return "hsl_hue";
		case GL_HSL_SATURATION:	return "hsl_saturation";
		case GL_HSL_COLOR:		return "hsl_color";
		case GL_HSL_LUMINOSITY:	return "hsl_luminosity";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

class AdvancedBlendCase : public TestCase
{
public:
							AdvancedBlendCase	(Context& context, const char* name, const char* desc, deUint32 mode, int overdrawCount, bool coherent, RenderTargetType rtType);

							~AdvancedBlendCase	(void);

	void					init				(void);
	void					deinit				(void);

	IterateResult			iterate		(void);

private:
							AdvancedBlendCase	(const AdvancedBlendCase&);
	AdvancedBlendCase&		operator=			(const AdvancedBlendCase&);

	const deUint32			m_blendMode;
	const int				m_overdrawCount;
	const bool				m_coherentBlending;
	const RenderTargetType	m_rtType;
	const int				m_numIters;

	bool					m_coherentExtensionSupported;

	deUint32				m_colorRbo;
	deUint32				m_fbo;

	deUint32				m_resolveColorRbo;
	deUint32				m_resolveFbo;

	glu::ShaderProgram*		m_program;

	ReferenceQuadRenderer*	m_referenceRenderer;
	TextureLevel*			m_refColorBuffer;

	const int				m_renderWidth;
	const int				m_renderHeight;
	const int				m_viewportWidth;
	const int				m_viewportHeight;

	int						m_iterNdx;
};

AdvancedBlendCase::AdvancedBlendCase (Context&			context,
									  const char*		name,
									  const char*		desc,
									  deUint32			mode,
									  int				overdrawCount,
									  bool				coherent,
									  RenderTargetType	rtType)
	: TestCase				(context, name, desc)
	, m_blendMode			(mode)
	, m_overdrawCount		(overdrawCount)
	, m_coherentBlending	(coherent)
	, m_rtType				(rtType)
	, m_numIters			(5)
	, m_colorRbo			(0)
	, m_fbo					(0)
	, m_resolveColorRbo		(0)
	, m_resolveFbo			(0)
	, m_program				(DE_NULL)
	, m_referenceRenderer	(DE_NULL)
	, m_refColorBuffer		(DE_NULL)
	, m_renderWidth			(rtType != RENDERTARGETTYPE_DEFAULT ? 2*MAX_VIEWPORT_WIDTH	: m_context.getRenderTarget().getWidth())
	, m_renderHeight		(rtType != RENDERTARGETTYPE_DEFAULT ? 2*MAX_VIEWPORT_HEIGHT	: m_context.getRenderTarget().getHeight())
	, m_viewportWidth		(de::min<int>(m_renderWidth,	MAX_VIEWPORT_WIDTH))
	, m_viewportHeight		(de::min<int>(m_renderHeight,	MAX_VIEWPORT_HEIGHT))
	, m_iterNdx				(0)
{
}

const char* getBlendLayoutQualifier (rr::BlendEquationAdvanced equation)
{
	static const char* s_qualifiers[] =
	{
		"blend_support_multiply",
		"blend_support_screen",
		"blend_support_overlay",
		"blend_support_darken",
		"blend_support_lighten",
		"blend_support_colordodge",
		"blend_support_colorburn",
		"blend_support_hardlight",
		"blend_support_softlight",
		"blend_support_difference",
		"blend_support_exclusion",
		"blend_support_hsl_hue",
		"blend_support_hsl_saturation",
		"blend_support_hsl_color",
		"blend_support_hsl_luminosity",
	};
	DE_STATIC_ASSERT(DE_LENGTH_OF_ARRAY(s_qualifiers) == rr::BLENDEQUATION_ADVANCED_LAST);
	DE_ASSERT(de::inBounds<int>(equation, 0, rr::BLENDEQUATION_ADVANCED_LAST));
	return s_qualifiers[equation];
}

glu::ProgramSources getBlendProgramSrc (rr::BlendEquationAdvanced equation, glu::RenderContext& renderContext)
{
	const bool supportsES32 = glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));

	static const char*	s_vertSrc	= "${GLSL_VERSION_DECL}\n"
									  "in highp vec4 a_position;\n"
									  "in mediump vec4 a_color;\n"
									  "out mediump vec4 v_color;\n"
									  "void main()\n"
									  "{\n"
									  "	gl_Position = a_position;\n"
									  "	v_color = a_color;\n"
									  "}\n";
	static const char*	s_fragSrc	= "${GLSL_VERSION_DECL}\n"
									  "${EXTENSION}"
									  "in mediump vec4 v_color;\n"
									  "layout(${SUPPORT_QUALIFIER}) out;\n"
									  "layout(location = 0) out mediump vec4 o_color;\n"
									  "void main()\n"
									  "{\n"
									  "	o_color = v_color;\n"
									  "}\n";

	map<string, string> args;
	args["GLSL_VERSION_DECL"] = supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["EXTENSION"] = supportsES32 ? "\n" : "#extension GL_KHR_blend_equation_advanced : require\n";
	args["SUPPORT_QUALIFIER"] = getBlendLayoutQualifier(equation);

	return glu::ProgramSources()
		<< glu::VertexSource(tcu::StringTemplate(s_vertSrc).specialize(args))
		<< glu::FragmentSource(tcu::StringTemplate(s_fragSrc).specialize(args));
}

void AdvancedBlendCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const bool				useFbo			= m_rtType != RENDERTARGETTYPE_DEFAULT;
	const bool				useSRGB			= m_rtType == RENDERTARGETTYPE_SRGB_FBO;

	m_coherentExtensionSupported = m_context.getContextInfo().isExtensionSupported("GL_KHR_blend_equation_advanced_coherent");

	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		if (!m_context.getContextInfo().isExtensionSupported("GL_KHR_blend_equation_advanced"))
			TCU_THROW(NotSupportedError, "GL_KHR_blend_equation_advanced is not supported");

	if (m_coherentBlending && !m_coherentExtensionSupported)
		TCU_THROW(NotSupportedError, "GL_KHR_blend_equation_advanced_coherent is not supported");

	TCU_CHECK(gl.blendBarrier);

	DE_ASSERT(!m_program);
	DE_ASSERT(!m_referenceRenderer);
	DE_ASSERT(!m_refColorBuffer);

	m_program = new glu::ShaderProgram(m_context.getRenderContext(), getBlendProgramSrc(sglr::rr_util::mapGLBlendEquationAdvanced(m_blendMode), m_context.getRenderContext()));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
	{
		delete m_program;
		m_program = DE_NULL;
		TCU_FAIL("Compile failed");
	}

	m_referenceRenderer	= new ReferenceQuadRenderer;
	m_refColorBuffer	= new TextureLevel(TextureFormat(useSRGB ? TextureFormat::sRGBA : TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_viewportWidth, m_viewportHeight);

	if (useFbo)
	{
		const deUint32	format		= useSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		const int		numSamples	= m_rtType == RENDERTARGETTYPE_MSAA_FBO ? 4 : 0;

		m_testCtx.getLog() << TestLog::Message << "Using FBO of size (" << m_renderWidth << ", " << m_renderHeight << ") with format "
											   << glu::getTextureFormatStr(format) << " and " << numSamples << " samples"
						   << TestLog::EndMessage;

		gl.genRenderbuffers(1, &m_colorRbo);
		gl.bindRenderbuffer(GL_RENDERBUFFER, m_colorRbo);
		gl.renderbufferStorageMultisample(GL_RENDERBUFFER, numSamples, format, m_renderWidth, m_renderHeight);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create color RBO");

		gl.genFramebuffers(1, &m_fbo);
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create FBO");

		TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

		if (numSamples > 0)
		{
			// Create resolve FBO
			gl.genRenderbuffers(1, &m_resolveColorRbo);
			gl.bindRenderbuffer(GL_RENDERBUFFER, m_resolveColorRbo);
			gl.renderbufferStorageMultisample(GL_RENDERBUFFER, 0, format, m_renderWidth, m_renderHeight);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create resolve color RBO");

			gl.genFramebuffers(1, &m_resolveFbo);
			gl.bindFramebuffer(GL_FRAMEBUFFER, m_resolveFbo);
			gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_resolveColorRbo);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create FBO");

			TCU_CHECK(gl.checkFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		}
	}
	else
		DE_ASSERT(m_rtType == RENDERTARGETTYPE_DEFAULT);

	m_iterNdx = 0;
}

AdvancedBlendCase::~AdvancedBlendCase (void)
{
	AdvancedBlendCase::deinit();
}

void AdvancedBlendCase::deinit (void)
{
	delete m_program;
	delete m_referenceRenderer;
	delete m_refColorBuffer;

	m_program			= DE_NULL;
	m_referenceRenderer	= DE_NULL;
	m_refColorBuffer	= DE_NULL;

	if (m_colorRbo || m_fbo)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

		if (m_colorRbo != 0)
		{
			gl.deleteRenderbuffers(1, &m_colorRbo);
			m_colorRbo = 0;
		}

		if (m_fbo != 0)
		{
			gl.deleteFramebuffers(1, &m_fbo);
			m_fbo = 0;
		}

		if (m_resolveColorRbo)
		{
			gl.deleteRenderbuffers(1, &m_resolveColorRbo);
			m_resolveColorRbo = 0;
		}

		if (m_resolveFbo)
		{
			gl.deleteRenderbuffers(1, &m_resolveFbo);
			m_resolveFbo = 0;
		}
	}
}

static tcu::Vec4 randomColor (de::Random* rnd)
{
	const float rgbValues[]		= { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	const float alphaValues[]	= { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };

	// \note Spec assumes premultiplied inputs.
	const float a = rnd->choose<float>(DE_ARRAY_BEGIN(alphaValues), DE_ARRAY_END(alphaValues));
	const float r = a * rnd->choose<float>(DE_ARRAY_BEGIN(rgbValues), DE_ARRAY_END(rgbValues));
	const float g = a * rnd->choose<float>(DE_ARRAY_BEGIN(rgbValues), DE_ARRAY_END(rgbValues));
	const float b = a * rnd->choose<float>(DE_ARRAY_BEGIN(rgbValues), DE_ARRAY_END(rgbValues));
	return tcu::Vec4(r, g, b, a);
}

static tcu::ConstPixelBufferAccess getLinearAccess (const tcu::ConstPixelBufferAccess& access)
{
	if (access.getFormat().order == TextureFormat::sRGBA)
		return tcu::ConstPixelBufferAccess(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8),
										   access.getWidth(), access.getHeight(), access.getDepth(),
										   access.getRowPitch(), access.getSlicePitch(), access.getDataPtr());
	else
		return access;
}

AdvancedBlendCase::IterateResult AdvancedBlendCase::iterate (void)
{
	const glu::RenderContext&		renderCtx		= m_context.getRenderContext();
	const glw::Functions&			gl				= renderCtx.getFunctions();
	de::Random						rnd				(deStringHash(getName()) ^ deInt32Hash(m_iterNdx));
	const int						viewportX		= rnd.getInt(0, m_renderWidth - m_viewportWidth);
	const int						viewportY		= rnd.getInt(0, m_renderHeight - m_viewportHeight);
	const bool						useFbo			= m_rtType != RENDERTARGETTYPE_DEFAULT;
	const bool						requiresResolve	= m_rtType == RENDERTARGETTYPE_MSAA_FBO;
	const int						numQuads		= m_overdrawCount+1;
	TextureLevel					renderedImg		(TextureFormat(TextureFormat::RGBA, TextureFormat::UNORM_INT8), m_viewportWidth, m_viewportHeight);
	vector<Vec4>					colors			(numQuads*4);

	for (vector<Vec4>::iterator col = colors.begin(); col != colors.end(); ++col)
		*col = randomColor(&rnd);

	// Render with GL.
	{
		const deUint32		program				= m_program->getProgram();
		const int			posLoc				= gl.getAttribLocation(program, "a_position");
		const int			colorLoc			= gl.getAttribLocation(program, "a_color");
		const glu::Buffer	indexBuffer			(renderCtx);
		const glu::Buffer	positionBuffer		(renderCtx);
		const glu::Buffer	colorBuffer			(renderCtx);
		vector<Vec2>		positions			(numQuads*4);
		vector<deUint16>	indices				(numQuads*6);
		const deUint16		singleQuadIndices[]	= { 0, 2, 1, 1, 2, 3 };
		const Vec2			singleQuadPos[]		=
		{
			Vec2(-1.0f, -1.0f),
			Vec2(-1.0f, +1.0f),
			Vec2(+1.0f, -1.0f),
			Vec2(+1.0f, +1.0f),
		};

		TCU_CHECK(posLoc >= 0 && colorLoc >= 0);

		for (int quadNdx = 0; quadNdx < numQuads; quadNdx++)
		{
			std::copy(DE_ARRAY_BEGIN(singleQuadPos), DE_ARRAY_END(singleQuadPos), &positions[quadNdx*4]);
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(singleQuadIndices); ndx++)
				indices[quadNdx*6 + ndx] = (deUint16)(quadNdx*4 + singleQuadIndices[ndx]);
		}

		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, *indexBuffer);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, (glw::GLsizeiptr)(indices.size()*sizeof(indices[0])), &indices[0], GL_STATIC_DRAW);

		gl.bindBuffer(GL_ARRAY_BUFFER, *positionBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, (glw::GLsizeiptr)(positions.size()*sizeof(positions[0])), &positions[0], GL_STATIC_DRAW);
		gl.enableVertexAttribArray(posLoc);
		gl.vertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, DE_NULL);

		gl.bindBuffer(GL_ARRAY_BUFFER, *colorBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, (glw::GLsizeiptr)(colors.size()*sizeof(colors[0])), &colors[0], GL_STATIC_DRAW);
		gl.enableVertexAttribArray(colorLoc);
		gl.vertexAttribPointer(colorLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create buffers");

		gl.useProgram(program);
		gl.viewport(viewportX, viewportY, m_viewportWidth, m_viewportHeight);
		gl.blendEquation(m_blendMode);

		// \note coherent extension enables GL_BLEND_ADVANCED_COHERENT_KHR by default
		if (m_coherentBlending)
			gl.enable(GL_BLEND_ADVANCED_COHERENT_KHR);
		else if (m_coherentExtensionSupported)
			gl.disable(GL_BLEND_ADVANCED_COHERENT_KHR);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set render state");

		gl.clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

		gl.disable(GL_BLEND);
		gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, DE_NULL);
		gl.enable(GL_BLEND);

		if (!m_coherentBlending)
			gl.blendBarrier();

		if (m_coherentBlending)
		{
			gl.drawElements(GL_TRIANGLES, 6*(numQuads-1), GL_UNSIGNED_SHORT, (const void*)(deUintptr)(6*sizeof(deUint16)));
		}
		else
		{
			for (int quadNdx = 1; quadNdx < numQuads; quadNdx++)
			{
				gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void*)(deUintptr)(quadNdx*6*sizeof(deUint16)));
				gl.blendBarrier();
			}
		}

		gl.flush();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Render failed");
	}

	// Render reference.
	{
		rr::FragmentOperationState		referenceState;
		const tcu::PixelBufferAccess	colorAccess		= gls::FragmentOpUtil::getMultisampleAccess(m_refColorBuffer->getAccess());
		const tcu::PixelBufferAccess	nullAccess		= tcu::PixelBufferAccess();
		IntegerQuad						quad;

		if (!useFbo && m_context.getRenderTarget().getPixelFormat().alphaBits == 0)
		{
			// Emulate lack of alpha by clearing to 1 and masking out alpha writes
			tcu::clear(*m_refColorBuffer, tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
			referenceState.colorMask = tcu::BVec4(true, true, true, false);
		}

		referenceState.blendEquationAdvaced	= sglr::rr_util::mapGLBlendEquationAdvanced(m_blendMode);

		quad.posA = tcu::IVec2(0, 0);
		quad.posB = tcu::IVec2(m_viewportWidth-1, m_viewportHeight-1);

		for (int quadNdx = 0; quadNdx < numQuads; quadNdx++)
		{
			referenceState.blendMode = quadNdx == 0 ? rr::BLENDMODE_NONE : rr::BLENDMODE_ADVANCED;
			std::copy(&colors[4*quadNdx], &colors[4*quadNdx] + 4, &quad.color[0]);
			m_referenceRenderer->render(colorAccess, nullAccess /* no depth */, nullAccess /* no stencil */, quad, referenceState);
		}
	}

	if (requiresResolve)
	{
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_resolveFbo);
		gl.blitFramebuffer(0, 0, m_renderWidth, m_renderHeight, 0, 0, m_renderWidth, m_renderHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Resolve blit failed");

		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_resolveFbo);
	}

	glu::readPixels(renderCtx, viewportX, viewportY, renderedImg.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels()");

	if (requiresResolve)
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	{
		const bool	isHSLMode	= m_blendMode == GL_HSL_HUE			||
								  m_blendMode == GL_HSL_SATURATION	||
								  m_blendMode == GL_HSL_COLOR		||
								  m_blendMode == GL_HSL_LUMINOSITY;
		bool		comparePass	= false;

		if (isHSLMode)
		{
			// Compensate for more demanding HSL code by using fuzzy comparison.
			const float threshold = 0.002f;
			comparePass = tcu::fuzzyCompare(m_testCtx.getLog(), "CompareResult", "Image Comparison Result",
											getLinearAccess(m_refColorBuffer->getAccess()),
											renderedImg.getAccess(),
											threshold, tcu::COMPARE_LOG_RESULT);
		}
		else
		{
			const UVec4 compareThreshold = (useFbo ? tcu::PixelFormat(8, 8, 8, 8) : m_context.getRenderTarget().getPixelFormat()).getColorThreshold().toIVec().asUint()
									 * UVec4(5) / UVec4(2) + UVec4(3 * m_overdrawCount);

			comparePass = tcu::bilinearCompare(m_testCtx.getLog(), "CompareResult", "Image Comparison Result",
											  getLinearAccess(m_refColorBuffer->getAccess()),
											  renderedImg.getAccess(),
											  tcu::RGBA(compareThreshold[0], compareThreshold[1], compareThreshold[2], compareThreshold[3]),
											  tcu::COMPARE_LOG_RESULT);
		}

		if (!comparePass)
		{
			m_context.getTestContext().setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
			return STOP;
		}
	}

	m_iterNdx += 1;

	if (m_iterNdx < m_numIters)
		return CONTINUE;
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
}

class BlendAdvancedCoherentStateCase : public TestCase
{
public:
											BlendAdvancedCoherentStateCase	(Context&						context,
																			 const char*					name,
																			 const char*					description,
																			 gls::StateQueryUtil::QueryType	type);
private:
	IterateResult							iterate							(void);

	const gls::StateQueryUtil::QueryType	m_type;
};

BlendAdvancedCoherentStateCase::BlendAdvancedCoherentStateCase	(Context&						context,
																 const char*					name,
																 const char*					description,
																 gls::StateQueryUtil::QueryType	type)
	: TestCase	(context, name, description)
	, m_type	(type)
{
}

BlendAdvancedCoherentStateCase::IterateResult BlendAdvancedCoherentStateCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, m_context.getContextInfo().isExtensionSupported("GL_KHR_blend_equation_advanced_coherent"), "GL_KHR_blend_equation_advanced_coherent is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// check inital value
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial");
		gls::StateQueryUtil::verifyStateBoolean(result, gl, GL_BLEND_ADVANCED_COHERENT_KHR, true, m_type);
	}

	// check toggle
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Toggle", "Toggle");
		gl.glDisable(GL_BLEND_ADVANCED_COHERENT_KHR);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "glDisable");

		gls::StateQueryUtil::verifyStateBoolean(result, gl, GL_BLEND_ADVANCED_COHERENT_KHR, false, m_type);

		gl.glEnable(GL_BLEND_ADVANCED_COHERENT_KHR);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "glEnable");

		gls::StateQueryUtil::verifyStateBoolean(result, gl, GL_BLEND_ADVANCED_COHERENT_KHR, true, m_type);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BlendEquationStateCase : public TestCase
{
public:
											BlendEquationStateCase	(Context&						context,
																	 const char*					name,
																	 const char*					description,
																	 const glw::GLenum*				equations,
																	 int							numEquations,
																	 gls::StateQueryUtil::QueryType	type);
private:
	IterateResult							iterate					(void);

	const gls::StateQueryUtil::QueryType	m_type;
	const glw::GLenum*						m_equations;
	const int								m_numEquations;
};

BlendEquationStateCase::BlendEquationStateCase	(Context&						context,
												 const char*					name,
												 const char*					description,
												 const glw::GLenum*				equations,
												 int							numEquations,
												 gls::StateQueryUtil::QueryType	type)
	: TestCase			(context, name, description)
	, m_type			(type)
	, m_equations		(equations)
	, m_numEquations	(numEquations)
{
}

BlendEquationStateCase::IterateResult BlendEquationStateCase::iterate (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_CHECK_AND_THROW(NotSupportedError, m_context.getContextInfo().isExtensionSupported("GL_KHR_blend_equation_advanced"), "GL_KHR_blend_equation_advanced is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	for (int ndx = 0; ndx < m_numEquations; ++ndx)
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Type", "Test " + de::toString(glu::getBlendEquationStr(m_equations[ndx])));

		gl.glBlendEquation(m_equations[ndx]);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "glBlendEquation");

		gls::StateQueryUtil::verifyStateInteger(result, gl, GL_BLEND_EQUATION, m_equations[ndx], m_type);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BlendEquationIndexedStateCase : public TestCase
{
public:
											BlendEquationIndexedStateCase	(Context&						context,
																			 const char*					name,
																			 const char*					description,
																			 const glw::GLenum*				equations,
																			 int							numEquations,
																			 gls::StateQueryUtil::QueryType	type);
private:
	IterateResult							iterate							(void);

	const gls::StateQueryUtil::QueryType	m_type;
	const glw::GLenum*						m_equations;
	const int								m_numEquations;
};

BlendEquationIndexedStateCase::BlendEquationIndexedStateCase	(Context&						context,
																 const char*					name,
																 const char*					description,
																 const glw::GLenum*				equations,
																 int							numEquations,
																 gls::StateQueryUtil::QueryType	type)
	: TestCase			(context, name, description)
	, m_type			(type)
	, m_equations		(equations)
	, m_numEquations	(numEquations)
{
}

BlendEquationIndexedStateCase::IterateResult BlendEquationIndexedStateCase::iterate (void)
{
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		TCU_CHECK_AND_THROW(NotSupportedError, m_context.getContextInfo().isExtensionSupported("GL_KHR_blend_equation_advanced"), "GL_KHR_blend_equation_advanced is not supported");
		TCU_CHECK_AND_THROW(NotSupportedError, m_context.getContextInfo().isExtensionSupported("GL_EXT_draw_buffers_indexed"), "GL_EXT_draw_buffers_indexed is not supported");
	}

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	for (int ndx = 0; ndx < m_numEquations; ++ndx)
	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Type", "Test " + de::toString(glu::getBlendEquationStr(m_equations[ndx])));

		gl.glBlendEquationi(2, m_equations[ndx]);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "glBlendEquationi");

		gls::StateQueryUtil::verifyStateIndexedInteger(result, gl, GL_BLEND_EQUATION, 2, m_equations[ndx], m_type);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

AdvancedBlendTests::AdvancedBlendTests (Context& context)
	: TestCaseGroup(context, "blend_equation_advanced", "GL_blend_equation_advanced Tests")
{
}

AdvancedBlendTests::~AdvancedBlendTests (void)
{
}

void AdvancedBlendTests::init (void)
{
	static const glw::GLenum s_blendEquations[] =
	{
		GL_MULTIPLY,
		GL_SCREEN,
		GL_OVERLAY,
		GL_DARKEN,
		GL_LIGHTEN,
		GL_COLORDODGE,
		GL_COLORBURN,
		GL_HARDLIGHT,
		GL_SOFTLIGHT,
		GL_DIFFERENCE,
		GL_EXCLUSION,
		GL_HSL_HUE,
		GL_HSL_SATURATION,
		GL_HSL_COLOR,
		GL_HSL_LUMINOSITY,
	};

	tcu::TestCaseGroup* const	stateQueryGroup		= new tcu::TestCaseGroup(m_testCtx, "state_query",		"State query tests");
	tcu::TestCaseGroup* const	basicGroup			= new tcu::TestCaseGroup(m_testCtx, "basic",			"Single quad only");
	tcu::TestCaseGroup* const	srgbGroup			= new tcu::TestCaseGroup(m_testCtx, "srgb",				"Advanced blending with sRGB FBO");
	tcu::TestCaseGroup* const	msaaGroup			= new tcu::TestCaseGroup(m_testCtx, "msaa",				"Advanced blending with MSAA FBO");
	tcu::TestCaseGroup* const	barrierGroup		= new tcu::TestCaseGroup(m_testCtx, "barrier",			"Multiple overlapping quads with blend barriers");
	tcu::TestCaseGroup* const	coherentGroup		= new tcu::TestCaseGroup(m_testCtx, "coherent",			"Overlapping quads with coherent blending");
	tcu::TestCaseGroup* const	coherentMsaaGroup	= new tcu::TestCaseGroup(m_testCtx, "coherent_msaa",	"Overlapping quads with coherent blending with MSAA FBO");

	addChild(stateQueryGroup);
	addChild(basicGroup);
	addChild(srgbGroup);
	addChild(msaaGroup);
	addChild(barrierGroup);
	addChild(coherentGroup);
	addChild(coherentMsaaGroup);

	// .state_query
	{
		using namespace gls::StateQueryUtil;

		stateQueryGroup->addChild(new BlendAdvancedCoherentStateCase(m_context, "blend_advanced_coherent_getboolean",	"Test BLEND_ADVANCED_COHERENT_KHR", QUERY_BOOLEAN));
		stateQueryGroup->addChild(new BlendAdvancedCoherentStateCase(m_context, "blend_advanced_coherent_isenabled",	"Test BLEND_ADVANCED_COHERENT_KHR", QUERY_ISENABLED));
		stateQueryGroup->addChild(new BlendAdvancedCoherentStateCase(m_context, "blend_advanced_coherent_getinteger",	"Test BLEND_ADVANCED_COHERENT_KHR", QUERY_INTEGER));
		stateQueryGroup->addChild(new BlendAdvancedCoherentStateCase(m_context, "blend_advanced_coherent_getinteger64",	"Test BLEND_ADVANCED_COHERENT_KHR", QUERY_INTEGER64));
		stateQueryGroup->addChild(new BlendAdvancedCoherentStateCase(m_context, "blend_advanced_coherent_getfloat",		"Test BLEND_ADVANCED_COHERENT_KHR", QUERY_FLOAT));

		stateQueryGroup->addChild(new BlendEquationStateCase(m_context, "blend_equation_getboolean",	"Test BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_BOOLEAN));
		stateQueryGroup->addChild(new BlendEquationStateCase(m_context, "blend_equation_getinteger",	"Test BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_INTEGER));
		stateQueryGroup->addChild(new BlendEquationStateCase(m_context, "blend_equation_getinteger64",	"Test BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_INTEGER64));
		stateQueryGroup->addChild(new BlendEquationStateCase(m_context, "blend_equation_getfloat",		"Test BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_FLOAT));

		stateQueryGroup->addChild(new BlendEquationIndexedStateCase(m_context, "blend_equation_getbooleani_v",		"Test per-attchment BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_INDEXED_BOOLEAN));
		stateQueryGroup->addChild(new BlendEquationIndexedStateCase(m_context, "blend_equation_getintegeri_v",		"Test per-attchment BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_INDEXED_INTEGER));
		stateQueryGroup->addChild(new BlendEquationIndexedStateCase(m_context, "blend_equation_getinteger64i_v",	"Test per-attchment BLEND_EQUATION", s_blendEquations, DE_LENGTH_OF_ARRAY(s_blendEquations), QUERY_INDEXED_INTEGER64));
	}

	// others
	for (int modeNdx = 0; modeNdx < DE_LENGTH_OF_ARRAY(s_blendEquations); modeNdx++)
	{
		const char* const		name		= getEquationName(s_blendEquations[modeNdx]);
		const char* const		desc		= "";
		const deUint32			mode		= s_blendEquations[modeNdx];

		basicGroup->addChild		(new AdvancedBlendCase(m_context, name, desc, mode, 1, false,	RENDERTARGETTYPE_DEFAULT));
		srgbGroup->addChild			(new AdvancedBlendCase(m_context, name, desc, mode, 1, false,	RENDERTARGETTYPE_SRGB_FBO));
		msaaGroup->addChild			(new AdvancedBlendCase(m_context, name, desc, mode, 1, false,	RENDERTARGETTYPE_MSAA_FBO));
		barrierGroup->addChild		(new AdvancedBlendCase(m_context, name, desc, mode, 4, false,	RENDERTARGETTYPE_DEFAULT));
		coherentGroup->addChild		(new AdvancedBlendCase(m_context, name, desc, mode, 4, true,	RENDERTARGETTYPE_DEFAULT));
		coherentMsaaGroup->addChild	(new AdvancedBlendCase(m_context, name, desc, mode, 4, true,	RENDERTARGETTYPE_MSAA_FBO));
	}
}

} // Functional
} // gles31
} // deqp
