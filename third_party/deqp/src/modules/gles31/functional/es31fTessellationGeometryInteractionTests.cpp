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
 * \brief Tessellation and geometry shader interaction tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTessellationGeometryInteractionTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuImageCompare.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuStringTemplate.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <sstream>
#include <algorithm>
#include <iterator>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static std::string specializeShader (const std::string& shaderSource, const glu::ContextType& contextType)
{
	const bool supportsES32 = glu::contextSupports(contextType, glu::ApiType::es(3, 2));
	std::map<std::string, std::string> shaderArgs;

	shaderArgs["VERSION_DECL"]					= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(contextType));
	shaderArgs["EXTENSION_GEOMETRY_SHADER"]		= (supportsES32) ? ("") : ("#extension GL_EXT_geometry_shader : require\n");
	shaderArgs["EXTENSION_TESSELATION_SHADER"]	= (supportsES32) ? ("") : ("#extension GL_EXT_tessellation_shader : require\n");

	return tcu::StringTemplate(shaderSource).specialize(shaderArgs);
}

static const char* const s_positionVertexShader =		"${VERSION_DECL}\n"
														"in highp vec4 a_position;\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = a_position;\n"
														"}\n";
static const char* const s_whiteOutputFragmentShader =	"${VERSION_DECL}\n"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = vec4(1.0);\n"
														"}\n";

static bool isBlack (const tcu::RGBA& c)
{
	return c.getRed() == 0 && c.getGreen() == 0 && c.getBlue() == 0;
}

class IdentityShaderCase : public TestCase
{
public:
					IdentityShaderCase	(Context& context, const char* name, const char* description);

protected:
	std::string		getVertexSource		(void) const;
	std::string		getFragmentSource	(void) const;
};

IdentityShaderCase::IdentityShaderCase (Context& context, const char* name, const char* description)
	: TestCase(context, name, description)
{
}

std::string IdentityShaderCase::getVertexSource (void) const
{
	std::string source =	"${VERSION_DECL}\n"
							"in highp vec4 a_position;\n"
							"out highp vec4 v_vertex_color;\n"
							"void main (void)\n"
							"{\n"
							"	gl_Position = a_position;\n"
							"	v_vertex_color = vec4(a_position.x * 0.5 + 0.5, a_position.y * 0.5 + 0.5, 1.0, 0.4);\n"
							"}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

std::string IdentityShaderCase::getFragmentSource (void) const
{
	std::string source =	"${VERSION_DECL}\n"
							"in mediump vec4 v_fragment_color;\n"
							"layout(location = 0) out mediump vec4 fragColor;\n"
							"void main (void)\n"
							"{\n"
							"	fragColor = v_fragment_color;\n"
							"}\n";

return specializeShader(source, m_context.getRenderContext().getType());
}

class IdentityGeometryShaderCase : public IdentityShaderCase
{
public:
	enum CaseType
	{
		CASE_TRIANGLES = 0,
		CASE_QUADS,
		CASE_ISOLINES,
	};

					IdentityGeometryShaderCase			(Context& context, const char* name, const char* description, CaseType caseType);
					~IdentityGeometryShaderCase			(void);

private:
	void			init								(void);
	void			deinit								(void);
	IterateResult	iterate								(void);

	std::string		getTessellationControlSource		(void) const;
	std::string		getTessellationEvaluationSource		(bool geometryActive) const;
	std::string		getGeometrySource					(void) const;

	enum
	{
		RENDER_SIZE = 128,
	};

	const CaseType	m_case;
	deUint32		m_patchBuffer;
};

IdentityGeometryShaderCase::IdentityGeometryShaderCase (Context& context, const char* name, const char* description, CaseType caseType)
	: IdentityShaderCase	(context, name, description)
	, m_case				(caseType)
	, m_patchBuffer			(0)
{
}

IdentityGeometryShaderCase::~IdentityGeometryShaderCase (void)
{
	deinit();
}

void IdentityGeometryShaderCase::init (void)
{
	// Requirements
	const bool supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 &&
		(!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		 !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader")))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	if (m_context.getRenderTarget().getWidth() < RENDER_SIZE ||
		m_context.getRenderTarget().getHeight() < RENDER_SIZE)
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_SIZE) + "x" + de::toString<int>(RENDER_SIZE) + " or larger render target.");

	// Log

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing tessellating shader program output does not change when a passthrough geometry shader is attached.\n"
		<< "Rendering two images, first with and second without a geometry shader. Expecting similar results.\n"
		<< "Using additive blending to detect overlap.\n"
		<< tcu::TestLog::EndMessage;

	// Resources

	{
		static const tcu::Vec4 patchBufferData[4] =
		{
			tcu::Vec4( -0.9f, -0.9f, 0.0f, 1.0f ),
			tcu::Vec4( -0.9f,  0.9f, 0.0f, 1.0f ),
			tcu::Vec4(  0.9f, -0.9f, 0.0f, 1.0f ),
			tcu::Vec4(  0.9f,  0.9f, 0.0f, 1.0f ),
		};

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.genBuffers(1, &m_patchBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_patchBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(patchBufferData), patchBufferData, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffer");
	}
}

void IdentityGeometryShaderCase::deinit (void)
{
	if (m_patchBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_patchBuffer);
		m_patchBuffer = 0;
	}
}

IdentityGeometryShaderCase::IterateResult IdentityGeometryShaderCase::iterate (void)
{
	const float				innerTessellationLevel	= 14.0f;
	const float				outerTessellationLevel	= 14.0f;
	const glw::Functions&	gl						= m_context.getRenderContext().getFunctions();
	tcu::Surface			resultWithGeometry		(RENDER_SIZE, RENDER_SIZE);
	tcu::Surface			resultWithoutGeometry	(RENDER_SIZE, RENDER_SIZE);

	const struct
	{
		const char*				name;
		const char*				description;
		bool					containsGeometryShader;
		tcu::PixelBufferAccess	surfaceAccess;
	} renderTargets[] =
	{
		{ "RenderWithGeometryShader",		"Render with geometry shader",		true,	resultWithGeometry.getAccess()		},
		{ "RenderWithoutGeometryShader",	"Render without geometry shader",	false,	resultWithoutGeometry.getAccess()	},
	};

	gl.viewport(0, 0, RENDER_SIZE, RENDER_SIZE);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set viewport");

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE);
	gl.blendEquation(GL_FUNC_ADD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set blend");

	m_testCtx.getLog() << tcu::TestLog::Message << "Tessellation level: inner " << innerTessellationLevel << ", outer " << outerTessellationLevel << tcu::TestLog::EndMessage;

	// render with and without geometry shader
	for (int renderNdx = 0; renderNdx < DE_LENGTH_OF_ARRAY(renderTargets); ++renderNdx)
	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), renderTargets[renderNdx].name, renderTargets[renderNdx].description);
		glu::ProgramSources			sources;

		sources	<< glu::VertexSource(getVertexSource())
				<< glu::FragmentSource(getFragmentSource())
				<< glu::TessellationControlSource(getTessellationControlSource())
				<< glu::TessellationEvaluationSource(getTessellationEvaluationSource(renderTargets[renderNdx].containsGeometryShader));

		if (renderTargets[renderNdx].containsGeometryShader)
			sources << glu::GeometrySource(getGeometrySource());

		{
			const glu::ShaderProgram	program					(m_context.getRenderContext(), sources);
			const glu::VertexArray		vao						(m_context.getRenderContext());
			const int					posLocation				= gl.getAttribLocation(program.getProgram(), "a_position");
			const int					innerTessellationLoc	= gl.getUniformLocation(program.getProgram(), "u_innerTessellationLevel");
			const int					outerTessellationLoc	= gl.getUniformLocation(program.getProgram(), "u_outerTessellationLevel");

			m_testCtx.getLog() << program;

			if (!program.isOk())
				throw tcu::TestError("could not build program");
			if (posLocation == -1)
				throw tcu::TestError("a_position location was -1");
			if (outerTessellationLoc == -1)
				throw tcu::TestError("u_outerTessellationLevel location was -1");

			gl.bindVertexArray(*vao);
			gl.bindBuffer(GL_ARRAY_BUFFER, m_patchBuffer);
			gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
			gl.enableVertexAttribArray(posLocation);
			GLU_EXPECT_NO_ERROR(gl.getError(), "setup attribs");

			gl.useProgram(program.getProgram());
			gl.uniform1f(outerTessellationLoc, outerTessellationLevel);

			if (innerTessellationLoc == -1)
				gl.uniform1f(innerTessellationLoc, innerTessellationLevel);

			GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

			gl.patchParameteri(GL_PATCH_VERTICES, (m_case == CASE_TRIANGLES) ? (3): (4));
			GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

			gl.drawArrays(GL_PATCHES, 0, 4);
			GLU_EXPECT_NO_ERROR(gl.getError(), "draw patches");

			glu::readPixels(m_context.getRenderContext(), 0, 0, renderTargets[renderNdx].surfaceAccess);
		}
	}

	if (tcu::intThresholdPositionDeviationCompare(m_testCtx.getLog(),
												  "ImageCompare",
												  "Image comparison",
												  resultWithoutGeometry.getAccess(),
												  resultWithGeometry.getAccess(),
												  tcu::UVec4(8, 8, 8, 255),
												  tcu::IVec3(1, 1, 0),
												  true,
												  tcu::COMPARE_LOG_RESULT))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");

	return STOP;
}

std::string IdentityGeometryShaderCase::getTessellationControlSource (void) const
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout(vertices = 4) out;\n"
			"\n"
			"uniform highp float u_innerTessellationLevel;\n"
			"uniform highp float u_outerTessellationLevel;\n"
			"in highp vec4 v_vertex_color[];\n"
			"out highp vec4 v_patch_color[];\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	v_patch_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			"\n";

	if (m_case == CASE_TRIANGLES)
		buf <<	"	gl_TessLevelOuter[0] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[1] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[2] = u_outerTessellationLevel;\n"
				"	gl_TessLevelInner[0] = u_innerTessellationLevel;\n";
	else if (m_case == CASE_QUADS)
		buf <<	"	gl_TessLevelOuter[0] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[1] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[2] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[3] = u_outerTessellationLevel;\n"
				"	gl_TessLevelInner[0] = u_innerTessellationLevel;\n"
				"	gl_TessLevelInner[1] = u_innerTessellationLevel;\n";
	else if (m_case == CASE_ISOLINES)
		buf <<	"	gl_TessLevelOuter[0] = u_outerTessellationLevel;\n"
				"	gl_TessLevelOuter[1] = u_outerTessellationLevel;\n";
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string IdentityGeometryShaderCase::getTessellationEvaluationSource (bool geometryActive) const
{
	const char* const	colorOutputName = ((geometryActive) ? ("v_evaluated_color") : ("v_fragment_color"));
	std::ostringstream	buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout("
				<< ((m_case == CASE_TRIANGLES) ? ("triangles") : (m_case == CASE_QUADS) ? ("quads") : ("isolines"))
				<< ") in;\n"
			"\n"
			"in highp vec4 v_patch_color[];\n"
			"out highp vec4 " << colorOutputName << ";\n"
			"\n"
			"// note: No need to use precise gl_Position since we do not require gapless geometry\n"
			"void main (void)\n"
			"{\n";

	if (m_case == CASE_TRIANGLES)
		buf <<	"	vec3 weights = vec3(pow(gl_TessCoord.x, 1.3), pow(gl_TessCoord.y, 1.3), pow(gl_TessCoord.z, 1.3));\n"
				"	vec3 cweights = gl_TessCoord;\n"
				"	gl_Position = vec4(weights.x * gl_in[0].gl_Position.xyz + weights.y * gl_in[1].gl_Position.xyz + weights.z * gl_in[2].gl_Position.xyz, 1.0);\n"
				"	" << colorOutputName << " = cweights.x * v_patch_color[0] + cweights.y * v_patch_color[1] + cweights.z * v_patch_color[2];\n";
	else if (m_case == CASE_QUADS || m_case == CASE_ISOLINES)
		buf <<	"	vec2 normalizedCoord = (gl_TessCoord.xy * 2.0 - vec2(1.0));\n"
				"	vec2 normalizedWeights = normalizedCoord * (vec2(1.0) - 0.3 * cos(normalizedCoord.yx * 1.57));\n"
				"	vec2 weights = normalizedWeights * 0.5 + vec2(0.5);\n"
				"	vec2 cweights = gl_TessCoord.xy;\n"
				"	gl_Position = mix(mix(gl_in[0].gl_Position, gl_in[1].gl_Position, weights.y), mix(gl_in[2].gl_Position, gl_in[3].gl_Position, weights.y), weights.x);\n"
				"	" << colorOutputName << " = mix(mix(v_patch_color[0], v_patch_color[1], cweights.y), mix(v_patch_color[2], v_patch_color[3], cweights.y), cweights.x);\n";
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string IdentityGeometryShaderCase::getGeometrySource (void) const
{
	const char* const	geometryInputPrimitive			= (m_case == CASE_ISOLINES) ? ("lines") : ("triangles");
	const char* const	geometryOutputPrimitive			= (m_case == CASE_ISOLINES) ? ("line_strip") : ("triangle_strip");
	const int			numEmitVertices					= (m_case == CASE_ISOLINES) ? (2) : (3);
	std::ostringstream	buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_GEOMETRY_SHADER}"
			"layout(" << geometryInputPrimitive << ") in;\n"
			"layout(" << geometryOutputPrimitive << ", max_vertices=" << numEmitVertices <<") out;\n"
			"\n"
			"in highp vec4 v_evaluated_color[];\n"
			"out highp vec4 v_fragment_color;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	for (int ndx = 0; ndx < gl_in.length(); ++ndx)\n"
			"	{\n"
			"		gl_Position = gl_in[ndx].gl_Position;\n"
			"		v_fragment_color = v_evaluated_color[ndx];\n"
			"		EmitVertex();\n"
			"	}\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

class IdentityTessellationShaderCase : public IdentityShaderCase
{
public:
	enum CaseType
	{
		CASE_TRIANGLES = 0,
		CASE_ISOLINES,
	};

					IdentityTessellationShaderCase		(Context& context, const char* name, const char* description, CaseType caseType);
					~IdentityTessellationShaderCase		(void);

private:
	void			init								(void);
	void			deinit								(void);
	IterateResult	iterate								(void);

	std::string		getTessellationControlSource		(void) const;
	std::string		getTessellationEvaluationSource		(void) const;
	std::string		getGeometrySource					(bool tessellationActive) const;

	enum
	{
		RENDER_SIZE = 256,
	};

	const CaseType	m_case;
	deUint32		m_dataBuffer;
};

IdentityTessellationShaderCase::IdentityTessellationShaderCase (Context& context, const char* name, const char* description, CaseType caseType)
	: IdentityShaderCase	(context, name, description)
	, m_case				(caseType)
	, m_dataBuffer			(0)
{
}

IdentityTessellationShaderCase::~IdentityTessellationShaderCase (void)
{
	deinit();
}

void IdentityTessellationShaderCase::init (void)
{
	// Requirements
	const bool supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 &&
		(!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		 !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader")))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	if (m_context.getRenderTarget().getWidth() < RENDER_SIZE ||
		m_context.getRenderTarget().getHeight() < RENDER_SIZE)
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_SIZE) + "x" + de::toString<int>(RENDER_SIZE) + " or larger render target.");

	// Log

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing geometry shading shader program output does not change when a passthrough tessellation shader is attached.\n"
		<< "Rendering two images, first with and second without a tessellation shader. Expecting similar results.\n"
		<< "Using additive blending to detect overlap.\n"
		<< tcu::TestLog::EndMessage;

	// Resources

	{
		static const tcu::Vec4	pointData[]	=
		{
			tcu::Vec4( -0.4f,  0.4f, 0.0f, 1.0f ),
			tcu::Vec4(  0.0f, -0.5f, 0.0f, 1.0f ),
			tcu::Vec4(  0.4f,  0.4f, 0.0f, 1.0f ),
		};
		const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();

		gl.genBuffers(1, &m_dataBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_dataBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(pointData), pointData, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffer");
	}
}

void IdentityTessellationShaderCase::deinit (void)
{
	if (m_dataBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_dataBuffer);
		m_dataBuffer = 0;
	}
}

