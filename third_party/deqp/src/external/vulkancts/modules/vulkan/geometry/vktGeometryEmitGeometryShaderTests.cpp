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
 * \brief Emit Geometry Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryEmitGeometryShaderTests.hpp"
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
using std::string;
using de::MovePtr;
using tcu::Vec4;
using tcu::TestStatus;
using tcu::TestContext;
using tcu::TestCaseGroup;

typedef enum VertexOutputs		{VERTEXT_NO_OP = -1,VERTEXT_ZERO, VERTEXT_ONE}	VertexOut;
typedef enum GeometryOutputs	{GEOMETRY_ZERO, GEOMETRY_ONE, GEOMETRY_TWO}		GeometryOut;

struct EmitTestSpec
{
	VkPrimitiveTopology	primitiveTopology;
	int					emitCountA;			//!< primitive A emit count
	int					endCountA;			//!< primitive A end count
	int					emitCountB;			//!<
	int					endCountB;			//!<
	string				name;
	string				desc;
};

class GeometryEmitTestInstance : public GeometryExpanderRenderTestInstance
{
public:
			GeometryEmitTestInstance	(Context&		context,
										 const char*	name);
	void	genVertexAttribData			(void);
};

GeometryEmitTestInstance::GeometryEmitTestInstance (Context& context, const char* name)
	: GeometryExpanderRenderTestInstance	(context, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, name)
{
	genVertexAttribData();
}

void GeometryEmitTestInstance::genVertexAttribData (void)
{
	m_numDrawVertices = 1;
	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexPosData[0] = Vec4(0, 0, 0, 1);

	m_vertexAttrData.resize(m_numDrawVertices);
	m_vertexAttrData[0] = Vec4(1, 1, 1, 1);
}

class EmitTest : public TestCase
{
public:
							EmitTest		(TestContext&				testCtx,
											 const EmitTestSpec&		emitTestSpec);

	void					initPrograms	(SourceCollections&			sourceCollections) const;
	virtual TestInstance*	createInstance	(Context&					context) const;

protected:
	string					shaderGeometry	(bool						pointSize) const;
	const EmitTestSpec	m_emitTestSpec;
};

EmitTest::EmitTest (TestContext& testCtx, const EmitTestSpec& emitTestSpec)
	: TestCase			(testCtx, emitTestSpec.name, emitTestSpec.desc)
	, m_emitTestSpec	(emitTestSpec)

{

}

