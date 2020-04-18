/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief Negative Advanced Blend Equation Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeAdvancedBlendEquationTests.hpp"

#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{
namespace
{

enum BlendEquation
{
	BLEND_EQUATION_MULTIPLY = 0,
	BLEND_EQUATION_SCREEN,
	BLEND_EQUATION_OVERLAY,
	BLEND_EQUATION_DARKEN,
	BLEND_EQUATION_LIGHTEN,
	BLEND_EQUATION_COLORDODGE,
	BLEND_EQUATION_COLORBURN,
	BLEND_EQUATION_HARDLIGHT,
	BLEND_EQUATION_SOFTLIGHT,
	BLEND_EQUATION_DIFFERENCE,
	BLEND_EQUATION_EXCLUSION,
	BLEND_EQUATION_HSL_HUE,
	BLEND_EQUATION_HSL_SATURATION,
	BLEND_EQUATION_HSL_COLOR,
	BLEND_EQUATION_HSL_LUMINOSITY,
	BLEND_EQUATION_ALL_EQUATIONS,

	BLEND_EQUATION_LAST
};

static const BlendEquation s_equations[] =
{
	BLEND_EQUATION_MULTIPLY,
	BLEND_EQUATION_SCREEN,
	BLEND_EQUATION_OVERLAY,
	BLEND_EQUATION_DARKEN,
	BLEND_EQUATION_LIGHTEN,
	BLEND_EQUATION_COLORDODGE,
	BLEND_EQUATION_COLORBURN,
	BLEND_EQUATION_HARDLIGHT,
	BLEND_EQUATION_SOFTLIGHT,
	BLEND_EQUATION_DIFFERENCE,
	BLEND_EQUATION_EXCLUSION,
	BLEND_EQUATION_HSL_HUE,
	BLEND_EQUATION_HSL_SATURATION,
	BLEND_EQUATION_HSL_COLOR,
	BLEND_EQUATION_HSL_LUMINOSITY
};

std::string getShaderLayoutEquation (BlendEquation equation)
{
	switch (equation)
	{
		case BLEND_EQUATION_MULTIPLY:          return "blend_support_multiply";
		case BLEND_EQUATION_SCREEN:            return "blend_support_screen";
		case BLEND_EQUATION_OVERLAY:           return "blend_support_overlay";
		case BLEND_EQUATION_DARKEN:            return "blend_support_darken";
		case BLEND_EQUATION_LIGHTEN:           return "blend_support_lighten";
		case BLEND_EQUATION_COLORDODGE:        return "blend_support_colordodge";
		case BLEND_EQUATION_COLORBURN:         return "blend_support_colorburn";
		case BLEND_EQUATION_HARDLIGHT:         return "blend_support_hardlight";
		case BLEND_EQUATION_SOFTLIGHT:         return "blend_support_softlight";
		case BLEND_EQUATION_DIFFERENCE:        return "blend_support_difference";
		case BLEND_EQUATION_EXCLUSION:         return "blend_support_exclusion";
		case BLEND_EQUATION_HSL_HUE:           return "blend_support_hsl_hue";
		case BLEND_EQUATION_HSL_SATURATION:    return "blend_support_hsl_saturation";
		case BLEND_EQUATION_HSL_COLOR:         return "blend_support_hsl_color";
		case BLEND_EQUATION_HSL_LUMINOSITY:    return "blend_support_hsl_luminosity";
		case BLEND_EQUATION_ALL_EQUATIONS:     return "blend_support_all_equations";
		default:
			DE_FATAL("Equation not supported.");
	}
	return DE_NULL;
}

glw::GLenum getEquation (BlendEquation equation)
{
	switch (equation)
	{
		case BLEND_EQUATION_MULTIPLY:          return GL_MULTIPLY;
		case BLEND_EQUATION_SCREEN:            return GL_SCREEN;
		case BLEND_EQUATION_OVERLAY:           return GL_OVERLAY;
		case BLEND_EQUATION_DARKEN:            return GL_DARKEN;
		case BLEND_EQUATION_LIGHTEN:           return GL_LIGHTEN;
		case BLEND_EQUATION_COLORDODGE:        return GL_COLORDODGE;
		case BLEND_EQUATION_COLORBURN:         return GL_COLORBURN;
		case BLEND_EQUATION_HARDLIGHT:         return GL_HARDLIGHT;
		case BLEND_EQUATION_SOFTLIGHT:         return GL_SOFTLIGHT;
		case BLEND_EQUATION_DIFFERENCE:        return GL_DIFFERENCE;
		case BLEND_EQUATION_EXCLUSION:         return GL_EXCLUSION;
		case BLEND_EQUATION_HSL_HUE:           return GL_HSL_HUE;
		case BLEND_EQUATION_HSL_SATURATION:    return GL_HSL_SATURATION;
		case BLEND_EQUATION_HSL_COLOR:         return GL_HSL_COLOR;
		case BLEND_EQUATION_HSL_LUMINOSITY:    return GL_HSL_LUMINOSITY;
		default:
			DE_FATAL("Equation not supported.");
	}
	return DE_NULL;
}

std::string generateVertexShaderSource (NegativeTestContext& ctx)
{
	const bool				supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version	= supportsES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	std::ostringstream		source;

	source	<< glu::getGLSLVersionDeclaration(version) << "\n"
			<< "void main ()\n"
			<< "{\n"
			<< "	gl_Position = vec4(0.0);\n"
			<< "}\n";

	return source.str();
}

std::string generateFragmentShaderSource (NegativeTestContext& ctx, BlendEquation equation)
{
	const bool				supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version	= supportsES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	std::ostringstream		source;

	source	<< glu::getGLSLVersionDeclaration(version) << "\n"
			<< (supportsES32 ? "" : "#extension GL_KHR_blend_equation_advanced : enable\n")
			<< "layout(" << getShaderLayoutEquation(equation) << ") out;\n"
			<< "layout(location=0) out mediump vec4 o_color;\n"
			<< "void main ()\n"
			<< "{\n"
			<< "	o_color = vec4(0);\n"
			<< "}\n";

	return source.str();
}

glu::ProgramSources generateProgramSources (NegativeTestContext& ctx, BlendEquation equation)
{
	return glu::ProgramSources()
		<< glu::VertexSource(generateVertexShaderSource(ctx))
		<< glu::FragmentSource(generateFragmentShaderSource(ctx, equation));
}

void blend_qualifier_mismatch (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		ctx.isExtensionSupported("GL_KHR_blend_equation_advanced") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)),
		"This test requires support for the extension GL_KHR_blend_equation_advanced or context version 3.2 or higher.");

	ctx.beginSection("GL_INVALID_OPERATION is generated if blending is enabled, and the blend qualifier is different from blend_support_all_equations and does not match the blend equation.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_equations); ++ndx)
	{
		glu::ShaderProgram program(ctx.getRenderContext(), generateProgramSources(ctx, s_equations[ndx]));

		ctx.getLog() << program;
		TCU_CHECK(program.isOk());

		ctx.glUseProgram(program.getProgram());
		ctx.expectError(GL_NO_ERROR);

		for (int ndx2 = 0; ndx2 < DE_LENGTH_OF_ARRAY(s_equations); ++ndx2)
		{
			if (s_equations[ndx] == s_equations[ndx2])
				continue;

			ctx.glEnable(GL_BLEND);
			ctx.glBlendEquation(getEquation(s_equations[ndx2]));
			ctx.expectError(GL_NO_ERROR);
			ctx.glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, 0);
			ctx.expectError(GL_INVALID_OPERATION);

			ctx.glDisable(GL_BLEND);
			ctx.glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, 0);
			ctx.expectError(GL_NO_ERROR);
		}
	}
	ctx.endSection();
}

