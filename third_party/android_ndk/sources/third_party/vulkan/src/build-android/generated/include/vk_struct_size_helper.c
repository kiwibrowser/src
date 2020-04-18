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
#include "vk_struct_size_helper.h"
#include <string.h>
#include <assert.h>

// Function definitions
size_t vk_size_vkallocationcallbacks(const VkAllocationCallbacks* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkAllocationCallbacks);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_ANDROID_KHR
size_t vk_size_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkAndroidSurfaceCreateInfoKHR);
        structSize += sizeof(ANativeWindow);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_ANDROID_KHR
size_t vk_size_vkapplicationinfo(const VkApplicationInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkApplicationInfo);
        structSize += (pStruct->pApplicationName != NULL) ? sizeof(char)*(1+strlen(pStruct->pApplicationName)) : 0;
        structSize += (pStruct->pEngineName != NULL) ? sizeof(char)*(1+strlen(pStruct->pEngineName)) : 0;
    }
    return structSize;
}
size_t vk_size_vkattachmentdescription(const VkAttachmentDescription* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkAttachmentDescription);
    }
    return structSize;
}
size_t vk_size_vkattachmentreference(const VkAttachmentReference* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkAttachmentReference);
    }
    return structSize;
}
size_t vk_size_vkbindsparseinfo(const VkBindSparseInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBindSparseInfo);
        structSize += pStruct->waitSemaphoreCount*sizeof(VkSemaphore);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bufferBindCount; i++) {
            structSize += vk_size_vksparsebuffermemorybindinfo(&pStruct->pBufferBinds[i]);
        }
        for (i = 0; i < pStruct->imageOpaqueBindCount; i++) {
            structSize += vk_size_vksparseimageopaquememorybindinfo(&pStruct->pImageOpaqueBinds[i]);
        }
        for (i = 0; i < pStruct->imageBindCount; i++) {
            structSize += vk_size_vksparseimagememorybindinfo(&pStruct->pImageBinds[i]);
        }
        structSize += pStruct->signalSemaphoreCount*sizeof(VkSemaphore);
    }
    return structSize;
}
size_t vk_size_vkbuffercopy(const VkBufferCopy* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBufferCopy);
    }
    return structSize;
}
size_t vk_size_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBufferCreateInfo);
        structSize += pStruct->queueFamilyIndexCount*sizeof(uint32_t);
    }
    return structSize;
}
size_t vk_size_vkbufferimagecopy(const VkBufferImageCopy* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBufferImageCopy);
    }
    return structSize;
}
size_t vk_size_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBufferMemoryBarrier);
    }
    return structSize;
}
size_t vk_size_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkBufferViewCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkclearattachment(const VkClearAttachment* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkClearAttachment);
    }
    return structSize;
}
size_t vk_size_vkclearcolorvalue(const VkClearColorValue* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkClearColorValue);
    }
    return structSize;
}
size_t vk_size_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkClearDepthStencilValue);
    }
    return structSize;
}
size_t vk_size_vkclearrect(const VkClearRect* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkClearRect);
    }
    return structSize;
}
size_t vk_size_vkclearvalue(const VkClearValue* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkClearValue);
    }
    return structSize;
}
size_t vk_size_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkCommandBufferAllocateInfo);
    }
    return structSize;
}
size_t vk_size_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkCommandBufferBeginInfo);
        structSize += vk_size_vkcommandbufferinheritanceinfo(pStruct->pInheritanceInfo);
    }
    return structSize;
}
size_t vk_size_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkCommandBufferInheritanceInfo);
    }
    return structSize;
}
size_t vk_size_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkCommandPoolCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkcomponentmapping(const VkComponentMapping* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkComponentMapping);
    }
    return structSize;
}
size_t vk_size_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkComputePipelineCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkCopyDescriptorSet);
    }
    return structSize;
}
size_t vk_size_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDebugMarkerMarkerInfoEXT);
        structSize += (pStruct->pMarkerName != NULL) ? sizeof(char)*(1+strlen(pStruct->pMarkerName)) : 0;
    }
    return structSize;
}
size_t vk_size_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDebugMarkerObjectNameInfoEXT);
        structSize += (pStruct->pObjectName != NULL) ? sizeof(char)*(1+strlen(pStruct->pObjectName)) : 0;
    }
    return structSize;
}
size_t vk_size_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDebugMarkerObjectTagInfoEXT);
        structSize += pStruct->tagSize;
    }
    return structSize;
}
size_t vk_size_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDebugReportCallbackCreateInfoEXT);
    }
    return structSize;
}
size_t vk_size_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDedicatedAllocationBufferCreateInfoNV);
    }
    return structSize;
}
size_t vk_size_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDedicatedAllocationImageCreateInfoNV);
    }
    return structSize;
}
size_t vk_size_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDedicatedAllocationMemoryAllocateInfoNV);
    }
    return structSize;
}
size_t vk_size_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorBufferInfo);
    }
    return structSize;
}
size_t vk_size_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorImageInfo);
    }
    return structSize;
}
size_t vk_size_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorPoolCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->poolSizeCount; i++) {
            structSize += vk_size_vkdescriptorpoolsize(&pStruct->pPoolSizes[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorPoolSize);
    }
    return structSize;
}
size_t vk_size_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorSetAllocateInfo);
        structSize += pStruct->descriptorSetCount*sizeof(VkDescriptorSetLayout);
    }
    return structSize;
}
size_t vk_size_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorSetLayoutBinding);
        structSize += pStruct->descriptorCount*sizeof(VkSampler);
    }
    return structSize;
}
size_t vk_size_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDescriptorSetLayoutCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bindingCount; i++) {
            structSize += vk_size_vkdescriptorsetlayoutbinding(&pStruct->pBindings[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDeviceCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->queueCreateInfoCount; i++) {
            structSize += vk_size_vkdevicequeuecreateinfo(&pStruct->pQueueCreateInfos[i]);
        }
        for (i = 0; i < pStruct->enabledLayerCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppEnabledLayerNames[i]))));
        }
        for (i = 0; i < pStruct->enabledExtensionCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppEnabledExtensionNames[i]))));
        }
        structSize += vk_size_vkphysicaldevicefeatures(pStruct->pEnabledFeatures);
    }
    return structSize;
}
size_t vk_size_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDeviceQueueCreateInfo);
        structSize += pStruct->queueCount*sizeof(float);
    }
    return structSize;
}
size_t vk_size_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDispatchIndirectCommand);
    }
    return structSize;
}
size_t vk_size_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayModeCreateInfoKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayModeParametersKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayModePropertiesKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayPlaneCapabilitiesKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayPlanePropertiesKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayPresentInfoKHR);
    }
    return structSize;
}
size_t vk_size_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplayPropertiesKHR);
        structSize += (pStruct->displayName != NULL) ? sizeof(char)*(1+strlen(pStruct->displayName)) : 0;
    }
    return structSize;
}
size_t vk_size_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDisplaySurfaceCreateInfoKHR);
    }
    return structSize;
}
size_t vk_size_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDrawIndexedIndirectCommand);
    }
    return structSize;
}
size_t vk_size_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkDrawIndirectCommand);
    }
    return structSize;
}
size_t vk_size_vkeventcreateinfo(const VkEventCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkEventCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExportMemoryAllocateInfoNV);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExportMemoryWin32HandleInfoNV);
        structSize += sizeof(SECURITY_ATTRIBUTES);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkextensionproperties(const VkExtensionProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExtensionProperties);
    }
    return structSize;
}
size_t vk_size_vkextent2d(const VkExtent2D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExtent2D);
    }
    return structSize;
}
size_t vk_size_vkextent3d(const VkExtent3D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExtent3D);
    }
    return structSize;
}
size_t vk_size_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExternalImageFormatPropertiesNV);
    }
    return structSize;
}
size_t vk_size_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkExternalMemoryImageCreateInfoNV);
    }
    return structSize;
}
size_t vk_size_vkfencecreateinfo(const VkFenceCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkFenceCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkformatproperties(const VkFormatProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkFormatProperties);
    }
    return structSize;
}
size_t vk_size_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkFramebufferCreateInfo);
        structSize += pStruct->attachmentCount*sizeof(VkImageView);
    }
    return structSize;
}
size_t vk_size_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkGraphicsPipelineCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->stageCount; i++) {
            structSize += vk_size_vkpipelineshaderstagecreateinfo(&pStruct->pStages[i]);
        }
        structSize += vk_size_vkpipelinevertexinputstatecreateinfo(pStruct->pVertexInputState);
        structSize += vk_size_vkpipelineinputassemblystatecreateinfo(pStruct->pInputAssemblyState);
        structSize += vk_size_vkpipelinetessellationstatecreateinfo(pStruct->pTessellationState);
        structSize += vk_size_vkpipelineviewportstatecreateinfo(pStruct->pViewportState);
        structSize += vk_size_vkpipelinerasterizationstatecreateinfo(pStruct->pRasterizationState);
        structSize += vk_size_vkpipelinemultisamplestatecreateinfo(pStruct->pMultisampleState);
        structSize += vk_size_vkpipelinedepthstencilstatecreateinfo(pStruct->pDepthStencilState);
        structSize += vk_size_vkpipelinecolorblendstatecreateinfo(pStruct->pColorBlendState);
        structSize += vk_size_vkpipelinedynamicstatecreateinfo(pStruct->pDynamicState);
    }
    return structSize;
}
size_t vk_size_vkimageblit(const VkImageBlit* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageBlit);
    }
    return structSize;
}
size_t vk_size_vkimagecopy(const VkImageCopy* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageCopy);
    }
    return structSize;
}
size_t vk_size_vkimagecreateinfo(const VkImageCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageCreateInfo);
        structSize += pStruct->queueFamilyIndexCount*sizeof(uint32_t);
    }
    return structSize;
}
size_t vk_size_vkimageformatproperties(const VkImageFormatProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageFormatProperties);
    }
    return structSize;
}
size_t vk_size_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageMemoryBarrier);
    }
    return structSize;
}
size_t vk_size_vkimageresolve(const VkImageResolve* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageResolve);
    }
    return structSize;
}
size_t vk_size_vkimagesubresource(const VkImageSubresource* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageSubresource);
    }
    return structSize;
}
size_t vk_size_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageSubresourceLayers);
    }
    return structSize;
}
size_t vk_size_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageSubresourceRange);
    }
    return structSize;
}
size_t vk_size_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImageViewCreateInfo);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkImportMemoryWin32HandleInfoNV);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkInstanceCreateInfo);
        structSize += vk_size_vkapplicationinfo(pStruct->pApplicationInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->enabledLayerCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppEnabledLayerNames[i]))));
        }
        for (i = 0; i < pStruct->enabledExtensionCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppEnabledExtensionNames[i]))));
        }
    }
    return structSize;
}
size_t vk_size_vklayerproperties(const VkLayerProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkLayerProperties);
    }
    return structSize;
}
size_t vk_size_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMappedMemoryRange);
    }
    return structSize;
}
size_t vk_size_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMemoryAllocateInfo);
    }
    return structSize;
}
size_t vk_size_vkmemorybarrier(const VkMemoryBarrier* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMemoryBarrier);
    }
    return structSize;
}
size_t vk_size_vkmemoryheap(const VkMemoryHeap* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMemoryHeap);
    }
    return structSize;
}
size_t vk_size_vkmemoryrequirements(const VkMemoryRequirements* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMemoryRequirements);
    }
    return structSize;
}
size_t vk_size_vkmemorytype(const VkMemoryType* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMemoryType);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_MIR_KHR
size_t vk_size_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkMirSurfaceCreateInfoKHR);
        structSize += sizeof(MirConnection);
        structSize += sizeof(MirSurface);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_MIR_KHR
