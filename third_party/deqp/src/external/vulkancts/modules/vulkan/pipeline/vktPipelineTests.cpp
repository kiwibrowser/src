/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Pipeline Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineTests.hpp"
#include "vktPipelineStencilTests.hpp"
#include "vktPipelineBlendTests.hpp"
#include "vktPipelineDepthTests.hpp"
#include "vktPipelineImageTests.hpp"
#include "vktPipelineInputAssemblyTests.hpp"
#include "vktPipelineSamplerTests.hpp"
#include "vktPipelineImageViewTests.hpp"
#include "vktPipelinePushConstantTests.hpp"
#include "vktPipelineSpecConstantTests.hpp"
#include "vktPipelineMultisampleTests.hpp"
#include "vktPipelineMultisampleInterpolationTests.hpp"
#include "vktPipelineMultisampleShaderBuiltInTests.hpp"
#include "vktPipelineVertexInputTests.hpp"
#include "vktPipelineTimestampTests.hpp"
#include "vktPipelineCacheTests.hpp"
#include "vktPipelineRenderToImageTests.hpp"
#include "vktPipelineFramebufferAttachmentTests.hpp"
#include "vktTestGroupUtil.hpp"

namespace vkt
{
namespace pipeline
{

namespace
{

void createChildren (tcu::TestCaseGroup* pipelineTests)
{
	tcu::TestContext&	testCtx	= pipelineTests->getTestContext();

	pipelineTests->addChild(createStencilTests					(testCtx));
	pipelineTests->addChild(createBlendTests					(testCtx));
	pipelineTests->addChild(createDepthTests					(testCtx));
	pipelineTests->addChild(createImageTests					(testCtx));
	pipelineTests->addChild(createSamplerTests					(testCtx));
	pipelineTests->addChild(createImageViewTests				(testCtx));
	pipelineTests->addChild(createPushConstantTests				(testCtx));
	pipelineTests->addChild(createSpecConstantTests				(testCtx));
	pipelineTests->addChild(createMultisampleTests				(testCtx));
	pipelineTests->addChild(createMultisampleInterpolationTests	(testCtx));
	pipelineTests->addChild(createMultisampleShaderBuiltInTests	(testCtx));
	pipelineTests->addChild(createTestGroup						(testCtx,	"vertex_input", "", createVertexInputTests));
	pipelineTests->addChild(createInputAssemblyTests			(testCtx));
	pipelineTests->addChild(createTimestampTests				(testCtx));
	pipelineTests->addChild(createCacheTests					(testCtx));
	pipelineTests->addChild(createRenderToImageTests			(testCtx));
	pipelineTests->addChild(createFramebufferAttachmentTests	(testCtx));
}

} // anonymous

tcu::TestCaseGroup* createTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "pipeline", "Pipeline Tests", createChildren);
}

} // pipeline
} // vkt
