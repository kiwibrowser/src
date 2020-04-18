/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Input Geometry Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryInputGeometryShaderTests.hpp"
#include "vktGeometryBasicClass.hpp"
#include "vktGeometryTestsUtil.hpp"

#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkPrograms.hpp"
#include "vkBuilderUtil.hpp"

#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"

#include <string>

using namespace vk;

namespace vkt
{
namespace geometry
{
namespace
{
using tcu::Vec4;
using tcu::TestStatus;
using tcu::TestContext;
using tcu::TestCaseGroup;
using de::MovePtr;
using std::string;
using std::vector;

class GeometryInputTestInstance : public GeometryExpanderRenderTestInstance
{
public:
			GeometryInputTestInstance	(Context&					context,
										 const VkPrimitiveTopology	primitiveType,
										 const char*				name);

			GeometryInputTestInstance	(Context&					context,
										 const VkPrimitiveTopology	primitiveType,
										 const char*				name,
										 const int					numDrawVertices);

	void	genVertexAttribData			(void);
};

GeometryInputTestInstance::GeometryInputTestInstance	(Context&						context,
														 const VkPrimitiveTopology		primitiveType,
														 const char*					name)
	: GeometryExpanderRenderTestInstance	(context, primitiveType, name)
{
	genVertexAttribData();
}

GeometryInputTestInstance::GeometryInputTestInstance	(Context&					context,
														 const VkPrimitiveTopology	primitiveType,
														 const char*				name,
														 const int					numDrawVertices)
	: GeometryExpanderRenderTestInstance	(context, primitiveType, name)
{
	genVertexAttribData();
	m_numDrawVertices = numDrawVertices;
}

void GeometryInputTestInstance::genVertexAttribData (void)
{
	// Create 1 X 2 grid in triangle strip adjacent - order
	const float	scale	= 0.3f;
	const Vec4	offset	(-0.5f, -0.2f, 0.0f, 1.0f);
	m_numDrawVertices	= 12;

	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexPosData[ 0] = Vec4( 0,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 1] = Vec4(-1, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 2] = Vec4( 0, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 3] = Vec4( 1,  1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 4] = Vec4( 1,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 5] = Vec4( 0, -2, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 6] = Vec4( 1, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 7] = Vec4( 2,  1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 8] = Vec4( 2,  0, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[ 9] = Vec4( 1, -2, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[10] = Vec4( 2, -1, 0.0f, 0.0f) * scale + offset;
	m_vertexPosData[11] = Vec4( 3,  0, 0.0f, 0.0f) * scale + offset;

	// Red and white
	m_vertexAttrData.resize(m_numDrawVertices);
	for (int i = 0; i < m_numDrawVertices; ++i)
		m_vertexAttrData[i] = (i % 2 == 0) ? Vec4(1, 1, 1, 1) : Vec4(1, 0, 0, 1);
}

class GeometryExpanderRenderTest : public TestCase
{
public:
							GeometryExpanderRenderTest	(TestContext&				testCtx,
														 const PrimitiveTestSpec&	inputPrimitives);

	void					initPrograms				(SourceCollections&			sourceCollections) const;
	virtual TestInstance*	createInstance				(Context&					context) const;

protected:
	string					shaderGeometry				(bool						pointSize) const;
	const VkPrimitiveTopology	m_primitiveType;
	const VkPrimitiveTopology	m_outputType;
};

GeometryExpanderRenderTest::GeometryExpanderRenderTest (TestContext& testCtx, const PrimitiveTestSpec& inputPrimitives)
	: TestCase			(testCtx, inputPrimitives.name, inputPrimitives.name)
	, m_primitiveType	(inputPrimitives.primitiveType)
	, m_outputType		(inputPrimitives.outputType)
{

}

void GeometryExpanderRenderTest::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) in highp vec4 a_position;\n"
			<<"layout(location = 1) in highp vec4 a_color;\n"
			<<"layout(location = 0) out highp vec4 v_geom_FragColor;\n"
			<<"void main (void)\n"
			<<"{\n"
			<<"	gl_Position = a_position;\n"
			<<"	v_geom_FragColor = a_color;\n"
			<<"}\n";
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(shaderGeometry(false));
		if (m_outputType == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
			sourceCollections.glslSources.add("geometry_pointsize") << glu::GeometrySource(shaderGeometry(true));
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) out highp vec4 fragColor;\n"
			<<"layout(location = 0) in highp vec4 v_frag_FragColor;\n"
			<<"void main (void)\n"
			<<"{\n"
			<<"	fragColor = v_frag_FragColor;\n"
			<<"}\n";
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* GeometryExpanderRenderTest::createInstance (Context& context) const
{
	return new GeometryInputTestInstance(context, m_primitiveType, getName());
}

string GeometryExpanderRenderTest::shaderGeometry (bool pointSize) const
{
	std::ostringstream src;
	src	<< "#version 310 es\n"
		<< "#extension GL_EXT_geometry_shader : require\n";
	if (pointSize)
		src	<<"#extension GL_EXT_geometry_point_size : require\n";
	src	<< "layout(" << inputTypeToGLString(m_primitiveType) << ") in;\n"
		<< "layout(" << outputTypeToGLString(m_outputType) << ", max_vertices = " << calcOutputVertices(m_primitiveType) << ") out;\n"
		<< "layout(location = 0) in highp vec4 v_geom_FragColor[];\n"
		<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	const highp vec4 offset0 = vec4(-0.07, -0.01, 0.0, 0.0);\n"
		<< "	const highp vec4 offset1 = vec4( 0.03, -0.03, 0.0, 0.0);\n"
		<< "	const highp vec4 offset2 = vec4(-0.01,  0.08, 0.0, 0.0);\n"
		<< "	highp vec4 yoffset = float(gl_PrimitiveIDIn) * vec4(0.02, 0.1, 0.0, 0.0);\n"
		<< "\n"
		<< "	for (highp int ndx = 0; ndx < gl_in.length(); ndx++)\n"
		<< "	{\n";
		if (pointSize)
			src	<< "		gl_PointSize = 1.0;\n";
	src	<< "		gl_Position = gl_in[ndx].gl_Position + offset0 + yoffset;\n"
		<< "		v_frag_FragColor = v_geom_FragColor[ndx];\n"
		<< "		EmitVertex();\n"
		<< "\n";
		if (pointSize)
			src	<< "		gl_PointSize = 1.0;\n";
	src	<< "		gl_Position = gl_in[ndx].gl_Position + offset1 + yoffset;\n"
		<< "		v_frag_FragColor = v_geom_FragColor[ndx];\n"
		<< "		EmitVertex();\n"
		<< "\n";
		if (pointSize)
			src	<< "		gl_PointSize = 1.0;\n";
	src	<< "		gl_Position = gl_in[ndx].gl_Position + offset2 + yoffset;\n"
		<< "		v_frag_FragColor = v_geom_FragColor[ndx];\n"
		<< "		EmitVertex();\n"
		<< "		EndPrimitive();\n"
		<< "	}\n"
		<< "}\n";
	return src.str();
}

class TriangleStripAdjacencyVertexCountTest : public GeometryExpanderRenderTest
{
public:
							TriangleStripAdjacencyVertexCountTest	(TestContext& testCtx, const PrimitiveTestSpec& inputPrimitives, const int numInputVertices);
	virtual TestInstance*	createInstance							(Context& context) const;
private:
	const int	m_numInputVertices;
};

TriangleStripAdjacencyVertexCountTest::TriangleStripAdjacencyVertexCountTest (TestContext& testCtx, const PrimitiveTestSpec& inputPrimitives, const int numInputVertices)
	: GeometryExpanderRenderTest	(testCtx, inputPrimitives)
	, m_numInputVertices			(numInputVertices)
{
}

TestInstance* TriangleStripAdjacencyVertexCountTest::createInstance (Context& context) const
{
	return new GeometryInputTestInstance(context, m_primitiveType, getName(), m_numInputVertices);
}

} // anonymous

TestCaseGroup* createInputGeometryShaderTests (TestContext& testCtx)
{
	MovePtr<TestCaseGroup> inputPrimitiveGroup		(new TestCaseGroup(testCtx, "input", "Different input primitives."));
	MovePtr<TestCaseGroup> basicPrimitiveGroup		(new TestCaseGroup(testCtx, "basic_primitive", "Basic Primitive geometry tests"));
	MovePtr<TestCaseGroup> triStripAdjacencyGroup	(new TestCaseGroup(testCtx, "triangle_strip_adjacency",	"Different triangle_strip_adjacency vertex counts."));
	MovePtr<TestCaseGroup> conversionPrimitiveGroup	(new TestCaseGroup(testCtx, "conversion", "Different input and output primitives."));

	const PrimitiveTestSpec inputPrimitives[] =
	{
		{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,						"points",					VK_PRIMITIVE_TOPOLOGY_POINT_LIST		},
		{ VK_PRIMITIVE_TOPOLOGY_LINE_LIST,						"lines",					VK_PRIMITIVE_TOPOLOGY_LINE_STRIP		},
		{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,						"line_strip",				VK_PRIMITIVE_TOPOLOGY_LINE_STRIP		},
		{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,					"triangles",				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	},
		{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,					"triangle_strip",			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	},
		{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,					"triangle_fan",				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	},
		{ VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,		"lines_adjacency",			VK_PRIMITIVE_TOPOLOGY_LINE_STRIP		},
		{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,		"line_strip_adjacency",		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP		},
		{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,	"triangles_adjacency",		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP	}
	};

		// more basic types
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(inputPrimitives); ++ndx)
			basicPrimitiveGroup->addChild(new GeometryExpanderRenderTest(testCtx, inputPrimitives[ndx]));

