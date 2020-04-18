#ifndef _VKTPIPELINECLEARUTIL_HPP
#define _VKTPIPELINECLEARUTIL_HPP
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
 * \brief Utilities for clear values.
 *//*--------------------------------------------------------------------*/

#include "tcuTexture.hpp"
#include "tcuVectorUtil.hpp"
#include "vkDefs.hpp"

namespace vkt
{
namespace pipeline
{

tcu::Vec4						defaultClearColor				(const tcu::TextureFormat& format);
tcu::IVec4						defaultClearColorInt			(const tcu::TextureFormat& format);
tcu::UVec4						defaultClearColorUint			(const tcu::TextureFormat& format);
tcu::Vec4						defaultClearColorUnorm			(void);
float							defaultClearDepth				(void);
deUint32						defaultClearStencil				(void);

vk::VkClearDepthStencilValue	defaultClearDepthStencilValue	(void);
vk::VkClearValue				defaultClearValue				(vk::VkFormat format);

} // pipeline
} // vkt

#endif // _VKTPIPELINECLEARUTIL_HPP