IdentityTessellationShaderCase::IterateResult IdentityTessellationShaderCase::iterate (void)
{
	const glw::Functions&	gl							= m_context.getRenderContext().getFunctions();
	tcu::Surface			resultWithTessellation		(RENDER_SIZE, RENDER_SIZE);
	tcu::Surface			resultWithoutTessellation	(RENDER_SIZE, RENDER_SIZE);
	const int				numPrimitiveVertices		= (m_case == CASE_TRIANGLES) ? (3) : (2);

	const struct
	{
		const char*				name;
		const char*				description;
		bool					containsTessellationShaders;
		tcu::PixelBufferAccess	surfaceAccess;
	} renderTargets[] =
	{
		{ "RenderWithTessellationShader",		"Render with tessellation shader",		true,	resultWithTessellation.getAccess()		},
		{ "RenderWithoutTessellationShader",	"Render without tessellation shader",	false,	resultWithoutTessellation.getAccess()	},
	};

	gl.viewport(0, 0, RENDER_SIZE, RENDER_SIZE);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set viewport");

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE);
	gl.blendEquation(GL_FUNC_ADD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set blend");

	// render with and without tessellation shader
	for (int renderNdx = 0; renderNdx < DE_LENGTH_OF_ARRAY(renderTargets); ++renderNdx)
	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), renderTargets[renderNdx].name, renderTargets[renderNdx].description);
		glu::ProgramSources			sources;

		sources	<< glu::VertexSource(getVertexSource())
				<< glu::FragmentSource(getFragmentSource())
				<< glu::GeometrySource(getGeometrySource(renderTargets[renderNdx].containsTessellationShaders));

		if (renderTargets[renderNdx].containsTessellationShaders)
			sources	<< glu::TessellationControlSource(getTessellationControlSource())
					<< glu::TessellationEvaluationSource(getTessellationEvaluationSource());

		{
			const glu::ShaderProgram	program					(m_context.getRenderContext(), sources);
			const glu::VertexArray		vao						(m_context.getRenderContext());
			const int					posLocation				= gl.getAttribLocation(program.getProgram(), "a_position");

			m_testCtx.getLog() << program;

			if (!program.isOk())
				throw tcu::TestError("could not build program");
			if (posLocation == -1)
				throw tcu::TestError("a_position location was -1");

			gl.bindVertexArray(*vao);
			gl.bindBuffer(GL_ARRAY_BUFFER, m_dataBuffer);
			gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
			gl.enableVertexAttribArray(posLocation);
			GLU_EXPECT_NO_ERROR(gl.getError(), "setup attribs");

			gl.useProgram(program.getProgram());
			GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

			if (renderTargets[renderNdx].containsTessellationShaders)
			{
				gl.patchParameteri(GL_PATCH_VERTICES, numPrimitiveVertices);
				GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

				gl.drawArrays(GL_PATCHES, 0, numPrimitiveVertices);
				GLU_EXPECT_NO_ERROR(gl.getError(), "draw patches");
			}
			else
			{
				gl.drawArrays((m_case == CASE_TRIANGLES) ? (GL_TRIANGLES) : (GL_LINES), 0, numPrimitiveVertices);
				GLU_EXPECT_NO_ERROR(gl.getError(), "draw primitives");
			}

			glu::readPixels(m_context.getRenderContext(), 0, 0, renderTargets[renderNdx].surfaceAccess);
		}
	}

	// compare
	{
		bool imageOk;

		if (m_context.getRenderTarget().getNumSamples() > 1)
			imageOk = tcu::fuzzyCompare(m_testCtx.getLog(),
										"ImageCompare",
										"Image comparison",
										resultWithoutTessellation.getAccess(),
										resultWithTessellation.getAccess(),
										0.03f,
										tcu::COMPARE_LOG_RESULT);
		else
			imageOk = tcu::intThresholdPositionDeviationCompare(m_testCtx.getLog(),
																"ImageCompare",
																"Image comparison",
																resultWithoutTessellation.getAccess(),
																resultWithTessellation.getAccess(),
																tcu::UVec4(8, 8, 8, 255),				//!< threshold
																tcu::IVec3(1, 1, 0),					//!< 3x3 search kernel
																true,									//!< fragments may end up over the viewport, just ignore them
																tcu::COMPARE_LOG_RESULT);

		if (imageOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}

	return STOP;
}

std::string IdentityTessellationShaderCase::getTessellationControlSource (void) const
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout(vertices = " << ((m_case == CASE_TRIANGLES) ? (3) : (2)) << ") out;\n"
			"\n"
			"in highp vec4 v_vertex_color[];\n"
			"out highp vec4 v_control_color[];\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	v_control_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			"\n";

	if (m_case == CASE_TRIANGLES)
		buf <<	"	gl_TessLevelOuter[0] = 1.0;\n"
				"	gl_TessLevelOuter[1] = 1.0;\n"
				"	gl_TessLevelOuter[2] = 1.0;\n"
				"	gl_TessLevelInner[0] = 1.0;\n";
	else if (m_case == CASE_ISOLINES)
		buf <<	"	gl_TessLevelOuter[0] = 1.0;\n"
				"	gl_TessLevelOuter[1] = 1.0;\n";
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string IdentityTessellationShaderCase::getTessellationEvaluationSource (void) const
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout("
				<< ((m_case == CASE_TRIANGLES) ? ("triangles") : ("isolines"))
				<< ") in;\n"
			"\n"
			"in highp vec4 v_control_color[];\n"
			"out highp vec4 v_evaluated_color;\n"
			"\n"
			"// note: No need to use precise gl_Position since we do not require gapless geometry\n"
			"void main (void)\n"
			"{\n";

	if (m_case == CASE_TRIANGLES)
		buf <<	"	gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;\n"
				"	v_evaluated_color = gl_TessCoord.x * v_control_color[0] + gl_TessCoord.y * v_control_color[1] + gl_TessCoord.z * v_control_color[2];\n";
	else if (m_case == CASE_ISOLINES)
		buf <<	"	gl_Position = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);\n"
				"	v_evaluated_color = mix(v_control_color[0], v_control_color[1], gl_TessCoord.x);\n";
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string IdentityTessellationShaderCase::getGeometrySource (bool tessellationActive) const
{
	const char* const	colorSourceName			= (tessellationActive) ? ("v_evaluated_color") : ("v_vertex_color");
	const char* const	geometryInputPrimitive	= (m_case == CASE_ISOLINES) ? ("lines") : ("triangles");
	const char* const	geometryOutputPrimitive	= (m_case == CASE_ISOLINES) ? ("line_strip") : ("triangle_strip");
	const int			numEmitVertices			= (m_case == CASE_ISOLINES) ? (11) : (8);
	std::ostringstream	buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_GEOMETRY_SHADER}"
			"layout(" << geometryInputPrimitive << ") in;\n"
			"layout(" << geometryOutputPrimitive << ", max_vertices=" << numEmitVertices <<") out;\n"
			"\n"
			"in highp vec4 " << colorSourceName << "[];\n"
			"out highp vec4 v_fragment_color;\n"
			"\n"
			"void main (void)\n"
			"{\n";

	if (m_case == CASE_TRIANGLES)
	{
		buf <<	"	vec4 centerPos = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0f;\n"
				"\n"
				"	for (int ndx = 0; ndx < 4; ++ndx)\n"
				"	{\n"
				"		gl_Position = centerPos + (centerPos - gl_in[ndx % 3].gl_Position);\n"
				"		v_fragment_color = " << colorSourceName << "[ndx % 3];\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = centerPos + 0.7 * (centerPos - gl_in[ndx % 3].gl_Position);\n"
				"		v_fragment_color = " << colorSourceName << "[ndx % 3];\n"
				"		EmitVertex();\n"
				"	}\n";

	}
	else if (m_case == CASE_ISOLINES)
	{
		buf <<	"	vec4 mdir = vec4(gl_in[0].gl_Position.y - gl_in[1].gl_Position.y, gl_in[1].gl_Position.x - gl_in[0].gl_Position.x, 0.0, 0.0);\n"
				"	for (int i = 0; i <= 10; ++i)\n"
				"	{\n"
				"		float xweight = cos(float(i) / 10.0 * 6.28) * 0.5 + 0.5;\n"
				"		float mweight = sin(float(i) / 10.0 * 6.28) * 0.1 + 0.1;\n"
				"		gl_Position = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, xweight) + mweight * mdir;\n"
				"		v_fragment_color = mix(" << colorSourceName << "[0], " << colorSourceName << "[1], xweight);\n"
				"		EmitVertex();\n"
				"	}\n";
	}
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

class FeedbackPrimitiveTypeCase : public TestCase
{
public:
	enum TessellationOutputType
	{
		TESSELLATION_OUT_TRIANGLES = 0,
		TESSELLATION_OUT_QUADS,
		TESSELLATION_OUT_ISOLINES,

		TESSELLATION_OUT_LAST
	};
	enum TessellationPointMode
	{
		TESSELLATION_POINTMODE_OFF = 0,
		TESSELLATION_POINTMODE_ON,

		TESSELLATION_POINTMODE_LAST
	};
	enum GeometryOutputType
	{
		GEOMETRY_OUTPUT_POINTS = 0,
		GEOMETRY_OUTPUT_LINES,
		GEOMETRY_OUTPUT_TRIANGLES,

		GEOMETRY_OUTPUT_LAST
	};

									FeedbackPrimitiveTypeCase				(Context& context,
																			 const char* name,
																			 const char* description,
																			 TessellationOutputType tessellationOutput,
																			 TessellationPointMode tessellationPointMode,
																			 GeometryOutputType geometryOutputType);
									~FeedbackPrimitiveTypeCase				(void);

private:
	void							init									(void);
	void							deinit									(void);
	IterateResult					iterate									(void);

	void							renderWithFeedback						(tcu::Surface& dst);
	void							renderWithoutFeedback					(tcu::Surface& dst);
	void							verifyFeedbackResults					(const std::vector<tcu::Vec4>& feedbackResult);
	void							verifyRenderedImage						(const tcu::Surface& image, const std::vector<tcu::Vec4>& vertices);

	void							genTransformFeedback					(void);
	int								getNumGeneratedElementsPerPrimitive		(void) const;
	int								getNumGeneratedPrimitives				(void) const;
	int								getNumTessellatedPrimitives				(void) const;
	int								getGeometryAmplification				(void) const;

	std::string						getVertexSource							(void) const;
	std::string						getFragmentSource						(void) const;
	std::string						getTessellationControlSource			(void) const;
	std::string						getTessellationEvaluationSource			(void) const;
	std::string						getGeometrySource						(void) const;

	static const char*				getTessellationOutputDescription		(TessellationOutputType tessellationOutput,
																			 TessellationPointMode tessellationPointMode);
	static const char*				getGeometryInputDescription				(TessellationOutputType tessellationOutput,
																			 TessellationPointMode tessellationPointMode);
	static const char*				getGeometryOutputDescription			(GeometryOutputType geometryOutput);
	glw::GLenum						getOutputPrimitiveGLType				(void) const;

	enum
	{
		RENDER_SIZE = 128,
	};

	const TessellationOutputType	m_tessellationOutput;
	const TessellationPointMode		m_tessellationPointMode;
	const GeometryOutputType		m_geometryOutputType;

	glu::ShaderProgram*				m_feedbackProgram;
	glu::ShaderProgram*				m_nonFeedbackProgram;
	deUint32						m_patchBuffer;
	deUint32						m_feedbackID;
	deUint32						m_feedbackBuffer;
};

FeedbackPrimitiveTypeCase::FeedbackPrimitiveTypeCase (Context& context,
									  const char* name,
									  const char* description,
									  TessellationOutputType tessellationOutput,
									  TessellationPointMode tessellationPointMode,
									  GeometryOutputType geometryOutputType)
	: TestCase					(context, name, description)
	, m_tessellationOutput		(tessellationOutput)
	, m_tessellationPointMode	(tessellationPointMode)
	, m_geometryOutputType		(geometryOutputType)
	, m_feedbackProgram			(DE_NULL)
	, m_nonFeedbackProgram		(DE_NULL)
	, m_patchBuffer				(0)
	, m_feedbackID				(0)
	, m_feedbackBuffer			(0)
{
	DE_ASSERT(tessellationOutput < TESSELLATION_OUT_LAST);
	DE_ASSERT(tessellationPointMode < TESSELLATION_POINTMODE_LAST);
	DE_ASSERT(geometryOutputType < GEOMETRY_OUTPUT_LAST);
}

FeedbackPrimitiveTypeCase::~FeedbackPrimitiveTypeCase (void)
{
	deinit();
}

void FeedbackPrimitiveTypeCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// Requirements
	const bool supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 &&
		(!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		 !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader")))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	if (m_context.getRenderTarget().getWidth() < RENDER_SIZE ||
		m_context.getRenderTarget().getHeight() < RENDER_SIZE)
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_SIZE) + "x" + de::toString<int>(RENDER_SIZE) + " or larger render target.");

	// Log

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing "
			<< getTessellationOutputDescription(m_tessellationOutput, m_tessellationPointMode)
			<< "->"
			<< getGeometryInputDescription(m_tessellationOutput, m_tessellationPointMode)
			<< " primitive conversion with and without transform feedback.\n"
		<< "Sending a patch of 4 vertices (2x2 uniform grid) to tessellation control shader.\n"
		<< "Control shader emits a patch of 9 vertices (3x3 uniform grid).\n"
		<< "Setting outer tessellation level = 3, inner = 3.\n"
		<< "Primitive generator emits " << getTessellationOutputDescription(m_tessellationOutput, m_tessellationPointMode) << "\n"
		<< "Geometry shader transforms emitted primitives to " << getGeometryOutputDescription(m_geometryOutputType) << "\n"
		<< "Reading back vertex positions of generated primitives using transform feedback.\n"
		<< "Verifying rendered image and feedback vertices are consistent.\n"
		<< "Rendering scene again with identical shader program, but without setting feedback varying. Expecting similar output image."
		<< tcu::TestLog::EndMessage;

	// Resources

	{
		static const tcu::Vec4 patchBufferData[4] =
		{
			tcu::Vec4( -0.9f, -0.9f, 0.0f, 1.0f ),
			tcu::Vec4( -0.9f,  0.9f, 0.0f, 1.0f ),
			tcu::Vec4(  0.9f, -0.9f, 0.0f, 1.0f ),
			tcu::Vec4(  0.9f,  0.9f, 0.0f, 1.0f ),
		};

		gl.genBuffers(1, &m_patchBuffer);
		gl.bindBuffer(GL_ARRAY_BUFFER, m_patchBuffer);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(patchBufferData), patchBufferData, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen buffer");
	}

	m_feedbackProgram = new glu::ShaderProgram(m_context.getRenderContext(),
											   glu::ProgramSources()
												<< glu::VertexSource(getVertexSource())
												<< glu::FragmentSource(getFragmentSource())
												<< glu::TessellationControlSource(getTessellationControlSource())
												<< glu::TessellationEvaluationSource(getTessellationEvaluationSource())
												<< glu::GeometrySource(getGeometrySource())
												<< glu::TransformFeedbackVarying("tf_someVertexPosition")
												<< glu::TransformFeedbackMode(GL_INTERLEAVED_ATTRIBS));
	m_testCtx.getLog() << *m_feedbackProgram;
	if (!m_feedbackProgram->isOk())
		throw tcu::TestError("failed to build program");

	m_nonFeedbackProgram = new glu::ShaderProgram(m_context.getRenderContext(),
												  glu::ProgramSources()
													<< glu::VertexSource(getVertexSource())
													<< glu::FragmentSource(getFragmentSource())
													<< glu::TessellationControlSource(getTessellationControlSource())
													<< glu::TessellationEvaluationSource(getTessellationEvaluationSource())
													<< glu::GeometrySource(getGeometrySource()));
	if (!m_nonFeedbackProgram->isOk())
	{
		m_testCtx.getLog() << *m_nonFeedbackProgram;
		throw tcu::TestError("failed to build program");
	}

	genTransformFeedback();
}

