/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Memory Tests
 *//*--------------------------------------------------------------------*/

#include "vktMemoryTests.hpp"

#include "vktMemoryAllocationTests.hpp"
#include "vktMemoryMappingTests.hpp"
#include "vktMemoryPipelineBarrierTests.hpp"
#include "vktMemoryRequirementsTests.hpp"
#include "vktMemoryBindingTests.hpp"
#include "vktTestGroupUtil.hpp"

namespace vkt
{
namespace memory
{

namespace
{

void createChildren (tcu::TestCaseGroup* memoryTests)
{
	tcu::TestContext&	testCtx		= memoryTests->getTestContext();

	memoryTests->addChild(createAllocationTests			(testCtx));
	memoryTests->addChild(createMappingTests			(testCtx));
	memoryTests->addChild(createPipelineBarrierTests	(testCtx));
	memoryTests->addChild(createRequirementsTests		(testCtx));
	memoryTests->addChild(createMemoryBindingTests		(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "memory", "Memory Tests", createChildren);
}

} // memory
} // vkt