		// triangle strip adjacency with different vertex counts
		for (int vertexCount = 0; vertexCount <= 12; ++vertexCount)
		{
			const string name = "vertex_count_" + de::toString(vertexCount);
			const PrimitiveTestSpec primitives = { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, name.c_str(), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP };

			triStripAdjacencyGroup->addChild(new TriangleStripAdjacencyVertexCountTest(testCtx, primitives, vertexCount));
		}

		// different type conversions
		{
			static const PrimitiveTestSpec conversionPrimitives[] =
			{
				{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	"triangles_to_points",	VK_PRIMITIVE_TOPOLOGY_POINT_LIST	},
				{ VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		"lines_to_points",		VK_PRIMITIVE_TOPOLOGY_POINT_LIST	},
				{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		"points_to_lines",		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP	},
				{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,	"triangles_to_lines",	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP	},
				{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		"points_to_triangles",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
				{ VK_PRIMITIVE_TOPOLOGY_LINE_LIST,		"lines_to_triangles",	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP}
			};

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(conversionPrimitives); ++ndx)
				conversionPrimitiveGroup->addChild(new GeometryExpanderRenderTest(testCtx, conversionPrimitives[ndx]));
		}

	inputPrimitiveGroup->addChild(basicPrimitiveGroup.release());
	inputPrimitiveGroup->addChild(triStripAdjacencyGroup.release());
	inputPrimitiveGroup->addChild(conversionPrimitiveGroup.release());
	return inputPrimitiveGroup.release();
}

} // geometry
} // vkt