void FeedbackPrimitiveTypeCase::deinit (void)
{
	if (m_patchBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_patchBuffer);
		m_patchBuffer = 0;
	}

	if (m_feedbackBuffer)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_feedbackBuffer);
		m_feedbackBuffer = 0;
	}

	if (m_feedbackID)
	{
		m_context.getRenderContext().getFunctions().deleteTransformFeedbacks(1, &m_feedbackID);
		m_feedbackID = 0;
	}

	if (m_feedbackProgram)
	{
		delete m_feedbackProgram;
		m_feedbackProgram = DE_NULL;
	}

	if (m_nonFeedbackProgram)
	{
		delete m_nonFeedbackProgram;
		m_nonFeedbackProgram = DE_NULL;
	}
}

FeedbackPrimitiveTypeCase::IterateResult FeedbackPrimitiveTypeCase::iterate (void)
{
	tcu::Surface feedbackResult		(RENDER_SIZE, RENDER_SIZE);
	tcu::Surface nonFeedbackResult	(RENDER_SIZE, RENDER_SIZE);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// render with and without XFB
	renderWithFeedback(feedbackResult);
	renderWithoutFeedback(nonFeedbackResult);

	// compare
	{
		bool imageOk;

		m_testCtx.getLog() << tcu::TestLog::Message << "Comparing the image rendered with no transform feedback against the image rendered with enabled transform feedback." << tcu::TestLog::EndMessage;

		if (m_context.getRenderTarget().getNumSamples() > 1)
			imageOk = tcu::fuzzyCompare(m_testCtx.getLog(),
										"ImageCompare",
										"Image comparison",
										feedbackResult.getAccess(),
										nonFeedbackResult.getAccess(),
										0.03f,
										tcu::COMPARE_LOG_RESULT);
		else
			imageOk = tcu::intThresholdPositionDeviationCompare(m_testCtx.getLog(),
																"ImageCompare",
																"Image comparison",
																feedbackResult.getAccess(),
																nonFeedbackResult.getAccess(),
																tcu::UVec4(8, 8, 8, 255),						//!< threshold
																tcu::IVec3(1, 1, 0),							//!< 3x3 search kernel
																true,											//!< fragments may end up over the viewport, just ignore them
																tcu::COMPARE_LOG_RESULT);

		if (!imageOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image comparison failed");
	}

	return STOP;
}

void FeedbackPrimitiveTypeCase::renderWithFeedback(tcu::Surface& dst)
{
	const glw::Functions&			gl							= m_context.getRenderContext().getFunctions();
	const glu::VertexArray			vao							(m_context.getRenderContext());
	const glu::Query				primitivesGeneratedQuery	(m_context.getRenderContext());
	const int						posLocation					= gl.getAttribLocation(m_feedbackProgram->getProgram(), "a_position");
	const glw::GLenum				feedbackPrimitiveMode		= getOutputPrimitiveGLType();

	if (posLocation == -1)
		throw tcu::TestError("a_position was -1");

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering with transform feedback" << tcu::TestLog::EndMessage;

	gl.viewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindVertexArray(*vao);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_patchBuffer);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup attribs");

	gl.useProgram(m_feedbackProgram->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

	gl.patchParameteri(GL_PATCH_VERTICES, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

	gl.beginQuery(GL_PRIMITIVES_GENERATED, *primitivesGeneratedQuery);
	GLU_EXPECT_NO_ERROR(gl.getError(), "begin GL_PRIMITIVES_GENERATED query");

	m_testCtx.getLog() << tcu::TestLog::Message << "Begin transform feedback with mode " << glu::getPrimitiveTypeStr(feedbackPrimitiveMode) << tcu::TestLog::EndMessage;

	gl.beginTransformFeedback(feedbackPrimitiveMode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "begin xfb");

	m_testCtx.getLog() << tcu::TestLog::Message << "Calling drawArrays with mode GL_PATCHES" << tcu::TestLog::EndMessage;

	gl.drawArrays(GL_PATCHES, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw patches");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "end xfb");

	gl.endQuery(GL_PRIMITIVES_GENERATED);
	GLU_EXPECT_NO_ERROR(gl.getError(), "end GL_PRIMITIVES_GENERATED query");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "readPixels");

	// verify GL_PRIMITIVES_GENERATED
	{
		glw::GLuint primitivesGeneratedResult = 0;
		gl.getQueryObjectuiv(*primitivesGeneratedQuery, GL_QUERY_RESULT, &primitivesGeneratedResult);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_PRIMITIVES_GENERATED value");

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying GL_PRIMITIVES_GENERATED, expecting " << getNumGeneratedPrimitives() << tcu::TestLog::EndMessage;

		if ((int)primitivesGeneratedResult != getNumGeneratedPrimitives())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error, GL_PRIMITIVES_GENERATED was " << primitivesGeneratedResult << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got unexpected GL_PRIMITIVES_GENERATED");
		}
		else
			m_testCtx.getLog() << tcu::TestLog::Message << "GL_PRIMITIVES_GENERATED valid." << tcu::TestLog::EndMessage;
	}

	// feedback
	{
		std::vector<tcu::Vec4>	feedbackResults		(getNumGeneratedElementsPerPrimitive() * getNumGeneratedPrimitives());
		const void*				mappedPtr			= gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, (glw::GLsizeiptr)(feedbackResults.size() * sizeof(tcu::Vec4)), GL_MAP_READ_BIT);
		glw::GLboolean			unmapResult;

		GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange");

		m_testCtx.getLog() << tcu::TestLog::Message << "Reading transform feedback buffer." << tcu::TestLog::EndMessage;
		if (!mappedPtr)
			throw tcu::TestError("mapBufferRange returned null");

		deMemcpy(feedbackResults[0].getPtr(), mappedPtr, (int)(feedbackResults.size() * sizeof(tcu::Vec4)));

		unmapResult = gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "unmapBuffer");

		if (unmapResult != GL_TRUE)
			throw tcu::TestError("unmapBuffer failed, did not return true");

		// verify transform results
		verifyFeedbackResults(feedbackResults);

		// verify feedback results are consistent with rendered image
		verifyRenderedImage(dst, feedbackResults);
	}
}

void FeedbackPrimitiveTypeCase::renderWithoutFeedback (tcu::Surface& dst)
{
	const glw::Functions&			gl							= m_context.getRenderContext().getFunctions();
	const glu::VertexArray			vao							(m_context.getRenderContext());
	const int						posLocation					= gl.getAttribLocation(m_nonFeedbackProgram->getProgram(), "a_position");

	if (posLocation == -1)
		throw tcu::TestError("a_position was -1");

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering without transform feedback" << tcu::TestLog::EndMessage;

	gl.viewport(0, 0, dst.getWidth(), dst.getHeight());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindVertexArray(*vao);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_patchBuffer);
	gl.vertexAttribPointer(posLocation, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(posLocation);
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup attribs");

	gl.useProgram(m_nonFeedbackProgram->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

	gl.patchParameteri(GL_PATCH_VERTICES, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

	m_testCtx.getLog() << tcu::TestLog::Message << "Calling drawArrays with mode GL_PATCHES" << tcu::TestLog::EndMessage;

	gl.drawArrays(GL_PATCHES, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "draw patches");

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "readPixels");
}

void FeedbackPrimitiveTypeCase::verifyFeedbackResults (const std::vector<tcu::Vec4>& feedbackResult)
{
	const int	geometryAmplification	= getGeometryAmplification();
	const int	elementsPerPrimitive	= getNumGeneratedElementsPerPrimitive();
	const int	errorFloodThreshold		= 8;
	int			readNdx					= 0;
	int			numErrors				= 0;

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying feedback results." << tcu::TestLog::EndMessage;

	for (int tessellatedPrimitiveNdx = 0; tessellatedPrimitiveNdx < getNumTessellatedPrimitives(); ++tessellatedPrimitiveNdx)
	{
		const tcu::Vec4	primitiveVertex = feedbackResult[readNdx];

		// check the generated vertices are in the proper range (range: -0.4 <-> 0.4)
		{
			const float	equalThreshold	=	1.0e-6f;
			const bool	centroidOk		=	(primitiveVertex.x() >= -0.4f - equalThreshold) &&
											(primitiveVertex.x() <=  0.4f + equalThreshold) &&
											(primitiveVertex.y() >= -0.4f - equalThreshold) &&
											(primitiveVertex.y() <=  0.4f + equalThreshold) &&
											(de::abs(primitiveVertex.z()) < equalThreshold) &&
											(de::abs(primitiveVertex.w() - 1.0f) < equalThreshold);

			if (!centroidOk && numErrors++ < errorFloodThreshold)
			{
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Element at index " << (readNdx) << " (tessellation invocation " << tessellatedPrimitiveNdx << ")\n"
					<< "\texpected vertex in range: ( [-0.4, 0.4], [-0.4, 0.4], 0.0, 1.0 )\n"
					<< "\tgot: " << primitiveVertex
					<< tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid feedback output");

				++readNdx;
				continue;
			}
		}

		// check all other primitives generated from this tessellated primitive have the same feedback value
		for (int generatedPrimitiveNdx = 0; generatedPrimitiveNdx < geometryAmplification; ++generatedPrimitiveNdx)
		for (int primitiveVertexNdx = 0; primitiveVertexNdx < elementsPerPrimitive; ++primitiveVertexNdx)
		{
			const tcu::Vec4 generatedElementVertex	= feedbackResult[readNdx];
			const tcu::Vec4 equalThreshold			(1.0e-6f);

			if (tcu::boolAny(tcu::greaterThan(tcu::abs(primitiveVertex - generatedElementVertex), equalThreshold)))
			{
				if (numErrors++ < errorFloodThreshold)
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "Element at index " << (readNdx) << " (tessellation invocation " << tessellatedPrimitiveNdx << ", geometry primitive " << generatedPrimitiveNdx << ", emitted vertex " << primitiveVertexNdx << "):\n"
						<< "\tfeedback result was not contant over whole primitive.\n"
						<< "\tfirst emitted value: " << primitiveVertex << "\n"
						<< "\tcurrent emitted value:" << generatedElementVertex << "\n"
						<< tcu::TestLog::EndMessage;
				}

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got multiple different feedback values for a single primitive");
			}

			readNdx++;
		}
	}

	if (numErrors > errorFloodThreshold)
		m_testCtx.getLog() << tcu::TestLog::Message << "Omitted " << (numErrors - errorFloodThreshold) << " error(s)." << tcu::TestLog::EndMessage;
}

static bool feedbackResultCompare (const tcu::Vec4& a, const tcu::Vec4& b)
{
	if (a.x() < b.x())
		return true;
	if (a.x() > b.x())
		return false;

	return a.y() < b.y();
}

void FeedbackPrimitiveTypeCase::verifyRenderedImage (const tcu::Surface& image, const std::vector<tcu::Vec4>& tfVertices)
{
	std::vector<tcu::Vec4> vertices;

	m_testCtx.getLog() << tcu::TestLog::Message << "Comparing result image against feedback results." << tcu::TestLog::EndMessage;

	// Check only unique vertices
	std::unique_copy(tfVertices.begin(), tfVertices.end(), std::back_insert_iterator<std::vector<tcu::Vec4> >(vertices));
	std::sort(vertices.begin(), vertices.end(), feedbackResultCompare);
	vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());

	// Verifying vertices recorded with feedback actually ended up on the result image
	for (int ndx = 0; ndx < (int)vertices.size(); ++ndx)
	{
		// Rasterization (of lines) may deviate by one pixel. In addition to that, allow minimal errors in rasterized position vs. feedback result.
		// This minimal error could result in a difference in rounding => allow one additional pixel in deviation

		const int			rasterDeviation	= 2;
		const tcu::IVec2	rasterPos		((int)deFloatRound((vertices[ndx].x() * 0.5f + 0.5f) * (float)image.getWidth()), (int)deFloatRound((vertices[ndx].y() * 0.5f + 0.5f) * (float)image.getHeight()));

		// Find produced rasterization results
		bool				found			= false;

		for (int dy = -rasterDeviation; dy <= rasterDeviation && !found; ++dy)
		for (int dx = -rasterDeviation; dx <= rasterDeviation && !found; ++dx)
		{
			// Raster result could end up outside the viewport
			if (rasterPos.x() + dx < 0 || rasterPos.x() + dx >= image.getWidth() ||
				rasterPos.y() + dy < 0 || rasterPos.y() + dy >= image.getHeight())
				found = true;
			else
			{
				const tcu::RGBA result = image.getPixel(rasterPos.x() + dx, rasterPos.y() + dy);

				if(!isBlack(result))
					found = true;
			}
		}

		if (!found)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "Vertex " << vertices[ndx] << "\n"
				<< "\tCould not find rasterization output for vertex.\n"
				<< "\tExpected non-black pixels near " << rasterPos
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid result image");
		}
	}
}

