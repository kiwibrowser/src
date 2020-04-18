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
#include <stdint.h>
#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include "vk_enum_string_helper.h"

// Function Prototypes
char* dynamic_display(const void* pStruct, const char* prefix);
char* vk_print_vkallocationcallbacks(const VkAllocationCallbacks* pStruct, const char* prefix);
char* vk_print_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkapplicationinfo(const VkApplicationInfo* pStruct, const char* prefix);
char* vk_print_vkattachmentdescription(const VkAttachmentDescription* pStruct, const char* prefix);
char* vk_print_vkattachmentreference(const VkAttachmentReference* pStruct, const char* prefix);
char* vk_print_vkbindsparseinfo(const VkBindSparseInfo* pStruct, const char* prefix);
char* vk_print_vkbuffercopy(const VkBufferCopy* pStruct, const char* prefix);
char* vk_print_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct, const char* prefix);
char* vk_print_vkbufferimagecopy(const VkBufferImageCopy* pStruct, const char* prefix);
char* vk_print_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct, const char* prefix);
char* vk_print_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct, const char* prefix);
char* vk_print_vkclearattachment(const VkClearAttachment* pStruct, const char* prefix);
char* vk_print_vkclearcolorvalue(const VkClearColorValue* pStruct, const char* prefix);
char* vk_print_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct, const char* prefix);
char* vk_print_vkclearrect(const VkClearRect* pStruct, const char* prefix);
char* vk_print_vkclearvalue(const VkClearValue* pStruct, const char* prefix);
char* vk_print_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct, const char* prefix);
char* vk_print_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct, const char* prefix);
char* vk_print_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct, const char* prefix);
char* vk_print_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct, const char* prefix);
char* vk_print_vkcomponentmapping(const VkComponentMapping* pStruct, const char* prefix);
char* vk_print_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct, const char* prefix);
char* vk_print_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct, const char* prefix);
char* vk_print_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct, const char* prefix);
char* vk_print_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct, const char* prefix);
char* vk_print_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct, const char* prefix);
char* vk_print_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct, const char* prefix);
char* vk_print_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct, const char* prefix);
char* vk_print_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct, const char* prefix);
char* vk_print_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct, const char* prefix);
char* vk_print_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct, const char* prefix);
char* vk_print_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct, const char* prefix);
char* vk_print_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct, const char* prefix);
char* vk_print_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct, const char* prefix);
char* vk_print_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct, const char* prefix);
char* vk_print_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct, const char* prefix);
char* vk_print_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct, const char* prefix);
char* vk_print_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct, const char* prefix);
char* vk_print_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct, const char* prefix);
char* vk_print_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct, const char* prefix);
char* vk_print_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct, const char* prefix);
char* vk_print_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct, const char* prefix);
char* vk_print_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct, const char* prefix);
char* vk_print_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct, const char* prefix);
char* vk_print_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct, const char* prefix);
char* vk_print_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct, const char* prefix);
char* vk_print_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct, const char* prefix);
char* vk_print_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct, const char* prefix);
char* vk_print_vkeventcreateinfo(const VkEventCreateInfo* pStruct, const char* prefix);
char* vk_print_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct, const char* prefix);
char* vk_print_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct, const char* prefix);
char* vk_print_vkextensionproperties(const VkExtensionProperties* pStruct, const char* prefix);
char* vk_print_vkextent2d(const VkExtent2D* pStruct, const char* prefix);
char* vk_print_vkextent3d(const VkExtent3D* pStruct, const char* prefix);
char* vk_print_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct, const char* prefix);
char* vk_print_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct, const char* prefix);
char* vk_print_vkfencecreateinfo(const VkFenceCreateInfo* pStruct, const char* prefix);
char* vk_print_vkformatproperties(const VkFormatProperties* pStruct, const char* prefix);
char* vk_print_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct, const char* prefix);
char* vk_print_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct, const char* prefix);
char* vk_print_vkimageblit(const VkImageBlit* pStruct, const char* prefix);
char* vk_print_vkimagecopy(const VkImageCopy* pStruct, const char* prefix);
char* vk_print_vkimagecreateinfo(const VkImageCreateInfo* pStruct, const char* prefix);
char* vk_print_vkimageformatproperties(const VkImageFormatProperties* pStruct, const char* prefix);
char* vk_print_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct, const char* prefix);
char* vk_print_vkimageresolve(const VkImageResolve* pStruct, const char* prefix);
char* vk_print_vkimagesubresource(const VkImageSubresource* pStruct, const char* prefix);
char* vk_print_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct, const char* prefix);
char* vk_print_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct, const char* prefix);
char* vk_print_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct, const char* prefix);
char* vk_print_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct, const char* prefix);
char* vk_print_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct, const char* prefix);
char* vk_print_vklayerproperties(const VkLayerProperties* pStruct, const char* prefix);
char* vk_print_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct, const char* prefix);
char* vk_print_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct, const char* prefix);
char* vk_print_vkmemorybarrier(const VkMemoryBarrier* pStruct, const char* prefix);
char* vk_print_vkmemoryheap(const VkMemoryHeap* pStruct, const char* prefix);
char* vk_print_vkmemoryrequirements(const VkMemoryRequirements* pStruct, const char* prefix);
char* vk_print_vkmemorytype(const VkMemoryType* pStruct, const char* prefix);
char* vk_print_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkoffset2d(const VkOffset2D* pStruct, const char* prefix);
char* vk_print_vkoffset3d(const VkOffset3D* pStruct, const char* prefix);
char* vk_print_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct, const char* prefix);
char* vk_print_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct, const char* prefix);
char* vk_print_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct, const char* prefix);
char* vk_print_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct, const char* prefix);
char* vk_print_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct, const char* prefix);
char* vk_print_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct, const char* prefix);
char* vk_print_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct, const char* prefix);
char* vk_print_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct, const char* prefix);
char* vk_print_vkpresentinfokhr(const VkPresentInfoKHR* pStruct, const char* prefix);
char* vk_print_vkpushconstantrange(const VkPushConstantRange* pStruct, const char* prefix);
char* vk_print_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct, const char* prefix);
char* vk_print_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct, const char* prefix);
char* vk_print_vkrect2d(const VkRect2D* pStruct, const char* prefix);
char* vk_print_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct, const char* prefix);
char* vk_print_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct, const char* prefix);
char* vk_print_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct, const char* prefix);
char* vk_print_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct, const char* prefix);
char* vk_print_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct, const char* prefix);
char* vk_print_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct, const char* prefix);
char* vk_print_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct, const char* prefix);
char* vk_print_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct, const char* prefix);
char* vk_print_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct, const char* prefix);
char* vk_print_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct, const char* prefix);
char* vk_print_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct, const char* prefix);
char* vk_print_vksparsememorybind(const VkSparseMemoryBind* pStruct, const char* prefix);
char* vk_print_vkspecializationinfo(const VkSpecializationInfo* pStruct, const char* prefix);
char* vk_print_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct, const char* prefix);
char* vk_print_vkstencilopstate(const VkStencilOpState* pStruct, const char* prefix);
char* vk_print_vksubmitinfo(const VkSubmitInfo* pStruct, const char* prefix);
char* vk_print_vksubpassdependency(const VkSubpassDependency* pStruct, const char* prefix);
char* vk_print_vksubpassdescription(const VkSubpassDescription* pStruct, const char* prefix);
char* vk_print_vksubresourcelayout(const VkSubresourceLayout* pStruct, const char* prefix);
char* vk_print_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct, const char* prefix);
char* vk_print_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct, const char* prefix);
char* vk_print_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct, const char* prefix);
char* vk_print_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct, const char* prefix);
char* vk_print_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct, const char* prefix);
char* vk_print_vkviewport(const VkViewport* pStruct, const char* prefix);
char* vk_print_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct, const char* prefix);
char* vk_print_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct, const char* prefix);
char* vk_print_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct, const char* prefix);
char* vk_print_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct, const char* prefix);

#if defined(_WIN32)
// Microsoft did not implement C99 in Visual Studio; but started adding it with
// VS2013.  However, VS2013 still did not have snprintf().  The following is a
// work-around.
#define snprintf _snprintf
#endif // _WIN32