void EmitTest::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "layout(location = 0) in highp vec4 a_position;\n"
			<< "layout(location = 1) in highp vec4 a_color;\n"
			<< "layout(location = 0) out highp vec4 v_geom_FragColor;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	gl_Position = a_position;\n"
			<< "	v_geom_FragColor = a_color;\n"
			<< "}\n";
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(shaderGeometry(false));
		if(m_emitTestSpec.primitiveTopology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
			sourceCollections.glslSources.add("geometry_pointsize") << glu::GeometrySource(shaderGeometry(true));
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "layout(location = 0) out mediump vec4 fragColor;\n"
			<< "layout(location = 0) in highp vec4 v_frag_FragColor;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	fragColor = v_frag_FragColor;\n"
			<<"}\n";
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* EmitTest::createInstance (Context& context) const
{
	return new GeometryEmitTestInstance(context, getName());
}

string EmitTest::shaderGeometry (bool pointSize) const
{
	std::ostringstream src;
	src	<< "#version 310 es\n"
		<< "#extension GL_EXT_geometry_shader : require\n";
	if (pointSize)
		src	<<"#extension GL_EXT_geometry_point_size : require\n";
	src	<< "layout(points) in;\n"
		<< "layout(" << outputTypeToGLString(m_emitTestSpec.primitiveTopology) << ", max_vertices = " << (m_emitTestSpec.emitCountA + m_emitTestSpec.emitCountB +1) << ") out;\n"
		<< "layout(location = 0) in highp vec4 v_geom_FragColor[];\n"
		<< "layout(location = 0) out highp vec4 v_frag_FragColor;\n"
		<< "void main (void)\n"
		<< "{\n"
		<< "	const highp vec4 position0 = vec4(-0.5,  0.5, 0.0, 0.0);\n"
		<< "	const highp vec4 position1 = vec4( 0.0,  0.1, 0.0, 0.0);\n"
		<< "	const highp vec4 position2 = vec4( 0.5,  0.5, 0.0, 0.0);\n"
		<< "	const highp vec4 position3 = vec4( 0.7, -0.2, 0.0, 0.0);\n"
		<< "	const highp vec4 position4 = vec4( 0.2,  0.2, 0.0, 0.0);\n"
		<< "	const highp vec4 position5 = vec4( 0.4, -0.3, 0.0, 0.0);\n";
	for (int i = 0; i < m_emitTestSpec.emitCountA; ++i)
	{
		if (pointSize)
			src	<< "	gl_PointSize = 1.0;\n";
		src <<	"	gl_Position = gl_in[0].gl_Position + position" << i << ";\n"
				"	gl_PrimitiveID = gl_PrimitiveIDIn;\n"
				"	v_frag_FragColor = v_geom_FragColor[0];\n"
				"	EmitVertex();\n"
				"\n";
	}

	for (int i = 0; i < m_emitTestSpec.endCountA; ++i)
		src << "	EndPrimitive();\n";

	for (int i = 0; i < m_emitTestSpec.emitCountB; ++i)
	{
		if (pointSize)
			src	<< "	gl_PointSize = 1.0;\n";
		src <<	"	gl_Position = gl_in[0].gl_Position + position" << (m_emitTestSpec.emitCountA + i) << ";\n"
				"	gl_PrimitiveID = gl_PrimitiveIDIn;\n"
				"	v_frag_FragColor = v_geom_FragColor[0];\n"
				"	EmitVertex();\n"
				"\n";
	}

	for (int i = 0; i < m_emitTestSpec.endCountB; ++i)
		src << "	EndPrimitive();\n";
	src	<< "}\n";
	return src.str();
}

} // anonymous

TestCaseGroup* createEmitGeometryShaderTests (TestContext& testCtx)
{
	MovePtr<TestCaseGroup> emitGroup	(new TestCaseGroup(testCtx, "emit", "Different emit counts."));

	// emit different amounts
	{
		EmitTestSpec emitTests[] =
		{
			{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		0,	0,	0,	0,	"points"		,	""},
			{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		0,	1,	0,	0,	"points"		,	""},
			{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		1,	1,	0,	0,	"points"		,	""},
			{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		0,	2,	0,	0,	"points"		,	""},
			{ VK_PRIMITIVE_TOPOLOGY_POINT_LIST,		1,	2,	0,	0,	"points"		,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		0,	0,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		0,	1,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		1,	1,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		2,	1,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		0,	2,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		1,	2,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		2,	2,	0,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,		2,	2,	2,	0,	"line_strip"	,	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	0,	0,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	0,	1,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	1,	1,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	2,	1,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	3,	1,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	0,	2,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	1,	2,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	2,	2,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	3,	2,	0,	0,	"triangle_strip",	""},
			{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,	3,	2,	3,	0,	"triangle_strip",	""},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(emitTests); ++ndx)
		{
			emitTests[ndx].name = std::string(emitTests[ndx].name) + "_emit_" + de::toString(emitTests[ndx].emitCountA) + "_end_" + de::toString(emitTests[ndx].endCountA);
			emitTests[ndx].desc = std::string(emitTests[ndx].name) + " output, emit " + de::toString(emitTests[ndx].emitCountA) + " vertices, call EndPrimitive " + de::toString(emitTests[ndx].endCountA) + " times";

			if (emitTests[ndx].emitCountB)
			{
				emitTests[ndx].name += "_emit_" + de::toString(emitTests[ndx].emitCountB) + "_end_" + de::toString(emitTests[ndx].endCountB);
				emitTests[ndx].desc += ", emit " + de::toString(emitTests[ndx].emitCountB) + " vertices, call EndPrimitive " + de::toString(emitTests[ndx].endCountB) + " times";
			}

			emitGroup->addChild(new EmitTest(testCtx, emitTests[ndx]));
		}
	}

	return emitGroup.release();
}

} // geometry
} // vkt
