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
 * \brief VK_EXT_debug_report utilities
 *//*--------------------------------------------------------------------*/

#include "vkDebugReportUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "deArrayUtil.hpp"

namespace vk
{

namespace
{

tcu::Format::Bitfield<32> shortDebugFlagsStr (VkDebugReportFlagsEXT flags)
{
	static const tcu::Format::BitDesc	s_bits[] =
	{
		tcu::Format::BitDesc(VK_DEBUG_REPORT_INFORMATION_BIT_EXT,			"INFO"),
		tcu::Format::BitDesc(VK_DEBUG_REPORT_WARNING_BIT_EXT,				"WARNING"),
		tcu::Format::BitDesc(VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,	"PERFORMANCE"),
		tcu::Format::BitDesc(VK_DEBUG_REPORT_ERROR_BIT_EXT,					"ERROR"),
		tcu::Format::BitDesc(VK_DEBUG_REPORT_DEBUG_BIT_EXT,					"DEBUG"),
	};

	return tcu::Format::Bitfield<32>(flags, DE_ARRAY_BEGIN(s_bits), DE_ARRAY_END(s_bits));
}

const char* getShortObjectTypeName (VkDebugReportObjectTypeEXT objectType)
{
	switch (objectType)
	{
		case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:							return "Unknown";
		case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:							return "Instance";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:					return "PhysicalDevice";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:							return "Device";
		case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:								return "Queue";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:							return "Semaphore";
		case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:					return "CommandBuffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:								return "Fence";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:						return "DeviceMemory";
		case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:							return "Buffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:								return "Image";
		case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:								return "Event";
		case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:						return "QueryPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:						return "BufferView";
		case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:						return "ImageView";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:						return "ShaderModule";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:					return "PipelineCache";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:					return "PipelineLayout";
		case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:						return "RenderPass";
		case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:							return "Pipeline";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:				return "DescriptorSetLayout";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:							return "Sampler";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:					return "DescriptorPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:					return "DescriptorSet";
		case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:						return "Framebuffer";
		case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:						return "CommandPool";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:						return "SurfaceKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:						return "SwapchainKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:						return "DebugReportEXT";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT:						return "DisplayKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT:					return "DisplayModeKHR";
		case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT:	return "DescriptorUpdateTemplateKHR";
		default:																return DE_NULL;
	}
}

tcu::Format::Enum<VkDebugReportObjectTypeEXT> shortObjectTypeStr (VkDebugReportObjectTypeEXT objectType)
{
	return tcu::Format::Enum<VkDebugReportObjectTypeEXT>(getShortObjectTypeName, objectType);
}

} // anonymous

std::ostream& operator<< (std::ostream& str, const DebugReportMessage& message)
{
	str << shortDebugFlagsStr(message.flags) << ": "
		<< message.message
		<< " (code " << tcu::toHex(message.messageCode);

	if (!message.layerPrefix.empty())
		str << " from " << message.layerPrefix;

	str << " at " << shortObjectTypeStr(message.objectType) << ":" << message.location << ")";

	return str;
}

namespace
{

VKAPI_ATTR VkBool32	VKAPI_CALL debugReportCallback (VkDebugReportFlagsEXT		flags,
													VkDebugReportObjectTypeEXT	objectType,
													deUint64					object,
													size_t						location,
													deInt32						messageCode,
													const char*					pLayerPrefix,
													const char*					pMessage,
													void*						pUserData)
{
	DebugReportRecorder::MessageList* const	messageList	= reinterpret_cast<DebugReportRecorder::MessageList*>(pUserData);

	messageList->append(DebugReportMessage(flags, objectType, object, location, messageCode, pLayerPrefix, pMessage));

	// Return false to indicate that the call should not return error and should
	// continue execution normally.
	return VK_FALSE;
}

Move<VkDebugReportCallbackEXT> createCallback (const InstanceInterface&				vki,
											   VkInstance							instance,
											   DebugReportRecorder::MessageList*	messageList)
{
	const VkDebugReportFlagsEXT					allFlags	= VK_DEBUG_REPORT_INFORMATION_BIT_EXT
															| VK_DEBUG_REPORT_WARNING_BIT_EXT
															| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
															| VK_DEBUG_REPORT_ERROR_BIT_EXT
															| VK_DEBUG_REPORT_DEBUG_BIT_EXT;

	const VkDebugReportCallbackCreateInfoEXT	createInfo	=
	{
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		DE_NULL,
		allFlags,
		debugReportCallback,
		messageList
	};

	return createDebugReportCallbackEXT(vki, instance, &createInfo);
}

} // anonymous

DebugReportRecorder::DebugReportRecorder (const InstanceInterface& vki, VkInstance instance)
	: m_messages	(1024)
	, m_callback	(createCallback(vki, instance, &m_messages))
{
}

DebugReportRecorder::~DebugReportRecorder (void)
{
}

bool isDebugReportSupported (const PlatformInterface& vkp)
{
	return isExtensionSupported(enumerateInstanceExtensionProperties(vkp, DE_NULL),
								RequiredExtension("VK_EXT_debug_report"));
}

} // vk
