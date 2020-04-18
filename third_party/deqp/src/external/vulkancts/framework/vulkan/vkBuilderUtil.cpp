/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
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
 * \brief Vulkan object builder utilities.
 *//*--------------------------------------------------------------------*/

#include "vkBuilderUtil.hpp"

#include "vkRefUtil.hpp"

namespace vk
{

// DescriptorSetLayoutBuilder

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder (void)
{
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addBinding (VkDescriptorType	descriptorType,
																	deUint32			descriptorCount,
																	VkShaderStageFlags	stageFlags,
																	const VkSampler*	pImmutableSamplers)
{
	if (pImmutableSamplers)
	{
		const ImmutableSamplerInfo immutableSamplerInfo =
		{
			(deUint32)m_bindings.size(),
			(deUint32)m_immutableSamplers.size()
		};

		m_immutableSamplerInfos.push_back(immutableSamplerInfo);

		for (size_t descriptorNdx = 0; descriptorNdx < descriptorCount; descriptorNdx++)
			m_immutableSamplers.push_back(pImmutableSamplers[descriptorNdx]);
	}

	// pImmutableSamplers will be updated at build time
	const VkDescriptorSetLayoutBinding binding =
	{
		(deUint32)m_bindings.size(),	// binding
		descriptorType,					// descriptorType
		descriptorCount,				// descriptorCount
		stageFlags,						// stageFlags
		DE_NULL,						// pImmutableSamplers
	};
	m_bindings.push_back(binding);
	return *this;
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addIndexedBinding (VkDescriptorType		descriptorType,
																		   deUint32				descriptorCount,
																		   VkShaderStageFlags	stageFlags,
																		   deUint32				dstBinding,
																		   const VkSampler*		pImmutableSamplers)
{
	if (pImmutableSamplers)
	{
		const ImmutableSamplerInfo immutableSamplerInfo =
		{
			(deUint32)dstBinding,
			(deUint32)m_immutableSamplers.size()
		};

		m_immutableSamplerInfos.push_back(immutableSamplerInfo);

		for (size_t descriptorNdx = 0; descriptorNdx < descriptorCount; descriptorNdx++)
			m_immutableSamplers.push_back(pImmutableSamplers[descriptorNdx]);
	}

	// pImmutableSamplers will be updated at build time
	const VkDescriptorSetLayoutBinding binding =
	{
		dstBinding,						// binding
		descriptorType,					// descriptorType
		descriptorCount,				// descriptorCount
		stageFlags,						// stageFlags
		DE_NULL,						// pImmutableSamplers
	};
	m_bindings.push_back(binding);
	return *this;
}

Move<VkDescriptorSetLayout> DescriptorSetLayoutBuilder::build (const DeviceInterface& vk, VkDevice device, VkDescriptorSetLayoutCreateFlags extraFlags) const
{
	// Create new layout bindings with pImmutableSamplers updated
	std::vector<VkDescriptorSetLayoutBinding>	bindings	= m_bindings;

	for (size_t samplerInfoNdx = 0; samplerInfoNdx < m_immutableSamplerInfos.size(); samplerInfoNdx++)
	{
		const ImmutableSamplerInfo&	samplerInfo	= m_immutableSamplerInfos[samplerInfoNdx];
		deUint32					bindingNdx	= 0;

		while (bindings[bindingNdx].binding != samplerInfo.bindingIndex)
		{
			bindingNdx++;

			if (bindingNdx >= (deUint32)bindings.size())
				DE_FATAL("Immutable sampler not found");
		}

		bindings[bindingNdx].pImmutableSamplers = &m_immutableSamplers[samplerInfo.samplerBaseIndex];
	}

	const VkDescriptorSetLayoutCreateInfo		createInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		DE_NULL,
		(VkDescriptorSetLayoutCreateFlags)extraFlags,			// flags
		(deUint32)bindings.size(),								// bindingCount
		(bindings.empty()) ? (DE_NULL) : (&bindings.front()),	// pBinding
	};

	return createDescriptorSetLayout(vk, device, &createInfo);
}

// DescriptorPoolBuilder

DescriptorPoolBuilder::DescriptorPoolBuilder (void)
{
}

DescriptorPoolBuilder& DescriptorPoolBuilder::addType (VkDescriptorType type, deUint32 numDescriptors)
{
	if (numDescriptors == 0u)
	{
		// nothing to do
		return *this;
	}
	else
	{
		for (size_t ndx = 0; ndx < m_counts.size(); ++ndx)
		{
			if (m_counts[ndx].type == type)
			{
				// augment existing requirement
				m_counts[ndx].descriptorCount += numDescriptors;
				return *this;
			}
		}

		{
			// new requirement
			const VkDescriptorPoolSize typeCount =
			{
				type,			// type
				numDescriptors,	// numDescriptors
			};

			m_counts.push_back(typeCount);
			return *this;
		}
	}
}

Move<VkDescriptorPool> DescriptorPoolBuilder::build (const DeviceInterface& vk, VkDevice device, VkDescriptorPoolCreateFlags flags, deUint32 maxSets) const
{
	const VkDescriptorPoolSize* const	typeCountPtr	= (m_counts.empty()) ? (DE_NULL) : (&m_counts[0]);
	const VkDescriptorPoolCreateInfo	createInfo		=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		DE_NULL,
		flags,
		maxSets,
		(deUint32)m_counts.size(),		// poolSizeCount
		typeCountPtr,					// pPoolSizes
	};

	return createDescriptorPool(vk, device, &createInfo);
}

// DescriptorSetUpdateBuilder

DescriptorSetUpdateBuilder::DescriptorSetUpdateBuilder (void)
{
}

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::write (VkDescriptorSet					destSet,
															   deUint32							destBinding,
															   deUint32							destArrayElement,
															   deUint32							count,
															   VkDescriptorType					descriptorType,
															   const VkDescriptorImageInfo*		pImageInfo,
															   const VkDescriptorBufferInfo*	pBufferInfo,
															   const VkBufferView*				pTexelBufferView)
{
	// pImageInfo, pBufferInfo and pTexelBufferView will be updated when calling update()
	const VkWriteDescriptorSet writeParams =
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		DE_NULL,
		destSet,			//!< destSet
		destBinding,		//!< destBinding
		destArrayElement,	//!< destArrayElement
		count,				//!< count
		descriptorType,		//!< descriptorType
		DE_NULL,
		DE_NULL,
		DE_NULL
	};

