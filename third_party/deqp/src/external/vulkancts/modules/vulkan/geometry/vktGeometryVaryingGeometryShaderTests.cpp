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
 * \brief Varying Geometry Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryVaryingGeometryShaderTests.hpp"
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
using tcu::TestStatus;
using tcu::TestContext;
using tcu::TestCaseGroup;
using std::string;
using de::MovePtr;

typedef enum VertexOutputs		{VERTEXT_NO_OP = -1,VERTEXT_ZERO, VERTEXT_ONE}	VertexOut;
typedef enum GeometryOutputs	{GEOMETRY_ZERO, GEOMETRY_ONE, GEOMETRY_TWO}		GeometryOut;

struct VaryingTestSpec
{
	VertexOutputs	vertexOutputs;
	GeometryOutputs	geometryOutputs;
	const string	name;
	const string	desc;
};

class GeometryVaryingTestInstance : public GeometryExpanderRenderTestInstance
{
public:
			GeometryVaryingTestInstance	(Context&		context,
										 const char*	name);

	void	genVertexAttribData			(void);
};

GeometryVaryingTestInstance::GeometryVaryingTestInstance (Context& context, const char* name)
	: GeometryExpanderRenderTestInstance	(context, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, name)
{
	genVertexAttribData();
}

void GeometryVaryingTestInstance::genVertexAttribData (void)
{
	m_numDrawVertices = 3;
	m_vertexPosData.resize(m_numDrawVertices);
	m_vertexPosData[0] = tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f);
	m_vertexPosData[1] = tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f);
	m_vertexPosData[2] = tcu::Vec4(0.1f, 0.0f, 0.0f, 1.0f);

	m_vertexAttrData.resize(m_numDrawVertices);
	m_vertexAttrData[0] = tcu::Vec4(0.7f, 0.4f, 0.6f, 1.0f);
	m_vertexAttrData[1] = tcu::Vec4(0.9f, 0.2f, 0.5f, 1.0f);
	m_vertexAttrData[2] = tcu::Vec4(0.1f, 0.8f, 0.3f, 1.0f);
}

class VaryingTest : public TestCase
{
public:
							VaryingTest		(TestContext&				testCtx,
											 const VaryingTestSpec&		varyingTestSpec);

	void					initPrograms	(SourceCollections&			sourceCollections) const;
	virtual TestInstance*	createInstance	(Context&					context) const;

protected:
	const VaryingTestSpec	m_varyingTestSpec;
};

VaryingTest::VaryingTest (TestContext& testCtx, const VaryingTestSpec& varyingTestSpec)
	: TestCase			(testCtx, varyingTestSpec.name, varyingTestSpec.desc)
	, m_varyingTestSpec	(varyingTestSpec)

{

}