size_t vk_size_vkoffset2d(const VkOffset2D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkOffset2D);
    }
    return structSize;
}
size_t vk_size_vkoffset3d(const VkOffset3D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkOffset3D);
    }
    return structSize;
}
size_t vk_size_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPhysicalDeviceFeatures);
    }
    return structSize;
}
size_t vk_size_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPhysicalDeviceLimits);
        structSize += pStruct->minMemoryMapAlignment;
    }
    return structSize;
}
size_t vk_size_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPhysicalDeviceMemoryProperties);
    }
    return structSize;
}
size_t vk_size_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPhysicalDeviceProperties);
    }
    return structSize;
}
size_t vk_size_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPhysicalDeviceSparseProperties);
    }
    return structSize;
}
size_t vk_size_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineCacheCreateInfo);
        structSize += pStruct->initialDataSize;
    }
    return structSize;
}
size_t vk_size_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineColorBlendAttachmentState);
    }
    return structSize;
}
size_t vk_size_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineColorBlendStateCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->attachmentCount; i++) {
            structSize += vk_size_vkpipelinecolorblendattachmentstate(&pStruct->pAttachments[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineDepthStencilStateCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineDynamicStateCreateInfo);
        structSize += pStruct->dynamicStateCount*sizeof(VkDynamicState);
    }
    return structSize;
}
size_t vk_size_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineInputAssemblyStateCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineLayoutCreateInfo);
        structSize += pStruct->setLayoutCount*sizeof(VkDescriptorSetLayout);
        uint32_t i = 0;
        for (i = 0; i < pStruct->pushConstantRangeCount; i++) {
            structSize += vk_size_vkpushconstantrange(&pStruct->pPushConstantRanges[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineMultisampleStateCreateInfo);
        structSize += sizeof(VkSampleMask);
    }
    return structSize;
}
size_t vk_size_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineRasterizationStateCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineRasterizationStateRasterizationOrderAMD);
    }
    return structSize;
}
size_t vk_size_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineShaderStageCreateInfo);
        structSize += (pStruct->pName != NULL) ? sizeof(char)*(1+strlen(pStruct->pName)) : 0;
        structSize += vk_size_vkspecializationinfo(pStruct->pSpecializationInfo);
    }
    return structSize;
}
size_t vk_size_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineTessellationStateCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineVertexInputStateCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->vertexBindingDescriptionCount; i++) {
            structSize += vk_size_vkvertexinputbindingdescription(&pStruct->pVertexBindingDescriptions[i]);
        }
        for (i = 0; i < pStruct->vertexAttributeDescriptionCount; i++) {
            structSize += vk_size_vkvertexinputattributedescription(&pStruct->pVertexAttributeDescriptions[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPipelineViewportStateCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->viewportCount; i++) {
            structSize += vk_size_vkviewport(&pStruct->pViewports[i]);
        }
        for (i = 0; i < pStruct->scissorCount; i++) {
            structSize += vk_size_vkrect2d(&pStruct->pScissors[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkpresentinfokhr(const VkPresentInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPresentInfoKHR);
        structSize += pStruct->waitSemaphoreCount*sizeof(VkSemaphore);
        structSize += pStruct->swapchainCount*sizeof(VkSwapchainKHR);
        structSize += pStruct->swapchainCount*sizeof(uint32_t);
        structSize += sizeof(VkResult);
    }
    return structSize;
}
size_t vk_size_vkpushconstantrange(const VkPushConstantRange* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkPushConstantRange);
    }
    return structSize;
}
size_t vk_size_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkQueryPoolCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkQueueFamilyProperties);
    }
    return structSize;
}
size_t vk_size_vkrect2d(const VkRect2D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkRect2D);
    }
    return structSize;
}
size_t vk_size_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkRenderPassBeginInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->clearValueCount; i++) {
            structSize += vk_size_vkclearvalue(&pStruct->pClearValues[i]);
        }
    }
    return structSize;
}
size_t vk_size_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkRenderPassCreateInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->attachmentCount; i++) {
            structSize += vk_size_vkattachmentdescription(&pStruct->pAttachments[i]);
        }
        for (i = 0; i < pStruct->subpassCount; i++) {
            structSize += vk_size_vksubpassdescription(&pStruct->pSubpasses[i]);
        }
        for (i = 0; i < pStruct->dependencyCount; i++) {
            structSize += vk_size_vksubpassdependency(&pStruct->pDependencies[i]);
        }
    }
    return structSize;
}
size_t vk_size_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSamplerCreateInfo);
    }
    return structSize;
}
size_t vk_size_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSemaphoreCreateInfo);
    }
    return structSize;
}
size_t vk_size_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkShaderModuleCreateInfo);
        structSize += pStruct->codeSize;
    }
    return structSize;
}
size_t vk_size_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseBufferMemoryBindInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bindCount; i++) {
            structSize += vk_size_vksparsememorybind(&pStruct->pBinds[i]);
        }
    }
    return structSize;
}
size_t vk_size_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseImageFormatProperties);
    }
    return structSize;
}
size_t vk_size_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseImageMemoryBind);
    }
    return structSize;
}
size_t vk_size_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseImageMemoryBindInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bindCount; i++) {
            structSize += vk_size_vksparseimagememorybind(&pStruct->pBinds[i]);
        }
    }
    return structSize;
}
size_t vk_size_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseImageMemoryRequirements);
    }
    return structSize;
}
size_t vk_size_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseImageOpaqueMemoryBindInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bindCount; i++) {
            structSize += vk_size_vksparsememorybind(&pStruct->pBinds[i]);
        }
    }
    return structSize;
}
size_t vk_size_vksparsememorybind(const VkSparseMemoryBind* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSparseMemoryBind);
    }
    return structSize;
}
size_t vk_size_vkspecializationinfo(const VkSpecializationInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSpecializationInfo);
        uint32_t i = 0;
        for (i = 0; i < pStruct->mapEntryCount; i++) {
            structSize += vk_size_vkspecializationmapentry(&pStruct->pMapEntries[i]);
        }
        structSize += pStruct->dataSize;
    }
    return structSize;
}
size_t vk_size_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSpecializationMapEntry);
        structSize += pStruct->size;
    }
    return structSize;
}
size_t vk_size_vkstencilopstate(const VkStencilOpState* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkStencilOpState);
    }
    return structSize;
}
size_t vk_size_vksubmitinfo(const VkSubmitInfo* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSubmitInfo);
        structSize += pStruct->waitSemaphoreCount*sizeof(VkSemaphore);
        structSize += sizeof(VkPipelineStageFlags);
        structSize += pStruct->commandBufferCount*sizeof(VkCommandBuffer);
        structSize += pStruct->signalSemaphoreCount*sizeof(VkSemaphore);
    }
    return structSize;
}
size_t vk_size_vksubpassdependency(const VkSubpassDependency* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSubpassDependency);
    }
    return structSize;
}
size_t vk_size_vksubpassdescription(const VkSubpassDescription* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSubpassDescription);
        uint32_t i = 0;
        for (i = 0; i < pStruct->inputAttachmentCount; i++) {
            structSize += vk_size_vkattachmentreference(&pStruct->pInputAttachments[i]);
        }
        for (i = 0; i < pStruct->colorAttachmentCount; i++) {
            structSize += vk_size_vkattachmentreference(&pStruct->pColorAttachments[i]);
        }
        for (i = 0; i < pStruct->colorAttachmentCount; i++) {
            structSize += vk_size_vkattachmentreference(&pStruct->pResolveAttachments[i]);
        }
        structSize += vk_size_vkattachmentreference(pStruct->pDepthStencilAttachment);
        structSize += pStruct->preserveAttachmentCount*sizeof(uint32_t);
    }
    return structSize;
}
size_t vk_size_vksubresourcelayout(const VkSubresourceLayout* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSubresourceLayout);
    }
    return structSize;
}
size_t vk_size_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSurfaceCapabilitiesKHR);
    }
    return structSize;
}
size_t vk_size_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSurfaceFormatKHR);
    }
    return structSize;
}
size_t vk_size_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkSwapchainCreateInfoKHR);
        structSize += pStruct->queueFamilyIndexCount*sizeof(uint32_t);
    }
    return structSize;
}
size_t vk_size_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkValidationFlagsEXT);
        structSize += sizeof(VkValidationCheckEXT);
    }
    return structSize;
}
size_t vk_size_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkVertexInputAttributeDescription);
    }
    return structSize;
}
size_t vk_size_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkVertexInputBindingDescription);
    }
    return structSize;
}
size_t vk_size_vkviewport(const VkViewport* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkViewport);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
size_t vk_size_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkWaylandSurfaceCreateInfoKHR);
        structSize += sizeof(struct wl_display);
        structSize += sizeof(struct wl_surface);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkWin32KeyedMutexAcquireReleaseInfoNV);
        structSize += pStruct->acquireCount*sizeof(VkDeviceMemory);
        structSize += pStruct->acquireCount*sizeof(uint64_t);
        structSize += pStruct->acquireCount*sizeof(uint32_t);
        structSize += pStruct->releaseCount*sizeof(VkDeviceMemory);
        structSize += pStruct->releaseCount*sizeof(uint64_t);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkWin32SurfaceCreateInfoKHR);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_WIN32_KHR
