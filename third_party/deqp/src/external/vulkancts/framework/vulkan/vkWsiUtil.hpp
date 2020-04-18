#ifndef _VKWSIUTIL_HPP
#define _VKWSIUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
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
 * \brief Windowing System Integration (WSI) Utilities.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkWsiPlatform.hpp"
#include "vkRef.hpp"

#include <vector>

namespace vk
{
namespace wsi
{

struct PlatformProperties
{
	enum FeatureFlags
	{
		FEATURE_INITIAL_WINDOW_SIZE		= (1<<0),		//!< Platform honors initial window size request
		FEATURE_RESIZE_WINDOW			= (1<<1),		//!< Platform supports resizing window
	};

	enum SwapchainExtent
	{
		SWAPCHAIN_EXTENT_MUST_MATCH_WINDOW_SIZE = 0,	//!< Swapchain extent must match window size
		SWAPCHAIN_EXTENT_SETS_WINDOW_SIZE,				//!< Window will be resized to swapchain size when first image is presented
		SWAPCHAIN_EXTENT_SCALED_TO_WINDOW_SIZE,			//!< Presented image contents will be scaled to window size

		SWAPCHAIN_EXTENT_LAST
	};

	deUint32		features;
	SwapchainExtent	swapchainExtent;
	deUint32		maxDisplays;
	deUint32		maxWindowsPerDisplay;
};

const char*						getName									(Type wsiType);
const char*						getExtensionName						(Type wsiType);

const PlatformProperties&		getPlatformProperties					(Type wsiType);

VkResult						createSurface							(const InstanceInterface&		vki,
																		 VkInstance						instance,
																		 Type							wsiType,
																		 const Display&					nativeDisplay,
																		 const Window&					nativeWindow,
																		 const VkAllocationCallbacks*	pAllocator,
																		 VkSurfaceKHR*					pSurface);

Move<VkSurfaceKHR>				createSurface							(const InstanceInterface&		vki,
																		 VkInstance						instance,
																		 Type							wsiType,
																		 const Display&					nativeDisplay,
																		 const Window&					nativeWindow,
																		 const VkAllocationCallbacks*	pAllocator = DE_NULL);

VkBool32						getPhysicalDeviceSurfaceSupport			(const InstanceInterface&		vki,
																		 VkPhysicalDevice				physicalDevice,
																		 deUint32						queueFamilyIndex,
																		 VkSurfaceKHR					surface);

VkSurfaceCapabilitiesKHR		getPhysicalDeviceSurfaceCapabilities	(const InstanceInterface&		vki,
																		 VkPhysicalDevice				physicalDevice,
																		 VkSurfaceKHR					surface);

std::vector<VkSurfaceFormatKHR>	getPhysicalDeviceSurfaceFormats			(const InstanceInterface&		vki,
																		 VkPhysicalDevice				physicalDevice,
																		 VkSurfaceKHR					surface);

std::vector<VkPresentModeKHR>	getPhysicalDeviceSurfacePresentModes	(const InstanceInterface&		vki,
																		 VkPhysicalDevice				physicalDevice,
																		 VkSurfaceKHR					surface);

std::vector<VkImage>			getSwapchainImages						(const DeviceInterface&			vkd,
																		 VkDevice						device,
																		 VkSwapchainKHR					swapchain);

} // wsi
} // vk

#endif // _VKWSIUTIL_HPP