char* vk_print_vkallocationcallbacks(const VkAllocationCallbacks* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%spUserData = 0x%p\n%spfnAllocation = 0x%p\n%spfnReallocation = 0x%p\n%spfnFree = 0x%p\n%spfnInternalAllocation = 0x%p\n%spfnInternalFree = 0x%p\n", prefix, (pStruct->pUserData), prefix, (void*)(pStruct->pfnAllocation), prefix, (void*)(pStruct->pfnReallocation), prefix, (void*)(pStruct->pfnFree), prefix, (void*)(pStruct->pfnInternalAllocation), prefix, (void*)(pStruct->pfnInternalFree));
    return str;
}
char* vk_print_vkandroidsurfacecreateinfokhr(const VkAndroidSurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%swindow = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->window));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkapplicationinfo(const VkApplicationInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%spApplicationName = 0x%p\n%sapplicationVersion = %u\n%spEngineName = 0x%p\n%sengineVersion = %u\n%sapiVersion = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->pApplicationName), prefix, (pStruct->applicationVersion), prefix, (pStruct->pEngineName), prefix, (pStruct->engineVersion), prefix, (pStruct->apiVersion));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkattachmentdescription(const VkAttachmentDescription* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sflags = %u\n%sformat = %s\n%ssamples = %s\n%sloadOp = %s\n%sstoreOp = %s\n%sstencilLoadOp = %s\n%sstencilStoreOp = %s\n%sinitialLayout = %s\n%sfinalLayout = %s\n", prefix, (pStruct->flags), prefix, string_VkFormat(pStruct->format), prefix, string_VkSampleCountFlagBits(pStruct->samples), prefix, string_VkAttachmentLoadOp(pStruct->loadOp), prefix, string_VkAttachmentStoreOp(pStruct->storeOp), prefix, string_VkAttachmentLoadOp(pStruct->stencilLoadOp), prefix, string_VkAttachmentStoreOp(pStruct->stencilStoreOp), prefix, string_VkImageLayout(pStruct->initialLayout), prefix, string_VkImageLayout(pStruct->finalLayout));
    return str;
}
char* vk_print_vkattachmentreference(const VkAttachmentReference* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sattachment = %u\n%slayout = %s\n", prefix, (pStruct->attachment), prefix, string_VkImageLayout(pStruct->layout));
    return str;
}
char* vk_print_vkbindsparseinfo(const VkBindSparseInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[4];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pBufferBinds) {
        tmpStr = vk_print_vksparsebuffermemorybindinfo(pStruct->pBufferBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spBufferBinds (0x%p)\n%s", prefix, (void*)pStruct->pBufferBinds, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pImageOpaqueBinds) {
        tmpStr = vk_print_vksparseimageopaquememorybindinfo(pStruct->pImageOpaqueBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spImageOpaqueBinds (0x%p)\n%s", prefix, (void*)pStruct->pImageOpaqueBinds, tmpStr);
    }
    else
        stp_strs[2] = "";
    if (pStruct->pImageBinds) {
        tmpStr = vk_print_vksparseimagememorybindinfo(pStruct->pImageBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[3] = (char*)malloc(len);
        snprintf(stp_strs[3], len, " %spImageBinds (0x%p)\n%s", prefix, (void*)pStruct->pImageBinds, tmpStr);
    }
    else
        stp_strs[3] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%swaitSemaphoreCount = %u\n%spWaitSemaphores = 0x%p\n%sbufferBindCount = %u\n%spBufferBinds = 0x%p\n%simageOpaqueBindCount = %u\n%spImageOpaqueBinds = 0x%p\n%simageBindCount = %u\n%spImageBinds = 0x%p\n%ssignalSemaphoreCount = %u\n%spSignalSemaphores = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->waitSemaphoreCount), prefix, (void*)(pStruct->pWaitSemaphores), prefix, (pStruct->bufferBindCount), prefix, (void*)(pStruct->pBufferBinds), prefix, (pStruct->imageOpaqueBindCount), prefix, (void*)(pStruct->pImageOpaqueBinds), prefix, (pStruct->imageBindCount), prefix, (void*)(pStruct->pImageBinds), prefix, (pStruct->signalSemaphoreCount), prefix, (void*)(pStruct->pSignalSemaphores));
    for (int32_t stp_index = 3; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkbuffercopy(const VkBufferCopy* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcOffset = " PRINTF_SIZE_T_SPECIFIER "\n%sdstOffset = " PRINTF_SIZE_T_SPECIFIER "\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (pStruct->srcOffset), prefix, (pStruct->dstOffset), prefix, (pStruct->size));
    return str;
}
char* vk_print_vkbuffercreateinfo(const VkBufferCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n%susage = %u\n%ssharingMode = %s\n%squeueFamilyIndexCount = %u\n%spQueueFamilyIndices = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->size), prefix, (pStruct->usage), prefix, string_VkSharingMode(pStruct->sharingMode), prefix, (pStruct->queueFamilyIndexCount), prefix, (void*)(pStruct->pQueueFamilyIndices));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkbufferimagecopy(const VkBufferImageCopy* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->imageSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %simageSubresource (0x%p)\n%s", prefix, (void*)&pStruct->imageSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->imageOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %simageOffset (0x%p)\n%s", prefix, (void*)&pStruct->imageOffset, tmpStr);
    tmpStr = vk_print_vkextent3d(&pStruct->imageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %simageExtent (0x%p)\n%s", prefix, (void*)&pStruct->imageExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbufferOffset = " PRINTF_SIZE_T_SPECIFIER "\n%sbufferRowLength = %u\n%sbufferImageHeight = %u\n%simageSubresource = 0x%p\n%simageOffset = 0x%p\n%simageExtent = 0x%p\n", prefix, (pStruct->bufferOffset), prefix, (pStruct->bufferRowLength), prefix, (pStruct->bufferImageHeight), prefix, (void*)&(pStruct->imageSubresource), prefix, (void*)&(pStruct->imageOffset), prefix, (void*)&(pStruct->imageExtent));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkbuffermemorybarrier(const VkBufferMemoryBarrier* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%ssrcAccessMask = %u\n%sdstAccessMask = %u\n%ssrcQueueFamilyIndex = %u\n%sdstQueueFamilyIndex = %u\n%sbuffer = 0x%p\n%soffset = " PRINTF_SIZE_T_SPECIFIER "\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->srcAccessMask), prefix, (pStruct->dstAccessMask), prefix, (pStruct->srcQueueFamilyIndex), prefix, (pStruct->dstQueueFamilyIndex), prefix, (void*)(pStruct->buffer), prefix, (pStruct->offset), prefix, (pStruct->size));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkbufferviewcreateinfo(const VkBufferViewCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sbuffer = 0x%p\n%sformat = %s\n%soffset = " PRINTF_SIZE_T_SPECIFIER "\n%srange = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->buffer), prefix, string_VkFormat(pStruct->format), prefix, (pStruct->offset), prefix, (pStruct->range));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkclearattachment(const VkClearAttachment* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkclearvalue(&pStruct->clearValue, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sclearValue (0x%p)\n%s", prefix, (void*)&pStruct->clearValue, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspectMask = %u\n%scolorAttachment = %u\n%sclearValue = 0x%p\n", prefix, (pStruct->aspectMask), prefix, (pStruct->colorAttachment), prefix, (void*)&(pStruct->clearValue));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkclearcolorvalue(const VkClearColorValue* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sfloat32 = 0x%p\n%sint32 = 0x%p\n%suint32 = 0x%p\n", prefix, (void*)(pStruct->float32), prefix, (void*)(pStruct->int32), prefix, (void*)(pStruct->uint32));
    return str;
}
char* vk_print_vkcleardepthstencilvalue(const VkClearDepthStencilValue* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sdepth = %f\n%sstencil = %u\n", prefix, (pStruct->depth), prefix, (pStruct->stencil));
    return str;
}
char* vk_print_vkclearrect(const VkClearRect* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkrect2d(&pStruct->rect, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %srect (0x%p)\n%s", prefix, (void*)&pStruct->rect, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%srect = 0x%p\n%sbaseArrayLayer = %u\n%slayerCount = %u\n", prefix, (void*)&(pStruct->rect), prefix, (pStruct->baseArrayLayer), prefix, (pStruct->layerCount));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkclearvalue(const VkClearValue* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = vk_print_vkclearcolorvalue(&pStruct->color, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %scolor (0x%p)\n%s", prefix, (void*)&pStruct->color, tmpStr);
    tmpStr = vk_print_vkcleardepthstencilvalue(&pStruct->depthStencil, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sdepthStencil (0x%p)\n%s", prefix, (void*)&pStruct->depthStencil, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%scolor = 0x%p\n%sdepthStencil = 0x%p\n", prefix, (void*)&(pStruct->color), prefix, (void*)&(pStruct->depthStencil));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcommandbufferallocateinfo(const VkCommandBufferAllocateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%scommandPool = 0x%p\n%slevel = %s\n%scommandBufferCount = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->commandPool), prefix, string_VkCommandBufferLevel(pStruct->level), prefix, (pStruct->commandBufferCount));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcommandbufferbegininfo(const VkCommandBufferBeginInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pInheritanceInfo) {
        tmpStr = vk_print_vkcommandbufferinheritanceinfo(pStruct->pInheritanceInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spInheritanceInfo (0x%p)\n%s", prefix, (void*)pStruct->pInheritanceInfo, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%spInheritanceInfo = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->pInheritanceInfo));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcommandbufferinheritanceinfo(const VkCommandBufferInheritanceInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%srenderPass = 0x%p\n%ssubpass = %u\n%sframebuffer = 0x%p\n%socclusionQueryEnable = %s\n%squeryFlags = %u\n%spipelineStatistics = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->renderPass), prefix, (pStruct->subpass), prefix, (void*)(pStruct->framebuffer), prefix, (pStruct->occlusionQueryEnable) ? "TRUE" : "FALSE", prefix, (pStruct->queryFlags), prefix, (pStruct->pipelineStatistics));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcommandpoolcreateinfo(const VkCommandPoolCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%squeueFamilyIndex = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->queueFamilyIndex));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcomponentmapping(const VkComponentMapping* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sr = %s\n%sg = %s\n%sb = %s\n%sa = %s\n", prefix, string_VkComponentSwizzle(pStruct->r), prefix, string_VkComponentSwizzle(pStruct->g), prefix, string_VkComponentSwizzle(pStruct->b), prefix, string_VkComponentSwizzle(pStruct->a));
    return str;
}
char* vk_print_vkcomputepipelinecreateinfo(const VkComputePipelineCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkpipelineshaderstagecreateinfo(&pStruct->stage, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sstage (0x%p)\n%s", prefix, (void*)&pStruct->stage, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sstage = 0x%p\n%slayout = 0x%p\n%sbasePipelineHandle = 0x%p\n%sbasePipelineIndex = %i\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)&(pStruct->stage), prefix, (void*)(pStruct->layout), prefix, (void*)(pStruct->basePipelineHandle), prefix, (pStruct->basePipelineIndex));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkcopydescriptorset(const VkCopyDescriptorSet* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%ssrcSet = 0x%p\n%ssrcBinding = %u\n%ssrcArrayElement = %u\n%sdstSet = 0x%p\n%sdstBinding = %u\n%sdstArrayElement = %u\n%sdescriptorCount = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->srcSet), prefix, (pStruct->srcBinding), prefix, (pStruct->srcArrayElement), prefix, (void*)(pStruct->dstSet), prefix, (pStruct->dstBinding), prefix, (pStruct->dstArrayElement), prefix, (pStruct->descriptorCount));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdebugmarkermarkerinfoext(const VkDebugMarkerMarkerInfoEXT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%spMarkerName = 0x%p\n%scolor = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->pMarkerName), prefix, (void*)(pStruct->color));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdebugmarkerobjectnameinfoext(const VkDebugMarkerObjectNameInfoEXT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sobjectType = %s\n%sobject = %" PRId64 "\n%spObjectName = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, string_VkDebugReportObjectTypeEXT(pStruct->objectType), prefix, (pStruct->object), prefix, (pStruct->pObjectName));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdebugmarkerobjecttaginfoext(const VkDebugMarkerObjectTagInfoEXT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sobjectType = %s\n%sobject = %" PRId64 "\n%stagName = %" PRId64 "\n%stagSize = " PRINTF_SIZE_T_SPECIFIER "\n%spTag = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, string_VkDebugReportObjectTypeEXT(pStruct->objectType), prefix, (pStruct->object), prefix, (pStruct->tagName), prefix, (pStruct->tagSize), prefix, (pStruct->pTag));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdebugreportcallbackcreateinfoext(const VkDebugReportCallbackCreateInfoEXT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%spfnCallback = 0x%p\n%spUserData = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->pfnCallback), prefix, (pStruct->pUserData));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdedicatedallocationbuffercreateinfonv(const VkDedicatedAllocationBufferCreateInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sdedicatedAllocation = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->dedicatedAllocation) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdedicatedallocationimagecreateinfonv(const VkDedicatedAllocationImageCreateInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sdedicatedAllocation = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->dedicatedAllocation) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdedicatedallocationmemoryallocateinfonv(const VkDedicatedAllocationMemoryAllocateInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%simage = 0x%p\n%sbuffer = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, (void*)(pStruct->buffer));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdescriptorbufferinfo(const VkDescriptorBufferInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbuffer = 0x%p\n%soffset = " PRINTF_SIZE_T_SPECIFIER "\n%srange = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (void*)(pStruct->buffer), prefix, (pStruct->offset), prefix, (pStruct->range));
    return str;
}
char* vk_print_vkdescriptorimageinfo(const VkDescriptorImageInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssampler = 0x%p\n%simageView = 0x%p\n%simageLayout = %s\n", prefix, (void*)(pStruct->sampler), prefix, (void*)(pStruct->imageView), prefix, string_VkImageLayout(pStruct->imageLayout));
    return str;
}
char* vk_print_vkdescriptorpoolcreateinfo(const VkDescriptorPoolCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pPoolSizes) {
        tmpStr = vk_print_vkdescriptorpoolsize(pStruct->pPoolSizes, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spPoolSizes (0x%p)\n%s", prefix, (void*)pStruct->pPoolSizes, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%smaxSets = %u\n%spoolSizeCount = %u\n%spPoolSizes = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->maxSets), prefix, (pStruct->poolSizeCount), prefix, (void*)(pStruct->pPoolSizes));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdescriptorpoolsize(const VkDescriptorPoolSize* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%stype = %s\n%sdescriptorCount = %u\n", prefix, string_VkDescriptorType(pStruct->type), prefix, (pStruct->descriptorCount));
    return str;
}
char* vk_print_vkdescriptorsetallocateinfo(const VkDescriptorSetAllocateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sdescriptorPool = 0x%p\n%sdescriptorSetCount = %u\n%spSetLayouts = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->descriptorPool), prefix, (pStruct->descriptorSetCount), prefix, (void*)(pStruct->pSetLayouts));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdescriptorsetlayoutbinding(const VkDescriptorSetLayoutBinding* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbinding = %u\n%sdescriptorType = %s\n%sdescriptorCount = %u\n%sstageFlags = %u\n%spImmutableSamplers = 0x%p\n", prefix, (pStruct->binding), prefix, string_VkDescriptorType(pStruct->descriptorType), prefix, (pStruct->descriptorCount), prefix, (pStruct->stageFlags), prefix, (void*)(pStruct->pImmutableSamplers));
    return str;
}
char* vk_print_vkdescriptorsetlayoutcreateinfo(const VkDescriptorSetLayoutCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pBindings) {
        tmpStr = vk_print_vkdescriptorsetlayoutbinding(pStruct->pBindings, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spBindings (0x%p)\n%s", prefix, (void*)pStruct->pBindings, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sbindingCount = %u\n%spBindings = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->bindingCount), prefix, (void*)(pStruct->pBindings));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdevicecreateinfo(const VkDeviceCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pQueueCreateInfos) {
        tmpStr = vk_print_vkdevicequeuecreateinfo(pStruct->pQueueCreateInfos, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spQueueCreateInfos (0x%p)\n%s", prefix, (void*)pStruct->pQueueCreateInfos, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pEnabledFeatures) {
        tmpStr = vk_print_vkphysicaldevicefeatures(pStruct->pEnabledFeatures, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spEnabledFeatures (0x%p)\n%s", prefix, (void*)pStruct->pEnabledFeatures, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%squeueCreateInfoCount = %u\n%spQueueCreateInfos = 0x%p\n%senabledLayerCount = %u\n%sppEnabledLayerNames = %s\n%senabledExtensionCount = %u\n%sppEnabledExtensionNames = %s\n%spEnabledFeatures = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->queueCreateInfoCount), prefix, (void*)(pStruct->pQueueCreateInfos), prefix, (pStruct->enabledLayerCount), prefix, (pStruct->ppEnabledLayerNames)[0], prefix, (pStruct->enabledExtensionCount), prefix, (pStruct->ppEnabledExtensionNames)[0], prefix, (void*)(pStruct->pEnabledFeatures));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdevicequeuecreateinfo(const VkDeviceQueueCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%squeueFamilyIndex = %u\n%squeueCount = %u\n%spQueuePriorities = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->queueFamilyIndex), prefix, (pStruct->queueCount), prefix, (void*)(pStruct->pQueuePriorities));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdispatchindirectcommand(const VkDispatchIndirectCommand* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %u\n%sy = %u\n%sz = %u\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
char* vk_print_vkdisplaymodecreateinfokhr(const VkDisplayModeCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkdisplaymodeparameterskhr(&pStruct->parameters, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sparameters (0x%p)\n%s", prefix, (void*)&pStruct->parameters, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sparameters = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)&(pStruct->parameters));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplaymodeparameterskhr(const VkDisplayModeParametersKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkextent2d(&pStruct->visibleRegion, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %svisibleRegion (0x%p)\n%s", prefix, (void*)&pStruct->visibleRegion, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%svisibleRegion = 0x%p\n%srefreshRate = %u\n", prefix, (void*)&(pStruct->visibleRegion), prefix, (pStruct->refreshRate));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplaymodepropertieskhr(const VkDisplayModePropertiesKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkdisplaymodeparameterskhr(&pStruct->parameters, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sparameters (0x%p)\n%s", prefix, (void*)&pStruct->parameters, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sdisplayMode = 0x%p\n%sparameters = 0x%p\n", prefix, (void*)(pStruct->displayMode), prefix, (void*)&(pStruct->parameters));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplayplanecapabilitieskhr(const VkDisplayPlaneCapabilitiesKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[8];
    tmpStr = vk_print_vkoffset2d(&pStruct->minSrcPosition, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sminSrcPosition (0x%p)\n%s", prefix, (void*)&pStruct->minSrcPosition, tmpStr);
    tmpStr = vk_print_vkoffset2d(&pStruct->maxSrcPosition, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %smaxSrcPosition (0x%p)\n%s", prefix, (void*)&pStruct->maxSrcPosition, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->minSrcExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sminSrcExtent (0x%p)\n%s", prefix, (void*)&pStruct->minSrcExtent, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->maxSrcExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %smaxSrcExtent (0x%p)\n%s", prefix, (void*)&pStruct->maxSrcExtent, tmpStr);
    tmpStr = vk_print_vkoffset2d(&pStruct->minDstPosition, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[4] = (char*)malloc(len);
    snprintf(stp_strs[4], len, " %sminDstPosition (0x%p)\n%s", prefix, (void*)&pStruct->minDstPosition, tmpStr);
    tmpStr = vk_print_vkoffset2d(&pStruct->maxDstPosition, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[5] = (char*)malloc(len);
    snprintf(stp_strs[5], len, " %smaxDstPosition (0x%p)\n%s", prefix, (void*)&pStruct->maxDstPosition, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->minDstExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[6] = (char*)malloc(len);
    snprintf(stp_strs[6], len, " %sminDstExtent (0x%p)\n%s", prefix, (void*)&pStruct->minDstExtent, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->maxDstExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[7] = (char*)malloc(len);
    snprintf(stp_strs[7], len, " %smaxDstExtent (0x%p)\n%s", prefix, (void*)&pStruct->maxDstExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + strlen(stp_strs[5]) + strlen(stp_strs[6]) + strlen(stp_strs[7]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssupportedAlpha = %u\n%sminSrcPosition = 0x%p\n%smaxSrcPosition = 0x%p\n%sminSrcExtent = 0x%p\n%smaxSrcExtent = 0x%p\n%sminDstPosition = 0x%p\n%smaxDstPosition = 0x%p\n%sminDstExtent = 0x%p\n%smaxDstExtent = 0x%p\n", prefix, (pStruct->supportedAlpha), prefix, (void*)&(pStruct->minSrcPosition), prefix, (void*)&(pStruct->maxSrcPosition), prefix, (void*)&(pStruct->minSrcExtent), prefix, (void*)&(pStruct->maxSrcExtent), prefix, (void*)&(pStruct->minDstPosition), prefix, (void*)&(pStruct->maxDstPosition), prefix, (void*)&(pStruct->minDstExtent), prefix, (void*)&(pStruct->maxDstExtent));
    for (int32_t stp_index = 7; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplayplanepropertieskhr(const VkDisplayPlanePropertiesKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%scurrentDisplay = 0x%p\n%scurrentStackIndex = %u\n", prefix, (void*)(pStruct->currentDisplay), prefix, (pStruct->currentStackIndex));
    return str;
}
char* vk_print_vkdisplaypresentinfokhr(const VkDisplayPresentInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkrect2d(&pStruct->srcRect, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcRect (0x%p)\n%s", prefix, (void*)&pStruct->srcRect, tmpStr);
    tmpStr = vk_print_vkrect2d(&pStruct->dstRect, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdstRect (0x%p)\n%s", prefix, (void*)&pStruct->dstRect, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%ssrcRect = 0x%p\n%sdstRect = 0x%p\n%spersistent = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->srcRect), prefix, (void*)&(pStruct->dstRect), prefix, (pStruct->persistent) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplaypropertieskhr(const VkDisplayPropertiesKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = vk_print_vkextent2d(&pStruct->physicalDimensions, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sphysicalDimensions (0x%p)\n%s", prefix, (void*)&pStruct->physicalDimensions, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->physicalResolution, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sphysicalResolution (0x%p)\n%s", prefix, (void*)&pStruct->physicalResolution, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sdisplay = 0x%p\n%sdisplayName = 0x%p\n%sphysicalDimensions = 0x%p\n%sphysicalResolution = 0x%p\n%ssupportedTransforms = %u\n%splaneReorderPossible = %s\n%spersistentContent = %s\n", prefix, (void*)(pStruct->display), prefix, (pStruct->displayName), prefix, (void*)&(pStruct->physicalDimensions), prefix, (void*)&(pStruct->physicalResolution), prefix, (pStruct->supportedTransforms), prefix, (pStruct->planeReorderPossible) ? "TRUE" : "FALSE", prefix, (pStruct->persistentContent) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdisplaysurfacecreateinfokhr(const VkDisplaySurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkextent2d(&pStruct->imageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %simageExtent (0x%p)\n%s", prefix, (void*)&pStruct->imageExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdisplayMode = 0x%p\n%splaneIndex = %u\n%splaneStackIndex = %u\n%stransform = %s\n%sglobalAlpha = %f\n%salphaMode = %s\n%simageExtent = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->displayMode), prefix, (pStruct->planeIndex), prefix, (pStruct->planeStackIndex), prefix, string_VkSurfaceTransformFlagBitsKHR(pStruct->transform), prefix, (pStruct->globalAlpha), prefix, string_VkDisplayPlaneAlphaFlagBitsKHR(pStruct->alphaMode), prefix, (void*)&(pStruct->imageExtent));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkdrawindexedindirectcommand(const VkDrawIndexedIndirectCommand* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sindexCount = %u\n%sinstanceCount = %u\n%sfirstIndex = %u\n%svertexOffset = %i\n%sfirstInstance = %u\n", prefix, (pStruct->indexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstIndex), prefix, (pStruct->vertexOffset), prefix, (pStruct->firstInstance));
    return str;
}
char* vk_print_vkdrawindirectcommand(const VkDrawIndirectCommand* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%svertexCount = %u\n%sinstanceCount = %u\n%sfirstVertex = %u\n%sfirstInstance = %u\n", prefix, (pStruct->vertexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstVertex), prefix, (pStruct->firstInstance));
    return str;
}
char* vk_print_vkeventcreateinfo(const VkEventCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkexportmemoryallocateinfonv(const VkExportMemoryAllocateInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%shandleTypes = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->handleTypes));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkexportmemorywin32handleinfonv(const VkExportMemoryWin32HandleInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%spAttributes = 0x%p\n%sdwAccess = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->pAttributes), prefix, (void*)(pStruct->dwAccess));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkextensionproperties(const VkExtensionProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sextensionName = %s\n%sspecVersion = %u\n", prefix, (pStruct->extensionName), prefix, (pStruct->specVersion));
    return str;
}
char* vk_print_vkextent2d(const VkExtent2D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%swidth = %u\n%sheight = %u\n", prefix, (pStruct->width), prefix, (pStruct->height));
    return str;
}
char* vk_print_vkextent3d(const VkExtent3D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%swidth = %u\n%sheight = %u\n%sdepth = %u\n", prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->depth));
    return str;
}
char* vk_print_vkexternalimageformatpropertiesnv(const VkExternalImageFormatPropertiesNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkimageformatproperties(&pStruct->imageFormatProperties, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %simageFormatProperties (0x%p)\n%s", prefix, (void*)&pStruct->imageFormatProperties, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%simageFormatProperties = 0x%p\n%sexternalMemoryFeatures = %u\n%sexportFromImportedHandleTypes = %u\n%scompatibleHandleTypes = %u\n", prefix, (void*)&(pStruct->imageFormatProperties), prefix, (pStruct->externalMemoryFeatures), prefix, (pStruct->exportFromImportedHandleTypes), prefix, (pStruct->compatibleHandleTypes));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkexternalmemoryimagecreateinfonv(const VkExternalMemoryImageCreateInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%shandleTypes = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->handleTypes));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkfencecreateinfo(const VkFenceCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkformatproperties(const VkFormatProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%slinearTilingFeatures = %u\n%soptimalTilingFeatures = %u\n%sbufferFeatures = %u\n", prefix, (pStruct->linearTilingFeatures), prefix, (pStruct->optimalTilingFeatures), prefix, (pStruct->bufferFeatures));
    return str;
}
char* vk_print_vkframebuffercreateinfo(const VkFramebufferCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%srenderPass = 0x%p\n%sattachmentCount = %u\n%spAttachments = 0x%p\n%swidth = %u\n%sheight = %u\n%slayers = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->renderPass), prefix, (pStruct->attachmentCount), prefix, (void*)(pStruct->pAttachments), prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->layers));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkgraphicspipelinecreateinfo(const VkGraphicsPipelineCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[11];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pStages) {
        tmpStr = vk_print_vkpipelineshaderstagecreateinfo(pStruct->pStages, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spStages (0x%p)\n%s", prefix, (void*)pStruct->pStages, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pVertexInputState) {
        tmpStr = vk_print_vkpipelinevertexinputstatecreateinfo(pStruct->pVertexInputState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spVertexInputState (0x%p)\n%s", prefix, (void*)pStruct->pVertexInputState, tmpStr);
    }
    else
        stp_strs[2] = "";
    if (pStruct->pInputAssemblyState) {
        tmpStr = vk_print_vkpipelineinputassemblystatecreateinfo(pStruct->pInputAssemblyState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[3] = (char*)malloc(len);
        snprintf(stp_strs[3], len, " %spInputAssemblyState (0x%p)\n%s", prefix, (void*)pStruct->pInputAssemblyState, tmpStr);
    }
    else
        stp_strs[3] = "";
    if (pStruct->pTessellationState) {
        tmpStr = vk_print_vkpipelinetessellationstatecreateinfo(pStruct->pTessellationState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[4] = (char*)malloc(len);
        snprintf(stp_strs[4], len, " %spTessellationState (0x%p)\n%s", prefix, (void*)pStruct->pTessellationState, tmpStr);
    }
    else
        stp_strs[4] = "";
    if (pStruct->pViewportState) {
        tmpStr = vk_print_vkpipelineviewportstatecreateinfo(pStruct->pViewportState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[5] = (char*)malloc(len);
        snprintf(stp_strs[5], len, " %spViewportState (0x%p)\n%s", prefix, (void*)pStruct->pViewportState, tmpStr);
    }
    else
        stp_strs[5] = "";
    if (pStruct->pRasterizationState) {
        tmpStr = vk_print_vkpipelinerasterizationstatecreateinfo(pStruct->pRasterizationState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[6] = (char*)malloc(len);
        snprintf(stp_strs[6], len, " %spRasterizationState (0x%p)\n%s", prefix, (void*)pStruct->pRasterizationState, tmpStr);
    }
    else
        stp_strs[6] = "";
    if (pStruct->pMultisampleState) {
        tmpStr = vk_print_vkpipelinemultisamplestatecreateinfo(pStruct->pMultisampleState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[7] = (char*)malloc(len);
        snprintf(stp_strs[7], len, " %spMultisampleState (0x%p)\n%s", prefix, (void*)pStruct->pMultisampleState, tmpStr);
    }
    else
        stp_strs[7] = "";
    if (pStruct->pDepthStencilState) {
        tmpStr = vk_print_vkpipelinedepthstencilstatecreateinfo(pStruct->pDepthStencilState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[8] = (char*)malloc(len);
        snprintf(stp_strs[8], len, " %spDepthStencilState (0x%p)\n%s", prefix, (void*)pStruct->pDepthStencilState, tmpStr);
    }
    else
        stp_strs[8] = "";
    if (pStruct->pColorBlendState) {
        tmpStr = vk_print_vkpipelinecolorblendstatecreateinfo(pStruct->pColorBlendState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[9] = (char*)malloc(len);
        snprintf(stp_strs[9], len, " %spColorBlendState (0x%p)\n%s", prefix, (void*)pStruct->pColorBlendState, tmpStr);
    }
    else
        stp_strs[9] = "";
    if (pStruct->pDynamicState) {
        tmpStr = vk_print_vkpipelinedynamicstatecreateinfo(pStruct->pDynamicState, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[10] = (char*)malloc(len);
        snprintf(stp_strs[10], len, " %spDynamicState (0x%p)\n%s", prefix, (void*)pStruct->pDynamicState, tmpStr);
    }
    else
        stp_strs[10] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + strlen(stp_strs[5]) + strlen(stp_strs[6]) + strlen(stp_strs[7]) + strlen(stp_strs[8]) + strlen(stp_strs[9]) + strlen(stp_strs[10]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sstageCount = %u\n%spStages = 0x%p\n%spVertexInputState = 0x%p\n%spInputAssemblyState = 0x%p\n%spTessellationState = 0x%p\n%spViewportState = 0x%p\n%spRasterizationState = 0x%p\n%spMultisampleState = 0x%p\n%spDepthStencilState = 0x%p\n%spColorBlendState = 0x%p\n%spDynamicState = 0x%p\n%slayout = 0x%p\n%srenderPass = 0x%p\n%ssubpass = %u\n%sbasePipelineHandle = 0x%p\n%sbasePipelineIndex = %i\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->stageCount), prefix, (void*)(pStruct->pStages), prefix, (void*)(pStruct->pVertexInputState), prefix, (void*)(pStruct->pInputAssemblyState), prefix, (void*)(pStruct->pTessellationState), prefix, (void*)(pStruct->pViewportState), prefix, (void*)(pStruct->pRasterizationState), prefix, (void*)(pStruct->pMultisampleState), prefix, (void*)(pStruct->pDepthStencilState), prefix, (void*)(pStruct->pColorBlendState), prefix, (void*)(pStruct->pDynamicState), prefix, (void*)(pStruct->layout), prefix, (void*)(pStruct->renderPass), prefix, (pStruct->subpass), prefix, (void*)(pStruct->basePipelineHandle), prefix, (pStruct->basePipelineIndex));
    for (int32_t stp_index = 10; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimageblit(const VkImageBlit* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[4];
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssrcSubresource (0x%p)\n%s", prefix, (void*)&pStruct->srcSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->srcOffsets[0], extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcOffsets[0] (0x%p)\n%s", prefix, (void*)&pStruct->srcOffsets[0], tmpStr);
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdstSubresource (0x%p)\n%s", prefix, (void*)&pStruct->dstSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->dstOffsets[0], extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %sdstOffsets[0] (0x%p)\n%s", prefix, (void*)&pStruct->dstOffsets[0], tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubresource = 0x%p\n%ssrcOffsets = 0x%p\n%sdstSubresource = 0x%p\n%sdstOffsets = 0x%p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)(pStruct->srcOffsets), prefix, (void*)&(pStruct->dstSubresource), prefix, (void*)(pStruct->dstOffsets));
    for (int32_t stp_index = 3; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimagecopy(const VkImageCopy* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[5];
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssrcSubresource (0x%p)\n%s", prefix, (void*)&pStruct->srcSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->srcOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcOffset (0x%p)\n%s", prefix, (void*)&pStruct->srcOffset, tmpStr);
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdstSubresource (0x%p)\n%s", prefix, (void*)&pStruct->dstSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->dstOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %sdstOffset (0x%p)\n%s", prefix, (void*)&pStruct->dstOffset, tmpStr);
    tmpStr = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[4] = (char*)malloc(len);
    snprintf(stp_strs[4], len, " %sextent (0x%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubresource = 0x%p\n%ssrcOffset = 0x%p\n%sdstSubresource = 0x%p\n%sdstOffset = 0x%p\n%sextent = 0x%p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->dstSubresource), prefix, (void*)&(pStruct->dstOffset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 4; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimagecreateinfo(const VkImageCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sextent (0x%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%simageType = %s\n%sformat = %s\n%sextent = 0x%p\n%smipLevels = %u\n%sarrayLayers = %u\n%ssamples = %s\n%stiling = %s\n%susage = %u\n%ssharingMode = %s\n%squeueFamilyIndexCount = %u\n%spQueueFamilyIndices = 0x%p\n%sinitialLayout = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkImageType(pStruct->imageType), prefix, string_VkFormat(pStruct->format), prefix, (void*)&(pStruct->extent), prefix, (pStruct->mipLevels), prefix, (pStruct->arrayLayers), prefix, string_VkSampleCountFlagBits(pStruct->samples), prefix, string_VkImageTiling(pStruct->tiling), prefix, (pStruct->usage), prefix, string_VkSharingMode(pStruct->sharingMode), prefix, (pStruct->queueFamilyIndexCount), prefix, (void*)(pStruct->pQueueFamilyIndices), prefix, string_VkImageLayout(pStruct->initialLayout));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimageformatproperties(const VkImageFormatProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkextent3d(&pStruct->maxExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %smaxExtent (0x%p)\n%s", prefix, (void*)&pStruct->maxExtent, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smaxExtent = 0x%p\n%smaxMipLevels = %u\n%smaxArrayLayers = %u\n%ssampleCounts = %u\n%smaxResourceSize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (void*)&(pStruct->maxExtent), prefix, (pStruct->maxMipLevels), prefix, (pStruct->maxArrayLayers), prefix, (pStruct->sampleCounts), prefix, (pStruct->maxResourceSize));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimagememorybarrier(const VkImageMemoryBarrier* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkimagesubresourcerange(&pStruct->subresourceRange, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssubresourceRange (0x%p)\n%s", prefix, (void*)&pStruct->subresourceRange, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%ssrcAccessMask = %u\n%sdstAccessMask = %u\n%soldLayout = %s\n%snewLayout = %s\n%ssrcQueueFamilyIndex = %u\n%sdstQueueFamilyIndex = %u\n%simage = 0x%p\n%ssubresourceRange = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->srcAccessMask), prefix, (pStruct->dstAccessMask), prefix, string_VkImageLayout(pStruct->oldLayout), prefix, string_VkImageLayout(pStruct->newLayout), prefix, (pStruct->srcQueueFamilyIndex), prefix, (pStruct->dstQueueFamilyIndex), prefix, (void*)(pStruct->image), prefix, (void*)&(pStruct->subresourceRange));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimageresolve(const VkImageResolve* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[5];
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->srcSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssrcSubresource (0x%p)\n%s", prefix, (void*)&pStruct->srcSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->srcOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcOffset (0x%p)\n%s", prefix, (void*)&pStruct->srcOffset, tmpStr);
    tmpStr = vk_print_vkimagesubresourcelayers(&pStruct->dstSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdstSubresource (0x%p)\n%s", prefix, (void*)&pStruct->dstSubresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->dstOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %sdstOffset (0x%p)\n%s", prefix, (void*)&pStruct->dstOffset, tmpStr);
    tmpStr = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[4] = (char*)malloc(len);
    snprintf(stp_strs[4], len, " %sextent (0x%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubresource = 0x%p\n%ssrcOffset = 0x%p\n%sdstSubresource = 0x%p\n%sdstOffset = 0x%p\n%sextent = 0x%p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->dstSubresource), prefix, (void*)&(pStruct->dstOffset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 4; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimagesubresource(const VkImageSubresource* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspectMask = %u\n%smipLevel = %u\n%sarrayLayer = %u\n", prefix, (pStruct->aspectMask), prefix, (pStruct->mipLevel), prefix, (pStruct->arrayLayer));
    return str;
}
char* vk_print_vkimagesubresourcelayers(const VkImageSubresourceLayers* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspectMask = %u\n%smipLevel = %u\n%sbaseArrayLayer = %u\n%slayerCount = %u\n", prefix, (pStruct->aspectMask), prefix, (pStruct->mipLevel), prefix, (pStruct->baseArrayLayer), prefix, (pStruct->layerCount));
    return str;
}
char* vk_print_vkimagesubresourcerange(const VkImageSubresourceRange* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspectMask = %u\n%sbaseMipLevel = %u\n%slevelCount = %u\n%sbaseArrayLayer = %u\n%slayerCount = %u\n", prefix, (pStruct->aspectMask), prefix, (pStruct->baseMipLevel), prefix, (pStruct->levelCount), prefix, (pStruct->baseArrayLayer), prefix, (pStruct->layerCount));
    return str;
}
char* vk_print_vkimageviewcreateinfo(const VkImageViewCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkcomponentmapping(&pStruct->components, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %scomponents (0x%p)\n%s", prefix, (void*)&pStruct->components, tmpStr);
    tmpStr = vk_print_vkimagesubresourcerange(&pStruct->subresourceRange, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %ssubresourceRange (0x%p)\n%s", prefix, (void*)&pStruct->subresourceRange, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%simage = 0x%p\n%sviewType = %s\n%sformat = %s\n%scomponents = 0x%p\n%ssubresourceRange = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->image), prefix, string_VkImageViewType(pStruct->viewType), prefix, string_VkFormat(pStruct->format), prefix, (void*)&(pStruct->components), prefix, (void*)&(pStruct->subresourceRange));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkimportmemorywin32handleinfonv(const VkImportMemoryWin32HandleInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%shandleType = %u\n%shandle = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->handleType), prefix, (void*)(pStruct->handle));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkinstancecreateinfo(const VkInstanceCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pApplicationInfo) {
        tmpStr = vk_print_vkapplicationinfo(pStruct->pApplicationInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spApplicationInfo (0x%p)\n%s", prefix, (void*)pStruct->pApplicationInfo, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%spApplicationInfo = 0x%p\n%senabledLayerCount = %u\n%sppEnabledLayerNames = %s\n%senabledExtensionCount = %u\n%sppEnabledExtensionNames = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->pApplicationInfo), prefix, (pStruct->enabledLayerCount), prefix, (pStruct->ppEnabledLayerNames)[0], prefix, (pStruct->enabledExtensionCount), prefix, (pStruct->ppEnabledExtensionNames)[0]);
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vklayerproperties(const VkLayerProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%slayerName = %s\n%sspecVersion = %u\n%simplementationVersion = %u\n%sdescription = %s\n", prefix, (pStruct->layerName), prefix, (pStruct->specVersion), prefix, (pStruct->implementationVersion), prefix, (pStruct->description));
    return str;
}
char* vk_print_vkmappedmemoryrange(const VkMappedMemoryRange* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%smemory = 0x%p\n%soffset = " PRINTF_SIZE_T_SPECIFIER "\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->memory), prefix, (pStruct->offset), prefix, (pStruct->size));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkmemoryallocateinfo(const VkMemoryAllocateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sallocationSize = " PRINTF_SIZE_T_SPECIFIER "\n%smemoryTypeIndex = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->allocationSize), prefix, (pStruct->memoryTypeIndex));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkmemorybarrier(const VkMemoryBarrier* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%ssrcAccessMask = %u\n%sdstAccessMask = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->srcAccessMask), prefix, (pStruct->dstAccessMask));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkmemoryheap(const VkMemoryHeap* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssize = " PRINTF_SIZE_T_SPECIFIER "\n%sflags = %u\n", prefix, (pStruct->size), prefix, (pStruct->flags));
    return str;
}
char* vk_print_vkmemoryrequirements(const VkMemoryRequirements* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssize = " PRINTF_SIZE_T_SPECIFIER "\n%salignment = " PRINTF_SIZE_T_SPECIFIER "\n%smemoryTypeBits = %u\n", prefix, (pStruct->size), prefix, (pStruct->alignment), prefix, (pStruct->memoryTypeBits));
    return str;
}
char* vk_print_vkmemorytype(const VkMemoryType* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%spropertyFlags = %u\n%sheapIndex = %u\n", prefix, (pStruct->propertyFlags), prefix, (pStruct->heapIndex));
    return str;
}
char* vk_print_vkmirsurfacecreateinfokhr(const VkMirSurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sconnection = 0x%p\n%smirSurface = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->connection), prefix, (pStruct->mirSurface));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkoffset2d(const VkOffset2D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %i\n%sy = %i\n", prefix, (pStruct->x), prefix, (pStruct->y));
    return str;
}
char* vk_print_vkoffset3d(const VkOffset3D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %i\n%sy = %i\n%sz = %i\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
char* vk_print_vkphysicaldevicefeatures(const VkPhysicalDeviceFeatures* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%srobustBufferAccess = %s\n%sfullDrawIndexUint32 = %s\n%simageCubeArray = %s\n%sindependentBlend = %s\n%sgeometryShader = %s\n%stessellationShader = %s\n%ssampleRateShading = %s\n%sdualSrcBlend = %s\n%slogicOp = %s\n%smultiDrawIndirect = %s\n%sdrawIndirectFirstInstance = %s\n%sdepthClamp = %s\n%sdepthBiasClamp = %s\n%sfillModeNonSolid = %s\n%sdepthBounds = %s\n%swideLines = %s\n%slargePoints = %s\n%salphaToOne = %s\n%smultiViewport = %s\n%ssamplerAnisotropy = %s\n%stextureCompressionETC2 = %s\n%stextureCompressionASTC_LDR = %s\n%stextureCompressionBC = %s\n%socclusionQueryPrecise = %s\n%spipelineStatisticsQuery = %s\n%svertexPipelineStoresAndAtomics = %s\n%sfragmentStoresAndAtomics = %s\n%sshaderTessellationAndGeometryPointSize = %s\n%sshaderImageGatherExtended = %s\n%sshaderStorageImageExtendedFormats = %s\n%sshaderStorageImageMultisample = %s\n%sshaderStorageImageReadWithoutFormat = %s\n%sshaderStorageImageWriteWithoutFormat = %s\n%sshaderUniformBufferArrayDynamicIndexing = %s\n%sshaderSampledImageArrayDynamicIndexing = %s\n%sshaderStorageBufferArrayDynamicIndexing = %s\n%sshaderStorageImageArrayDynamicIndexing = %s\n%sshaderClipDistance = %s\n%sshaderCullDistance = %s\n%sshaderFloat64 = %s\n%sshaderInt64 = %s\n%sshaderInt16 = %s\n%sshaderResourceResidency = %s\n%sshaderResourceMinLod = %s\n%ssparseBinding = %s\n%ssparseResidencyBuffer = %s\n%ssparseResidencyImage2D = %s\n%ssparseResidencyImage3D = %s\n%ssparseResidency2Samples = %s\n%ssparseResidency4Samples = %s\n%ssparseResidency8Samples = %s\n%ssparseResidency16Samples = %s\n%ssparseResidencyAliased = %s\n%svariableMultisampleRate = %s\n%sinheritedQueries = %s\n", prefix, (pStruct->robustBufferAccess) ? "TRUE" : "FALSE", prefix, (pStruct->fullDrawIndexUint32) ? "TRUE" : "FALSE", prefix, (pStruct->imageCubeArray) ? "TRUE" : "FALSE", prefix, (pStruct->independentBlend) ? "TRUE" : "FALSE", prefix, (pStruct->geometryShader) ? "TRUE" : "FALSE", prefix, (pStruct->tessellationShader) ? "TRUE" : "FALSE", prefix, (pStruct->sampleRateShading) ? "TRUE" : "FALSE", prefix, (pStruct->dualSrcBlend) ? "TRUE" : "FALSE", prefix, (pStruct->logicOp) ? "TRUE" : "FALSE", prefix, (pStruct->multiDrawIndirect) ? "TRUE" : "FALSE", prefix, (pStruct->drawIndirectFirstInstance) ? "TRUE" : "FALSE", prefix, (pStruct->depthClamp) ? "TRUE" : "FALSE", prefix, (pStruct->depthBiasClamp) ? "TRUE" : "FALSE", prefix, (pStruct->fillModeNonSolid) ? "TRUE" : "FALSE", prefix, (pStruct->depthBounds) ? "TRUE" : "FALSE", prefix, (pStruct->wideLines) ? "TRUE" : "FALSE", prefix, (pStruct->largePoints) ? "TRUE" : "FALSE", prefix, (pStruct->alphaToOne) ? "TRUE" : "FALSE", prefix, (pStruct->multiViewport) ? "TRUE" : "FALSE", prefix, (pStruct->samplerAnisotropy) ? "TRUE" : "FALSE", prefix, (pStruct->textureCompressionETC2) ? "TRUE" : "FALSE", prefix, (pStruct->textureCompressionASTC_LDR) ? "TRUE" : "FALSE", prefix, (pStruct->textureCompressionBC) ? "TRUE" : "FALSE", prefix, (pStruct->occlusionQueryPrecise) ? "TRUE" : "FALSE", prefix, (pStruct->pipelineStatisticsQuery) ? "TRUE" : "FALSE", prefix, (pStruct->vertexPipelineStoresAndAtomics) ? "TRUE" : "FALSE", prefix, (pStruct->fragmentStoresAndAtomics) ? "TRUE" : "FALSE", prefix, (pStruct->shaderTessellationAndGeometryPointSize) ? "TRUE" : "FALSE", prefix, (pStruct->shaderImageGatherExtended) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageImageExtendedFormats) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageImageMultisample) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageImageReadWithoutFormat) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageImageWriteWithoutFormat) ? "TRUE" : "FALSE", prefix, (pStruct->shaderUniformBufferArrayDynamicIndexing) ? "TRUE" : "FALSE", prefix, (pStruct->shaderSampledImageArrayDynamicIndexing) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageBufferArrayDynamicIndexing) ? "TRUE" : "FALSE", prefix, (pStruct->shaderStorageImageArrayDynamicIndexing) ? "TRUE" : "FALSE", prefix, (pStruct->shaderClipDistance) ? "TRUE" : "FALSE", prefix, (pStruct->shaderCullDistance) ? "TRUE" : "FALSE", prefix, (pStruct->shaderFloat64) ? "TRUE" : "FALSE", prefix, (pStruct->shaderInt64) ? "TRUE" : "FALSE", prefix, (pStruct->shaderInt16) ? "TRUE" : "FALSE", prefix, (pStruct->shaderResourceResidency) ? "TRUE" : "FALSE", prefix, (pStruct->shaderResourceMinLod) ? "TRUE" : "FALSE", prefix, (pStruct->sparseBinding) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidencyBuffer) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidencyImage2D) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidencyImage3D) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidency2Samples) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidency4Samples) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidency8Samples) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidency16Samples) ? "TRUE" : "FALSE", prefix, (pStruct->sparseResidencyAliased) ? "TRUE" : "FALSE", prefix, (pStruct->variableMultisampleRate) ? "TRUE" : "FALSE", prefix, (pStruct->inheritedQueries) ? "TRUE" : "FALSE");
    return str;
}
char* vk_print_vkphysicaldevicelimits(const VkPhysicalDeviceLimits* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smaxImageDimension1D = %u\n%smaxImageDimension2D = %u\n%smaxImageDimension3D = %u\n%smaxImageDimensionCube = %u\n%smaxImageArrayLayers = %u\n%smaxTexelBufferElements = %u\n%smaxUniformBufferRange = %u\n%smaxStorageBufferRange = %u\n%smaxPushConstantsSize = %u\n%smaxMemoryAllocationCount = %u\n%smaxSamplerAllocationCount = %u\n%sbufferImageGranularity = " PRINTF_SIZE_T_SPECIFIER "\n%ssparseAddressSpaceSize = " PRINTF_SIZE_T_SPECIFIER "\n%smaxBoundDescriptorSets = %u\n%smaxPerStageDescriptorSamplers = %u\n%smaxPerStageDescriptorUniformBuffers = %u\n%smaxPerStageDescriptorStorageBuffers = %u\n%smaxPerStageDescriptorSampledImages = %u\n%smaxPerStageDescriptorStorageImages = %u\n%smaxPerStageDescriptorInputAttachments = %u\n%smaxPerStageResources = %u\n%smaxDescriptorSetSamplers = %u\n%smaxDescriptorSetUniformBuffers = %u\n%smaxDescriptorSetUniformBuffersDynamic = %u\n%smaxDescriptorSetStorageBuffers = %u\n%smaxDescriptorSetStorageBuffersDynamic = %u\n%smaxDescriptorSetSampledImages = %u\n%smaxDescriptorSetStorageImages = %u\n%smaxDescriptorSetInputAttachments = %u\n%smaxVertexInputAttributes = %u\n%smaxVertexInputBindings = %u\n%smaxVertexInputAttributeOffset = %u\n%smaxVertexInputBindingStride = %u\n%smaxVertexOutputComponents = %u\n%smaxTessellationGenerationLevel = %u\n%smaxTessellationPatchSize = %u\n%smaxTessellationControlPerVertexInputComponents = %u\n%smaxTessellationControlPerVertexOutputComponents = %u\n%smaxTessellationControlPerPatchOutputComponents = %u\n%smaxTessellationControlTotalOutputComponents = %u\n%smaxTessellationEvaluationInputComponents = %u\n%smaxTessellationEvaluationOutputComponents = %u\n%smaxGeometryShaderInvocations = %u\n%smaxGeometryInputComponents = %u\n%smaxGeometryOutputComponents = %u\n%smaxGeometryOutputVertices = %u\n%smaxGeometryTotalOutputComponents = %u\n%smaxFragmentInputComponents = %u\n%smaxFragmentOutputAttachments = %u\n%smaxFragmentDualSrcAttachments = %u\n%smaxFragmentCombinedOutputResources = %u\n%smaxComputeSharedMemorySize = %u\n%smaxComputeWorkGroupCount = 0x%p\n%smaxComputeWorkGroupInvocations = %u\n%smaxComputeWorkGroupSize = 0x%p\n%ssubPixelPrecisionBits = %u\n%ssubTexelPrecisionBits = %u\n%smipmapPrecisionBits = %u\n%smaxDrawIndexedIndexValue = %u\n%smaxDrawIndirectCount = %u\n%smaxSamplerLodBias = %f\n%smaxSamplerAnisotropy = %f\n%smaxViewports = %u\n%smaxViewportDimensions = 0x%p\n%sviewportBoundsRange = 0x%p\n%sviewportSubPixelBits = %u\n%sminMemoryMapAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%sminTexelBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%sminUniformBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%sminStorageBufferOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%sminTexelOffset = %i\n%smaxTexelOffset = %u\n%sminTexelGatherOffset = %i\n%smaxTexelGatherOffset = %u\n%sminInterpolationOffset = %f\n%smaxInterpolationOffset = %f\n%ssubPixelInterpolationOffsetBits = %u\n%smaxFramebufferWidth = %u\n%smaxFramebufferHeight = %u\n%smaxFramebufferLayers = %u\n%sframebufferColorSampleCounts = %u\n%sframebufferDepthSampleCounts = %u\n%sframebufferStencilSampleCounts = %u\n%sframebufferNoAttachmentsSampleCounts = %u\n%smaxColorAttachments = %u\n%ssampledImageColorSampleCounts = %u\n%ssampledImageIntegerSampleCounts = %u\n%ssampledImageDepthSampleCounts = %u\n%ssampledImageStencilSampleCounts = %u\n%sstorageImageSampleCounts = %u\n%smaxSampleMaskWords = %u\n%stimestampComputeAndGraphics = %s\n%stimestampPeriod = %f\n%smaxClipDistances = %u\n%smaxCullDistances = %u\n%smaxCombinedClipAndCullDistances = %u\n%sdiscreteQueuePriorities = %u\n%spointSizeRange = 0x%p\n%slineWidthRange = 0x%p\n%spointSizeGranularity = %f\n%slineWidthGranularity = %f\n%sstrictLines = %s\n%sstandardSampleLocations = %s\n%soptimalBufferCopyOffsetAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%soptimalBufferCopyRowPitchAlignment = " PRINTF_SIZE_T_SPECIFIER "\n%snonCoherentAtomSize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (pStruct->maxImageDimension1D), prefix, (pStruct->maxImageDimension2D), prefix, (pStruct->maxImageDimension3D), prefix, (pStruct->maxImageDimensionCube), prefix, (pStruct->maxImageArrayLayers), prefix, (pStruct->maxTexelBufferElements), prefix, (pStruct->maxUniformBufferRange), prefix, (pStruct->maxStorageBufferRange), prefix, (pStruct->maxPushConstantsSize), prefix, (pStruct->maxMemoryAllocationCount), prefix, (pStruct->maxSamplerAllocationCount), prefix, (pStruct->bufferImageGranularity), prefix, (pStruct->sparseAddressSpaceSize), prefix, (pStruct->maxBoundDescriptorSets), prefix, (pStruct->maxPerStageDescriptorSamplers), prefix, (pStruct->maxPerStageDescriptorUniformBuffers), prefix, (pStruct->maxPerStageDescriptorStorageBuffers), prefix, (pStruct->maxPerStageDescriptorSampledImages), prefix, (pStruct->maxPerStageDescriptorStorageImages), prefix, (pStruct->maxPerStageDescriptorInputAttachments), prefix, (pStruct->maxPerStageResources), prefix, (pStruct->maxDescriptorSetSamplers), prefix, (pStruct->maxDescriptorSetUniformBuffers), prefix, (pStruct->maxDescriptorSetUniformBuffersDynamic), prefix, (pStruct->maxDescriptorSetStorageBuffers), prefix, (pStruct->maxDescriptorSetStorageBuffersDynamic), prefix, (pStruct->maxDescriptorSetSampledImages), prefix, (pStruct->maxDescriptorSetStorageImages), prefix, (pStruct->maxDescriptorSetInputAttachments), prefix, (pStruct->maxVertexInputAttributes), prefix, (pStruct->maxVertexInputBindings), prefix, (pStruct->maxVertexInputAttributeOffset), prefix, (pStruct->maxVertexInputBindingStride), prefix, (pStruct->maxVertexOutputComponents), prefix, (pStruct->maxTessellationGenerationLevel), prefix, (pStruct->maxTessellationPatchSize), prefix, (pStruct->maxTessellationControlPerVertexInputComponents), prefix, (pStruct->maxTessellationControlPerVertexOutputComponents), prefix, (pStruct->maxTessellationControlPerPatchOutputComponents), prefix, (pStruct->maxTessellationControlTotalOutputComponents), prefix, (pStruct->maxTessellationEvaluationInputComponents), prefix, (pStruct->maxTessellationEvaluationOutputComponents), prefix, (pStruct->maxGeometryShaderInvocations), prefix, (pStruct->maxGeometryInputComponents), prefix, (pStruct->maxGeometryOutputComponents), prefix, (pStruct->maxGeometryOutputVertices), prefix, (pStruct->maxGeometryTotalOutputComponents), prefix, (pStruct->maxFragmentInputComponents), prefix, (pStruct->maxFragmentOutputAttachments), prefix, (pStruct->maxFragmentDualSrcAttachments), prefix, (pStruct->maxFragmentCombinedOutputResources), prefix, (pStruct->maxComputeSharedMemorySize), prefix, (void*)(pStruct->maxComputeWorkGroupCount), prefix, (pStruct->maxComputeWorkGroupInvocations), prefix, (void*)(pStruct->maxComputeWorkGroupSize), prefix, (pStruct->subPixelPrecisionBits), prefix, (pStruct->subTexelPrecisionBits), prefix, (pStruct->mipmapPrecisionBits), prefix, (pStruct->maxDrawIndexedIndexValue), prefix, (pStruct->maxDrawIndirectCount), prefix, (pStruct->maxSamplerLodBias), prefix, (pStruct->maxSamplerAnisotropy), prefix, (pStruct->maxViewports), prefix, (void*)(pStruct->maxViewportDimensions), prefix, (void*)(pStruct->viewportBoundsRange), prefix, (pStruct->viewportSubPixelBits), prefix, (pStruct->minMemoryMapAlignment), prefix, (pStruct->minTexelBufferOffsetAlignment), prefix, (pStruct->minUniformBufferOffsetAlignment), prefix, (pStruct->minStorageBufferOffsetAlignment), prefix, (pStruct->minTexelOffset), prefix, (pStruct->maxTexelOffset), prefix, (pStruct->minTexelGatherOffset), prefix, (pStruct->maxTexelGatherOffset), prefix, (pStruct->minInterpolationOffset), prefix, (pStruct->maxInterpolationOffset), prefix, (pStruct->subPixelInterpolationOffsetBits), prefix, (pStruct->maxFramebufferWidth), prefix, (pStruct->maxFramebufferHeight), prefix, (pStruct->maxFramebufferLayers), prefix, (pStruct->framebufferColorSampleCounts), prefix, (pStruct->framebufferDepthSampleCounts), prefix, (pStruct->framebufferStencilSampleCounts), prefix, (pStruct->framebufferNoAttachmentsSampleCounts), prefix, (pStruct->maxColorAttachments), prefix, (pStruct->sampledImageColorSampleCounts), prefix, (pStruct->sampledImageIntegerSampleCounts), prefix, (pStruct->sampledImageDepthSampleCounts), prefix, (pStruct->sampledImageStencilSampleCounts), prefix, (pStruct->storageImageSampleCounts), prefix, (pStruct->maxSampleMaskWords), prefix, (pStruct->timestampComputeAndGraphics) ? "TRUE" : "FALSE", prefix, (pStruct->timestampPeriod), prefix, (pStruct->maxClipDistances), prefix, (pStruct->maxCullDistances), prefix, (pStruct->maxCombinedClipAndCullDistances), prefix, (pStruct->discreteQueuePriorities), prefix, (void*)(pStruct->pointSizeRange), prefix, (void*)(pStruct->lineWidthRange), prefix, (pStruct->pointSizeGranularity), prefix, (pStruct->lineWidthGranularity), prefix, (pStruct->strictLines) ? "TRUE" : "FALSE", prefix, (pStruct->standardSampleLocations) ? "TRUE" : "FALSE", prefix, (pStruct->optimalBufferCopyOffsetAlignment), prefix, (pStruct->optimalBufferCopyRowPitchAlignment), prefix, (pStruct->nonCoherentAtomSize));
    return str;
}
char* vk_print_vkphysicaldevicememoryproperties(const VkPhysicalDeviceMemoryProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = vk_print_vkmemorytype(&pStruct->memoryTypes[0], extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %smemoryTypes[0] (0x%p)\n%s", prefix, (void*)&pStruct->memoryTypes[0], tmpStr);
    tmpStr = vk_print_vkmemoryheap(&pStruct->memoryHeaps[0], extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %smemoryHeaps[0] (0x%p)\n%s", prefix, (void*)&pStruct->memoryHeaps[0], tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smemoryTypeCount = %u\n%smemoryTypes = 0x%p\n%smemoryHeapCount = %u\n%smemoryHeaps = 0x%p\n", prefix, (pStruct->memoryTypeCount), prefix, (void*)(pStruct->memoryTypes), prefix, (pStruct->memoryHeapCount), prefix, (void*)(pStruct->memoryHeaps));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkphysicaldeviceproperties(const VkPhysicalDeviceProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = vk_print_vkphysicaldevicelimits(&pStruct->limits, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %slimits (0x%p)\n%s", prefix, (void*)&pStruct->limits, tmpStr);
    tmpStr = vk_print_vkphysicaldevicesparseproperties(&pStruct->sparseProperties, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssparseProperties (0x%p)\n%s", prefix, (void*)&pStruct->sparseProperties, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sapiVersion = %u\n%sdriverVersion = %u\n%svendorID = %u\n%sdeviceID = %u\n%sdeviceType = %s\n%sdeviceName = %s\n%spipelineCacheUUID = 0x%p\n%slimits = 0x%p\n%ssparseProperties = 0x%p\n", prefix, (pStruct->apiVersion), prefix, (pStruct->driverVersion), prefix, (pStruct->vendorID), prefix, (pStruct->deviceID), prefix, string_VkPhysicalDeviceType(pStruct->deviceType), prefix, (pStruct->deviceName), prefix, (void*)(pStruct->pipelineCacheUUID), prefix, (void*)&(pStruct->limits), prefix, (void*)&(pStruct->sparseProperties));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkphysicaldevicesparseproperties(const VkPhysicalDeviceSparseProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sresidencyStandard2DBlockShape = %s\n%sresidencyStandard2DMultisampleBlockShape = %s\n%sresidencyStandard3DBlockShape = %s\n%sresidencyAlignedMipSize = %s\n%sresidencyNonResidentStrict = %s\n", prefix, (pStruct->residencyStandard2DBlockShape) ? "TRUE" : "FALSE", prefix, (pStruct->residencyStandard2DMultisampleBlockShape) ? "TRUE" : "FALSE", prefix, (pStruct->residencyStandard3DBlockShape) ? "TRUE" : "FALSE", prefix, (pStruct->residencyAlignedMipSize) ? "TRUE" : "FALSE", prefix, (pStruct->residencyNonResidentStrict) ? "TRUE" : "FALSE");
    return str;
}
char* vk_print_vkpipelinecachecreateinfo(const VkPipelineCacheCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sinitialDataSize = " PRINTF_SIZE_T_SPECIFIER "\n%spInitialData = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->initialDataSize), prefix, (pStruct->pInitialData));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinecolorblendattachmentstate(const VkPipelineColorBlendAttachmentState* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sblendEnable = %s\n%ssrcColorBlendFactor = %s\n%sdstColorBlendFactor = %s\n%scolorBlendOp = %s\n%ssrcAlphaBlendFactor = %s\n%sdstAlphaBlendFactor = %s\n%salphaBlendOp = %s\n%scolorWriteMask = %u\n", prefix, (pStruct->blendEnable) ? "TRUE" : "FALSE", prefix, string_VkBlendFactor(pStruct->srcColorBlendFactor), prefix, string_VkBlendFactor(pStruct->dstColorBlendFactor), prefix, string_VkBlendOp(pStruct->colorBlendOp), prefix, string_VkBlendFactor(pStruct->srcAlphaBlendFactor), prefix, string_VkBlendFactor(pStruct->dstAlphaBlendFactor), prefix, string_VkBlendOp(pStruct->alphaBlendOp), prefix, (pStruct->colorWriteMask));
    return str;
}
char* vk_print_vkpipelinecolorblendstatecreateinfo(const VkPipelineColorBlendStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pAttachments) {
        tmpStr = vk_print_vkpipelinecolorblendattachmentstate(pStruct->pAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spAttachments (0x%p)\n%s", prefix, (void*)pStruct->pAttachments, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%slogicOpEnable = %s\n%slogicOp = %s\n%sattachmentCount = %u\n%spAttachments = 0x%p\n%sblendConstants = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->logicOpEnable) ? "TRUE" : "FALSE", prefix, string_VkLogicOp(pStruct->logicOp), prefix, (pStruct->attachmentCount), prefix, (void*)(pStruct->pAttachments), prefix, (void*)(pStruct->blendConstants));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinedepthstencilstatecreateinfo(const VkPipelineDepthStencilStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkstencilopstate(&pStruct->front, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sfront (0x%p)\n%s", prefix, (void*)&pStruct->front, tmpStr);
    tmpStr = vk_print_vkstencilopstate(&pStruct->back, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sback (0x%p)\n%s", prefix, (void*)&pStruct->back, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdepthTestEnable = %s\n%sdepthWriteEnable = %s\n%sdepthCompareOp = %s\n%sdepthBoundsTestEnable = %s\n%sstencilTestEnable = %s\n%sfront = 0x%p\n%sback = 0x%p\n%sminDepthBounds = %f\n%smaxDepthBounds = %f\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->depthTestEnable) ? "TRUE" : "FALSE", prefix, (pStruct->depthWriteEnable) ? "TRUE" : "FALSE", prefix, string_VkCompareOp(pStruct->depthCompareOp), prefix, (pStruct->depthBoundsTestEnable) ? "TRUE" : "FALSE", prefix, (pStruct->stencilTestEnable) ? "TRUE" : "FALSE", prefix, (void*)&(pStruct->front), prefix, (void*)&(pStruct->back), prefix, (pStruct->minDepthBounds), prefix, (pStruct->maxDepthBounds));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinedynamicstatecreateinfo(const VkPipelineDynamicStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdynamicStateCount = %u\n%spDynamicStates = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->dynamicStateCount), prefix, (void*)(pStruct->pDynamicStates));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelineinputassemblystatecreateinfo(const VkPipelineInputAssemblyStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%stopology = %s\n%sprimitiveRestartEnable = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkPrimitiveTopology(pStruct->topology), prefix, (pStruct->primitiveRestartEnable) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinelayoutcreateinfo(const VkPipelineLayoutCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pPushConstantRanges) {
        tmpStr = vk_print_vkpushconstantrange(pStruct->pPushConstantRanges, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spPushConstantRanges (0x%p)\n%s", prefix, (void*)pStruct->pPushConstantRanges, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%ssetLayoutCount = %u\n%spSetLayouts = 0x%p\n%spushConstantRangeCount = %u\n%spPushConstantRanges = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->setLayoutCount), prefix, (void*)(pStruct->pSetLayouts), prefix, (pStruct->pushConstantRangeCount), prefix, (void*)(pStruct->pPushConstantRanges));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinemultisamplestatecreateinfo(const VkPipelineMultisampleStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%srasterizationSamples = %s\n%ssampleShadingEnable = %s\n%sminSampleShading = %f\n%spSampleMask = %u\n%salphaToCoverageEnable = %s\n%salphaToOneEnable = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkSampleCountFlagBits(pStruct->rasterizationSamples), prefix, (pStruct->sampleShadingEnable) ? "TRUE" : "FALSE", prefix, (pStruct->minSampleShading), prefix, (pStruct->pSampleMask), prefix, (pStruct->alphaToCoverageEnable) ? "TRUE" : "FALSE", prefix, (pStruct->alphaToOneEnable) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinerasterizationstatecreateinfo(const VkPipelineRasterizationStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdepthClampEnable = %s\n%srasterizerDiscardEnable = %s\n%spolygonMode = %s\n%scullMode = %u\n%sfrontFace = %s\n%sdepthBiasEnable = %s\n%sdepthBiasConstantFactor = %f\n%sdepthBiasClamp = %f\n%sdepthBiasSlopeFactor = %f\n%slineWidth = %f\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->depthClampEnable) ? "TRUE" : "FALSE", prefix, (pStruct->rasterizerDiscardEnable) ? "TRUE" : "FALSE", prefix, string_VkPolygonMode(pStruct->polygonMode), prefix, (pStruct->cullMode), prefix, string_VkFrontFace(pStruct->frontFace), prefix, (pStruct->depthBiasEnable) ? "TRUE" : "FALSE", prefix, (pStruct->depthBiasConstantFactor), prefix, (pStruct->depthBiasClamp), prefix, (pStruct->depthBiasSlopeFactor), prefix, (pStruct->lineWidth));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinerasterizationstaterasterizationorderamd(const VkPipelineRasterizationStateRasterizationOrderAMD* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%srasterizationOrder = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, string_VkRasterizationOrderAMD(pStruct->rasterizationOrder));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelineshaderstagecreateinfo(const VkPipelineShaderStageCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pSpecializationInfo) {
        tmpStr = vk_print_vkspecializationinfo(pStruct->pSpecializationInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spSpecializationInfo (0x%p)\n%s", prefix, (void*)pStruct->pSpecializationInfo, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sstage = %s\n%smodule = 0x%p\n%spName = 0x%p\n%spSpecializationInfo = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkShaderStageFlagBits(pStruct->stage), prefix, (void*)(pStruct->module), prefix, (pStruct->pName), prefix, (void*)(pStruct->pSpecializationInfo));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinetessellationstatecreateinfo(const VkPipelineTessellationStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%spatchControlPoints = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->patchControlPoints));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelinevertexinputstatecreateinfo(const VkPipelineVertexInputStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pVertexBindingDescriptions) {
        tmpStr = vk_print_vkvertexinputbindingdescription(pStruct->pVertexBindingDescriptions, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spVertexBindingDescriptions (0x%p)\n%s", prefix, (void*)pStruct->pVertexBindingDescriptions, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pVertexAttributeDescriptions) {
        tmpStr = vk_print_vkvertexinputattributedescription(pStruct->pVertexAttributeDescriptions, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spVertexAttributeDescriptions (0x%p)\n%s", prefix, (void*)pStruct->pVertexAttributeDescriptions, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%svertexBindingDescriptionCount = %u\n%spVertexBindingDescriptions = 0x%p\n%svertexAttributeDescriptionCount = %u\n%spVertexAttributeDescriptions = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->vertexBindingDescriptionCount), prefix, (void*)(pStruct->pVertexBindingDescriptions), prefix, (pStruct->vertexAttributeDescriptionCount), prefix, (void*)(pStruct->pVertexAttributeDescriptions));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpipelineviewportstatecreateinfo(const VkPipelineViewportStateCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pViewports) {
        tmpStr = vk_print_vkviewport(pStruct->pViewports, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spViewports (0x%p)\n%s", prefix, (void*)pStruct->pViewports, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pScissors) {
        tmpStr = vk_print_vkrect2d(pStruct->pScissors, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spScissors (0x%p)\n%s", prefix, (void*)pStruct->pScissors, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sviewportCount = %u\n%spViewports = 0x%p\n%sscissorCount = %u\n%spScissors = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->viewportCount), prefix, (void*)(pStruct->pViewports), prefix, (pStruct->scissorCount), prefix, (void*)(pStruct->pScissors));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpresentinfokhr(const VkPresentInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%swaitSemaphoreCount = %u\n%spWaitSemaphores = 0x%p\n%sswapchainCount = %u\n%spSwapchains = 0x%p\n%spImageIndices = 0x%p\n%spResults = 0x%s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->waitSemaphoreCount), prefix, (void*)(pStruct->pWaitSemaphores), prefix, (pStruct->swapchainCount), prefix, (void*)(pStruct->pSwapchains), prefix, (void*)(pStruct->pImageIndices), prefix, string_VkResult(*pStruct->pResults));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkpushconstantrange(const VkPushConstantRange* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sstageFlags = %u\n%soffset = %u\n%ssize = %u\n", prefix, (pStruct->stageFlags), prefix, (pStruct->offset), prefix, (pStruct->size));
    return str;
}
char* vk_print_vkquerypoolcreateinfo(const VkQueryPoolCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%squeryType = %s\n%squeryCount = %u\n%spipelineStatistics = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkQueryType(pStruct->queryType), prefix, (pStruct->queryCount), prefix, (pStruct->pipelineStatistics));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkqueuefamilyproperties(const VkQueueFamilyProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkextent3d(&pStruct->minImageTransferGranularity, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sminImageTransferGranularity (0x%p)\n%s", prefix, (void*)&pStruct->minImageTransferGranularity, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%squeueFlags = %u\n%squeueCount = %u\n%stimestampValidBits = %u\n%sminImageTransferGranularity = 0x%p\n", prefix, (pStruct->queueFlags), prefix, (pStruct->queueCount), prefix, (pStruct->timestampValidBits), prefix, (void*)&(pStruct->minImageTransferGranularity));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkrect2d(const VkRect2D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = vk_print_vkoffset2d(&pStruct->offset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %soffset (0x%p)\n%s", prefix, (void*)&pStruct->offset, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sextent (0x%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soffset = 0x%p\n%sextent = 0x%p\n", prefix, (void*)&(pStruct->offset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkrenderpassbegininfo(const VkRenderPassBeginInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkrect2d(&pStruct->renderArea, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %srenderArea (0x%p)\n%s", prefix, (void*)&pStruct->renderArea, tmpStr);
    if (pStruct->pClearValues) {
        tmpStr = vk_print_vkclearvalue(pStruct->pClearValues, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spClearValues (0x%p)\n%s", prefix, (void*)pStruct->pClearValues, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%srenderPass = 0x%p\n%sframebuffer = 0x%p\n%srenderArea = 0x%p\n%sclearValueCount = %u\n%spClearValues = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->renderPass), prefix, (void*)(pStruct->framebuffer), prefix, (void*)&(pStruct->renderArea), prefix, (pStruct->clearValueCount), prefix, (void*)(pStruct->pClearValues));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkrenderpasscreateinfo(const VkRenderPassCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[4];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pAttachments) {
        tmpStr = vk_print_vkattachmentdescription(pStruct->pAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spAttachments (0x%p)\n%s", prefix, (void*)pStruct->pAttachments, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pSubpasses) {
        tmpStr = vk_print_vksubpassdescription(pStruct->pSubpasses, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spSubpasses (0x%p)\n%s", prefix, (void*)pStruct->pSubpasses, tmpStr);
    }
    else
        stp_strs[2] = "";
    if (pStruct->pDependencies) {
        tmpStr = vk_print_vksubpassdependency(pStruct->pDependencies, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[3] = (char*)malloc(len);
        snprintf(stp_strs[3], len, " %spDependencies (0x%p)\n%s", prefix, (void*)pStruct->pDependencies, tmpStr);
    }
    else
        stp_strs[3] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sattachmentCount = %u\n%spAttachments = 0x%p\n%ssubpassCount = %u\n%spSubpasses = 0x%p\n%sdependencyCount = %u\n%spDependencies = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->attachmentCount), prefix, (void*)(pStruct->pAttachments), prefix, (pStruct->subpassCount), prefix, (void*)(pStruct->pSubpasses), prefix, (pStruct->dependencyCount), prefix, (void*)(pStruct->pDependencies));
    for (int32_t stp_index = 3; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksamplercreateinfo(const VkSamplerCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%smagFilter = %s\n%sminFilter = %s\n%smipmapMode = %s\n%saddressModeU = %s\n%saddressModeV = %s\n%saddressModeW = %s\n%smipLodBias = %f\n%sanisotropyEnable = %s\n%smaxAnisotropy = %f\n%scompareEnable = %s\n%scompareOp = %s\n%sminLod = %f\n%smaxLod = %f\n%sborderColor = %s\n%sunnormalizedCoordinates = %s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, string_VkFilter(pStruct->magFilter), prefix, string_VkFilter(pStruct->minFilter), prefix, string_VkSamplerMipmapMode(pStruct->mipmapMode), prefix, string_VkSamplerAddressMode(pStruct->addressModeU), prefix, string_VkSamplerAddressMode(pStruct->addressModeV), prefix, string_VkSamplerAddressMode(pStruct->addressModeW), prefix, (pStruct->mipLodBias), prefix, (pStruct->anisotropyEnable) ? "TRUE" : "FALSE", prefix, (pStruct->maxAnisotropy), prefix, (pStruct->compareEnable) ? "TRUE" : "FALSE", prefix, string_VkCompareOp(pStruct->compareOp), prefix, (pStruct->minLod), prefix, (pStruct->maxLod), prefix, string_VkBorderColor(pStruct->borderColor), prefix, (pStruct->unnormalizedCoordinates) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksemaphorecreateinfo(const VkSemaphoreCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkshadermodulecreateinfo(const VkShaderModuleCreateInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%scodeSize = " PRINTF_SIZE_T_SPECIFIER "\n%spCode = %u\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->codeSize), prefix, (pStruct->pCode));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparsebuffermemorybindinfo(const VkSparseBufferMemoryBindInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pBinds) {
        tmpStr = vk_print_vksparsememorybind(pStruct->pBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spBinds (0x%p)\n%s", prefix, (void*)pStruct->pBinds, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbuffer = 0x%p\n%sbindCount = %u\n%spBinds = 0x%p\n", prefix, (void*)(pStruct->buffer), prefix, (pStruct->bindCount), prefix, (void*)(pStruct->pBinds));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparseimageformatproperties(const VkSparseImageFormatProperties* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vkextent3d(&pStruct->imageGranularity, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %simageGranularity (0x%p)\n%s", prefix, (void*)&pStruct->imageGranularity, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspectMask = %u\n%simageGranularity = 0x%p\n%sflags = %u\n", prefix, (pStruct->aspectMask), prefix, (void*)&(pStruct->imageGranularity), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparseimagememorybind(const VkSparseImageMemoryBind* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    tmpStr = vk_print_vkimagesubresource(&pStruct->subresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssubresource (0x%p)\n%s", prefix, (void*)&pStruct->subresource, tmpStr);
    tmpStr = vk_print_vkoffset3d(&pStruct->offset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %soffset (0x%p)\n%s", prefix, (void*)&pStruct->offset, tmpStr);
    tmpStr = vk_print_vkextent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sextent (0x%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssubresource = 0x%p\n%soffset = 0x%p\n%sextent = 0x%p\n%smemory = 0x%p\n%smemoryOffset = " PRINTF_SIZE_T_SPECIFIER "\n%sflags = %u\n", prefix, (void*)&(pStruct->subresource), prefix, (void*)&(pStruct->offset), prefix, (void*)&(pStruct->extent), prefix, (void*)(pStruct->memory), prefix, (pStruct->memoryOffset), prefix, (pStruct->flags));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparseimagememorybindinfo(const VkSparseImageMemoryBindInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pBinds) {
        tmpStr = vk_print_vksparseimagememorybind(pStruct->pBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spBinds (0x%p)\n%s", prefix, (void*)pStruct->pBinds, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%simage = 0x%p\n%sbindCount = %u\n%spBinds = 0x%p\n", prefix, (void*)(pStruct->image), prefix, (pStruct->bindCount), prefix, (void*)(pStruct->pBinds));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparseimagememoryrequirements(const VkSparseImageMemoryRequirements* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = vk_print_vksparseimageformatproperties(&pStruct->formatProperties, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %sformatProperties (0x%p)\n%s", prefix, (void*)&pStruct->formatProperties, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sformatProperties = 0x%p\n%simageMipTailFirstLod = %u\n%simageMipTailSize = " PRINTF_SIZE_T_SPECIFIER "\n%simageMipTailOffset = " PRINTF_SIZE_T_SPECIFIER "\n%simageMipTailStride = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (void*)&(pStruct->formatProperties), prefix, (pStruct->imageMipTailFirstLod), prefix, (pStruct->imageMipTailSize), prefix, (pStruct->imageMipTailOffset), prefix, (pStruct->imageMipTailStride));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparseimageopaquememorybindinfo(const VkSparseImageOpaqueMemoryBindInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pBinds) {
        tmpStr = vk_print_vksparsememorybind(pStruct->pBinds, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spBinds (0x%p)\n%s", prefix, (void*)pStruct->pBinds, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%simage = 0x%p\n%sbindCount = %u\n%spBinds = 0x%p\n", prefix, (void*)(pStruct->image), prefix, (pStruct->bindCount), prefix, (void*)(pStruct->pBinds));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksparsememorybind(const VkSparseMemoryBind* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sresourceOffset = " PRINTF_SIZE_T_SPECIFIER "\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n%smemory = 0x%p\n%smemoryOffset = " PRINTF_SIZE_T_SPECIFIER "\n%sflags = %u\n", prefix, (pStruct->resourceOffset), prefix, (pStruct->size), prefix, (void*)(pStruct->memory), prefix, (pStruct->memoryOffset), prefix, (pStruct->flags));
    return str;
}
char* vk_print_vkspecializationinfo(const VkSpecializationInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pMapEntries) {
        tmpStr = vk_print_vkspecializationmapentry(pStruct->pMapEntries, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spMapEntries (0x%p)\n%s", prefix, (void*)pStruct->pMapEntries, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smapEntryCount = %u\n%spMapEntries = 0x%p\n%sdataSize = " PRINTF_SIZE_T_SPECIFIER "\n%spData = 0x%p\n", prefix, (pStruct->mapEntryCount), prefix, (void*)(pStruct->pMapEntries), prefix, (pStruct->dataSize), prefix, (pStruct->pData));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkspecializationmapentry(const VkSpecializationMapEntry* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sconstantID = %u\n%soffset = %u\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (pStruct->constantID), prefix, (pStruct->offset), prefix, (pStruct->size));
    return str;
}
char* vk_print_vkstencilopstate(const VkStencilOpState* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sfailOp = %s\n%spassOp = %s\n%sdepthFailOp = %s\n%scompareOp = %s\n%scompareMask = %u\n%swriteMask = %u\n%sreference = %u\n", prefix, string_VkStencilOp(pStruct->failOp), prefix, string_VkStencilOp(pStruct->passOp), prefix, string_VkStencilOp(pStruct->depthFailOp), prefix, string_VkCompareOp(pStruct->compareOp), prefix, (pStruct->compareMask), prefix, (pStruct->writeMask), prefix, (pStruct->reference));
    return str;
}
char* vk_print_vksubmitinfo(const VkSubmitInfo* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%swaitSemaphoreCount = %u\n%spWaitSemaphores = 0x%p\n%spWaitDstStageMask = %u\n%scommandBufferCount = %u\n%spCommandBuffers = 0x%p\n%ssignalSemaphoreCount = %u\n%spSignalSemaphores = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->waitSemaphoreCount), prefix, (void*)(pStruct->pWaitSemaphores), prefix, (pStruct->pWaitDstStageMask), prefix, (pStruct->commandBufferCount), prefix, (void*)(pStruct->pCommandBuffers), prefix, (pStruct->signalSemaphoreCount), prefix, (void*)(pStruct->pSignalSemaphores));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksubpassdependency(const VkSubpassDependency* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubpass = %u\n%sdstSubpass = %u\n%ssrcStageMask = %u\n%sdstStageMask = %u\n%ssrcAccessMask = %u\n%sdstAccessMask = %u\n%sdependencyFlags = %u\n", prefix, (pStruct->srcSubpass), prefix, (pStruct->dstSubpass), prefix, (pStruct->srcStageMask), prefix, (pStruct->dstStageMask), prefix, (pStruct->srcAccessMask), prefix, (pStruct->dstAccessMask), prefix, (pStruct->dependencyFlags));
    return str;
}
char* vk_print_vksubpassdescription(const VkSubpassDescription* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[4];
    if (pStruct->pInputAttachments) {
        tmpStr = vk_print_vkattachmentreference(pStruct->pInputAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spInputAttachments (0x%p)\n%s", prefix, (void*)pStruct->pInputAttachments, tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pColorAttachments) {
        tmpStr = vk_print_vkattachmentreference(pStruct->pColorAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spColorAttachments (0x%p)\n%s", prefix, (void*)pStruct->pColorAttachments, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pResolveAttachments) {
        tmpStr = vk_print_vkattachmentreference(pStruct->pResolveAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spResolveAttachments (0x%p)\n%s", prefix, (void*)pStruct->pResolveAttachments, tmpStr);
    }
    else
        stp_strs[2] = "";
    if (pStruct->pDepthStencilAttachment) {
        tmpStr = vk_print_vkattachmentreference(pStruct->pDepthStencilAttachment, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[3] = (char*)malloc(len);
        snprintf(stp_strs[3], len, " %spDepthStencilAttachment (0x%p)\n%s", prefix, (void*)pStruct->pDepthStencilAttachment, tmpStr);
    }
    else
        stp_strs[3] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sflags = %u\n%spipelineBindPoint = %s\n%sinputAttachmentCount = %u\n%spInputAttachments = 0x%p\n%scolorAttachmentCount = %u\n%spColorAttachments = 0x%p\n%spResolveAttachments = 0x%p\n%spDepthStencilAttachment = 0x%p\n%spreserveAttachmentCount = %u\n%spPreserveAttachments = 0x%p\n", prefix, (pStruct->flags), prefix, string_VkPipelineBindPoint(pStruct->pipelineBindPoint), prefix, (pStruct->inputAttachmentCount), prefix, (void*)(pStruct->pInputAttachments), prefix, (pStruct->colorAttachmentCount), prefix, (void*)(pStruct->pColorAttachments), prefix, (void*)(pStruct->pResolveAttachments), prefix, (void*)(pStruct->pDepthStencilAttachment), prefix, (pStruct->preserveAttachmentCount), prefix, (void*)(pStruct->pPreserveAttachments));
    for (int32_t stp_index = 3; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksubresourcelayout(const VkSubresourceLayout* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soffset = " PRINTF_SIZE_T_SPECIFIER "\n%ssize = " PRINTF_SIZE_T_SPECIFIER "\n%srowPitch = " PRINTF_SIZE_T_SPECIFIER "\n%sarrayPitch = " PRINTF_SIZE_T_SPECIFIER "\n%sdepthPitch = " PRINTF_SIZE_T_SPECIFIER "\n", prefix, (pStruct->offset), prefix, (pStruct->size), prefix, (pStruct->rowPitch), prefix, (pStruct->arrayPitch), prefix, (pStruct->depthPitch));
    return str;
}
char* vk_print_vksurfacecapabilitieskhr(const VkSurfaceCapabilitiesKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    tmpStr = vk_print_vkextent2d(&pStruct->currentExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %scurrentExtent (0x%p)\n%s", prefix, (void*)&pStruct->currentExtent, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->minImageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sminImageExtent (0x%p)\n%s", prefix, (void*)&pStruct->minImageExtent, tmpStr);
    tmpStr = vk_print_vkextent2d(&pStruct->maxImageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %smaxImageExtent (0x%p)\n%s", prefix, (void*)&pStruct->maxImageExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sminImageCount = %u\n%smaxImageCount = %u\n%scurrentExtent = 0x%p\n%sminImageExtent = 0x%p\n%smaxImageExtent = 0x%p\n%smaxImageArrayLayers = %u\n%ssupportedTransforms = %u\n%scurrentTransform = %s\n%ssupportedCompositeAlpha = %u\n%ssupportedUsageFlags = %u\n", prefix, (pStruct->minImageCount), prefix, (pStruct->maxImageCount), prefix, (void*)&(pStruct->currentExtent), prefix, (void*)&(pStruct->minImageExtent), prefix, (void*)&(pStruct->maxImageExtent), prefix, (pStruct->maxImageArrayLayers), prefix, (pStruct->supportedTransforms), prefix, string_VkSurfaceTransformFlagBitsKHR(pStruct->currentTransform), prefix, (pStruct->supportedCompositeAlpha), prefix, (pStruct->supportedUsageFlags));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vksurfaceformatkhr(const VkSurfaceFormatKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sformat = %s\n%scolorSpace = %s\n", prefix, string_VkFormat(pStruct->format), prefix, string_VkColorSpaceKHR(pStruct->colorSpace));
    return str;
}
char* vk_print_vkswapchaincreateinfokhr(const VkSwapchainCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = vk_print_vkextent2d(&pStruct->imageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %simageExtent (0x%p)\n%s", prefix, (void*)&pStruct->imageExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%ssurface = 0x%p\n%sminImageCount = %u\n%simageFormat = %s\n%simageColorSpace = %s\n%simageExtent = 0x%p\n%simageArrayLayers = %u\n%simageUsage = %u\n%simageSharingMode = %s\n%squeueFamilyIndexCount = %u\n%spQueueFamilyIndices = 0x%p\n%spreTransform = %s\n%scompositeAlpha = %s\n%spresentMode = %s\n%sclipped = %s\n%soldSwapchain = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->surface), prefix, (pStruct->minImageCount), prefix, string_VkFormat(pStruct->imageFormat), prefix, string_VkColorSpaceKHR(pStruct->imageColorSpace), prefix, (void*)&(pStruct->imageExtent), prefix, (pStruct->imageArrayLayers), prefix, (pStruct->imageUsage), prefix, string_VkSharingMode(pStruct->imageSharingMode), prefix, (pStruct->queueFamilyIndexCount), prefix, (void*)(pStruct->pQueueFamilyIndices), prefix, string_VkSurfaceTransformFlagBitsKHR(pStruct->preTransform), prefix, string_VkCompositeAlphaFlagBitsKHR(pStruct->compositeAlpha), prefix, string_VkPresentModeKHR(pStruct->presentMode), prefix, (pStruct->clipped) ? "TRUE" : "FALSE", prefix, (void*)(pStruct->oldSwapchain));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkvalidationflagsext(const VkValidationFlagsEXT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sdisabledValidationCheckCount = %u\n%spDisabledValidationChecks = 0x%s\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->disabledValidationCheckCount), prefix, string_VkValidationCheckEXT(*pStruct->pDisabledValidationChecks));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkvertexinputattributedescription(const VkVertexInputAttributeDescription* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%slocation = %u\n%sbinding = %u\n%sformat = %s\n%soffset = %u\n", prefix, (pStruct->location), prefix, (pStruct->binding), prefix, string_VkFormat(pStruct->format), prefix, (pStruct->offset));
    return str;
}
char* vk_print_vkvertexinputbindingdescription(const VkVertexInputBindingDescription* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbinding = %u\n%sstride = %u\n%sinputRate = %s\n", prefix, (pStruct->binding), prefix, (pStruct->stride), prefix, string_VkVertexInputRate(pStruct->inputRate));
    return str;
}
char* vk_print_vkviewport(const VkViewport* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %f\n%sy = %f\n%swidth = %f\n%sheight = %f\n%sminDepth = %f\n%smaxDepth = %f\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->minDepth), prefix, (pStruct->maxDepth));
    return str;
}
char* vk_print_vkwaylandsurfacecreateinfokhr(const VkWaylandSurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdisplay = 0x%p\n%ssurface = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->display), prefix, (pStruct->surface));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkwin32keyedmutexacquirereleaseinfonv(const VkWin32KeyedMutexAcquireReleaseInfoNV* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sacquireCount = %u\n%spAcquireSyncs = 0x%p\n%spAcquireKeys = 0x%p\n%spAcquireTimeoutMilliseconds = 0x%p\n%sreleaseCount = %u\n%spReleaseSyncs = 0x%p\n%spReleaseKeys = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->acquireCount), prefix, (void*)(pStruct->pAcquireSyncs), prefix, (void*)(pStruct->pAcquireKeys), prefix, (void*)(pStruct->pAcquireTimeoutMilliseconds), prefix, (pStruct->releaseCount), prefix, (void*)(pStruct->pReleaseSyncs), prefix, (void*)(pStruct->pReleaseKeys));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkwin32surfacecreateinfokhr(const VkWin32SurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%shinstance = 0x%p\n%shwnd = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->hinstance), prefix, (void*)(pStruct->hwnd));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkwritedescriptorset(const VkWriteDescriptorSet* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pImageInfo) {
        tmpStr = vk_print_vkdescriptorimageinfo(pStruct->pImageInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spImageInfo (0x%p)\n%s", prefix, (void*)pStruct->pImageInfo, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pBufferInfo) {
        tmpStr = vk_print_vkdescriptorbufferinfo(pStruct->pBufferInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spBufferInfo (0x%p)\n%s", prefix, (void*)pStruct->pBufferInfo, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sdstSet = 0x%p\n%sdstBinding = %u\n%sdstArrayElement = %u\n%sdescriptorCount = %u\n%sdescriptorType = %s\n%spImageInfo = 0x%p\n%spBufferInfo = 0x%p\n%spTexelBufferView = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->dstSet), prefix, (pStruct->dstBinding), prefix, (pStruct->dstArrayElement), prefix, (pStruct->descriptorCount), prefix, string_VkDescriptorType(pStruct->descriptorType), prefix, (void*)(pStruct->pImageInfo), prefix, (void*)(pStruct->pBufferInfo), prefix, (void*)(pStruct->pTexelBufferView));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkxcbsurfacecreateinfokhr(const VkXcbSurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sconnection = 0x%p\n%swindow = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->connection), prefix, (void*)(pStruct->window));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* vk_print_vkxlibsurfacecreateinfokhr(const VkXlibSurfaceCreateInfoKHR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (0x%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = 0x%p\n%sflags = %u\n%sdpy = 0x%p\n%swindow = 0x%p\n", prefix, string_VkStructureType(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (pStruct->dpy), prefix, (void*)(pStruct->window));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* dynamic_display(const void* pStruct, const char* prefix)
{
    // Cast to APP_INFO ptr initially just to pull sType off struct
    if (pStruct == NULL) {
        return NULL;
    }
    VkStructureType sType = ((VkApplicationInfo*)pStruct)->sType;
    char indent[100];
    strcpy(indent, "    ");
    strcat(indent, prefix);
    switch (sType)
    {
        case VK_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            return vk_print_vkapplicationinfo((VkApplicationInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
        {
            return vk_print_vkbindsparseinfo((VkBindSparseInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        {
            return vk_print_vkbuffercreateinfo((VkBufferCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        {
            return vk_print_vkbuffermemorybarrier((VkBufferMemoryBarrier*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        {
            return vk_print_vkbufferviewcreateinfo((VkBufferViewCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO:
        {
            return vk_print_vkcommandbufferallocateinfo((VkCommandBufferAllocateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO:
        {
            return vk_print_vkcommandbufferbegininfo((VkCommandBufferBeginInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO:
        {
            return vk_print_vkcommandbufferinheritanceinfo((VkCommandBufferInheritanceInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
        {
            return vk_print_vkcommandpoolcreateinfo((VkCommandPoolCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            return vk_print_vkcomputepipelinecreateinfo((VkComputePipelineCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET:
        {
            return vk_print_vkcopydescriptorset((VkCopyDescriptorSet*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO:
        {
            return vk_print_vkdescriptorpoolcreateinfo((VkDescriptorPoolCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO:
        {
            return vk_print_vkdescriptorsetallocateinfo((VkDescriptorSetAllocateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        {
            return vk_print_vkdescriptorsetlayoutcreateinfo((VkDescriptorSetLayoutCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            return vk_print_vkdevicecreateinfo((VkDeviceCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO:
        {
            return vk_print_vkdevicequeuecreateinfo((VkDeviceQueueCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            return vk_print_vkeventcreateinfo((VkEventCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            return vk_print_vkfencecreateinfo((VkFenceCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        {
            return vk_print_vkframebuffercreateinfo((VkFramebufferCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            return vk_print_vkgraphicspipelinecreateinfo((VkGraphicsPipelineCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            return vk_print_vkimagecreateinfo((VkImageCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        {
            return vk_print_vkimagememorybarrier((VkImageMemoryBarrier*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            return vk_print_vkimageviewcreateinfo((VkImageViewCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO:
        {
            return vk_print_vkinstancecreateinfo((VkInstanceCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE:
        {
            return vk_print_vkmappedmemoryrange((VkMappedMemoryRange*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO:
        {
            return vk_print_vkmemoryallocateinfo((VkMemoryAllocateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
        {
            return vk_print_vkmemorybarrier((VkMemoryBarrier*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO:
        {
            return vk_print_vkpipelinecachecreateinfo((VkPipelineCacheCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinecolorblendstatecreateinfo((VkPipelineColorBlendStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinedepthstencilstatecreateinfo((VkPipelineDepthStencilStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinedynamicstatecreateinfo((VkPipelineDynamicStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelineinputassemblystatecreateinfo((VkPipelineInputAssemblyStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO:
        {
            return vk_print_vkpipelinelayoutcreateinfo((VkPipelineLayoutCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinemultisamplestatecreateinfo((VkPipelineMultisampleStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinerasterizationstatecreateinfo((VkPipelineRasterizationStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            return vk_print_vkpipelineshaderstagecreateinfo((VkPipelineShaderStageCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinetessellationstatecreateinfo((VkPipelineTessellationStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelinevertexinputstatecreateinfo((VkPipelineVertexInputStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO:
        {
            return vk_print_vkpipelineviewportstatecreateinfo((VkPipelineViewportStateCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            return vk_print_vkquerypoolcreateinfo((VkQueryPoolCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO:
        {
            return vk_print_vkrenderpassbegininfo((VkRenderPassBeginInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        {
            return vk_print_vkrenderpasscreateinfo((VkRenderPassCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            return vk_print_vksamplercreateinfo((VkSamplerCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
        {
            return vk_print_vksemaphorecreateinfo((VkSemaphoreCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:
        {
            return vk_print_vkshadermodulecreateinfo((VkShaderModuleCreateInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_SUBMIT_INFO:
        {
            return vk_print_vksubmitinfo((VkSubmitInfo*)pStruct, indent);
        }
        break;
        case VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET:
        {
            return vk_print_vkwritedescriptorset((VkWriteDescriptorSet*)pStruct, indent);
        }
        break;
        default:
        return NULL;
    }
}