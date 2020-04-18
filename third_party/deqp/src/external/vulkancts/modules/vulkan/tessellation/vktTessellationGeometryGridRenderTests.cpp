/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Tessellation Geometry Interaction - Grid render (limits, scatter)
*//*--------------------------------------------------------------------*/

#include "vktTessellationGeometryGridRenderTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuSurface.hpp"
#include "tcuRGBA.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "deUniquePtr.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum Constants
{
	RENDER_SIZE = 256,
};

enum FlagBits
{
	FLAG_TESSELLATION_MAX_SPEC			= 1u << 0,
	FLAG_GEOMETRY_MAX_SPEC				= 1u << 1,
	FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC	= 1u << 2,

	FLAG_GEOMETRY_SCATTER_INSTANCES		= 1u << 3,
	FLAG_GEOMETRY_SCATTER_PRIMITIVES	= 1u << 4,
	FLAG_GEOMETRY_SEPARATE_PRIMITIVES	= 1u << 5, //!< if set, geometry shader outputs separate grid cells and not continuous slices
	FLAG_GEOMETRY_SCATTER_LAYERS		= 1u << 6,
};
typedef deUint32 Flags;

class GridRenderTestCase : public TestCase
{
public:
	void			initPrograms			(vk::SourceCollections& programCollection) const;
	TestInstance*	createInstance			(Context& context) const;

					GridRenderTestCase		(tcu::TestContext& testCtx, const std::string& name, const std::string& description, const Flags flags);

private:
	const Flags		m_flags;
	const int		m_tessGenLevel;
	const int		m_numGeometryInvocations;
	const int		m_numLayers;
	int				m_numGeometryPrimitivesPerInvocation;
};

GridRenderTestCase::GridRenderTestCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const Flags flags)
	: TestCase					(testCtx, name, description)
	, m_flags					(flags)
	, m_tessGenLevel			((m_flags & FLAG_TESSELLATION_MAX_SPEC)			? 64 : 5)
	, m_numGeometryInvocations	((m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC)	? 32 : 4)
	, m_numLayers				((m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)		? 8  : 1)
{
	DE_ASSERT(((flags & (FLAG_GEOMETRY_SCATTER_PRIMITIVES | FLAG_GEOMETRY_SCATTER_LAYERS)) != 0) == ((flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) != 0));

	testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Testing tessellation and geometry shaders that output a large number of primitives.\n"
		<< getDescription()
		<< tcu::TestLog::EndMessage;

	if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
		m_testCtx.getLog() << tcu::TestLog::Message << "Rendering to 2d texture array, numLayers = " << m_numLayers << tcu::TestLog::EndMessage;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Tessellation level: " << m_tessGenLevel << ", mode = quad.\n"
		<< "\tEach input patch produces " << (m_tessGenLevel*m_tessGenLevel) << " (" << (m_tessGenLevel*m_tessGenLevel*2) << " triangles)\n"
		<< tcu::TestLog::EndMessage;

	int geometryOutputComponents	  = 0;
	int geometryOutputVertices		  = 0;
	int geometryTotalOutputComponents = 0;

	if (m_flags & FLAG_GEOMETRY_MAX_SPEC)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Using geometry shader minimum maximum output limits." << tcu::TestLog::EndMessage;

		geometryOutputComponents	  = 64;
		geometryOutputVertices		  = 256;
		geometryTotalOutputComponents = 1024;
	}
	else
	{
		geometryOutputComponents	  = 64;
		geometryOutputVertices		  = 16;
		geometryTotalOutputComponents = 1024;
	}

	if ((m_flags & FLAG_GEOMETRY_MAX_SPEC) || (m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC))
	{
		tcu::MessageBuilder msg(&m_testCtx.getLog());

		msg << "Geometry shader, targeting following limits:\n";

		if (m_flags & FLAG_GEOMETRY_MAX_SPEC)
			msg	<< "\tmaxGeometryOutputComponents = "	   << geometryOutputComponents << "\n"
				<< "\tmaxGeometryOutputVertices = "		   << geometryOutputVertices << "\n"
				<< "\tmaxGeometryTotalOutputComponents = " << geometryTotalOutputComponents << "\n";

		if (m_flags & FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC)
			msg << "\tmaxGeometryShaderInvocations = "	   << m_numGeometryInvocations;

		msg << tcu::TestLog::EndMessage;
	}

	const bool	separatePrimitives				  = (m_flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) != 0;
	const int	numComponentsPerVertex			  = 8; // vec4 pos, vec4 color
	int			numVerticesPerInvocation		  = 0;
	int			geometryVerticesPerPrimitive	  = 0;
	int			geometryPrimitivesOutPerPrimitive = 0;

	if (separatePrimitives)
	{
		const int	numComponentLimit		 = geometryTotalOutputComponents / (4 * numComponentsPerVertex);
		const int	numOutputLimit			 = geometryOutputVertices / 4;

		m_numGeometryPrimitivesPerInvocation = de::min(numComponentLimit, numOutputLimit);
		numVerticesPerInvocation			 = m_numGeometryPrimitivesPerInvocation * 4;
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
		m_numGeometryPrimitivesPerInvocation	= (numSliceNodes - 1) * 2;
	}

	geometryVerticesPerPrimitive	  = numVerticesPerInvocation * m_numGeometryInvocations;
	geometryPrimitivesOutPerPrimitive = m_numGeometryPrimitivesPerInvocation * m_numGeometryInvocations;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Geometry shader:\n"
		<< "\tTotal output vertex count per invocation: "		  << numVerticesPerInvocation << "\n"
		<< "\tTotal output primitive count per invocation: "	  << m_numGeometryPrimitivesPerInvocation << "\n"
		<< "\tNumber of invocations per primitive: "			  << m_numGeometryInvocations << "\n"
		<< "\tTotal output vertex count per input primitive: "	  << geometryVerticesPerPrimitive << "\n"
		<< "\tTotal output primitive count per input primitive: " << geometryPrimitivesOutPerPrimitive << "\n"
		<< tcu::TestLog::EndMessage;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Program:\n"
		<< "\tTotal program output vertices count per input patch: "  << (m_tessGenLevel*m_tessGenLevel*2 * geometryVerticesPerPrimitive) << "\n"
		<< "\tTotal program output primitive count per input patch: " << (m_tessGenLevel*m_tessGenLevel*2 * geometryPrimitivesOutPerPrimitive) << "\n"
		<< tcu::TestLog::EndMessage;
}

