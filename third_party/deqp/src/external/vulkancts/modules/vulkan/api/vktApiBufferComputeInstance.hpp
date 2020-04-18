#ifndef _VKTAPIBUFFERCOMPUTEINSTANCE_HPP
#define _VKTAPIBUFFERCOMPUTEINSTANCE_HPP
/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tcuVectorType.hpp"
#include "vkRef.hpp"
#include "vkMemUtil.hpp"
#include "vktTestCase.hpp"

namespace vkt
{
namespace api
{

vk::Move<vk::VkBuffer>					createDataBuffer		(vkt::Context&					context,
																 deUint32						offset,
																 deUint32						bufferSize,
																 deUint32						initData,
																 deUint32						initDataSize,
																 deUint32						uninitData,
																 de::MovePtr<vk::Allocation>*	outAllocation);

vk::Move<vk::VkBuffer>					createColorDataBuffer (	deUint32 offset,
																deUint32 bufferSize,
																const tcu::Vec4& color1,
																const tcu::Vec4& color2,
																de::MovePtr<vk::Allocation>* outAllocation,
																vkt::Context& context);

vk::Move<vk::VkDescriptorSetLayout>		createDescriptorSetLayout (vkt::Context& context);

vk::Move<vk::VkDescriptorPool>			createDescriptorPool (vkt::Context& context);

vk::Move<vk::VkDescriptorSet>			createDescriptorSet		(vkt::Context&				context,
																 vk::VkDescriptorPool		pool,
																 vk::VkDescriptorSetLayout	layout,
																 vk::VkBuffer				buffer,
																 deUint32					offset,
																 vk::VkBuffer				resBuf);

vk::Move<vk::VkDescriptorSet>			createDescriptorSet (vk::VkDescriptorPool pool,
															  vk::VkDescriptorSetLayout layout,
															  vk::VkBuffer viewA, deUint32 offsetA,
															  vk::VkBuffer viewB,
															  deUint32 offsetB,
															  vk::VkBuffer resBuf,
															  vkt::Context& context);

} // api
} // vkt

#endif // _VKTAPIBUFFERCOMPUTEINSTANCE_HPP
