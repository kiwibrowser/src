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
 * \brief SPIR-V Assembly Tests
 *//*--------------------------------------------------------------------*/

#include "vktSpvAsmTests.hpp"

#include "vktSpvAsmInstructionTests.hpp"
#include "vktTestGroupUtil.hpp"

namespace vkt
{
namespace SpirVAssembly
{

namespace
{

void createChildren (tcu::TestCaseGroup* spirVAssemblyTests)
{
	tcu::TestContext&	testCtx		= spirVAssemblyTests->getTestContext();

	spirVAssemblyTests->addChild(createInstructionTests(testCtx));
	// \todo [2015-09-28 antiagainst] control flow
	// \todo [2015-09-28 antiagainst] multiple entry points for the same shader stage
	// \todo [2015-09-28 antiagainst] multiple shaders in the same module
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "spirv_assembly", "SPIR-V Assembly tests", createChildren);
}

} // SpirVAssembly
} // vkt
