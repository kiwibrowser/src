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
 * \brief Tessellation and geometry shader interaction stress tests.
 *//*--------------------------------------------------------------------*/

#include "es31sTessellationGeometryInteractionTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTextureUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
#include "gluPixelTransfer.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include <sstream>

namespace deqp
{
namespace gles31
{
namespace Stress
{
namespace
{

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
	};

						GridRenderCase					(Context& context, const char* name, const char* description, int flags);
						~GridRenderCase					(void);

private:
	void				init							(void);
	void				deinit							(void);
	IterateResult		iterate							(void);

	void				renderTo						(std::vector<tcu::Surface>& dst);
	bool				verifyResultLayer				(int layerNdx, const tcu::Surface& dst);

	const char*			getVertexSource					(void);
	const char*			getFragmentSource				(void);
	std::string			getTessellationControlSource	(int tessLevel);
	std::string			getTessellationEvaluationSource	(int tessLevel);
	std::string			getGeometryShaderSource			(int numPrimitives, int numInstances);

	enum
	{
		RENDER_SIZE = 256
	};

	const int			m_flags;

	glu::ShaderProgram*	m_program;
	int					m_numLayers;
};

GridRenderCase::GridRenderCase (Context& context, const char* name, const char* description, int flags)
	: TestCase		(context, name, description)
	, m_flags		(flags)
	, m_program		(DE_NULL)
	, m_numLayers	(1)
{
	DE_ASSERT(((m_flags & FLAG_TESSELLATION_MAX_SPEC) == 0)			|| ((m_flags & FLAG_TESSELLATION_MAX_IMPLEMENTATION) == 0));
	DE_ASSERT(((m_flags & FLAG_GEOMETRY_MAX_SPEC) == 0)				|| ((m_flags & FLAG_GEOMETRY_MAX_IMPLEMENTATION) == 0));
	DE_ASSERT(((m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC) == 0)	|| ((m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION) == 0));
}

GridRenderCase::~GridRenderCase (void)
{
	deinit();
}

void GridRenderCase::init (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	// Requirements

	if (!m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader") ||
		!m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		throw tcu::NotSupportedError("Test requires GL_EXT_tessellation_shader and GL_EXT_geometry_shader extensions");

	if (m_context.getRenderTarget().getWidth() < RENDER_SIZE ||
		m_context.getRenderTarget().getHeight() < RENDER_SIZE)
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(RENDER_SIZE) + "x" + de::toString<int>(RENDER_SIZE) + " or larger render target.");

	// Log

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing tessellation and geometry shaders that output a large number of primitives.\n"
		<< getDescription()
		<< tcu::TestLog::EndMessage;

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
				const int	numComponentsPerVertex		= 8; // vec4 pos, vec4 color

				// If FLAG_GEOMETRY_SEPARATE_PRIMITIVES is not set, geometry shader fills a rectangle area in slices.
				// Each slice is a triangle strip and is generated by a single shader invocation.
				// One slice with 4 segment ends (nodes) and 3 segments:
				//    .__.__.__.
				//    |\ |\ |\ |
				//    |_\|_\|_\|

				const int	numSliceNodesComponentLimit			= geometryTotalOutputComponents / (2 * numComponentsPerVertex);			// each node 2 vertices
				const int	numSliceNodesOutputLimit			= geometryOutputVertices / 2;											// each node 2 vertices
				const int	numSliceNodes						= de::min(numSliceNodesComponentLimit, numSliceNodesOutputLimit);

				const int	numVerticesPerInvocation			= numSliceNodes * 2;
				const int	numPrimitivesPerInvocation			= (numSliceNodes - 1) * 2;

				const int	geometryVerticesPerPrimitive		= numVerticesPerInvocation * geometryShaderInvocations;
				const int	geometryPrimitivesOutPerPrimitive	= numPrimitivesPerInvocation * geometryShaderInvocations;

				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "Geometry shader:\n"
					<< "\tTotal output vertex count per invocation: " << (numVerticesPerInvocation) << "\n"
					<< "\tTotal output primitive count per invocation: " << (numPrimitivesPerInvocation) << "\n"
					<< "\tNumber of invocations per primitive: " << geometryShaderInvocations << "\n"
					<< "\tTotal output vertex count per input primitive: " << (geometryVerticesPerPrimitive) << "\n"
					<< "\tTotal output primitive count per input primitive: " << (geometryPrimitivesOutPerPrimitive) << "\n"
					<< tcu::TestLog::EndMessage;

				sources	<< glu::GeometrySource(getGeometryShaderSource(numPrimitivesPerInvocation, geometryShaderInvocations));

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

	// clear viewport
	gl.clear(GL_COLOR_BUFFER_BIT);

	// draw
	{
		glw::GLenum glerror;

		gl.drawArrays(GL_PATCHES, 0, 1);

		// allow always OOM
		glerror = gl.getError();
		if (glerror == GL_OUT_OF_MEMORY)
			throw AllowedRenderFailureException("got GL_OUT_OF_MEMORY while drawing");

		GLU_EXPECT_NO_ERROR(glerror, "draw patches");
	}

	// Read layers

	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.front().getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "read pixels");
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

const char* GridRenderCase::getVertexSource (void)
{
	return	"#version 310 es\n"
			"in highp vec4 a_position;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"}\n";
}

const char* GridRenderCase::getFragmentSource (void)
{
	return	"#version 310 es\n"
			"flat in mediump vec4 v_color;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = v_color;\n"
			"}\n";
}

std::string GridRenderCase::getTessellationControlSource (int tessLevel)
{
	std::ostringstream buf;

	buf <<	"#version 310 es\n"
			"#extension GL_EXT_tessellation_shader : require\n"
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

	return buf.str();
}

std::string GridRenderCase::getTessellationEvaluationSource (int tessLevel)
{
	std::ostringstream buf;

	buf <<	"#version 310 es\n"
			"#extension GL_EXT_tessellation_shader : require\n"
			"layout(quads) in;\n"
			"\n"
			"out mediump ivec2 v_tessellationGridPosition;\n"
			"\n"
			"// note: No need to use precise gl_Position since position does not depend on order\n"
			"void main (void)\n"
			"{\n"
			"	// Fill the whole viewport\n"
			"	gl_Position = vec4(gl_TessCoord.x * 2.0 - 1.0, gl_TessCoord.y * 2.0 - 1.0, 0.0, 1.0);\n"
			"	// Calculate position in tessellation grid\n"
			"	v_tessellationGridPosition = ivec2(round(gl_TessCoord.xy * float(" << tessLevel << ")));\n"
			"}\n";

	return buf.str();
}

std::string GridRenderCase::getGeometryShaderSource (int numPrimitives, int numInstances)
{
	std::ostringstream buf;

	buf	<<	"#version 310 es\n"
			"#extension GL_EXT_geometry_shader : require\n"
			"layout(triangles, invocations=" << numInstances << ") in;\n"
			"layout(triangle_strip, max_vertices=" << (numPrimitives + 2) << ") out;\n"
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
			"\n"
			"	// Fill the input area with slices\n"
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
			"	outputSliceArea.w = triangleOffset + invocationOffset + sliceHeight + gapOffset;\n""\n"
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
			"	}\n"
			"}\n";

	return buf.str();
}

} // anonymous