void VaryingTest::initPrograms (SourceCollections& sourceCollections) const
{
	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) in highp vec4 a_position;\n"
			<<"layout(location = 1) in highp vec4 a_color;\n";
		switch(m_varyingTestSpec.vertexOutputs)
		{
			case VERTEXT_NO_OP:
				src	<< "void main (void)\n"
					<< "{\n"
					<< "}\n";
				break;
			case VERTEXT_ZERO:
				src	<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "}\n";
				break;
			case VERTEXT_ONE:
				src	<<"layout(location = 0) out highp vec4 v_geom_0;\n"
					<< "void main (void)\n"
					<< "{\n"
					<< "	gl_Position = a_position;\n"
					<< "	v_geom_0 = a_color;\n"
					<< "}\n";
				break;
			default:
				DE_ASSERT(0);
		}
		sourceCollections.glslSources.add("vertex") << glu::VertexSource(src.str());
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "layout(triangles) in;\n"
			<< "layout(triangle_strip, max_vertices = 3) out;\n";

		if (m_varyingTestSpec.vertexOutputs == VERTEXT_ONE)
			src	<< "layout(location = 0) in highp vec4 v_geom_0[];\n";

		if (m_varyingTestSpec.geometryOutputs >= GEOMETRY_ONE)
			src	<< "layout(location = 0) out highp vec4 v_frag_0;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<< "layout(location = 1) out highp vec4 v_frag_1;\n";

		src	<< "void main (void)\n"
			<< "{\n"
			<< "	highp vec4 offset = vec4(-0.2, -0.2, 0.0, 0.0);\n"
			<< "	highp vec4 inputColor;\n"
			<< "\n";
		if (m_varyingTestSpec.vertexOutputs == VERTEXT_ONE)
			src	<< "	inputColor = v_geom_0[0];\n";
		else
			src	<< "	inputColor = vec4(1.0, 0.0, 0.0, 1.0);\n";

		if (m_varyingTestSpec.vertexOutputs == VERTEXT_NO_OP)
			src	<< "	gl_Position = vec4(0.0, 0.0, 0.0, 1.0) + offset;\n";
		if (m_varyingTestSpec.vertexOutputs >= VERTEXT_ZERO)
			src	<< "	gl_Position = gl_in[0].gl_Position + offset;\n";

		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_ONE)
			src	<< "	v_frag_0 = inputColor;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<< "	v_frag_0 = inputColor * 0.5;\n"
				<< "	v_frag_1 = inputColor.yxzw * 0.5;\n";

		src	<< "	EmitVertex();\n"
			<< "\n";
		if (m_varyingTestSpec.vertexOutputs == VERTEXT_ONE)
			src	<< "	inputColor = v_geom_0[1];\n";
		else
			src	<< "	inputColor = vec4(1.0, 0.0, 0.0, 1.0);\n";

		if (m_varyingTestSpec.vertexOutputs == VERTEXT_NO_OP)
				src	<< "	gl_Position = vec4(1.0, 0.0, 0.0, 1.0) + offset;\n";
		if (m_varyingTestSpec.vertexOutputs >= VERTEXT_ZERO)
				src	<< "	gl_Position = gl_in[1].gl_Position + offset;\n";

		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_ONE)
			src	<< "	v_frag_0 = inputColor;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<< "	v_frag_0 = inputColor * 0.5;\n"
				<< "	v_frag_1 = inputColor.yxzw * 0.5;\n";

		src	<< "	EmitVertex();\n"
			<< "\n";

		if (m_varyingTestSpec.vertexOutputs == VERTEXT_ONE)
			src	<< "	inputColor = v_geom_0[2];\n";
		else
			src	<< "	inputColor = vec4(1.0, 0.0, 0.0, 1.0);\n";

		if (m_varyingTestSpec.vertexOutputs == VERTEXT_NO_OP)
				src	<< "	gl_Position = vec4(1.0, 1.0, 0.0, 1.0) + offset;\n";
		if (m_varyingTestSpec.vertexOutputs >= VERTEXT_ZERO)
				src	<< "	gl_Position = gl_in[2].gl_Position + offset;\n";

		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_ONE)
			src	<< "	v_frag_0 = inputColor;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<< "	v_frag_0 = inputColor * 0.5;\n"
				<< "	v_frag_1 = inputColor.yxzw * 0.5;\n";

		src	<< "	EmitVertex();\n"
			<< "\n"
			<< "	EndPrimitive();\n"
			<< "}\n";
		sourceCollections.glslSources.add("geometry") << glu::GeometrySource(src.str());
	}

	{
		std::ostringstream src;
		src	<< "#version 310 es\n"
			<<"layout(location = 0) out highp vec4 fragColor;\n";
		if (m_varyingTestSpec.geometryOutputs >= GEOMETRY_ONE)
			src	<<"layout(location = 0) in highp vec4 v_frag_0;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<<"layout(location = 1) in highp vec4 v_frag_1;\n";

		src	<<"void main (void)\n"
			<<"{\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_ZERO)
			src	<<"fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_ONE)
			src	<<"	fragColor = v_frag_0;\n";
		if (m_varyingTestSpec.geometryOutputs == GEOMETRY_TWO)
			src	<<"	fragColor = v_frag_0 + v_frag_1.yxzw;\n";
		src	<<"}\n";
		sourceCollections.glslSources.add("fragment") << glu::FragmentSource(src.str());
	}
}

TestInstance* VaryingTest::createInstance (Context& context) const
{
	return new GeometryVaryingTestInstance(context, getName());
}

} // anonymous

TestCaseGroup* createVaryingGeometryShaderTests (TestContext& testCtx)
{
	MovePtr<TestCaseGroup> varyingGroup	(new TestCaseGroup(testCtx, "varying",  "Test varyings."));

	// varying
	{
		static const VaryingTestSpec varyingTests[] =
		{
			{ VERTEXT_NO_OP,	GEOMETRY_ONE,	"vertex_no_op_geometry_out_1", "vertex_no_op_geometry_out_1" },
			{ VERTEXT_ZERO,		GEOMETRY_ONE,	"vertex_out_0_geometry_out_1", "vertex_out_0_geometry_out_1" },
			{ VERTEXT_ZERO,		GEOMETRY_TWO,	"vertex_out_0_geometry_out_2", "vertex_out_0_geometry_out_2" },
			{ VERTEXT_ONE,		GEOMETRY_ZERO,	"vertex_out_1_geometry_out_0", "vertex_out_1_geometry_out_0" },
			{ VERTEXT_ONE,		GEOMETRY_TWO,	"vertex_out_1_geometry_out_2", "vertex_out_1_geometry_out_2" },
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(varyingTests); ++ndx)
			varyingGroup->addChild(new VaryingTest(testCtx, varyingTests[ndx]));
	}

	return varyingGroup.release();
}

} // geometry
} // vkt