void FeedbackPrimitiveTypeCase::genTransformFeedback (void)
{
	const glw::Functions&			gl						= m_context.getRenderContext().getFunctions();
	const int						elementsPerPrimitive	= getNumGeneratedElementsPerPrimitive();
	const int						feedbackPrimitives		= getNumGeneratedPrimitives();
	const int						feedbackElements		= elementsPerPrimitive * feedbackPrimitives;
	const std::vector<tcu::Vec4>	initialBuffer			(feedbackElements, tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f));

	gl.genTransformFeedbacks(1, &m_feedbackID);
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_feedbackID);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen transform feedback");

	gl.genBuffers(1, &m_feedbackBuffer);
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_feedbackBuffer);
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(tcu::Vec4) * initialBuffer.size(), initialBuffer[0].getPtr(), GL_STATIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "gen feedback buffer");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_feedbackBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind feedback buffer");
}

static int getTriangleNumOutputPrimitives (int tessellationLevel)
{
	if (tessellationLevel == 1)
		return 1;
	else if (tessellationLevel == 2)
		return 6;
	else
		return 3 * (2 + 2 * (tessellationLevel - 2)) + getTriangleNumOutputPrimitives(tessellationLevel - 2);
}

static int getTriangleNumOutputPrimitivesPoints (int tessellationLevel)
{
	if (tessellationLevel == 0)
		return 1;
	else if (tessellationLevel == 1)
		return 3;
	else
		return 3 + 3 * (tessellationLevel - 1) + getTriangleNumOutputPrimitivesPoints(tessellationLevel - 2);
}

