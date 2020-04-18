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
 * \brief Tessellation Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTessellationLimitsTests.hpp"
#include "vktTessellationCoordinatesTests.hpp"
#include "vktTessellationWindingTests.hpp"
#include "vktTessellationShaderInputOutputTests.hpp"
#include "vktTessellationMiscDrawTests.hpp"
#include "vktTessellationCommonEdgeTests.hpp"
#include "vktTessellationFractionalSpacingTests.hpp"
#include "vktTessellationPrimitiveDiscardTests.hpp"
#include "vktTessellationInvarianceTests.hpp"
#include "vktTessellationUserDefinedIO.hpp"
#include "vktTessellationGeometryPassthroughTests.hpp"
#include "vktTessellationGeometryPointSizeTests.hpp"
#include "vktTessellationGeometryGridRenderTests.hpp"

#include "deUniquePtr.hpp"

namespace vkt
{
namespace tessellation
{
namespace
{

tcu::TestCaseGroup* createGeometryInteractionTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "geometry_interaction", "Tessellation and geometry shader interaction tests"));

	group->addChild(createGeometryPassthroughTests		(testCtx));
	group->addChild(createGeometryGridRenderLimitsTests	(testCtx));
	group->addChild(createGeometryGridRenderScatterTests(testCtx));
	group->addChild(createGeometryPointSizeTests		(testCtx));

	return group.release();
}

void createChildren (tcu::TestCaseGroup* tessellationTests)
{
	tcu::TestContext& testCtx = tessellationTests->getTestContext();

	tessellationTests->addChild(createLimitsTests				(testCtx));
	tessellationTests->addChild(createCoordinatesTests			(testCtx));
	tessellationTests->addChild(createWindingTests				(testCtx));
	tessellationTests->addChild(createShaderInputOutputTests	(testCtx));
	tessellationTests->addChild(createMiscDrawTests				(testCtx));
	tessellationTests->addChild(createCommonEdgeTests			(testCtx));
	tessellationTests->addChild(createFractionalSpacingTests	(testCtx));
	tessellationTests->addChild(createPrimitiveDiscardTests		(testCtx));
	tessellationTests->addChild(createInvarianceTests			(testCtx));
	tessellationTests->addChild(createUserDefinedIOTests		(testCtx));
	tessellationTests->addChild(createGeometryInteractionTests	(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "tessellation", "Tessellation tests", createChildren);
}

} // tessellation
} // vkt
