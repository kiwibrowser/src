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
//#includes, #defines, globals and such...
#include <vulkan/vulkan.h>
#include <vk_enum_string_helper.h>
#include <stdint.h>
#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include "vk_enum_validate_helper.h"

// Function Prototypes
uint32_t vk_validate_vkallocationcallbacks(const VkAllocationCallbacks* pStruct);
#ifdef VK_USE_PLATFORM_ANDROID_KHR
uint32_t vk_validate_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_ANDROID_KHR
uint32_t vk_validate_vkapplicationinfo(const VkApplicationInfo* pStruct);
uint32_t vk_validate_vkattachmentdescription(const VkAttachmentDescription* pStruct);
uint32_t vk_validate_vkattachmentreference(const VkAttachmentReference* pStruct);
uint32_t vk_validate_vkbindsparseinfo(const VkBindSparseInfo* pStruct);
uint32_t vk_validate_vkbuffercopy(const VkBufferCopy* pStruct);
uint32_t vk_validate_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct);
uint32_t vk_validate_vkbufferimagecopy(const VkBufferImageCopy* pStruct);
uint32_t vk_validate_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct);
uint32_t vk_validate_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct);
uint32_t vk_validate_vkclearattachment(const VkClearAttachment* pStruct);
uint32_t vk_validate_vkclearcolorvalue(const VkClearColorValue* pStruct);
uint32_t vk_validate_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct);
uint32_t vk_validate_vkclearrect(const VkClearRect* pStruct);
uint32_t vk_validate_vkclearvalue(const VkClearValue* pStruct);
uint32_t vk_validate_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct);
uint32_t vk_validate_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct);
uint32_t vk_validate_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct);
uint32_t vk_validate_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct);
uint32_t vk_validate_vkcomponentmapping(const VkComponentMapping* pStruct);
uint32_t vk_validate_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct);
uint32_t vk_validate_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct);
uint32_t vk_validate_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct);
uint32_t vk_validate_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct);
uint32_t vk_validate_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct);
uint32_t vk_validate_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct);
uint32_t vk_validate_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct);
uint32_t vk_validate_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct);
uint32_t vk_validate_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct);
uint32_t vk_validate_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct);
uint32_t vk_validate_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct);
uint32_t vk_validate_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct);
uint32_t vk_validate_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct);
uint32_t vk_validate_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct);
uint32_t vk_validate_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct);
uint32_t vk_validate_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct);
uint32_t vk_validate_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct);
uint32_t vk_validate_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct);
uint32_t vk_validate_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct);
uint32_t vk_validate_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct);
uint32_t vk_validate_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct);
uint32_t vk_validate_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct);
uint32_t vk_validate_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct);
uint32_t vk_validate_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct);
uint32_t vk_validate_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct);
uint32_t vk_validate_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct);
uint32_t vk_validate_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct);
uint32_t vk_validate_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct);
uint32_t vk_validate_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct);
uint32_t vk_validate_vkeventcreateinfo(const VkEventCreateInfo* pStruct);
uint32_t vk_validate_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct);
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkextensionproperties(const VkExtensionProperties* pStruct);
uint32_t vk_validate_vkextent2d(const VkExtent2D* pStruct);
uint32_t vk_validate_vkextent3d(const VkExtent3D* pStruct);
uint32_t vk_validate_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct);
uint32_t vk_validate_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct);
uint32_t vk_validate_vkfencecreateinfo(const VkFenceCreateInfo* pStruct);
uint32_t vk_validate_vkformatproperties(const VkFormatProperties* pStruct);
uint32_t vk_validate_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct);
uint32_t vk_validate_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct);
uint32_t vk_validate_vkimageblit(const VkImageBlit* pStruct);
uint32_t vk_validate_vkimagecopy(const VkImageCopy* pStruct);
uint32_t vk_validate_vkimagecreateinfo(const VkImageCreateInfo* pStruct);
uint32_t vk_validate_vkimageformatproperties(const VkImageFormatProperties* pStruct);
uint32_t vk_validate_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct);
uint32_t vk_validate_vkimageresolve(const VkImageResolve* pStruct);
uint32_t vk_validate_vkimagesubresource(const VkImageSubresource* pStruct);
uint32_t vk_validate_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct);
uint32_t vk_validate_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct);
uint32_t vk_validate_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct);
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct);
uint32_t vk_validate_vklayerproperties(const VkLayerProperties* pStruct);
uint32_t vk_validate_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct);
uint32_t vk_validate_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct);
uint32_t vk_validate_vkmemorybarrier(const VkMemoryBarrier* pStruct);
uint32_t vk_validate_vkmemoryheap(const VkMemoryHeap* pStruct);
uint32_t vk_validate_vkmemoryrequirements(const VkMemoryRequirements* pStruct);
uint32_t vk_validate_vkmemorytype(const VkMemoryType* pStruct);
#ifdef VK_USE_PLATFORM_MIR_KHR
uint32_t vk_validate_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_MIR_KHR
uint32_t vk_validate_vkoffset2d(const VkOffset2D* pStruct);
uint32_t vk_validate_vkoffset3d(const VkOffset3D* pStruct);
uint32_t vk_validate_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct);
uint32_t vk_validate_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct);
uint32_t vk_validate_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct);
uint32_t vk_validate_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct);
uint32_t vk_validate_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct);
uint32_t vk_validate_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct);
uint32_t vk_validate_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct);
uint32_t vk_validate_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct);
uint32_t vk_validate_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct);
uint32_t vk_validate_vkpresentinfokhr(const VkPresentInfoKHR* pStruct);
uint32_t vk_validate_vkpushconstantrange(const VkPushConstantRange* pStruct);
uint32_t vk_validate_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct);
uint32_t vk_validate_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct);
uint32_t vk_validate_vkrect2d(const VkRect2D* pStruct);
uint32_t vk_validate_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct);
uint32_t vk_validate_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct);
uint32_t vk_validate_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct);
uint32_t vk_validate_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct);
uint32_t vk_validate_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct);
uint32_t vk_validate_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct);
uint32_t vk_validate_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct);
uint32_t vk_validate_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct);
uint32_t vk_validate_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct);
uint32_t vk_validate_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct);
uint32_t vk_validate_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct);
uint32_t vk_validate_vksparsememorybind(const VkSparseMemoryBind* pStruct);
uint32_t vk_validate_vkspecializationinfo(const VkSpecializationInfo* pStruct);
uint32_t vk_validate_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct);
uint32_t vk_validate_vkstencilopstate(const VkStencilOpState* pStruct);
uint32_t vk_validate_vksubmitinfo(const VkSubmitInfo* pStruct);
uint32_t vk_validate_vksubpassdependency(const VkSubpassDependency* pStruct);
uint32_t vk_validate_vksubpassdescription(const VkSubpassDescription* pStruct);
uint32_t vk_validate_vksubresourcelayout(const VkSubresourceLayout* pStruct);
uint32_t vk_validate_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct);
uint32_t vk_validate_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct);
uint32_t vk_validate_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct);
uint32_t vk_validate_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct);
uint32_t vk_validate_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct);
uint32_t vk_validate_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct);
uint32_t vk_validate_vkviewport(const VkViewport* pStruct);
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
uint32_t vk_validate_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct);
#ifdef VK_USE_PLATFORM_XCB_KHR
uint32_t vk_validate_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
uint32_t vk_validate_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct);
#endif //VK_USE_PLATFORM_XLIB_KHR