int FeedbackPrimitiveTypeCase::getNumGeneratedElementsPerPrimitive (void) const
{
	if (m_geometryOutputType == GEOMETRY_OUTPUT_TRIANGLES)
		return 3;
	else if (m_geometryOutputType == GEOMETRY_OUTPUT_LINES)
		return 2;
	else if (m_geometryOutputType == GEOMETRY_OUTPUT_POINTS)
		return 1;
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

int FeedbackPrimitiveTypeCase::getNumGeneratedPrimitives (void) const
{
	return getNumTessellatedPrimitives() * getGeometryAmplification();
}

int FeedbackPrimitiveTypeCase::getNumTessellatedPrimitives (void) const
{
	const int tessellationLevel = 3;

	if (m_tessellationPointMode == TESSELLATION_POINTMODE_OFF)
	{
		if (m_tessellationOutput == TESSELLATION_OUT_TRIANGLES)
			return getTriangleNumOutputPrimitives(tessellationLevel);
		else if (m_tessellationOutput == TESSELLATION_OUT_QUADS)
			return tessellationLevel * tessellationLevel * 2; // tessellated as triangles
		else if (m_tessellationOutput == TESSELLATION_OUT_ISOLINES)
			return tessellationLevel * tessellationLevel;
	}
	else if (m_tessellationPointMode == TESSELLATION_POINTMODE_ON)
	{
		if (m_tessellationOutput == TESSELLATION_OUT_TRIANGLES)
			return getTriangleNumOutputPrimitivesPoints(tessellationLevel);
		else if (m_tessellationOutput == TESSELLATION_OUT_QUADS)
			return (tessellationLevel + 1) * (tessellationLevel + 1);
		else if (m_tessellationOutput == TESSELLATION_OUT_ISOLINES)
			return tessellationLevel * (tessellationLevel + 1);
	}

	DE_ASSERT(false);
	return -1;
}

int FeedbackPrimitiveTypeCase::getGeometryAmplification (void) const
{
	const int outputAmplification	= (m_geometryOutputType == GEOMETRY_OUTPUT_LINES) ? (2) : (1);
	const int numInputVertices		= (m_tessellationPointMode) ? (1) : (m_tessellationOutput == TESSELLATION_OUT_ISOLINES) ? (2) : (3);

	return outputAmplification * numInputVertices;
}

glw::GLenum FeedbackPrimitiveTypeCase::getOutputPrimitiveGLType (void) const
{
	if (m_geometryOutputType == GEOMETRY_OUTPUT_TRIANGLES)
		return GL_TRIANGLES;
	else if (m_geometryOutputType == GEOMETRY_OUTPUT_LINES)
		return GL_LINES;
	else if (m_geometryOutputType == GEOMETRY_OUTPUT_POINTS)
		return GL_POINTS;
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

std::string FeedbackPrimitiveTypeCase::getVertexSource (void) const
{
	return specializeShader(s_positionVertexShader, m_context.getRenderContext().getType());
}

std::string FeedbackPrimitiveTypeCase::getFragmentSource (void) const
{
	return specializeShader(s_whiteOutputFragmentShader, m_context.getRenderContext().getType());
}

std::string FeedbackPrimitiveTypeCase::getTessellationControlSource (void) const
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout(vertices = 9) out;\n"
			"\n"
			"uniform highp float u_innerTessellationLevel;\n"
			"uniform highp float u_outerTessellationLevel;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	if (gl_PatchVerticesIn != 4)\n"
			"		return;\n"
			"\n"
			"	// Convert input 2x2 grid to 3x3 grid\n"
			"	float xweight = float(gl_InvocationID % 3) / 2.0f;\n"
			"	float yweight = float(gl_InvocationID / 3) / 2.0f;\n"
			"\n"
			"	vec4 y0 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, yweight);\n"
			"	vec4 y1 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, yweight);\n"
			"\n"
			"	gl_out[gl_InvocationID].gl_Position = mix(y0, y1, xweight);\n"
			"\n";

	if (m_tessellationOutput == TESSELLATION_OUT_TRIANGLES)
		buf <<	"	gl_TessLevelOuter[0] = 3.0;\n"
				"	gl_TessLevelOuter[1] = 3.0;\n"
				"	gl_TessLevelOuter[2] = 3.0;\n"
				"	gl_TessLevelInner[0] = 3.0;\n";
	else if (m_tessellationOutput == TESSELLATION_OUT_QUADS)
		buf <<	"	gl_TessLevelOuter[0] = 3.0;\n"
				"	gl_TessLevelOuter[1] = 3.0;\n"
				"	gl_TessLevelOuter[2] = 3.0;\n"
				"	gl_TessLevelOuter[3] = 3.0;\n"
				"	gl_TessLevelInner[0] = 3.0;\n"
				"	gl_TessLevelInner[1] = 3.0;\n";
	else if (m_tessellationOutput == TESSELLATION_OUT_ISOLINES)
		buf <<	"	gl_TessLevelOuter[0] = 3.0;\n"
				"	gl_TessLevelOuter[1] = 3.0;\n";
	else
		DE_ASSERT(false);

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string FeedbackPrimitiveTypeCase::getTessellationEvaluationSource (void) const
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout("
				<< ((m_tessellationOutput == TESSELLATION_OUT_TRIANGLES) ? ("triangles") : (m_tessellationOutput == TESSELLATION_OUT_QUADS) ? ("quads") : ("isolines"))
				<< ((m_tessellationPointMode) ? (", point_mode") : (""))
				<< ") in;\n"
			"\n"
			"out highp vec4 v_tessellationCoords;\n"
			"\n"
			"// note: No need to use precise gl_Position since we do not require gapless geometry\n"
			"void main (void)\n"
			"{\n"
			"	if (gl_PatchVerticesIn != 9)\n"
			"		return;\n"
			"\n"
			"	vec4 patchCentroid = vec4(0.0);\n"
			"	for (int ndx = 0; ndx < gl_PatchVerticesIn; ++ndx)\n"
			"		patchCentroid += gl_in[ndx].gl_Position;\n"
			"	patchCentroid /= patchCentroid.w;\n"
			"\n";

	if (m_tessellationOutput == TESSELLATION_OUT_TRIANGLES)
		buf <<	"	// map barycentric coords to 2d coords\n"
				"	const vec3 tessDirX = vec3( 0.4,  0.4, 0.0);\n"
				"	const vec3 tessDirY = vec3( 0.0, -0.4, 0.0);\n"
				"	const vec3 tessDirZ = vec3(-0.4,  0.4, 0.0);\n"
				"	gl_Position = patchCentroid + vec4(gl_TessCoord.x * tessDirX + gl_TessCoord.y * tessDirY + gl_TessCoord.z * tessDirZ, 0.0);\n";
	else if (m_tessellationOutput == TESSELLATION_OUT_QUADS || m_tessellationOutput == TESSELLATION_OUT_ISOLINES)
		buf <<	"	gl_Position = patchCentroid + vec4(gl_TessCoord.x * 0.8 - 0.4, gl_TessCoord.y * 0.8 - 0.4, 0.0, 0.0);\n";
	else
		DE_ASSERT(false);

	buf <<	"	v_tessellationCoords = vec4(gl_TessCoord, 0.0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string FeedbackPrimitiveTypeCase::getGeometrySource (void) const
{
	const char* const	geometryInputPrimitive			= (m_tessellationPointMode) ? ("points") : (m_tessellationOutput == TESSELLATION_OUT_ISOLINES) ? ("lines") : ("triangles");
	const char* const	geometryOutputPrimitive			= (m_geometryOutputType == GEOMETRY_OUTPUT_POINTS) ? ("points") : (m_geometryOutputType == GEOMETRY_OUTPUT_LINES) ? ("line_strip") : ("triangle_strip");
	const int			numInputVertices				= (m_tessellationPointMode) ? (1) : (m_tessellationOutput == TESSELLATION_OUT_ISOLINES) ? (2) : (3);
	const int			numSingleVertexOutputVertices	= (m_geometryOutputType == GEOMETRY_OUTPUT_POINTS) ? (1) : (m_geometryOutputType == GEOMETRY_OUTPUT_LINES) ? (4) : (3);
	const int			numEmitVertices					= numInputVertices * numSingleVertexOutputVertices;
	std::ostringstream	buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_GEOMETRY_SHADER}"
			"layout(" << geometryInputPrimitive << ") in;\n"
			"layout(" << geometryOutputPrimitive << ", max_vertices=" << numEmitVertices <<") out;\n"
			"\n"
			"in highp vec4 v_tessellationCoords[];\n"
			"out highp vec4 tf_someVertexPosition;\n"
			"\n"
			"void main (void)\n"
			"{\n"
			"	// Emit primitive\n"
			"	for (int ndx = 0; ndx < gl_in.length(); ++ndx)\n"
			"	{\n";

	switch (m_geometryOutputType)
	{
		case GEOMETRY_OUTPUT_POINTS:
			buf <<	"		// Draw point on vertex\n"
					"		gl_Position = gl_in[ndx].gl_Position;\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n";
			break;

		case GEOMETRY_OUTPUT_LINES:
			buf <<	"		// Draw cross on vertex\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4(-0.02, -0.02, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4( 0.02,  0.02, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		EndPrimitive();\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4( 0.02, -0.02, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4(-0.02,  0.02, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		EndPrimitive();\n";
			break;

		case GEOMETRY_OUTPUT_TRIANGLES:
			buf <<	"		// Draw triangle on vertex\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4(  0.00, -0.02, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4(  0.02,  0.00, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		gl_Position = gl_in[ndx].gl_Position + vec4( -0.02,  0.00, 0.0, 0.0);\n"
					"		tf_someVertexPosition = gl_in[gl_in.length() - 1].gl_Position;\n"
					"		EmitVertex();\n"
					"		EndPrimitive();\n";
			break;

		default:
			DE_ASSERT(false);
			return "";
	}

	buf <<	"	}\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

const char* FeedbackPrimitiveTypeCase::getTessellationOutputDescription (TessellationOutputType tessellationOutput, TessellationPointMode pointMode)
{
	switch (tessellationOutput)
	{
		case TESSELLATION_OUT_TRIANGLES:	return (pointMode) ? ("points (triangles in point mode)") : ("triangles");
		case TESSELLATION_OUT_QUADS:		return (pointMode) ? ("points (quads in point mode)")     : ("quads");
		case TESSELLATION_OUT_ISOLINES:		return (pointMode) ? ("points (isolines in point mode)")  : ("isolines");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

const char* FeedbackPrimitiveTypeCase::getGeometryInputDescription (TessellationOutputType tessellationOutput, TessellationPointMode pointMode)
{
	switch (tessellationOutput)
	{
		case TESSELLATION_OUT_TRIANGLES:	return (pointMode) ? ("points") : ("triangles");
		case TESSELLATION_OUT_QUADS:		return (pointMode) ? ("points") : ("triangles");
		case TESSELLATION_OUT_ISOLINES:		return (pointMode) ? ("points") : ("lines");
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

const char* FeedbackPrimitiveTypeCase::getGeometryOutputDescription (GeometryOutputType geometryOutput)
{
	switch (geometryOutput)
	{
		case GEOMETRY_OUTPUT_POINTS:		return "points";
		case GEOMETRY_OUTPUT_LINES:			return "lines";
		case GEOMETRY_OUTPUT_TRIANGLES:		return "triangles";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

class PointSizeCase : public TestCase
{
public:
	enum Flags
	{
		FLAG_VERTEX_SET						= 0x01,		// !< set gl_PointSize in vertex shader
		FLAG_TESSELLATION_CONTROL_SET		= 0x02,		// !< set gl_PointSize in tessellation evaluation shader
		FLAG_TESSELLATION_EVALUATION_SET	= 0x04,		// !< set gl_PointSize in tessellation control shader
		FLAG_TESSELLATION_ADD				= 0x08,		// !< read and add to gl_PointSize in tessellation shader pair
		FLAG_TESSELLATION_DONT_SET			= 0x10,		// !< don't set gl_PointSize in tessellation shader
		FLAG_GEOMETRY_SET					= 0x20,		// !< set gl_PointSize in geometry shader
		FLAG_GEOMETRY_ADD					= 0x40,		// !< read and add to gl_PointSize in geometry shader
		FLAG_GEOMETRY_DONT_SET				= 0x80,		// !< don't set gl_PointSize in geometry shader
	};

						PointSizeCase					(Context& context, const char* name, const char* description, int flags);
						~PointSizeCase					(void);

	static std::string	genTestCaseName					(int flags);
	static std::string	genTestCaseDescription			(int flags);

private:
	void				init							(void);
	void				deinit							(void);
	IterateResult		iterate							(void);

	void				checkExtensions					(void) const;
	void				checkPointSizeRequirements		(void) const;

	void				renderTo						(tcu::Surface& dst);
	bool				verifyImage						(const tcu::Surface& src);
	int					getExpectedPointSize			(void) const;

	std::string			genVertexSource					(void) const;
	std::string			genFragmentSource				(void) const;
	std::string			genTessellationControlSource	(void) const;
	std::string			genTessellationEvaluationSource	(void) const;
	std::string			genGeometrySource				(void) const;

	enum
	{
		RENDER_SIZE = 32,
	};

	const int			m_flags;
	glu::ShaderProgram*	m_program;
};

PointSizeCase::PointSizeCase (Context& context, const char* name, const char* description, int flags)
	: TestCase	(context, name, description)
	, m_flags	(flags)
	, m_program	(DE_NULL)
{
}

PointSizeCase::~PointSizeCase (void)
{
	deinit();
}

std::string PointSizeCase::genTestCaseName (int flags)
{
	std::ostringstream buf;

	// join per-bit descriptions into a single string with '_' separator
	if (flags & FLAG_VERTEX_SET)					buf																		<< "vertex_set";
	if (flags & FLAG_TESSELLATION_CONTROL_SET)		buf << ((flags & (FLAG_TESSELLATION_CONTROL_SET-1))		? ("_") : (""))	<< "control_set";
	if (flags & FLAG_TESSELLATION_EVALUATION_SET)	buf << ((flags & (FLAG_TESSELLATION_EVALUATION_SET-1))	? ("_") : (""))	<< "evaluation_set";
	if (flags & FLAG_TESSELLATION_ADD)				buf << ((flags & (FLAG_TESSELLATION_ADD-1))				? ("_") : (""))	<< "control_pass_eval_add";
	if (flags & FLAG_TESSELLATION_DONT_SET)			buf << ((flags & (FLAG_TESSELLATION_DONT_SET-1))		? ("_") : (""))	<< "eval_default";
	if (flags & FLAG_GEOMETRY_SET)					buf << ((flags & (FLAG_GEOMETRY_SET-1))					? ("_") : (""))	<< "geometry_set";
	if (flags & FLAG_GEOMETRY_ADD)					buf << ((flags & (FLAG_GEOMETRY_ADD-1))					? ("_") : (""))	<< "geometry_add";
	if (flags & FLAG_GEOMETRY_DONT_SET)				buf << ((flags & (FLAG_GEOMETRY_DONT_SET-1))			? ("_") : (""))	<< "geometry_default";

	return buf.str();
}

std::string PointSizeCase::genTestCaseDescription (int flags)
{
	std::ostringstream buf;

	// join per-bit descriptions into a single string with ", " separator
	if (flags & FLAG_VERTEX_SET)					buf																			<< "set point size in vertex shader";
	if (flags & FLAG_TESSELLATION_CONTROL_SET)		buf << ((flags & (FLAG_TESSELLATION_CONTROL_SET-1))		? (", ") : (""))	<< "set point size in tessellation control shader";
	if (flags & FLAG_TESSELLATION_EVALUATION_SET)	buf << ((flags & (FLAG_TESSELLATION_EVALUATION_SET-1))	? (", ") : (""))	<< "set point size in tessellation evaluation shader";
	if (flags & FLAG_TESSELLATION_ADD)				buf << ((flags & (FLAG_TESSELLATION_ADD-1))				? (", ") : (""))	<< "add to point size in tessellation shader";
	if (flags & FLAG_TESSELLATION_DONT_SET)			buf << ((flags & (FLAG_TESSELLATION_DONT_SET-1))		? (", ") : (""))	<< "don't set point size in tessellation evaluation shader";
	if (flags & FLAG_GEOMETRY_SET)					buf << ((flags & (FLAG_GEOMETRY_SET-1))					? (", ") : (""))	<< "set point size in geometry shader";
	if (flags & FLAG_GEOMETRY_ADD)					buf << ((flags & (FLAG_GEOMETRY_ADD-1))					? (", ") : (""))	<< "add to point size in geometry shader";
	if (flags & FLAG_GEOMETRY_DONT_SET)				buf << ((flags & (FLAG_GEOMETRY_DONT_SET-1))			? (", ") : (""))	<< "don't set point size in geometry shader";

	return buf.str();
}

void PointSizeCase::init (void)
{
	checkExtensions();
	checkPointSizeRequirements();

	// log

	if (m_flags & FLAG_VERTEX_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Setting point size in vertex shader to 2.0." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_TESSELLATION_CONTROL_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Setting point size in tessellation control shader to 4.0. (And ignoring it in evaluation)." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_TESSELLATION_EVALUATION_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Setting point size in tessellation evaluation shader to 4.0." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_TESSELLATION_ADD)
		m_testCtx.getLog() << tcu::TestLog::Message << "Reading point size in tessellation control shader and adding 2.0 to it in evaluation." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_TESSELLATION_DONT_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Not setting point size in tessellation evaluation shader (resulting in the default point size)." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_GEOMETRY_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Setting point size in geometry shader to 6.0." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_GEOMETRY_ADD)
		m_testCtx.getLog() << tcu::TestLog::Message << "Reading point size in geometry shader and adding 2.0." << tcu::TestLog::EndMessage;
	if (m_flags & FLAG_GEOMETRY_DONT_SET)
		m_testCtx.getLog() << tcu::TestLog::Message << "Not setting point size in geometry shader (resulting in the default point size)." << tcu::TestLog::EndMessage;

	// program

	{
		glu::ProgramSources sources;
		sources	<< glu::VertexSource(genVertexSource())
				<< glu::FragmentSource(genFragmentSource());

		if (m_flags & (FLAG_TESSELLATION_CONTROL_SET | FLAG_TESSELLATION_EVALUATION_SET | FLAG_TESSELLATION_ADD | FLAG_TESSELLATION_DONT_SET))
			sources << glu::TessellationControlSource(genTessellationControlSource())
					<< glu::TessellationEvaluationSource(genTessellationEvaluationSource());

		if (m_flags & (FLAG_GEOMETRY_SET | FLAG_GEOMETRY_ADD | FLAG_GEOMETRY_DONT_SET))
			sources << glu::GeometrySource(genGeometrySource());

		m_program = new glu::ShaderProgram(m_context.getRenderContext(), sources);

		m_testCtx.getLog() << *m_program;
		if (!m_program->isOk())
			throw tcu::TestError("failed to build program");
	}
}

void PointSizeCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;
}

PointSizeCase::IterateResult PointSizeCase::iterate (void)
{
	tcu::Surface resultImage(RENDER_SIZE, RENDER_SIZE);

	renderTo(resultImage);

	if (verifyImage(resultImage))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");

	return STOP;
}

void PointSizeCase::checkExtensions (void) const
{
	std::vector<std::string>	requiredExtensions;
	const bool					supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	bool						allOk				= true;

	if ((m_flags & (FLAG_TESSELLATION_CONTROL_SET | FLAG_TESSELLATION_EVALUATION_SET | FLAG_TESSELLATION_ADD | FLAG_TESSELLATION_DONT_SET)) && !supportsES32)
		requiredExtensions.push_back("GL_EXT_tessellation_shader");

	if (m_flags & (FLAG_TESSELLATION_CONTROL_SET | FLAG_TESSELLATION_EVALUATION_SET | FLAG_TESSELLATION_ADD))
		requiredExtensions.push_back("GL_EXT_tessellation_point_size");

	if ((m_flags & (m_flags & (FLAG_GEOMETRY_SET | FLAG_GEOMETRY_ADD | FLAG_GEOMETRY_DONT_SET))) && !supportsES32)
		requiredExtensions.push_back("GL_EXT_geometry_shader");

	if (m_flags & (m_flags & (FLAG_GEOMETRY_SET | FLAG_GEOMETRY_ADD)))
		requiredExtensions.push_back("GL_EXT_geometry_point_size");

	for (int ndx = 0; ndx < (int)requiredExtensions.size(); ++ndx)
		if (!m_context.getContextInfo().isExtensionSupported(requiredExtensions[ndx].c_str()))
			allOk = false;

	if (!allOk)
	{
		std::ostringstream extensionList;

		for (int ndx = 0; ndx < (int)requiredExtensions.size(); ++ndx)
		{
			if (ndx != 0)
				extensionList << ", ";
			extensionList << requiredExtensions[ndx];
		}

		throw tcu::NotSupportedError("Test requires {" + extensionList.str() + "} extension(s)");
	}
}

void PointSizeCase::checkPointSizeRequirements (void) const
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	float					aliasedSizeRange[2]	= { 0.0f, 0.0f };
	const int				requiredSize		= getExpectedPointSize();

	gl.getFloatv(GL_ALIASED_POINT_SIZE_RANGE, aliasedSizeRange);

	if (float(requiredSize) > aliasedSizeRange[1])
		throw tcu::NotSupportedError("Test requires point size " + de::toString(requiredSize));
}

void PointSizeCase::renderTo (tcu::Surface& dst)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const bool				tessellationActive	= (m_flags & (FLAG_TESSELLATION_CONTROL_SET | FLAG_TESSELLATION_EVALUATION_SET | FLAG_TESSELLATION_ADD | FLAG_TESSELLATION_DONT_SET)) != 0;
	const int				positionLocation	= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const glu::VertexArray	vao					(m_context.getRenderContext());

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering single point." << tcu::TestLog::EndMessage;

	if (positionLocation == -1)
		throw tcu::TestError("Attribute a_position location was -1");

	gl.viewport(0, 0, RENDER_SIZE, RENDER_SIZE);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindVertexArray(*vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind vao");

	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

	gl.vertexAttrib4f(positionLocation, 0.0f, 0.0f, 0.0f, 1.0f);

	if (tessellationActive)
	{
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "draw patches");
	}
	else
	{
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "draw points");
	}

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
}

bool PointSizeCase::verifyImage (const tcu::Surface& src)
{
	const bool MSAATarget	= (m_context.getRenderTarget().getNumSamples() > 1);
	const int expectedSize	= getExpectedPointSize();

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying rendered point size. Expecting " << expectedSize << " pixels." << tcu::TestLog::EndMessage;
	m_testCtx.getLog() << tcu::TestLog::Image("RenderImage", "Rendered image", src.getAccess());

	{
		bool		resultAreaFound	= false;
		tcu::IVec4	resultArea;

		// Find rasterization output area

		for (int y = 0; y < src.getHeight(); ++y)
		for (int x = 0; x < src.getWidth();  ++x)
		{
			if (!isBlack(src.getPixel(x, y)))
			{
				if (!resultAreaFound)
				{
					// first fragment
					resultArea = tcu::IVec4(x, y, x + 1, y + 1);
					resultAreaFound = true;
				}
				else
				{
					// union area
					resultArea.x() = de::min(resultArea.x(), x);
					resultArea.y() = de::min(resultArea.y(), y);
					resultArea.z() = de::max(resultArea.z(), x+1);
					resultArea.w() = de::max(resultArea.w(), y+1);
				}
			}
		}

		if (!resultAreaFound)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verification failed, could not find any point fragments." << tcu::TestLog::EndMessage;
			return false;
		}

		// verify area size
		if (MSAATarget)
		{
			const tcu::IVec2 pointSize = resultArea.swizzle(2,3) - resultArea.swizzle(0, 1);

			// MSAA: edges may be a little fuzzy
			if (de::abs(pointSize.x() - pointSize.y()) > 1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Rasterized point is not a square. Detected point size was " << pointSize << tcu::TestLog::EndMessage;
				return false;
			}

			// MSAA may produce larger areas, allow one pixel larger
			if (expectedSize != de::max(pointSize.x(), pointSize.y()) && (expectedSize+1) != de::max(pointSize.x(), pointSize.y()))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Point size invalid, expected " << expectedSize << ", got " << de::max(pointSize.x(), pointSize.y()) << tcu::TestLog::EndMessage;
				return false;
			}
		}
		else
		{
			const tcu::IVec2 pointSize = resultArea.swizzle(2,3) - resultArea.swizzle(0, 1);

			if (pointSize.x() != pointSize.y())
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Rasterized point is not a square. Point size was " << pointSize << tcu::TestLog::EndMessage;
				return false;
			}

			if (pointSize.x() != expectedSize)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR! Point size invalid, expected " << expectedSize << ", got " << pointSize.x() << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return true;
}

int PointSizeCase::getExpectedPointSize (void) const
{
	int addition = 0;

	// geometry
	if (m_flags & FLAG_GEOMETRY_DONT_SET)
		return 1;
	else if (m_flags & FLAG_GEOMETRY_SET)
		return 6;
	else if (m_flags & FLAG_GEOMETRY_ADD)
		addition += 2;

	// tessellation
	if (m_flags & FLAG_TESSELLATION_EVALUATION_SET)
		return 4 + addition;
	else if (m_flags & FLAG_TESSELLATION_ADD)
		addition += 2;
	else if (m_flags & (FLAG_TESSELLATION_CONTROL_SET | FLAG_TESSELLATION_DONT_SET))
	{
		DE_ASSERT((m_flags & FLAG_GEOMETRY_ADD) == 0); // reading pointSize undefined
		return 1;
	}

	// vertex
	if (m_flags & FLAG_VERTEX_SET)
		return 2 + addition;

	// undefined
	DE_ASSERT(false);
	return -1;
}

std::string PointSizeCase::genVertexSource (void) const
{
	std::ostringstream buf;

	buf	<< "${VERSION_DECL}\n"
		<< "in highp vec4 a_position;\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	gl_Position = a_position;\n";

	if (m_flags & FLAG_VERTEX_SET)
		buf << "	gl_PointSize = 2.0;\n";

	buf	<< "}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string PointSizeCase::genFragmentSource (void) const
{
	return specializeShader(s_whiteOutputFragmentShader, m_context.getRenderContext().getType());
}

std::string PointSizeCase::genTessellationControlSource (void) const
{
	std::ostringstream buf;

	buf	<< "${VERSION_DECL}\n"
		<< "${EXTENSION_TESSELATION_SHADER}"
		<< ((m_flags & FLAG_TESSELLATION_DONT_SET) ? ("") : ("#extension GL_EXT_tessellation_point_size : require\n"))
		<< "layout(vertices = 1) out;\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	gl_TessLevelOuter[0] = 3.0;\n"
		<< "	gl_TessLevelOuter[1] = 3.0;\n"
		<< "	gl_TessLevelOuter[2] = 3.0;\n"
		<< "	gl_TessLevelInner[0] = 3.0;\n"
		<< "	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n";

	if (m_flags & FLAG_TESSELLATION_ADD)
		buf << "	// pass as is to eval\n"
			<< "	gl_out[gl_InvocationID].gl_PointSize = gl_in[gl_InvocationID].gl_PointSize;\n";
	else if (m_flags & FLAG_TESSELLATION_CONTROL_SET)
		buf << "	// thrown away\n"
			<< "	gl_out[gl_InvocationID].gl_PointSize = 4.0;\n";

	buf	<< "}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string PointSizeCase::genTessellationEvaluationSource (void) const
{
	std::ostringstream buf;

	buf	<< "${VERSION_DECL}\n"
		<< "${EXTENSION_TESSELATION_SHADER}"
		<< ((m_flags & FLAG_TESSELLATION_DONT_SET) ? ("") : ("#extension GL_EXT_tessellation_point_size : require\n"))
		<< "layout(triangles, point_mode) in;\n"
		<< "void main ()\n"
		<< "{\n"
		<< "	// hide all but one vertex\n"
		<< "	if (gl_TessCoord.x < 0.99)\n"
		<< "		gl_Position = vec4(-2.0, 0.0, 0.0, 1.0);\n"
		<< "	else\n"
		<< "		gl_Position = gl_in[0].gl_Position;\n";

	if (m_flags & FLAG_TESSELLATION_ADD)
		buf << "\n"
			<< "	// add to point size\n"
			<< "	gl_PointSize = gl_in[0].gl_PointSize + 2.0;\n";
	else if (m_flags & FLAG_TESSELLATION_EVALUATION_SET)
		buf << "\n"
			<< "	// set point size\n"
			<< "	gl_PointSize = 4.0;\n";

	buf	<< "}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string PointSizeCase::genGeometrySource (void) const
{
	std::ostringstream buf;

	buf	<< "${VERSION_DECL}\n"
		<< "${EXTENSION_GEOMETRY_SHADER}"
		<< ((m_flags & FLAG_GEOMETRY_DONT_SET) ? ("") : ("#extension GL_EXT_geometry_point_size : require\n"))
		<< "layout (points) in;\n"
		<< "layout (points, max_vertices=1) out;\n"
		<< "\n"
		<< "void main ()\n"
		<< "{\n";

	if (m_flags & FLAG_GEOMETRY_SET)
		buf	<< "	gl_Position = gl_in[0].gl_Position;\n"
			<< "	gl_PointSize = 6.0;\n";
	else if (m_flags & FLAG_GEOMETRY_ADD)
		buf	<< "	gl_Position = gl_in[0].gl_Position;\n"
			<< "	gl_PointSize = gl_in[0].gl_PointSize + 2.0;\n";
	else if (m_flags & FLAG_GEOMETRY_DONT_SET)
		buf	<< "	gl_Position = gl_in[0].gl_Position;\n";

	buf	<< "	EmitVertex();\n"
		<< "}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

class AllowedRenderFailureException : public std::runtime_error
{
public:
	AllowedRenderFailureException (const char* message) : std::runtime_error(message) { }
};

class GridRenderCase : public TestCase
{
public:
	enum Flags
	{
		FLAG_TESSELLATION_MAX_SPEC						= 0x0001,
		FLAG_TESSELLATION_MAX_IMPLEMENTATION			= 0x0002,
		FLAG_GEOMETRY_MAX_SPEC							= 0x0004,
		FLAG_GEOMETRY_MAX_IMPLEMENTATION				= 0x0008,
		FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC				= 0x0010,
		FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION	= 0x0020,

		FLAG_GEOMETRY_SCATTER_INSTANCES					= 0x0040,
		FLAG_GEOMETRY_SCATTER_PRIMITIVES				= 0x0080,
		FLAG_GEOMETRY_SEPARATE_PRIMITIVES				= 0x0100, //!< if set, geometry shader outputs separate grid cells and not continuous slices
		FLAG_GEOMETRY_SCATTER_LAYERS					= 0x0200,

		FLAG_ALLOW_OUT_OF_MEMORY						= 0x0400, //!< allow draw command to set GL_OUT_OF_MEMORY
	};

						GridRenderCase					(Context& context, const char* name, const char* description, int flags);
						~GridRenderCase					(void);

private:
	void				init							(void);
	void				deinit							(void);
	IterateResult		iterate							(void);

	void				renderTo						(std::vector<tcu::Surface>& dst);
	bool				verifyResultLayer				(int layerNdx, const tcu::Surface& dst);

	std::string			getVertexSource					(void);
	std::string			getFragmentSource				(void);
	std::string			getTessellationControlSource	(int tessLevel);
	std::string			getTessellationEvaluationSource	(int tessLevel);
	std::string			getGeometryShaderSource			(int numPrimitives, int numInstances, int tessLevel);

	enum
	{
		RENDER_SIZE = 256
	};

	const int			m_flags;

	glu::ShaderProgram*	m_program;
	deUint32			m_texture;
	int					m_numLayers;
};

GridRenderCase::GridRenderCase (Context& context, const char* name, const char* description, int flags)
	: TestCase		(context, name, description)
	, m_flags		(flags)
	, m_program		(DE_NULL)
	, m_texture		(0)
	, m_numLayers	(1)
{
	DE_ASSERT(((m_flags & FLAG_TESSELLATION_MAX_SPEC) == 0)			|| ((m_flags & FLAG_TESSELLATION_MAX_IMPLEMENTATION) == 0));
	DE_ASSERT(((m_flags & FLAG_GEOMETRY_MAX_SPEC) == 0)				|| ((m_flags & FLAG_GEOMETRY_MAX_IMPLEMENTATION) == 0));
	DE_ASSERT(((m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC) == 0)	|| ((m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION) == 0));
	DE_ASSERT(((m_flags & (FLAG_GEOMETRY_SCATTER_PRIMITIVES | FLAG_GEOMETRY_SCATTER_LAYERS)) != 0) == ((m_flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) != 0));
}

GridRenderCase::~GridRenderCase (void)
{
	deinit();
}

void GridRenderCase::init (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// Requirements

	if (!supportsES32 &&
		(!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		 !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader")))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	if ((m_flags & FLAG_GEOMETRY_SCATTER_LAYERS) == 0)
	{
		if (m_context.getRenderTarget().getWidth() < RENDER_SIZE ||
			m_context.getRenderTarget().getHeight() < RENDER_SIZE)
			throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_SIZE) + "x" + de::toString<int>(RENDER_SIZE) + " or larger render target.");
	}

	// Log

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing tessellation and geometry shaders that output a large number of primitives.\n"
		<< getDescription()
		<< tcu::TestLog::EndMessage;

	// Render target
	if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
	{
		// set limits
		m_numLayers = 8;

		m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to 2d texture array, numLayers = " << m_numLayers << tcu::TestLog::EndMessage;

		gl.genTextures(1, &m_texture);
		gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
		gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, RENDER_SIZE, RENDER_SIZE, m_numLayers);

		gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLU_EXPECT_NO_ERROR(gl.getError(), "gen texture");
	}

	// Gen program
	{
		glu::ProgramSources	sources;
		int					tessGenLevel = -1;

		sources	<< glu::VertexSource(getVertexSource())
				<< glu::FragmentSource(getFragmentSource());

		// Tessellation limits
		{
			if (m_flags & FLAG_TESSELLATION_MAX_IMPLEMENTATION)
			{
				gl.getIntegerv(GL_MAX_TESS_GEN_LEVEL, &tessGenLevel);
				GLU_EXPECT_NO_ERROR(gl.getError(), "query tessellation limits");
			}
			else if (m_flags & FLAG_TESSELLATION_MAX_SPEC)
			{
				tessGenLevel = 64;
			}
			else
			{
				tessGenLevel = 5;
			}

			m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Tessellation level: " << tessGenLevel << ", mode = quad.\n"
					<< "\tEach input patch produces " << (tessGenLevel*tessGenLevel) << " (" << (tessGenLevel*tessGenLevel*2) << " triangles)\n"
					<< tcu::TestLog::EndMessage;

			sources << glu::TessellationControlSource(getTessellationControlSource(tessGenLevel))
					<< glu::TessellationEvaluationSource(getTessellationEvaluationSource(tessGenLevel));
		}

		// Geometry limits
		{
			int		geometryOutputComponents		= -1;
			int		geometryOutputVertices			= -1;
			int		geometryTotalOutputComponents	= -1;
			int		geometryShaderInvocations		= -1;
			bool	logGeometryLimits				= false;
			bool	logInvocationLimits				= false;

			if (m_flags & FLAG_GEOMETRY_MAX_IMPLEMENTATION)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Using implementation maximum geometry shader output limits." << tcu::TestLog::EndMessage;

				gl.getIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &geometryOutputComponents);
				gl.getIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &geometryOutputVertices);
				gl.getIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &geometryTotalOutputComponents);
				GLU_EXPECT_NO_ERROR(gl.getError(), "query geometry limits");

				logGeometryLimits = true;
			}
			else if (m_flags & FLAG_GEOMETRY_MAX_SPEC)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Using geometry shader extension minimum maximum output limits." << tcu::TestLog::EndMessage;

				geometryOutputComponents = 128;
				geometryOutputVertices = 256;
				geometryTotalOutputComponents = 1024;
				logGeometryLimits = true;
			}
			else
			{
				geometryOutputComponents = 128;
				geometryOutputVertices = 16;
				geometryTotalOutputComponents = 1024;
			}

			if (m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION)
			{
				gl.getIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &geometryShaderInvocations);
				GLU_EXPECT_NO_ERROR(gl.getError(), "query geometry invocation limits");

				logInvocationLimits = true;
			}
			else if (m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC)
			{
				geometryShaderInvocations = 32;
				logInvocationLimits = true;
			}
			else
			{
				geometryShaderInvocations = 4;
			}

			if (logGeometryLimits || logInvocationLimits)
			{
				tcu::MessageBuilder msg(&m_testCtx.getLog());

				msg << "Geometry shader, targeting following limits:\n";

				if (logGeometryLimits)
					msg	<< "\tGL_MAX_GEOMETRY_OUTPUT_COMPONENTS = " << geometryOutputComponents << "\n"
						<< "\tGL_MAX_GEOMETRY_OUTPUT_VERTICES = " << geometryOutputVertices << "\n"
						<< "\tGL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS = " << geometryTotalOutputComponents << "\n";

				if (logInvocationLimits)
					msg << "\tGL_MAX_GEOMETRY_SHADER_INVOCATIONS = " << geometryShaderInvocations;

				msg << tcu::TestLog::EndMessage;
			}

			{
				const bool	separatePrimitives			= (m_flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) != 0;
				const int	numComponentsPerVertex		= 8; // vec4 pos, vec4 color
				int			numVerticesPerInvocation;
				int			numPrimitivesPerInvocation;
				int			geometryVerticesPerPrimitive;
				int			geometryPrimitivesOutPerPrimitive;

				if (separatePrimitives)
				{
					const int	numComponentLimit	= geometryTotalOutputComponents / (4 * numComponentsPerVertex);
					const int	numOutputLimit		= geometryOutputVertices / 4;

					numPrimitivesPerInvocation		= de::min(numComponentLimit, numOutputLimit);
					numVerticesPerInvocation		= numPrimitivesPerInvocation * 4;
				}
				else
				{
					// If FLAG_GEOMETRY_SEPARATE_PRIMITIVES is not set, geometry shader fills a rectangle area in slices.
					// Each slice is a triangle strip and is generated by a single shader invocation.
					// One slice with 4 segment ends (nodes) and 3 segments:
					//    .__.__.__.
					//    |\ |\ |\ |
					//    |_\|_\|_\|

					const int	numSliceNodesComponentLimit	= geometryTotalOutputComponents / (2 * numComponentsPerVertex);			// each node 2 vertices
					const int	numSliceNodesOutputLimit	= geometryOutputVertices / 2;											// each node 2 vertices
					const int	numSliceNodes				= de::min(numSliceNodesComponentLimit, numSliceNodesOutputLimit);

					numVerticesPerInvocation				= numSliceNodes * 2;
					numPrimitivesPerInvocation				= (numSliceNodes - 1) * 2;
				}

				geometryVerticesPerPrimitive = numVerticesPerInvocation * geometryShaderInvocations;
				geometryPrimitivesOutPerPrimitive = numPrimitivesPerInvocation * geometryShaderInvocations;

				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Geometry shader:\n"
					<< "\tTotal output vertex count per invocation: " << (numVerticesPerInvocation) << "\n"
					<< "\tTotal output primitive count per invocation: " << (numPrimitivesPerInvocation) << "\n"
					<< "\tNumber of invocations per primitive: " << geometryShaderInvocations << "\n"
					<< "\tTotal output vertex count per input primitive: " << (geometryVerticesPerPrimitive) << "\n"
					<< "\tTotal output primitive count per input primitive: " << (geometryPrimitivesOutPerPrimitive) << "\n"
					<< tcu::TestLog::EndMessage;

				sources	<< glu::GeometrySource(getGeometryShaderSource(numPrimitivesPerInvocation, geometryShaderInvocations, tessGenLevel));

				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Program:\n"
					<< "\tTotal program output vertices count per input patch: " << (tessGenLevel*tessGenLevel*2 * geometryVerticesPerPrimitive) << "\n"
					<< "\tTotal program output primitive count per input patch: " << (tessGenLevel*tessGenLevel*2 * geometryPrimitivesOutPerPrimitive) << "\n"
					<< tcu::TestLog::EndMessage;
			}
		}

		m_program = new glu::ShaderProgram(m_context.getRenderContext(), sources);
		m_testCtx.getLog() << *m_program;
		if (!m_program->isOk())
			throw tcu::TestError("failed to build program");
	}
}

void GridRenderCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;

	if (m_texture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texture);
		m_texture = 0;
	}
}

