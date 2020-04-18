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
 * \brief API Tests
 *//*--------------------------------------------------------------------*/

#include "vktApiTests.hpp"

#include "vktTestGroupUtil.hpp"
#include "vktApiSmokeTests.hpp"
#include "vktApiDeviceInitializationTests.hpp"
#include "vktApiObjectManagementTests.hpp"
#include "vktApiBufferTests.hpp"
#include "vktApiBufferViewCreateTests.hpp"
#include "vktApiBufferViewAccessTests.hpp"
#include "vktApiFeatureInfo.hpp"
#include "vktApiCommandBuffersTests.hpp"
#include "vktApiCopiesAndBlittingTests.hpp"
#include "vktApiImageClearingTests.hpp"
#include "vktApiFillBufferTests.hpp"
#include "vktApiDescriptorPoolTests.hpp"
#include "vktApiNullHandleTests.hpp"
#include "vktApiGranularityTests.hpp"
#include "vktApiGetMemoryCommitment.hpp"
#include "vktApiExternalMemoryTests.hpp"

namespace vkt
{
namespace api
{

namespace
{

void createBufferViewTests (tcu::TestCaseGroup* bufferViewTests)
{
	tcu::TestContext&	testCtx		= bufferViewTests->getTestContext();

	bufferViewTests->addChild(createBufferViewCreateTests	(testCtx));
	bufferViewTests->addChild(createBufferViewAccessTests	(testCtx));
}

void createApiTests (tcu::TestCaseGroup* apiTests)
{
	tcu::TestContext&	testCtx		= apiTests->getTestContext();

	apiTests->addChild(createSmokeTests					(testCtx));
	apiTests->addChild(api::createFeatureInfoTests		(testCtx));
	apiTests->addChild(createDeviceInitializationTests	(testCtx));
	apiTests->addChild(createObjectManagementTests		(testCtx));
	apiTests->addChild(createBufferTests				(testCtx));
	apiTests->addChild(createTestGroup					(testCtx, "buffer_view",	"BufferView tests",		createBufferViewTests));
	apiTests->addChild(createCommandBuffersTests		(testCtx));
	apiTests->addChild(createCopiesAndBlittingTests		(testCtx));
	apiTests->addChild(createImageClearingTests			(testCtx));
	apiTests->addChild(createFillAndUpdateBufferTests	(testCtx));
	apiTests->addChild(createDescriptorPoolTests		(testCtx));
	apiTests->addChild(createNullHandleTests			(testCtx));
	apiTests->addChild(createGranularityQueryTests		(testCtx));
	apiTests->addChild(createMemoryCommitmentTests		(testCtx));
	apiTests->addChild(createExternalMemoryTests		(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "api", "API Tests", createApiTests);
}

} // api
} // vkt
