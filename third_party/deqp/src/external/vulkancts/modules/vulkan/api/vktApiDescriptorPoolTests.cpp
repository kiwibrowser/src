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
 * \brief Descriptor pool tests
 *//*--------------------------------------------------------------------*/

#include "vktApiDescriptorPoolTests.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkDeviceUtil.hpp"

#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include "tcuPlatform.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"
#include "deInt32.h"
#include "deSTLUtil.hpp"

namespace vkt
{
namespace api
{

namespace
{

using namespace std;
using namespace vk;

tcu::TestStatus resetDescriptorPoolTest (Context& context, deUint32 numIterations)
{
	const deUint32				numDescriptorSetsPerIter = 2048;
	const DeviceInterface&		vkd						 = context.getDeviceInterface();
	const VkDevice				device					 = context.getDevice();

	const VkDescriptorPoolSize descriptorPoolSize =
	{
		VK_DESCRIPTOR_TYPE_SAMPLER, // type
		numDescriptorSetsPerIter	// descriptorCount
	};

	// \todo [2016-05-24 collinbaker] Test with flag VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
	const VkDescriptorPoolCreateInfo descriptorPoolInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,	// sType
		NULL,											// pNext
		0,												// flags
		numDescriptorSetsPerIter,						// maxSets
		1,												// poolSizeCount
		&descriptorPoolSize								// pPoolSizes
	};

	{
		const Unique<VkDescriptorPool> descriptorPool(
			createDescriptorPool(vkd, device,
								 &descriptorPoolInfo));

		const VkDescriptorSetLayoutBinding descriptorSetLayoutBinding =
		{
			0,							// binding
			VK_DESCRIPTOR_TYPE_SAMPLER, // descriptorType
			1,							// descriptorCount
			VK_SHADER_STAGE_ALL,		// stageFlags
			NULL						// pImmutableSamplers
		};

		const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,	// sType
			NULL,													// pNext
			0,														// flags
			1,														// bindingCount
			&descriptorSetLayoutBinding								// pBindings
		};

		{
			typedef de::SharedPtr<Unique<VkDescriptorSetLayout> > DescriptorSetLayoutPtr;

			vector<DescriptorSetLayoutPtr> descriptorSetLayouts;
			descriptorSetLayouts.reserve(numDescriptorSetsPerIter);

			for (deUint32 ndx = 0; ndx < numDescriptorSetsPerIter; ++ndx)
			{
				descriptorSetLayouts.push_back(
					DescriptorSetLayoutPtr(
						new Unique<VkDescriptorSetLayout>(
							createDescriptorSetLayout(vkd, device,
													  &descriptorSetLayoutInfo))));
			}

			vector<VkDescriptorSetLayout> descriptorSetLayoutsRaw(numDescriptorSetsPerIter);

			for (deUint32 ndx = 0; ndx < numDescriptorSetsPerIter; ++ndx)
			{
				descriptorSetLayoutsRaw[ndx] = **descriptorSetLayouts[ndx];
			}

			const VkDescriptorSetAllocateInfo descriptorSetInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // sType
				NULL,											// pNext
				*descriptorPool,								// descriptorPool
				numDescriptorSetsPerIter,						// descriptorSetCount
				&descriptorSetLayoutsRaw[0]						// pSetLayouts
			};

			vector<VkDescriptorSet> testSets(numDescriptorSetsPerIter);

			for (deUint32 ndx = 0; ndx < numIterations; ++ndx)
			{
				// The test should crash in this loop at some point if there is a memory leak
				VK_CHECK(vkd.allocateDescriptorSets(device, &descriptorSetInfo, &testSets[0]));
				VK_CHECK(vkd.resetDescriptorPool(device, *descriptorPool, 0));
			}

		}
	}

	// If it didn't crash, pass
	return tcu::TestStatus::pass("Pass");
}