void GridRenderTestCase::initPrograms (SourceCollections& programCollection) const
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "layout(location = 0) flat in highp   vec4 v_color;\n"
			<< "layout(location = 0) out     mediump vec4 fragColor;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    fragColor = v_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}

	// Tessellation control
	{
		std::ostringstream src;
		src <<	glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				"#extension GL_EXT_tessellation_shader : require\n"
				"layout(vertices = 1) out;\n"
				"\n"
				"void main (void)\n"
				"{\n"
				"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
				"    gl_TessLevelInner[0] = float(" << m_tessGenLevel << ");\n"
				"    gl_TessLevelInner[1] = float(" << m_tessGenLevel << ");\n"
				"    gl_TessLevelOuter[0] = float(" << m_tessGenLevel << ");\n"
				"    gl_TessLevelOuter[1] = float(" << m_tessGenLevel << ");\n"
				"    gl_TessLevelOuter[2] = float(" << m_tessGenLevel << ");\n"
				"    gl_TessLevelOuter[3] = float(" << m_tessGenLevel << ");\n"
				"}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "layout(quads) in;\n"
			<< "\n"
			<< "layout(location = 0) out mediump ivec2 v_tessellationGridPosition;\n"
			<< "\n"
			<< "// note: No need to use precise gl_Position since position does not depend on order\n"
			<< "void main (void)\n"
			<< "{\n";

		if (m_flags & (FLAG_GEOMETRY_SCATTER_INSTANCES | FLAG_GEOMETRY_SCATTER_PRIMITIVES | FLAG_GEOMETRY_SCATTER_LAYERS))
			src << "    // Cover only a small area in a corner. The area will be expanded in geometry shader to cover whole viewport\n"
				<< "    gl_Position = vec4(gl_TessCoord.x * 0.3 - 1.0, gl_TessCoord.y * 0.3 - 1.0, 0.0, 1.0);\n";
		else
			src << "    // Fill the whole viewport\n"
				<< "    gl_Position = vec4(gl_TessCoord.x * 2.0 - 1.0, gl_TessCoord.y * 2.0 - 1.0, 0.0, 1.0);\n";

		src << "    // Calculate position in tessellation grid\n"
			<< "    v_tessellationGridPosition = ivec2(round(gl_TessCoord.xy * float(" << m_tessGenLevel << ")));\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}

	// Geometry shader
	{
		const int numInvocations = m_numGeometryInvocations;
		const int numPrimitives  = m_numGeometryPrimitivesPerInvocation;

		std::ostringstream src;

		src	<< glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "layout(triangles, invocations = " << numInvocations << ") in;\n"
			<< "layout(triangle_strip, max_vertices = " << ((m_flags & FLAG_GEOMETRY_SEPARATE_PRIMITIVES) ? (4 * numPrimitives) : (numPrimitives + 2)) << ") out;\n"
			<< "\n"
			<< "layout(location = 0) in       mediump ivec2 v_tessellationGridPosition[];\n"
			<< "layout(location = 0) flat out highp   vec4  v_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    const float equalThreshold = 0.001;\n"
			<< "    const float gapOffset = 0.0001; // subdivision performed by the geometry shader might produce gaps. Fill potential gaps by enlarging the output slice a little.\n"
			<< "\n"
			<< "    // Input triangle is generated from an axis-aligned rectangle by splitting it in half\n"
			<< "    // Original rectangle can be found by finding the bounding AABB of the triangle\n"
			<< "    vec4 aabb = vec4(min(gl_in[0].gl_Position.x, min(gl_in[1].gl_Position.x, gl_in[2].gl_Position.x)),\n"
			<< "                     min(gl_in[0].gl_Position.y, min(gl_in[1].gl_Position.y, gl_in[2].gl_Position.y)),\n"
			<< "                     max(gl_in[0].gl_Position.x, max(gl_in[1].gl_Position.x, gl_in[2].gl_Position.x)),\n"
			<< "                     max(gl_in[0].gl_Position.y, max(gl_in[1].gl_Position.y, gl_in[2].gl_Position.y)));\n"
			<< "\n"
			<< "    // Location in tessellation grid\n"
			<< "    ivec2 gridPosition = ivec2(min(v_tessellationGridPosition[0], min(v_tessellationGridPosition[1], v_tessellationGridPosition[2])));\n"
			<< "\n"
			<< "    // Which triangle of the two that split the grid cell\n"
			<< "    int numVerticesOnBottomEdge = 0;\n"
			<< "    for (int ndx = 0; ndx < 3; ++ndx)\n"
			<< "        if (abs(gl_in[ndx].gl_Position.y - aabb.w) < equalThreshold)\n"
			<< "            ++numVerticesOnBottomEdge;\n"
			<< "    bool isBottomTriangle = numVerticesOnBottomEdge == 2;\n"
			<< "\n";

		if (m_flags & FLAG_GEOMETRY_SCATTER_PRIMITIVES)
		{
			// scatter primitives
			src << "    // Draw grid cells\n"
				<< "    int inputTriangleNdx = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
				<< "    for (int ndx = 0; ndx < " << numPrimitives << "; ++ndx)\n"
				<< "    {\n"
				<< "        ivec2 dstGridSize = ivec2(" << m_tessGenLevel << " * " << numPrimitives << ", 2 * " << m_tessGenLevel << " * " << numInvocations << ");\n"
				<< "        ivec2 dstGridNdx = ivec2(" << m_tessGenLevel << " * ndx + gridPosition.x, " << m_tessGenLevel << " * inputTriangleNdx + 2 * gridPosition.y + ndx * 127) % dstGridSize;\n"
				<< "        vec4 dstArea;\n"
				<< "        dstArea.x = float(dstGridNdx.x)   / float(dstGridSize.x) * 2.0 - 1.0 - gapOffset;\n"
				<< "        dstArea.y = float(dstGridNdx.y)   / float(dstGridSize.y) * 2.0 - 1.0 - gapOffset;\n"
				<< "        dstArea.z = float(dstGridNdx.x+1) / float(dstGridSize.x) * 2.0 - 1.0 + gapOffset;\n"
				<< "        dstArea.w = float(dstGridNdx.y+1) / float(dstGridSize.y) * 2.0 - 1.0 + gapOffset;\n"
				<< "\n"
				<< "        vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 outputColor = (((dstGridNdx.y + dstGridNdx.x) % 2) == 0) ? (green) : (yellow);\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.x, dstArea.y, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.x, dstArea.w, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.z, dstArea.y, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.z, dstArea.w, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "        EndPrimitive();\n"
				<< "    }\n";
		}
		else if (m_flags & FLAG_GEOMETRY_SCATTER_LAYERS)
		{
			// Number of subrectangle instances = num layers
			DE_ASSERT(m_numLayers == numInvocations * 2);

			src << "    // Draw grid cells, send each primitive to a separate layer\n"
				<< "    int baseLayer = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
				<< "    for (int ndx = 0; ndx < " << numPrimitives << "; ++ndx)\n"
				<< "    {\n"
				<< "        ivec2 dstGridSize = ivec2(" << m_tessGenLevel << " * " << numPrimitives << ", " << m_tessGenLevel << ");\n"
				<< "        ivec2 dstGridNdx = ivec2((gridPosition.x * " << numPrimitives << " * 7 + ndx)*13, (gridPosition.y * 127 + ndx) * 19) % dstGridSize;\n"
				<< "        vec4 dstArea;\n"
				<< "        dstArea.x = float(dstGridNdx.x) / float(dstGridSize.x) * 2.0 - 1.0 - gapOffset;\n"
				<< "        dstArea.y = float(dstGridNdx.y) / float(dstGridSize.y) * 2.0 - 1.0 - gapOffset;\n"
				<< "        dstArea.z = float(dstGridNdx.x+1) / float(dstGridSize.x) * 2.0 - 1.0 + gapOffset;\n"
				<< "        dstArea.w = float(dstGridNdx.y+1) / float(dstGridSize.y) * 2.0 - 1.0 + gapOffset;\n"
				<< "\n"
				<< "        vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 outputColor = (((dstGridNdx.y + dstGridNdx.x) % 2) == 0) ? (green) : (yellow);\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.x, dstArea.y, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.x, dstArea.w, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.z, dstArea.y, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(dstArea.z, dstArea.w, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        gl_Layer = ((baseLayer + ndx) * 11) % " << m_numLayers << ";\n"
				<< "        EmitVertex();\n"
				<< "        EndPrimitive();\n"
				<< "    }\n";
		}
		else
		{
			if (m_flags & FLAG_GEOMETRY_SCATTER_INSTANCES)
			{
				src << "    // Scatter slices\n"
					<< "    int inputTriangleNdx = gl_InvocationID * 2 + ((isBottomTriangle) ? (1) : (0));\n"
					<< "    ivec2 srcSliceNdx = ivec2(gridPosition.x, gridPosition.y * " << (numInvocations*2) << " + inputTriangleNdx);\n"
					<< "    ivec2 dstSliceNdx = ivec2(7 * srcSliceNdx.x, 127 * srcSliceNdx.y) % ivec2(" << m_tessGenLevel << ", " << m_tessGenLevel << " * " << (numInvocations*2) << ");\n"
					<< "\n"
					<< "    // Draw slice to the dstSlice slot\n"
					<< "    vec4 outputSliceArea;\n"
					<< "    outputSliceArea.x = float(dstSliceNdx.x)   / float(" << m_tessGenLevel << ") * 2.0 - 1.0 - gapOffset;\n"
					<< "    outputSliceArea.y = float(dstSliceNdx.y)   / float(" << (m_tessGenLevel * numInvocations * 2) << ") * 2.0 - 1.0 - gapOffset;\n"
					<< "    outputSliceArea.z = float(dstSliceNdx.x+1) / float(" << m_tessGenLevel << ") * 2.0 - 1.0 + gapOffset;\n"
					<< "    outputSliceArea.w = float(dstSliceNdx.y+1) / float(" << (m_tessGenLevel * numInvocations * 2) << ") * 2.0 - 1.0 + gapOffset;\n";
			}
			else
			{
				src << "    // Fill the input area with slices\n"
					<< "    // Upper triangle produces slices only to the upper half of the quad and vice-versa\n"
					<< "    float triangleOffset = (isBottomTriangle) ? ((aabb.w + aabb.y) / 2.0) : (aabb.y);\n"
					<< "    // Each slice is a invocation\n"
					<< "    float sliceHeight = (aabb.w - aabb.y) / float(2 * " << numInvocations << ");\n"
					<< "    float invocationOffset = float(gl_InvocationID) * sliceHeight;\n"
					<< "\n"
					<< "    vec4 outputSliceArea;\n"
					<< "    outputSliceArea.x = aabb.x - gapOffset;\n"
					<< "    outputSliceArea.y = triangleOffset + invocationOffset - gapOffset;\n"
					<< "    outputSliceArea.z = aabb.z + gapOffset;\n"
					<< "    outputSliceArea.w = triangleOffset + invocationOffset + sliceHeight + gapOffset;\n";
			}

			src << "\n"
				<< "    // Draw slice\n"
				<< "    for (int ndx = 0; ndx < " << ((numPrimitives+2)/2) << "; ++ndx)\n"
				<< "    {\n"
				<< "        vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);\n"
				<< "        vec4 outputColor = (((gl_InvocationID + ndx) % 2) == 0) ? (green) : (yellow);\n"
				<< "        float xpos = mix(outputSliceArea.x, outputSliceArea.z, float(ndx) / float(" << (numPrimitives/2) << "));\n"
				<< "\n"
				<< "        gl_Position = vec4(xpos, outputSliceArea.y, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(xpos, outputSliceArea.w, 0.0, 1.0);\n"
				<< "        v_color = outputColor;\n"
				<< "        EmitVertex();\n"
				<< "    }\n";
		}

		src <<	"}\n";

		programCollection.glslSources.add("geom") << glu::GeometrySource(src.str());
	}
}

