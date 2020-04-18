/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Vulkan framework tests.
 *//*--------------------------------------------------------------------*/

#include "ditVulkanTests.hpp"
#include "ditTestCase.hpp"

#include "vkImageUtil.hpp"

#include "deUniquePtr.hpp"

namespace dit
{

tcu::TestCaseGroup* createVulkanTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	group	(new tcu::TestCaseGroup(testCtx, "vulkan", "Vulkan Framework Tests"));

	group->addChild(new SelfCheckCase(testCtx, "image_util", "ImageUtil self-check tests", vk::imageUtilSelfTest));

	return group.release();
}

} // dit