tcu::TestStatus outOfPoolMemoryTest (Context& context)
{
	const DeviceInterface&	vkd							= context.getDeviceInterface();
	const VkDevice			device						= context.getDevice();
	const bool				expectOutOfPoolMemoryError	= de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance1");
	deUint32				numErrorsReturned			= 0;

	const struct FailureCase
	{
		deUint32	poolDescriptorCount;		//!< total number of descriptors (of a given type) in the descriptor pool
		deUint32	poolMaxSets;				//!< max number of descriptor sets that can be allocated from the pool
		deUint32	bindingCount;				//!< number of bindings per descriptor set layout
		deUint32	bindingDescriptorCount;		//!< number of descriptors in a binding (array size) (in all bindings)
		deUint32	descriptorSetCount;			//!< number of descriptor sets to allocate
		string		description;				//!< the log message for this failure condition
	} failureCases[] =
	{
		//	pool			pool		binding		binding		alloc set
		//	descr. count	max sets	count		array size	count
		{	4u,				2u,			1u,			1u,			3u,		"Out of descriptor sets",											},
		{	3u,				4u,			1u,			1u,			4u,		"Out of descriptors (due to the number of sets)",					},
		{	2u,				1u,			3u,			1u,			1u,		"Out of descriptors (due to the number of bindings)",				},
		{	3u,				2u,			1u,			2u,			2u,		"Out of descriptors (due to descriptor array size)",				},
		{	5u,				1u,			2u,			3u,			1u,		"Out of descriptors (due to descriptor array size in all bindings)",},
	};

	context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Creating a descriptor pool with insufficient resources. Descriptor set allocation is likely to fail."
		<< tcu::TestLog::EndMessage;

	for (deUint32 failureCaseNdx = 0u; failureCaseNdx < DE_LENGTH_OF_ARRAY(failureCases); ++failureCaseNdx)
	{
		const FailureCase& params = failureCases[failureCaseNdx];
		context.getTestContext().getLog() << tcu::TestLog::Message << "Checking: " << params.description << tcu::TestLog::EndMessage;

		for (VkDescriptorType	descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
								descriptorType < VK_DESCRIPTOR_TYPE_LAST;
								descriptorType = static_cast<VkDescriptorType>(descriptorType + 1))
		{
			context.getTestContext().getLog() << tcu::TestLog::Message << "- " << getDescriptorTypeName(descriptorType) << tcu::TestLog::EndMessage;

			const VkDescriptorPoolSize					descriptorPoolSize =
			{
				descriptorType,												// type
				params.poolDescriptorCount,									// descriptorCount
			};

			const VkDescriptorPoolCreateInfo			descriptorPoolCreateInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,				// VkStructureType                sType;
				DE_NULL,													// const void*                    pNext;
				(VkDescriptorPoolCreateFlags)0,								// VkDescriptorPoolCreateFlags    flags;
				params.poolMaxSets,											// uint32_t                       maxSets;
				1u,															// uint32_t                       poolSizeCount;
				&descriptorPoolSize,										// const VkDescriptorPoolSize*    pPoolSizes;
			};

			const Unique<VkDescriptorPool>				descriptorPool(createDescriptorPool(vkd, device, &descriptorPoolCreateInfo));

			const VkDescriptorSetLayoutBinding			descriptorSetLayoutBinding =
			{
				0u,															// uint32_t              binding;
				descriptorType,												// VkDescriptorType      descriptorType;
				params.bindingDescriptorCount,								// uint32_t              descriptorCount;
				VK_SHADER_STAGE_ALL,										// VkShaderStageFlags    stageFlags;
				DE_NULL,													// const VkSampler*      pImmutableSamplers;
			};

			vector<VkDescriptorSetLayoutBinding>	descriptorSetLayoutBindings (params.bindingCount, descriptorSetLayoutBinding);

			for (deUint32 binding = 0; binding < deUint32(descriptorSetLayoutBindings.size()); ++binding)
			{
				descriptorSetLayoutBindings[binding].binding = binding;
			}

			const VkDescriptorSetLayoutCreateInfo		descriptorSetLayoutInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,		// VkStructureType                        sType;
				DE_NULL,													// const void*                            pNext;
				(VkDescriptorSetLayoutCreateFlags)0,						// VkDescriptorSetLayoutCreateFlags       flags;
				static_cast<deUint32>(descriptorSetLayoutBindings.size()),	// uint32_t                               bindingCount;
				&descriptorSetLayoutBindings[0],							// const VkDescriptorSetLayoutBinding*    pBindings;
			};

			const Unique<VkDescriptorSetLayout>			descriptorSetLayout	(createDescriptorSetLayout(vkd, device, &descriptorSetLayoutInfo));
			const vector<VkDescriptorSetLayout>			rawSetLayouts		(params.descriptorSetCount, *descriptorSetLayout);
			vector<VkDescriptorSet>						rawDescriptorSets	(params.descriptorSetCount, DE_NULL);

			const VkDescriptorSetAllocateInfo			descriptorSetAllocateInfo =
			{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,				// VkStructureType                 sType;
				DE_NULL,													// const void*                     pNext;
				*descriptorPool,											// VkDescriptorPool                descriptorPool;
				static_cast<deUint32>(rawSetLayouts.size()),				// uint32_t                        descriptorSetCount;
				&rawSetLayouts[0],											// const VkDescriptorSetLayout*    pSetLayouts;
			};

			const VkResult result = vkd.allocateDescriptorSets(device, &descriptorSetAllocateInfo, &rawDescriptorSets[0]);

			if (result != VK_SUCCESS)
			{
				++numErrorsReturned;

				if (expectOutOfPoolMemoryError && result != VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
					return tcu::TestStatus::fail("Expected VK_ERROR_OUT_OF_POOL_MEMORY_KHR but got " + string(getResultName(result)) + " instead");
			}
			else
				context.getTestContext().getLog() << tcu::TestLog::Message << "  Allocation was successful anyway" << tcu::TestLog::EndMessage;
		}
	}

	if (numErrorsReturned == 0u)
		return tcu::TestStatus::pass("Not validated");
	else
		return tcu::TestStatus::pass("Pass");
}

} // anonymous

tcu::TestCaseGroup* createDescriptorPoolTests (tcu::TestContext& testCtx)
{
	const deUint32 numIterationsHigh = 4096;

	de::MovePtr<tcu::TestCaseGroup> descriptorPoolTests(
		new tcu::TestCaseGroup(testCtx, "descriptor_pool", "Descriptor Pool Tests"));

	addFunctionCase(descriptorPoolTests.get(),
					"repeated_reset_short",
					"Test 2 cycles of vkAllocateDescriptorSets and vkResetDescriptorPool (should pass)",
					resetDescriptorPoolTest, 2U);
	addFunctionCase(descriptorPoolTests.get(),
					"repeated_reset_long",
					"Test many cycles of vkAllocateDescriptorSets and vkResetDescriptorPool",
					resetDescriptorPoolTest, numIterationsHigh);
	addFunctionCase(descriptorPoolTests.get(),
					"out_of_pool_memory",
					"Test that when we run out of descriptors a correct error code is returned",
					outOfPoolMemoryTest);

	return descriptorPoolTests.release();
}

} // api
} // vkt