void attachment_advanced_equation (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		ctx.isExtensionSupported("GL_KHR_blend_equation_advanced") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)),
		"This test requires support for the extension GL_KHR_blend_equation_advanced or context version 3.2 or higher.");

	glw::GLuint			fbo				= 0x1234;
	glw::GLuint			texture			= 0x1234;
	const glw::GLenum	attachments[]	= { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	ctx.glGenTextures(1, &texture);
	ctx.glBindTexture(GL_TEXTURE_2D, texture);
	ctx.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	ctx.glGenFramebuffers(1, &fbo);
	ctx.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	ctx.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	ctx.expectError(GL_NO_ERROR);
	ctx.glCheckFramebufferStatus(GL_FRAMEBUFFER);

	ctx.beginSection("GL_INVALID_OPERATION is generated if blending is enabled, advanced equations are used, and the draw buffer for other color outputs is not NONE unless NVX_blend_equation_advanced_multi_draw_buffers is supported.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_equations); ++ndx)
	{
		glu::ShaderProgram	program(ctx.getRenderContext(), generateProgramSources(ctx, s_equations[ndx]));
		ctx.getLog() << program;
		TCU_CHECK(program.isOk());

		ctx.glUseProgram(program.getProgram());
		ctx.glEnable(GL_BLEND);
		ctx.glDrawBuffers(2, attachments);
		ctx.expectError(GL_NO_ERROR);
		ctx.glBlendEquation(getEquation(s_equations[ndx]));
		ctx.glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, 0);
		if (ctx.isExtensionSupported("GL_NVX_blend_equation_advanced_multi_draw_buffers"))
			ctx.expectError(GL_NO_ERROR);
		else
			ctx.expectError(GL_INVALID_OPERATION);
	}
	ctx.endSection();

	ctx.beginSection("GL_NO_ERROR is generated if no advanced blend equations are used.");
	ctx.glBlendEquation(GL_FUNC_ADD);
	ctx.glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, 0);
	ctx.expectError(GL_NO_ERROR);
	ctx.endSection();

	ctx.beginSection("GL_NO_ERROR is generated if blending is disabled.");
	ctx.glDisable(GL_BLEND);
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_equations); ++ndx)
	{
		ctx.glBlendEquation(getEquation(s_equations[ndx]));
		ctx.glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_INT, 0);
		ctx.expectError(GL_NO_ERROR);
	}
	ctx.endSection();

	ctx.glDeleteFramebuffers(1, &fbo);
	ctx.glDeleteTextures(1, &texture);
}

} // anonymous

std::vector<FunctionContainer> getNegativeAdvancedBlendEquationTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{blend_qualifier_mismatch,			"blend_qualifier_mismatch",			"Test blend qualifier mismatch."			},
		{attachment_advanced_equation,		"attachment_advanced_equation",		"Test draw buffer for other color outputs." },
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
