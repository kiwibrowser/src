#ifndef _VKTIMAGELOADSTOREUTIL_HPP
#define _VKTIMAGELOADSTOREUTIL_HPP
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
 * \brief Image load/store utilities
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "vkDefs.hpp"
#include "vkImageUtil.hpp"
#include "vktImageTestsUtil.hpp"
#include "vktImageTexture.hpp"
#include "tcuVector.hpp"
#include "deSharedPtr.hpp"

namespace vkt
{
namespace image
{

typedef de::SharedPtr<vk::Unique<vk::VkDescriptorSet> >	SharedVkDescriptorSet;
typedef de::SharedPtr<vk::Unique<vk::VkImageView> >		SharedVkImageView;

template<typename T>
inline de::SharedPtr<vk::Unique<T> > makeVkSharedPtr (vk::Move<T> vkMove)
{
	return de::SharedPtr<vk::Unique<T> >(new vk::Unique<T>(vkMove));
}

inline float computeStoreColorBias (const vk::VkFormat format)
{
	return isSnormFormat(format) ? -1.0f : 0.0f;
}

inline bool isIntegerFormat (const vk::VkFormat format)
{
	return isIntFormat(format) || isUintFormat(format);
}

inline bool colorScaleAndBiasAreValid (const vk::VkFormat format, const float colorScale, const float colorBias)
{
	// Only normalized (fixed-point) formats may have scale/bias
	const bool integerOrFloatFormat = isIntFormat(format) || isUintFormat(format) || isFloatFormat(format);
	return !integerOrFloatFormat || (colorScale == 1.0f && colorBias == 0.0f);
}

float					computeStoreColorScale				(const vk::VkFormat format, const tcu::IVec3 imageSize);
ImageType				getImageTypeForSingleLayer			(const ImageType imageType);
vk::VkImageCreateInfo	makeImageCreateInfo					(const Texture& texture, const vk::VkFormat format, const vk::VkImageUsageFlags usage, const vk::VkImageCreateFlags flags);
vk::VkDeviceSize		getOptimalUniformBufferChunkSize	(const vk::InstanceInterface& vki, const vk::VkPhysicalDevice physDevice, vk::VkDeviceSize minimumRequiredChunkSizeBytes);
bool					isStorageImageExtendedFormat		(const vk::VkFormat format);

} // image
} // vkt

#endif // _VKTIMAGELOADSTOREUTIL_HPP
