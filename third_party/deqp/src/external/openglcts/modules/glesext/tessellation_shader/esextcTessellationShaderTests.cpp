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

#include "esextcTessellationShaderTests.hpp"
#include "esextcTessellationShaderBarrier.hpp"
#include "esextcTessellationShaderErrors.hpp"
#include "esextcTessellationShaderInvariance.hpp"
#include "esextcTessellationShaderIsolines.hpp"
#include "esextcTessellationShaderMaxPatchVertices.hpp"
#include "esextcTessellationShaderPoints.hpp"
#include "esextcTessellationShaderPrimitiveCoverage.hpp"
#include "esextcTessellationShaderProgramInterfaces.hpp"
#include "esextcTessellationShaderProperties.hpp"
#include "esextcTessellationShaderQuads.hpp"
#include "esextcTessellationShaderTCTE.hpp"
#include "esextcTessellationShaderTessellation.hpp"
#include "esextcTessellationShaderTriangles.hpp"
#include "esextcTessellationShaderVertexOrdering.hpp"
#include "esextcTessellationShaderVertexSpacing.hpp"
#include "esextcTessellationShaderWinding.hpp"
#include "esextcTessellationShaderXFB.hpp"

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param glslVersion   GLSL version
 **/
TessellationShaderTests::TessellationShaderTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_shader", "EXT_tessellation_shader tests")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderTests::init(void)
{
	TestCaseGroupBase* vertexGroup = new TestCaseGroupBase(m_context, m_extParams, "vertex", "");
	vertexGroup->addChild(new TessellationShaderVertexOrdering(m_context, m_extParams));
	vertexGroup->addChild(new TessellationShaderVertexSpacing(m_context, m_extParams));
	addChild(vertexGroup);

	TestCaseGroupBase* singleGroup = new TestCaseGroupBase(m_context, m_extParams, "single", "");

	singleGroup->addChild(new TessellationShaderPropertiesDefaultContextWideValues(m_context, m_extParams));

	singleGroup->addChild(new TessellationShadersIsolines(m_context, m_extParams));

	singleGroup->addChild(new TessellationShaderProgramInterfaces(m_context, m_extParams));

	singleGroup->addChild(new TessellationShaderPropertiesProgramObject(m_context, m_extParams));
	singleGroup->addChild(new TessellationShaderXFB(m_context, m_extParams));
	singleGroup->addChild(new TessellationShaderMaxPatchVertices(m_context, m_extParams));
	singleGroup->addChild(new TessellationShaderPrimitiveCoverage(m_context, m_extParams));
	addChild(singleGroup);

	addChild(new TessellationShaderQuadsTests(m_context, m_extParams));
	addChild(new TessellationShaderTCTETests(m_context, m_extParams));
	addChild(new TessellationShaderTessellationTests(m_context, m_extParams));
	addChild(new TessellationShaderTrianglesTests(m_context, m_extParams));
	addChild(new TessellationShaderPointsTests(m_context, m_extParams));
	addChild(new TessellationShaderBarrierTests(m_context, m_extParams));
	addChild(new TessellationShaderErrors(m_context, m_extParams));
	addChild(new TessellationShaderInvarianceTests(m_context, m_extParams));
	addChild(new TesselationShaderWindingTests(m_context, m_extParams));
}

} // namespace glcts