class GridRenderTestInstance : public TestInstance
{
public:
	struct Params
	{
		Flags	flags;
		int		numLayers;

		Params (void) : flags(), numLayers() {}
	};
						GridRenderTestInstance	(Context& context, const Params& params) : TestInstance(context), m_params(params) {}
	tcu::TestStatus		iterate					(void);

private:
	Params				m_params;
};

TestInstance* GridRenderTestCase::createInstance (Context& context) const
{
	GridRenderTestInstance::Params params;

	params.flags	 = m_flags;
	params.numLayers = m_numLayers;

	return new GridRenderTestInstance(context, params);
}

bool verifyResultLayer (tcu::TestLog& log, const tcu::ConstPixelBufferAccess& image, const int layerNdx)
{
	tcu::Surface errorMask	(image.getWidth(), image.getHeight());
	bool		 foundError	= false;

	tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

	log << tcu::TestLog::Message << "Verifying output layer " << layerNdx  << tcu::TestLog::EndMessage;

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth(); ++x)
	{
		const int		threshold	= 8;
		const tcu::RGBA	color		(image.getPixel(x, y));

		// Color must be a linear combination of green and yellow
		if (color.getGreen() < 255 - threshold || color.getBlue() > threshold)
		{
			errorMask.setPixel(x, y, tcu::RGBA::red());
			foundError = true;
		}
	}

	if (!foundError)
	{
		log << tcu::TestLog::Message << "Image valid." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("ImageVerification", "Image verification")
			<< tcu::TestLog::Image("Result", "Rendered result", image)
			<< tcu::TestLog::EndImageSet;
		return true;
	}
	else
	{
		log	<< tcu::TestLog::Message << "Image verification failed, found invalid pixels." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("ImageVerification", "Image verification")
			<< tcu::TestLog::Image("Result", "Rendered result", image)
			<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
		return false;
	}
}

