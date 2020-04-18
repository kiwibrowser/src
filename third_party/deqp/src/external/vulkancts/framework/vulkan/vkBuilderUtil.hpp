#ifndef _VKBUILDERUTIL_HPP
#define _VKBUILDERUTIL_HPP
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

#include "vkDefs.hpp"
#include "vkRef.hpp"

#include <vector>

namespace vk
{

class DescriptorSetLayoutBuilder
{
public:
												DescriptorSetLayoutBuilder		(void);

	DescriptorSetLayoutBuilder&					addBinding						(VkDescriptorType	descriptorType,
																				 deUint32			descriptorCount,
																				 VkShaderStageFlags	stageFlags,
																				 const VkSampler*	pImmutableSamplers);

	DescriptorSetLayoutBuilder&					addIndexedBinding				(VkDescriptorType	descriptorType,
																				 deUint32			descriptorCount,
																				 VkShaderStageFlags	stageFlags,
																				 deUint32			dstBinding,
																				 const VkSampler*	pImmutableSamplers);

	Move<VkDescriptorSetLayout>					build							(const DeviceInterface& vk, VkDevice device, VkDescriptorSetLayoutCreateFlags extraFlags = 0) const;

	// helpers

	inline DescriptorSetLayoutBuilder&			addSingleBinding				(VkDescriptorType	descriptorType,
																				 VkShaderStageFlags	stageFlags)
	{
		return addBinding(descriptorType, 1u, stageFlags, (VkSampler*)DE_NULL);
	}
	inline DescriptorSetLayoutBuilder&			addSingleIndexedBinding			(VkDescriptorType	descriptorType,
																				 VkShaderStageFlags	stageFlags,
																				 deUint32			dstBinding)
	{
		return addIndexedBinding(descriptorType, 1u, stageFlags, dstBinding, (VkSampler*)DE_NULL);
	}
	inline DescriptorSetLayoutBuilder&			addArrayBinding					(VkDescriptorType	descriptorType,
																				 deUint32			descriptorCount,
																				 VkShaderStageFlags	stageFlags)
	{
		return addBinding(descriptorType, descriptorCount, stageFlags, (VkSampler*)DE_NULL);
	}
	inline DescriptorSetLayoutBuilder&			addSingleSamplerBinding			(VkDescriptorType	descriptorType,
																				 VkShaderStageFlags	stageFlags,
																				 const VkSampler*	immutableSampler)	//!< \note: Using pointer to sampler to clarify that handle is not
																														//!<        copied and argument lifetime is expected to cover build()
																														//!<        call.
	{
		return addBinding(descriptorType, 1u, stageFlags, immutableSampler);
	}
	inline DescriptorSetLayoutBuilder&			addSingleIndexedSamplerBinding	(VkDescriptorType	descriptorType,
																				 VkShaderStageFlags	stageFlags,
																				 deUint32			dstBinding,
																				 const VkSampler*	immutableSampler)	//!< \note: Using pointer to sampler to clarify that handle is not
																														//!<        copied and argument lifetime is expected to cover build()
																														//!<        call.
	{
		return addIndexedBinding(descriptorType, 1u, stageFlags, dstBinding, immutableSampler);
	}
	inline DescriptorSetLayoutBuilder&			addArraySamplerBinding			(VkDescriptorType	descriptorType,
																				 deUint32			descriptorCount,
																				 VkShaderStageFlags	stageFlags,
																				 const VkSampler*	pImmutableSamplers)
	{
		return addBinding(descriptorType, descriptorCount, stageFlags, pImmutableSamplers);
	}

private:
												DescriptorSetLayoutBuilder		(const DescriptorSetLayoutBuilder&); // delete
	DescriptorSetLayoutBuilder&					operator=						(const DescriptorSetLayoutBuilder&); // delete

	std::vector<VkDescriptorSetLayoutBinding>	m_bindings;

	struct ImmutableSamplerInfo
	{
		deUint32 bindingIndex;
		deUint32 samplerBaseIndex;
	};

	std::vector<ImmutableSamplerInfo>			m_immutableSamplerInfos;
	std::vector<VkSampler>						m_immutableSamplers;
};

class DescriptorPoolBuilder
{
public:
										DescriptorPoolBuilder	(void);

	DescriptorPoolBuilder&				addType					(VkDescriptorType type, deUint32 numDescriptors = 1u);
	Move<VkDescriptorPool>				build					(const DeviceInterface& vk, VkDevice device, VkDescriptorPoolCreateFlags flags, deUint32 maxSets) const;

private:
										DescriptorPoolBuilder	(const DescriptorPoolBuilder&); // delete
	DescriptorPoolBuilder&				operator=				(const DescriptorPoolBuilder&); // delete

	std::vector<VkDescriptorPoolSize>	m_counts;
};

class DescriptorSetUpdateBuilder
{
public:
	class Location
	{
	public:
		static inline Location	binding				(deUint32 binding_)
		{
			return Location(binding_, 0u);
		}
		static inline Location	bindingArrayElement	(deUint32 binding_, deUint32 arrayElement)
		{
			return Location(binding_, arrayElement);
		}