size_t vk_size_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkWriteDescriptorSet);
        uint32_t i = 0;
        for (i = 0; i < pStruct->descriptorCount; i++) {
            structSize += vk_size_vkdescriptorimageinfo(&pStruct->pImageInfo[i]);
        }
        for (i = 0; i < pStruct->descriptorCount; i++) {
            structSize += vk_size_vkdescriptorbufferinfo(&pStruct->pBufferInfo[i]);
        }
        structSize += pStruct->descriptorCount*sizeof(VkBufferView);
    }
    return structSize;
}
#ifdef VK_USE_PLATFORM_XCB_KHR
size_t vk_size_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkXcbSurfaceCreateInfoKHR);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
size_t vk_size_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(VkXlibSurfaceCreateInfoKHR);
        structSize += sizeof(Display);
    }
    return structSize;
}
#endif //VK_USE_PLATFORM_XLIB_KHR
// CODEGEN : file ../vk_helper.py line #1442
size_t get_struct_chain_size(const void* pStruct)
{
    // Just use VkApplicationInfo as struct until actual type is resolved
    VkApplicationInfo* pNext = (VkApplicationInfo*)pStruct;
    size_t structSize = 0;
    while (pNext) {
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_APPLICATION_INFO:
            {
                structSize += vk_size_vkapplicationinfo((VkApplicationInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
            {
                structSize += vk_size_vkbindsparseinfo((VkBindSparseInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
            {
                structSize += vk_size_vkbuffercreateinfo((VkBufferCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            {
                structSize += vk_size_vkbuffermemorybarrier((VkBufferMemoryBarrier*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
            {
                structSize += vk_size_vkbufferviewcreateinfo((VkBufferViewCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO:
            {
                structSize += vk_size_vkcommandbufferallocateinfo((VkCommandBufferAllocateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO:
            {
                structSize += vk_size_vkcommandbufferbegininfo((VkCommandBufferBeginInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO:
            {
                structSize += vk_size_vkcommandbufferinheritanceinfo((VkCommandBufferInheritanceInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
            {
                structSize += vk_size_vkcommandpoolcreateinfo((VkCommandPoolCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            {
                structSize += vk_size_vkcomputepipelinecreateinfo((VkComputePipelineCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
            {
                structSize += vk_size_vkcopydescriptorset((VkCopyDescriptorSet*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO:
            {
                structSize += vk_size_vkdescriptorpoolcreateinfo((VkDescriptorPoolCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO:
            {
                structSize += vk_size_vkdescriptorsetallocateinfo((VkDescriptorSetAllocateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
            {
                structSize += vk_size_vkdescriptorsetlayoutcreateinfo((VkDescriptorSetLayoutCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
            {
                structSize += vk_size_vkdevicecreateinfo((VkDeviceCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO:
            {
                structSize += vk_size_vkdevicequeuecreateinfo((VkDeviceQueueCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            {
                structSize += vk_size_vkeventcreateinfo((VkEventCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            {
                structSize += vk_size_vkfencecreateinfo((VkFenceCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
            {
                structSize += vk_size_vkframebuffercreateinfo((VkFramebufferCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            {
                structSize += vk_size_vkgraphicspipelinecreateinfo((VkGraphicsPipelineCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            {
                structSize += vk_size_vkimagecreateinfo((VkImageCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            {
                structSize += vk_size_vkimagememorybarrier((VkImageMemoryBarrier*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
            {
                structSize += vk_size_vkimageviewcreateinfo((VkImageViewCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO:
            {
                structSize += vk_size_vkinstancecreateinfo((VkInstanceCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE:
            {
                structSize += vk_size_vkmappedmemoryrange((VkMappedMemoryRange*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO:
            {
                structSize += vk_size_vkmemoryallocateinfo((VkMemoryAllocateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
            {
                structSize += vk_size_vkmemorybarrier((VkMemoryBarrier*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinecachecreateinfo((VkPipelineCacheCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinecolorblendstatecreateinfo((VkPipelineColorBlendStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinedepthstencilstatecreateinfo((VkPipelineDepthStencilStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinedynamicstatecreateinfo((VkPipelineDynamicStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelineinputassemblystatecreateinfo((VkPipelineInputAssemblyStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinelayoutcreateinfo((VkPipelineLayoutCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinemultisamplestatecreateinfo((VkPipelineMultisampleStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinerasterizationstatecreateinfo((VkPipelineRasterizationStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelineshaderstagecreateinfo((VkPipelineShaderStageCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinetessellationstatecreateinfo((VkPipelineTessellationStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelinevertexinputstatecreateinfo((VkPipelineVertexInputStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
            {
                structSize += vk_size_vkpipelineviewportstatecreateinfo((VkPipelineViewportStateCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            {
                structSize += vk_size_vkquerypoolcreateinfo((VkQueryPoolCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO:
            {
                structSize += vk_size_vkrenderpassbegininfo((VkRenderPassBeginInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
            {
                structSize += vk_size_vkrenderpasscreateinfo((VkRenderPassCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
            {
                structSize += vk_size_vksamplercreateinfo((VkSamplerCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
            {
                structSize += vk_size_vksemaphorecreateinfo((VkSemaphoreCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
            {
                structSize += vk_size_vkshadermodulecreateinfo((VkShaderModuleCreateInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_SUBMIT_INFO:
            {
                structSize += vk_size_vksubmitinfo((VkSubmitInfo*)pNext);
                break;
            }
            case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
            {
                structSize += vk_size_vkwritedescriptorset((VkWriteDescriptorSet*)pNext);
                break;
            }
            default:
                assert(0);
                structSize += 0;
        }
        pNext = (VkApplicationInfo*)pNext->pNext;
    }
    return structSize;
}
// CODEGEN : file ../vk_helper.py line #1442
size_t get_dynamic_struct_size(const void* pStruct)
{
    // Just use VkApplicationInfo as struct until actual type is resolved
    VkApplicationInfo* pNext = (VkApplicationInfo*)pStruct;
    size_t structSize = 0;
    switch (pNext->sType) {
        case VK_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            structSize += vk_size_vkapplicationinfo((VkApplicationInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
        {
            structSize += vk_size_vkbindsparseinfo((VkBindSparseInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        {
            structSize += vk_size_vkbuffercreateinfo((VkBufferCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        {
            structSize += vk_size_vkbuffermemorybarrier((VkBufferMemoryBarrier*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        {
            structSize += vk_size_vkbufferviewcreateinfo((VkBufferViewCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO:
        {
            structSize += vk_size_vkcommandbufferallocateinfo((VkCommandBufferAllocateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO:
        {
            structSize += vk_size_vkcommandbufferbegininfo((VkCommandBufferBeginInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO:
        {
            structSize += vk_size_vkcommandbufferinheritanceinfo((VkCommandBufferInheritanceInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
        {
            structSize += vk_size_vkcommandpoolcreateinfo((VkCommandPoolCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            structSize += vk_size_vkcomputepipelinecreateinfo((VkComputePipelineCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        {
            structSize += vk_size_vkcopydescriptorset((VkCopyDescriptorSet*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO:
        {
            structSize += vk_size_vkdescriptorpoolcreateinfo((VkDescriptorPoolCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO:
        {
            structSize += vk_size_vkdescriptorsetallocateinfo((VkDescriptorSetAllocateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        {
            structSize += vk_size_vkdescriptorsetlayoutcreateinfo((VkDescriptorSetLayoutCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            structSize += vk_size_vkdevicecreateinfo((VkDeviceCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO:
        {
            structSize += vk_size_vkdevicequeuecreateinfo((VkDeviceQueueCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            structSize += vk_size_vkeventcreateinfo((VkEventCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            structSize += vk_size_vkfencecreateinfo((VkFenceCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        {
            structSize += vk_size_vkframebuffercreateinfo((VkFramebufferCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            structSize += vk_size_vkgraphicspipelinecreateinfo((VkGraphicsPipelineCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            structSize += vk_size_vkimagecreateinfo((VkImageCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        {
            structSize += vk_size_vkimagememorybarrier((VkImageMemoryBarrier*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            structSize += vk_size_vkimageviewcreateinfo((VkImageViewCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO:
        {
            structSize += vk_size_vkinstancecreateinfo((VkInstanceCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE:
        {
            structSize += vk_size_vkmappedmemoryrange((VkMappedMemoryRange*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO:
        {
            structSize += vk_size_vkmemoryallocateinfo((VkMemoryAllocateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
        {
            structSize += vk_size_vkmemorybarrier((VkMemoryBarrier*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinecachecreateinfo((VkPipelineCacheCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinecolorblendstatecreateinfo((VkPipelineColorBlendStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinedepthstencilstatecreateinfo((VkPipelineDepthStencilStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinedynamicstatecreateinfo((VkPipelineDynamicStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelineinputassemblystatecreateinfo((VkPipelineInputAssemblyStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinelayoutcreateinfo((VkPipelineLayoutCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinemultisamplestatecreateinfo((VkPipelineMultisampleStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinerasterizationstatecreateinfo((VkPipelineRasterizationStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelineshaderstagecreateinfo((VkPipelineShaderStageCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinetessellationstatecreateinfo((VkPipelineTessellationStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelinevertexinputstatecreateinfo((VkPipelineVertexInputStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
        {
            structSize += vk_size_vkpipelineviewportstatecreateinfo((VkPipelineViewportStateCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            structSize += vk_size_vkquerypoolcreateinfo((VkQueryPoolCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO:
        {
            structSize += vk_size_vkrenderpassbegininfo((VkRenderPassBeginInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        {
            structSize += vk_size_vkrenderpasscreateinfo((VkRenderPassCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            structSize += vk_size_vksamplercreateinfo((VkSamplerCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
        {
            structSize += vk_size_vksemaphorecreateinfo((VkSemaphoreCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
        {
            structSize += vk_size_vkshadermodulecreateinfo((VkShaderModuleCreateInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_SUBMIT_INFO:
        {
            structSize += vk_size_vksubmitinfo((VkSubmitInfo*)pNext);
            break;
        }
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        {
            structSize += vk_size_vkwritedescriptorset((VkWriteDescriptorSet*)pNext);
            break;
        }
        default:
            assert(0);
            structSize += 0;
    }
    return structSize;
}