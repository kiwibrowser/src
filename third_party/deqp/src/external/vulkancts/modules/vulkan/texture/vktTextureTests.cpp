/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Texture tests.
 *//*--------------------------------------------------------------------*/

#include "vktTextureTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTextureFilteringTests.hpp"
#include "vktTextureMipmapTests.hpp"
#include "vktTextureFilteringExplicitLodTests.hpp"
#include "vktTextureShadowTests.hpp"
#include "vktTextureFilteringAnisotropyTests.hpp"
#include "vktTextureCompressedFormatTests.hpp"

namespace vkt
{
namespace texture
{
namespace
{

void createTextureTests (tcu::TestCaseGroup* textureTests)
{
	tcu::TestContext&	testCtx	= textureTests->getTestContext();

	textureTests->addChild(createTextureFilteringTests			(testCtx));
	textureTests->addChild(createTextureMipmappingTests			(testCtx));
	textureTests->addChild(createExplicitLodTests				(testCtx));
	textureTests->addChild(createTextureShadowTests				(testCtx));
	textureTests->addChild(createFilteringAnisotropyTests		(testCtx));
	textureTests->addChild(createTextureCompressedFormatTests	(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "texture", "Texture Tests", createTextureTests);
}

} // texture
} // vkt
