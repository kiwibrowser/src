/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 Google Inc.
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
 * \brief WSI Tests
 *//*--------------------------------------------------------------------*/

#include "vktWsiTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vkWsiUtil.hpp"

#include "vktWsiSurfaceTests.hpp"
#include "vktWsiSwapchainTests.hpp"
#include "vktWsiDisplayTests.hpp"
#include "vktWsiIncrementalPresentTests.hpp"
#include "vktWsiDisplayTimingTests.hpp"
#include "vktWsiSharedPresentableImageTests.hpp"
#include "vktWsiColorSpaceTests.hpp"

namespace vkt
{
namespace wsi
{

namespace
{

void createTypeSpecificTests (tcu::TestCaseGroup* testGroup, vk::wsi::Type wsiType)
{
	addTestGroup(testGroup, "surface",				"VkSurface Tests",				createSurfaceTests,				wsiType);
	addTestGroup(testGroup, "swapchain",			"VkSwapchain Tests",			createSwapchainTests,			wsiType);
	addTestGroup(testGroup, "incremental_present",	"Incremental present tests",	createIncrementalPresentTests,	wsiType);
	addTestGroup(testGroup, "display_timing",		"Display Timing Tests",			createDisplayTimingTests,		wsiType);
	addTestGroup(testGroup, "shared_presentable_image",	"VK_KHR_shared_presentable_image tests",	createSharedPresentableImageTests,	wsiType);
	addTestGroup(testGroup, "colorspace",	        "ColorSpace tests",	            createColorSpaceTests,	        wsiType);
}

void createWsiTests (tcu::TestCaseGroup* apiTests)
{
	for (int typeNdx = 0; typeNdx < vk::wsi::TYPE_LAST; ++typeNdx)
	{
		const vk::wsi::Type	wsiType	= (vk::wsi::Type)typeNdx;

		addTestGroup(apiTests, getName(wsiType), "", createTypeSpecificTests, wsiType);
	}

	addTestGroup(apiTests, "display", "Display coverage tests", createDisplayCoverageTests);
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "wsi", "WSI Tests", createWsiTests);
}

} // wsi
} // vkt
