/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation.
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

//#includes, #defines, globals and such...
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

// Function Prototypes
size_t get_struct_chain_size(const void* pStruct);
size_t get_dynamic_struct_size(const void* pStruct);
size_t vk_size_vkallocationcallbacks(const VkAllocationCallbacks* pStruct);
#ifdef VK_USE_PLATFORM_ANDROID_KHR
size_t vk_size_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_ANDROID_KHR
size_t vk_size_vkapplicationinfo(const VkApplicationInfo* pStruct);
size_t vk_size_vkattachmentdescription(const VkAttachmentDescription* pStruct);
size_t vk_size_vkattachmentreference(const VkAttachmentReference* pStruct);
size_t vk_size_vkbindsparseinfo(const VkBindSparseInfo* pStruct);
size_t vk_size_vkbuffercopy(const VkBufferCopy* pStruct);
size_t vk_size_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct);
size_t vk_size_vkbufferimagecopy(const VkBufferImageCopy* pStruct);
size_t vk_size_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct);
size_t vk_size_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct);
size_t vk_size_vkclearattachment(const VkClearAttachment* pStruct);
size_t vk_size_vkclearcolorvalue(const VkClearColorValue* pStruct);
size_t vk_size_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct);
size_t vk_size_vkclearrect(const VkClearRect* pStruct);
size_t vk_size_vkclearvalue(const VkClearValue* pStruct);
size_t vk_size_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct);
size_t vk_size_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct);
size_t vk_size_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct);
size_t vk_size_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct);
size_t vk_size_vkcomponentmapping(const VkComponentMapping* pStruct);
size_t vk_size_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct);
size_t vk_size_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct);
size_t vk_size_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct);
size_t vk_size_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct);
size_t vk_size_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct);
size_t vk_size_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct);
size_t vk_size_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct);
size_t vk_size_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct);
size_t vk_size_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct);
size_t vk_size_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct);
size_t vk_size_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct);
size_t vk_size_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct);
size_t vk_size_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct);
size_t vk_size_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct);
size_t vk_size_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct);
size_t vk_size_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct);
size_t vk_size_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct);
size_t vk_size_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct);
size_t vk_size_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct);
size_t vk_size_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct);
size_t vk_size_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct);
size_t vk_size_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct);
size_t vk_size_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct);
size_t vk_size_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct);
size_t vk_size_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct);
size_t vk_size_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct);
size_t vk_size_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct);
size_t vk_size_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct);
size_t vk_size_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct);
size_t vk_size_vkeventcreateinfo(const VkEventCreateInfo* pStruct);
size_t vk_size_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct);
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkextensionproperties(const VkExtensionProperties* pStruct);
size_t vk_size_vkextent2d(const VkExtent2D* pStruct);
size_t vk_size_vkextent3d(const VkExtent3D* pStruct);
size_t vk_size_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct);
size_t vk_size_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct);
size_t vk_size_vkfencecreateinfo(const VkFenceCreateInfo* pStruct);
size_t vk_size_vkformatproperties(const VkFormatProperties* pStruct);
size_t vk_size_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct);
size_t vk_size_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct);
size_t vk_size_vkimageblit(const VkImageBlit* pStruct);
size_t vk_size_vkimagecopy(const VkImageCopy* pStruct);
size_t vk_size_vkimagecreateinfo(const VkImageCreateInfo* pStruct);
size_t vk_size_vkimageformatproperties(const VkImageFormatProperties* pStruct);
size_t vk_size_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct);
size_t vk_size_vkimageresolve(const VkImageResolve* pStruct);
size_t vk_size_vkimagesubresource(const VkImageSubresource* pStruct);
size_t vk_size_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct);
size_t vk_size_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct);
size_t vk_size_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct);
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct);
size_t vk_size_vklayerproperties(const VkLayerProperties* pStruct);
size_t vk_size_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct);
size_t vk_size_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct);
size_t vk_size_vkmemorybarrier(const VkMemoryBarrier* pStruct);
size_t vk_size_vkmemoryheap(const VkMemoryHeap* pStruct);
size_t vk_size_vkmemoryrequirements(const VkMemoryRequirements* pStruct);
size_t vk_size_vkmemorytype(const VkMemoryType* pStruct);
#ifdef VK_USE_PLATFORM_MIR_KHR
size_t vk_size_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_MIR_KHR
size_t vk_size_vkoffset2d(const VkOffset2D* pStruct);
size_t vk_size_vkoffset3d(const VkOffset3D* pStruct);
size_t vk_size_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct);
size_t vk_size_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct);
size_t vk_size_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct);
size_t vk_size_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct);
size_t vk_size_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct);
size_t vk_size_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct);
size_t vk_size_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct);
size_t vk_size_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct);
size_t vk_size_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct);
size_t vk_size_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct);
size_t vk_size_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct);
size_t vk_size_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct);
size_t vk_size_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct);
size_t vk_size_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct);
size_t vk_size_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct);
size_t vk_size_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct);
size_t vk_size_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct);
size_t vk_size_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct);
size_t vk_size_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct);
size_t vk_size_vkpresentinfokhr(const VkPresentInfoKHR* pStruct);
size_t vk_size_vkpushconstantrange(const VkPushConstantRange* pStruct);
size_t vk_size_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct);
size_t vk_size_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct);
size_t vk_size_vkrect2d(const VkRect2D* pStruct);
size_t vk_size_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct);
size_t vk_size_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct);
size_t vk_size_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct);
size_t vk_size_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct);
size_t vk_size_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct);
size_t vk_size_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct);
size_t vk_size_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct);
size_t vk_size_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct);
size_t vk_size_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct);
size_t vk_size_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct);
size_t vk_size_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct);
size_t vk_size_vksparsememorybind(const VkSparseMemoryBind* pStruct);
size_t vk_size_vkspecializationinfo(const VkSpecializationInfo* pStruct);
size_t vk_size_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct);
size_t vk_size_vkstencilopstate(const VkStencilOpState* pStruct);
size_t vk_size_vksubmitinfo(const VkSubmitInfo* pStruct);
size_t vk_size_vksubpassdependency(const VkSubpassDependency* pStruct);
size_t vk_size_vksubpassdescription(const VkSubpassDescription* pStruct);
size_t vk_size_vksubresourcelayout(const VkSubresourceLayout* pStruct);
size_t vk_size_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct);
size_t vk_size_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct);
size_t vk_size_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct);
size_t vk_size_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct);
size_t vk_size_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct);
size_t vk_size_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct);
size_t vk_size_vkviewport(const VkViewport* pStruct);
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
size_t vk_size_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct);
#ifdef VK_USE_PLATFORM_XCB_KHR
size_t vk_size_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
size_t vk_size_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_XLIB_KHR

#ifdef __cplusplus
}
#endif