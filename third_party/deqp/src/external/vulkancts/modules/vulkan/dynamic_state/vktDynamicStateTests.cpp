/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Dynamic State Tests
 *//*--------------------------------------------------------------------*/

#include "vktDynamicStateTests.hpp"

#include "vktDynamicStateVPTests.hpp"
#include "vktDynamicStateRSTests.hpp"
#include "vktDynamicStateCBTests.hpp"
#include "vktDynamicStateDSTests.hpp"
#include "vktDynamicStateGeneralTests.hpp"
#include "vktTestGroupUtil.hpp"

namespace vkt
{
namespace DynamicState
{

namespace
{

void createChildren (tcu::TestCaseGroup* group)
{
	tcu::TestContext&	testCtx		= group->getTestContext();

	group->addChild(new DynamicStateVPTests(testCtx));
	group->addChild(new DynamicStateRSTests(testCtx));
	group->addChild(new DynamicStateCBTests(testCtx));
	group->addChild(new DynamicStateDSTests(testCtx));
	group->addChild(new DynamicStateGeneralTests(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "dynamic_state", "Dynamic State Tests", createChildren);
}

} // DynamicState
} // vkt