GridRenderCase::IterateResult GridRenderCase::iterate (void)
{
	std::vector<tcu::Surface>	renderedLayers	(m_numLayers);
	bool						allLayersOk		= true;

	for (int ndx = 0; ndx < m_numLayers; ++ndx)
		renderedLayers[ndx].setSize(RENDER_SIZE, RENDER_SIZE);

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering single point at the origin. Expecting yellow and green colored grid-like image. (High-frequency grid may appear unicolored)." << tcu::TestLog::EndMessage;

	try
	{
		renderTo(renderedLayers);
	}
	catch (const AllowedRenderFailureException& ex)
	{
		// Got accepted failure
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Could not render, reason: " << ex.what() << "\n"
			<< "Failure is allowed."
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

	for (int ndx = 0; ndx < m_numLayers; ++ndx)
		allLayersOk &= verifyResultLayer(ndx, renderedLayers[ndx]);

	if (allLayersOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
	return STOP;
}

void GridRenderCase::renderTo (std::vector<tcu::Surface>& dst)
{
	const glw::Functions&			gl					= m_context.getRenderContext().getFunctions();
	const int						positionLocation	= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const glu::VertexArray			vao					(m_context.getRenderContext());
	de::MovePtr<glu::Framebuffer>	fbo;

	if (positionLocation == -1)
		throw tcu::TestError("Attribute a_position location was -1");

	gl.viewport(0, 0, dst.front().getWidth(), dst.front().getHeight());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");

	gl.bindVertexArray(*vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind vao");

	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

	gl.vertexAttrib4f(positionLocation, 0.0f, 0.0f, 0.0f, 1.0f);

	if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
	{
		// clear texture contents
		{
			glu::Framebuffer clearFbo(m_context.getRenderContext());
			gl.bindFramebuffer(GL_FRAMEBUFFER, *clearFbo);

			for (int layerNdx = 0; layerNdx < m_numLayers; ++layerNdx)
			{
				gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0, layerNdx);
				gl.clear(GL_COLOR_BUFFER_BIT);
			}

			GLU_EXPECT_NO_ERROR(gl.getError(), "clear tex contents");
		}

		// create and bind layered fbo

		fbo = de::MovePtr<glu::Framebuffer>(new glu::Framebuffer(m_context.getRenderContext()));

		gl.bindFramebuffer(GL_FRAMEBUFFER, **fbo);
		gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen fbo");
	}
	else
	{
		// clear viewport
		gl.clear(GL_COLOR_BUFFER_BIT);
	}

	// draw
	{
		glw::GLenum glerror;

		gl.drawArrays(GL_PATCHES, 0, 1);

		glerror = gl.getError();
		if (glerror == GL_OUT_OF_MEMORY && (m_flags & FLAG_ALLOW_OUT_OF_MEMORY))
			throw AllowedRenderFailureException("got GL_OUT_OF_MEMORY while drawing");

		GLU_EXPECT_NO_ERROR(glerror, "draw patches");
	}

	// Read layers

	if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
	{
		glu::Framebuffer readFbo(m_context.getRenderContext());
		gl.bindFramebuffer(GL_FRAMEBUFFER, *readFbo);

		for (int layerNdx = 0; layerNdx < m_numLayers; ++layerNdx)
		{
			gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0, layerNdx);
			glu::readPixels(m_context.getRenderContext(), 0, 0, dst[layerNdx].getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");
		}
	}
	else
	{
		glu::readPixels(m_context.getRenderContext(), 0, 0, dst.front().getAccess());
		GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");
	}
}

bool GridRenderCase::verifyResultLayer (int layerNdx, const tcu::Surface& image)
{
	tcu::Surface	errorMask	(image.getWidth(), image.getHeight());
	bool			foundError	= false;

	tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying output layer " << layerNdx  << tcu::TestLog::EndMessage;

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth(); ++x)
	{
		const int		threshold	= 8;
		const tcu::RGBA	color		= image.getPixel(x, y);

		// Color must be a linear combination of green and yellow
		if (color.getGreen() < 255 - threshold || color.getBlue() > threshold)
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			foundError = true;
		}
	}

	if (!foundError)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message << "Image valid." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("ImageVerification", "Image verification")
			<< tcu::TestLog::Image("Result", "Rendered result", image.getAccess())
			<< tcu::TestLog::EndImageSet;
		return true;
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message << "Image verification failed, found invalid pixels." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("ImageVerification", "Image verification")
			<< tcu::TestLog::Image("Result", "Rendered result", image.getAccess())
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
		return false;
	}
}

