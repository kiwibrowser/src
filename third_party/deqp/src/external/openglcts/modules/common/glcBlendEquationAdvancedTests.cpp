/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

#include "glcBlendEquationAdvancedTests.hpp"
// de
#include "deRandom.hpp"
#include "deString.h"
// tcu
#include "tcuRGBA.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorType.hpp"
#include "tcuVectorUtil.hpp"
// glu
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
// glw
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
// de
#include "deMath.h"
// stl
#include <algorithm>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace glcts
{
using tcu::TestLog;

static const float	s_pos[]	 = { -1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f };
static const deUint16 s_indices[] = { 0, 1, 2, 2, 1, 3 };

// Lists all modes introduced by the extension.
static const glw::GLenum s_modes[] = { GL_MULTIPLY_KHR,		  GL_SCREEN_KHR,	 GL_OVERLAY_KHR,	   GL_DARKEN_KHR,
									   GL_LIGHTEN_KHR,		  GL_COLORDODGE_KHR, GL_COLORBURN_KHR,	 GL_HARDLIGHT_KHR,
									   GL_SOFTLIGHT_KHR,	  GL_DIFFERENCE_KHR, GL_EXCLUSION_KHR,	 GL_HSL_HUE_KHR,
									   GL_HSL_SATURATION_KHR, GL_HSL_COLOR_KHR,  GL_HSL_LUMINOSITY_KHR };

static const char* GetModeStr(glw::GLenum mode)
{
	switch (mode)
	{
	case GL_MULTIPLY_KHR:
		return "GL_MULTIPLY_KHR";
	case GL_SCREEN_KHR:
		return "GL_SCREEN_KHR";
	case GL_OVERLAY_KHR:
		return "GL_OVERLAY_KHR";
	case GL_DARKEN_KHR:
		return "GL_DARKEN_KHR";
	case GL_LIGHTEN_KHR:
		return "GL_LIGHTEN_KHR";
	case GL_COLORDODGE_KHR:
		return "GL_COLORDODGE_KHR";
	case GL_COLORBURN_KHR:
		return "GL_COLORBURN_KHR";
	case GL_HARDLIGHT_KHR:
		return "GL_HARDLIGHT_KHR";
	case GL_SOFTLIGHT_KHR:
		return "GL_SOFTLIGHT_KHR";
	case GL_DIFFERENCE_KHR:
		return "GL_DIFFERENCE_KHR";
	case GL_EXCLUSION_KHR:
		return "GL_EXCLUSION_KHR";
	case GL_HSL_HUE_KHR:
		return "GL_HSL_HUE_KHR";
	case GL_HSL_SATURATION_KHR:
		return "GL_HSL_SATURATION_KHR";
	case GL_HSL_COLOR_KHR:
		return "GL_HSL_COLOR_KHR";
	case GL_HSL_LUMINOSITY_KHR:
		return "GL_HSL_LUMINOSITY_KHR";
	default:
		DE_ASSERT(DE_FALSE && "Blend mode not from GL_KHR_blend_equation_advanced.");
		return "Blend mode not from GL_KHR_blend_equation_advanced.";
	}
}

static const char* GetLayoutQualifierStr(glw::GLenum mode)
{
	switch (mode)
	{
	case GL_MULTIPLY_KHR:
		return "blend_support_multiply";
	case GL_SCREEN_KHR:
		return "blend_support_screen";
	case GL_OVERLAY_KHR:
		return "blend_support_overlay";
	case GL_DARKEN_KHR:
		return "blend_support_darken";
	case GL_LIGHTEN_KHR:
		return "blend_support_lighten";
	case GL_COLORDODGE_KHR:
		return "blend_support_colordodge";
	case GL_COLORBURN_KHR:
		return "blend_support_colorburn";
	case GL_HARDLIGHT_KHR:
		return "blend_support_hardlight";
	case GL_SOFTLIGHT_KHR:
		return "blend_support_softlight";
	case GL_DIFFERENCE_KHR:
		return "blend_support_difference";
	case GL_EXCLUSION_KHR:
		return "blend_support_exclusion";
	case GL_HSL_HUE_KHR:
		return "blend_support_hsl_hue";
	case GL_HSL_SATURATION_KHR:
		return "blend_support_hsl_saturation";
	case GL_HSL_COLOR_KHR:
		return "blend_support_hsl_color";
	case GL_HSL_LUMINOSITY_KHR:
		return "blend_support_hsl_luminosity";
	default:
		DE_ASSERT(DE_FALSE && "Blend mode not from GL_KHR_blend_equation_advanced.");
		return "Blend mode not from GL_KHR_blend_equation_advanced.";
	}
}

static bool IsExtensionSupported(deqp::Context& context, const char* extension)
{
	const std::vector<std::string>& v = context.getContextInfo().getExtensions();
	return std::find(v.begin(), v.end(), extension) != v.end();
}

static float GetP0(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	return src[3] * dst[3];
}
static float GetP1(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	return src[3] * (1.f - dst[3]);
}
static float GetP2(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	return dst[3] * (1.f - src[3]);
}

static tcu::Vec4 Blend(const tcu::Vec4& rgb, const tcu::Vec4& src, const tcu::Vec4& dst)
{
	float	 p[3]   = { GetP0(src, dst), GetP1(src, dst), GetP2(src, dst) };
	float	 alpha  = p[0] + p[1] + p[2];
	tcu::Vec4 rgbOut = (p[0] * rgb) + (p[1] * src) + (p[2] * dst);
	return tcu::Vec4(rgbOut[0], rgbOut[1], rgbOut[2], alpha);
}

static tcu::Vec4 BlendMultiply(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = src * dst;

	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendScreen(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = src + dst - src * dst;

	return Blend(rgb, src, dst);
}

static float Overlay(float s, float d)
{
	if (d <= 0.5f)
		return 2.f * s * d;
	else
		return 1.f - 2.f * (1.f - s) * (1.f - d);
}

static tcu::Vec4 BlendOverlay(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(Overlay(src[0], dst[0]), Overlay(src[1], dst[1]), Overlay(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendDarken(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(de::min(src[0], dst[0]), de::min(src[1], dst[1]), de::min(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendLighten(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(de::max(src[0], dst[0]), de::max(src[1], dst[1]), de::max(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static float ColorDodge(float s, float d)
{
	if (d <= 0.f)
		return 0.f;
	else if (d > 0.f && s < 1.f)
		return de::min(1.f, d / (1.f - s));
	else
		return 1.f;
}

static tcu::Vec4 BlendColorDodge(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(ColorDodge(src[0], dst[0]), ColorDodge(src[1], dst[1]), ColorDodge(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static float ColorBurn(float s, float d)
{
	if (d >= 1.f)
		return 1.f;
	else if (d < 1.f && s > 0.f)
		return 1.f - de::min(1.f, (1.f - d) / s);
	else
	{
		DE_ASSERT(d < 1.f && s <= 0.f);
		return 0.f;
	}
}

static tcu::Vec4 BlendColorBurn(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(ColorBurn(src[0], dst[0]), ColorBurn(src[1], dst[1]), ColorBurn(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static float HardLight(float s, float d)
{
	if (s <= 0.5f)
		return 2.f * s * d;
	else
		return 1.f - 2.f * (1.f - s) * (1.f - d);
}

static tcu::Vec4 BlendHardLight(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(HardLight(src[0], dst[0]), HardLight(src[1], dst[1]), HardLight(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static float SoftLight(float s, float d)
{
	if (s <= 0.5f)
		return d - (1.f - 2.f * s) * d * (1.f - d);
	else if (d <= 0.25f)
	{
		DE_ASSERT(s > 0.5f && d <= 0.25f);
		return d + (2.f * s - 1.f) * d * ((16.f * d - 12.f) * d + 3.f);
	}
	else
	{
		DE_ASSERT(s > 0.5f && d > 0.25f);
		return d + (2.f * s - 1.f) * (deFloatSqrt(d) - d);
	}
}

static tcu::Vec4 BlendSoftLight(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(SoftLight(src[0], dst[0]), SoftLight(src[1], dst[1]), SoftLight(src[2], dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendDifference(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(deFloatAbs(src[0] - dst[0]), deFloatAbs(src[1] - dst[1]), deFloatAbs(src[2] - dst[2]), 0.f);

	return Blend(rgb, src, dst);
}

static float Exclusion(float s, float d)
{
	return s + d - 2.f * s * d;
}

static tcu::Vec4 BlendExclusion(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb(Exclusion(src[0], dst[0]), Exclusion(src[1], dst[1]), Exclusion(src[2], dst[2]), 0.f);
	return Blend(rgb, src, dst);
}

static float Luminance(const tcu::Vec4& rgba)
{
	// Coefficients from the KHR_GL_blend_equation_advanced test spec.
	return 0.30f * rgba[0] + 0.59f * rgba[1] + 0.11f * rgba[2];
}

// Minimum of R, G and B components.
static float MinRGB(const tcu::Vec4& rgba)
{
	return deFloatMin(deFloatMin(rgba[0], rgba[1]), rgba[2]);
}

// Maximum of R, G and B components.
static float MaxRGB(const tcu::Vec4& rgba)
{
	return deFloatMax(deFloatMax(rgba[0], rgba[1]), rgba[2]);
}

static float Saturation(const tcu::Vec4& rgba)
{
	return MaxRGB(rgba) - MinRGB(rgba);
}

// Take the base RGB color <cbase> and override its luminosity
// with that of the RGB color <clum>.
static tcu::Vec4 SetLum(const tcu::Vec4& cbase, const tcu::Vec4& clum)
{
	float	 lbase = Luminance(cbase);
	float	 llum  = Luminance(clum);
	float	 ldiff = llum - lbase;
	tcu::Vec4 color = cbase + tcu::Vec4(ldiff);
	tcu::Vec4 vllum = tcu::Vec4(llum);
	if (MinRGB(color) < 0.0f)
	{
		return vllum + ((color - vllum) * llum) / (llum - MinRGB(color));
	}
	else if (MaxRGB(color) > 1.0f)
	{
		return vllum + ((color - vllum) * (1.f - llum)) / (MaxRGB(color) - llum);
	}
	else
	{
		return color;
	}
}

// Take the base RGB color <cbase> and override its saturation with
// that of the RGB color <csat>.  The override the luminosity of the
// result with that of the RGB color <clum>.
static tcu::Vec4 SetLumSat(const tcu::Vec4& cbase, const tcu::Vec4& csat, const tcu::Vec4& clum)
{
	float	 minbase = MinRGB(cbase);
	float	 sbase   = Saturation(cbase);
	float	 ssat	= Saturation(csat);
	tcu::Vec4 color;
	if (sbase > 0)
	{
		// From the extension spec:
		// Equivalent (modulo rounding errors) to setting the
		// smallest (R,G,B) component to 0, the largest to <ssat>,
		// and interpolating the "middle" component based on its
		// original value relative to the smallest/largest.
		color = (cbase - tcu::Vec4(minbase)) * ssat / sbase;
	}
	else
	{
		color = tcu::Vec4(0.0f);
	}
	return SetLum(color, clum);
}

static tcu::Vec4 BlendHSLHue(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = SetLumSat(src, dst, dst);
	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendHSLSaturation(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = SetLumSat(dst, src, dst);
	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendHSLColor(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = SetLum(src, dst);
	return Blend(rgb, src, dst);
}

static tcu::Vec4 BlendHSLuminosity(const tcu::Vec4& src, const tcu::Vec4& dst)
{
	tcu::Vec4 rgb = SetLum(dst, src);
	return Blend(rgb, src, dst);
}

typedef tcu::Vec4 (*BlendFunc)(const tcu::Vec4& src, const tcu::Vec4& dst);

static BlendFunc GetBlendFunc(glw::GLenum mode)
{
	switch (mode)
	{
	case GL_MULTIPLY_KHR:
		return BlendMultiply;
	case GL_SCREEN_KHR:
		return BlendScreen;
	case GL_OVERLAY_KHR:
		return BlendOverlay;
	case GL_DARKEN_KHR:
		return BlendDarken;
	case GL_LIGHTEN_KHR:
		return BlendLighten;
	case GL_COLORDODGE_KHR:
		return BlendColorDodge;
	case GL_COLORBURN_KHR:
		return BlendColorBurn;
	case GL_HARDLIGHT_KHR:
		return BlendHardLight;
	case GL_SOFTLIGHT_KHR:
		return BlendSoftLight;
	case GL_DIFFERENCE_KHR:
		return BlendDifference;
	case GL_EXCLUSION_KHR:
		return BlendExclusion;
	case GL_HSL_HUE_KHR:
		return BlendHSLHue;
	case GL_HSL_SATURATION_KHR:
		return BlendHSLSaturation;
	case GL_HSL_COLOR_KHR:
		return BlendHSLColor;
	case GL_HSL_LUMINOSITY_KHR:
		return BlendHSLuminosity;
	default:
		DE_ASSERT(DE_FALSE && "Blend mode not from GL_KHR_blend_equation_advanced.");
		return NULL;
	}
}

static tcu::Vec4 ToNormal(const tcu::Vec4& v)
{
	float a = v[3];
	if (a == 0)
		return tcu::Vec4(0.f, 0.f, 0.f, 0.f);
	return tcu::Vec4(v[0] / a, v[1] / a, v[2] / a, a);
}

// Blend premultiplied src and dst with given blend mode.
static tcu::Vec4 Blend(glw::GLenum mode, const tcu::Vec4& src, const tcu::Vec4& dst)
{
	BlendFunc blend   = GetBlendFunc(mode);
	tcu::Vec4 srcNorm = ToNormal(src);
	tcu::Vec4 dstNorm = ToNormal(dst);

	return blend(srcNorm, dstNorm);
}

static std::string GetDef2DVtxSrc(glu::GLSLVersion glslVersion)
{
	std::stringstream str;

	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_430);

	str << glu::getGLSLVersionDeclaration(glslVersion) << "\n"
		<< "in highp vec2 aPos;\n"
		   "void main() {\n"
		   "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
		   "}\n";
	return str.str();
}

static std::string GetSolidShader(
	const char* layoutQualifier, const char* glslVersion,
	const char* extensionDirective = "#extension GL_KHR_blend_equation_advanced : require")
{
	static const char* frgSrcTemplate = "${VERSION_DIRECTIVE}\n"
										"${EXTENSION_DIRECTIVE}\n"
										"\n"
										"precision highp float;\n"
										"\n"
										"${LAYOUT_QUALIFIER}\n"
										"layout (location = 0) out vec4 oCol;\n"
										"\n"
										"uniform vec4 uSrcCol;\n"
										"\n"
										"void main (void) {\n"
										"   oCol = uSrcCol;\n"
										"}\n";

	std::map<std::string, std::string> args;
	args["VERSION_DIRECTIVE"]   = glslVersion;
	args["EXTENSION_DIRECTIVE"] = extensionDirective;
	if (layoutQualifier)
		args["LAYOUT_QUALIFIER"] = std::string("layout (") + layoutQualifier + std::string(") out;");
	else
		args["LAYOUT_QUALIFIER"] = ""; // none

	return tcu::StringTemplate(frgSrcTemplate).specialize(args);
}

/*
 * Framebuffer helper.
 * Creates and binds FBO and either one or two renderbuffer color attachments (fmt0, fmt1) with
 * given size (width, height).
 *
 * Note: Does not restore previous 1) fbo binding, 2) scissor or 3) viewport upon exit.
 */
class FBOSentry
{
public:
	FBOSentry(const glw::Functions& gl, int width, int height, glw::GLenum fmt0);
	FBOSentry(const glw::Functions& gl, int width, int height, glw::GLenum fmt0, glw::GLenum fmt1);
	~FBOSentry();

private:
	void init(int width, int height, glw::GLenum fmt0, glw::GLenum fmt1);
	const glw::Functions& m_gl;
	glw::GLuint			  m_fbo;
	glw::GLuint			  m_rbo[2];
};

FBOSentry::FBOSentry(const glw::Functions& gl, int width, int height, glw::GLenum fmt0) : m_gl(gl)
{
	init(width, height, fmt0, GL_NONE);
}

FBOSentry::FBOSentry(const glw::Functions& gl, int width, int height, glw::GLenum fmt0, glw::GLenum fmt1) : m_gl(gl)
{
	init(width, height, fmt0, fmt1);
}

FBOSentry::~FBOSentry()
{
	m_gl.deleteFramebuffers(1, &m_fbo);
	m_gl.deleteRenderbuffers(2, m_rbo);
}

void FBOSentry::init(int width, int height, glw::GLenum fmt0, glw::GLenum fmt1)
{
	m_gl.genFramebuffers(1, &m_fbo);
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	m_gl.genRenderbuffers(2, m_rbo);
	m_gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[0]);
	m_gl.renderbufferStorage(GL_RENDERBUFFER, fmt0, width, height);
	m_gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo[0]);

	if (fmt1 != GL_NONE)
	{
		m_gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo[1]);
		m_gl.renderbufferStorage(GL_RENDERBUFFER, fmt1, width, height);
		m_gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, m_rbo[1]);
	}

	glw::GLenum status = m_gl.checkFramebufferStatus(GL_FRAMEBUFFER);
	if (m_gl.getError() != GL_NO_ERROR || status != GL_FRAMEBUFFER_COMPLETE)
	{
		TCU_FAIL("Framebuffer failed");
	}

	m_gl.viewport(0, 0, width, height);
	m_gl.scissor(0, 0, width, height);
}

class CoherentBlendTestCaseGroup : public deqp::TestCaseGroup
{
public:
	CoherentBlendTestCaseGroup(deqp::Context& context, glu::GLSLVersion glslVersion)
		: TestCaseGroup(context, "test_coherency", ""), m_glslVersion(glslVersion)
	{
	}

	void init(void)
	{
		addChild(
			new CoherentBlendTest(m_context, "mixedSequence", m_glslVersion, 4, DE_LENGTH_OF_ARRAY(s_mixed), s_mixed));
		addChild(new CoherentBlendTest(m_context, "multiplySequence", m_glslVersion, 1, DE_LENGTH_OF_ARRAY(s_multiply),
									   s_multiply));
	}

private:
	struct BlendStep;

	class CoherentBlendTest : public deqp::TestCase
	{
	public:
		CoherentBlendTest(deqp::Context& context, const char* name, glu::GLSLVersion glslVersion, int repeatCount,
						  int numSteps, const BlendStep* steps)
			: TestCase(context, name, "")
			, m_glslVersion(glslVersion)
			, m_repeatCount(repeatCount)
			, m_numSteps(numSteps)
			, m_steps(steps)
		{
			DE_ASSERT(repeatCount > 0 && numSteps > 0 && steps);
		}

		IterateResult iterate(void);

	private:
		glu::GLSLVersion m_glslVersion;
		int				 m_repeatCount;
		int				 m_numSteps;
		const BlendStep* m_steps;
	};

	struct BlendStep
	{
		glw::GLenum  mode;
		glw::GLfloat src[4]; // rgba
	};

	// Blend sequences.
	static const BlendStep s_mixed[11];
	static const BlendStep s_multiply[7];

	glu::GLSLVersion m_glslVersion;
};

const CoherentBlendTestCaseGroup::BlendStep CoherentBlendTestCaseGroup::s_mixed[] = {
	{ GL_LIGHTEN_KHR, { 0.250f, 0.375f, 1.000f, 1.000f } },	// =>  (0.250, 0.375, 1.000, 1.000) (from src)
	{ GL_OVERLAY_KHR, { 1.000f, 1.000f, 1.000f, 1.000f } },	// => ~(0.500, 0.750, 1.000, 1.000)
	{ GL_HARDLIGHT_KHR, { 0.750f, 0.750f, 0.250f, 1.000f } },  // => ~(0.750, 1.000, 0.500, 1.000)
	{ GL_DARKEN_KHR, { 0.250f, 0.250f, 0.250f, 1.000f } },	 // =>  (0.250, 0.250, 0.250, 1.000) (from src)
	{ GL_COLORDODGE_KHR, { 0.750f, 0.875f, 1.000f, 1.000f } }, // => ~(1.000, 1.000, 1.000, 1.000)
	{ GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } },   // => ~(0.500, 0.500, 0.500, 1.000)
	{ GL_SCREEN_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } },	 // => ~(0.750, 0.750, 0.750, 1.000)
	{ GL_DARKEN_KHR, { 0.250f, 0.500f, 0.500f, 1.000f } },	 // =>  (0.250, 0.500, 0.500, 1.000) (from src)
	{ GL_DIFFERENCE_KHR, { 0.000f, 0.875f, 0.125f, 1.000f } }, // => ~(0.250, 0.375, 0.375, 1.000)
	{ GL_EXCLUSION_KHR, { 1.000f, 0.500f, 0.750f, 1.000f } },  // => ~(0.750, 0.500, 0.563, 1.000)
	{ GL_DARKEN_KHR, { 0.125f, 0.125f, 0.125f, 1.000f } },	 // =>  (0.125, 0.125, 0.125, 1.000) (from src)
	// Last row is unique and "accurate" since it comes from the source.
	// That means so that it can be easily tested for correctness.
};

const CoherentBlendTestCaseGroup::BlendStep CoherentBlendTestCaseGroup::s_multiply[] = {
	{ GL_LIGHTEN_KHR, { 1.000f, 1.000f, 1.000f, 1.000f } },  { GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } },
	{ GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } }, { GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } },
	{ GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } }, { GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } },
	{ GL_MULTIPLY_KHR, { 0.500f, 0.500f, 0.500f, 1.000f } }, // ~4 in 8bits
};

CoherentBlendTestCaseGroup::CoherentBlendTest::IterateResult CoherentBlendTestCaseGroup::CoherentBlendTest::iterate(
	void)
{
	static const int		 dim = 1024;
	const tcu::RenderTarget& rt  = m_context.getRenderContext().getRenderTarget();
	const tcu::PixelFormat&  pf  = rt.getPixelFormat();
	const glw::Functions&	gl  = m_context.getRenderContext().getFunctions();
	TestLog&				 log = m_testCtx.getLog();

	// Check that extension is supported.
	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}

	bool needBarrier = !IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced_coherent");

	tcu::Vec4 dstCol(0.f, 0.f, 0.f, 0.f);

	FBOSentry fbo(gl, dim, dim, GL_RGBA8);

	// Setup progra
	std::string frgSrc = GetSolidShader("blend_support_all_equations", glu::getGLSLVersionDeclaration(m_glslVersion));
	glu::ShaderProgram p(m_context.getRenderContext(),
						 glu::makeVtxFragSources(GetDef2DVtxSrc(m_glslVersion).c_str(), frgSrc.c_str()));
	if (!p.isOk())
	{
		log << p;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(p.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

	glu::VertexArrayBinding posBinding = glu::va::Float("aPos", 2, 4, 0, &s_pos[0]);

	// Enable blending.
	gl.disable(GL_DITHER);
	gl.enable(GL_BLEND);

	// Clear.
	gl.clearColor(dstCol[0], dstCol[1], dstCol[2], dstCol[3]);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// Blend barrier.
	if (needBarrier)
		gl.blendBarrier();

	// Repeat block.
	for (int i = 0; i < m_repeatCount; i++)
		// Loop blending steps.
		for (int j = 0; j < m_numSteps; j++)
		{
			const BlendStep& s = m_steps[j];
			tcu::Vec4		 srcCol(s.src[0], s.src[1], s.src[2], s.src[3]);
			tcu::Vec4		 refCol = Blend(s.mode, srcCol, dstCol);
			dstCol					= refCol;

			// Set blend equation.
			gl.blendEquation(s.mode);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BlendEquation failed");

			// Set source color.
			gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uSrcCol"), srcCol[0], srcCol[1], srcCol[2], srcCol[3]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniforms failed");

			// Draw.
			glu::draw(m_context.getRenderContext(), p.getProgram(), 1, &posBinding,
					  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

			// Blend barrier.
			if (needBarrier)
				gl.blendBarrier();
		}

	// Read the results.
	glw::GLubyte* result = new glw::GLubyte[4 * dim * dim];
	gl.readPixels(0, 0, dim, dim, GL_RGBA, GL_UNSIGNED_BYTE, result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");

	// Check that first pixel is ok.
	tcu::RGBA res		= tcu::RGBA::fromBytes(result);
	tcu::RGBA ref		= pf.convertColor(tcu::RGBA(dstCol));
	tcu::RGBA threshold = pf.getColorThreshold() + pf.getColorThreshold();
	bool	  firstOk   = tcu::compareThreshold(ref, res, threshold);

	// Check that all pixels are the same as the first one.
	bool allSame = true;
	for (int i = 0; i < dim * (dim - 1); i++)
		allSame = allSame && (0 == memcmp(result, result + (i * 4), 4));

	bool pass = firstOk && allSame;
	if (!pass)
	{
		log << TestLog::Message << "Exceeds: " << threshold << " diff:" << tcu::computeAbsDiff(ref, res)
			<< "  res:" << res << "  ref:" << tcu::RGBA(dstCol) << TestLog::EndMessage;
	}

	delete[] result;

	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail (results differ)");
	return STOP;
}

class BlendTestCaseGroup : public deqp::TestCaseGroup
{
public:
	enum QualifierType
	{
		MATCHING_QUALIFIER, // Use single qualifier that matches used blending mode.
		ALL_QUALIFIER		// Use "all_equations" qualifier.
	};

	BlendTestCaseGroup(deqp::Context& context, glu::GLSLVersion glslVersion, QualifierType qualifierType)
		: TestCaseGroup(context, (qualifierType == ALL_QUALIFIER) ? "blend_all" : "blend_specific",
						"Test all added blends.")
		, m_glslVersion(glslVersion)
		, m_useAllQualifier(qualifierType == ALL_QUALIFIER)
	{
		DE_ASSERT(qualifierType == MATCHING_QUALIFIER || qualifierType == ALL_QUALIFIER);
	}

	void init(void)
	{
		// Pump individual modes.
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(s_modes); i++)
		{
			addChild(new BlendTest(m_context, m_glslVersion, s_modes[i], m_useAllQualifier,
								   DE_LENGTH_OF_ARRAY(s_common) / 8, s_common));
		}
	}

private:
	glu::GLSLVersion m_glslVersion;
	bool			 m_useAllQualifier;

	class BlendTest : public deqp::TestCase
	{
	public:
		BlendTest(deqp::Context& context, glu::GLSLVersion glslVersion, glw::GLenum mode, bool useAllQualifier,
				  int numColors, const glw::GLfloat* colors)
			: TestCase(context, (std::string(GetModeStr(mode)) + (useAllQualifier ? "_all_qualifier" : "")).c_str(),
					   "Test new blend modes for correctness.")
			, m_glslVersion(glslVersion)
			, m_mode(mode)
			, m_useAllQualifier(useAllQualifier)
			, m_numColors(numColors)
			, m_colors(colors)
		{
		}

		IterateResult iterate(void);

	private:
		void getTestColors(int index, tcu::Vec4& src, tcu::Vec4& dst) const;
		void getCoordinates(int index, int& x, int& y) const;

		glu::GLSLVersion	m_glslVersion;
		glw::GLenum			m_mode;
		bool				m_useAllQualifier;
		int					m_numColors;
		const glw::GLfloat* m_colors;
	};

	static const glw::GLfloat s_common[46 * 8];
};

// Alpha values for pre-multiplied colors.
static const float A1 = 0.750f; // Between 1    and 0.5
static const float A2 = 0.375f; // Between 0.5  and 0.25
static const float A3 = 0.125f; // Between 0.25 and 0.0

const glw::GLfloat BlendTestCaseGroup::s_common[] = {
	// Test that pre-multiplied is converted correctly.
	// Should not test invalid premultiplied colours (1, 1, 1, 0).
	1.000f, 0.750f, 0.500f, 1.00f, 0.000f, 0.000f, 0.000f, 0.00f, 0.250f, 0.125f, 0.000f, 1.00f, 0.000f, 0.000f, 0.000f,
	0.00f,

	// Test clamping.
	1.000f, 0.750f, 0.500f, 1.00f, -0.125f, -0.125f, -0.125f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f, -0.125f, -0.125f,
	-0.125f, 1.00f, 1.000f, 0.750f, 0.500f, 1.00f, 1.125f, 1.125f, 1.125f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f, 1.125f,
	1.125f, 1.125f, 1.00f,

	// Cobinations that test other branches of blend equations.
	1.000f, 0.750f, 0.500f, 1.00f, 1.000f, 1.000f, 1.000f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f, 1.000f, 1.000f, 1.000f,
	1.00f, 1.000f, 0.750f, 0.500f, 1.00f, 0.500f, 0.500f, 0.500f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f, 0.500f, 0.500f,
	0.500f, 1.00f, 1.000f, 0.750f, 0.500f, 1.00f, 0.250f, 0.250f, 0.250f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f, 0.250f,
	0.250f, 0.250f, 1.00f, 1.000f, 0.750f, 0.500f, 1.00f, 0.125f, 0.125f, 0.125f, 1.00f, 0.250f, 0.125f, 0.000f, 1.00f,
	0.125f, 0.125f, 0.125f, 1.00f, 1.000f, 0.750f, 0.500f, 1.00f, 0.000f, 0.000f, 0.000f, 1.00f, 0.250f, 0.125f, 0.000f,
	1.00f, 0.000f, 0.000f, 0.000f, 1.00f,

	// Above block with few different pre-multiplied alpha values.
	A1 * 1.000f, A1 * 0.750f, A1 * 0.500f, A1 * 1.00f, A1 * 1.000f, A1 * 1.000f, A1 * 1.000f, A1 * 1.00f, A1 * 0.250f,
	A1 * 0.125f, A1 * 0.000f, A1 * 1.00f, A1 * 1.000f, A1 * 1.000f, A1 * 1.000f, A1 * 1.00f, A1 * 1.000f, A1 * 0.750f,
	A1 * 0.500f, A1 * 1.00f, A1 * 0.500f, A1 * 0.500f, A1 * 0.500f, A1 * 1.00f, A1 * 0.250f, A1 * 0.125f, A1 * 0.000f,
	A1 * 1.00f, A1 * 0.500f, A1 * 0.500f, A1 * 0.500f, A1 * 1.00f, A1 * 1.000f, A1 * 0.750f, A1 * 0.500f, A1 * 1.00f,
	A1 * 0.250f, A1 * 0.250f, A1 * 0.250f, A1 * 1.00f, A1 * 0.250f, A1 * 0.125f, A1 * 0.000f, A1 * 1.00f, A1 * 0.250f,
	A1 * 0.250f, A1 * 0.250f, A1 * 1.00f, A1 * 1.000f, A1 * 0.750f, A1 * 0.500f, A1 * 1.00f, A1 * 0.125f, A1 * 0.125f,
	A1 * 0.125f, A1 * 1.00f, A1 * 0.250f, A1 * 0.125f, A1 * 0.000f, A1 * 1.00f, A1 * 0.125f, A1 * 0.125f, A1 * 0.125f,
	A1 * 1.00f, A1 * 1.000f, A1 * 0.750f, A1 * 0.500f, A1 * 1.00f, A1 * 0.000f, A1 * 0.000f, A1 * 0.000f, A1 * 1.00f,
	A1 * 0.250f, A1 * 0.125f, A1 * 0.000f, A1 * 1.00f, A1 * 0.000f, A1 * 0.000f, A1 * 0.000f, A1 * 1.00f,

	A2 * 1.000f, A2 * 0.750f, A2 * 0.500f, A2 * 1.00f, A2 * 1.000f, A2 * 1.000f, A2 * 1.000f, A2 * 1.00f, A2 * 0.250f,
	A2 * 0.125f, A2 * 0.000f, A2 * 1.00f, A2 * 1.000f, A2 * 1.000f, A2 * 1.000f, A2 * 1.00f, A2 * 1.000f, A2 * 0.750f,
	A2 * 0.500f, A2 * 1.00f, A2 * 0.500f, A2 * 0.500f, A2 * 0.500f, A2 * 1.00f, A2 * 0.250f, A2 * 0.125f, A2 * 0.000f,
	A2 * 1.00f, A2 * 0.500f, A2 * 0.500f, A2 * 0.500f, A2 * 1.00f, A2 * 1.000f, A2 * 0.750f, A2 * 0.500f, A2 * 1.00f,
	A2 * 0.250f, A2 * 0.250f, A2 * 0.250f, A2 * 1.00f, A2 * 0.250f, A2 * 0.125f, A2 * 0.000f, A2 * 1.00f, A2 * 0.250f,
	A2 * 0.250f, A2 * 0.250f, A2 * 1.00f, A2 * 1.000f, A2 * 0.750f, A2 * 0.500f, A2 * 1.00f, A2 * 0.125f, A2 * 0.125f,
	A2 * 0.125f, A2 * 1.00f, A2 * 0.250f, A2 * 0.125f, A2 * 0.000f, A2 * 1.00f, A2 * 0.125f, A2 * 0.125f, A2 * 0.125f,
	A2 * 1.00f, A2 * 1.000f, A2 * 0.750f, A2 * 0.500f, A2 * 1.00f, A2 * 0.000f, A2 * 0.000f, A2 * 0.000f, A2 * 1.00f,
	A2 * 0.250f, A2 * 0.125f, A2 * 0.000f, A2 * 1.00f, A2 * 0.000f, A2 * 0.000f, A2 * 0.000f, A2 * 1.00f,

	A3 * 1.000f, A3 * 0.750f, A3 * 0.500f, A3 * 1.00f, A3 * 1.000f, A3 * 1.000f, A3 * 1.000f, A3 * 1.00f, A3 * 0.250f,
	A3 * 0.125f, A3 * 0.000f, A3 * 1.00f, A3 * 1.000f, A3 * 1.000f, A3 * 1.000f, A3 * 1.00f, A3 * 1.000f, A3 * 0.750f,
	A3 * 0.500f, A3 * 1.00f, A3 * 0.500f, A3 * 0.500f, A3 * 0.500f, A3 * 1.00f, A3 * 0.250f, A3 * 0.125f, A3 * 0.000f,
	A3 * 1.00f, A3 * 0.500f, A3 * 0.500f, A3 * 0.500f, A3 * 1.00f, A3 * 1.000f, A3 * 0.750f, A3 * 0.500f, A3 * 1.00f,
	A3 * 0.250f, A3 * 0.250f, A3 * 0.250f, A3 * 1.00f, A3 * 0.250f, A3 * 0.125f, A3 * 0.000f, A3 * 1.00f, A3 * 0.250f,
	A3 * 0.250f, A3 * 0.250f, A3 * 1.00f, A3 * 1.000f, A3 * 0.750f, A3 * 0.500f, A3 * 1.00f, A3 * 0.125f, A3 * 0.125f,
	A3 * 0.125f, A3 * 1.00f, A3 * 0.250f, A3 * 0.125f, A3 * 0.000f, A3 * 1.00f, A3 * 0.125f, A3 * 0.125f, A3 * 0.125f,
	A3 * 1.00f, A3 * 1.000f, A3 * 0.750f, A3 * 0.500f, A3 * 1.00f, A3 * 0.000f, A3 * 0.000f, A3 * 0.000f, A3 * 1.00f,
	A3 * 0.250f, A3 * 0.125f, A3 * 0.000f, A3 * 1.00f, A3 * 0.000f, A3 * 0.000f, A3 * 0.000f, A3 * 1.00f,
};

static tcu::Vec4 MaskChannels(const tcu::PixelFormat& pf, const tcu::Vec4& v)
{
	return tcu::Vec4(pf.redBits > 0 ? v[0] : 0.f, pf.greenBits > 0 ? v[1] : 0.f, pf.blueBits > 0 ? v[2] : 0.f,
					 pf.alphaBits > 0 ? v[3] : 1.f);
}

//
// Quantize the input colour by the colour bit depth for each channel.
//
static tcu::Vec4 QuantizeChannels(const tcu::PixelFormat& pf, const tcu::Vec4& v)
{
	float maxChanel[4] = { static_cast<float>(1 << pf.redBits) - 1.0f, static_cast<float>(1 << pf.greenBits) - 1.0f,
						   static_cast<float>(1 << pf.blueBits) - 1.0f, static_cast<float>(1 << pf.alphaBits) - 1.0f };

	return tcu::Vec4(static_cast<float>((unsigned int)(v[0] * maxChanel[0])) / maxChanel[0],
					 static_cast<float>((unsigned int)(v[1] * maxChanel[1])) / maxChanel[1],
					 static_cast<float>((unsigned int)(v[2] * maxChanel[2])) / maxChanel[2],
					 pf.alphaBits ? static_cast<float>((unsigned int)(v[3] * maxChanel[3])) / maxChanel[3] : 1.0f);
}

void BlendTestCaseGroup::BlendTest::getTestColors(int index, tcu::Vec4& src, tcu::Vec4& dst) const
{
	DE_ASSERT(0 <= index && index < m_numColors);

	const tcu::RenderTarget& rt = m_context.getRenderContext().getRenderTarget();
	const tcu::PixelFormat&  pf = rt.getPixelFormat();
	const glw::GLfloat*		 s  = m_colors + 8 * index;

	src = MaskChannels(pf, tcu::Vec4(s[0], s[1], s[2], s[3]));
	dst = MaskChannels(pf, tcu::Vec4(s[4], s[5], s[6], s[7]));
	src = tcu::clamp(src, tcu::Vec4(0.f), tcu::Vec4(1.f));
	dst = tcu::clamp(dst, tcu::Vec4(0.f), tcu::Vec4(1.f));

	// Quantize the destination channels
	// this matches what implementation does on render target write
	dst = QuantizeChannels(pf, dst);
}

void BlendTestCaseGroup::BlendTest::getCoordinates(int index, int& x, int& y) const
{
	const tcu::RenderTarget& rt = m_context.getRenderContext().getRenderTarget();
	y							= index / rt.getWidth();
	x							= index % rt.getWidth();
}

BlendTestCaseGroup::BlendTest::IterateResult BlendTestCaseGroup::BlendTest::iterate(void)
{
	const tcu::RenderTarget& rt  = m_context.getRenderContext().getRenderTarget();
	const tcu::PixelFormat&  pf  = rt.getPixelFormat();
	const glw::Functions&	gl  = m_context.getRenderContext().getFunctions();
	TestLog&				 log = m_testCtx.getLog();

	// Check that extension is supported.
	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}

	// Setup program.
	std::string frgSrc =
		GetSolidShader(m_useAllQualifier ? "blend_support_all_equations" : GetLayoutQualifierStr(m_mode),
					   glu::getGLSLVersionDeclaration(m_glslVersion));
	glu::ShaderProgram p(m_context.getRenderContext(),
						 glu::makeVtxFragSources(GetDef2DVtxSrc(m_glslVersion).c_str(), frgSrc.c_str()));
	if (!p.isOk())
	{
		log << p;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(p.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

	glu::VertexArrayBinding posBinding = glu::va::Float("aPos", 2, 4, 0, &s_pos[0]);

	// Enable blending and set blend equation.
	gl.disable(GL_DITHER);
	gl.enable(GL_SCISSOR_TEST);
	gl.enable(GL_BLEND);
	gl.blendEquation(m_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BlendEquation failed");

	bool needBarrier = !IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced_coherent");

	// Render loop.
	for (int colorIndex = 0; colorIndex < m_numColors; colorIndex++)
	{
		tcu::Vec4 srcCol, dstCol;
		getTestColors(colorIndex, srcCol, dstCol);

		// Get pixel to blend.
		int x, y;
		getCoordinates(colorIndex, x, y);
		gl.scissor(x, y, 1, 1);

		// Clear to destination color.
		gl.clearColor(dstCol[0], dstCol[1], dstCol[2], dstCol[3]);
		gl.clear(GL_COLOR_BUFFER_BIT);
		if (needBarrier)
			gl.blendBarrier();

		// Set source color.
		gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uSrcCol"), srcCol[0], srcCol[1], srcCol[2], srcCol[3]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Uniforms failed");

		// Draw.
		glu::draw(m_context.getRenderContext(), p.getProgram(), 1, &posBinding,
				  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");
		if (needBarrier)
			gl.blendBarrier();
	}

	// Read the results.
	const int	 w			  = rt.getWidth();
	const int	 h			  = rt.getHeight();
	glw::GLubyte* resultBytes = new glw::GLubyte[4 * w * h];
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.readPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, resultBytes);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");

	bool pass = true;
	for (int colorIndex = 0; colorIndex < m_numColors; colorIndex++)
	{
		tcu::Vec4 srcCol, dstCol;
		getTestColors(colorIndex, srcCol, dstCol);

		// Get result and calculate reference.
		int x, y;
		getCoordinates(colorIndex, x, y);

		tcu::Vec4 refCol	= Blend(m_mode, srcCol, dstCol);
		tcu::RGBA ref		= pf.convertColor(tcu::RGBA(refCol));
		tcu::RGBA res		= tcu::RGBA::fromBytes(resultBytes + 4 * (x + w * y));
		tcu::RGBA tmp		= pf.getColorThreshold();
		tcu::RGBA threshold = tcu::RGBA(std::min(2 + 2 * tmp.getRed(), 255), std::min(2 + 2 * tmp.getGreen(), 255),
										std::min(2 + 2 * tmp.getBlue(), 255), std::min(2 + 2 * tmp.getAlpha(), 255));
		bool pixelOk = tcu::compareThreshold(ref, res, threshold);
		pass		 = pass && pixelOk;
		if (!pixelOk)
		{
			log << TestLog::Message << "(" << x << "," << y << ")  "
				<< "(" << colorIndex << ") "
				<< "Exceeds: " << threshold << " diff:" << tcu::computeAbsDiff(ref, res) << "  res:" << res
				<< "  ref:" << ref << "  dst:" << tcu::RGBA(dstCol) << "  src:" << tcu::RGBA(srcCol)
				<< TestLog::EndMessage;
		}
	}

	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail (results differ)");
	delete[] resultBytes;
	return STOP;
}

/*
 * From 'Other' part of the spec:
 *    "Test different behaviors for GLSL #extension
 *     GL_XXX_blend_equation_advanced"
 *
 * - require : Covered by "Blend" tests.
 * - enable  : Use layout modifier from GL_KHR_blend_equation_advanced and
 *             expect compile to succeed. (warn if not supported)
 * - warn    : Use layout modifier from GL_KHR_blend_equation_advanced and
 *             expect compile to succeed. (work, but issue warning)
 * - disable : Use layout modifier from GL_KHR_blend_equation_advanced and
 *             expect compile to fail with error.
 *
 */
class ExtensionDirectiveTestCaseGroup : public deqp::TestCaseGroup
{
public:
	ExtensionDirectiveTestCaseGroup(deqp::Context& context, glu::GLSLVersion glslVersion)
		: TestCaseGroup(context, "extension_directive", "Test #extension directive."), m_glslVersion(glslVersion)
	{
	}

	void init(void)
	{
		addChild(new ExtensionDirectiveTestCase(m_context, m_glslVersion, "disable"));
		addChild(new ExtensionDirectiveTestCase(m_context, m_glslVersion, "enable"));
		addChild(new ExtensionDirectiveTestCase(m_context, m_glslVersion, "warn"));
	}

private:
	class ExtensionDirectiveTestCase : public deqp::TestCase
	{
	public:
		ExtensionDirectiveTestCase(deqp::Context& context, glu::GLSLVersion glslVersion, const char* behaviour)
			: TestCase(context, (std::string("extension_directive_") + behaviour).c_str(), "Test #extension directive.")
			, m_glslVersion(glslVersion)
			, m_behaviourStr(behaviour)
		{
			// Initialize expected compiler behaviour.
			std::string b(behaviour);
			if (b == "disable")
			{
				m_requireInfoLog = true;
				m_requireCompile = false;
			}
			else if (b == "enable")
			{
				m_requireInfoLog = false;
				m_requireCompile = true;
			}
			else
			{
				DE_ASSERT(b == "warn");
				m_requireInfoLog = false;
				m_requireCompile = true;
			}
		}

		IterateResult iterate(void)
		{
			const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
			TestLog&			  log = m_testCtx.getLog();
			const int			  dim = 4;

			// Check that extension is supported.
			if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
				return STOP;
			}

			FBOSentry fbo(gl, dim, dim, GL_RGBA8);

			tcu::Vec4 dstCol(1.f, 1.f, 1.f, 1.f);
			tcu::Vec4 srcCol(0.f, 0.f, 0.f, 1.f);

			// Clear to destination color.
			gl.clearColor(dstCol.x(), dstCol.y(), dstCol.z(), dstCol.w());
			gl.clear(GL_COLOR_BUFFER_BIT);

			// Setup program.
			std::string directive = "#extension GL_KHR_blend_equation_advanced : " + m_behaviourStr;
			std::string frgSrc = GetSolidShader("blend_support_multiply", glu::getGLSLVersionDeclaration(m_glslVersion),
												directive.c_str());
			glu::ShaderProgram p(m_context.getRenderContext(),
								 glu::makeVtxFragSources(GetDef2DVtxSrc(m_glslVersion).c_str(), frgSrc.c_str()));
			// If check that there is some info log if it is expected.
			const bool infoLogOk =
				m_requireInfoLog ? p.getShaderInfo(glu::SHADERTYPE_FRAGMENT).infoLog.size() > 0 : true;
			if (!p.isOk())
			{
				if (m_requireCompile)
				{
					log << p;
					TCU_FAIL("Compile failed");
				}
				else
				{
					// If shader was expected to fail, so assume info log has something.
					bool pass = infoLogOk;
					m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL,
											pass ? "Pass" : "Fail. Expected info log.");
				}
				return STOP;
			}
			gl.useProgram(p.getProgram());
			GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

			// Program ok, check whether info log was as expected.
			if (!infoLogOk)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail. No warnings were generated.");
				return STOP;
			}

			// Enable blending and set blend equation.
			gl.disable(GL_DITHER);
			gl.enable(GL_BLEND);
			gl.blendEquation(GL_MULTIPLY_KHR);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BlendEquation failed");

			// Setup source color.
			gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uSrcCol"), srcCol.x(), srcCol.y(), srcCol.z(),
						 srcCol.w());
			GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform failed");

			glu::VertexArrayBinding posBinding = glu::va::Float("aPos", 2, 4, 0, &s_pos[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Attributes failed");

			glu::draw(m_context.getRenderContext(), p.getProgram(), 1, &posBinding,
					  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));
			GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

			// Check the result to see that extension was actually enabled.
			glw::GLubyte result[4] = { 1, 2, 3, 4 };
			gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");
			bool pass = tcu::RGBA::fromBytes(result) == tcu::RGBA(0, 0, 0, 0xFF);
			m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");

			return STOP;
		}

	private:
		glu::GLSLVersion m_glslVersion;
		std::string		 m_behaviourStr;
		bool			 m_requireInfoLog;
		bool			 m_requireCompile;
	};

	glu::GLSLVersion m_glslVersion;
};

/*
 * From 'Other' part of the spec:
 *    "If XXX_blend_equation_advanced_coherent is supported, test
 *     Each blending mode needs to be tested without specifying the proper
 *     blend_support_[mode] or blend_support_all layout qualifier in the
 *     fragment shader. Expect INVALID_OPERATION GL error after calling
 *     DrawElements/Arrays."
 */
class MissingQualifierTestGroup : public deqp::TestCaseGroup
{
public:
	enum MissingType
	{
		MISMATCH, // wrong qualifier in the shader.
		MISSING,  // no qualifier at all.
	};

	MissingQualifierTestGroup(deqp::Context& context, glu::GLSLVersion glslVersion, MissingType missingType)
		: TestCaseGroup(context, missingType == MISMATCH ? "mismatching_qualifier" : "missing_qualifier", "")
		, m_glslVersion(glslVersion)
		, m_missingType(missingType)
	{
	}

	void init(void)
	{
		// Pump individual modes.
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(s_modes); i++)
		{
			const char* qualifier = m_missingType == MISSING ?
										DE_NULL :
										GetLayoutQualifierStr(s_modes[(i + 1) % DE_LENGTH_OF_ARRAY(s_modes)]);
			addChild(new MissingCase(m_context, m_glslVersion, s_modes[i], qualifier));
		}
	}

private:
	class MissingCase : public deqp::TestCase
	{
	public:
		MissingCase(deqp::Context& context, glu::GLSLVersion glslVersion, glw::GLenum mode, const char* layoutQualifier)
			: TestCase(context, GetModeStr(mode), "")
			, m_glslVersion(glslVersion)
			, m_mode(mode)
			, m_layoutQualifier(layoutQualifier)
		{
		}

		IterateResult iterate(void);

	private:
		glu::GLSLVersion m_glslVersion;
		glw::GLenum		 m_mode;
		const char*		 m_layoutQualifier; // NULL => no qualifier at all.
	};

	glu::GLSLVersion m_glslVersion;
	MissingType		 m_missingType;
};

MissingQualifierTestGroup::MissingCase::IterateResult MissingQualifierTestGroup::MissingCase::iterate(void)
{
	const int			  dim = 4;
	const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
	TestLog&			  log = m_testCtx.getLog();

	// Check that extension is supported.
	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}

	FBOSentry fbo(gl, dim, dim, GL_RGBA8);

	tcu::Vec4 dstCol(1.f, 1.f, 1.f, 1.f);
	tcu::Vec4 srcCol(0.f, 0.f, 0.f, 1.f);

	// Clear to destination color.
	gl.clearColor(dstCol.x(), dstCol.y(), dstCol.z(), dstCol.w());
	gl.clear(GL_COLOR_BUFFER_BIT);

	// Setup program.
	glu::ShaderProgram p(m_context.getRenderContext(),
						 glu::makeVtxFragSources(
							 GetDef2DVtxSrc(m_glslVersion).c_str(),
							 GetSolidShader(m_layoutQualifier, glu::getGLSLVersionDeclaration(m_glslVersion)).c_str()));
	if (!p.isOk())
	{
		log << p;
		TCU_FAIL("Compile failed");
	}

	gl.useProgram(p.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

	// Enable blending and set blend equation.
	gl.disable(GL_DITHER);
	gl.enable(GL_BLEND);
	gl.blendEquation(m_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BlendEquation failed");

	// Setup source color.
	gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uSrcCol"), srcCol.x(), srcCol.y(), srcCol.z(), srcCol.w());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniform failed");

	glu::VertexArrayBinding posBinding = glu::va::Float("aPos", 2, 4, 0, &s_pos[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Attributes failed");

	glu::draw(m_context.getRenderContext(), p.getProgram(), 1, &posBinding,
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));

	glw::GLenum error = gl.getError();
	bool		pass  = (error == GL_INVALID_OPERATION);

	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");

	return STOP;
}

/*
 * From 'Other' part of the spec:
 *    "If XXX_blend_equation_advanced_coherent is supported, test
 *     BLEND_ADVANCED_COHERENT_XXX setting:
 *     - The setting should work with Enable, Disable and IsEnable without producing errors
 *     - Default value should be TRUE"
 *
 *  1. Test that coherent is enabled by default.
 *  2. Disable and check the state.
 *  3. Enable and check the state and test that rendering does not produce errors.
 */

class CoherentEnableCaseGroup : public deqp::TestCaseGroup
{
public:
	CoherentEnableCaseGroup(deqp::Context& context) : TestCaseGroup(context, "coherent", "")
	{
	}

	void init(void)
	{
		addChild(new CoherentEnableCase(m_context));
	}

private:
	class CoherentEnableCase : public deqp::TestCase
	{
	public:
		CoherentEnableCase(deqp::Context& context) : TestCase(context, "enableDisable", "")
		{
		}
		IterateResult iterate(void);
	};
};

CoherentEnableCaseGroup::CoherentEnableCase::IterateResult CoherentEnableCaseGroup::CoherentEnableCase::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// Check that extension is supported.
	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}
	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced_coherent"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced_coherent");
		return STOP;
	}

	std::vector<bool> res;
	// Enabled by default.
	res.push_back(gl.isEnabled(GL_BLEND_ADVANCED_COHERENT_KHR) == GL_TRUE);
	res.push_back(gl.getError() == GL_NO_ERROR);

	// Check disabling.
	gl.disable(GL_BLEND_ADVANCED_COHERENT_KHR);
	res.push_back(gl.getError() == GL_NO_ERROR);
	res.push_back(gl.isEnabled(GL_BLEND_ADVANCED_COHERENT_KHR) == GL_FALSE);
	res.push_back(gl.getError() == GL_NO_ERROR);

	// Check enabling.
	gl.enable(GL_BLEND_ADVANCED_COHERENT_KHR);
	res.push_back(gl.getError() == GL_NO_ERROR);
	res.push_back(gl.isEnabled(GL_BLEND_ADVANCED_COHERENT_KHR) == GL_TRUE);
	res.push_back(gl.getError() == GL_NO_ERROR);

	// Pass if no failures found.
	bool pass = std::find(res.begin(), res.end(), false) == res.end();

	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");

	return STOP;
}
/*
 * From 'Other' part of the spec:
 *    "Test that rendering into more than one color buffers at once produces
 *     INVALID_OPERATION error when calling drawArrays/drawElements"
 */
class MRTCaseGroup : public deqp::TestCaseGroup
{
public:
	MRTCaseGroup(deqp::Context& context, glu::GLSLVersion glslVersion)
		: TestCaseGroup(context, "MRT", "GL_KHR_blend_equation_advanced"), m_glslVersion(glslVersion)
	{
	}

	void init(void)
	{
		addChild(new MRTCase(m_context, m_glslVersion, MRTCase::ARRAY));
		addChild(new MRTCase(m_context, m_glslVersion, MRTCase::SEPARATE));
	}

private:
	class MRTCase : public deqp::TestCase
	{
	public:
		enum DeclarationType
		{
			ARRAY,
			SEPARATE
		};

		MRTCase(deqp::Context& context, glu::GLSLVersion glslVersion, DeclarationType declType)
			: TestCase(context, (declType == ARRAY ? "MRT_array" : "MRT_separate"), "GL_KHR_blend_equation_advanced")
			, m_glslVersion(glslVersion)
			, m_declarationType(declType)
		{
			DE_ASSERT(m_declarationType == ARRAY || m_declarationType == SEPARATE);
		}

		IterateResult iterate(void);

	private:
		glu::GLSLVersion m_glslVersion;
		DeclarationType  m_declarationType;
	};

	glu::GLSLVersion m_glslVersion;
};

MRTCaseGroup::MRTCase::IterateResult MRTCaseGroup::MRTCase::iterate(void)
{
	TestLog&			  log = m_testCtx.getLog();
	const glw::Functions& gl  = m_context.getRenderContext().getFunctions();

	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}

	static const char* frgSrcTemplateArray = "${VERSION_DIRECTIVE}\n"
											 "#extension GL_KHR_blend_equation_advanced : require\n"
											 "\n"
											 "precision highp float;\n"
											 "layout (blend_support_multiply) out;\n"
											 "layout (location = 0) out vec4 oCol[2];\n"
											 "\n"
											 "uniform vec4 uMultCol;\n"
											 "\n"
											 "void main (void) {\n"
											 "   oCol[0] = uMultCol;\n"
											 "   oCol[1] = uMultCol;\n"
											 "}\n";

	static const char* frgSrcTemplateSeparate = "${VERSION_DIRECTIVE}\n"
												"#extension GL_KHR_blend_equation_advanced : require\n"
												"\n"
												"precision highp float;\n"
												"layout (blend_support_multiply) out;\n"
												"layout (location = 0) out vec4 oCol0;\n"
												"layout (location = 1) out vec4 oCol1;\n"
												"\n"
												"uniform vec4 uMultCol;\n"
												"\n"
												"void main (void) {\n"
												"   oCol0 = uMultCol;\n"
												"   oCol1 = uMultCol;\n"
												"}\n";

	static const char* frgSrcTemplate = m_declarationType == ARRAY ? frgSrcTemplateArray : frgSrcTemplateSeparate;

	std::map<std::string, std::string> args;
	args["VERSION_DIRECTIVE"] = glu::getGLSLVersionDeclaration(m_glslVersion);
	std::string frgSrc		  = tcu::StringTemplate(frgSrcTemplate).specialize(args);

	FBOSentry fbo(gl, 4, 4, GL_RGBA8, GL_RGBA8);

	static const glw::GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	gl.drawBuffers(2, bufs);

	// Clear buffers to white.
	gl.clearColor(1.f, 1.f, 1.f, 1.f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// Setup program.
	glu::ShaderProgram p(m_context.getRenderContext(),
						 glu::makeVtxFragSources(GetDef2DVtxSrc(m_glslVersion).c_str(), frgSrc.c_str()));
	if (!p.isOk())
	{
		log << p;
		TCU_FAIL("Compile failed");
	}

	gl.useProgram(p.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

	// Enable blending and set blend equation.
	gl.disable(GL_DITHER);
	gl.enable(GL_BLEND);
	gl.blendEquation(GL_DARKEN_KHR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BlendEquation failed");

	// Multiply with zero.
	gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uMultCol"), 0.f, 0.f, 0.f, 1.00f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniforms failed");

	// Set vertex buffer
	glw::GLuint vbo;
	gl.genBuffers(1, &vbo);
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(s_pos), s_pos, GL_STATIC_DRAW);

	// Set vertices.
	glw::GLuint vao;
	gl.genVertexArrays(1, &vao);
	gl.bindVertexArray(vao);
	glw::GLint loc = gl.getAttribLocation(p.getProgram(), "aPos");
	gl.enableVertexAttribArray(loc);
	gl.vertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 8, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Attributes failed");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	bool errorOk = (gl.getError() == GL_INVALID_OPERATION);

	gl.drawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, s_indices);
	errorOk = errorOk && (gl.getError() == GL_INVALID_OPERATION);

	if (!errorOk)
		log << TestLog::Message << "DrawArrays/DrawElements didn't produce error." << TestLog::EndMessage;

	// Expect unaltered destination pixels.
	bool contentsOk = true;
	for (int i = 0; i < 2; i++)
	{
		glw::GLubyte result[4] = { 1, 2, 3, 4 };
		gl.readBuffer(bufs[0]);
		gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");
		if (tcu::RGBA::fromBytes(result) != tcu::RGBA::white())
		{
			contentsOk = false;
			log << TestLog::Message << "Buffer " << i << " "
				<< "contents changed: " << tcu::RGBA::fromBytes(result) << " expected:" << tcu::RGBA::white()
				<< TestLog::EndMessage;
		}
	}

	gl.deleteVertexArrays(1, &vao);
	gl.deleteBuffers(1, &vbo);

	bool pass = errorOk && contentsOk;
	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");
	return STOP;
}

/*
 * From "Other" part of the spec:
 *    "Test that the new blending modes cannot be used with
 *     BlendEquationSeparate(i). Expect INVALID_ENUM GL error"
 *
 * Tests that BlendEquationSeparate does not accept extension's blending modes
 * either in rgb or alpha parameter.
 */
class BlendEquationSeparateCase : public deqp::TestCaseGroup
{
public:
	BlendEquationSeparateCase(deqp::Context& context)
		: TestCaseGroup(context, "BlendEquationSeparate",
						"Test that advanced blend modes are correctly rejected from glBlendEquationSeparate.")
	{
	}

	void init(void)
	{
		// Pump individual modes.
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(s_modes); i++)
			addChild(new ModeCase(m_context, s_modes[i]));
	}

private:
	class ModeCase : public deqp::TestCase
	{
	public:
		ModeCase(deqp::Context& context, glw::GLenum mode)
			: TestCase(context, GetModeStr(mode), "Test one mode"), m_mode(mode)
		{
		}

		IterateResult iterate(void)
		{
			// Check that extension is supported.
			if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
				return STOP;
			}

			const glw::Functions& gl = m_context.getRenderContext().getFunctions();

			// Set separate blend equations.
			// Expect error and that default value (FUNC_ADD) is not changed.

			// RGB.
			gl.blendEquationSeparate(m_mode, GL_FUNC_ADD);
			bool	   rgbOk = gl.getError() == GL_INVALID_ENUM;
			glw::GLint rgbEq = GL_NONE;
			gl.getIntegerv(GL_BLEND_EQUATION_RGB, &rgbEq);
			rgbOk = rgbOk && (rgbEq == GL_FUNC_ADD);

			// Alpha.
			gl.blendEquationSeparate(GL_FUNC_ADD, m_mode);
			bool	   alphaOk = gl.getError() == GL_INVALID_ENUM;
			glw::GLint alphaEq = GL_NONE;
			gl.getIntegerv(GL_BLEND_EQUATION_ALPHA, &alphaEq);
			alphaOk = alphaOk && (alphaEq == GL_FUNC_ADD);

			bool pass = rgbOk && alphaOk;
			m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");
			return STOP;
		}

	private:
		glw::GLenum m_mode;
	};
};

/*
 *  From "Other" part of the spec:
 *     "Check that GLSL GL_KHR_blend_equation_advanced #define exists and is 1"
 *
 *  Test that regardless of extension directive the definition exists and has value 1.
 */

class PreprocessorCaseGroup : public deqp::TestCaseGroup
{
public:
	PreprocessorCaseGroup(deqp::Context& context, glu::GLSLVersion glslVersion)
		: TestCaseGroup(context, "preprocessor", "GL_KHR_blend_equation_advanced"), m_glslVersion(glslVersion)
	{
	}

	void init(void)
	{
		addChild(new PreprocessorCase(m_context, m_glslVersion, DE_NULL));
		addChild(new PreprocessorCase(m_context, m_glslVersion, "require"));
		addChild(new PreprocessorCase(m_context, m_glslVersion, "enable"));
		addChild(new PreprocessorCase(m_context, m_glslVersion, "warn"));
		addChild(new PreprocessorCase(m_context, m_glslVersion, "disable"));
	}

private:
	class PreprocessorCase : public deqp::TestCase
	{
	public:
		PreprocessorCase(deqp::Context& context, glu::GLSLVersion glslVersion, const char* behaviour)
			: TestCase(context, behaviour ? behaviour : "none", "GL_KHR_blend_equation_advanced")
			, m_glslVersion(glslVersion)
			, m_behaviour(behaviour)
		{
		}

		IterateResult iterate(void);

	private:
		glu::GLSLVersion m_glslVersion;
		const char*		 m_behaviour;
	};

	glu::GLSLVersion m_glslVersion;
};

PreprocessorCaseGroup::PreprocessorCase::IterateResult PreprocessorCaseGroup::PreprocessorCase::iterate(void)
{
	TestLog&			  log = m_testCtx.getLog();
	const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
	const int			  dim = 4;

	if (!IsExtensionSupported(m_context, "GL_KHR_blend_equation_advanced"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_KHR_blend_equation_advanced");
		return STOP;
	}

	FBOSentry fbo(gl, dim, dim, GL_RGBA8);
	gl.clearColor(0.125f, 0.125f, 0.125f, 1.f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	// Test that GL_KHR_blend_equation_advanced is defined and it has value 1.
	// Renders green pixels if above is true, red pixels otherwise.
	static const char* frgSrcTemplate = "${VERSION_DIRECTIVE}\n"
										"${EXTENSION_DIRECTIVE}\n"
										"precision highp float;\n"
										"\n"
										"uniform vec4 uDefined;\n"
										"uniform vec4 uNonDefined;\n"
										"\n"
										"uniform int  uValue;\n"
										"\n"
										"layout(location = 0) out vec4 oCol;\n"
										"\n"
										"void main (void) {\n"
										"    vec4 col = uNonDefined;\n"
										"#if defined(GL_KHR_blend_equation_advanced)\n"
										"    int val = GL_KHR_blend_equation_advanced;\n"
										"    if (uValue == val) {\n"
										"        col = uDefined;\n"
										"    }\n"
										"#endif\n"
										"    oCol = col;\n"
										"}\n";

	std::map<std::string, std::string> args;
	args["VERSION_DIRECTIVE"] = glu::getGLSLVersionDeclaration(m_glslVersion);
	if (m_behaviour)
		args["EXTENSION_DIRECTIVE"] = std::string("#extension GL_KHR_blend_equation_advanced : ") + m_behaviour;
	else
		args["EXTENSION_DIRECTIVE"] = "";
	std::string frgSrc				= tcu::StringTemplate(frgSrcTemplate).specialize(args);

	glu::ShaderProgram p(m_context.getRenderContext(),
						 glu::makeVtxFragSources(GetDef2DVtxSrc(m_glslVersion).c_str(), frgSrc.c_str()));
	if (!p.isOk())
	{
		log << p;
		TCU_FAIL("Compile failed");
	}
	gl.useProgram(p.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Program failed");

	gl.uniform1i(gl.getUniformLocation(p.getProgram(), "uValue"), 1);
	gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uDefined"), 0.f, 1.f, 0.f, 1.f);
	gl.uniform4f(gl.getUniformLocation(p.getProgram(), "uNonDefined"), 1.f, 0.f, 1.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uniforms failed");

	glu::VertexArrayBinding posBinding = glu::va::Float("aPos", 2, 4, 0, &s_pos[0]);
	glu::draw(m_context.getRenderContext(), p.getProgram(), 1, &posBinding,
			  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(s_indices), &s_indices[0]));
	GLU_EXPECT_NO_ERROR(gl.getError(), "Draw failed");

	// Check the results.
	tcu::Surface resultSurface(dim, dim);
	glu::readPixels(m_context.getRenderContext(), 0, 0, resultSurface.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Read pixels failed");
	bool pass = tcu::RGBA::green() == resultSurface.getPixel(0, 0);

	m_testCtx.setTestResult(pass ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, pass ? "Pass" : "Fail");

	return STOP;
}

BlendEquationAdvancedTests::BlendEquationAdvancedTests(deqp::Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "blend_equation_advanced", "KHR_blend_equation_advanced tests"), m_glslVersion(glslVersion)
{
}

BlendEquationAdvancedTests::~BlendEquationAdvancedTests(void)
{
}

void BlendEquationAdvancedTests::init(void)
{
	// Test that enable/disable and getting status works.
	addChild(new CoherentEnableCaseGroup(m_context));

	// Test that preprocessor macro GL_KHR_blend_equation_advanced
	// is always defined and its value is 1.
	addChild(new PreprocessorCaseGroup(m_context, m_glslVersion));

	// Test that BlendEquationSeparate rejects advanced blend modes.
	addChild(new BlendEquationSeparateCase(m_context));

	// Test that advanced blend equations cannot be used with multiple render targets.
	addChild(new MRTCaseGroup(m_context, m_glslVersion));

	// Test that using new blend modes produce errors if appropriate qualifier
	// is not in the shader (test without any blend qualifier and with mismatching qualifier).
	addChild(new MissingQualifierTestGroup(m_context, m_glslVersion, MissingQualifierTestGroup::MISMATCH));
	addChild(new MissingQualifierTestGroup(m_context, m_glslVersion, MissingQualifierTestGroup::MISSING));

	// Test #extension directive behaviour.
	// Case "require" is tested indirectly by blending tests.
	addChild(new ExtensionDirectiveTestCaseGroup(m_context, m_glslVersion));

	// Test that each blend mode produces correct results.
	addChild(new BlendTestCaseGroup(m_context, m_glslVersion, BlendTestCaseGroup::ALL_QUALIFIER));
	addChild(new BlendTestCaseGroup(m_context, m_glslVersion, BlendTestCaseGroup::MATCHING_QUALIFIER));

	// Test that coherent blending or barrier works.
	addChild(new CoherentBlendTestCaseGroup(m_context, m_glslVersion));
}

} // glcts
