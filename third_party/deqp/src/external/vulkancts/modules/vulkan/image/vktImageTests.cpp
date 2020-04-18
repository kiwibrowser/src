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
 * \brief Image Tests
 *//*--------------------------------------------------------------------*/

#include "vktImageTests.hpp"
#include "vktImageLoadStoreTests.hpp"
#include "vktImageMultisampleLoadStoreTests.hpp"
#include "vktImageMutableTests.hpp"
#include "vktImageQualifiersTests.hpp"
#include "vktImageSizeTests.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktImageAtomicOperationTests.hpp"
#include "vktImageCompressionTranscodingSupport.hpp"
#include "vktImageTranscodingSupportTests.hpp"

namespace vkt
{
namespace image
{

namespace
{

void createChildren (tcu::TestCaseGroup* imageTests)
{
	tcu::TestContext&	testCtx		= imageTests->getTestContext();

	imageTests->addChild(createImageStoreTests(testCtx));
	imageTests->addChild(createImageLoadStoreTests(testCtx));
	imageTests->addChild(createImageMultisampleLoadStoreTests(testCtx));
	imageTests->addChild(createImageMutableTests(testCtx));
	imageTests->addChild(createImageFormatReinterpretTests(testCtx));
	imageTests->addChild(createImageQualifiersTests(testCtx));
	imageTests->addChild(createImageSizeTests(testCtx));
	imageTests->addChild(createImageAtomicOperationTests(testCtx));
	imageTests->addChild(createImageCompressionTranscodingTests(testCtx));
	imageTests->addChild(createImageTranscodingSupportTests(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "image", "Image tests", createChildren);
}

} // image
} // vkt