	m_writes.push_back(writeParams);

	// Store a copy of pImageInfo, pBufferInfo and pTexelBufferView
	WriteDescriptorInfo	writeInfo;

	if (pImageInfo)
		writeInfo.imageInfos.insert(writeInfo.imageInfos.end(), pImageInfo, pImageInfo + count);

	if (pBufferInfo)
		writeInfo.bufferInfos.insert(writeInfo.bufferInfos.end(), pBufferInfo, pBufferInfo + count);

	if (pTexelBufferView)
		writeInfo.texelBufferViews.insert(writeInfo.texelBufferViews.end(), pTexelBufferView, pTexelBufferView + count);

	m_writeDescriptorInfos.push_back(writeInfo);

	return *this;
}

DescriptorSetUpdateBuilder& DescriptorSetUpdateBuilder::copy (VkDescriptorSet	srcSet,
															  deUint32			srcBinding,
															  deUint32			srcArrayElement,
															  VkDescriptorSet	destSet,
															  deUint32			destBinding,
															  deUint32			destArrayElement,
															  deUint32			count)
{
	const VkCopyDescriptorSet copyParams =
	{
		VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
		DE_NULL,
		srcSet,				//!< srcSet
		srcBinding,			//!< srcBinding
		srcArrayElement,	//!< srcArrayElement
		destSet,			//!< destSet
		destBinding,		//!< destBinding
		destArrayElement,	//!< destArrayElement
		count,				//!< count
	};
	m_copies.push_back(copyParams);
	return *this;
}

void DescriptorSetUpdateBuilder::update (const DeviceInterface& vk, VkDevice device) const
{
	// Update VkWriteDescriptorSet structures with stored info
	std::vector<VkWriteDescriptorSet> writes	= m_writes;

	for (size_t writeNdx = 0; writeNdx < m_writes.size(); writeNdx++)
	{
		const WriteDescriptorInfo& writeInfo = m_writeDescriptorInfos[writeNdx];

		if (!writeInfo.imageInfos.empty())
			writes[writeNdx].pImageInfo			= &writeInfo.imageInfos[0];

		if (!writeInfo.bufferInfos.empty())
			writes[writeNdx].pBufferInfo		= &writeInfo.bufferInfos[0];

		if (!writeInfo.texelBufferViews.empty())
			writes[writeNdx].pTexelBufferView	= &writeInfo.texelBufferViews[0];
	}

	const VkWriteDescriptorSet* const	writePtr	= (m_writes.empty()) ? (DE_NULL) : (&writes[0]);
	const VkCopyDescriptorSet* const	copyPtr		= (m_copies.empty()) ? (DE_NULL) : (&m_copies[0]);

	vk.updateDescriptorSets(device, (deUint32)writes.size(), writePtr, (deUint32)m_copies.size(), copyPtr);
}

void DescriptorSetUpdateBuilder::updateWithPush (const DeviceInterface& vk, VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, deUint32 setIdx, deUint32 descriptorIdx, deUint32 numDescriptors) const
{
	// Write all descriptors or just a subset?
	deUint32							count		= (numDescriptors) ? numDescriptors : (deUint32)m_writes.size();

	// Update VkWriteDescriptorSet structures with stored info
	std::vector<VkWriteDescriptorSet>	writes		= m_writes;

	for (size_t writeNdx = 0; writeNdx < m_writes.size(); writeNdx++)
	{
		const WriteDescriptorInfo& writeInfo = m_writeDescriptorInfos[writeNdx];

		if (!writeInfo.imageInfos.empty())
			writes[writeNdx].pImageInfo			= &writeInfo.imageInfos[0];

		if (!writeInfo.bufferInfos.empty())
			writes[writeNdx].pBufferInfo		= &writeInfo.bufferInfos[0];

		if (!writeInfo.texelBufferViews.empty())
			writes[writeNdx].pTexelBufferView	= &writeInfo.texelBufferViews[0];
	}

	const VkWriteDescriptorSet* const	writePtr	= (m_writes.empty()) ? (DE_NULL) : (&writes[descriptorIdx]);

	vk.cmdPushDescriptorSetKHR(cmd, bindPoint, pipelineLayout, setIdx, count, writePtr);
}

void DescriptorSetUpdateBuilder::clear(void)
{
	m_writeDescriptorInfos.clear();
	m_writes.clear();
	m_copies.clear();
}

} // vk
