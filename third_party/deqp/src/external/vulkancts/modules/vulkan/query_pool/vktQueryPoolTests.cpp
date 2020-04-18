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
 * \brief Vulkan Query Pool Tests
 *//*--------------------------------------------------------------------*/

#include "vktQueryPoolTests.hpp"

#include "vktTestGroupUtil.hpp"
#include "vktQueryPoolOcclusionTests.hpp"
#include "vktQueryPoolStatisticsTests.hpp"

namespace vkt
{
namespace QueryPool
{

namespace
{

void createChildren (tcu::TestCaseGroup* queryPoolTests)
{
	tcu::TestContext&	testCtx		= queryPoolTests->getTestContext();

	queryPoolTests->addChild(new QueryPoolOcclusionTests(testCtx));
	queryPoolTests->addChild(new QueryPoolStatisticsTests(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "query_pool", "query pool tests", createChildren);
}

} // QueryPool
} // vkt
