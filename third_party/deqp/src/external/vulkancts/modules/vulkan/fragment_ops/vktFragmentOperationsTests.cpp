/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
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
 * \brief Fragment operations tests
 *//*--------------------------------------------------------------------*/

#include "vktFragmentOperationsTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktFragmentOperationsScissorTests.hpp"
#include "vktFragmentOperationsEarlyFragmentTests.hpp"

namespace vkt
{
namespace FragmentOperations
{
namespace
{

void addFragmentOperationsTests (tcu::TestCaseGroup* fragmentOperationsTestsGroup)
{
	tcu::TestContext& testCtx = fragmentOperationsTestsGroup->getTestContext();

	fragmentOperationsTestsGroup->addChild(createScissorTests		(testCtx));
	fragmentOperationsTestsGroup->addChild(createEarlyFragmentTests	(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "fragment_operations", "Fragment operations tests", addFragmentOperationsTests);
}

} // FragmentOperations
} // vkt