uint32_t vk_validate_vkallocationcallbacks(const VkAllocationCallbacks* pStruct)
{
    return 1;
}
#ifdef VK_USE_PLATFORM_ANDROID_KHR
uint32_t vk_validate_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_ANDROID_KHR
uint32_t vk_validate_vkapplicationinfo(const VkApplicationInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkattachmentdescription(const VkAttachmentDescription* pStruct)
{
    if (!validate_VkFormat(pStruct->format))
        return 0;
    if (!validate_VkSampleCountFlagBits(pStruct->samples))
        return 0;
    if (!validate_VkAttachmentLoadOp(pStruct->loadOp))
        return 0;
    if (!validate_VkAttachmentStoreOp(pStruct->storeOp))
        return 0;
    if (!validate_VkAttachmentLoadOp(pStruct->stencilLoadOp))
        return 0;
    if (!validate_VkAttachmentStoreOp(pStruct->stencilStoreOp))
        return 0;
    if (!validate_VkImageLayout(pStruct->initialLayout))
        return 0;
    if (!validate_VkImageLayout(pStruct->finalLayout))
        return 0;
    return 1;
}
uint32_t vk_validate_vkattachmentreference(const VkAttachmentReference* pStruct)
{
    if (!validate_VkImageLayout(pStruct->layout))
        return 0;
    return 1;
}
uint32_t vk_validate_vkbindsparseinfo(const VkBindSparseInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pBufferBinds && !vk_validate_vksparsebuffermemorybindinfo((const VkSparseBufferMemoryBindInfo*)pStruct->pBufferBinds))
        return 0;
    if (pStruct->pImageOpaqueBinds && !vk_validate_vksparseimageopaquememorybindinfo((const VkSparseImageOpaqueMemoryBindInfo*)pStruct->pImageOpaqueBinds))
        return 0;
    if (pStruct->pImageBinds && !vk_validate_vksparseimagememorybindinfo((const VkSparseImageMemoryBindInfo*)pStruct->pImageBinds))
        return 0;
    return 1;
}
uint32_t vk_validate_vkbuffercopy(const VkBufferCopy* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkSharingMode(pStruct->sharingMode))
        return 0;
    return 1;
}
uint32_t vk_validate_vkbufferimagecopy(const VkBufferImageCopy* pStruct)
{
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->imageSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->imageOffset))
        return 0;
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->imageExtent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkFormat(pStruct->format))
        return 0;
    return 1;
}
uint32_t vk_validate_vkclearattachment(const VkClearAttachment* pStruct)
{
    if (!vk_validate_vkclearvalue((const VkClearValue*)&pStruct->clearValue))
        return 0;
    return 1;
}
uint32_t vk_validate_vkclearcolorvalue(const VkClearColorValue* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkclearrect(const VkClearRect* pStruct)
{
    if (!vk_validate_vkrect2d((const VkRect2D*)&pStruct->rect))
        return 0;
    return 1;
}
uint32_t vk_validate_vkclearvalue(const VkClearValue* pStruct)
{
    if (!vk_validate_vkclearcolorvalue((const VkClearColorValue*)&pStruct->color))
        return 0;
    if (!vk_validate_vkcleardepthstencilvalue((const VkClearDepthStencilValue*)&pStruct->depthStencil))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkCommandBufferLevel(pStruct->level))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pInheritanceInfo && !vk_validate_vkcommandbufferinheritanceinfo((const VkCommandBufferInheritanceInfo*)pStruct->pInheritanceInfo))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcomponentmapping(const VkComponentMapping* pStruct)
{
    if (!validate_VkComponentSwizzle(pStruct->r))
        return 0;
    if (!validate_VkComponentSwizzle(pStruct->g))
        return 0;
    if (!validate_VkComponentSwizzle(pStruct->b))
        return 0;
    if (!validate_VkComponentSwizzle(pStruct->a))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!vk_validate_vkpipelineshaderstagecreateinfo((const VkPipelineShaderStageCreateInfo*)&pStruct->stage))
        return 0;
    return 1;
}
uint32_t vk_validate_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkDebugReportObjectTypeEXT(pStruct->objectType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkDebugReportObjectTypeEXT(pStruct->objectType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct)
{
    if (!validate_VkImageLayout(pStruct->imageLayout))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pPoolSizes && !vk_validate_vkdescriptorpoolsize((const VkDescriptorPoolSize*)pStruct->pPoolSizes))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct)
{
    if (!validate_VkDescriptorType(pStruct->type))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct)
{
    if (!validate_VkDescriptorType(pStruct->descriptorType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pBindings && !vk_validate_vkdescriptorsetlayoutbinding((const VkDescriptorSetLayoutBinding*)pStruct->pBindings))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pQueueCreateInfos && !vk_validate_vkdevicequeuecreateinfo((const VkDeviceQueueCreateInfo*)pStruct->pQueueCreateInfos))
        return 0;
    if (pStruct->pEnabledFeatures && !vk_validate_vkphysicaldevicefeatures((const VkPhysicalDeviceFeatures*)pStruct->pEnabledFeatures))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!vk_validate_vkdisplaymodeparameterskhr((const VkDisplayModeParametersKHR*)&pStruct->parameters))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct)
{
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->visibleRegion))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct)
{
    if (!vk_validate_vkdisplaymodeparameterskhr((const VkDisplayModeParametersKHR*)&pStruct->parameters))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct)
{
    if (!vk_validate_vkoffset2d((const VkOffset2D*)&pStruct->minSrcPosition))
        return 0;
    if (!vk_validate_vkoffset2d((const VkOffset2D*)&pStruct->maxSrcPosition))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->minSrcExtent))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->maxSrcExtent))
        return 0;
    if (!vk_validate_vkoffset2d((const VkOffset2D*)&pStruct->minDstPosition))
        return 0;
    if (!vk_validate_vkoffset2d((const VkOffset2D*)&pStruct->maxDstPosition))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->minDstExtent))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->maxDstExtent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!vk_validate_vkrect2d((const VkRect2D*)&pStruct->srcRect))
        return 0;
    if (!vk_validate_vkrect2d((const VkRect2D*)&pStruct->dstRect))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct)
{
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->physicalDimensions))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->physicalResolution))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkSurfaceTransformFlagBitsKHR(pStruct->transform))
        return 0;
    if (!validate_VkDisplayPlaneAlphaFlagBitsKHR(pStruct->alphaMode))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->imageExtent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkeventcreateinfo(const VkEventCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkextensionproperties(const VkExtensionProperties* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkextent2d(const VkExtent2D* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkextent3d(const VkExtent3D* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct)
{
    if (!vk_validate_vkimageformatproperties((const VkImageFormatProperties*)&pStruct->imageFormatProperties))
        return 0;
    return 1;
}
uint32_t vk_validate_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkfencecreateinfo(const VkFenceCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkformatproperties(const VkFormatProperties* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pStages && !vk_validate_vkpipelineshaderstagecreateinfo((const VkPipelineShaderStageCreateInfo*)pStruct->pStages))
        return 0;
    if (pStruct->pVertexInputState && !vk_validate_vkpipelinevertexinputstatecreateinfo((const VkPipelineVertexInputStateCreateInfo*)pStruct->pVertexInputState))
        return 0;
    if (pStruct->pInputAssemblyState && !vk_validate_vkpipelineinputassemblystatecreateinfo((const VkPipelineInputAssemblyStateCreateInfo*)pStruct->pInputAssemblyState))
        return 0;
    if (pStruct->pTessellationState && !vk_validate_vkpipelinetessellationstatecreateinfo((const VkPipelineTessellationStateCreateInfo*)pStruct->pTessellationState))
        return 0;
    if (pStruct->pViewportState && !vk_validate_vkpipelineviewportstatecreateinfo((const VkPipelineViewportStateCreateInfo*)pStruct->pViewportState))
        return 0;
    if (pStruct->pRasterizationState && !vk_validate_vkpipelinerasterizationstatecreateinfo((const VkPipelineRasterizationStateCreateInfo*)pStruct->pRasterizationState))
        return 0;
    if (pStruct->pMultisampleState && !vk_validate_vkpipelinemultisamplestatecreateinfo((const VkPipelineMultisampleStateCreateInfo*)pStruct->pMultisampleState))
        return 0;
    if (pStruct->pDepthStencilState && !vk_validate_vkpipelinedepthstencilstatecreateinfo((const VkPipelineDepthStencilStateCreateInfo*)pStruct->pDepthStencilState))
        return 0;
    if (pStruct->pColorBlendState && !vk_validate_vkpipelinecolorblendstatecreateinfo((const VkPipelineColorBlendStateCreateInfo*)pStruct->pColorBlendState))
        return 0;
    if (pStruct->pDynamicState && !vk_validate_vkpipelinedynamicstatecreateinfo((const VkPipelineDynamicStateCreateInfo*)pStruct->pDynamicState))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimageblit(const VkImageBlit* pStruct)
{
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->srcSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->srcOffsets))
        return 0;
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->dstSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->dstOffsets))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimagecopy(const VkImageCopy* pStruct)
{
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->srcSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->srcOffset))
        return 0;
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->dstSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->dstOffset))
        return 0;
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimagecreateinfo(const VkImageCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkImageType(pStruct->imageType))
        return 0;
    if (!validate_VkFormat(pStruct->format))
        return 0;
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->extent))
        return 0;
    if (!validate_VkSampleCountFlagBits(pStruct->samples))
        return 0;
    if (!validate_VkImageTiling(pStruct->tiling))
        return 0;
    if (!validate_VkSharingMode(pStruct->sharingMode))
        return 0;
    if (!validate_VkImageLayout(pStruct->initialLayout))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimageformatproperties(const VkImageFormatProperties* pStruct)
{
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->maxExtent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkImageLayout(pStruct->oldLayout))
        return 0;
    if (!validate_VkImageLayout(pStruct->newLayout))
        return 0;
    if (!vk_validate_vkimagesubresourcerange((const VkImageSubresourceRange*)&pStruct->subresourceRange))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimageresolve(const VkImageResolve* pStruct)
{
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->srcSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->srcOffset))
        return 0;
    if (!vk_validate_vkimagesubresourcelayers((const VkImageSubresourceLayers*)&pStruct->dstSubresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->dstOffset))
        return 0;
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkimagesubresource(const VkImageSubresource* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkImageViewType(pStruct->viewType))
        return 0;
    if (!validate_VkFormat(pStruct->format))
        return 0;
    if (!vk_validate_vkcomponentmapping((const VkComponentMapping*)&pStruct->components))
        return 0;
    if (!vk_validate_vkimagesubresourcerange((const VkImageSubresourceRange*)&pStruct->subresourceRange))
        return 0;
    return 1;
}
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pApplicationInfo && !vk_validate_vkapplicationinfo((const VkApplicationInfo*)pStruct->pApplicationInfo))
        return 0;
    return 1;
}
uint32_t vk_validate_vklayerproperties(const VkLayerProperties* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkmemorybarrier(const VkMemoryBarrier* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkmemoryheap(const VkMemoryHeap* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkmemoryrequirements(const VkMemoryRequirements* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkmemorytype(const VkMemoryType* pStruct)
{
    return 1;
}
#ifdef VK_USE_PLATFORM_MIR_KHR
uint32_t vk_validate_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_MIR_KHR
uint32_t vk_validate_vkoffset2d(const VkOffset2D* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkoffset3d(const VkOffset3D* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct)
{
    if (!vk_validate_vkmemorytype((const VkMemoryType*)&pStruct->memoryTypes))
        return 0;
    if (!vk_validate_vkmemoryheap((const VkMemoryHeap*)&pStruct->memoryHeaps))
        return 0;
    return 1;
}
uint32_t vk_validate_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct)
{
    if (!validate_VkPhysicalDeviceType(pStruct->deviceType))
        return 0;
    if (!vk_validate_vkphysicaldevicelimits((const VkPhysicalDeviceLimits*)&pStruct->limits))
        return 0;
    if (!vk_validate_vkphysicaldevicesparseproperties((const VkPhysicalDeviceSparseProperties*)&pStruct->sparseProperties))
        return 0;
    return 1;
}
uint32_t vk_validate_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct)
{
    if (!validate_VkBlendFactor(pStruct->srcColorBlendFactor))
        return 0;
    if (!validate_VkBlendFactor(pStruct->dstColorBlendFactor))
        return 0;
    if (!validate_VkBlendOp(pStruct->colorBlendOp))
        return 0;
    if (!validate_VkBlendFactor(pStruct->srcAlphaBlendFactor))
        return 0;
    if (!validate_VkBlendFactor(pStruct->dstAlphaBlendFactor))
        return 0;
    if (!validate_VkBlendOp(pStruct->alphaBlendOp))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkLogicOp(pStruct->logicOp))
        return 0;
    if (pStruct->pAttachments && !vk_validate_vkpipelinecolorblendattachmentstate((const VkPipelineColorBlendAttachmentState*)pStruct->pAttachments))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkCompareOp(pStruct->depthCompareOp))
        return 0;
    if (!vk_validate_vkstencilopstate((const VkStencilOpState*)&pStruct->front))
        return 0;
    if (!vk_validate_vkstencilopstate((const VkStencilOpState*)&pStruct->back))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkPrimitiveTopology(pStruct->topology))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pPushConstantRanges && !vk_validate_vkpushconstantrange((const VkPushConstantRange*)pStruct->pPushConstantRanges))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkSampleCountFlagBits(pStruct->rasterizationSamples))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkPolygonMode(pStruct->polygonMode))
        return 0;
    if (!validate_VkFrontFace(pStruct->frontFace))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkRasterizationOrderAMD(pStruct->rasterizationOrder))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkShaderStageFlagBits(pStruct->stage))
        return 0;
    if (pStruct->pSpecializationInfo && !vk_validate_vkspecializationinfo((const VkSpecializationInfo*)pStruct->pSpecializationInfo))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pVertexBindingDescriptions && !vk_validate_vkvertexinputbindingdescription((const VkVertexInputBindingDescription*)pStruct->pVertexBindingDescriptions))
        return 0;
    if (pStruct->pVertexAttributeDescriptions && !vk_validate_vkvertexinputattributedescription((const VkVertexInputAttributeDescription*)pStruct->pVertexAttributeDescriptions))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pViewports && !vk_validate_vkviewport((const VkViewport*)pStruct->pViewports))
        return 0;
    if (pStruct->pScissors && !vk_validate_vkrect2d((const VkRect2D*)pStruct->pScissors))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpresentinfokhr(const VkPresentInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkpushconstantrange(const VkPushConstantRange* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkQueryType(pStruct->queryType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct)
{
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->minImageTransferGranularity))
        return 0;
    return 1;
}
uint32_t vk_validate_vkrect2d(const VkRect2D* pStruct)
{
    if (!vk_validate_vkoffset2d((const VkOffset2D*)&pStruct->offset))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t vk_validate_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!vk_validate_vkrect2d((const VkRect2D*)&pStruct->renderArea))
        return 0;
    if (pStruct->pClearValues && !vk_validate_vkclearvalue((const VkClearValue*)pStruct->pClearValues))
        return 0;
    return 1;
}
uint32_t vk_validate_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (pStruct->pAttachments && !vk_validate_vkattachmentdescription((const VkAttachmentDescription*)pStruct->pAttachments))
        return 0;
    if (pStruct->pSubpasses && !vk_validate_vksubpassdescription((const VkSubpassDescription*)pStruct->pSubpasses))
        return 0;
    if (pStruct->pDependencies && !vk_validate_vksubpassdependency((const VkSubpassDependency*)pStruct->pDependencies))
        return 0;
    return 1;
}
uint32_t vk_validate_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkFilter(pStruct->magFilter))
        return 0;
    if (!validate_VkFilter(pStruct->minFilter))
        return 0;
    if (!validate_VkSamplerMipmapMode(pStruct->mipmapMode))
        return 0;
    if (!validate_VkSamplerAddressMode(pStruct->addressModeU))
        return 0;
    if (!validate_VkSamplerAddressMode(pStruct->addressModeV))
        return 0;
    if (!validate_VkSamplerAddressMode(pStruct->addressModeW))
        return 0;
    if (!validate_VkCompareOp(pStruct->compareOp))
        return 0;
    if (!validate_VkBorderColor(pStruct->borderColor))
        return 0;
    return 1;
}
uint32_t vk_validate_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct)
{
    if (pStruct->pBinds && !vk_validate_vksparsememorybind((const VkSparseMemoryBind*)pStruct->pBinds))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct)
{
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->imageGranularity))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct)
{
    if (!vk_validate_vkimagesubresource((const VkImageSubresource*)&pStruct->subresource))
        return 0;
    if (!vk_validate_vkoffset3d((const VkOffset3D*)&pStruct->offset))
        return 0;
    if (!vk_validate_vkextent3d((const VkExtent3D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct)
{
    if (pStruct->pBinds && !vk_validate_vksparseimagememorybind((const VkSparseImageMemoryBind*)pStruct->pBinds))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct)
{
    if (!vk_validate_vksparseimageformatproperties((const VkSparseImageFormatProperties*)&pStruct->formatProperties))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct)
{
    if (pStruct->pBinds && !vk_validate_vksparsememorybind((const VkSparseMemoryBind*)pStruct->pBinds))
        return 0;
    return 1;
}
uint32_t vk_validate_vksparsememorybind(const VkSparseMemoryBind* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkspecializationinfo(const VkSpecializationInfo* pStruct)
{
    if (pStruct->pMapEntries && !vk_validate_vkspecializationmapentry((const VkSpecializationMapEntry*)pStruct->pMapEntries))
        return 0;
    return 1;
}
uint32_t vk_validate_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct)
{
    return 1;
}
uint32_t vk_validate_vkstencilopstate(const VkStencilOpState* pStruct)
{
    if (!validate_VkStencilOp(pStruct->failOp))
        return 0;
    if (!validate_VkStencilOp(pStruct->passOp))
        return 0;
    if (!validate_VkStencilOp(pStruct->depthFailOp))
        return 0;
    if (!validate_VkCompareOp(pStruct->compareOp))
        return 0;
    return 1;
}
uint32_t vk_validate_vksubmitinfo(const VkSubmitInfo* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vksubpassdependency(const VkSubpassDependency* pStruct)
{
    return 1;
}
uint32_t vk_validate_vksubpassdescription(const VkSubpassDescription* pStruct)
{
    if (!validate_VkPipelineBindPoint(pStruct->pipelineBindPoint))
        return 0;
    if (pStruct->pInputAttachments && !vk_validate_vkattachmentreference((const VkAttachmentReference*)pStruct->pInputAttachments))
        return 0;
    if (pStruct->pColorAttachments && !vk_validate_vkattachmentreference((const VkAttachmentReference*)pStruct->pColorAttachments))
        return 0;
    if (pStruct->pResolveAttachments && !vk_validate_vkattachmentreference((const VkAttachmentReference*)pStruct->pResolveAttachments))
        return 0;
    if (pStruct->pDepthStencilAttachment && !vk_validate_vkattachmentreference((const VkAttachmentReference*)pStruct->pDepthStencilAttachment))
        return 0;
    return 1;
}
uint32_t vk_validate_vksubresourcelayout(const VkSubresourceLayout* pStruct)
{
    return 1;
}
uint32_t vk_validate_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct)
{
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->currentExtent))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->minImageExtent))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->maxImageExtent))
        return 0;
    if (!validate_VkSurfaceTransformFlagBitsKHR(pStruct->currentTransform))
        return 0;
    return 1;
}
uint32_t vk_validate_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct)
{
    if (!validate_VkFormat(pStruct->format))
        return 0;
    if (!validate_VkColorSpaceKHR(pStruct->colorSpace))
        return 0;
    return 1;
}
uint32_t vk_validate_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkFormat(pStruct->imageFormat))
        return 0;
    if (!validate_VkColorSpaceKHR(pStruct->imageColorSpace))
        return 0;
    if (!vk_validate_vkextent2d((const VkExtent2D*)&pStruct->imageExtent))
        return 0;
    if (!validate_VkSharingMode(pStruct->imageSharingMode))
        return 0;
    if (!validate_VkSurfaceTransformFlagBitsKHR(pStruct->preTransform))
        return 0;
    if (!validate_VkCompositeAlphaFlagBitsKHR(pStruct->compositeAlpha))
        return 0;
    if (!validate_VkPresentModeKHR(pStruct->presentMode))
        return 0;
    return 1;
}
uint32_t vk_validate_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
uint32_t vk_validate_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct)
{
    if (!validate_VkFormat(pStruct->format))
        return 0;
    return 1;
}
uint32_t vk_validate_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct)
{
    if (!validate_VkVertexInputRate(pStruct->inputRate))
        return 0;
    return 1;
}
uint32_t vk_validate_vkviewport(const VkViewport* pStruct)
{
    return 1;
}
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
uint32_t vk_validate_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
uint32_t vk_validate_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    if (!validate_VkDescriptorType(pStruct->descriptorType))
        return 0;
    if (pStruct->pImageInfo && !vk_validate_vkdescriptorimageinfo((const VkDescriptorImageInfo*)pStruct->pImageInfo))
        return 0;
    if (pStruct->pBufferInfo && !vk_validate_vkdescriptorbufferinfo((const VkDescriptorBufferInfo*)pStruct->pBufferInfo))
        return 0;
    return 1;
}
#ifdef VK_USE_PLATFORM_XCB_KHR
uint32_t vk_validate_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
uint32_t vk_validate_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct)
{
    if (!validate_VkStructureType(pStruct->sType))
        return 0;
    return 1;
}
#endif //VK_USE_PLATFORM_XLIB_KHR