tcu::TestStatus GridRenderTestInstance::iterate (void)
{
	requireFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_GEOMETRY_SHADER);

	m_context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Rendering single point at the origin. Expecting yellow and green colored grid-like image. (High-frequency grid may appear unicolored)."
		<< tcu::TestLog::EndMessage;

	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Color attachment

	const tcu::IVec2			  renderSize			   = tcu::IVec2(RENDER_SIZE, RENDER_SIZE);
	const VkFormat				  colorFormat			   = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageAllLayersRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, m_params.numLayers);
	const VkImageCreateInfo		  colorImageCreateInfo	   = makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, m_params.numLayers);
	const VkImageViewType		  colorAttachmentViewType  = (m_params.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY);
	const Image					  colorAttachmentImage	   (vk, device, allocator, colorImageCreateInfo, MemoryRequirement::Any);

	// Color output buffer: image will be copied here for verification (big enough for all layers).

	const VkDeviceSize	colorBufferSizeBytes	= renderSize.x()*renderSize.y() * m_params.numLayers * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer				(vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Pipeline: no vertex input attributes nor descriptors.

	const Unique<VkImageView>		colorAttachmentView(makeImageView						(vk, device, *colorAttachmentImage, colorAttachmentViewType, colorFormat, colorImageAllLayersRange));
	const Unique<VkRenderPass>		renderPass		   (makeRenderPass						(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer		   (makeFramebuffer						(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), m_params.numLayers));
	const Unique<VkPipelineLayout>	pipelineLayout	   (makePipelineLayoutWithoutDescriptors(vk, device));
	const Unique<VkCommandPool>		cmdPool			   (makeCommandPool						(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		   (allocateCommandBuffer				(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline (GraphicsPipelineBuilder()
		.setRenderSize	(renderSize)
		.setShader		(vk, device, VK_SHADER_STAGE_VERTEX_BIT,				  m_context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader		(vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				  m_context.getBinaryCollection().get("frag"), DE_NULL)
		.setShader		(vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	  m_context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader		(vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader		(vk, device, VK_SHADER_STAGE_GEOMETRY_BIT,				  m_context.getBinaryCollection().get("geom"), DE_NULL)
		.build			(vk, device, *pipelineLayout, *renderPass));

	beginCommandBuffer(vk, *cmdBuffer);

	// Change color attachment image layout
	{
		const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
			(VkAccessFlags)0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			*colorAttachmentImage, colorImageAllLayersRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
	}

	// Begin render pass
	{
		const VkRect2D renderArea = {
			makeOffset2D(0, 0),
			makeExtent2D(renderSize.x(), renderSize.y()),
		};
		const tcu::Vec4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

		beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

	vk.cmdDraw(*cmdBuffer, 1u, 1u, 0u, 0u);
	endRenderPass(vk, *cmdBuffer);

	// Copy render result to a host-visible buffer
	{
		const VkImageMemoryBarrier colorAttachmentPreCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*colorAttachmentImage, colorImageAllLayersRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentPreCopyBarrier);
	}
	{
		const VkImageSubresourceLayers subresourceLayers = makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, m_params.numLayers);
		const VkBufferImageCopy		   copyRegion		 = makeBufferImageCopy(makeExtent3D(renderSize.x(), renderSize.y(), 1), subresourceLayers);
		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &copyRegion);
	}
	{
		const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *colorBuffer, 0ull, colorBufferSizeBytes);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Verify results
	{
		const Allocation& alloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), colorBufferSizeBytes);

		const tcu::ConstPixelBufferAccess imageAllLayers(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), m_params.numLayers, alloc.getHostPtr());

		bool allOk = true;
		for (int ndx = 0; ndx < m_params.numLayers; ++ndx)
			allOk = allOk && verifyResultLayer(m_context.getTestContext().getLog(),
											   tcu::getSubregion(imageAllLayers, 0, 0, ndx, renderSize.x(), renderSize.y(), 1),
											   ndx);

		return (allOk ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Image comparison failed"));
	}
}

struct TestCaseDescription
{
	const char*	name;
	const char*	desc;
	Flags		flags;
};

} // anonymous

//! Ported from dEQP-GLES31.functional.tessellation_geometry_interaction.render.limits.*
//! \note Tests that check implementation defined limits were omitted, because they rely on runtime shader source generation
//!       (e.g. changing the number of vertices output from geometry shader). CTS currently doesn't support that,
//!       because some platforms require precompiled shaders.
tcu::TestCaseGroup* createGeometryGridRenderLimitsTests  (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "limits", "Render with properties near their limits"));

	static const TestCaseDescription cases[] =
	{
		{
			"output_required_max_tessellation",
			"Minimum maximum tessellation level",
			FLAG_TESSELLATION_MAX_SPEC
		},
		{
			"output_required_max_geometry",
			"Output minimum maximum number of vertices the geometry shader",
			FLAG_GEOMETRY_MAX_SPEC
		},
		{
			"output_required_max_invocations",
			"Minimum maximum number of geometry shader invocations",
			FLAG_GEOMETRY_INVOCATIONS_MAX_SPEC
		},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ++ndx)
		group->addChild(new GridRenderTestCase(testCtx, cases[ndx].name, cases[ndx].desc, cases[ndx].flags));

	return group.release();
}

//! Ported from dEQP-GLES31.functional.tessellation_geometry_interaction.render.scatter.*
tcu::TestCaseGroup* createGeometryGridRenderScatterTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "scatter", "Scatter output primitives"));

	static const TestCaseDescription cases[] =
	{
		{
			"geometry_scatter_instances",
			"Each geometry shader instance outputs its primitives far from other instances of the same execution",
			FLAG_GEOMETRY_SCATTER_INSTANCES
		},
		{
			"geometry_scatter_primitives",
			"Each geometry shader instance outputs its primitives far from other primitives of the same instance",
			FLAG_GEOMETRY_SCATTER_PRIMITIVES | FLAG_GEOMETRY_SEPARATE_PRIMITIVES
		},
		{
			"geometry_scatter_layers",
			"Each geometry shader instance outputs its primitives to multiple layers and far from other primitives of the same instance",
			FLAG_GEOMETRY_SCATTER_LAYERS | FLAG_GEOMETRY_SEPARATE_PRIMITIVES
		},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ++ndx)
		group->addChild(new GridRenderTestCase(testCtx, cases[ndx].name, cases[ndx].desc, cases[ndx].flags));

	return group.release();
}

} // tessellation
} // vkt