std::string GridRenderCase::getVertexSource (void)
{
	return specializeShader(s_positionVertexShader, m_context.getRenderContext().getType());
}

std::string GridRenderCase::getFragmentSource (void)
{
	const char* source = "${VERSION_DECL}\n"
						 "flat in mediump vec4 v_color;\n"
						 "layout(location = 0) out mediump vec4 fragColor;\n"
						 "void main (void)\n"
						 "{\n"
						 "	fragColor = v_color;\n"
						 "}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

std::string GridRenderCase::getTessellationControlSource (int tessLevel)
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout(vertices=1) out;\n"
			"\n"
			"void main()\n"
			"{\n"
			"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			"	gl_TessLevelOuter[0] = " << tessLevel << ".0;\n"
			"	gl_TessLevelOuter[1] = " << tessLevel << ".0;\n"
			"	gl_TessLevelOuter[2] = " << tessLevel << ".0;\n"
			"	gl_TessLevelOuter[3] = " << tessLevel << ".0;\n"
			"	gl_TessLevelInner[0] = " << tessLevel << ".0;\n"
			"	gl_TessLevelInner[1] = " << tessLevel << ".0;\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string GridRenderCase::getTessellationEvaluationSource (int tessLevel)
{
	std::ostringstream buf;

	buf <<	"${VERSION_DECL}\n"
			"${EXTENSION_TESSELATION_SHADER}"
			"layout(quads) in;\n"
			"\n"
			"out mediump ivec2 v_tessellationGridPosition;\n"
			"\n"
			"// note: No need to use precise gl_Position since position does not depend on order\n"
			"void main (void)\n"
			"{\n";

	if (m_flags & (FLAG_GEOMETRY_SCATTER_INSTANCES | FLAG_GEOMETRY_SCATTER_PRIMITIVES | FLAG_GEOMETRY_SCATTER_LAYERS))
		buf <<	"	// Cover only a small area in a corner. The area will be expanded in geometry shader to cover whole viewport\n"
				"	gl_Position = vec4(gl_TessCoord.x * 0.3 - 1.0, gl_TessCoord.y * 0.3 - 1.0, 0.0, 1.0);\n";
	else
		buf <<	"	// Fill the whole viewport\n"
				"	gl_Position = vec4(gl_TessCoord.x * 2.0 - 1.0, gl_TessCoord.y * 2.0 - 1.0, 0.0, 1.0);\n";

	buf <<	"	// Calculate position in tessellation grid\n"
			"	v_tessellationGridPosition = ivec2(round(gl_TessCoord.xy * float(" << tessLevel << ")));\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string GridRenderCase::getGeometryShaderSource (int numPrimitives, int numInstances, int tessLevel)
{
	std::ostringstream buf;

	buf	<<	"${VERSION_DECL}\n"
			"${EXTENSION_GEOMETRY_SHADER}"
			"layout(triangles, invocations=" << numInstances << ") in;\n"
			"layout(triangle_strip, max_vertices=" << ((m_flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) ? (4 * numPrimitives) : (numPrimitives + 2)) << ") out;\n"
			"\n"
			"in mediump ivec2 v_tessellationGridPosition[];\n"
			"flat out highp vec4 v_color;\n"
			"\n"
			"void main ()\n"
			"{\n"
			"	const float equalThreshold = 0.001;\n"
			"	const float gapOffset = 0.0001; // subdivision performed by the geometry shader might produce gaps. Fill potential gaps by enlarging the output slice a little.\n"
			"\n"
			"	// Input triangle is generated from an axis-aligned rectangle by splitting it in half\n"
			"	// Original rectangle can be found by finding the bounding AABB of the triangle\n"
			"	vec4 aabb = vec4(min(gl_in[0].gl_Position.x, min(gl_in[1].gl_Position.x, gl_in[2].gl_Position.x)),\n"
			"	                 min(gl_in[0].gl_Position.y, min(gl_in[1].gl_Position.y, gl_in[2].gl_Position.y)),\n"
			"	                 max(gl_in[0].gl_Position.x, max(gl_in[1].gl_Position.x, gl_in[2].gl_Position.x)),\n"
			"	                 max(gl_in[0].gl_Position.y, max(gl_in[1].gl_Position.y, gl_in[2].gl_Position.y)));\n"
			"\n"
			"	// Location in tessellation grid\n"
			"	ivec2 gridPosition = ivec2(min(v_tessellationGridPosition[0], min(v_tessellationGridPosition[1], v_tessellationGridPosition[2])));\n"
			"\n"
			"	// Which triangle of the two that split the grid cell\n"
			"	int numVerticesOnBottomEdge = 0;\n"
			"	for (int ndx = 0; ndx < 3; ++ndx)\n"
			"		if (abs(gl_in[ndx].gl_Position.y - aabb.w) < equalThreshold)\n"
			"			++numVerticesOnBottomEdge;\n"
			"	bool isBottomTriangle = numVerticesOnBottomEdge == 2;\n"
			"\n";

	if (m_flags & FLAG_GEOMETRY_SCATTER_PRIMITIVES)
	{
		// scatter primitives
		buf <<	"	// Draw grid cells\n"
				"	int inputTriangleNdx = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
				"	for (int ndx = 0; ndx < " << numPrimitives << "; ++ndx)\n"
				"	{\n"
				"		ivec2 dstGridSize = ivec2(" << tessLevel << " * " << numPrimitives << ", 2 * " << tessLevel << " * " << numInstances << ");\n"
				"		ivec2 dstGridNdx = ivec2(" << tessLevel << " * ndx + gridPosition.x, " << tessLevel << " * inputTriangleNdx + 2 * gridPosition.y + ndx * 127) % dstGridSize;\n"
				"		vec4 dstArea;\n"
				"		dstArea.x = float(dstGridNdx.x) / float(dstGridSize.x) * 2.0 - 1.0 - gapOffset;\n"
				"		dstArea.y = float(dstGridNdx.y) / float(dstGridSize.y) * 2.0 - 1.0 - gapOffset;\n"
				"		dstArea.z = float(dstGridNdx.x+1) / float(dstGridSize.x) * 2.0 - 1.0 + gapOffset;\n"
				"		dstArea.w = float(dstGridNdx.y+1) / float(dstGridSize.y) * 2.0 - 1.0 + gapOffset;\n"
				"\n"
				"		vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"		vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"		vec4 outputColor = (((dstGridNdx.y + dstGridNdx.x) % 2) == 0) ? (green) : (yellow);\n"
				"\n"
				"		gl_Position = vec4(dstArea.x, dstArea.y, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.x, dstArea.w, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.z, dstArea.y, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.z, dstArea.w, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"		EndPrimitive();\n"
				"	}\n";
	}
	else if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
	{
		// Number of subrectangle instances = num layers
		DE_ASSERT(m_numLayers == numInstances * 2);

		buf <<	"	// Draw grid cells, send each primitive to a separate layer\n"
				"	int baseLayer = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
				"	for (int ndx = 0; ndx < " << numPrimitives << "; ++ndx)\n"
				"	{\n"
				"		ivec2 dstGridSize = ivec2(" << tessLevel << " * " << numPrimitives << ", " << tessLevel << ");\n"
				"		ivec2 dstGridNdx = ivec2((gridPosition.x * " << numPrimitives << " * 7 + ndx)*13, (gridPosition.y * 127 + ndx) * 19) % dstGridSize;\n"
				"		vec4 dstArea;\n"
				"		dstArea.x = float(dstGridNdx.x) / float(dstGridSize.x) * 2.0 - 1.0 - gapOffset;\n"
				"		dstArea.y = float(dstGridNdx.y) / float(dstGridSize.y) * 2.0 - 1.0 - gapOffset;\n"
				"		dstArea.z = float(dstGridNdx.x+1) / float(dstGridSize.x) * 2.0 - 1.0 + gapOffset;\n"
				"		dstArea.w = float(dstGridNdx.y+1) / float(dstGridSize.y) * 2.0 - 1.0 + gapOffset;\n"
				"\n"
				"		vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"		vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"		vec4 outputColor = (((dstGridNdx.y + dstGridNdx.x) % 2) == 0) ? (green) : (yellow);\n"
				"\n"
				"		gl_Position = vec4(dstArea.x, dstArea.y, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.x, dstArea.w, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.z, dstArea.y, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(dstArea.z, dstArea.w, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				"		EmitVertex();\n"
				"		EndPrimitive();\n"
				"	}\n";
	}
	else
	{
		if (m_flags & FLAG_GEOMETRY_SCATTER_INSTANCES)
		{
			buf <<	"	// Scatter slices\n"
					"	int inputTriangleNdx = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
					"	ivec2 srcSliceNdx = ivec2(gridPosition.x, gridPosition.y * " << (numInstances*2) << " + inputTriangleNdx);\n"
					"	ivec2 dstSliceNdx = ivec2(7 * srcSliceNdx.x, 127 * srcSliceNdx.y) % ivec2(" << tessLevel << ", " << tessLevel << " * " << (numInstances*2) << ");\n"
					"\n"
					"	// Draw slice to the dstSlice slot\n"
					"	vec4 outputSliceArea;\n"
					"	outputSliceArea.x = float(dstSliceNdx.x) / float(" << tessLevel << ") * 2.0 - 1.0 - gapOffset;\n"
					"	outputSliceArea.y = float(dstSliceNdx.y) / float(" << (tessLevel * numInstances * 2) << ") * 2.0 - 1.0 - gapOffset;\n"
					"	outputSliceArea.z = float(dstSliceNdx.x+1) / float(" << tessLevel << ") * 2.0 - 1.0 + gapOffset;\n"
					"	outputSliceArea.w = float(dstSliceNdx.y+1) / float(" << (tessLevel * numInstances * 2) << ") * 2.0 - 1.0 + gapOffset;\n";
		}
		else
		{
			buf <<	"	// Fill the input area with slices\n"
					"	// Upper triangle produces slices only to the upper half of the quad and vice-versa\n"
					"	float triangleOffset = (isBottomTriangle) ? ((aabb.w + aabb.y) / 2.0) : (aabb.y);\n"
					"	// Each slice is a invocation\n"
					"	float sliceHeight = (aabb.w - aabb.y) / float(2 * " << numInstances << ");\n"
					"	float invocationOffset = float(gl_InvocationID) * sliceHeight;\n"
					"\n"
					"	vec4 outputSliceArea;\n"
					"	outputSliceArea.x = aabb.x - gapOffset;\n"
					"	outputSliceArea.y = triangleOffset + invocationOffset - gapOffset;\n"
					"	outputSliceArea.z = aabb.z + gapOffset;\n"
					"	outputSliceArea.w = triangleOffset + invocationOffset + sliceHeight + gapOffset;\n";
		}

		buf <<	"\n"
				"	// Draw slice\n"
				"	for (int ndx = 0; ndx < " << ((numPrimitives+2)/2) << "; ++ndx)\n"
				"	{\n"
				"		vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"		vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"		vec4 outputColor = (((gl_InvocationID + ndx) % 2) == 0) ? (green) : (yellow);\n"
				"		float xpos = mix(outputSliceArea.x, outputSliceArea.z, float(ndx) / float(" << (numPrimitives/2) << "));\n"
				"\n"
				"		gl_Position = vec4(xpos, outputSliceArea.y, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"\n"
				"		gl_Position = vec4(xpos, outputSliceArea.w, 0.0, 1.0);\n"
				"		v_color = outputColor;\n"
				"		EmitVertex();\n"
				"	}\n";
	}

	buf <<	"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

class FeedbackRecordVariableSelectionCase : public TestCase
{
public:
						FeedbackRecordVariableSelectionCase		(Context& context, const char* name, const char* description);
						~FeedbackRecordVariableSelectionCase	(void);

private:
	void				init									(void);
	void				deinit									(void);
	IterateResult		iterate									(void);

	std::string			getVertexSource							(void);
	std::string			getFragmentSource						(void);
	std::string			getTessellationControlSource			(void);
	std::string			getTessellationEvaluationSource			(void);
	std::string			getGeometrySource						(void);

	glu::ShaderProgram*	m_program;
	deUint32			m_xfbBuf;
};

FeedbackRecordVariableSelectionCase::FeedbackRecordVariableSelectionCase (Context& context, const char* name, const char* description)
	: TestCase	(context, name, description)
	, m_program	(DE_NULL)
	, m_xfbBuf	(0)
{
}

FeedbackRecordVariableSelectionCase::~FeedbackRecordVariableSelectionCase (void)
{
	deinit();
}

void FeedbackRecordVariableSelectionCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 &&
		(!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		 !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader")))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	m_testCtx.getLog() << tcu::TestLog::Message << "Declaring multiple output variables with the same name in multiple shader stages. Capturing the value of the varying using transform feedback." << tcu::TestLog::EndMessage;

	// gen feedback buffer fit for 1 triangle (4 components)
	{
		static const tcu::Vec4 initialData[3] =
		{
			tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f),
			tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f),
			tcu::Vec4(-1.0f, -1.0f, -1.0f, -1.0f),
		};

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		m_testCtx.getLog() << tcu::TestLog::Message << "Creating buffer for transform feedback. Allocating storage for one triangle. Filling with -1.0" << tcu::TestLog::EndMessage;

		gl.genBuffers(1, &m_xfbBuf);
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_xfbBuf);
		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, (int)(sizeof(tcu::Vec4[3])), initialData, GL_DYNAMIC_READ);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gen xfb buf");
	}

	// gen shader
	m_program = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources()
																	 << glu::VertexSource(getVertexSource())
																	 << glu::FragmentSource(getFragmentSource())
																	 << glu::TessellationControlSource(getTessellationControlSource())
																	 << glu::TessellationEvaluationSource(getTessellationEvaluationSource())
																	 << glu::GeometrySource(getGeometrySource())
																	 << glu::TransformFeedbackMode(GL_INTERLEAVED_ATTRIBS)
																	 << glu::TransformFeedbackVarying("tf_feedback"));
	m_testCtx.getLog() << *m_program;

	if (!m_program->isOk())
		throw tcu::TestError("could not build program");
}