	private:
		// \note private to force use of factory methods that have more descriptive names
		inline					Location			(deUint32 binding_, deUint32 arrayElement)
			: m_binding			(binding_)
			, m_arrayElement	(arrayElement)
		{
		}

		friend class DescriptorSetUpdateBuilder;

		const deUint32			m_binding;
		const deUint32			m_arrayElement;
	};

										DescriptorSetUpdateBuilder	(void);
										/* DescriptorSetUpdateBuilder	(const DescriptorSetUpdateBuilder&); // do not delete */

	DescriptorSetUpdateBuilder&			write						(VkDescriptorSet				destSet,
																	 deUint32						destBinding,
																	 deUint32						destArrayElement,
																	 deUint32						count,
																	 VkDescriptorType				descriptorType,
																	 const VkDescriptorImageInfo*	pImageInfo,
																	 const VkDescriptorBufferInfo*	pBufferInfo,
																	 const VkBufferView*			pTexelBufferView);

	DescriptorSetUpdateBuilder&			copy						(VkDescriptorSet	srcSet,
																	 deUint32			srcBinding,
																	 deUint32			srcArrayElement,
																	 VkDescriptorSet	destSet,
																	 deUint32			destBinding,
																	 deUint32			destArrayElement,
																	 deUint32			count);

	void								update						(const DeviceInterface& vk, VkDevice device) const;
	void								updateWithPush				(const DeviceInterface& vk, VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, deUint32 setIdx, deUint32 descriptorIdx = 0, deUint32 numDescriptors = 0) const;
	void								clear						(void);

	// helpers

	inline DescriptorSetUpdateBuilder&	writeSingle					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 const VkDescriptorImageInfo*	pImageInfo)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, 1u, descriptorType, pImageInfo, DE_NULL, DE_NULL);
	}

	inline DescriptorSetUpdateBuilder&	writeSingle					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 const VkDescriptorBufferInfo*	pBufferInfo)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, 1u, descriptorType, DE_NULL, pBufferInfo, DE_NULL);
	}

	inline DescriptorSetUpdateBuilder&	writeSingle					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 const VkBufferView*			pTexelBufferView)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, 1u, descriptorType, DE_NULL, DE_NULL, pTexelBufferView);
	}

	inline DescriptorSetUpdateBuilder&	writeArray					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 deUint32						numDescriptors,
																	 const VkDescriptorImageInfo*	pImageInfo)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, numDescriptors, descriptorType, pImageInfo, DE_NULL, DE_NULL);
	}

	inline DescriptorSetUpdateBuilder&	writeArray					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 deUint32						numDescriptors,
																	 const VkDescriptorBufferInfo*	pBufferInfo)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, numDescriptors, descriptorType, DE_NULL, pBufferInfo, DE_NULL);
	}

	inline DescriptorSetUpdateBuilder&	writeArray					(VkDescriptorSet				destSet,
																	 const Location&				destLocation,
																	 VkDescriptorType				descriptorType,
																	 deUint32						numDescriptors,
																	 const VkBufferView*			pTexelBufferView)
	{
		return write(destSet, destLocation.m_binding, destLocation.m_arrayElement, numDescriptors, descriptorType, DE_NULL, DE_NULL, pTexelBufferView);
	}

	inline DescriptorSetUpdateBuilder&	copySingle					(VkDescriptorSet	srcSet,
																	 const Location&	srcLocation,
																	 VkDescriptorSet	destSet,
																	 const Location&	destLocation)
	{
		return copy(srcSet, srcLocation.m_binding, srcLocation.m_arrayElement, destSet, destLocation.m_binding, destLocation.m_arrayElement, 1u);
	}

	inline DescriptorSetUpdateBuilder&	copyArray					(VkDescriptorSet	srcSet,
																	 const Location&	srcLocation,
																	 VkDescriptorSet	destSet,
																	 const Location&	destLocation,
																	 deUint32			count)
	{
		return copy(srcSet, srcLocation.m_binding, srcLocation.m_arrayElement, destSet, destLocation.m_binding, destLocation.m_arrayElement, count);
	}

private:
	DescriptorSetUpdateBuilder&			operator=					(const DescriptorSetUpdateBuilder&); // delete

	struct WriteDescriptorInfo
	{
		std::vector<VkDescriptorImageInfo>	imageInfos;
		std::vector<VkDescriptorBufferInfo>	bufferInfos;
		std::vector<VkBufferView>			texelBufferViews;
	};

	std::vector<WriteDescriptorInfo>	m_writeDescriptorInfos;

	std::vector<VkWriteDescriptorSet>	m_writes;
	std::vector<VkCopyDescriptorSet>	m_copies;
};

} // vk

#endif // _VKBUILDERUTIL_HPP