TessellationGeometryInteractionTests::TessellationGeometryInteractionTests (Context& context)
	: TestCaseGroup(context, "tessellation_geometry_interaction", "Tessellation and geometry shader interaction stress tests")
{
}

TessellationGeometryInteractionTests::~TessellationGeometryInteractionTests (void)
{
}

void TessellationGeometryInteractionTests::init (void)
{
	tcu::TestCaseGroup* const multilimitGroup = new tcu::TestCaseGroup(m_testCtx, "render_multiple_limits", "Various render tests");

	addChild(multilimitGroup);

	// .render_multiple_limits
	{
		static const struct LimitCaseDef
		{
			const char*	name;
			const char*	desc;
			int			flags;
		} cases[] =
		{
			// Test multiple limits at the same time

			{
				"output_required_max_tessellation_max_geometry",
				"Minimum maximum tessellation level and geometry shader output vertices",
				GridRenderCase::FLAG_TESSELLATION_MAX_SPEC | GridRenderCase::FLAG_GEOMETRY_MAX_SPEC
			},
			{
				"output_implementation_max_tessellation_max_geometry",
				"Maximum tessellation level and geometry shader output vertices supported by the implementation",
				GridRenderCase::FLAG_TESSELLATION_MAX_IMPLEMENTATION | GridRenderCase::FLAG_GEOMETRY_MAX_IMPLEMENTATION
			},
			{
				"output_required_max_tessellation_max_invocations",
				"Minimum maximum tessellation level and geometry shader invocations",
				GridRenderCase::FLAG_TESSELLATION_MAX_SPEC | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC
			},
			{
				"output_implementation_max_tessellation_max_invocations",
				"Maximum tessellation level and geometry shader invocations supported by the implementation",
				GridRenderCase::FLAG_TESSELLATION_MAX_IMPLEMENTATION | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION
			},
			{
				"output_required_max_geometry_max_invocations",
				"Minimum maximum geometry shader output vertices and invocations",
				GridRenderCase::FLAG_GEOMETRY_MAX_SPEC | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC
			},
			{
				"output_implementation_max_geometry_max_invocations",
				"Maximum geometry shader output vertices and invocations invocations supported by the implementation",
				GridRenderCase::FLAG_GEOMETRY_MAX_IMPLEMENTATION | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION
			},

			// Test all limits simultaneously
			{
				"output_max_required",
				"Output minimum maximum number of vertices",
				GridRenderCase::FLAG_TESSELLATION_MAX_SPEC | GridRenderCase::FLAG_GEOMETRY_MAX_SPEC | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC
			},
			{
				"output_max_implementation",
				"Output maximum number of vertices supported by the implementation",
				GridRenderCase::FLAG_TESSELLATION_MAX_IMPLEMENTATION | GridRenderCase::FLAG_GEOMETRY_MAX_IMPLEMENTATION | GridRenderCase::FLAG_GEOMETRY_INVOCATIONS_MAX_IMPLEMENTATION
			},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ++ndx)
			multilimitGroup->addChild(new GridRenderCase(m_context, cases[ndx].name, cases[ndx].desc, cases[ndx].flags));
	}
}

} // Stress
} // gles31
} // deqp