void FeedbackRecordVariableSelectionCase::deinit (void)
{
	delete m_program;
	m_program = DE_NULL;

	if (m_xfbBuf)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_xfbBuf);
		m_xfbBuf = 0;
	}
}

FeedbackRecordVariableSelectionCase::IterateResult FeedbackRecordVariableSelectionCase::iterate (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	const int				posLoc	= gl.getAttribLocation(m_program->getProgram(), "a_position");
	const glu::VertexArray	vao		(m_context.getRenderContext());

	if (posLoc == -1)
		throw tcu::TestError("a_position attribute location was -1");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	m_testCtx.getLog() << tcu::TestLog::Message << "Rendering a patch of size 3." << tcu::TestLog::EndMessage;

	// Render and feed back

	gl.viewport(0, 0, 1, 1);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindVertexArray(*vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindVertexArray");

	gl.vertexAttrib4f(posLoc, 0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "vertexAttrib4f");

	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "use program");

	gl.patchParameteri(GL_PATCH_VERTICES, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set patch param");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bind xfb buf");

	gl.beginTransformFeedback(GL_TRIANGLES);
	GLU_EXPECT_NO_ERROR(gl.getError(), "beginTransformFeedback");

	gl.drawArrays(GL_PATCHES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "beginTransformFeedback");

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying the value of tf_feedback using transform feedback, expecting (3.0, 3.0, 3.0, 3.0)." << tcu::TestLog::EndMessage;

	// Read back result (one triangle)
	{
		tcu::Vec4	feedbackValues[3];
		const void* mapPtr				= gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, (int)sizeof(feedbackValues), GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "mapBufferRange");

		if (mapPtr == DE_NULL)
			throw tcu::TestError("mapBufferRange returned null");

		deMemcpy(feedbackValues, mapPtr, sizeof(feedbackValues));

		if (gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER) != GL_TRUE)
			throw tcu::TestError("unmapBuffer did not return TRUE");

		for (int ndx = 0; ndx < 3; ++ndx)
		{
			if (!tcu::boolAll(tcu::lessThan(tcu::abs(feedbackValues[ndx] - tcu::Vec4(3.0f)), tcu::Vec4(0.001f))))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Feedback vertex " << ndx << ": expected (3.0, 3.0, 3.0, 3.0), got " << feedbackValues[ndx] << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got unexpected feedback results");
			}
		}
	}

	return STOP;
}

std::string FeedbackRecordVariableSelectionCase::getVertexSource (void)
{
	std::string source =	"${VERSION_DECL}\n"
							"in highp vec4 a_position;\n"
							"out highp vec4 tf_feedback;\n"
							"void main()\n"
							"{\n"
							"	gl_Position = a_position;\n"
							"	tf_feedback = vec4(1.0, 1.0, 1.0, 1.0);\n"
							"}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

std::string FeedbackRecordVariableSelectionCase::getFragmentSource (void)
{
	return specializeShader(s_whiteOutputFragmentShader, m_context.getRenderContext().getType());
}

std::string FeedbackRecordVariableSelectionCase::getTessellationControlSource (void)
{
	std::string source =	"${VERSION_DECL}\n"
							"${EXTENSION_TESSELATION_SHADER}"
							"layout(vertices=3) out;\n"
							"void main()\n"
							"{\n"
							"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
							"	gl_TessLevelOuter[0] = 1.0;\n"
							"	gl_TessLevelOuter[1] = 1.0;\n"
							"	gl_TessLevelOuter[2] = 1.0;\n"
							"	gl_TessLevelInner[0] = 1.0;\n"
							"}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

std::string FeedbackRecordVariableSelectionCase::getTessellationEvaluationSource (void)
{
	std::string source =	"${VERSION_DECL}\n"
							"${EXTENSION_TESSELATION_SHADER}"
							"layout(triangles) in;\n"
							"out highp vec4 tf_feedback;\n"
							"void main()\n"
							"{\n"
							"	gl_Position = gl_in[0].gl_Position * gl_TessCoord.x + gl_in[1].gl_Position * gl_TessCoord.y + gl_in[2].gl_Position * gl_TessCoord.z;\n"
							"	tf_feedback = vec4(2.0, 2.0, 2.0, 2.0);\n"
							"}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

std::string FeedbackRecordVariableSelectionCase::getGeometrySource(void)
{
	std::string source =	"${VERSION_DECL}\n"
							"${EXTENSION_GEOMETRY_SHADER}"
							"layout (triangles) in;\n"
							"layout (triangle_strip, max_vertices=3) out;\n"
							"out highp vec4 tf_feedback;\n"
							"void main()\n"
							"{\n"
							"	for (int ndx = 0; ndx < 3; ++ndx)\n"
							"	{\n"
							"		gl_Position = gl_in[ndx].gl_Position + vec4(float(ndx), float(ndx)*float(ndx), 0.0, 0.0);\n"
							"		tf_feedback = vec4(3.0, 3.0, 3.0, 3.0);\n"
							"		EmitVertex();\n"
							"	}\n"
							"	EndPrimitive();\n"
							"}\n";

	return specializeShader(source, m_context.getRenderContext().getType());
}

} // anonymous

TessellationGeometryInteractionTests::TessellationGeometryInteractionTests (Context& context)
	: TestCaseGroup(context, "tessellation_geometry_interaction", "Tessellation and geometry shader interaction tests")
{
}

TessellationGeometryInteractionTests::~TessellationGeometryInteractionTests (void)
{
}

void TessellationGeometryInteractionTests::init (void)
{
	tcu::TestCaseGroup* const renderGroup		= new tcu::TestCaseGroup(m_testCtx, "render",		"Various render tests");
	tcu::TestCaseGroup* const feedbackGroup		= new tcu::TestCaseGroup(m_testCtx, "feedback",		"Test transform feedback");
	tcu::TestCaseGroup* const pointSizeGroup	= new tcu::TestCaseGroup(m_testCtx, "point_size",	"Test point size");

	addChild(renderGroup);
	addChild(feedbackGroup);
	addChild(pointSizeGroup);

	// .render
	{
		tcu::TestCaseGroup* const passthroughGroup	= new tcu::TestCaseGroup(m_testCtx, "passthrough",	"Render various types with either passthrough geometry or tessellation shader");
		tcu::TestCaseGroup* const limitGroup		= new tcu::TestCaseGroup(m_testCtx, "limits",		"Render with properties near their limits");
		tcu::TestCaseGroup* const scatterGroup		= new tcu::TestCaseGroup(m_testCtx, "scatter",		"Scatter output primitives");

		renderGroup->addChild(passthroughGroup);
		renderGroup->addChild(limitGroup);
		renderGroup->addChild(scatterGroup);

		// .passthrough
		{
			// tessellate_tris_passthrough_geometry_no_change
			// tessellate_quads_passthrough_geometry_no_change
			// tessellate_isolines_passthrough_geometry_no_change
			passthroughGroup->addChild(new IdentityGeometryShaderCase(m_context, "tessellate_tris_passthrough_geometry_no_change",		"Passthrough geometry shader has no effect", IdentityGeometryShaderCase::CASE_TRIANGLES));
			passthroughGroup->addChild(new IdentityGeometryShaderCase(m_context, "tessellate_quads_passthrough_geometry_no_change",		"Passthrough geometry shader has no effect", IdentityGeometryShaderCase::CASE_QUADS));
			passthroughGroup->addChild(new IdentityGeometryShaderCase(m_context, "tessellate_isolines_passthrough_geometry_no_change",	"Passthrough geometry shader has no effect", IdentityGeometryShaderCase::CASE_ISOLINES));

			// passthrough_tessellation_geometry_shade_triangles_no_change
			// passthrough_tessellation_geometry_shade_lines_no_change
			passthroughGroup->addChild(new IdentityTessellationShaderCase(m_context, "passthrough_tessellation_geometry_shade_triangles_no_change",	"Passthrough tessellation shader has no effect", IdentityTessellationShaderCase::CASE_TRIANGLES));
			passthroughGroup->addChild(new IdentityTessellationShaderCase(m_context, "passthrough_tessellation_geometry_shade_lines_no_change",		"Passthrough tessellation shader has no effect", IdentityTessellationShaderCase::CASE_ISOLINES));
		}

		// .limits
		{
			static const struct LimitCaseDef
			{
				const char*	name;
				const char*	desc;
				int			flags;
			} cases[] =
			{
				// Test single limit
				{
					"output_required_max_tessellation",
					"Minimum maximum tessellation level",
					GridRenderCase::FLAG_TESSELLATION_MAX_SPEC
				},
				{
					"output_implementation_max_tessellation",
					"Maximum tessellation level supported by the implementation",
					GridRenderCase::FLAG_TESSELLATION_MAX_IMPLEMENTATION
				},
				{
					"output_required_max_geometry",
					"Output minimum maximum number of vertices the geometry shader",
					GridRenderCase::FLAG_GEOMETRY_MAX_SPEC
				},
				{
					"output_implementation_max_geometry",
					"Output maximum number of vertices in the geometry shader supported by the implementation",
					GridRenderCase::FLAG_GEOMETRY_MAX_IMPLEMENTATION
				},
				{
					"output_required_max_invocations",
					"Minimum maximum number of geometry shader invocations",
					GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC
				},
				{
					"output_implementation_max_invocations",
					"Maximum number of geometry shader invocations supported by the implementation",
					GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION
				},
			};

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ++ndx)
				limitGroup->addChild(new GridRenderCase(m_context, cases[ndx].name, cases[ndx].desc, cases[ndx].flags));
		}

		// .scatter
		{
			scatterGroup->addChild(new GridRenderCase(m_context,
													  "geometry_scatter_instances",
													  "Each geometry shader instance outputs its primitives far from other instances of the same execution",
													  GridRenderCase::FLAG_GEOMETRY_SCATTER_INSTANCES));
			scatterGroup->addChild(new GridRenderCase(m_context,
													  "geometry_scatter_primitives",
													  "Each geometry shader instance outputs its primitives far from other primitives of the same instance",
													  GridRenderCase::FLAG_GEOMETRY_SCATTER_PRIMITIVES | GridRenderCase::FLAG_GEOMETRY_SEPARATE_PRIMITIVES));
			scatterGroup->addChild(new GridRenderCase(m_context,
													  "geometry_scatter_layers",
													  "Each geometry shader instance outputs its primitives to multiple layers and far from other primitives of the same instance",
													  GridRenderCase::FLAG_GEOMETRY_SCATTER_LAYERS | GridRenderCase::FLAG_GEOMETRY_SEPARATE_PRIMITIVES));
		}
	}

	// .feedback
	{
		static const struct PrimitiveCaseConfig
		{
			const char*											name;
			const char*											description;
			FeedbackPrimitiveTypeCase::TessellationOutputType	tessellationOutput;
			FeedbackPrimitiveTypeCase::TessellationPointMode	tessellationPointMode;
			FeedbackPrimitiveTypeCase::GeometryOutputType		geometryOutputType;
		} caseConfigs[] =
		{
			// tess output triangles -> geo input triangles, output points
			{
				"tessellation_output_triangles_geometry_output_points",
				"Tessellation outputs triangles, geometry outputs points",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_TRIANGLES,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_OFF,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_POINTS
			},

			// tess output quads <-> geo input triangles, output points
			{
				"tessellation_output_quads_geometry_output_points",
				"Tessellation outputs quads, geometry outputs points",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_QUADS,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_OFF,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_POINTS
			},

			// tess output isolines <-> geo input lines, output points
			{
				"tessellation_output_isolines_geometry_output_points",
				"Tessellation outputs isolines, geometry outputs points",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_ISOLINES,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_OFF,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_POINTS
			},

			// tess output triangles, point_mode <-> geo input points, output lines
			{
				"tessellation_output_triangles_point_mode_geometry_output_lines",
				"Tessellation outputs triangles in point mode, geometry outputs lines",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_TRIANGLES,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_ON,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_LINES
			},

			// tess output quads, point_mode <-> geo input points, output lines
			{
				"tessellation_output_quads_point_mode_geometry_output_lines",
				"Tessellation outputs quads in point mode, geometry outputs lines",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_QUADS,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_ON,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_LINES
			},

			// tess output isolines, point_mode <-> geo input points, output triangles
			{
				"tessellation_output_isolines_point_mode_geometry_output_triangles",
				"Tessellation outputs isolines in point mode, geometry outputs triangles",
				FeedbackPrimitiveTypeCase::TESSELLATION_OUT_ISOLINES,
				FeedbackPrimitiveTypeCase::TESSELLATION_POINTMODE_ON,
				FeedbackPrimitiveTypeCase::GEOMETRY_OUTPUT_TRIANGLES
			},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(caseConfigs); ++ndx)
		{
			feedbackGroup->addChild(new FeedbackPrimitiveTypeCase(m_context,
																  caseConfigs[ndx].name,
																  caseConfigs[ndx].description,
																  caseConfigs[ndx].tessellationOutput,
																  caseConfigs[ndx].tessellationPointMode,
																  caseConfigs[ndx].geometryOutputType));
		}

		feedbackGroup->addChild(new FeedbackRecordVariableSelectionCase(m_context, "record_variable_selection", "Record a variable that has been declared as an output variable in multiple shader stages"));
	}

	// .point_size
	{
		static const int caseFlags[] =
		{
			PointSizeCase::FLAG_VERTEX_SET,
												PointSizeCase::FLAG_TESSELLATION_EVALUATION_SET,
																										PointSizeCase::FLAG_GEOMETRY_SET,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_CONTROL_SET,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_EVALUATION_SET,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_DONT_SET,
			PointSizeCase::FLAG_VERTEX_SET	|															PointSizeCase::FLAG_GEOMETRY_SET,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_EVALUATION_SET		|	PointSizeCase::FLAG_GEOMETRY_SET,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_ADD				|	PointSizeCase::FLAG_GEOMETRY_ADD,
			PointSizeCase::FLAG_VERTEX_SET	|	PointSizeCase::FLAG_TESSELLATION_EVALUATION_SET		|	PointSizeCase::FLAG_GEOMETRY_DONT_SET,
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(caseFlags); ++ndx)
		{
			const std::string name = PointSizeCase::genTestCaseName(caseFlags[ndx]);
			const std::string desc = PointSizeCase::genTestCaseDescription(caseFlags[ndx]);

			pointSizeGroup->addChild(new PointSizeCase(m_context, name.c_str(), desc.c_str(), caseFlags[ndx]));
		}
	}
}

} // Functional
} // gles31
} // deqp
